/*
 * MP3 decoding support using libmad:  Adapted from the SoX library at
 * http://sourceforge.net/projects/sox/, LGPLv2, Copyright (c) 2007-2009
 * SoX contributors, written by Fabrizio Gennari <fabrizio.ge@tiscali.it>,
 * with the decoding part based on the decoder tutorial program madlld
 * written by Bertrand Petit <madlld@phoe.fmug.org> (BSD license, see at
 * http://www.bsd-dk.dk/~elrond/audio/madlld/).  The tag identification
 * functions were adapted from the GPL-licensed libid3tag library, see at
 * http://www.underbit.com/products/mad/.  Adapted to Quake and Hexen II
 * game engines by O.Sezer :
 * Copyright (C) 2010-2011 O.Sezer <sezero@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "quakedef.h"

#if defined(USE_CODEC_MP3)
#include "snd_codec.h"
#include "snd_codeci.h"
#include "snd_mp3.h"
#include <mad.h>

#define ID3_TAG_FLAG_FOOTERPRESENT 0x10

/* Under Windows, importing data from DLLs is a dicey proposition. This is true
 * when using dlopen, but also true if linking directly against the DLL if the
 * header does not mark the data as __declspec(dllexport), which mad.h does not.
 * Sidestep the issue by defining our own mad_timer_zero. This is needed because
 * mad_timer_zero is used in some of the mad.h macros.
 */
#define mad_timer_zero mad_timer_zero_stub
static mad_timer_t const mad_timer_zero_stub = {0, 0};

/* MAD returns values with MAD_F_FRACBITS (28) bits of precision, though it's
   not certain that all of them are meaningful. Default to 16 bits to
   align with most users expectation of output file should be 16 bits. */
#define MP3_MAD_SAMPLEBITS	16
#define MP3_MAD_SAMPLEWIDTH	(MP3_MAD_SAMPLEBITS / 8)
#define MP3_BUFFER_SIZE		(5 * 8192)

/* Private data */
typedef struct _mp3_priv_t
{
	unsigned char mp3_buffer[MP3_BUFFER_SIZE];
	struct mad_stream	Stream;
	struct mad_frame	Frame;
	struct mad_synth	Synth;
	mad_timer_t		Timer;
	ptrdiff_t		cursamp;
	size_t			FrameCount;
} mp3_priv_t;

/* This function merges the functions tagtype() and id3_tag_query()
 * from MAD's libid3tag, so we don't have to link to it
 * Returns 0 if the frame is not an ID3 tag, tag length if it is */
static qboolean tag_is_id3v1(const unsigned char *data, size_t length)
{
	if (length >= 3 &&
	     data[0] == 'T' && data[1] == 'A' && data[2] == 'G')
	{
		return true;
	}
	return false;
}

static qboolean tag_is_id3v2(const unsigned char *data, size_t length)
{
	if (length >= 10 &&
	    (data[0] == 'I' && data[1] == 'D' && data[2] == '3') &&
	    data[3] < 0xff && data[4] < 0xff &&
	    data[6] < 0x80 && data[7] < 0x80 && data[8] < 0x80 && data[9] < 0x80)
	{
		return true;
	}
	return false;
}

static int mp3_tagsize(const unsigned char *data, size_t length)
{
	if (tag_is_id3v1(data, length))
		return 128;

	if (tag_is_id3v2(data, length))
	{
		unsigned char flags;
		unsigned int size;
		flags = data[5];
		size = 10 + (data[6]<<21) + (data[7]<<14) + (data[8]<<7) + data[9];
		if (flags & ID3_TAG_FLAG_FOOTERPRESENT)
			size += 10;
		for ( ; size < length && !data[size]; ++size)
			;  /* Consume padding */
		return size;
	}

	return 0;
}

/* Attempts to read an ID3 tag at the current location in stream and
 * consume it all.  Returns -1 if no tag is found.  Its up to caller
 * to recover.  */
static int mp3_inputtag(snd_stream_t *stream)
{
	mp3_priv_t *p = (mp3_priv_t *) stream->priv;
	int rc = -1;
	size_t remaining;
	size_t tagsize;

	/* FIXME: This needs some more work if we are to ever
	 * look at the ID3 frame.  This is because the Stream
	 * may not be able to hold the complete ID3 frame.
	 * We should consume the whole frame inside tagtype()
	 * instead of outside of tagframe().  That would support
	 * recovering when Stream contains less then 8-bytes (header)
	 * and also when ID3v2 is bigger then Stream buffer size.
	 * Need to pass in stream so that buffer can be
	 * consumed as well as letting additional data to be
	 * read in.
	 */
	remaining = p->Stream.bufend - p->Stream.next_frame;
	tagsize = mp3_tagsize(p->Stream.this_frame, remaining);
	if (tagsize != 0)
	{
		mad_stream_skip(&p->Stream, tagsize);
		rc = 0;
	}

	/* We know that a valid frame hasn't been found yet
	 * so help libmad out and go back into frame seek mode.
	 * This is true whether an ID3 tag was found or not.
	 */
	mad_stream_sync(&p->Stream);

	return rc;
}

/* (Re)fill the stream buffer that is to be decoded.  If any data
 * still exists in the buffer then they are first shifted to be
 * front of the stream buffer.  */
static int mp3_inputdata(snd_stream_t *stream)
{
	mp3_priv_t *p = (mp3_priv_t *) stream->priv;
	size_t bytes_read;
	size_t remaining;

	remaining = p->Stream.bufend - p->Stream.next_frame;

	/* libmad does not consume all the buffer it's given. Some
	 * data, part of a truncated frame, is left unused at the
	 * end of the buffer. That data must be put back at the
	 * beginning of the buffer and taken in account for
	 * refilling the buffer. This means that the input buffer
	 * must be large enough to hold a complete frame at the
	 * highest observable bit-rate (currently 448 kb/s).
	 * TODO: Is 2016 bytes the size of the largest frame?
	 * (448000*(1152/32000))/8
	 */
	memmove(p->mp3_buffer, p->Stream.next_frame, remaining);

	bytes_read = FS_fread(p->mp3_buffer + remaining, 1,
				MP3_BUFFER_SIZE - remaining, &stream->fh);
	if (bytes_read == 0)
	{
		return -1;
	}

	mad_stream_buffer(&p->Stream, p->mp3_buffer, bytes_read+remaining);
	p->Stream.error = MAD_ERROR_NONE;

	return 0;
}

static int mp3_startread(snd_stream_t *stream)
{
	mp3_priv_t *p = (mp3_priv_t *) stream->priv;
	size_t ReadSize;

	mad_stream_init(&p->Stream);
	mad_frame_init(&p->Frame);
	mad_synth_init(&p->Synth);
	mad_timer_reset(&p->Timer);

	/* Decode at least one valid frame to find out the input
	 * format.  The decoded frame will be saved off so that it
	 * can be processed later.
	 */
	ReadSize = FS_fread(p->mp3_buffer, 1, MP3_BUFFER_SIZE, &stream->fh);
	if (ReadSize != MP3_BUFFER_SIZE)
	{
		if (FS_feof(&stream->fh) || FS_ferror(&stream->fh))
			return -1;
	}

	mad_stream_buffer(&p->Stream, p->mp3_buffer, ReadSize);

	/* Find a valid frame before starting up.  This makes sure
	 * that we have a valid MP3 and also skips past ID3v2 tags
	 * at the beginning of the audio file.
	 */
	p->Stream.error = MAD_ERROR_NONE;
	while (mad_frame_decode(&p->Frame,&p->Stream))
	{
		/* check whether input buffer needs a refill */
		if (p->Stream.error == MAD_ERROR_BUFLEN)
		{
			if (mp3_inputdata(stream) == -1)
				return -1;

			continue;
		}

		/* Consume any ID3 tags */
		mp3_inputtag(stream);

		/* FIXME: We should probably detect when we've read
		 * a bunch of non-ID3 data and still haven't found a
		 * frame.  In that case we can abort early without
		 * scanning the whole file.
		 */
		p->Stream.error = MAD_ERROR_NONE;
	}

	if (p->Stream.error)
	{
		Con_Printf("MP3: No valid MP3 frame found\n");
		return -1;
	}

	switch(p->Frame.header.mode)
	{
	case MAD_MODE_SINGLE_CHANNEL:
	case MAD_MODE_DUAL_CHANNEL:
	case MAD_MODE_JOINT_STEREO:
	case MAD_MODE_STEREO:
		stream->info.channels = MAD_NCHANNELS(&p->Frame.header);
		break;
	default:
		Con_Printf("MP3: Cannot determine number of channels\n");
		return -1;
	}

	p->FrameCount = 1;

	mad_timer_add(&p->Timer,p->Frame.header.duration);
	mad_synth_frame(&p->Synth,&p->Frame);
	stream->info.width = MP3_MAD_SAMPLEWIDTH;
	stream->info.rate = p->Synth.pcm.samplerate;

	p->cursamp = 0;

	return 0;
}

/* Read up to len samples from p->Synth
 * If needed, read some more MP3 data, decode them and synth them
 * Place in buf[].
 * Return number of samples read.  */
static int mp3_decode(snd_stream_t *stream, byte *buf, int len)
{
	mp3_priv_t *p = (mp3_priv_t *) stream->priv;
	int donow, i, done = 0;
	mad_fixed_t sample;
	int chan, x;

	do
	{
		x = (p->Synth.pcm.length - p->cursamp) * stream->info.channels;
		donow = q_min(len, x);
		i = 0;
		while (i < donow)
		{
			for (chan = 0; chan < stream->info.channels; chan++)
			{
				sample = p->Synth.pcm.samples[chan][p->cursamp];
				/* convert from fixed to short,
				 * write in host-endian format. */
				if (sample <= -MAD_F_ONE)
					sample = -0x7FFF;
				else if (sample >= MAD_F_ONE)
					sample = 0x7FFF;
				else
					sample >>= (MAD_F_FRACBITS + 1 - 16);
				if (bigendien)
				{
					*buf++ = (sample >> 8) & 0xFF;
					*buf++ = sample & 0xFF;
				}
				else
				{ /* LITTLE_ENDIAN */
					*buf++ = sample & 0xFF;
					*buf++ = (sample >> 8) & 0xFF;
				}
				i++;
			}
			p->cursamp++;
		};

		len -= donow;
		done += donow;

		if (len == 0)
			break;

		/* check whether input buffer needs a refill */
		if (p->Stream.error == MAD_ERROR_BUFLEN)
		{
			if (mp3_inputdata(stream) == -1)
			{
				/* check feof() ?? */
				Con_DPrintf("mp3 EOF\n");
				break;
			}
		}

		if (mad_frame_decode(&p->Frame, &p->Stream))
		{
			if (MAD_RECOVERABLE(p->Stream.error))
			{
				mp3_inputtag(stream);
				continue;
			}
			else
			{
				if (p->Stream.error == MAD_ERROR_BUFLEN)
					continue;
				else
				{
					Con_Printf("MP3: unrecoverable frame level error (%s)\n",
							mad_stream_errorstr(&p->Stream));
					break;
				}
			}
		}
		p->FrameCount++;
		mad_timer_add(&p->Timer, p->Frame.header.duration);
		mad_synth_frame(&p->Synth, &p->Frame);
		p->cursamp = 0;
	} while (1);

	return done;
}

static int mp3_stopread(snd_stream_t *stream)
{
	mp3_priv_t *p = (mp3_priv_t*) stream->priv;

	mad_synth_finish(&p->Synth);
	mad_frame_finish(&p->Frame);
	mad_stream_finish(&p->Stream);

	return 0;
}

static int mp3_madseek(snd_stream_t *stream, unsigned long offset)
{
	mp3_priv_t *p = (mp3_priv_t *) stream->priv;
	size_t   initial_bitrate = p->Frame.header.bitrate;
	size_t   tagsize = 0, consumed = 0;
	int vbr = 0;		/* Variable Bit Rate, bool */
	qboolean depadded = false;
	unsigned long to_skip_samples = 0;

	/* Reset all */
	FS_rewind(&stream->fh);
	mad_timer_reset(&p->Timer);
	p->FrameCount = 0;

	/* They where opened in startread */
	mad_synth_finish(&p->Synth);
	mad_frame_finish(&p->Frame);
	mad_stream_finish(&p->Stream);

	mad_stream_init(&p->Stream);
	mad_frame_init(&p->Frame);
	mad_synth_init(&p->Synth);

	offset /= stream->info.channels;
	to_skip_samples = offset;

	while (1)	/* Read data from the MP3 file */
	{
		int bytes_read, padding = 0;
		size_t leftover = p->Stream.bufend - p->Stream.next_frame;

		memcpy(p->mp3_buffer, p->Stream.this_frame, leftover);
		bytes_read = FS_fread(p->mp3_buffer + leftover, (size_t) 1,
					MP3_BUFFER_SIZE - leftover, &stream->fh);
		if (bytes_read <= 0)
		{
			Con_DPrintf("seek failure. unexpected EOF (frames=%lu leftover=%lu)\n",
					(unsigned long)p->FrameCount, (unsigned long)leftover);
			break;
		}
		for ( ; !depadded && padding < bytes_read && !p->mp3_buffer[padding]; ++padding)
			;
		depadded = true;
		mad_stream_buffer(&p->Stream, p->mp3_buffer + padding, leftover + bytes_read - padding);

		while (to_skip_samples >= 0)	/* Decode frame headers */
		{
			static unsigned short samples;
			p->Stream.error = MAD_ERROR_NONE;

			/* Not an audio frame */
			if (mad_header_decode(&p->Frame.header, &p->Stream) == -1)
			{
				if (p->Stream.error == MAD_ERROR_BUFLEN)
					break;	/* Normal behaviour; get some more data from the file */
				if (!MAD_RECOVERABLE(p->Stream.error))
				{
					Con_DPrintf("unrecoverable MAD error\n");
					break;
				}
				if (p->Stream.error == MAD_ERROR_LOSTSYNC)
				{
					unsigned long available = (p->Stream.bufend - p->Stream.this_frame);
					tagsize = mp3_tagsize(p->Stream.this_frame, (size_t) available);
					if (tagsize)
					{	/* It's some ID3 tags, so just skip */
						if (tagsize >= available)
						{
							FS_fseek(&stream->fh, (long)(tagsize - available), SEEK_CUR);
							depadded = false;
						}
						mad_stream_skip(&p->Stream, q_min(tagsize, available));
					}
					else
					{
						Con_DPrintf("MAD lost sync\n");
					}
				}
				else
				{
					Con_DPrintf("recoverable MAD error\n");
				}
				continue;
			}

			consumed +=  p->Stream.next_frame - p->Stream.this_frame;
			vbr      |= (p->Frame.header.bitrate != initial_bitrate);

			samples = 32 * MAD_NSBSAMPLES(&p->Frame.header);

			p->FrameCount++;
			mad_timer_add(&p->Timer, p->Frame.header.duration);

			if (to_skip_samples <= samples)
			{
				mad_frame_decode(&p->Frame,&p->Stream);
				mad_synth_frame(&p->Synth, &p->Frame);
				p->cursamp = to_skip_samples;
				return 0;
			}
			else	to_skip_samples -= samples;

			/* If not VBR, we can extrapolate frame size */
			if (p->FrameCount == 64 && !vbr)
			{
				p->FrameCount = offset / samples;
				to_skip_samples = offset % samples;

				if (0 != FS_fseek(&stream->fh, (p->FrameCount * consumed / 64) + tagsize, SEEK_SET))
					return -1;

				/* Reset Stream for refilling buffer */
				mad_stream_finish(&p->Stream);
				mad_stream_init(&p->Stream);
				break;
			}
		}
	};

	return -1;
}

static qboolean S_MP3_CodecInitialize (void)
{
	return true;
}

static void S_MP3_CodecShutdown (void)
{
}

static snd_stream_t *S_MP3_CodecOpenStream (const char *filename)
{
	snd_stream_t *stream;

	stream = S_CodecUtilOpen(filename, &mp3_codec);
	if (!stream)
		return NULL;

	stream->priv = Z_Malloc(sizeof(mp3_priv_t));

	if (mp3_startread(stream) < 0)
	{
		Con_Printf("%s is not a valid MP3 file.\n", filename);
		Z_Free(stream->priv);
		S_CodecUtilClose(&stream);
		return NULL;
	}

	return stream;
}

static int S_MP3_CodecReadStream (snd_stream_t *stream, int bytes, void *buffer)
{
	int res = mp3_decode(stream, (byte *)buffer, bytes / stream->info.width);
	return res * stream->info.width;
}

static void S_MP3_CodecCloseStream (snd_stream_t *stream)
{
	mp3_stopread(stream);
	Z_Free(stream->priv);
	S_CodecUtilClose(&stream);
}

static int S_MP3_CodecRewindStream (snd_stream_t *stream)
{
#if 0
	mp3_stopread(stream);
	FS_rewind(&stream->fh);
	return mp3_startread(stream);
#else
	return mp3_madseek(stream, 0);
#endif
}

snd_codec_t mp3_codec =
{
	CODECTYPE_MP3,
	true,	/* always available. */
	".mp3",
	S_MP3_CodecInitialize,
	S_MP3_CodecShutdown,
	S_MP3_CodecOpenStream,
	S_MP3_CodecReadStream,
	S_MP3_CodecRewindStream,
	S_MP3_CodecCloseStream,
	NULL
};

#endif	/* USE_CODEC_MP3 */


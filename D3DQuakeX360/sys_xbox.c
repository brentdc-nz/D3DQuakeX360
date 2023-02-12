/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2005 John Fitzgibbons and others
Copyright (C) 2007-2008 Kristian Duske

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "quakedef.h"

#include <sys/types.h>
#include <errno.h>
#include <io.h>
#include <direct.h>

#include "xbox\xb_input.h"

qboolean	isDedicated;
cvar_t		sys_throttle = {"sys_throttle", "0.02", true};

static HANDLE		hinput, houtput;
#define	MAX_HANDLES		32	/* johnfitz -- was 10 */
static FILE		*sys_handles[MAX_HANDLES];

static int findhandle (void)
{
	int i;

	for (i = 1; i < MAX_HANDLES; i++)
	{
		if (!sys_handles[i])
			return i;
	}

	Sys_Error ("out of handles");

	return -1;
}

double Sys_FloatTime (void)
{
	return GetTickCount() / 1000.0;
}

void Sys_Init (void)
{
}

void Sys_Printf (const char *fmt, ...)
{
	va_list		argptr;
	char		text[1024];

	va_start (argptr,fmt);
	q_vsnprintf (text, sizeof(text), fmt, argptr);
	va_end (argptr);

	if (isDedicated)
	{
//		WriteFile(houtput, text, strlen(text), &dummy, NULL); // TODO
	}
	else
	{
		OutputDebugString(text);
	}
}

long Sys_filelength (FILE *f)
{
	long		pos, end;

	pos = ftell (f);
	fseek (f, 0, SEEK_END);
	end = ftell (f);
	fseek (f, pos, SEEK_SET);

	return end;
}

int Sys_FileOpenRead (const char *path, int *hndl)
{
	FILE	*f;
	int	i, retval;

	i = findhandle ();
	f = fopen(path, "rb");

	if (!f)
	{
		*hndl = -1;
		retval = -1;
	}
	else
	{
		sys_handles[i] = f;
		*hndl = i;
		retval = Sys_filelength(f);
	}

	return retval;
}

int Sys_FileRead (int handle, void *dest, int count)
{
	return fread (dest, 1, count, sys_handles[handle]);
}

void Sys_FileSeek (int handle, int position)
{
	fseek (sys_handles[handle], position, SEEK_SET);
}

void Sys_FileClose (int handle)
{
	fclose (sys_handles[handle]);
	sys_handles[handle] = NULL;
}

void swap_characters(const char *str, char find, char replace)
{
	int i;
    char temp;
    int length = strlen(str);
    char *non_const_str = (char *) str;

    for (i = 0; i < length; i++)
    {
        if (non_const_str[i] == find)
        {
            temp = non_const_str[i];
            non_const_str[i] = replace;
            replace = temp;
        }
    }
}

int	Sys_FileTime (const char *path)
{
	FILE	*f;
	int		retval;

#ifdef _XBOX
	swap_characters(path, '/', '\\'); // FIXME: Better way to go 3 swaps deep!
	swap_characters(path, '/', '\\');
	swap_characters(path, '/', '\\');
#endif

	f = fopen(path, "rb");

	if (f)
	{
		fclose(f);
		retval = 1;
	}
	else
	{
		retval = -1;
	}
	
	return retval;
}

char *Sys_ConsoleInput (void)
{
	return NULL; // TODO : Virtual keybaord?
}

int Sys_FileWrite (int handle, const void *data, int count)
{
	return fwrite (data, 1, count, sys_handles[handle]);
}

int Sys_FileOpenWrite (const char *path)
{
	FILE	*f;
	int		i;

	i = findhandle ();
	f = fopen(path, "wb");

	if (!f)
		Sys_Error ("Error opening %s: %s", path, strerror(errno));

	sys_handles[i] = f;
	return i;
}

void Sys_mkdir (const char *path)
{
	int rc = _mkdir (path);
	if (rc != 0 && errno != EEXIST)
		Sys_Error("Unable to create directory %s", path);
}

void Sys_Quit (void)
{
	Host_Shutdown();

	// Return to dashboard
	XLaunchNewImage("", 0);	
}

void Sys_Sleep (void)
{
	Sleep(1);
}

void Sys_SendKeyEvents (void)
{
	static int clear = 0;

	WORD Rslt = GamepadUpdate();

	if(Rslt > 0)
	{
		Key_Event(Rslt, true);
		clear = 1;
	}

	if(clear && Rslt == 0)
	{
		Key_Event(K_XBOX_START, false);
		Key_Event(K_XBOX_BACK, false);

		Key_Event(K_XBOX_A, false);
		Key_Event(K_XBOX_B, false);
		Key_Event(K_XBOX_X, false);
		Key_Event(K_XBOX_Y, false);

		Key_Event(K_XBOX_UP, false);
		Key_Event(K_XBOX_DOWN, false);
		Key_Event(K_XBOX_LEFT, false);
		Key_Event(K_XBOX_RIGHT, false);

		Key_Event(K_XBOX_LTRIG, false);
		Key_Event(K_XBOX_RTRIG, false);

		clear = 0;
	}
}
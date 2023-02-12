/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2009 John Fitzgibbons and others

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

// gl_vidnt.c -- NT GL vid component

#include "quakedef.h"

#define MAX_MODE_LIST	600 // johnfitz -- was 30
#define VID_ROW_SIZE	3
#define WARP_WIDTH		320
#define WARP_HEIGHT		200
#define MAXWIDTH		10000
#define MAXHEIGHT		10000
#define BASEWIDTH		320
#define BASEHEIGHT		200

#define MODE_WINDOWED			0
#define NO_MODE					(MODE_WINDOWED - 1)
#define MODE_FULLSCREEN_DEFAULT	(MODE_WINDOWED + 1)

typedef struct {
	modestate_t	type;
	int			width;
	int			height;
	int			modenum;
	int			dib;
	int			fullscreen;
	int			bpp;
	int			refreshrate; //johnfitz
	int			halfscreen;
	char		modedesc[17];
} vmode_t;

typedef struct {
	int			width;
	int			height;
} lmode_t;

lmode_t	lowresmodes[] = {
	{320, 200},
	{320, 240},
	{400, 300},
	{512, 384},
};

const char *gl_vendor;
const char *gl_renderer;
const char *gl_version;
const char *gl_extensions;
const char *wgl_extensions; //johnfitz

qboolean DDActive;
qboolean scr_skipupdate;

RECT		WindowRect;
viddef_t	vid;				// global video state

byte	gammatable[256];

void GL_Init (void);
void VID_BuildGammaTable(float fGamma);
void VID_CheckGamma (void);
#if 1//def _FAKEGLX360
void XDKSetGamma(const unsigned char* gammaTable);
#endif

qboolean isPermedia = false;
qboolean isIntelVideo = false; //johnfitz -- intel video workarounds from Baker
qboolean gl_mtexable = false;
qboolean gl_texture_env_combine = false; //johnfitz
qboolean gl_texture_env_add = false; //johnfitz
qboolean gl_swap_control = false; //johnfitz
qboolean gl_anisotropy_able = false; //johnfitz
float gl_max_anisotropy; //johnfitz

int gl_stencilbits; //johnfitz

qboolean vid_locked = false; //johnfitz

void GL_SetupState (void); //johnfitz

//====================================

//johnfitz -- new cvars
#ifndef _XBOX
cvar_t		vid_fullscreen = {"vid_fullscreen", "1", true};
#endif
cvar_t		vid_width = {"vid_width", "640", true};
cvar_t		vid_height = {"vid_height", "480", true};
cvar_t		vid_bpp = {"vid_bpp", "16", true};
cvar_t		vid_refreshrate = {"vid_refreshrate", "60", true};
cvar_t		vid_vsync = {"vid_vsync", "0", true};
//johnfitz
cvar_t		vid_gamma = {"gamma", "0.57", true};

/*
===============
GL_SetupState -- johnfitz

does all the stuff from GL_Init that needs to be done every time a new GL render context is created
GL_Init will still do the stuff that only needs to be done once
===============
*/
void GL_SetupState (void)
{
	glClearColor (0.15,0.15,0.15,0); //johnfitz -- originally 1,0,0,0
	glCullFace(GL_BACK); //johnfitz -- glquake used CCW with backwards culling -- let's do it right
	glFrontFace(GL_CW); //johnfitz -- glquake used CCW with backwards culling -- let's do it right
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.666);
	glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	glShadeModel (GL_FLAT);
//	glHint (GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST); //johnfitz
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glDepthRange (0, 0.49999); //johnfitz -- moved here becuase gl_ztrick is gone.
	glDepthFunc (GL_LEQUAL); //johnfitz -- moved here becuase gl_ztrick is gone.
}

/*
=================
GL_BeginRendering -- sets values of glx, gly, glwidth, glheight
=================
*/
void GL_BeginRendering (int *x, int *y, int *width, int *height)
{
	*x = *y = 0;

	// TEST: Need todo properly
	*width = 1280;//WindowRect.right - WindowRect.left;
	*height = 720;//WindowRect.bottom - WindowRect.top;

	VID_CheckGamma();
}

/*
=================
GL_EndRendering
=================
*/
void GL_EndRendering (void)
{
	if (!scr_skipupdate || block_drawing)
#ifdef _XBOX
		XDKGlDisplay();
#else
		SwapBuffers(maindc);
#endif
}

/*
================
VID_Gamma_f -- callback when the cvar changes
================
*/
void VID_CheckGamma (void)
{
	static float oldgamma;
	
	if (vid_gamma.value == oldgamma)
		return;
	
	oldgamma = vid_gamma.value;

	VID_BuildGammaTable(vid_gamma.value);

	XDKSetGamma(gammatable);
}

//==========================================================================
//
//  INIT
//
//==========================================================================

/*
===================
VID_Init
===================
*/
void VID_Init (void)
{

	Cvar_RegisterVariable (&vid_width, NULL); //johnfitz
	Cvar_RegisterVariable (&vid_height, NULL); //johnfitz
	Cvar_RegisterVariable (&vid_bpp, NULL); //johnfitz
	Cvar_RegisterVariable (&vid_refreshrate, NULL); //johnfitz
	Cvar_RegisterVariable (&vid_gamma, NULL);

	Con_Printf("------------- Init Video -------------\n");
	Con_Printf("%cVideo Init\n", 2);

//	memset(&devmode, 0, sizeof(devmode));

	GL_Init ();

	// Build before setting
	VID_BuildGammaTable(1.0);
}

/*
===============
VID_BuildGammaTable
===============
*/
void VID_BuildGammaTable(float fGamma)
{
	int	i, inf;
	
	if (fGamma == 1.0)
	{
		for (i=0 ; i<256 ; i++)
			gammatable[i] = i;

		return;
	}
	
	for (i=0 ; i<256 ; i++)
	{
		inf = 255 * pow ( (i+0.5)/255.5 , fGamma ) + 0.5;

		if (inf < 0)
			inf = 0;

		if (inf > 255)
			inf = 255;

		gammatable[i] = inf;
	}
}

/*
===============
GL_Init
===============
*/
void GL_Init (void)
{
#ifdef _XBOX
	XDKGlInit();
	
	//johnfitz -- stuff
	vid.width = 1280;
	vid.height = 720;
	vid.conwidth = vid.width & 0xFFFFFFF8;
	vid.conheight = vid.conwidth * vid.height / vid.width;
	//johnfitz	
	
	vid.colormap = host_colormap;
#endif

	GL_SetupState (); //johnfitz
}
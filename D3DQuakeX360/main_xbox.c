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
#include <stdio.h>

#define DEFAULT_MEMORY 0x4000000

int main(/*int argc, char *argv[]*/)
{
#ifdef SDL_EVENTS
	SDL_Event	event;
#endif	
	quakeparms_t	parms;
	int		t;
	int		done = 0;
	double		time, oldtime, newtime;

#ifdef _XBOX
	parms.basedir = "D:";
#else
	parms.basedir = ".";
#endif

#ifdef _XBOX
	parms.argc = 0;
	parms.argv = 0;
#else
	parms.argc = argc;
	parms.argv = argv;
#endif

	COM_InitArgv(parms.argc, parms.argv);

	isDedicated = (COM_CheckParm("-dedicated") != 0);

	Sys_Init();

	// default memory size
	// TODO: less mem, eg. 16 mb, for dedicated server??
	parms.memsize = DEFAULT_MEMORY;

	if (COM_CheckParm("-heapsize"))
	{
		t = COM_CheckParm("-heapsize") + 1;
		if (t < com_argc)
			parms.memsize = Q_atoi(com_argv[t]) * 1024;
	}

	parms.membase = malloc (parms.memsize);

	if (!parms.membase)
		Sys_Error ("Not enough memory free; check disk space\n");

	Sys_Printf("Quake %1.2f (c) id Software\n", VERSION);
	Sys_Printf("GLQuake %1.2f (c) id Software\n", GLQUAKE_VERSION);
	Sys_Printf("FitzQuake %1.2f (c) John Fitzgibbons\n", FITZQUAKE_VERSION);
	Sys_Printf("FitzQuake SDL port (c) SleepwalkR, Baker\n");
	Sys_Printf("QuakeSpasm %1.2f.%d (c) Ozkan Sezer, Stevenaaus\n",
					FITZQUAKE_VERSION, QUAKESPASM_VER_PATCH);

	Sys_Printf("Host_Init\n");
	Host_Init(&parms);

	oldtime = Sys_FloatTime();
	if (isDedicated)
	{
		while (1)
		{
			newtime = Sys_FloatTime ();
			time = newtime - oldtime;

			while (time < sys_ticrate.value )
			{
				Sleep(1);
				newtime = Sys_FloatTime ();
				time = newtime - oldtime;
			}

			Host_Frame (time);
			oldtime = newtime;
		}
	}
	else
	while (!done)
	{
#ifdef SDL_EVENTS		
		while (!done && SDL_PollEvent (&event))
		{
			switch (event.type)
			{
			case SDL_ACTIVEEVENT:
				if (event.active.state & (SDL_APPACTIVE|SDL_APPINPUTFOCUS))
				{
					if (!COM_CheckParm("-bgsound")) {
						if (event.active.gain)
							S_UnblockSound();
						else 
							S_BlockSound();
					}
				}
				break;
			case SDL_MOUSEMOTION:
				IN_MouseMove(event.motion.xrel, event.motion.yrel);
				break;
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
			  switch (event.button.button)
			  {
			  case SDL_BUTTON_LEFT:
				Key_Event(K_MOUSE1, event.button.type == SDL_MOUSEBUTTONDOWN);
				break;
			  case SDL_BUTTON_RIGHT:
				Key_Event(K_MOUSE2, event.button.type == SDL_MOUSEBUTTONDOWN);
				break;
			  case SDL_BUTTON_MIDDLE:
				Key_Event(K_MOUSE3, event.button.type == SDL_MOUSEBUTTONDOWN);
				break;
			  case SDL_BUTTON_WHEELUP:
				Key_Event(K_MWHEELUP, event.button.type == SDL_MOUSEBUTTONDOWN);
				break;
			  case SDL_BUTTON_WHEELDOWN:
				Key_Event(K_MWHEELDOWN, event.button.type == SDL_MOUSEBUTTONDOWN);
				break;
			  case SDL_BUTTON_X1:
				Key_Event(K_MOUSE4, event.button.type == SDL_MOUSEBUTTONDOWN);
				break;
			  case SDL_BUTTON_X2:
				Key_Event(K_MOUSE5, event.button.type == SDL_MOUSEBUTTONDOWN);
				break;
			  }
			  break;
			case SDL_KEYDOWN:
			case SDL_KEYUP:
				// SHIFT + ESC and circomflex always opens the console no matter what
				if ((event.key.keysym.sym == SDLK_ESCAPE && (event.key.keysym.mod & KMOD_SHIFT))
				    || (event.key.keysym.sym == SDLK_CARET))
				{
					if (event.key.type == SDL_KEYDOWN)
						Con_ToggleConsole_f();
				}
				else if ((event.key.keysym.sym == SDLK_RETURN) &&
					 (event.key.keysym.mod & KMOD_ALT))
				{
					if (event.key.type == SDL_KEYDOWN)
						VID_Toggle();
				}
				else
				{
					Key_Event(Key_Map(&(event.key)), event.key.type == SDL_KEYDOWN);
				}
				break;
			case SDL_QUIT:
				done = 1;
				break;
			default:
				break;
			}
		}
#endif //SDL_EVENTS		

		newtime = Sys_FloatTime();
		time = newtime - oldtime;
		Host_Frame(time);

		// Throttle the game loop just a little bit
		// and make the game run a little cooler:
		// noone needs more than 1000fps, I think
		if (time < sys_throttle.value)
			Sleep(1);

		oldtime = newtime;
	}

	Sys_Quit();
	return 0;
}

void Sys_Error (const char *error, ...)
{
	// TODO
}
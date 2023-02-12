/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2009 John Fitzgibbons and others
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

#ifndef _QUAKE_INPUT_H
#define _QUAKE_INPUT_H

// input.h -- external (non-keyboard) input devices

void IN_Init (void);

void IN_Shutdown (void);

void IN_Commands (void);
// oportunity for devices to stick commands on the script buffer

// mouse moved by dx and dy pixels
void IN_MouseMove(int dx, int dy);

void IN_Move (usercmd_t *cmd);
// add additional movement on top of the keyboard move cmd

void IN_ClearStates (void);
// restores all button and position states to defaults

#ifndef _XBOX
// called when the app becomes active
void IN_Activate ();
#endif

#ifndef _XBOX
// called when the app becomes inactive
void IN_Deactivate (qboolean free_cursor);
#endif

/* for SDL builds only */
void BuildKeyMaps (void);
int Key_Map (void *event); /* SDL_KeyboardEvent *event */

#endif	/* _QUAKE_INPUT_H */
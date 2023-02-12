/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2012 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

/**
 *  \file SDL.h
 *  
 *  Main include header for the SDL library
 */

/**
 *  \mainpage Simple DirectMedia Layer (SDL)
 *  
 *  http://www.libsdl.org/
 *  
 *  \section intro_sec Introduction
 *  
 *  This is the Simple DirectMedia Layer, a general API that provides low
 *  level access to audio, keyboard, mouse, joystick, 3D hardware via OpenGL,
 *  and 2D framebuffer across multiple platforms.
 *  
 *  SDL is written in C, but works with C++ natively, and has bindings to
 *  several other languages, including Ada, C#, Eiffel, Erlang, Euphoria,
 *  Guile, Haskell, Java, Lisp, Lua, ML, Objective C, Pascal, Perl, PHP,
 *  Pike, Pliant, Python, Ruby, and Smalltalk.
 *  
 *  This library is distributed under GNU LGPL version 2, which can be
 *  found in the file  "COPYING".  This license allows you to use SDL
 *  freely in commercial programs as long as you link with the dynamic
 *  library.
 *  
 *  The best way to learn how to use SDL is to check out the header files in
 *  the "include" subdirectory and the programs in the "test" subdirectory.
 *  The header files and test programs are well commented and always up to date.
 *  More documentation is available in HTML format in "docs/index.html", and
 *  a documentation wiki is available online at:
 *  	http://www.libsdl.org/cgi/docwiki.cgi
 *  
 *  The test programs in the "test" subdirectory are in the public domain.
 *  
 *  Frequently asked questions are answered online:
 *  	http://www.libsdl.org/faq.php
 *  
 *  If you need help with the library, or just want to discuss SDL related
 *  issues, you can join the developers mailing list:
 *  	http://www.libsdl.org/mailing-list.php
 *  
 *  Enjoy!
 *  	Sam Lantinga				(slouken@libsdl.org)
 */

#ifndef _SDL_H
#define _SDL_H

#include <xtl.h>
#include "SDL_stdinc.h"
#include "SDL_types.h"
#include "SDL_audio.h"
#include "SDL_sysaudio.h"
#if 0
#include "SDL_main.h"
#include "SDL_assert.h"
#include "SDL_atomic.h"
#include "SDL_audio.h"
#include "SDL_clipboard.h"
#include "SDL_cpuinfo.h"
#include "SDL_endian.h"
#include "SDL_error.h"
#include "SDL_events.h"
#include "SDL_gamecontroller.h"
#include "SDL_hints.h"
#include "SDL_loadso.h"
#include "SDL_log.h"
#include "SDL_messagebox.h"
#include "SDL_mutex.h"
#include "SDL_power.h"
#include "SDL_render.h"
#include "SDL_rwops.h"
#include "SDL_system.h"
#include "SDL_thread.h"
#include "SDL_timer.h"
#include "SDL_version.h"
#include "SDL_video.h"
#endif

#endif /* _SDL_H */

/* vi: set ts=4 sw=4 expandtab: */
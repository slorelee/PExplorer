/*
 * Copyright 2004 Martin Fuchs
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */


//
// Explorer clone - precompiled header support
//
// precomp.h
//
// Martin Fuchs, 17.05.2004
//

#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES         1
#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_COUNT   1

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <wingdi.h>
#include <winnls.h>
#include <ole2.h>
#include <shlwapi.h>

#include "utility/utility.h"

#include "explorer.h"
#include "desktop/desktop.h"

#include "globals.h"
#include "externals.h"

#include "vendor/json.h"
#include "jconfig/jcfg.h"


#define DEFWINBRUSH() GetSysColorBrush(COLOR_BTNFACE)
//#define TASKBAR_BRUSH() DEFWINBRUSH()

#define COLOR_LIGHTGREEN() RGB(200, 235, 205)
#define WINXPBLUEBRUSH() CreateSolidBrush(RGB(0, 78, 192))
#define LIGHTGREENBRUSH() CreateSolidBrush(COLOR_LIGHTGREEN())

#define NOTIFYAREA_WIDTH_DEF    100
#define NOTIFYAREA_SPACE        3
#define NOTIFYICON_SIZE         DPI_SX(16)
#define NOTIFYICON_DIST         (NOTIFYICON_SIZE + NOTIFYICON_SIZE / 2 + 1)
#define NOTIFYICON_X            1
#define NOTIFYICON_Y            1
#define NOTIFY_HINT_Y (DESKTOPBARBAR_HEIGHT - NOTIFYICON_Y * 2)

/*
 * Copyright 2005 Martin Fuchs
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
// Explorer clone
//
// shellservices.cpp
//
// Martin Fuchs, 28.03.2005
//


#include <precomp.h>

#include <olectl.h>
#include <ole2.h>
#include "shellservices.h"

static IOleCommandTarget *StartSSO(TCHAR *strClsid)
{
    CLSID clsid;
    LPVOID lpVoid;
    IOleCommandTarget *target = NULL;

    CLSIDFromString(strClsid, &clsid);

    // The SSO might have a custom manifest.
    // Activate it before loading the object.
    //ULONG_PTR ulCookie;
    //HANDLE hContext = ELActivateActCtxForClsid(clsid, &ulCookie);

    if (SUCCEEDED(CoCreateInstance(clsid, NULL,
        CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
        IID_IOleCommandTarget, &lpVoid)))
    {
        // Start ShellServiceObject
        reinterpret_cast <IOleCommandTarget*> (lpVoid)->Exec(&CGID_ShellServiceObject,
            OLECMDID_NEW,
            OLECMDEXECOPT_DODEFAULT,
            NULL, NULL);
        target = reinterpret_cast <IOleCommandTarget*>(lpVoid);
    }

    //if (hContext != INVALID_HANDLE_VALUE)
    //ELDeactivateActCtx(hContext, &ulCookie);

    return target;
}

//Purpose: Loads the system icons
void SSOThread::LoadSSO()
{
    IOleCommandTarget *target = NULL;
    /*
    HKEY hkey;
    CLSID clsid;
    TCHAR name[MAX_PATH];

    if (!RegOpenKey(HKEY_LOCAL_MACHINE,
        TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellServiceObjects"), &hkey)) {
        for (int idx = 0; ; ++idx) {
            if (ERROR_SUCCESS != RegEnumKey(hkey, idx, name, MAX_PATH))
                break;

            if (!_alive)
                break;

            if (target = StartSSO(name))
                _ssoIconList.push_back(target);
        }
        RegCloseKey(hkey);
    }
    */
    if (target = StartSSO(L"{35CEC8A3-2BE6-11D2-8773-92E220524153}")) {
        _ssoIconList.push_back(target);
    }
}

// Purpose: Unload the system icons
void SSOThread::UnloadSSO()
{
    // Go through each element of the array and stop it...
    while (!_ssoIconList.empty())
    {
        if (_ssoIconList.back()->Exec(&CGID_ShellServiceObject, OLECMDID_SAVE,
            OLECMDEXECOPT_DODEFAULT, NULL, NULL) == S_OK)
            _ssoIconList.back()->Release();
        _ssoIconList.pop_back();
    }
}

int SSOThread::Run()
{
    ComInit usingCOM(COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE | COINIT_SPEED_OVER_MEMORY);

    LoadSSO();

    if (!_ssoIconList.empty()) {
        MSG msg;

        while (_alive) {
            if (MsgWaitForMultipleObjects(1, &_evtFinish, FALSE, INFINITE, QS_ALLINPUT) == WAIT_OBJECT_0 + 0)
                break;  // _evtFinish has been set.

            while (_alive) {
                if (!PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
                    break;

                if (msg.message == WM_QUIT)
                    break;

                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        // shutdown all running Shell Service Objects
        UnloadSSO();
    }

    return 0;
}

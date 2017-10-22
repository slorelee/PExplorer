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

#include <vector>

//
// Explorer clone
//
// shellservices.h
//
// Martin Fuchs, 28.03.2005
//


// launch start programs
extern "C" int startup(int argc, const TCHAR *argv[]);

typedef std::vector<IOleCommandTarget *> SSOVector;

// load Shell Service Objects (volume control, printer/network icons, ...)
struct SSOThread : public Thread {
    int Run();
private:
    SSOVector _ssoIconList;
    void LoadSSO();
    void UnloadSSO();
};

/*
 * Copyright 2003, 2004, 2005 Martin Fuchs
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
 // quicklaunch.cpp
 //
 // Martin Fuchs, 22.08.2003
 //


#include <precomp.h>

#include "../resource.h"

#include "quicklaunch.h"


QuickLaunchEntry::QuickLaunchEntry()
{
	_hbmp = 0;
}

QuickLaunchMap::~QuickLaunchMap()
{
	while(!empty()) {
		iterator it = begin();
		DeleteBitmap(it->second._hbmp);
		erase(it);
	}
}


QuickLaunchBar::QuickLaunchBar(HWND hwnd)
 :	super(hwnd)
{
	CONTEXT("QuickLaunchBar::QuickLaunchBar()");

	_dir = NULL;
	_next_id = IDC_FIRST_QUICK_ID;
	_btn_dist = 20;
	_size = 0;

	HWND hwndToolTip = (HWND) SendMessage(hwnd, TB_GETTOOLTIPS, 0, 0);

	SetWindowStyle(hwndToolTip, GetWindowStyle(hwndToolTip)|TTS_ALWAYSTIP);

	 // delay refresh to some time later
	PostMessage(hwnd, PM_REFRESH, 0, 0);
}

QuickLaunchBar::~QuickLaunchBar()
{
	delete _dir;
}

HWND QuickLaunchBar::Create(HWND hwndParent)
{
	CONTEXT("QuickLaunchBar::Create()");

	ClientRect clnt(hwndParent);

	HWND hwnd = CreateToolbarEx(hwndParent,
								WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN|
								CCS_TOP|CCS_NODIVIDER|CCS_NOPARENTALIGN|CCS_NORESIZE|
								TBSTYLE_TOOLTIPS|TBSTYLE_WRAPABLE|TBSTYLE_FLAT,
								IDW_QUICKLAUNCHBAR, 0, 0, 0, NULL, 0, 0, 0, TASKBAR_ICON_SIZE, TASKBAR_ICON_SIZE, sizeof(TBBUTTON));
	if (hwnd)
		new QuickLaunchBar(hwnd);

	return hwnd;
}

void QuickLaunchBar::AddShortcuts()
{
	CONTEXT("QuickLaunchBar::AddShortcuts()");

	WaitCursor wait;

	try {
		TCHAR path[MAX_PATH];

		SpecialFolderFSPath app_data(CSIDL_APPDATA, _hwnd);	///@todo perhaps also look into CSIDL_COMMON_APPDATA ?

		_stprintf(path, TEXT("%s\\")QUICKLAUNCH_FOLDER, (LPCTSTR)app_data);

		RecursiveCreateDirectory(path);
		_dir = new ShellDirectory(GetDesktopFolder(), path, _hwnd);

		_dir->smart_scan(SORT_NAME);

		// immediatelly extract the shortcut icons
		for(Entry*entry=_dir->_down; entry; entry=entry->_next)
			entry->_icon_id = entry->safe_extract_icon(ICF_LARGE);
	} catch(COMException&) {
		return;
	}


	ShellFolder desktop_folder;
	WindowCanvas canvas(_hwnd);

	COLORREF bk_color = RGB(0, 0, 0);
	HBRUSH bk_brush = TASKBAR_BRUSH(); //GetSysColorBrush(COLOR_BTNFACE);

	int icon_num = 0;
	AddButton(ID_MINIMIZE_ALL, g_Globals._icon_cache.get_icon(ICID_MINIMIZE).create_bitmap(bk_color, bk_brush, canvas, TASKBAR_ICON_SIZE), ResString(IDS_MINIMIZE_ALL), NULL);
	AddButton(ID_EXPLORE, g_Globals._icon_cache.get_icon(ICID_EXPLORER).create_bitmap(bk_color, bk_brush, canvas, TASKBAR_ICON_SIZE), ResString(IDS_TITLE), NULL);
	icon_num += 2;

	TBBUTTON sep = { 0, -1, TBSTATE_ENABLED, BTNS_SEP,{ 0, 0 }, 0, 0 };
	SendMessage(_hwnd, TB_INSERTBUTTON, INT_MAX, (LPARAM)&sep);

	for (Entry*entry = _dir->_down; entry; entry = entry->_next) {
		// hide files like "desktop.ini"
		if (entry->_data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
			continue;

		// hide subfolders
		if (!(entry->_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			HBITMAP hbmp = g_Globals._icon_cache.get_icon(entry->_icon_id).create_bitmap(bk_color, bk_brush, canvas, TASKBAR_ICON_SIZE);

			AddButton(_next_id++, hbmp, entry->_display_name, entry);	//entry->_etype==ET_SHELL? desktop_folder.get_name(static_cast<ShellEntry*>(entry)->_pidl): entry->_display_name);
			icon_num++;
		}
	}

	_btn_dist = LOWORD(SendMessage(_hwnd, TB_GETBUTTONSIZE, 0, 0));
	_size = _entries.size() * _btn_dist;

	SendMessage(GetParent(_hwnd), PM_RESIZE_CHILDREN, 0, 0);

	//adjust QuickLaunchBar width
	static LONG cx = -1;
	if (cx == -1) {
		ICONINFO ici;
		GetIconInfo(g_Globals._icon_cache.get_icon(ICID_MINIMIZE).get_hicon(), &ici);
		BITMAP bm;
		GetObject(ici.hbmColor, sizeof(BITMAP), &bm);
		cx = bm.bmWidth;
		DeleteObject(ici.hbmColor);
		DeleteObject(ici.hbmMask);
	} else {
		cx = GetSystemMetrics(SM_CXSMICON);
	}
	REBARBANDINFO rbBand;
	rbBand.cbSize = sizeof(REBARBANDINFO);
	rbBand.wID = IDW_QUICKLAUNCHBAR;
	rbBand.fMask = RBBIM_ID | RBBIM_SIZE;
	rbBand.cx = (cx + 3) * icon_num + TASKBAR_ICON_SIZE * 2;
	SendMessage(GetParent(_hwnd), RB_SETBANDINFO, (WPARAM)0, (LPARAM)&rbBand);
}

void QuickLaunchBar::AddButton(int id, HBITMAP hbmp, LPCTSTR name, Entry* entry, int flags)
{
	TBADDBITMAP ab = {0, (UINT_PTR)hbmp};
	int bmp_idx = SendMessage(_hwnd, TB_ADDBITMAP, 1, (LPARAM)&ab);

	QuickLaunchEntry qle;

	qle._hbmp = hbmp;
	qle._title = name;
	qle._entry = entry;

	_entries[id] = qle;

	TBBUTTON btn = {0, 0, flags, BTNS_BUTTON|BTNS_NOPREFIX, {0, 0}, 0, 0};

	btn.idCommand = id;
	btn.iBitmap = bmp_idx;

	SendMessage(_hwnd, TB_INSERTBUTTON, INT_MAX, (LPARAM)&btn);
}

LRESULT QuickLaunchBar::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	switch(nmsg) {
	  case PM_REFRESH:
		AddShortcuts();
		break;

	  case PM_GET_WIDTH: {
		 // take line wrapping into account
		int btns = SendMessage(_hwnd, TB_BUTTONCOUNT, 0, 0);
		int rows = SendMessage(_hwnd, TB_GETROWS, 0, 0);

		if (rows<2 || rows==btns)
			return _size;

		RECT rect;
		int max_cx = 0;

		for(QuickLaunchMap::const_iterator it=_entries.begin(); it!=_entries.end(); ++it) {
			SendMessage(_hwnd, TB_GETRECT, it->first, (LPARAM)&rect);

			if (rect.right > max_cx)
				max_cx = rect.right;
		}

		return max_cx;}
	  case WM_CONTEXTMENU: {
		TBBUTTON btn;
		QuickLaunchMap::iterator it;
		Point screen_pt(lparam), clnt_pt=screen_pt;
		ScreenToClient(_hwnd, &clnt_pt);

		Entry* entry = NULL;
		int idx = SendMessage(_hwnd, TB_HITTEST, 0, (LPARAM)&clnt_pt);

		if (idx>=0 &&
			SendMessage(_hwnd, TB_GETBUTTON, idx, (LPARAM)&btn)!=-1 &&
			(it=_entries.find(btn.idCommand))!=_entries.end()) {
			entry = it->second._entry;
		}

		if (entry) {	// entry is NULL for desktop switch buttons
			HRESULT hr = entry->do_context_menu(_hwnd, screen_pt, _cm_ifs);

			if (!SUCCEEDED(hr))
				CHECKERROR(hr);
		} else
			goto def;
		break;}

	  default: def:
		return super::WndProc(nmsg, wparam, lparam);
	}

	return 0;
}

int QuickLaunchBar::Command(int id, int code)
{
	CONTEXT("QuickLaunchBar::Command()");

	if ((id&~0xFF) == IDC_FIRST_QUICK_ID) {
		QuickLaunchEntry& qle = _entries[id];

		if (qle._entry) {
			qle._entry->launch_entry(_hwnd);
			return 0;
		}
	}

	return 0; // Don't return 1 to avoid recursion with DesktopBar::Command()
}

int QuickLaunchBar::Notify(int id, NMHDR* pnmh)
{
	switch(pnmh->code) {
	  case TTN_GETDISPINFO: {
		NMTTDISPINFO* ttdi = (NMTTDISPINFO*) pnmh;

		int id = ttdi->hdr.idFrom;
		ttdi->lpszText = _entries[id]._title.str();
#ifdef TTF_DI_SETITEM
		ttdi->uFlags |= TTF_DI_SETITEM;
#endif

		 // enable multiline tooltips (break at CR/LF and for very long one-line strings)
		SendMessage(pnmh->hwndFrom, TTM_SETMAXTIPWIDTH, 0, 400);

		break;}

		return super::Notify(id, pnmh);
	}

	return 0;
}

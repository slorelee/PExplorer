#pragma once

#include <Windows.h>
#include "../utility/window.h"

struct PictureButton2 : public PictureButton {
    typedef PictureButton super;

    PictureButton2(HWND hwnd, HICON hIcon, HICON hIcon2, HBRUSH hbrush, HBRUSH hbrush2,
        COLORREF textcolor = -1, bool flat = false)
        : super(hwnd, hIcon, hbrush, textcolor, flat),
        _hIcon(hIcon), _hHotIcon(hIcon2), _hBmp(0), _hBrush(hbrush), _hHotBrush(hbrush2), _flat(flat)
    {
        _cx = super::_cx;
        _cy = super::_cy;
        }

protected:
    void DrawItem(LPDRAWITEMSTRUCT dis);
    HICON   _hIcon;
    HICON   _hHotIcon;
    HBITMAP _hBmp;
    HBRUSH  _hBrush;
    HBRUSH  _hHotBrush;

    int     _cx;
    int     _cy;

    COLORREF _textColor;
    bool    _flat;
};


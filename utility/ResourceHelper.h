#pragma once

#include <Windows.h>

LPVOID LoadCustomResource(UINT rID, LPTSTR rType);
BOOL FreeCustomResource(LPVOID res);

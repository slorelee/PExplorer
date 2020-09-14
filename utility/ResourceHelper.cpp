
#include <Windows.h>
#include "ResourceHelper.h"

LPVOID LoadCustomResource(UINT rID, LPTSTR rType) {
	HRSRC  hData = FindResource(NULL, MAKEINTRESOURCE(rID), rType);
	if (!hData) return NULL;
	HGLOBAL hRes = LoadResource(NULL, hData);
	if (!hRes) return NULL;
	LPVOID pData = LockResource(hRes);
	if (!pData) {
		FreeResource(hRes);
		return NULL;
	}
	int iResSize = SizeofResource(NULL, hData);
	PCHAR pNewHeap = (PCHAR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, iResSize);
	if (pNewHeap) {
		memcpy(pNewHeap, pData, iResSize);
	}
	FreeResource(hRes);
	return (LPVOID)pNewHeap;
}

BOOL FreeCustomResource(LPVOID res) {
	return HeapFree(GetProcessHeap(), 0, res);
}

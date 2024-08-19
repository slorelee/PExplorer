#pragma once

#include <string>
#include <windows.h>


#ifndef string_t
#ifdef UNICODE
#define string_t std::wstring
#else
#define string_t string
#endif
#endif

extern std::wstring s2w(const std::string& str, UINT cp = CP_ACP);
extern std::string w2s(const std::wstring& wstr);
extern std::string w2s(const wchar_t *wstr);

extern std::string utf8toansi(const std::string& str);

extern void resstr_expand(string_t &restr);
extern void varstr_expand(string_t &str);

#ifndef LOGA
extern void _logA_(LPCSTR txt, char endmark);

#define LOGA(txt) _logA_(txt, '\n')
#define LOGA2(txt, end) _logA_(txt, end)
#endif

extern void _logU2A_(LPCWSTR txt);

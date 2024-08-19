
#include "precomp.h"

#include <Windows.h>
#include <tchar.h>
#include "LogHelper.h"

#include "utility.h"

std::wstring s2w(const std::string& str, UINT cp)
{
    int len = MultiByteToWideChar(cp, 0, str.c_str(), -1, NULL, 0);
    if (len == 0) {
        return std::wstring(TEXT(""));
    }
    wchar_t *wide = new wchar_t[len];
    MultiByteToWideChar(cp, 0, str.c_str(), -1, wide, len);
    std::wstring wstr(wide);
    delete[] wide;
    return wstr;
}

std::string _w2s(const wchar_t *wstr, UINT cp)
{
    int len = 0;
    const wchar_t *srcTemp = wstr;
    char *destTemp = NULL;

    len = WideCharToMultiByte(cp, 0, srcTemp, -1, NULL, 0, NULL, NULL);
    destTemp = new char[len];
    WideCharToMultiByte(cp, 0, srcTemp, -1, destTemp, len, NULL, NULL);

    std::string str = destTemp;
    delete[]destTemp;
    return str;
}

std::string w2s(const wchar_t *wstr)
{
    return _w2s(wstr, CP_ACP);
}

std::string w2s(const std::wstring& wstr)
{
    return w2s(wstr.c_str());
}

std::string w2utf8(const wchar_t *wstr)
{
    return _w2s(wstr, CP_UTF8);
}

std::string w2utf8(const std::wstring& wstr)
{
    return w2utf8(wstr.c_str());
}

std::string utf8toansi(const std::string& str)
{
    int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
    if (len == 0) {
        return std::string("");
    }
    wchar_t *wide = new wchar_t[len];
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, wide, len);

    int ansiLen = ::WideCharToMultiByte(CP_ACP, NULL, wide, -1, NULL, 0, NULL, NULL);
    char *ansiStr = new char[ansiLen];
    WideCharToMultiByte(CP_ACP, NULL, wide, -1, ansiStr, ansiLen, NULL, NULL);
    delete[] wide;
    return ansiStr;
}

string_t GetParameter(string_t cmdline, string_t key, BOOL hasValue)
{
    int hasQuote = 0;
    key = _T(" ") + key + _T(" ");
    cmdline = _T(" ") + cmdline + _T(" ");
    string_t::size_type pos = cmdline.find(key);
    string_t::size_type startPos = 0;
    string_t::size_type endPos = 0;
    if (pos != string_t::npos) {
        if (!hasValue) return key;
        startPos = pos + key.length();
        if (cmdline[startPos] == _T('\"')) {
            endPos = cmdline.find(_T('\"'), startPos + 1);
            hasQuote = 1;
        } else {
            endPos = cmdline.find(_T(' '), startPos);
        }
        if (endPos != string_t::npos) {
            if (hasQuote) return cmdline.substr(startPos + 1, endPos - startPos - 1);
            return cmdline.substr(startPos, endPos - startPos);
        }
    }
    return _T("");
}

void str_replace(string_t &s1, const string_t &s2, const string_t &s3)
{
    string_t::size_type pos = 0;
    string_t::size_type a = s2.size();
    string_t::size_type b = s3.size();
    while ((pos = s1.find(s2, pos)) != string_t::npos) {
        s1.replace(pos, a, s3);
        pos += b;
    }
}

TCHAR *localename()
{
    static TCHAR name[LOCALE_NAME_MAX_LENGTH] = { 0 };
    if (name[0] != _T('\0')) return name;
    GetSystemDefaultLocaleName(name, LOCALE_NAME_MAX_LENGTH);
    return name;
}

string_t load_resstr(const TCHAR *file, const TCHAR *sid)
{
    string_t str = TEXT("");
    TCHAR buff[256] = { 0 };
    int id = _ttoi(sid);
    buff[0] = _T('\0');
    HINSTANCE res = LoadLibrary(file);
    if (!res) {
        string_t mui = FmtString(TEXT("%s\\%s.mui"), localename(), file);
        res = LoadLibrary(mui.c_str());
    }
    if (res) {
        LoadString(res, id, buff, 256);
        FreeLibrary(res);
        str = buff;
    }
    return (str);
}

void resstr_expand(string_t &restr)
{
    size_t s = 0;
    size_t e = 0;
    size_t sep = 0;
    string_t resid;
    string_t ptr;
    if (restr.length() < 3) return;
    s = restr.find(TEXT("#{"));
    while (s != string_t::npos) {
        e = restr.find(TEXT("}"), s);
        if (e != string_t::npos) {
            resid = restr.substr(s, e - s + 1);
            if (resid == TEXT("#{#}")) {
                s = restr.find(TEXT("#{"), s + 1);
                continue;
            }
            sep = resid.find(TEXT(","));
            if (sep != string_t::npos) {
                ptr = load_resstr(resid.substr(3, sep - 3).c_str(), resid.substr(sep + 1, e - sep - 1).c_str());
                str_replace(restr, resid, ptr);
            }
        }
        s = restr.find(TEXT("#{"), s + 1);
    }
    if (restr.find(TEXT("#{#}")) != string_t::npos) {
        str_replace(restr, TEXT("#{#}"), TEXT("#"));
    }
}

void varstr_expand(string_t &str)
{
    TCHAR buff[MAX_PATH] = { 0 };
    resstr_expand(str);
    ExpandEnvironmentStrings(str.c_str(), buff, MAX_PATH);
    str = buff;
}

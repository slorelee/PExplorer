#pragma once

#include <stdio.h>
#include <string>
#include<Windows.h>

#include "lua.hpp"
#include "LuaHelper.h"
#include "../DUI/Helper.h"

#ifndef string_t
#ifdef UNICODE
#define string_t std::wstring
#else
#define string_t string
#endif
#endif

enum TokenValue {
    TOK_UNSET = 0,
    TOK_STRING,
    TOK_INTEGER,
    TOK_BOOL,
    TOK_TRUE,
    TOK_FALSE,
    TOK_STRARR,
    TOK_LIST,
    TOK_ELEM,
    TOK_NUMBER,
    TOK_UNDEFINED,
};

typedef struct _Token {
    TokenValue type;
    string_t str;
    union {
        LPVOID pObj;
        int iVal;
        bool bVal;
        double dVal;
    };
    string_t attr; //element
} Token;


extern BOOL isWinXShellAsShell();
extern int FakeExplorer();
extern HRESULT DoFileVerb(PCTSTR tzFile, PCTSTR verb);

extern void ShellContextMenuVerb(const TCHAR *file, TCHAR *verb);


#define PUSH_STR(v) {lua_pushstring(L, w2s(v.str).c_str());ret++;}
#define PUSH_INT(v) {lua_pushinteger(L, v.iVal);ret++;}
#define PUSH_BOOL(v) {lua_pushboolean(L, v.iVal);ret++;}
#define PUSH_INTVAL(val) {lua_pushinteger(L, val);ret++;}





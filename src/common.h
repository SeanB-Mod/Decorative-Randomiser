#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
inline std::wstring module_path(HMODULE module){wchar_t path[32768]{};DWORD n=GetModuleFileNameW(module,path,32768);return n&&n<32768?std::wstring(path,n):std::wstring();}
inline std::wstring parent_path(const std::wstring& p){auto n=p.find_last_of(L"\\/");return n==std::wstring::npos?L"":p.substr(0,n+1);}

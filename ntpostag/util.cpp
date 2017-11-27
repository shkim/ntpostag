#include "stdafx.h"
#include "postag.h"

#include <locale>
#include <codecvt>

BEGIN_NAMESPACE_NTPOSTAG

std::wstring CstringToWstring(const char* src)
{
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t
#if defined(__GNUC__)
        // Bug: The std::little_endian should be selected directly in std::codecvt_utf8_utf16 for GCC v5.4.0 for Ubuntu 16.04 LTS.
        , 0x10FFFF
        , std::little_endian
#endif
	>> convert;

	return convert.from_bytes(src);
}

std::string WstringToCstring(const wchar_t* src)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> convert;
	return convert.to_bytes(src);
}

#ifdef _WIN32

void WidePrintf(const wchar_t *fmt, ...)
{
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD Written;
	WCHAR buf[1024 * 10];

	va_list arglist;
	va_start(arglist, fmt);
	vswprintf(buf, sizeof(buf) / sizeof(buf[0]), fmt, arglist);
	WriteConsoleW(hOut, buf, (DWORD)wcslen(buf), &Written, NULL);
	va_end(arglist);
}

#else // _WIN32

#endif // _WIN32

char *TrimString(char *psz)
{
	char *s, *e;

	if (psz == NULL) return NULL;

	s = psz;
	while (*s == ' ' || *s == '\t') s++;

	for (e = s; *e != '\0'; e++);

	do *e-- = '\0'; while (*e == ' ' || *e == '\t' || *e == '\r' || *e == '\n');

	return (s > e) ? NULL : s;
}

wchar_t *TrimString(wchar_t *psz)
{
	wchar_t *s, *e;

	if(psz == NULL) return NULL;

	s = psz;
	while(*s == ' ' || *s == '\t') s++;

	for(e=s; *e != '\0'; e++);

	do *e-- = L'\0'; while(*e == L' ' || *e == L'\t' || *e == L'\r' || *e == L'\n');

	return (s>e) ? NULL : s;
}

END_NAMESPACE_NTPOSTAG

#pragma once

BEGIN_NAMESPACE_NTPOSTAG

std::wstring CstringToWstring(const char* src);
std::string WstringToCstring(const wchar_t* src);

#ifdef _WIN32
	void WidePrintf(const wchar_t *fmt, ...);
	#ifndef _NOUSE_CPPREST
		#define WstringToUstring(s)	s
		#define UstringToWstring(s)	s
		#define CstringToUstring(s)	CstringToWstring(s).c_str()
	#endif
#else
	#define WidePrintf wprintf
	#ifndef _NOUSE_CPPREST
		#define WstringToUstring(s)	WstringToCstring(s).c_str()
		#define UstringToWstring(s)	CstringToWstring(s).c_str()
		#define CstringToUstring(s)	s
	#endif
#endif

char *TrimString(char *psz);
wchar_t *TrimString(wchar_t *psz);

END_NAMESPACE_NTPOSTAG

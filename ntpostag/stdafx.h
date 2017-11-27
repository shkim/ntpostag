// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define _NOUSE_CPPREST


#ifdef _WIN32
//#define _UNITTEST	// VS bug? can't build unless "Debug" or "Release" configuration.

#include "targetver.h"
#include <windows.h>
#include <tchar.h>
#include <assert.h>
#else
#include "compat.h"
#endif

#include <stdio.h>
#include <string.h>

#include <vector>
#include <deque>
#include <list>
#include <map>
#include <set>
#include <memory>
#include <functional>

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/optional.hpp>

#define ASSERT assert

#define BEGIN_NAMESPACE_NTPOSTAG	namespace ntpostag {
#define END_NAMESPACE_NTPOSTAG		}
#define USING_NAMESPACE_NTPOSTAG	using namespace ntpostag;

#if defined(_MSC_VER) && defined(_UNITTEST)
#pragma warning(disable : 4251)
#pragma warning(disable : 4275)
#endif

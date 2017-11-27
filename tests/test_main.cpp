//#define _TURN_OFF_PLATFORM_STRING

#include "stdafx.h"
#include "postag.h"

#ifdef _UNITTEST

//#undef U
#include <gtest/gtest.h>

USING_NAMESPACE_NTPOSTAG

class _Environment : public ::testing::Environment
{
public:
	virtual ~_Environment() {}

	virtual void SetUp()
	{
		if (!g_posdic.LoadData(L"../posdic.dat"))
		{
			throw "PosDic::LoadData failed.";
		}
	}

	virtual void TearDown()
	{
		g_posdic.Unload();
	}
};

#if 1//def _WIN32

int main(int argc, char **argv)
{
	testing::InitGoogleTest(&argc, argv);
	AddGlobalTestEnvironment(new _Environment());
	testing::GTEST_FLAG(filter) = "ConjugationTests.FindRootForAnonymousPredicates";
	return RUN_ALL_TESTS();
}

#endif

#endif	// _UNITTEST

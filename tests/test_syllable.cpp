#include "stdafx.h"
#include "postag.h"

#ifdef _UNITTEST

//#undef U
#include "gtest/gtest.h"

USING_NAMESPACE_NTPOSTAG

TEST(SyllableTests, Decomposition)
{
	Character s1, s2, s3;

	s1.Set(L'가');
	s2.Set(L'헉');
	s3.Set(L'A');

	EXPECT_TRUE(s1.GetCategory() == Character::CT_HANGUL_COMPOSED);
	EXPECT_EQ(s1.GetCode(), L'가');
	EXPECT_EQ(s1.GetCho(), L'ㄱ');
	EXPECT_EQ(s1.GetJoong(), L'ㅏ');
	EXPECT_EQ(s1.GetZong(), 0);

	EXPECT_TRUE(s2.GetCategory() == Character::CT_HANGUL_COMPOSED);
	EXPECT_EQ(s2.GetCode(), L'헉');
	EXPECT_EQ(s2.GetCho(), L'ㅎ');
	EXPECT_EQ(s2.GetJoong(), L'ㅓ');
	EXPECT_EQ(s2.GetZong(), L'ㄱ');

	EXPECT_TRUE(s3.GetCategory() == Character::CT_ALPHABET);
	EXPECT_EQ(s3.GetCode(), L'A');
	EXPECT_EQ(s3.GetCho(), 0);
	EXPECT_EQ(s3.GetJoong(), 0);
	EXPECT_EQ(s3.GetZong(), 0);
}

#endif	// _UNITTEST
#include "stdafx.h"
#include "postag.h"
#include "util.h"

#ifdef _UNITTEST

//#undef U
#include "gtest/gtest.h"

USING_NAMESPACE_NTPOSTAG

/*
* 줄바꿈이 두번 이상 이어지면 하나의 문장이 끝난 것으로 본다.
* (?), (!), (.) 등 괄호로 묶인 문장 마침표는 무시한다.
* 문장 속에 "(Double Quote)나 '(Single Quote)로 묶인 경우, 문장으로 간주하지 않는다.
* 문장이 문장 마침표로 끝난 뒤에, 빈 칸 없이 새로운 문장이 시작되는 경우,
* 문장 마침표(SF)로 끝나면 하나의 문장으로 본다.
* Quote가 닫히지 않은 경우, Quote 단위로 문장을 분리하지 않는다.
* 분리된 문장은 앞 뒤 공백을 제거한다.
*/

static void DumpParaStrs(Paragraph& para)
{
	auto strs = para.GetSentences();
	for (auto str : strs)
	{
		WidePrintf(L"'%s'\n", str.GetString().c_str());
	}
}

static void CheckSplitResult(Paragraph& para, const std::initializer_list<const wchar_t*>& answers)
{
	auto sens = para.GetSentences();
	EXPECT_EQ(sens.size(), answers.size()) << "Splitted sentence counts are different.";
	if (sens.size() != answers.size())
		return;

	int i = 0;
	for (auto ans : answers)
	{
		ASSERT_STREQ(sens[i++].GetString().c_str(), ans);
	}
}

TEST(SplitSentenceTests, SplitByNewline)
{
	Paragraph para;

	EXPECT_FALSE(para.Set(L"\n"));
	EXPECT_FALSE(para.Set(L" "));
	EXPECT_FALSE(para.Set(L"\n\r"));
	EXPECT_FALSE(para.Set(L"   "));

	EXPECT_TRUE(para.Set(L"A"));
	CheckSplitResult(para, { L"A" });

	EXPECT_TRUE(para.Set(L"A\n\n  \n\n"));
	CheckSplitResult(para, { L"A" });

	EXPECT_TRUE(para.Set(L"A\nB\nC"));
	CheckSplitResult(para, { L"A B C" });

	EXPECT_TRUE(para.Set(L"A\r\nB\n\rC"));
	CheckSplitResult(para, { L"A B C" });

	EXPECT_TRUE(para.Set(L"A\n\nB\r\rC"));
	CheckSplitResult(para, { L"A", L"B", L"C" });

	EXPECT_TRUE(para.Set(L"\rA\n\nB\r\rC\n"));
	CheckSplitResult(para, { L"A", L"B", L"C" });

	EXPECT_TRUE(para.Set(L"ABC\n\n   DEF \r\rGHI\n\n\n 123 \n  456  \n\n"));
	CheckSplitResult(para, { L"ABC", L"DEF", L"GHI", L"123 456" });

	EXPECT_TRUE(para.Set(L"한글\n\n가나다\r\n123\n\r!@#\r\rXXX\n\n\n987"));
	CheckSplitResult(para, { L"한글", L"가나다 123 !@#", L"XXX", L"987" });
}

static void CheckQuoteResult(Paragraph& para, const std::initializer_list<const wchar_t*>& answers)
{
	std::vector<std::wstring> quotedStrs;

	auto sens = para.GetSentences();
	for (auto sen : sens)
	{
		auto qs = sen.GetQuotedStrings();
		if (!qs.empty())
			quotedStrs.insert(quotedStrs.end(), qs.begin(), qs.end());
	}

	EXPECT_EQ(quotedStrs.size(), answers.size()) << "Quoted string counts are different.";
	if (quotedStrs.size() != answers.size())
		return;

	int i = 0;
	for (auto ans : answers)
	{
		ASSERT_STREQ(quotedStrs[i++].c_str(), ans);
	}
}

TEST(SplitSentenceTests, ExtractQuotedString)
{
	Paragraph para;

	para.Set(L"AAA \"BBB\"CC DDD.");
	CheckQuoteResult(para, { L"\"BBB\"" });

	para.Set(L"김씨는 \"가나다라마바사\"라고 말하였다.");
	CheckQuoteResult(para, { L"\"가나다라마바사\"" });

	para.Set(L"AAA \"QQ1\"BB \"QQ2\"CC \"QQ3\"DD EEE.");
	CheckQuoteResult(para, { L"\"QQ1\"", L"\"QQ2\"", L"\"QQ3\"" });

	para.Set(L"AAA “QQ1”BB ‟QQ2”CC ‘QQ3’DD EEE.");
	CheckQuoteResult(para, { L"“QQ1”", L"‟QQ2”", L"‘QQ3’" });
}

TEST(SplitSentenceTests, SentencePartition)
{
	Paragraph para;
	
	para.Set(L"A\"B(C)D\"E");
}

#endif	// _UNITTEST
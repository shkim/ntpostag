#include "stdafx.h"
#include "postag.h"
#include "util.h"

#ifdef _UNITTEST

#include "gtest/gtest.h"

USING_NAMESPACE_NTPOSTAG

static const wchar_t* s_anonPredicates[] =
{
	L"주장하다:", L"주장했다", L"주장하였다",
	L"내놓다:", L"내놨다", L"내놓았다",
	L"강조하다:", L"강조했다", L"강조했었다",
	L"선언하다:", L"선언했다", L"",

	L"당부하다:", L"당부했다", L"",
	L"호소하다:", L"호소했다", L"",
	L"나타내다:", L"나타냈다", L"",
	L"하다:", L"했다", L"",
	L"아쉬워하다:", L"아쉬워했다", L"",
	L"안타까워하다:", L"안타까워했다", L"",

	L"우려하다:", L"우려했다", L"",
	L"표명하다:", L"표명했다", L"",	// 우려를

	L"단언하다:", L"단언했다", L"",
	L"단정하다:", L"단정했다",
	L"일축하다:", L"일축했다",
	L"고수하다:", L"고수했다",

	L"촉구하다:", L"촉구했다",
	L"경고하다:", L"경고했다",
	L"놓다:", L"놨다",
	L"가하다:", L"가했다",

	L"지적하다:", L"지적했다",
	L"꼬집다:", L"꼬집었다",
	L"비판하다:", L"비판했다",
	L"비난하다:", L"비난했다",

	L"해명하다:", L"해명했다",
	L"반발하다:", L"반발했다",
	L"항변하다:", L"항변했다",
	L"표시하다:", L"표시했다",	// 불쾌감을
	L"높히다:", L"높혔다",	// 목소리를

	L"제기하다:", L"제기했다",	// 의혹을, 의문을, 문제를
	//L"의아해 했다",

	L"시사하다:", L"시사했다",
	L"내다보다:", L"내다봤다",
	L"내비치다:", L"내비쳤다",
	L"기대하다:", L"기대했다",
	L"전망하다:", L"전망했다",
	L"나타내다:", L"나타냈다",	// 기대를
	L"이다:", L"있다",	// 관측도 나오고

	L"평하다:", L"평했다",
	L"평가하다:", L"평가했다",
	L"분석하다:", L"분석했다",

	//L"~라고도 했다",
	L"정도이다:", L"정도다",	// 말할
	L"털어놓다:", L"털어놓았다",
	L"토로하다:", L"토로했다",
	L"귀띔하다:", L"귀띔했다",
	L"못하다:", L"못했다",	// 벌어진 입을 다물지, 말을 제대로 잇지, 흥분을 감추지
	L"모으다:", L"모았다",	// 입을
	L"보이다:", L"보였다",	// 자조적인 태도를, 신중한 반응을
	L"붙이다:", L"붙였다",	// 망설이다 한마디 더
	L"쉬다:", L"쉬었다",	// 한숨을
	L"내두르다:", L"내둘렀다",	// 혀를
	L"나오다:", L"나왔다",	// 완화된 어조로


	L"전하다:", L"전해졌다",
	L"알리다:", L"알려졌다",
	NULL
};

TEST(ConjugationTests, FindRootForAnonymousPredicates)
{
	std::wstring expectedRoot;
	for (int i = 0; i < (sizeof(s_anonPredicates) / sizeof(s_anonPredicates[0])); i++)
	{
		const wchar_t* pszWord = s_anonPredicates[i];
		if (pszWord == NULL)
			break;

		if (pszWord[0] == 0)
			continue;	// L""

		std::wstring word = pszWord;
		if (word.back() == L':')
		{
			word.pop_back();
			expectedRoot = word;
			continue;
		}

		std::vector<PosInfo*> vec = g_posdic.MatchPredicate(CharString(word.c_str()));
		if (vec.empty())
		{
			WidePrintf(L"Match(%ls) failed: no predicate guessed.\n", word.c_str());
			//EXPECT_FALSE(vec.empty());
			continue;
		}

		EXPECT_EQ(expectedRoot, vec.front()->GetString());
		if (expectedRoot != vec.front()->GetString())
		{
			WidePrintf(L"Expected(%ls) != Guessed(%ls)\n", expectedRoot.c_str(), vec.front()->GetString().c_str());
		}
	}

	g_posdic.MatchPredicate(CharString(L"갑니다"));
}

#endif // _UNITTEST

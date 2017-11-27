#include "stdafx.h"
#include "postag.h"

#ifdef _UNITTEST

#include "gtest/gtest.h"

USING_NAMESPACE_NTPOSTAG

TEST(PostagTests, Simple)
{	
	Paragraph para;
	para.Set(L"한글 출력이 잘 될까요?");
	//para.GetSentences().front().Analyze();

	para.Set(L"박원순 서울 시장은 이날 오전 청와대에서 열린");
	//para.GetSentences().front().Analyze();
}

#endif	// _UNITTEST
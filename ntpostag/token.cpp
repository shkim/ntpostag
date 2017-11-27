#include "stdafx.h"
#include "postag.h"
#include "util.h"

BEGIN_NAMESPACE_NTPOSTAG


TokenPosNominee::TokenPosNominee(int capacity)
{
	if (capacity > 0)
	{
		m_poses.reserve(capacity);
	}

	m_pIsDependencyOf = NULL;
}

TokenPosNominee::~TokenPosNominee()
{
	for (auto pInfo : m_poses)
	{
		pInfo->DeleteIfNotRegistered();
	}

	m_poses.clear();
}

void TokenPosNominee::AddPos(PosInfo* pInfo)
{
	m_poses.push_back(PosInfoWrapper(pInfo));
}

int TokenPosNominee::GetScore()
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

inline bool IsCategory_NonLetter(const Character* pChar)
{
	return pChar->GetCategory() < Character::CT_SYMBOL;
}

inline bool IsCategory_LanguageLetter(const Character* pChar)
{
	return pChar->GetCategory() >= Character::CT_ALPHABET;
}

void Token::GatherPossiblePoses()
{
	if (IsCategory_NonLetter(m_str.GetCharacter(0)))
	{
		PosInfo* pInfo = PosInfo::CreateFromPuncMark(&m_str);

		TokenPosNominee nominee;
		nominee.AddPos(pInfo);
		m_nominees.push_back(nominee);
		return;
	}

	// 토큰이 사전 등록 단어인지 검사: 부사, 관형사, 명사 단독 등
	auto exactWords = g_posdic.MatchExactWord(m_str);
	if (!exactWords.empty())
	{
		for (PosInfo* pInfo : exactWords)
		{
			TokenPosNominee nominee;
			nominee.AddPos(pInfo);
			m_nominees.push_back(nominee);
		}
	}

	// [명사+조사] 형태인지?
	auto josaMatches = g_posdic.MatchJosa(m_str);
	if (!josaMatches.empty())
	{
		for (PosInfoJosa* pJosa : josaMatches)
		{
			int lenCoreRemain = m_str.GetLength() - pJosa->GetLength();
			ASSERT(lenCoreRemain > 0);
			auto pNoun = g_posdic.MatchNoun(m_str.GetSubstring(0, lenCoreRemain));
			if (pNoun)
			{
				TokenPosNominee nominee;
				nominee.AddPos(pNoun);
				nominee.AddPos(pJosa);
				m_nominees.push_back(nominee);
			}
		}
	}

	// 용언?
	auto nominieePreds = g_posdic.MatchPredicate(m_str);
	if (!nominieePreds.empty())
	{
		for (PosInfo* pInfo : nominieePreds)
		{
			TokenPosNominee nominee;
			nominee.AddPos(pInfo);
			m_nominees.push_back(nominee);
		}
	}
}

const TokenPosNominee& Token::GetMostPossiblePos() const
{
	ASSERT(!m_nominees.empty());
	return m_nominees.front();
}

/*
boost::optional<TokenPosNominee> GetPos()
{
	if (m_nominees.empty())
		return boost::none;

	return m_nominees.front();
}
*/
END_NAMESPACE_NTPOSTAG

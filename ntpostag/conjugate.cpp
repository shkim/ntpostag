#include "stdafx.h"
#include "postag.h"
#include "util.h"

BEGIN_NAMESPACE_NTPOSTAG

// eg. 할까요 minus ㄹ까요 => 하
wchar_t Conjugation::GetRemainChar(const CharString* predStr) const
{
	if (m_remainChar != 0)
		return m_remainChar;

	int divpos = -GetLength();
	ASSERT(divpos < 0);

	Character rch;
	const Character* pChar = predStr->GetCharacter(divpos);
	switch (m_finalMatchType)
	{
	case Conjugation::MATCH_FULL:
		ASSERT(pChar->GetCode() == m_word.at(0));
		return 0;

	case Conjugation::MATCH_EXCEPT_CHO:
	case Conjugation::MATCH_JOONG_AND_NOZONG:
		ASSERT(!"TODO");
		break;

	case Conjugation::MATCH_ZONG_ONLY:
		ASSERT(pChar->HasZong());
		rch.SetHangul(pChar->GetCho(), pChar->GetJoong(), 0);
		ASSERT(rch.GetCategory() == Character::CT_HANGUL_COMPOSED);
		return rch.GetCode();
	}

	ASSERT(!"Invalid m_finalMatchType");
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

void ConjTreeNode::Clear()
{
	for (auto itr = m_children.begin(); itr != m_children.end(); ++itr)
	{
		delete itr->second;
	}
	m_children.clear();

	for (Conjugation* p : m_items)
	{
		delete p;
	}
	m_items.clear();
}

Conjugation* ConjTreeNode::Add(const wchar_t* pszConjSpec, const std::initializer_list<EPosMeta>& metas)
{
	Conjugation::MatchType matchType;
	switch (pszConjSpec[0])
	{
	case 'c':
		matchType = Conjugation::MATCH_EXCEPT_CHO;
		break;
	case 'j':
		matchType = Conjugation::MATCH_JOONG_AND_NOZONG;
		break;
	case 'z':
		matchType = Conjugation::MATCH_ZONG_ONLY;
		break;
	default:
		matchType = Conjugation::MATCH_FULL;
		break;
	}

	if (matchType != Conjugation::MATCH_FULL)
	{
		pszConjSpec++;
	}

	Character charLast;
	charLast.Set(pszConjSpec[0]);
	ASSERT(charLast.GetCategory() == Character::CT_HANGUL_COMPOSED
		|| charLast.GetCategory() == Character::CT_HANGUL_PARTIAL);

	if (charLast.GetCategory() == Character::CT_HANGUL_PARTIAL
		&& matchType == Conjugation::MATCH_FULL)
	{
		if (charLast.GetJoong() > 0)
		{
			matchType = Conjugation::MATCH_JOONG_AND_NOZONG;
		}
		else
		{
			matchType = Conjugation::MATCH_ZONG_ONLY;
		}
	}

	Conjugation* pNewConj = new Conjugation();

	size_t wordlen;
	const wchar_t* pColon = wcschr(pszConjSpec, ':');
	if (pColon != NULL)
	{
		pNewConj->m_remainChar = pColon[1];
		wordlen = pColon - pszConjSpec;
	}
	else
	{
		pNewConj->m_remainChar = 0;
		wordlen = wcslen(pszConjSpec);
	}

	pNewConj->m_finalMatchType = matchType;
	pNewConj->m_word = std::wstring(pszConjSpec, wordlen);	
	pNewConj->m_metas = metas;

	std::vector<wchar_t> keys;
	keys.reserve(wordlen);
	while (wordlen-- > 0)
	{
		wchar_t ch;
		if (wordlen > 0 || matchType == Conjugation::MATCH_FULL)
		{
			ch = pszConjSpec[wordlen];
		}
		else
		{
			switch (matchType)
			{
			case Conjugation::MATCH_EXCEPT_CHO:
				ch = MakeMaskKey_ExceptCho(&charLast);
				break;
			case Conjugation::MATCH_JOONG_AND_NOZONG:
				ch = MakeMaskKey_JoongNoZong(&charLast);
				break;
			case Conjugation::MATCH_ZONG_ONLY:
				//ch = MakeMaskKey_ZongOnly(&charLast);
				ch = charLast.GetZong();
				if (ch == 0)
					ch = charLast.GetCho();
				ASSERT(ch > 0);
				break;

			default:
				ASSERT(0);
			}
		}

		keys.push_back(ch);
	}

	Insert(pNewConj, keys, 0);
	return pNewConj;
}

wchar_t ConjTreeNode::MakeMaskKey_ExceptCho(const Character* pChar)
{
	wchar_t joong = pChar->GetJoong();
	wchar_t zong = pChar->GetZong();
	ASSERT(joong > 0 && zong > 0);
	return ((joong << 2) ^ zong);
}

wchar_t ConjTreeNode::MakeMaskKey_JoongNoZong(const Character* pChar)
{
	wchar_t joong = pChar->GetJoong();
	ASSERT(joong > 0);
	return joong;
}

wchar_t ConjTreeNode::MakeMaskKey_ZongOnly(const Character* pChar)
{
	wchar_t zong = pChar->GetZong();
	ASSERT(zong > 0);
	return zong;
}

void ConjTreeNode::Insert(Conjugation* pItem, const std::vector<wchar_t>& keys, int depth)
{
	if (depth == keys.size())
	{
		m_items.push_back(pItem);
		return;
	}

	wchar_t ch = keys.at(depth);
	ConjTreeNode* pChild;
	auto itr = m_children.find(ch);
	if (itr == m_children.end())
	{
		pChild = new ConjTreeNode();
		m_children[ch] = pChild;
	}
	else
	{
		pChild = itr->second;
	}

	pChild->Insert(pItem, keys, depth +1);
}

void ConjTreeNode::FindMatch(const CharString& str, int depth, std::vector<Conjugation*>& matchedConjs)
{
	int len = str.GetLength();
	if (++depth <= len)
	{
		const Character* pChar = str.GetCharacter(len - depth);

		// try full match
		auto itr = m_children.find(pChar->GetCode());
		if (itr != m_children.end())
		{
			itr->second->FindMatch(str, depth, matchedConjs);
		}

		ASSERT(pChar->GetCategory() == Character::CT_HANGUL_COMPOSED);

		if (pChar->HasZong())
		{
			// try zong-only match
			itr = m_children.find(MakeMaskKey_ZongOnly(pChar));
			if (itr != m_children.end())
			{
				itr->second->FindMatch(str, depth, matchedConjs);
			}

			// try except-cho match
			itr = m_children.find(MakeMaskKey_ExceptCho(pChar));
			if (itr != m_children.end())
			{
				itr->second->FindMatch(str, depth, matchedConjs);
			}
		}
		else
		{
			// try joong match
			itr = m_children.find(MakeMaskKey_JoongNoZong(pChar));
			if (itr != m_children.end())
			{
				itr->second->FindMatch(str, depth, matchedConjs);
			}
		}
	}

	if (!m_items.empty())
	{
		matchedConjs.insert(matchedConjs.end(), m_items.begin(), m_items.end());
	}
}

struct DecomposingPredicate
{
	DecomposingPredicate(const CharString& str) : remain(str)
	{
		pTail = NULL;
	}

	CharString remain;
	Conjugation* pTail;
	std::vector<Conjugation*> pretails;
};

std::vector<PosInfo*> Dictionary::MatchPredicate(const CharString& str)
{
	std::vector<PosInfo*> ret;
	std::deque<DecomposingPredicate> Q;

	auto addMatched = [&ret](DecomposingPredicate& curr, PosInfo* pMatchedPred) {
		pMatchedPred->AddMetas(curr.pTail->m_metas);
		for (auto pretail : curr.pretails)
		{
			pMatchedPred->AddMetas(pretail->m_metas);
		}

		ret.push_back(pMatchedPred);
	};

	// match against irregular conjugations
	{
		int len = str.GetLength();
		PosTreeNode* pNode = m_rootIrrConjs.FindChild(str.GetCharCode(--len));
		while (pNode != NULL && len > 0)
		{
			pNode = pNode->FindChild(str.GetCharCode(--len));
		}

		if (pNode != NULL)
		{
			const PosInfoIrrConj* irrconj = (PosInfoIrrConj*) pNode->_GetFirstItem();
			PosInfo* pIrrPred = irrconj->GetRoot()->Clone();
			pIrrPred->AddMetas(irrconj->GetTailConj()->m_metas);
			ret.push_back(pIrrPred);
			return ret;
		}
	}

	Q.push_back(DecomposingPredicate(str));
	while (!Q.empty())
	{
		DecomposingPredicate curr = Q.front();
		Q.pop_front();
		//WidePrintf(L"MatchPredicateBegin: %s\n", curr.remain.GetString().c_str());

		PosInfo* pRoot = NULL;

		// 조사매칭

		std::vector<Conjugation*> matched;

		// 어말어미 매칭
		if (curr.pTail == NULL)
		{
			m_rootConjTails.FindMatches(curr.remain, matched);
			for (auto matchCount = matched.size(); matchCount > 0; )
			{
				auto conj = matched.at(--matchCount);
				auto remainStr = curr.remain.GetConjugationRemoved(conj);
				if (remainStr.GetLength() == 0)
					continue;

				if (matchCount > 0)
				{
					ASSERT(curr.pretails.empty());
					auto spawn = DecomposingPredicate(remainStr);
					spawn.pTail = conj;
					Q.push_back(spawn);
				}
				else
				{
					curr.remain = remainStr;
					curr.pTail = conj;
					break;
				}
			}
		}

		if (curr.pTail == NULL)
		{
			// 매칭되는 어말어미 못찾음
			continue;
		}


	_findPredicateRoot:

		// 어간매칭
		// TODO: 개-맛있다 등 접두어 처리
//		WidePrintf(L"MatchPredRoot: %s\n", curr.remain.GetString().c_str());

		PosTreeNode* pNode = m_pActualPredicatesRoot;
		int len = curr.remain.GetLength();
		while (len > 0)
		{
			pNode = pNode->FindChild(curr.remain.GetCharCode(--len));
			if (pNode == NULL)
				break;
		}

		if (pNode != NULL)
		{
			pNode->ForEachItem([&pRoot](PosInfo* pPosInfo) {
				//WidePrintf(L"Root matched: %s", spPosInfo->GetString().c_str());
				ASSERT(pRoot == NULL);
				pRoot = pPosInfo;
			});

			addMatched(curr, pRoot->Clone());
			continue;
		}
		else
		{
			wchar_t lastCh = curr.remain.GetCharCode(-1);
			if (lastCh == L'하')
			{
				//WidePrintf(L"try 하 predicate: %ls\n", curr.remain.GetString().c_str());
				PosInfo* pNoun = MatchNoun(curr.remain.GetSubstring(0, curr.remain.GetLength() - 1));
				if (pNoun != NULL)
				{
					//WidePrintf(L"Noun(%ls) matched.\n", pNoun->GetString().c_str());
					addMatched(curr, PosInfo::CreateNounPredicate(&curr.remain));
					continue;
				}
			}
		}

		// 선어말어미 및 TODO:조동사 매칭
		matched.clear();
		m_rootConjPretails.FindMatches(curr.remain, matched);
		if (matched.size() > 0)
		{
			ASSERT(matched.size() == 1);	// FIXME
			for (auto conj : matched)
			{
				curr.remain = curr.remain.GetConjugationRemoved(conj);
				curr.pretails.push_back(conj);
			}

			goto _findPredicateRoot;
		}

		

/*
		pNode = &m_rootIrrConjs;
		len = curr.remain.GetLength();
		while (len > 0)
		{
			pNode = pNode->FindChild(curr.remain.GetCharCode(--len));
			if (pNode == NULL)
				break;
		}
		
		if (pNode != NULL)
		{
			pNode->ForEachItem([](PosInfo* pPosInfo) {
				PosInfoIrrConj* pIrr = static_cast<PosInfoIrrConj*>(pPosInfo);
				WidePrintf(L"IrrConj matched: root=%ls,tail=%ls", pIrr->GetRoot()->GetString().c_str(), pIrr->GetTailConj()->GetString().c_str());
			});
		}
*/
	}
	
	return ret;
}

////////////////////////////////////////////////////////////////////////////////

Conjugation* Dictionary::AddTailConj(const wchar_t* pszSpec, const std::initializer_list<EPosMeta>& metas)
{
	return m_rootConjTails.Add(pszSpec, metas);
}

Conjugation* Dictionary::AddPretailConj(const wchar_t* pszSpec, const std::initializer_list<EPosMeta>& metas)
{
	return m_rootConjPretails.Add(pszSpec, metas);
}

void Dictionary::InitData_Conjugations()
{
	// 종결어미, 평서형
	AddTailConj(L"다", { POSMETA_PRED_ENDING });
	AddTailConj(L"는다", { POSMETA_PRED_ENDING });
	AddTailConj(L"ㄴ다", { POSMETA_PRED_ENDING });
	AddTailConj(L"습니다", { POSMETA_PRED_ENDING });
	AddTailConj(L"ㅂ니다", { POSMETA_PRED_ENDING });
	AddTailConj(L"네", { POSMETA_PRED_ENDING });
	AddTailConj(L"느니라", { POSMETA_PRED_ENDING });
	AddTailConj(L"렷다", { POSMETA_PRED_ENDING });
	AddTailConj(L"마", { POSMETA_PRED_ENDING });

	// 종결어미, 의문형
	AddTailConj(L"ㄹ까요", { POSMETA_PRED_ENDING, POSMETA_PRED_ENDING_QUESTION, POSMETA_PRED_POLITENESS });
	AddTailConj(L"ㄹ까", { POSMETA_PRED_ENDING, POSMETA_PRED_ENDING_QUESTION });
	AddTailConj(L"ㄹ니까요", { POSMETA_PRED_ENDING, POSMETA_PRED_ENDING_QUESTION, POSMETA_PRED_POLITENESS });
	AddTailConj(L"ㄹ니까", { POSMETA_PRED_ENDING, POSMETA_PRED_ENDING_QUESTION });
	AddTailConj(L"느냐", { POSMETA_PRED_ENDING, POSMETA_PRED_ENDING_QUESTION });
	AddTailConj(L"니", { POSMETA_PRED_ENDING, POSMETA_PRED_ENDING_QUESTION, POSMETA_PRED_INFORMAL });
	AddTailConj(L"는가", { POSMETA_PRED_ENDING, POSMETA_PRED_ENDING_QUESTION });
	AddTailConj(L"나", { POSMETA_PRED_ENDING, POSMETA_PRED_ENDING_QUESTION });
	AddTailConj(L"오", { POSMETA_PRED_ENDING, POSMETA_PRED_ENDING_QUESTION });	// CHECK
	
	// 종결어미, 감탄형
	AddTailConj(L"로구나", { POSMETA_PRED_ENDING, POSMETA_PRED_ENDING_EXCLAMATION });
	AddTailConj(L"구나", { POSMETA_PRED_ENDING, POSMETA_PRED_ENDING_EXCLAMATION });
	AddTailConj(L"군", { POSMETA_PRED_ENDING, POSMETA_PRED_ENDING_EXCLAMATION });
	AddTailConj(L"어라", { POSMETA_PRED_ENDING, POSMETA_PRED_ENDING_EXCLAMATION }); // CHECK
	AddTailConj(L"아나", { POSMETA_PRED_ENDING, POSMETA_PRED_ENDING_EXCLAMATION });

	// 종결어미, 명령형
	AddTailConj(L"ㅂ시오", { POSMETA_PRED_ENDING, POSMETA_PRED_ENDING_IMPERATIVE });
	AddTailConj(L"려무나", { POSMETA_PRED_ENDING, POSMETA_PRED_ENDING_IMPERATIVE });
	AddTailConj(L"어라", { POSMETA_PRED_ENDING, POSMETA_PRED_ENDING_IMPERATIVE }); // CHECK
	AddTailConj(L"아라", { POSMETA_PRED_ENDING, POSMETA_PRED_ENDING_IMPERATIVE });
	AddTailConj(L"게", { POSMETA_PRED_ENDING, POSMETA_PRED_ENDING_IMPERATIVE });
	AddTailConj(L"오", { POSMETA_PRED_ENDING, POSMETA_PRED_ENDING_IMPERATIVE });

	// 종결어미, 청유형
	AddTailConj(L"자", { POSMETA_PRED_ENDING, POSMETA_PRED_ENDING_REQUEST });
	AddTailConj(L"세", { POSMETA_PRED_ENDING, POSMETA_PRED_ENDING_REQUEST });
	AddTailConj(L"ㅂ시다", { POSMETA_PRED_ENDING, POSMETA_PRED_ENDING_REQUEST });
	AddTailConj(L"시지요", { POSMETA_PRED_ENDING, POSMETA_PRED_ENDING_REQUEST });


	// 비종결어미, 연결어미, 대등적
	AddTailConj(L"자", { POSMETA_PRED_CONJUNCT });
	AddTailConj(L"며", { POSMETA_PRED_CONJUNCT });
	AddTailConj(L"으나", { POSMETA_PRED_CONJUNCT });
	AddTailConj(L"나", { POSMETA_PRED_CONJUNCT });
	AddTailConj(L"는데", { POSMETA_PRED_CONJUNCT });
	AddTailConj(L"ㄴ데", { POSMETA_PRED_CONJUNCT });

	// 비종결어미, 연결어미, 종속적
	AddTailConj(L"면", { POSMETA_PRED_CONJUNCT });
	AddTailConj(L"니", { POSMETA_PRED_CONJUNCT });
	AddTailConj(L"자", { POSMETA_PRED_CONJUNCT });
	AddTailConj(L"어서", { POSMETA_PRED_CONJUNCT });
	AddTailConj(L"서", { POSMETA_PRED_CONJUNCT });

	// 비종결어미, 연결어미, 보조적
	AddTailConj(L"아", { POSMETA_PRED_CONJUNCT });
	AddTailConj(L"어", { POSMETA_PRED_CONJUNCT });
	AddTailConj(L"게", { POSMETA_PRED_CONJUNCT });
	AddTailConj(L"지", { POSMETA_PRED_CONJUNCT });
	AddTailConj(L"고", { POSMETA_PRED_CONJUNCT });


	// 비종결어미, 전성어미, 명사형
	AddTailConj(L"기", { POSMETA_PRED_TRANS_NOUN });
	AddTailConj(L"음", { POSMETA_PRED_TRANS_NOUN });
	AddTailConj(L"ㅁ", { POSMETA_PRED_TRANS_NOUN });

	// 비종결어미, 전성어미, 관형사형
	AddTailConj(L"은", { POSMETA_PRED_TRANS_DETERMINER });
	AddTailConj(L"는", { POSMETA_PRED_TRANS_DETERMINER });
	AddTailConj(L"ㄴ", { POSMETA_PRED_TRANS_DETERMINER });
	AddTailConj(L"ㄹ", { POSMETA_PRED_TRANS_DETERMINER });
	AddTailConj(L"던", { POSMETA_PRED_TRANS_DETERMINER });

	// 비종결어미, 전성어미, 부사형
	AddTailConj(L"니", { POSMETA_PRED_TRANS_ADVERB });
	AddTailConj(L"어서", { POSMETA_PRED_TRANS_ADVERB });
	AddTailConj(L"게", { POSMETA_PRED_TRANS_ADVERB });
	AddTailConj(L"도록", { POSMETA_PRED_TRANS_ADVERB });


	// 선어말어미
	AddPretailConj(L"시", { POSMETA_PRED_HONORIFIC });
	AddPretailConj(L"었", { POSMETA_PRED_PAST });
	AddPretailConj(L"였", { POSMETA_PRED_PAST });
	AddPretailConj(L"았", { POSMETA_PRED_PAST });
	AddPretailConj(L"했:하", { POSMETA_PRED_PAST });
	AddPretailConj(L"혔:히", { POSMETA_PRED_PAST });
	AddPretailConj(L"놨:놓", { POSMETA_PRED_PAST });
	AddPretailConj(L"왔:오", { POSMETA_PRED_PAST });
	AddPretailConj(L"냈:내", { POSMETA_PRED_PAST });
	AddPretailConj(L"졌:지", { POSMETA_PRED_PAST });
	AddPretailConj(L"봤:보", { POSMETA_PRED_PAST });
	AddPretailConj(L"쳤:치", { POSMETA_PRED_PAST });
	AddPretailConj(L"셨", { POSMETA_PRED_HONORIFIC, POSMETA_PRED_PAST });
	AddPretailConj(L"는", { POSMETA_PRED_PRESENT });
	AddPretailConj(L"겠", { POSMETA_PRED_FUTURE });
	AddPretailConj(L"옵", { POSMETA_PRED_POLITENESS });
}

END_NAMESPACE_NTPOSTAG

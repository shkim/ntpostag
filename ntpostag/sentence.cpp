#include "stdafx.h"
#include "postag.h"
#include "util.h"

BEGIN_NAMESPACE_NTPOSTAG

Sentence::Sentence(std::deque<Part>& parts)
{
	m_parts.reserve(parts.size());

	for (auto src : parts)
	{
		m_parts.push_back(PartWithTokens(src));
	}

	ASSERT(!m_parts.empty());
}

std::wstring Sentence::GetString() const
{
	Character* pFrom = m_parts.front().pFrom;
	Character* pTill = m_parts.back().pTill;
	auto len = (pTill - pFrom) +1;

	std::wstring str;
	str.reserve(len);
	for (auto part : m_parts)
	{
		for (auto p = part.pFrom; p <= part.pTill; p++)
		{
			str.push_back(p->GetCode());
		}
	}

	return str;
}

std::vector<std::wstring> Sentence::GetQuotedStrings() const
{
	std::vector<std::wstring> ret;

	Character* pLastTill = m_parts.front().pFrom;

	for (auto part : m_parts)
	{
		if (part.pTill <= pLastTill)
		{
			continue;
		}

		if (part.depth == 1 && (part.type == PART_QUOTATION || part.type == PART_EMPHASIS))
		{
			std::wstring str;
			str.reserve(part.pPartTill - part.pFrom);
			for (Character* p = part.pFrom; p <= part.pPartTill; p++)
			{
				str.push_back(p->GetCode());
			}

			pLastTill = part.pPartTill;
			ret.push_back(str);
		}
	}

	return ret;
}

/*
void Sentence::DumpTokens()
{
	for (size_t i = 0; i < m_tokens.size(); i++)
	{
		auto token = m_tokens[i];
		WidePrintf(L"[t%d] '%ls' ", i, token.GetString().c_str());
		auto pos = token.GetPos();
		if (!pos)
		{
			WidePrintf(L"NoPos\n");
			continue;
		}

		pos->PrintDebugInfo();
		WidePrintf(L"\n");		
	}
}

void TokenPosNominee::PrintDebugInfo()
{
	for (PosInfo* pInfo : m_poses)
	{
		WidePrintf(L"[%ls,%ls:", pInfo->GetString().c_str(), g_posdic.GetPosTypeName(pInfo->GetType()).c_str());
		for (auto meta : pInfo->GetMetas())
		{
			auto metaName = g_posdic.GetMetaName(meta);
			WidePrintf(L"%ls,", metaName.c_str());
		}
		WidePrintf(L"],");
	}
}
*/

enum TokenCategory
{
	TC_SPACE,
	TC_PUNCT,
	TC_LETTER
};

TokenCategory ToTokenCategory(Character::Category cate)
{
	if (cate <= Character::CT_NEWLINE)
	{
		return TC_SPACE;
	}
	else if (cate < Character::CT_SYMBOL)
	{
		return TC_PUNCT;
	}
	else
	{
		return TC_LETTER;
	}
}

void Sentence::AnalyzePos()
{
	// prepare tokens
	for (auto part : m_parts)
	{
		ASSERT(part.tokens.empty());

		const Character* pFrom = part.pFrom;
		TokenCategory curCate = ToTokenCategory(pFrom->GetCategory());

		const Character* pTill = pFrom;
		while (pTill < part.pTill)
		{
			const Character* pCurTill = pTill;
			++pTill;
			TokenCategory nextCate = ToTokenCategory(pTill->GetCategory());
			if (curCate == nextCate)
				continue;

			if (TC_SPACE != curCate)
			{
				part.tokens.emplace_back(pFrom, pCurTill);
			}

			curCate = nextCate;
			pFrom = pTill;
		}

		if (TC_SPACE != curCate)
		{
			part.tokens.emplace_back(pFrom, pTill);
		}

		for (Token& token : part.tokens)
		{
			token.GatherPossiblePoses();
		}
	}

	/*
	std::vector<Sentence::PosResult> ret;

	for (Token& token : m_tokens)
	{
		auto pos = token.GetPos();
		if (!pos)
		{
			WidePrintf(L"GetPos(%ls) NULL\n", token.GetString().c_str());
			continue;
		}

		for (PosInfo* pInfo : pos->m_poses)
		{
			Sentence::PosResult item;
			if (pos->m_poses.size() == 1)
			{
				item.word = token.GetString();
			}
			else
			{
				item.word = pInfo->GetString();
			}

			item.pInfo = pInfo;	// CHECKME
			ret.push_back(item);
		}
	}

	return ret;*/
}

#ifndef _NOUSE_CPPREST

using namespace web;

json::value Sentence::GetResultJson()
{
	json::value ret;
	ret[U("aa")] = json::value::number(33);
	return ret;
}

web::json::value Sentence::GetUnknownWords()
{
	json::value ret = json::value::array(3);
	ret[0] = json::value::string(U("하늘"));
	ret[1] = json::value::string(U("구름"));
	ret[2] = json::value::string(U("바다"));
	return ret;
}

#endif // _NOUSE_CPPREST

END_NAMESPACE_NTPOSTAG

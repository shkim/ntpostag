#include "stdafx.h"
#include "postag.h"
#include "util.h"

#include <boost/circular_buffer.hpp>

BEGIN_NAMESPACE_NTPOSTAG

static std::map<wchar_t, wchar_t> s_quotePairs;
static std::map<wchar_t, wchar_t> s_alternativeQuotePairs;
static std::map<wchar_t, Sentence::PartType> s_char2PartTypes;

void CreateQuotePairTable()
{
	s_quotePairs[L'('] = L')';
	s_quotePairs[L'{'] = L'}';
	s_quotePairs[L'['] = L']';
	s_quotePairs[L'<'] = L'>';
	s_quotePairs[L'"'] = L'"';
	s_quotePairs[L'\''] = L'\'';
	s_quotePairs[L'`'] = '\'';
	s_quotePairs[L'‘'] = L'’';
	s_quotePairs[L'‛'] = L'’';
	s_quotePairs[L'“'] = L'”';
	s_quotePairs[L'‟'] = L'”';

	s_alternativeQuotePairs[L'"'] = L'”';
	s_alternativeQuotePairs[L'“'] = L'„';
	s_alternativeQuotePairs[L'‟'] = L'„';
	s_alternativeQuotePairs[L'\''] = L'’';
	s_alternativeQuotePairs[L'`'] = L'’';

	s_char2PartTypes[L'"'] = Sentence::PART_QUOTATION;
	s_char2PartTypes[L'“'] = Sentence::PART_QUOTATION;
	s_char2PartTypes[L'‟'] = Sentence::PART_QUOTATION;
	
	s_char2PartTypes[L'['] = Sentence::PART_QUOTATION;
	s_char2PartTypes[L'<'] = Sentence::PART_QUOTATION;
	s_char2PartTypes[L'{'] = Sentence::PART_QUOTATION;

	s_char2PartTypes[L'\''] = Sentence::PART_EMPHASIS;
	s_char2PartTypes[L'`'] = Sentence::PART_EMPHASIS;
	s_char2PartTypes[L'‘'] = Sentence::PART_EMPHASIS;
	s_char2PartTypes[L'‛'] = Sentence::PART_EMPHASIS;

	s_char2PartTypes[L'('] = Sentence::PART_MODIFIER;	
}

static Sentence::PartType ToSentencePartType(wchar_t ch)
{
	auto itr = s_char2PartTypes.find(ch);
	if (itr != s_char2PartTypes.end())
	{
		return itr->second;
	}

	return Sentence::PART_EMPHASIS;	// FIXME: default?
}

void DestroyQuotePairTable()
{
	s_quotePairs.clear();
	s_alternativeQuotePairs.clear();
	s_char2PartTypes.clear();
}

bool Paragraph::Set(const wchar_t* pszInputParagraph, int chlen, bool bNormalizeCharacters)
{
	if (!m_sentences.empty())
		m_sentences.clear();

	if (chlen < 0)
	{
		chlen = (int)wcslen(pszInputParagraph);
	}

	m_charbuff.resize(chlen);

	Character::Category prevCate = Character::CT_IGNORE;
	size_t iDst = 0;
	for (auto iSrc = 0; iSrc < chlen; iSrc++)
	{
		m_charbuff[iDst].Set(pszInputParagraph[iSrc]);

		Character::Category curCate = m_charbuff[iDst].GetCategory();
		if (curCate == Character::CT_IGNORE)
		{
			continue;
		}

		if (curCate == Character::CT_SPACE)
		{
			if (prevCate == Character::CT_SPACE)
			{
				// ignore (duplicated) second space
				continue;
			}
		}

		if (bNormalizeCharacters)
		{
			switch (curCate)
			{
			case Character::CT_SPACE:
				m_charbuff[iDst].ForceSetCode(' ');
				break;

			// TODO: convert wide -> narrow
			case Character::CT_NUMBER:
			case Character::CT_ALPHABET:
				break;

			default:
				;
			}
					
		}

		prevCate = curCate;
		iDst++;
	}

	while (iDst > 0)
	{
		prevCate = m_charbuff[iDst - 1].GetCategory();
		if (prevCate == Character::CT_SPACE || prevCate == Character::CT_NEWLINE)
		{
			--iDst;	// TrimRight
			continue;
		}

		break;
	}

	if (iDst == 0)
	{
		// empty input string
		return false;
	}

	if (iDst != chlen)
	{
		// truncate
		m_charbuff.resize(iDst);
	}

	return SplitSentences();
}

struct QuoteCharItem
{
	wchar_t ch;
	size_t location;
	unsigned int closerLocation;

	QuoteCharItem(wchar_t a, size_t b)
	{
		ch = a;
		location = b;
		closerLocation = 0;
	}
};

struct QuoteRetryItem
{
	wchar_t ch;
	size_t location;
	int length;	// > 1 if '', <<, [[ etc

	QuoteRetryItem(wchar_t a, size_t b, int c)
	{
		ch = a;
		location = b;
		length = c;
	}
};

struct PairedQuoteInfo
{
	size_t from, to;

	PairedQuoteInfo(size_t a, size_t b)
	{
		from = a;
		to = b;
	}
};

static std::deque<Sentence::Part> PartitionQuotedBlocks(Character* const pCharBase, Character* pCharEnd, std::vector<QuoteCharItem>& quoteChars)
{
	std::vector<QuoteRetryItem> retryList;
	std::list<PairedQuoteInfo> pairList;

	auto numQuoteChars = quoteChars.size();
	for (auto i = 1; i < numQuoteChars; i++)
	{
		QuoteCharItem& opener = quoteChars[i -1];
		const wchar_t openerChar = opener.ch;
		if (openerChar == 0)
			continue;

		if (quoteChars[i].ch == openerChar && quoteChars[i].location == opener.location + 1)
		{
			// sequenced quote char handling, eg: ''inner'' , <<inner>> , [[[inner]]]
			int seqlen = 2;
			i++;
			while (quoteChars[i].ch == openerChar && quoteChars[i].location == opener.location + 1)
			{
				seqlen++;
				i++;
			}

			retryList.emplace_back(openerChar, opener.location, seqlen);
			continue;
		}


		auto itr = s_quotePairs.find(openerChar);
		if (itr == s_quotePairs.end())
		{
			ASSERT(!"Unknown quote opener (lookup table must be edited!)");
			continue;
		}

		wchar_t closerChar = itr->second;
		size_t idxCloser = 0;
		for (auto j = i; j < numQuoteChars; j++)
		{
			if (quoteChars[j].ch == closerChar)
			{
				// closer found
				idxCloser = j;
				break;
			}
		}

		if (idxCloser == 0)
		{
			// try alternative closer
			itr = s_alternativeQuotePairs.find(openerChar);
			if (itr != s_alternativeQuotePairs.end())
			{
				closerChar = itr->second;
				idxCloser = 0;
				for (auto j = i; j < numQuoteChars; j++)
				{
					if (quoteChars[j].ch == closerChar)
					{
						// closer found
						idxCloser = j;
						break;
					}
				}
			}
		}

		if (idxCloser > 0)
		{
			QuoteCharItem& closer = quoteChars[idxCloser];
			auto distance = closer.location - opener.location;
			if (distance <= 1)
			{
				// empty quote: [],(),<> etc ==> ignore
			}
			else
			{
				pairList.emplace_back(opener.location, closer.location);
			}

			closer.ch = 0;
			continue;
		}
		else
		{
			retryList.emplace_back(openerChar, opener.location, 1);
		}
	}

	numQuoteChars = retryList.size();
	if (numQuoteChars > 1)
	{		
		for (auto i = 1; i < numQuoteChars; i++)
		{
			QuoteRetryItem& opener = retryList[i - 1];
			if (opener.ch == 0)
				continue;

			auto itr = s_quotePairs.find(opener.ch);
			if (itr == s_quotePairs.end())
			{
				ASSERT(!"Unknown quote opener (lookup table must be edited!)");
				continue;
			}

			wchar_t closerChar = itr->second;

			itr = s_alternativeQuotePairs.find(opener.ch);
			wchar_t closerCharAlt = (itr != s_alternativeQuotePairs.end()) ? itr->second : -1;

			size_t idxCloser = 0;
			for (auto j = i; j < numQuoteChars; j++)
			{
				if (retryList[j].length == opener.length
					&& retryList[j].ch == closerChar
					&& retryList[j].ch == closerCharAlt)
				{
					// closer found
					idxCloser = j;
					break;
				}
			}

			if (idxCloser > 0)
			{
				QuoteRetryItem& closer = retryList[idxCloser];
				auto distance = closer.location - (opener.location + opener.length);
				if (distance <= 0)
				{
					// empty quote: <<>>, [[]] etc ==> ignore
				}
				else
				{
					pairList.emplace_back(opener.location, closer.location + closer.length -1);
				}
			}
		}

		// TODO: remained unpaired quotes
	}


	std::deque<Sentence::Part> parts;
	Sentence::Part curDepthPart;
	curDepthPart.pFrom = pCharBase;
	curDepthPart.pTill = pCharEnd;
	curDepthPart.pPartTill = pCharEnd;
	curDepthPart.partId = 0;
	curDepthPart.depth = 0;
	curDepthPart.type = ToSentencePartType(curDepthPart.pFrom->GetCode());
	
	if (pairList.empty())
	{
		parts.push_back(curDepthPart);
	}
	else
	{
		pairList.sort([](const PairedQuoteInfo& a, const PairedQuoteInfo& b) { return a.from < b.from; });

		// check if pairs corssing each other
		// TODO

		std::deque<Sentence::Part> rightParts;
		int partIdCounter = 0;

		for (PairedQuoteInfo& pair : pairList)
		{
			if (curDepthPart.pTill < &pCharBase[pair.from])
			{
				parts.push_back(curDepthPart);

				ASSERT(!rightParts.empty());
				curDepthPart = rightParts.front();
				rightParts.pop_front();
			}

			if (curDepthPart.pFrom < &pCharBase[pair.from])
			{
				// add left
				Sentence::Part part = curDepthPart;
				part.pTill = &pCharBase[pair.from -1];
				parts.push_back(part);
			}

			if (&pCharBase[pair.to] < curDepthPart.pTill)
			{
				// add right
				Sentence::Part part = curDepthPart;
				part.pFrom = &pCharBase[pair.to + 1];
				rightParts.push_front(part);
			}

			curDepthPart.depth++;
			curDepthPart.partId = ++partIdCounter;
			curDepthPart.pFrom = &pCharBase[pair.from];
			curDepthPart.pTill = &pCharBase[pair.to];
			curDepthPart.pPartTill = curDepthPart.pTill;
			curDepthPart.type = ToSentencePartType(curDepthPart.pFrom->GetCode());
		}

		parts.push_back(curDepthPart);
		
		for (auto rpart : rightParts)
		{
			parts.push_back(rpart);
		}
	}

#if 0
	int seq = 0;
	for (auto part : parts)
	{
		std::wstring str;
		for (const Character* p = part.pFrom; p <= part.pTill; p++)
		{
			str.push_back(p->GetCode());
		}
		
		WidePrintf(L"[%d:%d] %s(%d) d=%d\n", seq++, part.partId, str.c_str(), str.length(), part.depth);
	}
#endif

	return parts;
}

static bool TrimSentenceParts(std::deque<Sentence::Part>& parts)
{
	// trim left
	while (!parts.empty())
	{
		Sentence::Part& firstPart = parts.front();
		while (firstPart.pFrom <= firstPart.pTill)
		{
			if (firstPart.pFrom->GetCategory() != Character::CT_SPACE)
				break;

			firstPart.pFrom++;
		}

		if (firstPart.pFrom <= firstPart.pTill)
			break;

		parts.pop_front();
	}

	// trim right
	while (!parts.empty())
	{
		Sentence::Part& lastPart = parts.back();
		while (lastPart.pFrom <= lastPart.pTill)
		{
			if (lastPart.pTill->GetCategory() != Character::CT_SPACE)
				break;

			lastPart.pTill--;
		}

		if (lastPart.pFrom <= lastPart.pTill)
			break;

		parts.pop_back();
	}

	return !parts.empty();
}

void Paragraph::SplitByDotThenAddSentence(std::deque<Sentence::Part>& parts)
{
	auto numParts = parts.size();
	for (auto i = 0; i < numParts; i++)
	{
		Sentence::Part& part = parts.at(i);
		if (part.depth > 0)
			continue;

		Character* pDotChar = NULL;
		for (Character* pChar = part.pFrom; pChar <= part.pTill; pChar++)
		{
			if (pChar->GetCategory() == Character::CT_PUNCMARK_END)
			{
				pDotChar = pChar;
				break;
			}
		}

		if (pDotChar == NULL)
		{
			// no dot found. don't split in this part
			continue;
		}
	}

	if (TrimSentenceParts(parts))
		m_sentences.emplace_back(parts);
}

bool Paragraph::SplitSentences()
{
	ASSERT(m_charbuff.size() > 0);
	Character* pLimit = &m_charbuff[m_charbuff.size() - 1];
	Character* pCurrent = &m_charbuff[0];

	std::vector<std::pair<Character*, Character*>> tempSentences;

	Character* pStart = pCurrent;
	auto addTmpSen = [&tempSentences](Character* start, Character* end) {
		while (start->GetCategory() == Character::CT_NEWLINE)
		{
			if (++start > end) 
				break;
		}			

		while (end->GetCategory() == Character::CT_NEWLINE)
		{
			if (--end < start)
				break;
		}			

		if (start <= end)
		{
			tempSentences.push_back(std::make_pair(start, end));
		}
	};

	// Pass 1: force split sentences by two or more continuous new lines.
	boost::circular_buffer<wchar_t> nlchars(8);
	while (pCurrent <= pLimit)
	{
		if (pCurrent->GetCategory() == Character::CT_NEWLINE)
		{
			nlchars.push_back(pCurrent->GetCode());
		}
		else
		{
			auto nlcnt = nlchars.size();
			if (nlcnt > 0)
			{
				if (nlcnt == 2)
				{
					// if "\r\n" or "\n\r", do not treat as two newlines.
					bool hasCR = nlchars[0] == '\r' || nlchars[1] == '\r';
					bool hasLF = nlchars[0] == '\n' || nlchars[1] == '\n';
					if (hasCR && hasLF)
					{
						ASSERT(pCurrent[-1].GetCategory() == Character::CT_NEWLINE);
						pCurrent[-1].ForceSetCategory(Character::CT_IGNORE);
						nlcnt = 1;
					}
				}

				if (nlcnt >= 2)
				{
					addTmpSen(pStart, &pCurrent[-1]);
					pStart = pCurrent;
				}

				nlchars.clear();
			}
		}

		++pCurrent;
	}

	addTmpSen(pStart, &pCurrent[-1]);

	for (auto tmpSen : tempSentences)
	{
		// Pass 2. collect quote chars and remove ignorable chars
		std::vector<QuoteCharItem> quoteChars;

		// trim left
		while (tmpSen.first->GetCategory() <= Character::CT_NEWLINE)
		{
			++tmpSen.first;
		}

		Character::Category prevCate = Character::CT_IGNORE;
		for (Character* pChar = tmpSen.first; pChar <= tmpSen.second; pChar++)
		{
			auto cate = pChar->GetCategory();

			if (cate == Character::CT_NEWLINE)
			{
				if (prevCate == Character::CT_SPACE)
				{
					// remove redundant spaces
					cate = Character::CT_IGNORE;
				}
				else
				{
					pChar->Set(' ');
					prevCate = Character::CT_SPACE;
					continue;
				}
			}
			else if (cate == Character::CT_SPACE)
			{
				if (prevCate == Character::CT_SPACE)
				{
					// remove redundant spaces
					cate = Character::CT_IGNORE;
				}
			}

			if (cate == Character::CT_IGNORE)
			{
				auto remainChars = tmpSen.second - pChar;
				memmove(pChar, &pChar[1], remainChars * sizeof(Character));
				--tmpSen.second;
				--pChar;
				continue;
			}

			if (cate == Character::CT_PUNCMARK_QUOTE)
			{
				if (pChar->GetCode() == '\'' && prevCate == Character::CT_ALPHABET 
					&& pChar < tmpSen.second && pChar[1].GetCategory() == Character::CT_ALPHABET)
				{
					// english apostrophe
				}
				else
				{
					auto iLocation = pChar - tmpSen.first;
					quoteChars.emplace_back(pChar->GetCode(), iLocation);
				}
			}

			prevCate = cate;
		}

		// trim right
		while (tmpSen.second->GetCategory() == Character::CT_SPACE)
		{
			--tmpSen.second;
		}

		// Pass 3: partition quoted blocks
		ASSERT(tmpSen.first <= tmpSen.second);
		auto parts = PartitionQuotedBlocks(tmpSen.first, tmpSen.second, quoteChars);

		// Pass 4: logically split sentence by dot character
		SplitByDotThenAddSentence(parts);
	}

	return !m_sentences.empty();
}

END_NAMESPACE_NTPOSTAG
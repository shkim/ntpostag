#include "stdafx.h"
#include "postag.h"
#include "util.h"

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <sstream>
#include <fstream>
#include <codecvt>

//using namespace web;

BEGIN_NAMESPACE_NTPOSTAG

struct IdMetaPair
{
	EPosMeta eid;
	const wchar_t* word;
};

IdMetaPair metaPairs[] =
{
	{ POSMETA_PRED_FUTURE, L"미래" },
	{ POSMETA_SUBJECTIVE, L"주격" },
	{ POSMETA_OBJECTIVE, L"목적격" },
	{ POSMETA_TASTE, L"맛" },
	{ POSMETA_ACTION, L"행위" },
	{ POSMETA_TIME, L"시간" },
	{ POSMETA_ABSTRACT, L"추상" },

	{ POSMETA_ENTITY_FULLNAME, L"성명" },
	{ POSMETA_ENTITY_FIRSTNAME, L"이름" },
	{ POSMETA_ENTITY_LASTNAME, L"성씨" },
	{ POSMETA_ENTITY_APPELLATION, L"호칭" },
	{ POSMETA_ENTITY_PERSON, L"사람" },

	{ POSMETA_ENTITY_LOCATION_NAME, L"지명" },
	{ POSMETA_ENTITY_LOCATION_UNIT, L"지명결합" },
	{ POSMETA_ENTITY_LOCATION, L"장소" },

	{ POSMETA_ENTITY_ORGANIZATION_NAME, L"조직명" },
	{ POSMETA_ENTITY_ORGANIZATION_UNIT, L"조직" },

	{ POSMETA_ENTITY_NATION, L"국가명" },

	{ POSMETA_PERSON_POLITICIAN, L"정치인" },
	{ POSMETA_PERSON_SPORTSMAN, L"체육인" },
	{ POSMETA_PERSON_ENTERTAINER, L"연예인" }

};

IdMetaPair runtimeMetaPairs[] =
{
	{ POSMETA_PRED_ENDING, L"종결어미" },
	{ POSMETA_PRED_ENDING_QUESTION, L"의문형 종결어미" },
	{ POSMETA_PRED_ENDING_EXCLAMATION, L"감탄형 종결어미" },
	{ POSMETA_PRED_ENDING_IMPERATIVE, L"명령형 종결어미" },
	{ POSMETA_PRED_ENDING_REQUEST, L"청유형 종결어미" },
	{ POSMETA_PRED_CONJUNCT, L"연결어미" },
	{ POSMETA_PRED_TRANS_NOUN, L"명사형 전성어미" },
	{ POSMETA_PRED_TRANS_DETERMINER, L"관형사형 전성어미" },
	{ POSMETA_PRED_TRANS_ADVERB, L"부사형 전성어미" },

	{ POSMETA_PRED_HONORIFIC, L"높임" },
	{ POSMETA_PRED_POLITENESS, L"공손" },
	{ POSMETA_PRED_PRESENT, L"현재" },
	{ POSMETA_PRED_PAST, L"과거" },
	{ POSMETA_PRED_FUTURE, L"미래" },
	{ POSMETA_PRED_GUESS, L"추측" },
	{ POSMETA_PRED_INFORMAL, L"비격식" }
};

void Dictionary::InitTypeNames()
{
	for (size_t i = 0; i < sizeof(metaPairs) / sizeof(metaPairs[0]); i++)
	{
		m_mapMetaNames[metaPairs[i].eid] = metaPairs[i].word;
	}

	for (size_t i = 0; i < sizeof(runtimeMetaPairs) / sizeof(runtimeMetaPairs[0]); i++)
	{
		m_mapMetaNames[runtimeMetaPairs[i].eid] = runtimeMetaPairs[i].word;
	}

	m_mapPTypeNames[POSTYPE_NOUN] = L"명사";
	m_mapPTypeNames[POSTYPE_JOSA] = L"조사";
	m_mapPTypeNames[POSTYPE_PREDICATE] = L"용언";
	m_mapPTypeNames[POSTYPE_ADVERB] = L"부사";
	m_mapPTypeNames[POSTYPE_DETERMINER] = L"관형사";
	m_mapPTypeNames[POSTYPE_PUNCMARK] = L"문장부호";
	m_mapPTypeNames[POSTYPE_DETAIL_VERB] = L"동사";
	m_mapPTypeNames[POSTYPE_DETAIL_ADJECTIVE] = L"형용사";
}

static std::wstring ErrName(L"Error");

const std::wstring& Dictionary::GetMetaName(EPosMeta meta) const
{
	auto itr = m_mapMetaNames.find(meta);
	if (itr != m_mapMetaNames.end())
	{
		return itr->second;
	}

	ASSERT(!"Unknown Meta");
	return ErrName;
}

const std::wstring& Dictionary::GetPosTypeName(EPosType type) const
{
	auto itr = m_mapPTypeNames.find(type);
	if (itr != m_mapPTypeNames.end())
	{
		return itr->second;
	}

	ASSERT(!"Unknown postype");
	return ErrName;
}

////////////////////////////////////////////////////////////////////////////////

class MemoryBuffer
{
public:
	void Write(const void* pSrc, size_t len)
	{
		BYTE* p = (BYTE*)pSrc;
		m_buffer.insert(m_buffer.end(), p, &p[len]);
	}

	void Write(const MemoryBuffer& buf)
	{
		m_buffer.insert(m_buffer.end(), buf.m_buffer.begin(), buf.m_buffer.end());
	}

	void WriteInt32(size_t num)
	{
		ASSERT(num <= 0xFFFFFFFF);
		int val = (int)num;
		Write(&val, sizeof(val));
	}

	void WriteInt16(size_t num)
	{
		ASSERT(num <= 0xFFFF);
		short val = (short)num;
		Write(&val, sizeof(val));
	}

	void WriteInt8(size_t num)
	{
		ASSERT(num <= 0xFF);
		BYTE val = (BYTE)num;
		Write(&val, sizeof(val));
	}

	inline size_t GetSize()
	{
		return m_buffer.size();
	}

	inline const BYTE* GetBuffer()
	{
		return &m_buffer[0];
	}

private:
	std::vector<BYTE> m_buffer;
};

struct DicSrcItem
{
	// common
	std::wstring word;
	EPosType type;
	EPosType detailType;
	std::set<EPosMeta> metas;

	// if josa
	PosInfoJosa::JongCondition zong;

	// if irregular-conjugation
	std::wstring root;
	std::wstring tail;
};

class DicTreeNode
{
public:
	~DicTreeNode();
	void Insert(DicSrcItem* pItem, size_t depth, bool reverseDir);

	void WalkItems(std::function<void(const DicSrcItem* pItem)> fnCallback);
	DicSrcItem* FindByWord(const std::wstring& word, bool reverseDir, int depth=0);
	void Serialize(MemoryBuffer& outbuf, int& serialIdGen, EPosType postype);

	void _Dump(int depth);

private:
	std::map<wchar_t, DicTreeNode*> m_children;
	std::vector<DicSrcItem*> m_items;
};

DicTreeNode::~DicTreeNode()
{
	for (auto itr = m_children.begin(); itr != m_children.end(); ++itr)
	{
		delete itr->second;
	}
}

void DicTreeNode::Insert(DicSrcItem* pItem, size_t depth, bool reverseDir)
{
	size_t len = pItem->word.length();

	if (len == depth)
	{
		m_items.push_back(pItem);
		return;
	}

	ASSERT(depth < len);

	wchar_t ch;
	if (reverseDir)
	{
		ch = pItem->word.at(len - 1 - depth);
	}
	else
	{
		ch = pItem->word.at(depth);
	}

	DicTreeNode* pChild;
	auto itr = m_children.find(ch);
	if (itr == m_children.end())
	{
		pChild = new DicTreeNode();
		m_children[ch] = pChild;
	}
	else
	{
		pChild = itr->second;
	}

	pChild->Insert(pItem, depth + 1, reverseDir);
}

void DicTreeNode::WalkItems(std::function<void(const DicSrcItem* pItem)> fnCallback)
{
	for (DicSrcItem* pItem : m_items)
	{
		fnCallback(pItem);
	}

	for (auto itr : m_children)
	{
		itr.second->WalkItems(fnCallback);
	}
}

DicSrcItem* DicTreeNode::FindByWord(const std::wstring& word, bool reverseDir, int depth)
{
	if (depth < word.length())
	{
		size_t lookupIdx;
		if (reverseDir)
		{
			lookupIdx = word.length() - 1 - depth;
		}
		else
		{
			lookupIdx = depth;
		}

		wchar_t ch = word.at(lookupIdx);
		auto itr = m_children.find(ch);
		if (itr == m_children.end())
		{
			return NULL;
		}

		return itr->second->FindByWord(word, reverseDir, depth + 1);
	}
	else
	{
		ASSERT(depth == word.length() && word.length() > 0);
		ASSERT(m_items.size() == 1);
		return m_items.empty() ? NULL : m_items.at(0);
	}
}

void DicTreeNode::Serialize(MemoryBuffer& outbuf, int& serialIdGen, EPosType postype)
{
	outbuf.WriteInt32(++serialIdGen);

	size_t numChildren = m_children.size();
	outbuf.WriteInt16(numChildren);

	if (numChildren > 0)
	{
		std::vector<wchar_t> letters;
		letters.reserve(numChildren);
		for (auto itr : m_children)
		{
			letters.push_back(itr.first);
		}
		outbuf.Write(&letters[0], sizeof(wchar_t) * numChildren);

		for (auto itr : m_children)
		{
			itr.second->Serialize(outbuf, serialIdGen, postype);
		}
	}

	BYTE seqNum = 0;
	outbuf.WriteInt32(m_items.size());
	for (auto item : m_items)
	{
		outbuf.WriteInt8(++seqNum);

		outbuf.WriteInt8(item->type);

		switch (postype)
		{
		case POSTYPE_JOSA:
			outbuf.WriteInt8(item->zong);
			break;
		case POSTYPE_IRREGULAR_CONJUGATION:
			outbuf.WriteInt8(item->root.size());
			outbuf.Write(item->root.c_str(), item->root.size() * sizeof(wchar_t));
			outbuf.WriteInt8(item->tail.size());
			outbuf.Write(item->tail.c_str(), item->tail.size() * sizeof(wchar_t));
			break;
		default:
			;
		}

		outbuf.WriteInt8(item->word.size());
		outbuf.Write(item->word.c_str(), item->word.size() * sizeof(wchar_t));

		outbuf.WriteInt8(item->metas.size());
		for (auto meta : item->metas)
		{
			outbuf.WriteInt16(meta);
		}

	}
}

void DicTreeNode::_Dump(int depth)
{
	WidePrintf(L"Depth %d: %d children, %d items\n", depth, m_children.size(), m_items.size());

	for (auto item : m_items)
	{
		WidePrintf(L"'%ls',", item->word.c_str());
	}
	WidePrintf(L"\n");

	for (auto child : m_children)
	{
		WidePrintf(L"%lc", child.first);
	}
	WidePrintf(L"\n");

	for (auto child : m_children)
	{
		WidePrintf(L"Visit '%lc'\n", child.first);
		child.second->_Dump(depth + 1);
	}
}

class DicMaker
{
public:
	DicMaker();
	~DicMaker();

	bool AddItem(const std::wstring& word, boost::property_tree::wptree& json);
	bool SanityCheck();
	bool WriteToFile(const std::wstring& outFilename);

private:
	DicTreeNode m_words;
	DicTreeNode m_josas;
	DicTreeNode m_predicates;	// 동사,형용사,이다(조사)
	DicTreeNode m_irrconjs;
	
	std::vector<DicSrcItem*> m_items;

	std::map<std::wstring, EPosMeta> m_mapMeta;
	std::map<std::wstring, EPosType> m_mapPType;
};


DicMaker::DicMaker()
{
	for (size_t i = 0; i < sizeof(metaPairs) / sizeof(metaPairs[0]); i++)
	{
		m_mapMeta[metaPairs[i].word] = metaPairs[i].eid;
	}

	m_mapPType[L"명사"] = POSTYPE_NOUN;
	m_mapPType[L"조사"] = POSTYPE_JOSA;
	m_mapPType[L"용언"] = POSTYPE_PREDICATE;
	m_mapPType[L"부사"] = POSTYPE_ADVERB;
	m_mapPType[L"관형사"] = POSTYPE_DETERMINER;
	m_mapPType[L"불규칙활용"] = POSTYPE_IRREGULAR_CONJUGATION;
}

DicMaker::~DicMaker()
{
	for (auto item : m_items)
	{
		delete item;
	}
}

bool DicMaker::AddItem(const std::wstring& word, boost::property_tree::wptree& json)
{
	auto pos = json.get<std::wstring>(L"pos");
	auto itrType = m_mapPType.find(pos);
	if (itrType == m_mapPType.end())
	{
		WidePrintf(L"Unknown pos type: %s\n", pos.c_str());
		return false;
	}

	DicSrcItem* pItem = new DicSrcItem();
	m_items.push_back(pItem);

	if (json.find(L"dpos") != json.not_found())
	{
		auto dpos = json.get<std::wstring>(L"dpos");
		if (dpos == L"동사")
		{
			pItem->detailType = POSTYPE_DETAIL_VERB;
		}
		else if (dpos == L"형용사")
		{
			pItem->detailType = POSTYPE_DETAIL_ADJECTIVE;
		}
		else
		{
			WidePrintf(L"Unknown dpos type: %s\n", dpos.c_str());
			return false;
		}
	}
	else
	{
		pItem->detailType = POSTYPE_UNKNOWN;
	}

	if (json.find(L"meta") != json.not_found())
	{
		auto metas = json.get_child(L"meta");
		for (auto& meta : metas)
		{
			auto metaVal = meta.second.get_value<std::wstring>();
			auto itrMeta = m_mapMeta.find(metaVal);
			if (itrMeta == m_mapMeta.end())
			{
				WidePrintf(L"Unknown meta value: '%s'\n", metaVal.c_str());
				return false;
			}

			pItem->metas.insert(itrMeta->second);
		}
	}

	pItem->word = word;
	pItem->type = itrType->second;
	if (pItem->type == POSTYPE_JOSA)
	{
		PosInfoJosa::JongCondition cond;
		auto zong = json.get<std::wstring>(L"zong");
		if (zong == L"all")
		{
			cond = PosInfoJosa::JC_ALL;
		}
		else if (zong == L"yes")
		{
			cond = PosInfoJosa::JC_REQ_JONG;
		}
		else if (zong == L"no")
		{
			cond = PosInfoJosa::JC_NO_JONG;
		}
		else
		{
			WidePrintf(L"Unknown zong value: '%s'\n", zong.c_str());
			return false;
		}

		pItem->zong = cond;
		m_josas.Insert(pItem, 0, true);
	}
	else if (pItem->type == POSTYPE_IRREGULAR_CONJUGATION)
	{
		pItem->root = json.get<std::wstring>(L"root");
		pItem->tail = json.get<std::wstring>(L"tail");

		m_irrconjs.Insert(pItem, 0, true);
	}
	else if (pItem->type == POSTYPE_PREDICATE)
	{
		m_predicates.Insert(pItem, 0, true);
	}
	else
	{
		m_words.Insert(pItem, 0, false);
	}

	return true;
}

bool DicMaker::SanityCheck()
{
	int failCount = 0;

	// check if irregular conjugations has known root predicates.
	m_irrconjs.WalkItems([this, &failCount](const DicSrcItem* pItem) {
		ASSERT(pItem->type == POSTYPE_IRREGULAR_CONJUGATION);
		DicSrcItem* pRoot = m_predicates.FindByWord(pItem->root, true);
		if (pRoot == NULL)
		{
			WidePrintf(L"Irregular conjugation '%ls' has unknown root predicate '%ls'.\n", pItem->word.c_str(), pItem->root.c_str());
			failCount++;
		}
		else
		{
			ASSERT(pRoot->type == POSTYPE_PREDICATE);
		}
	});

	return (failCount == 0);
}

bool DicMaker::WriteToFile(const std::wstring& outFilename)
{
	DicFileHeader header;
	memset(&header, 0, sizeof(header));

	header.magic = DICFILE_MAGIC;
	header.version = DICFILE_VERSION;
	header.sizeofWchar = sizeof(wchar_t);
	header.sizeofPointer = sizeof(void*);

	MemoryBuffer bufWords;
	m_words.Serialize(bufWords, header.numWords, POSTYPE_UNKNOWN);
	header.wordBlockSize = bufWords.GetSize();

	MemoryBuffer bufPreds;
	m_predicates.Serialize(bufPreds, header.numPredicates, POSTYPE_UNKNOWN);
	header.verbBlockSize = bufPreds.GetSize();

	MemoryBuffer bufJosas;
	m_josas.Serialize(bufJosas, header.numJosas, POSTYPE_JOSA);
	header.josaBlockSize = bufJosas.GetSize();

	MemoryBuffer bufIrrConjs;
	m_irrconjs.Serialize(bufIrrConjs, header.numIrrConjs, POSTYPE_IRREGULAR_CONJUGATION);
	header.ircjBlockSize = bufIrrConjs.GetSize();

	MemoryBuffer bufFinal;
	bufFinal.Write(&header, sizeof(header));
	bufFinal.Write(bufWords);
	bufFinal.Write(bufPreds);
	bufFinal.Write(bufJosas);
	bufFinal.Write(bufIrrConjs);

	FILE* fp;
#ifdef _WIN32
	_wfopen_s(&fp, outFilename.c_str(), L"wb");
#else
	fp = fopen(WstringToCstring(outFilename.c_str()).c_str(), "wb");
#endif

	if (!fp)
	{
		WidePrintf(L"Couldn't open file: %s\n", outFilename.c_str());
		return false;
	}

	size_t written = fwrite(bufFinal.GetBuffer(), 1, bufFinal.GetSize(), fp);
	fclose(fp);

	WidePrintf(L"%ls: wrote %d bytes (total %d words, %d predicates, %d josas)\n", outFilename.c_str(),
		written, header.numWords, header.numPredicates, header.numJosas);

	return (written == bufFinal.GetSize());
}

bool MakeDicFile(const std::wstring& srcTxtFile, const std::wstring& dstBinFile)
{
	boost::filesystem::wifstream wif(srcTxtFile);
	if (!wif.good())
	{
		WidePrintf(L"Dictionary source file open failed: %ls\n", srcTxtFile.c_str());
		return false;
	}

	std::locale old_locale;
	std::locale utf8_locale(old_locale, new std::codecvt_utf8<wchar_t>);
	wif.imbue(utf8_locale);

	DicMaker dmk;
	bool bParseError = false;
	std::wstring line;
	int nLineNum = 0;
	while (std::getline(wif, line))
	{
		++nLineNum;
		if (line.empty())
			continue;

		auto colon = line.find(':');
		if (colon == std::wstring::npos)
		{
			bParseError = true;
			break;
		}

		std::wstring word = line.substr(0, colon);
		std::wstring jsonStr = line.substr(colon + 1);

		boost::property_tree::wptree pt;
		try
		{
			std::wstringstream ss;
			ss << jsonStr;
			boost::property_tree::read_json(ss, pt);
		}
		catch (boost::property_tree::json_parser_error&)
		{
			WidePrintf(L"JSON parsing failed: %ls\n", jsonStr.c_str());
			break;
		}		

		if (!dmk.AddItem(word, pt))
		{
			bParseError = true;
			break;
		}
	}

	if (bParseError)
	{
		WidePrintf(L"Parsing failed in line %d: %ls\n", nLineNum, line.c_str());
		return false;
	}

	if (!dmk.SanityCheck())
	{
		return false;
	}

	if (!dmk.WriteToFile(dstBinFile))
	{
		return false;
	}

	return true;
}

END_NAMESPACE_NTPOSTAG

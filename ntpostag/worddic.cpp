#include "stdafx.h"
#include "postag.h"
#include "util.h"

#include <cstring>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <codecvt>

#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/positioning.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

BEGIN_NAMESPACE_NTPOSTAG

Dictionary g_posdic;

PosInfo::~PosInfo()
{
	ASSERT(m_isDicRegistered == false);
}

PosInfo::PosInfo(EPosType type)
{
	m_type = type;
	m_isDicRegistered = false;
}

PosInfo* PosInfo::CreateFromPuncMark(CharString* pStr)
{
	PosInfo* pRet = new PosInfo(POSTYPE_PUNCMARK);
	pRet->m_word = pStr->GetString();
	return pRet;
}

PosInfo* PosInfo::CreateNounPredicate(CharString* pPredRoot)
{
#ifdef _DEBUG
	wchar_t lastCh = pPredRoot->GetCharCode(-1);
	ASSERT(lastCh == L'하' || lastCh == L'되' || lastCh == L'이');
#endif

	PosInfo* pRet = new PosInfo(POSTYPE_PREDICATE);
	pRet->m_word.reserve(pPredRoot->GetLength() + 1);
	pRet->m_word.append(pPredRoot->GetString());
	pRet->m_word.push_back(L'다');
	return pRet;
}

PosInfo* PosInfo::Clone() const
{
	PosInfo* pRet = new PosInfo(m_type);
	pRet->m_word = m_word;
	pRet->m_metas = m_metas;
	return pRet;
}

void PosInfo::DeleteIfNotRegistered()
{
	if (!m_isDicRegistered)
	{
		delete this;
	}
}

void PosInfo::SetType(EPosType type)
{
	if (type != m_type)
	{
		ASSERT(m_type == POSTYPE_UNKNOWN && type != POSTYPE_UNKNOWN);
		m_type = type;
	}
}

void PosInfo::AddMetas(std::set<EPosMeta> metas)
{
	for (auto m : metas)
	{
		AddMeta(m);
	}
}

void PosInfo::AddMeta(EPosMeta meta)
{
	// TODO: logic check
	for (auto itr = m_metas.begin(); itr < m_metas.end(); ++itr)
	{
		auto old = *itr;
		if (meta < old)
		{
			m_metas.insert(itr, meta);
			return;
		}
		else if (meta == old)
		{
			// duplicated
			return;
		}
	}

	m_metas.push_back(meta);
}

bool PosInfo::HasMeta(EPosMeta meta)
{
	return std::binary_search(m_metas.begin(), m_metas.end(), meta);
}

////////////////////////////////////////////////////////////////////////////////

/*
const std::wstring& PosInfoIrrConj::GetRootString() const
{
	return m_pRoot->GetString();
}

const std::wstring& PosInfoIrrConj::GetTailString() const
{
	return m_pConj->GetString();
}
*/
////////////////////////////////////////////////////////////////////////////////

class FileReader
{
public:
	FileReader(FILE* fp)
	{
		m_fp = fp;
		m_errorOccurred = false;
	}

	~FileReader()
	{
		fclose(m_fp);
		m_fp = NULL;
	}

	void Read(void* pBuffer, size_t len)
	{
		auto cnt = fread(pBuffer, 1, len, m_fp);
		if (cnt != len)
			m_errorOccurred = true;
	}

	int ReadInt8()
	{
		unsigned char val;
		Read(&val, sizeof(val));
		return val;
	}

	int ReadInt16()
	{
		unsigned short val;
		Read(&val, sizeof(val));
		return val;
	}

	int ReadInt32()
	{
		int val;
		Read(&val, sizeof(val));
		return val;
	}

	bool HasError()
	{
		return m_errorOccurred;
	}

private:
	FILE* m_fp;
	bool m_errorOccurred;
};

void PosTreeNode::Clear()
{
	for (auto child : m_children)
	{
		delete child;
	}
	m_children.clear();
	m_syllables.clear();

	m_items.clear();
}

PosTreeNode* PosTreeNode::FindChild(wchar_t target)
{
	if (m_syllables.empty())
		return NULL;

	int low = 0;
	int high = (int)m_syllables.size() -1;
	
	while (low <= high)
	{
		int mid = (high + low) / 2;

		int diff = m_syllables[mid] - target;
		if (diff < 0)
		{
			low = mid + 1;
		}
		else if (diff > 0)
		{
			high = mid - 1;
		}
		else
		{
			return m_children[mid];
		}
	}

#ifdef _DEBUG
	for (wchar_t ch : m_syllables)
	{
		ASSERT(ch != target);
	}
#endif

	return NULL;
}

template<typename T> static void _readLengthByteThenArray(std::vector<T>& vec, FileReader* pFile)
{
	int len = pFile->ReadInt8();
	if (len > 0)
	{
		vec.resize(len);
		pFile->Read(&vec[0], len * sizeof(T));
	}
}

static void _readLengthByteThenString(std::wstring& str, FileReader* pFile)
{
	int len = pFile->ReadInt8();
	if (len > 0)
	{
		str.resize(len);
		pFile->Read(&str[0], len * sizeof(wchar_t));
	}
}

bool PosTreeNode::Deserialize(FileReader* pFile, int& serialId)
{
	if (pFile->HasError())
		return false;

	int fileSerialId = pFile->ReadInt32();
	if (fileSerialId != ++serialId)
	{
		printf("PosTreeNode serial not match: %d (expected %d)\n", fileSerialId, serialId);
		return false;
	}

	int numChildren = pFile->ReadInt16();

	if (numChildren > 0)
	{
		m_syllables.resize(numChildren);
		pFile->Read(&m_syllables[0], sizeof(wchar_t) * numChildren);

		m_children.resize(numChildren);
		for (int i = 0; i < numChildren; i++)
		{
			PosTreeNode* pChild = new PosTreeNode();
			m_children[i] = pChild;
			pChild->Deserialize(pFile, serialId);
		}
	}

	int numItems = pFile->ReadInt32();
	if (numItems > 0)
	{
		m_items.reserve(numItems);
		BYTE seqNum = 0;

		for (int i = 0; i < numItems; i++)
		{
			BYTE fileSeqNum = pFile->ReadInt8();
			if (fileSeqNum != ++seqNum)
			{
				printf("PosTreeNode itemSeq not match: %d (expected %d)\n", fileSeqNum, seqNum);
				return false;
			}

			PosInfo* pItem;
			EPosType posType = (EPosType)pFile->ReadInt8();
			if (posType == POSTYPE_JOSA)
			{
				auto pJosaItem = new PosInfoJosa();
				pJosaItem->m_zongCondition = (PosInfoJosa::JongCondition)pFile->ReadInt8();
				pItem = pJosaItem;
			}
			else if (posType == POSTYPE_IRREGULAR_CONJUGATION)
			{
				std::wstring root;
				std::wstring tail;
				_readLengthByteThenString(root, pFile);
				_readLengthByteThenString(tail, pFile);

				auto pIrcjItem = new PosInfoIrrConj();
				pIrcjItem->m_pRoot = g_posdic._FindPredicate(root);
				pIrcjItem->m_pConj = g_posdic._FindTailConjugation(tail);
				pItem = pIrcjItem;
			}
			else
			{
				pItem = new PosInfo();
			}

			pItem->SetType(posType);
			m_items.push_back(pItem);

			_readLengthByteThenString(pItem->m_word, pFile);
			_readLengthByteThenArray<EPosMeta>(pItem->m_metas, pFile);

			g_posdic.Register(pItem);
		}
	}

	return !pFile->HasError();
}

void PosTreeNode::ForEachItem(std::function<void(PosInfo*)> fnCallback)
{
	for (auto item : m_items)
	{
		fnCallback(item);
	}
}

const PosInfo* PosTreeNode::_GetFirstItem()
{
	ASSERT(m_items.size() == 1);
	return m_items.at(0);
}

bool PosTreeNode::CheckIntegrity(const std::wstring& cumulatedStr, bool reverseDir)
{
	size_t numChildren = m_children.size();
	for (size_t i=0; i<numChildren; i++)
	{
		if (i > 0 && m_syllables[i - 1] >= m_syllables[i])
		{
			WidePrintf(L"Children syllables are NOT SORTED.\n");
			return false;
		}

		std::wstring cumul;
		if (reverseDir)
		{
			cumul = m_syllables[i] + cumulatedStr;
		}
		else
		{
			cumul = cumulatedStr + m_syllables[i];
		}

		if (!m_children[i]->CheckIntegrity(cumul, reverseDir))
		{
			return false;
		}
	}

	for (auto item : m_items)
	{
		if (memcmp(item->GetString().c_str(), cumulatedStr.c_str(), sizeof(wchar_t) * cumulatedStr.length()) != 0)
		{
			WidePrintf(L"Cumulated PosNode string diff: expected=%ls\n", cumulatedStr.c_str());
			return false;
		}
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////

bool Dictionary::IsLoaded() const
{
	return m_rootConjTails.GetChildrenSize() > 0;
}

void Dictionary::Register(PosInfo* pInfo)
{
	ASSERT(pInfo->m_isDicRegistered == false);
	pInfo->m_isDicRegistered = true;
	m_vecPosInfoReg.push_back(pInfo);
}

void Dictionary::Unload()
{
	m_pActualPredicatesRoot = NULL;

	m_rootWords.Clear();
	m_rootPredicates.Clear();
	m_rootJosas.Clear();
	m_rootIrrConjs.Clear();

	m_rootConjTails.Clear();
	m_rootConjPretails.Clear();

	m_mapMetaNames.clear();
	m_mapPTypeNames.clear();

	for (auto pInfo : m_vecPosInfoReg)
	{
#ifdef _DEBUG
		pInfo->m_isDicRegistered = false;
#endif
		delete pInfo;
	}
	m_vecPosInfoReg.clear();

	Character::DestroyLookupTable();
}

bool Dictionary::LoadData(const std::wstring& filename)
{
	FILE* fp;

#ifdef _WIN32
	_wfopen_s(&fp, filename.c_str(), L"rb");
#else
	fp = fopen(WstringToCstring(filename.c_str()).c_str(), "rb");
#endif	

	if (!fp)
	{
		std::wcout << "Dictionary file open failed: " << filename.c_str() << std::endl;
		return false;
	}

	FileReader fr(fp);

	DicFileHeader header;
	fr.Read(&header, sizeof(header));
	if (header.magic != DICFILE_MAGIC
	|| header.version != DICFILE_VERSION
	|| header.sizeofWchar != sizeof(wchar_t)
	|| header.sizeofPointer != sizeof(void*))
	{
		std::wcout << "Invalid dictionary file: " << filename.c_str() << std::endl;
		return false;
	}

	m_vecPosInfoReg.reserve(header.numWords + header.numJosas 
		+ header.numPredicates + header.numIrrConjs);
	
	int sidWords = 0;
	bool b1 = m_rootWords.Deserialize(&fr, sidWords);

	int sidPreds = 0;
	bool b2 = m_rootPredicates.Deserialize(&fr, sidPreds);

	int sidJosas = 0;
	bool b3 = m_rootJosas.Deserialize(&fr, sidJosas);

	ASSERT(m_rootPredicates.GetChildCount() == 1);
	m_pActualPredicatesRoot = m_rootPredicates.FindChild(L'다');

	InitData_Conjugations();

	int sidIrcjs = 0;
	bool b4 = m_rootIrrConjs.Deserialize(&fr, sidIrcjs);

	if (!b1 || !b2 || !b3 || !b4
	|| header.numWords != sidWords
	|| header.numPredicates != sidPreds
	|| header.numJosas != sidJosas
	|| header.numIrrConjs != sidIrcjs)
	{
		std::wcout << "Deserialization failed." << std::endl;
	}

	if (!m_rootWords.CheckIntegrity(L"", false)
	|| !m_rootIrrConjs.CheckIntegrity(L"", false)
	|| !m_rootPredicates.CheckIntegrity(L"", true)
	|| !m_rootJosas.CheckIntegrity(L"", true))
	{
		std::wcout << "CheckIntegrity failed." << std::endl;
	}

	//WidePrintf(L"Verbs:%d, iRrConjs:%d\n", header.numPredicates, header.numIrrConjs);
	InitTypeNames();

	Character::CreateLookupTable();
	return true;
}

bool PosInfoJosa::IsFulfillZongCondition(const Character* pChar)
{
	switch (m_zongCondition)
	{
	case JC_ALL:
		return true;

	case JC_NO_JONG:
		// TODO: 서울로 case
		return !pChar->HasZong();

	case JC_REQ_JONG:
		return pChar->HasZong();

	default:
		ASSERT(!"Invalid zong condition value");
		return false;
	}
}

std::vector<PosInfoJosa*> Dictionary::MatchJosa(const CharString& str)
{
	std::vector<PosInfoJosa*> matches;

	PosTreeNode* pNode = &m_rootJosas;
	int len = str.GetLength();
	while (--len > 0)
	{
		pNode = pNode->FindChild(str.GetCharCode(len));
		if (pNode == NULL)
			break;

		const Character* pPrevChar = str.GetCharacter(len - 1);

		pNode->ForEachItem([pPrevChar, &matches](PosInfo* pPosInfo) {
			PosInfoJosa* pJosa = static_cast<PosInfoJosa*>(pPosInfo);
			if (pJosa->IsFulfillZongCondition(pPrevChar))
				matches.push_back(pJosa);
		});
	}

	return matches;
}

std::vector<PosInfo*> Dictionary::MatchExactWord(const CharString& str)
{
	std::vector<PosInfo*> matches;

	PosTreeNode* pNode = &m_rootWords;
	for (int i = 0; i < str.GetLength(); i++)
	{
		pNode = pNode->FindChild(str.GetCharCode(i));
		if (pNode == NULL)
			break;
	}

	if (pNode != NULL)
	{
		// FIXME
		pNode->ForEachItem([&matches](PosInfo* spInfo) {
			matches.push_back(spInfo);
		});
	}

	return matches;
}

// TODO: return most matched result
PosInfo* Dictionary::MatchNoun(const CharString& str)
{
	PosInfo* noun = NULL;

	PosTreeNode* pNode = &m_rootWords;
	for (int i = 0; i < str.GetLength(); i++)
	{
		pNode = pNode->FindChild(str.GetCharCode(i));
		if (pNode == NULL)
			break;
	}

	if (pNode != NULL)
	{
		// FIXME
		pNode->ForEachItem([&noun](PosInfo* pInfo) {
			if (pInfo->GetType() == POSTYPE_NOUN)
			{
				ASSERT(noun == NULL);
				noun = pInfo;
			}
		});
	}

	return noun;
}

const PosInfo* Dictionary::_FindPredicate(const std::wstring& word)
{
	ASSERT(word.back() == L'다' && word.length() >= 2);

	PosTreeNode* pNode = m_pActualPredicatesRoot;
	int len = (int)word.length() -1;
	while (len > 0)
	{
		pNode = pNode->FindChild(word.at(--len));
		if (pNode == NULL)
			break;
	}

	if (pNode == NULL)
	{
		WidePrintf(L"_FindPredicate(%ls) failed\n", word.c_str());
		ASSERT(!"_FindPredicate failed");
		return NULL;
	}

	return pNode->_GetFirstItem();
}

const Conjugation* Dictionary::_FindTailConjugation(const std::wstring& word)
{
	ConjTreeNode* pNode = &m_rootConjTails;
	int len = (int)word.length();
	ASSERT(len > 0);
	while (len > 0)
	{
		auto itr = pNode->m_children.find(word.at(--len));
		if (itr == pNode->m_children.end())
		{
			ASSERT(0);
			return NULL;
		}

		pNode = itr->second;
	}

	ASSERT(pNode->m_items.size() == 1);
	return pNode->m_items.at(0);
}

END_NAMESPACE_NTPOSTAG

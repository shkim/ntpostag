#pragma once

BEGIN_NAMESPACE_NTPOSTAG

enum EPosType : unsigned char
{
	POSTYPE_UNKNOWN =0,

	POSTYPE_NOUN,			// 명사
	POSTYPE_ADVERB,			// 부사
	POSTYPE_DETERMINER,		// 관형사,한정사
	POSTYPE_JOSA,			// 조사
	POSTYPE_PREDICATE,      // 용언
	POSTYPE_IRREGULAR_CONJUGATION,  // 불규칙 용언 활용

	POSTYPE_PUNCMARK,		// 문장부호

	// for detailed type
	POSTYPE_DETAIL_VERB,   // 동사
	POSTYPE_DETAIL_ADJECTIVE,   // 형용사
};

enum EPosMeta : unsigned short
{
	POSMETA_NONE =0,

	POSMETA_SUBJECTIVE,
	POSMETA_OBJECTIVE,
	POSMETA_TASTE,
	POSMETA_ACTION,
	POSMETA_TIME,
	POSMETA_ABSTRACT,

	// 개체명:
	POSMETA_ENTITY_FULLNAME,
	POSMETA_ENTITY_FIRSTNAME,
	POSMETA_ENTITY_LASTNAME,
	POSMETA_ENTITY_APPELLATION, // 호칭
	POSMETA_ENTITY_PERSON,  // 사람을 뜻하는 명사

	POSMETA_ENTITY_LOCATION_NAME,        // 지역명
	POSMETA_ENTITY_LOCATION_UNIT,   // ~시, ~도
	POSMETA_ENTITY_LOCATION,

	POSMETA_ENTITY_ORGANIZATION_NAME,    // 구체적인 조직명
	POSMETA_ENTITY_ORGANIZATION_UNIT,    // ~은행, ~회사

	POSMETA_ENTITY_NATION,

	POSMETA_PERSON_POLITICIAN,
	POSMETA_PERSON_SPORTSMAN,
	POSMETA_PERSON_ENTERTAINER,


	// 종결어미:
	POSMETA_PRED_ENDING,	// 종결어미 (평서형)
	POSMETA_PRED_ENDING_QUESTION,	// 의문형 종결어미
	POSMETA_PRED_ENDING_EXCLAMATION,	// 감탄형 종결어미
	POSMETA_PRED_ENDING_IMPERATIVE,	// 명령형 종결어미
	POSMETA_PRED_ENDING_REQUEST, // 청유형 종결어미

	// 비종결어미:
	POSMETA_PRED_CONJUNCT,	// 비종결, 연결어미
	POSMETA_PRED_TRANS_NOUN,	// 비종결, 전성어미, 명사형
	POSMETA_PRED_TRANS_DETERMINER,	// 비종결, 전성어미, 관형사형
	POSMETA_PRED_TRANS_ADVERB,	// 비종결, 전성어미, 부사형

	// 선어말어미 및 기타:
	POSMETA_PRED_HONORIFIC,		// ~시~
	POSMETA_PRED_PRESENT,		// ~는~
	POSMETA_PRED_PAST,			// ~었~
	POSMETA_PRED_FUTURE,		// ~겠~
	POSMETA_PRED_POLITENESS,	// ~옵~
	POSMETA_PRED_GUESS,		// 추측
	POSMETA_PRED_INFORMAL,	// 하대


	POSMETA_PUNCTUATION,

};

class FileReader;
class PosInfo;
class CharString;

class PosTreeNode
{
public:
	~PosTreeNode() { Clear(); }

	void Clear();
	PosTreeNode* FindChild(wchar_t ch);

	bool Deserialize(FileReader* pFile, int& serialId);
	void ForEachItem(std::function<void(PosInfo*)> fnCallback);
	const PosInfo* _GetFirstItem();

	bool CheckIntegrity(const std::wstring& cumulatedStr, bool reverseDir);

	inline size_t GetChildCount() const
	{
		return m_children.size();
	}

private:
	std::vector<wchar_t> m_syllables;
	std::vector<PosTreeNode*> m_children;
	std::vector<PosInfo*> m_items;	// leaf
};

class PosInfo
{
public:
	static PosInfo* CreateFromPuncMark(CharString* pStr);
	static PosInfo* CreateNounPredicate(CharString* pPredRoot);
	PosInfo* Clone() const;

	void DeleteIfNotRegistered();
	void SetType(EPosType type);
	bool HasMeta(EPosMeta meta);
	void AddMeta(EPosMeta meta);
	void AddMetas(std::set<EPosMeta> metas);

	inline EPosType GetType() const
	{
		return m_type;
	}

	inline int GetLength() const
	{
		return int(m_word.size());
	}

	inline const std::vector<EPosMeta> GetMetas() const
	{
		return m_metas;
	}

	inline const std::wstring& GetString() const
	{
		return m_word;
	}

protected:
	virtual ~PosInfo();
	PosInfo(EPosType type = POSTYPE_UNKNOWN);
	//PosInfo(const PosInfo& src);

	EPosType m_type;
	bool m_isDicRegistered;
	std::wstring m_word;
	std::vector<EPosMeta> m_metas;

	friend bool PosTreeNode::Deserialize(FileReader* pFile, int& serialId);
	friend class Dictionary;
};

class PosInfoWrapper
{
public:
	PosInfoWrapper(PosInfo* pInfo)
	{
		m_pInfo = pInfo;
	}

	~PosInfoWrapper()
	{
		m_pInfo->DeleteIfNotRegistered();
	}

	inline PosInfo* operator->() const
	{
		return m_pInfo;
	}

private:
	PosInfo* m_pInfo;
};

class Character;

class PosInfoJosa : public PosInfo
{
public:
	bool IsFulfillZongCondition(const Character* pChar);

	enum JongCondition
	{
		JC_ALL,
		JC_REQ_JONG,
		JC_NO_JONG
	};

protected:
	virtual ~PosInfoJosa() {}
	JongCondition m_zongCondition;

	friend bool PosTreeNode::Deserialize(FileReader* pFile, int& serialId);
};

class Conjugation;

class PosInfoIrrConj : public PosInfo
{
public:
	inline const PosInfo* GetRoot() const
	{
		return m_pRoot;
	}

	inline const Conjugation* GetTailConj() const
	{
		return m_pConj;
	}

	//const std::wstring& GetRootString() const;
	//const std::wstring& GetTailString() const;

private:
	virtual ~PosInfoIrrConj() {}

	const PosInfo* m_pRoot;
	const Conjugation* m_pConj;

	friend bool PosTreeNode::Deserialize(FileReader* pFile, int& serialId);
};

#define DICFILE_MAGIC		*((int*)"DICF")
#define DICFILE_VERSION		1

struct DicFileHeader
{
	int magic;

	short version;
	BYTE sizeofWchar;	// 2 or 4
	BYTE sizeofPointer;	// 4 or 8

	int numWords;
	int numPredicates;
	int numJosas;
	int numIrrConjs;

	size_t wordBlockSize;
	size_t verbBlockSize;
	size_t josaBlockSize;
	size_t ircjBlockSize;
};

END_NAMESPACE_NTPOSTAG

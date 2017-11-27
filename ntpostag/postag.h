#pragma once

#include "posdic.h"

BEGIN_NAMESPACE_NTPOSTAG

class Character
{
public:
	static void CreateLookupTable();
	static void DestroyLookupTable();

	enum Category : unsigned char
	{
		CT_IGNORE = 0,
		CT_SPACE,	// space, tab, unicode spaces
		CT_NEWLINE,	// cr,lf

		CT_PUNCMARK_END,	// .?!
		CT_PUNCMARK_COMMA,	// ,
		CT_PUNCMARK_QUOTE,

		CT_SYMBOL,
		CT_NUMBER,

		CT_ALPHABET,
		CT_HANJA,

		CT_HANGUL_PARTIAL,
		CT_HANGUL_COMPOSED
	};

	void Set(wchar_t ch);
	void SetHangul(wchar_t cho, wchar_t joong, wchar_t zong);

	void ForceSetCode(wchar_t ch);	// set code without changing the category
	void ForceSetCategory(Category cate);	// update category without modifying the m_code

	inline wchar_t GetCode() const
	{
		return m_code;
	}

	inline Category GetCategory() const
	{
		return m_category;
	}

	// valid if hangul ==>
	wchar_t GetCho() const;
	wchar_t GetJoong() const;
	wchar_t GetZong() const;
	bool HasZong() const;

private:
	wchar_t m_code;

	Category m_category;
	signed char m_cho;
	signed char m_joong;
	signed char m_zong;
};

class Conjugation;

class CharString
{
public:
	CharString(const Character* pFrom, const Character* pTill);
	CharString(const wchar_t* pChars, int len = -1);
	CharString(const CharString& src);

	static CharString EmptyString();

	const Character* GetCharacter(int pos) const;
	
	inline wchar_t GetCharCode(int pos) const
	{
		return GetCharacter(pos)->GetCode();
	}
	
	inline int GetLength() const
	{
		return m_numChars;
	}

	CharString GetSubstring(int start, int end = -1) const;
	CharString GetConjugationRemoved(const Conjugation* pConj) const;
	std::wstring GetString() const;	// only for DEBUG purpose

private:
	CharString(int zero);

	const Character* m_pFrom;
	const Character* m_pTill;
	int m_numChars;

	typedef std::vector<Character> CharVector;
	typedef boost::shared_ptr<CharVector> CharVectorSP;

	CharVectorSP m_spOwnChars;
};

class TokenPosNominee
{
public:
	TokenPosNominee(int capacity =0);
	~TokenPosNominee();

	void AddPos(PosInfo* pInfo);

	//void PrintDebugInfo();

	int GetScore();

	TokenPosNominee* m_pIsDependencyOf;

	struct
	{

	};

	std::vector<PosInfoWrapper> m_poses;
};

class Token
{
public:
	Token(const Character* pFrom, const Character* pTill) : m_str(pFrom, pTill) {}

	void GatherPossiblePoses();

	const TokenPosNominee& GetMostPossiblePos() const;

	inline std::wstring GetString() const
	{
		return m_str.GetString();
	}

	//boost::optional<TokenPosNominee> GetPos();

private:
	CharString m_str;
	std::vector<TokenPosNominee> m_nominees;
};

class Sentence
{
public:
	enum PartType
	{
		PART_MAIN = 0,
		PART_QUOTATION,
		PART_MODIFIER,
		PART_EMPHASIS
	};

	struct Part
	{
		Character* pFrom;
		Character* pTill;	// part includes pTill character! always part[0].pTill < part[1].pFrom
		
		Character* pPartTill;	// including children parts
		int partId;

		int depth;
		//bool isQuote;

		PartType type;
	};

	struct PartWithTokens : Part
	{
		PartWithTokens(const Part& src) : Part(src) {}

		std::vector<Token> tokens;
	};

	Sentence(std::deque<Part>& parts);

	std::wstring GetString() const;
	std::vector<std::wstring> GetQuotedStrings() const;
	
	void AnalyzePos();
	
#ifndef _NOUSE_CPPREST
	web::json::value GetResultJson();
	web::json::value GetUnknownWords();
#endif

	void DumpTokens();

private:
	std::vector<PartWithTokens> m_parts;
};

// not really a paragraph processor. just a raw input string preprocessor
// such as character normalization, sentences splitting.
class Paragraph
{
public:
	bool Set(const wchar_t* pszInputParagraph, int chlen=-1, bool bNormalizeCharacters=false);

	inline std::vector<Sentence> GetSentences()
	{
		return m_sentences;
	}

private:
	bool SplitSentences();
	void SplitByDotThenAddSentence(std::deque<Sentence::Part>& parts);

	std::vector<Character> m_charbuff;
	std::vector<Sentence> m_sentences;
};

class Conjugation
{
public:
	enum MatchType
	{
		MATCH_FULL,	// default
		MATCH_EXCEPT_CHO,	// c
		MATCH_JOONG_AND_NOZONG,	// j
		MATCH_ZONG_ONLY	// z
	};

	inline const std::wstring& GetString() const
	{
		return m_word;
	}

	inline int GetLength() const
	{
		return (int)m_word.length();
	}

	wchar_t GetRemainChar(const CharString* predStr) const;

	//std::function<bool(const Syllable* pHead, const Syllable* pTail, const Token* pCurrentToken)> checkAfterMatch;
	std::function<bool()> checkAfterMatch;

	wchar_t m_remainChar;	// or 0
	MatchType m_finalMatchType;
	
	std::wstring m_word;
	std::set<EPosMeta> m_metas;
};

class ConjTreeNode
{
public:
	~ConjTreeNode() { Clear(); }

	void Clear();

	// only for root node
	Conjugation* Add(const wchar_t* pszConjSpec, const std::initializer_list<EPosMeta>& metas);
	
	// TODO: rename
	inline void FindMatches(const CharString& str, std::vector<Conjugation*>& matchedConjs)
	{
		FindMatch(str, 0, matchedConjs);
	}

	inline size_t GetChildrenSize() const
	{
		return m_children.size();
	}

private:
	void Insert(Conjugation* pItem, const std::vector<wchar_t>& keys, int depth);
	void FindMatch(const CharString& str, int pos, std::vector<Conjugation*>& matchedConjs);

	static wchar_t MakeMaskKey_ExceptCho(const Character* pChar);
	static wchar_t MakeMaskKey_JoongNoZong(const Character* pChar);
	static wchar_t MakeMaskKey_ZongOnly(const Character* pChar);

	std::map<wchar_t, ConjTreeNode*> m_children;
	std::vector<Conjugation*> m_items;	// leaf nodes

	friend class Dictionary;
};

class Dictionary
{
public:
	void Unload();
	bool LoadData(const std::wstring& filename);
	bool IsLoaded() const;
	void Register(PosInfo* pInfo);

	const std::wstring& GetMetaName(EPosMeta meta) const;
	const std::wstring& GetPosTypeName(EPosType type) const;

	std::vector<PosInfoJosa*> MatchJosa(const CharString& str);
	std::vector<PosInfo*> MatchExactWord(const CharString& str);
	PosInfo* MatchNoun(const CharString& str);
	std::vector<PosInfo*> MatchPredicate(const CharString& str);

	// only used for PosInfoIrrConj
	const PosInfo* _FindPredicate(const std::wstring& word);
	const Conjugation* _FindTailConjugation(const std::wstring& word);

private:
	PosTreeNode m_rootWords;
	PosTreeNode m_rootPredicates;	// rather use m_pActualPredicatesRoot
	PosTreeNode m_rootJosas;
	PosTreeNode m_rootIrrConjs;
	PosTreeNode* m_pActualPredicatesRoot;
	std::vector<PosInfo*> m_vecPosInfoReg;

	ConjTreeNode m_rootConjTails;
	ConjTreeNode m_rootConjPretails;
	
	std::map<EPosMeta, std::wstring> m_mapMetaNames;
	std::map<EPosType, std::wstring> m_mapPTypeNames;
	void InitTypeNames();

	void InitData_Conjugations();
	Conjugation* AddTailConj(const wchar_t* pszSpec, const std::initializer_list<EPosMeta>& metas);
	Conjugation* AddPretailConj(const wchar_t* pszSpec, const std::initializer_list<EPosMeta>& metas);
};

extern Dictionary g_posdic;

bool MakeDicFile(const std::wstring& srcTxtFile, const std::wstring& dstBinFile);

#ifndef _NOUSE_CPPREST
bool StartApiListen(int nWebPort);
#endif

END_NAMESPACE_NTPOSTAG


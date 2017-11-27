#include "stdafx.h"
#include "postag.h"

BEGIN_NAMESPACE_NTPOSTAG

// ㄱ      ㄲ      ㄴ      ㄷ      ㄸ      ㄹ      ㅁ      ㅂ      ㅃ      ㅅ      ㅆ      ㅇ      ㅈ      ㅉ      ㅊ      ㅋ      ㅌ      ㅍ      ㅎ
static wchar_t ChoSung[] = { 0x3131, 0x3132, 0x3134, 0x3137, 0x3138, 0x3139, 0x3141, 0x3142, 0x3143, 0x3145, 0x3146, 0x3147, 0x3148, 0x3149, 0x314a, 0x314b, 0x314c, 0x314d, 0x314e };
// ㅏ      ㅐ      ㅑ      ㅒ      ㅓ      ㅔ      ㅕ      ㅖ      ㅗ      ㅘ      ㅙ      ㅚ      ㅛ      ㅜ      ㅝ      ㅞ      ㅟ      ㅠ      ㅡ      ㅢ      ㅣ
static wchar_t JoongSung[] = { 0x314f, 0x3150, 0x3151, 0x3152, 0x3153, 0x3154, 0x3155, 0x3156, 0x3157, 0x3158, 0x3159, 0x315a, 0x315b, 0x315c, 0x315d, 0x315e, 0x315f, 0x3160, 0x3161, 0x3162, 0x3163 };
// ㄱ      ㄲ      ㄳ      ㄴ      ㄵ      ㄶ      ㄷ      ㄹ      ㄺ      ㄻ      ㄼ      ㄽ      ㄾ      ㄿ      ㅀ      ㅁ      ㅂ      ㅄ      ㅅ      ㅆ      ㅇ      ㅈ      ㅊ      ㅋ      ㅌ      ㅍ      ㅎ
static wchar_t ZongSung[] = { 0, 0x3131, 0x3132, 0x3133, 0x3134, 0x3135, 0x3136, 0x3137, 0x3139, 0x313a, 0x313b, 0x313c, 0x313d, 0x313e, 0x313f, 0x3140, 0x3141, 0x3142, 0x3144, 0x3145, 0x3146, 0x3147, 0x3148, 0x314a, 0x314b, 0x314c, 0x314d, 0x314e };

static bool IsCompositeHangul(wchar_t ch)
{
	// Hangul unicode range: "AC00:가" ~ "D7A3:힣"
	return (0xAC00 <= ch && ch <= 0xD7A3);
}

static bool IsPartialHangulPhoneme(wchar_t ch)
{
	// partial hangul component in ChoSung ~ ZongSung
	return (0x3131 <= ch && ch <= 0x3163);
}

static bool IsUnicodeGeneralPunctuation(wchar_t ch)
{
	return (0x2000 <= ch && ch <= 0x206F);
}

void CreateQuotePairTable();
void DestroyQuotePairTable();

static std::map<wchar_t, Character::Category> s_char2cate;

void Character::CreateLookupTable()
{
	s_char2cate[0] = CT_IGNORE;

	s_char2cate[' '] = CT_SPACE;
	s_char2cate['\t'] = CT_SPACE;
	s_char2cate[0x2000] = CT_SPACE;
	s_char2cate[0x2001] = CT_SPACE;
	s_char2cate[0x2002] = CT_SPACE;
	s_char2cate[0x2003] = CT_SPACE;
	s_char2cate[0x2004] = CT_SPACE;
	s_char2cate[0x2005] = CT_SPACE;
	s_char2cate[0x2006] = CT_SPACE;
	s_char2cate[0x2007] = CT_SPACE;
	s_char2cate[0x2008] = CT_SPACE;
	s_char2cate[0x2009] = CT_SPACE;
	s_char2cate[0x200A] = CT_SPACE;
	s_char2cate[0x200B] = CT_IGNORE;
	s_char2cate[0x200C] = CT_IGNORE;
	s_char2cate[0x200D] = CT_IGNORE;
	s_char2cate[0x200E] = CT_IGNORE;
	s_char2cate[0x200F] = CT_IGNORE;

	s_char2cate['\r'] = CT_NEWLINE;
	s_char2cate['\n'] = CT_NEWLINE;
	s_char2cate[0x2028] = CT_NEWLINE;
	s_char2cate[0x2029] = CT_NEWLINE;

	s_char2cate[0x202A] = CT_IGNORE;
	s_char2cate[0x202B] = CT_IGNORE;
	s_char2cate[0x202C] = CT_IGNORE;
	s_char2cate[0x202D] = CT_IGNORE;
	s_char2cate[0x202E] = CT_IGNORE;
	s_char2cate[0x202F] = CT_IGNORE;

	s_char2cate['('] = CT_PUNCMARK_QUOTE;
	s_char2cate[')'] = CT_PUNCMARK_QUOTE;
	s_char2cate['{'] = CT_PUNCMARK_QUOTE;
	s_char2cate['}'] = CT_PUNCMARK_QUOTE;
	s_char2cate['['] = CT_PUNCMARK_QUOTE;
	s_char2cate[']'] = CT_PUNCMARK_QUOTE;
	s_char2cate['<'] = CT_PUNCMARK_QUOTE;
	s_char2cate['>'] = CT_PUNCMARK_QUOTE;
	s_char2cate['"'] = CT_PUNCMARK_QUOTE;
	s_char2cate['\''] = CT_PUNCMARK_QUOTE;
	s_char2cate['`'] = CT_PUNCMARK_QUOTE;
	s_char2cate[0x2018] = CT_PUNCMARK_QUOTE;	// ‘
	s_char2cate[0x2019] = CT_PUNCMARK_QUOTE;	// ’
	s_char2cate[0x201B] = CT_PUNCMARK_QUOTE;	// ‛
	s_char2cate[0x201C] = CT_PUNCMARK_QUOTE;	// “
	s_char2cate[0x201D] = CT_PUNCMARK_QUOTE;	// ”
	s_char2cate[0x201E] = CT_PUNCMARK_QUOTE;	// „
	s_char2cate[0x201F] = CT_PUNCMARK_QUOTE;	// ‟
	s_char2cate[0x2039] = CT_PUNCMARK_QUOTE;	// ‹
	s_char2cate[0x203A] = CT_PUNCMARK_QUOTE;	// ›

	s_char2cate[','] = CT_PUNCMARK_COMMA;
	s_char2cate[0x201A] = CT_PUNCMARK_COMMA;	// ‚
	s_char2cate[0x2027] = CT_PUNCMARK_COMMA;	// ‧

	s_char2cate['.'] = CT_PUNCMARK_END;
	s_char2cate['!'] = CT_PUNCMARK_END;
	s_char2cate['?'] = CT_PUNCMARK_END;
	s_char2cate[0x2024] = CT_PUNCMARK_END;	// ․
	s_char2cate[0x2025] = CT_PUNCMARK_END;	// ‥
	s_char2cate[0x2026] = CT_PUNCMARK_END;	// …
	s_char2cate[0x203C] = CT_PUNCMARK_END;	// ‼
	s_char2cate[0x203D] = CT_PUNCMARK_END;	// ‽
	s_char2cate[0x2047] = CT_PUNCMARK_END;	// ⁇
	s_char2cate[0x2048] = CT_PUNCMARK_END;	// ⁈
	s_char2cate[0x2049] = CT_PUNCMARK_END;	// ⁉
	s_char2cate[0xFF01] = CT_PUNCMARK_END;	// ！
	s_char2cate[0xFF1F] = CT_PUNCMARK_END;	// ？

	s_char2cate[0xFFEF] = CT_IGNORE;
	s_char2cate[0xFFFF] = CT_IGNORE;

	CreateQuotePairTable();
}

void Character::DestroyLookupTable()
{
	s_char2cate.clear();
	DestroyQuotePairTable();
}

////////////////////////////////////////////////////////////////////////////////

void Character::ForceSetCode(wchar_t ch)
{
	m_code = ch;
}

void Character::ForceSetCategory(Category cate)
{
	m_category = cate;
}

void Character::Set(wchar_t ch)
{
	m_code = ch;

	if (IsCompositeHangul(ch))
	{
		int c = ch - 0xAC00;
		int a = c / (21 * 28);
		c = c % (21 * 28);
		int b = c / 28;
		c = c % 28;

		m_cho = (char)a;
		m_joong = (char)b;
		m_zong = (char)c;
		m_category = CT_HANGUL_COMPOSED;
	}
	else
	{
		m_cho = -1;
		m_joong = -1;
		m_zong = 0;

		if (IsPartialHangulPhoneme(ch))
		{
			m_category = CT_HANGUL_PARTIAL;

			for (int i = 0; i < sizeof(JoongSung) / sizeof(JoongSung[0]); i++)
			{
				if (ch == JoongSung[i])
				{
					m_joong = i;
					return;
				}
			}

			for (int i = 0; i < sizeof(ChoSung) / sizeof(ChoSung[0]); i++)
			{
				if (ch == ChoSung[i])
				{
					m_cho = i;
					return;
				}
			}

			for (int i = 0; i < sizeof(ZongSung) / sizeof(ZongSung[0]); i++)
			{
				if (ch == ZongSung[i])
				{
					m_zong = i;
					return;
				}
			}

			ASSERT(!"Unknown hangul code?");
		}

		if (0x4E00 <= ch && ch <= 0x9FFF)
		{
			m_category = CT_HANJA;
			return;
		}

		if (('0' <= ch && ch <= '9') || (L'０' <= ch && ch <= L'９'))
		{
			m_category = CT_NUMBER;
			return;
		}

		if (('A' <= ch && ch <= 'Z') 
		|| ('a' <= ch && ch <= 'z')
		|| (L'Ａ' <= ch && ch <= L'Ｚ')	// 0xFF21 ~ 0xFF3A
		|| (L'ａ' <= ch && ch <= L'ｚ'))	// 0xFF41 ~ 0xFF5A
		{
			m_category = CT_ALPHABET;
			return;
		}

		auto itr = s_char2cate.find(ch);
		if (itr == s_char2cate.end())
		{
			m_category = CT_SYMBOL;
		}
		else
		{
			m_category = itr->second;
		}
	}
}

void Character::SetHangul(wchar_t cho, wchar_t joong, wchar_t zong)
{
	m_cho = -1;
	for (int i = 0; i < sizeof(ChoSung) / sizeof(ChoSung[0]); i++)
	{
		if (cho == ChoSung[i])
		{
			m_cho = i;
			break;
		}
	}

	m_joong = -1;
	for (int i = 0; i < sizeof(JoongSung) / sizeof(JoongSung[0]); i++)
	{
		if (joong == JoongSung[i])
		{
			m_joong = i;
			break;
		}
	}

	m_zong = 0;
	for (int i = 0; i < sizeof(ZongSung) / sizeof(ZongSung[0]); i++)
	{
		if (zong == ZongSung[i])
		{
			m_zong = i;
			break;
		}
	}

	if (m_joong >= 0)
	{
		int ch = (m_cho * 21 * 28) + (m_joong * 28) + m_zong;
		m_code = (wchar_t)(ch + 0xAC00);
		m_category = CT_HANGUL_COMPOSED;
	}
	else if (m_cho >= 0)
	{
		m_code = ChoSung[m_cho];
		m_category = CT_HANGUL_PARTIAL;
	}
	else if (m_zong >= 0)
	{
		m_code = ZongSung[m_zong];
		m_category = CT_HANGUL_PARTIAL;
	}
}

wchar_t Character::GetCho() const
{
	return m_cho >= 0 ? ChoSung[m_cho] : 0;
}

wchar_t Character::GetJoong() const
{
	return m_joong >= 0 ? JoongSung[m_joong] : 0;
}

wchar_t Character::GetZong() const
{
	return ZongSung[m_zong];
}

bool Character::HasZong() const
{
	return m_zong > 0;
}


////////////////////////////////////////////////////////////////////////////////

CharString::CharString(const Character* pFrom, const Character* pTill)
{
	m_pFrom = pFrom;
	m_pTill = pTill;
	m_numChars = (int)(pTill - pFrom) + 1;
	ASSERT(m_numChars > 0);
}

CharString::CharString(const wchar_t* pChars, int len)
{
	if (len < 0)
	{
		len = (int)wcslen(pChars);
	}

	ASSERT(len > 0);
	m_spOwnChars = boost::make_shared<CharVector>(len);
	for (int i = 0; i < len; i++)
	{
		m_spOwnChars->at(i).Set(pChars[i]);
	}

	m_pFrom = &m_spOwnChars->front();
	m_pTill = &m_spOwnChars->back();
	m_numChars = len;
}

CharString::CharString(const CharString& src)
{
	m_pFrom = src.m_pFrom;
	m_pTill = src.m_pTill;
	m_numChars = src.m_numChars;
	m_spOwnChars = src.m_spOwnChars;
}

// FIXME
CharString::CharString(int zero)
{
	ASSERT(zero == 0);
	m_pFrom = NULL;
	m_pTill = NULL;
	m_numChars = 0;
}

CharString CharString::EmptyString()
{
	return CharString(0);
}

const Character* CharString::GetCharacter(int pos) const
{
	if (pos < 0)
	{
		// -1: last, -2: last -1, ...
		pos = m_numChars + pos;
	}

	ASSERT(pos < m_numChars);
	return &m_pFrom[pos];
}

std::wstring CharString::GetString() const
{
	std::wstring str;
	str.reserve(m_numChars);
	for (const Character* p = m_pFrom; p <= m_pTill; p++)
	{
		str.push_back(p->GetCode());
	}

	return str;
}

// new substring will not include the charater at [end].
CharString CharString::GetSubstring(int start, int end) const
{
	const Character* pFrom = &m_pFrom[start];
	const Character* pTill;
	if (end < 0)
	{
		pTill = m_pTill;
	}
	else
	{
		pTill = &m_pFrom[end -1];
	}

	ASSERT(pFrom <= pTill);

	CharString ret(pFrom, pTill);
	if (m_spOwnChars)
	{
		ret.m_spOwnChars = m_spOwnChars;
		ASSERT(&ret.m_spOwnChars->front() == pFrom);
	}

	return ret;
}

CharString CharString::GetConjugationRemoved(const Conjugation* pConj) const
{
	int cutLen = pConj->GetLength();
	ASSERT(m_numChars >= cutLen);

	wchar_t appendChar = pConj->GetRemainChar(this);

	auto spOwnChars = boost::make_shared<CharVector>();
	if (cutLen < m_numChars)
	{
		int remain = m_numChars - cutLen;
		spOwnChars->reserve(remain + 1);
		for (int i = 0; i < remain; i++)
		{
			spOwnChars->push_back(m_pFrom[i]);
		}
	}
	else if (appendChar == 0)
	{
		return CharString::EmptyString();
	}

	if (appendChar != 0)
	{
		Character ch;
		ch.Set(appendChar);
		spOwnChars->push_back(ch);
	}

	CharString ret(&spOwnChars->front(), &spOwnChars->back());
	ret.m_spOwnChars = spOwnChars;

	return ret;
}

END_NAMESPACE_NTPOSTAG

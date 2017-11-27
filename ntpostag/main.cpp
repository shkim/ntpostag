//
//  main.cpp
//  ntpostag
//
//  Created by shkim on 4/29/17.
//  Copyright © 2017 newstrust. All rights reserved.
//
#include "stdafx.h"
#include "postag.h"
#include "util.h"

#include <locale.h>

BEGIN_NAMESPACE_NTPOSTAG

END_NAMESPACE_NTPOSTAG

USING_NAMESPACE_NTPOSTAG

#ifndef _UNITTEST

static int makePosDic(const char* txtfname, const char* datfname)
{
	auto datfwname = CstringToWstring(datfname);
	if (MakeDicFile(CstringToWstring(txtfname), datfwname))
	{
		if (g_posdic.LoadData(datfwname))
		{
			WidePrintf(L"Generated dictionary file <%ls> and tested successfully.\n", datfwname.c_str());
			return 0;
		}
	}

	return -1;
}

static void enterInteractivePostagShell()
{
	char buff[4096];
	Paragraph para;

	for(;;)
	{
		WidePrintf(L"Please enter sentences (paragraph) to analyze.\n");
		if (!fgets(buff, sizeof(buff), stdin))
		{
			break;
		}

		const char* pszLine = TrimString(buff);
		if (!pszLine)
		{
			break;
		}

		if (para.Set(CstringToWstring(pszLine).c_str()))
		{
			para.GetSentences().front().AnalyzePos();
		}
	}

	WidePrintf(L"Bye.\n");
}

void printHelp(const char* exename)
{
	WidePrintf(L"Usage: %ls <command> [args...]\n", CstringToWstring(exename).c_str());
	WidePrintf(L"command and arguments:\n"
		L"\tmakedic <source text filename> <output binary filename>\n"
#ifndef _NOUSE_CPPREST
		L"\tdaemon <dictionary filename> <listen port number>\n"
#endif
		L"\tinteractive <dictionary filename>\n"
	);
}

int main(int argc, const char * argv[])
{
	setlocale(LC_ALL, "");

	if (argc < 2)
	{
		printHelp(argv[0]);
		return 0;
	}

	if (!strcmp(argv[1], "makedic"))
	{
		if (argc < 4)
		{
			printHelp(argv[0]);
			return -1;
		}

		return makePosDic(argv[2], argv[3]);
	}
#ifndef _NOUSE_CPPREST
	else if (!strcmp(argv[1], "daemon"))
	{
		if (argc < 4)
		{
			printHelp(argv[0]);
			return -1;
		}

		int portnum = atoi(argv[3]);
		if (portnum <= 0 || portnum >= 0xFFFF)
		{
			WidePrintf(L"Invalid port number: %s\n", argv[3]);
			return -1;
		}

		if(!g_posdic.LoadData(CstringToWstring(argv[2])))
		{
			WidePrintf(L"Dictionary file loading failed: %ls\n", CstringToWstring(argv[2]).c_str());
			return -1;
		}

		StartApiListen(portnum);
		//sen.DumpTokens();
	}
#endif
	else if (!strcmp(argv[1], "interactive"))
	{
		enterInteractivePostagShell();
	}
	else
	{
		WidePrintf(L"Unknown command: %ls\n", CstringToWstring(argv[1]).c_str());
		return -1;
	}

	return 0;
}

#endif  // _UNITTEST

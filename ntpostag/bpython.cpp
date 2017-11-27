#include "stdafx.h"
#include "postag.h"
#include "util.h"

#ifndef _WIN32

#include <boost/python.hpp>

using namespace boost::python;
USING_NAMESPACE_NTPOSTAG

char const* greet()
{
	return "hello, world";
}

char const* load_dic(std::wstring filename)
{
	if (g_posdic.IsLoaded())
	{
		return "Already loaded";
	}
	else if (g_posdic.LoadData(filename))
	{
		return "Success";
	}
	else
	{
		PyErr_SetString(PyExc_RuntimeWarning, "Postag Dictionary loading failed");
		return "Failed";
	}
}

static bool check_dic_loaded()
{
	if (g_posdic.IsLoaded())
		return true;

	PyErr_SetString(PyExc_RuntimeWarning, "Postag Dictionary not loaded");
	return false;
}

boost::python::list split_sentences(std::wstring str)
{
	boost::python::list ret;

	if (!check_dic_loaded())
	{
		return ret;
	}

	Paragraph para;
	para.Set(str.c_str());
	auto sens = para.GetSentences();
	for (auto sen : sens)
	{
		ret.append(sen.GetString());
	}

	return ret;
}

boost::python::list get_quoted_strings(std::wstring str)
{
	boost::python::list ret;

	if (!check_dic_loaded())
	{
		return ret;
	}

	Paragraph para;
	para.Set(str.c_str());
	auto sens = para.GetSentences();
	for (auto sen : sens)
	{
		auto qss = sen.GetQuotedStrings();
		for (auto qs : qss)
		{
			ret.append(qs);
		}
	}

	return ret;
}

boost::python::list analyze_pos(std::wstring str)
{
	boost::python::list ret;

	if (!check_dic_loaded())
	{
		return ret;
	}

	Paragraph para;
	para.Set(str.c_str());
	auto sens = para.GetSentences();
	for (auto sen : sens)
	{
		auto poses = sen.AnalyzePos();
		for (auto src : poses)
		{
			dict dst;
			dst["word"] = src.word;
			dst["pos"] = g_posdic.GetPosTypeName(src.pos->GetType());
			if (src.pos->GetType() == POSTYPE_PREDICATE)
			{
				dst["root"] = src.pos->GetString();
			}

			list metas;
			for (auto meta : src.pos->GetMetas())
			{
				metas.append(g_posdic.GetMetaName(meta));
			}
			dst["meta"] = metas;

			ret.append(dst);
		}
	}

	return ret;
}


BOOST_PYTHON_MODULE(ntpostag)
{
	def("greet", greet);

	def("load_dic", load_dic);
	def("sentences", split_sentences);
	def("quoted_strings", get_quoted_strings);
	def("pos", analyze_pos);
}

#endif
#include "stdafx.h"
#include "postag.h"

#ifndef _NOUSE_CPPREST

#include <cpprest/http_listener.h>

using namespace web;
using namespace web::http::experimental;

BEGIN_NAMESPACE_NTPOSTAG

static void ReplyBadRequest(http::http_request& request)
{
	request.reply(http::status_codes::BadRequest, U("Invalid path."));
}

static void ReplyInvalidUri(http::http_request& request)
{
	request.reply(http::status_codes::NotFound, U("Unknown URI."));
}

static void ReplyPosTagRequest(http::http_request& request)
{
#ifdef _WIN32
	request.extract_utf16string().then([=](utf16string input) {
#else
	request.extract_utf8string().then([=](utf8string input) {
#endif

		json::value jsonBody;
		jsonBody[U("input")] = json::value::string(utility::conversions::to_string_t(input));

#ifdef _WIN32
		Sentence sen(input.c_str());
#else
		const char* utf8str = input.c_str();
		std::mbstate_t state = std::mbstate_t();
		int lenInput = 1 + std::mbsrtowcs(NULL, &utf8str, 0, &state);
		std::vector<wchar_t> wstr(lenInput);
		std::mbsrtowcs(&wstr[0], &utf8str, wstr.size(), &state);
		Sentence sen(&wstr[0]);
#endif

		sen.Analyze();

		jsonBody[U("result")] = sen.GetResultJson();
		jsonBody[U("unknown_words")] = sen.GetUnknownWords();

		request.reply(http::status_codes::OK, jsonBody);
	});
}

static void HandleRestPost(http::http_request request)
{
	const auto& reluri = request.relative_uri();
	//WidePrintf(L"POST: %ls\n", reluri.path().c_str());
	auto paths = uri::split_path(uri::decode(reluri.path()));

	if (paths.empty())
	{
		ReplyBadRequest(request);
		return;
	}

	const utility::string_t& firstToken = paths.at(0);
	if (!firstToken.compare(U("api")))
	{
		if (paths.size() == 2 && !paths.at(1).compare(U("postag")) )
		{
			ReplyPosTagRequest(request);
			return;
		}
	}

	ReplyInvalidUri(request);
}

static bool s_isTerminateApiListen;

#ifdef _WIN32
static BOOL WINAPI CtrlHandler(DWORD nSignal)
#else
static void OnSignal(int nSignal)
#endif
{
	printf("Got console signal: %d\n", nSignal);
	s_isTerminateApiListen = true;

#ifdef _WIN32
	return TRUE;
#endif
}

static void _ListenMain(int nWebPort)
{
	uri_builder uri;
	uri.set_scheme(U("http"));
#if defined(_WIN32) && defined(_DEBUG)
	uri.set_host(U("localhost"));
#else
	//uri.set_host(U("*"));
	uri.set_host(U("0.0.0.0"));
#endif
	uri.set_port(nWebPort);

	boost::scoped_ptr<listener::http_listener> pRestApiListener(new listener::http_listener(uri.to_uri()));
	pRestApiListener->support(http::methods::POST, std::bind(HandleRestPost, std::placeholders::_1));
	pRestApiListener
		->open()
		.then([nWebPort]() { printf("[NTPosTag] Start listening on TCP port %d...\n", nWebPort); })
		.wait();

#ifdef _WIN32
	SetConsoleCtrlHandler(CtrlHandler, TRUE);
#else
	struct sigaction sa;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_NODEFER;// | SA_RESETHAND;
	sa.sa_handler = OnSignal;
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);
#endif

	//FIXME: use system event
	s_isTerminateApiListen = false;
	while (!s_isTerminateApiListen)
	{
#ifdef _WIN32
		Sleep(1000);
#else
		sleep(1);
#endif
	}

	pRestApiListener->close().wait();
}

bool StartApiListen(int nWebPort)
{
	try
	{
		_ListenMain(nWebPort);
		return true;
	}
	catch (std::exception const & e)
	{
		std::wcout << e.what() << std::endl;
		return false;
	}
}

END_NAMESPACE_NTPOSTAG

#endif // _NOUSE_CPPREST

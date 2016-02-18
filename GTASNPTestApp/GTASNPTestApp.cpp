#include "stdafx.h"
#include <stdio.h>
#include <iostream>
#include <cpr/cpr.h>
#include <cpr/multipart.h>
#pragma comment(lib, "wldap32.lib")
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "libcurl.lib")

int main()
{
	auto url = cpr::Url{ "http://gtasnp.com/upload/process" };
	auto multipart = cpr::Multipart{ { "file", cpr::File{ "GTASAsf1.b" } } };
	auto header = cpr::Header
	{
		{ "Host", "gtasnp.com" },
		{ "Accept", "application/json" },
		{ "Accept-Encoding", "gzip, deflate" },
		{ "Cache-Control", "no-cache" },
		{ "X-Requested-With", "XMLHttpRequest" },
		{ "Referer", "http://gtasnp.com/upload" }
	};

	auto r = cpr::Post(url, multipart, header);
	std::cout << r.text << std::endl;
	return 0;
}


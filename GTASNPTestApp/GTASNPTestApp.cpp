#include "stdafx.h"
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <cpr/cpr.h>
#include <cpr/multipart.h>
#pragma comment(lib, "wldap32.lib")
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "libcurl.lib")

int main()
{
	std::string save("http://gtasnp.com/download/file/EMg5?slot=1");

	auto r = cpr::Get(cpr::Url{ save });

	if (r.status_code == 200)
	{
		std::cout << r.status_code << std::endl;

		std::fstream ifs;
		ifs.open(".\\1.bin", std::fstream::binary | std::fstream::in | std::fstream::out | std::fstream::trunc);

		if (ifs.is_open())
		{
			ifs << r.text;
			ifs.close();
		}

	}
	

	//upload
	/*auto url = cpr::Url{ "http://gtasnp.com/upload/process" };
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
	std::cout << r.text << std::endl;*/
	return 0;
}


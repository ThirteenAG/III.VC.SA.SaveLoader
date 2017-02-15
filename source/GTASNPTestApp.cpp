#include "stdafx.h"

int main()
{
	std::string save("http://gtasnp.com/download/file/ZAzP7H?slot=1");

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


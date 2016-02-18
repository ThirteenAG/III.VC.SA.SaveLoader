#pragma warning (disable:4996) //gvm
#include "stdafx.h"
#include <stdio.h>
#include <iostream>
#include <string>
#include <cpr/cpr.h>
#include <cpr/multipart.h>
#include "..\includes\hooking\Hooking.Patterns.h"
#include "..\includes\injector\injector.hpp"
#include "..\includes\injector\assembly.hpp"
#include "..\includes\injector\hooking.hpp"
#include "..\includes\injector\calling.hpp"
#include "..\includes\injector\utility.hpp"
#include "IniReader.h"
#include <stdint.h>
#pragma comment(lib, "wldap32.lib")
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "libcurl.lib")

bool bDelay;
auto& gvm = injector::address_manager::singleton();

int32_t nSaveNum;
bool bSkipIntro, bSkipOutro;
bool bDisableLoadingScreens;
bool bUploadSaves, bDownloadSaves;
char* szCustomUserFilesDirectory;

char* pUserDirPath;

uint32_t bCurrentSaveSlot;
uint32_t* TheText;
static wchar_t backupText[100];
wchar_t* (__thiscall *pfGetText)(int, char *);
const static wchar_t SnPString[] = L"THIS IS GTASNP TEST STRING";

DWORD WINAPI Thread(LPVOID)
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
	MessageBox(0, r.text.c_str(), r.text.c_str(), 0);
}

injector::hook_back<bool(__fastcall*)(DWORD* _this, char bSlotIndex)> hbPcSaveSaveSlot;
bool __fastcall PcSaveSaveSlotHook(DWORD* _this, char bSlotIndex)
{
	_asm mov bCurrentSaveSlot, eax //for some reason bSlotIndex returns dl instead of eax
	return hbPcSaveSaveSlot.fun(_this, bSlotIndex);
}

injector::hook_back<void(__fastcall*)(DWORD* _this, int PageId)> hbMenuGotoPageHook;
void __fastcall MenuGotoPageHook(DWORD* _this, int PageId)
{
	wchar_t* ptr = pfGetText((int)TheText, "FES_SSC");
	if (backupText[0] == 0)
		wcscpy(backupText, ptr);

	if (bCurrentSaveSlot == 7) //8th
	{
		wcscpy(ptr, SnPString);


	}
	else
	{
		wcscpy(ptr, backupText);
	}

	return hbMenuGotoPageHook.fun(_this, PageId);
}

char* __cdecl InitUserDirectories()
{
	CreateDirectory(szCustomUserFilesDirectory, NULL);
	return szCustomUserFilesDirectory;
}

void GetSystemTimeFromSave(SYSTEMTIME& SystemLastWriteTime, WIN32_FIND_DATA& fd)
{
	FILE* hFile = fopen(fd.cFileName, "rb");
	if (hFile) {
		fseek(hFile, 0x34, SEEK_SET);
		fread(&SystemLastWriteTime, sizeof(SYSTEMTIME), 1, hFile);
		fclose(hFile);
	}
	SystemTimeToFileTime(&SystemLastWriteTime, &fd.ftLastWriteTime);
}

void FindFiles()
{
	nSaveNum = 129;
	std::string SFPath(pUserDirPath);
	SFPath += "*.b";

	SYSTEMTIME SystemLastWriteTime;
	WIN32_FIND_DATA fd;
	HANDLE File = FindFirstFile(SFPath.c_str(), &fd);
	GetSystemTimeFromSave(SystemLastWriteTime, fd);
	FILETIME LastWriteTime = fd.ftLastWriteTime;

	if (File != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (CompareFileTime(&fd.ftLastWriteTime, &LastWriteTime) >= 0)
			{
				LastWriteTime = fd.ftLastWriteTime;
				std::string str(fd.cFileName);
				auto n = str.find_first_of("0123456789");
				if (n != std::string::npos)
				{
					nSaveNum = std::atoi(&str[n]);
				}
			}
			GetSystemTimeFromSave(SystemLastWriteTime, fd);
		} while (FindNextFile(File, &fd));
		FindClose(File);
	}
}

injector::hook_back<void(__cdecl*)(void)> hbFrontendIdle;
void __cdecl FrontendIdleHook()
{
	bool bNoLoad = (GetAsyncKeyState(VK_SHIFT) & 0xF000) != 0;
	if (!bNoLoad && nSaveNum != -1)
	{
		if (nSaveNum == 0)
			FindFiles();

		//MessageBox(0, std::to_string(nSaveNum).c_str(), 0, 0);

		if (nSaveNum != 0 && nSaveNum != 129)
		{
			auto pattern = hook::pattern("8B 44 24 04 C7 05 ? ? ? ? 00 00 00 00 68 ? ? ? ? 50");
			static auto CheckSlotDataValid = (bool(__cdecl*)(int))(pattern.get(0).get<uint32_t>(0));
			if (!CheckSlotDataValid(nSaveNum - 1))
			{
				nSaveNum = 129;
			}
		}

		auto pattern = hook::pattern("53 B9 ? ? ? ? 83 EC 28 68 ? ? ? ? 68"); //0x869630
		static uint32_t* CMenuManager = *pattern.get(0).get<uint32_t*>(2);
		if (nSaveNum == 129) //NG
		{
			uint32_t NewGameStart = 7;
			injector::WriteMemory((uint32_t)CMenuManager + 0xF8, NewGameStart);
		}
		else
		{
			injector::WriteMemory<char>((uint32_t)CMenuManager + 0x100, nSaveNum - 1); //LastUsedSlot
			injector::WriteMemory<char>((uint32_t)CMenuManager + 0xF8, 12); //currentMenuItem
		}

		injector::WriteMemory<uint8_t>((uint32_t)CMenuManager + 0x11, 1);
		injector::WriteMemory<uint8_t>((uint32_t)CMenuManager + 0x38, 0);
		injector::WriteMemory<uint8_t>((uint32_t)CMenuManager + 0x39, 1);
		injector::WriteMemory<uint8_t>((uint32_t)CMenuManager + 0x3C, 1);
	}
	auto pattern = hook::pattern("E8 ? ? ? ? 83 C4 08 B8 01 00 00 00 5B C3");
	auto pattern2 = hook::pattern("83 EC 08 E8 ? ? ? ? DD D8");
	injector::MakeCALL(pattern.get(3).get<uint32_t>(0), pattern2.get(0).get<uint32_t>(0)); //0x4A5BF2 0x4A5C60
	return hbFrontendIdle.fun();
}

void III()
{

}

void VC()
{
	auto pattern = hook::pattern("68 ? ? ? ? 68 ? ? ? ? 50 C7 84 24 80 00 00 00 00");
	pUserDirPath = *pattern.get(0).get<char*>(1);

	pattern = hook::pattern("C7 05 ? ? ? ? 00 00 00 00 C7 05 ? ? ? ? 00 00 00 00 E8");
	static uint32_t* dword_813D44 = *pattern.get(17).get<uint32_t*>(12);
	static uint32_t* dword_601A40 = pattern.get(17).get<uint32_t>(25);
	struct psInitialize
	{
		void operator()(injector::reg_pack&)
		{
			injector::WriteMemory(dword_813D44, 0, true);

			if (strncmp(szCustomUserFilesDirectory, "0", 1) != 0)
			{
				auto pattern = hook::pattern("E8 ? ? ? ? 50 E8 ? ? ? ? 59 C3");
				injector::MakeCALL(pattern.get(0).get<uint32_t>(0), InitUserDirectories, true); //0x48E020
				pattern = hook::pattern("E8 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 89 C5");
				injector::MakeCALL(pattern.get(2).get<uint32_t>(0), InitUserDirectories, true); //0x61D8CA
				injector::MakeCALL(dword_601A40, InitUserDirectories, true); //0x601A40
				pattern = hook::pattern("50 E8 ? ? ? ? 59 E8 ? ? ? ? C7 05 ? ? ? ? 00 00 00 00");
				injector::MakeJMP((uint32_t)dword_601A40 + 5, pattern.get(0).get<uint32_t>(0), true);
			}

			if (bSkipIntro)
			{
				auto pattern = hook::pattern("C7 05 ? ? ? ? 01 00 00 00 E9");
				injector::WriteMemory<uint8_t>(pattern.get(3).get<uint32_t>(6), 0x05, true); //0x5FFFAB
			}

			if (bSkipOutro)
			{
				auto pattern = hook::pattern("83 3D ? ? ? ? 00 75 10");
				auto pattern2 = hook::pattern("6A 00 6A 1E E8 ? ? ? ? 59 59");
				injector::MakeJMP(pattern.get(0).get<uint32_t>(0), pattern2.get(0).get<uint32_t>(0), true); // 0x495809 0x49596E outro
			}

			if (bDisableLoadingScreens)
			{
				auto pattern = hook::pattern("53 83 EC 68 68 ? ? ? ? E8");
				injector::WriteMemory<uint8_t>(pattern.get(0).get<uint32_t>(0), 0xC3, true); //0x4A69D0
			}

			auto pattern = hook::pattern("E8 ? ? ? ? 83 C4 08 B8 01 00 00 00 5B C3");
			hbFrontendIdle.fun = injector::MakeCALL(pattern.get(3).get<uint32_t>(0), FrontendIdleHook).get(); //0x4A5BF2
		}
	}; injector::MakeInline<psInitialize>(pattern.get(17).get<uint32_t>(10), pattern.get(17).get<uint32_t>(20));

	if (bUploadSaves)
	{
		auto pattern = hook::pattern("E8 ? ? ? ? B9 ? ? ? ? 88 C3 E8 ? ? ? ? 84 DB");
		hbPcSaveSaveSlot.fun = injector::MakeCALL(pattern.get(0).get<uint32_t>(0), PcSaveSaveSlotHook).get(); //0x49728C

		pattern = hook::pattern("E8 ? ? ? ? EB 4E");
		hbMenuGotoPageHook.fun = injector::MakeCALL(pattern.get(0).get<uint32_t>(0), MenuGotoPageHook).get(); //0x4972A5

		pattern = hook::pattern("E8 ? ? ? ? DB 05 ? ? ? ? 50 89 C3 D8 0D");
		auto GetTextCall = pattern.get(0).get<uint32_t>(0);
		auto GetText = injector::GetBranchDestination(GetTextCall, true).as_int();
		pfGetText = (wchar_t *(__thiscall *)(int, char *))GetText;
		TheText = *pattern.get(0).get<uint32_t*>(-9);
	}

}

void SA()
{

}

DWORD WINAPI Init(LPVOID)
{
	CIniReader iniReader("");
	nSaveNum = iniReader.ReadInteger("MAIN", "LoadSlot", 0);
	bSkipIntro = iniReader.ReadInteger("MAIN", "SkipIntro", 1) != 0;
	bSkipOutro = iniReader.ReadInteger("MAIN", "SkipOutro", 1) != 0;
	bDisableLoadingScreens = iniReader.ReadInteger("MAIN", "DisableLoadingScreens", 1) != 0;
	szCustomUserFilesDirectory = iniReader.ReadString("MAIN", "CustomUserFilesDirectoryInGameDir", "0");

	bUploadSaves = iniReader.ReadInteger("GTASnP.com", "UploadSaves", 1) != 0;
	bDownloadSaves = iniReader.ReadInteger("GTASnP.com", "DownloadSaves", 1) != 0;

	auto pattern = hook::pattern("64 89 25 00 00 00 00");
	if (!(pattern.size() > 0) && !bDelay)
	{
		bDelay = true;
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Init, NULL, 0, NULL);
		return 0;
	}

	if (bDelay)
	{
		while (!(pattern.size() > 0))
			pattern = hook::pattern("64 89 25 00 00 00 00");
	}


	if (gvm.IsIII())
	{
		III();
	}
	else
	{
		if (gvm.IsVC())
		{
			VC();
		}
		else
		{
			if (gvm.IsSA())
			{
				SA();
			}
		}
	}
	return 0;
}


BOOL APIENTRY DllMain(HINSTANCE hInst, DWORD reason, LPVOID)
{
	if (reason == DLL_PROCESS_ATTACH)
	{

		Init(NULL);
	}
	return TRUE;
}

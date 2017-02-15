#include "stdafx.h"

bool bDelay;
auto& gvm = injector::address_manager::singleton();

int32_t nSaveNum;
bool bSkipIntro, bSkipOutro, bDisableLoadingScreens;
bool bUploadSaves, bDownloadSaves, bCopyUrlToClipboard;
char szCustomUserFilesDirectory[MAX_PATH];

char* pUserDirPath;
uint32_t bCurrentSaveSlot;
uint32_t* TheText;
static wchar_t backupText[50];
static wchar_t backupText2[50];
size_t bckpTxtSize, bckpTxtSize2;
wchar_t* (__thiscall *pfGetText)(int, char *);
char* (__thiscall *pfGetTextSA)(int, char *);
const static wchar_t SnPString[] = L"Uploading to gtasnp.com...";
const static char SnPStringSA[] = "Uploading to gtasnp.com...";

char* __cdecl InitUserDirectories()
{
	CreateDirectory(szCustomUserFilesDirectory, NULL);
	return szCustomUserFilesDirectory;
}

DWORD WINAPI UploadSave(LPVOID lpParameter)
{
	std::string SFPath(pUserDirPath);
	SFPath += "8.b";
	auto url = cpr::Url{ "http://gtasnp.com/upload/process" };
	auto multipart = cpr::Multipart{ { "file", cpr::File{ SFPath } } };
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

	Json::Value parsedFromString;
	Json::Reader reader;
	bool parsingSuccessful = reader.parse(r.text, parsedFromString);
	if (parsingSuccessful && parsedFromString["error_code"].asBool() == false)
	{
		/*
		{
		game:          'gtasa_pc', // string if successful, false if error
		error_code:    false,      // string if error, false if no error
		error_message: '',         // always string, empty if no error
		uuid:          'xhnDKn'    // string if successful, false if error
		}
		*/
		std::string result;
		if (gvm.IsSA())
		{
			std::string wc("Save uploaded to gtasnp.com/");
			result = (parsedFromString["uuid"].asCString());
			wc += result;
			strncpy((char*)lpParameter, &wc[0], bckpTxtSize);
		}
		else
		{
			std::wstring wc(L"Save uploaded to gtasnp.com/");
			if (gvm.IsIII())
			{
				wc = L"URL: gtasnp.com/";
			}
			result = (parsedFromString["uuid"].asCString());
			std::wstring uuid(result.size(), L'#');
			mbstowcs(&uuid[0], result.c_str(), result.size());
			wc += uuid;
			wcsncpy((wchar_t*)lpParameter, &wc[0], bckpTxtSize);
		}

		CIniReader iniWriter("");
		result = "http://gtasnp.com/" + result;
		iniWriter.WriteString("GTASnP.com", "LatestUpload", (char*)result.c_str());

		if (bCopyUrlToClipboard)
		{
			OpenClipboard(0);
			EmptyClipboard();
			HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, result.size() + 1);
			if (hg)
			{
				memcpy(GlobalLock(hg), result.c_str(), result.size() + 1);
				GlobalUnlock(hg);
				SetClipboardData(CF_TEXT, hg);
				CloseClipboard();
				GlobalFree(hg);
			}
			CloseClipboard();
		}
		//MessageBox(0, parsedFromString["uuid"].asCString(), 0, 0);
	}
	else
	{
		std::string result(parsedFromString["error_message"].asCString());
		std::wstring wc(result.size(), L'#');
		mbstowcs(&wc[0], result.c_str(), result.size());
		wcsncpy((wchar_t*)lpParameter, &wc[0], bckpTxtSize);
	}

	return 1;
}

DWORD WINAPI DownloadSave(LPVOID lpParameter)
{
	std::string SFPath(pUserDirPath);
	SFPath += "8.b";

	std::string ID((char*)lpParameter);
	ID = ID.substr(ID.find_last_of("/") + 1);

	std::string URL;
	URL = "gtasnp.com/download/file/" + ID + "?slot=8";

	auto r = cpr::Get(cpr::Url{ URL });

	if (r.status_code == 200)
	{
		std::fstream fs;
		fs.open(SFPath, std::fstream::binary | std::fstream::in | std::fstream::out | std::fstream::trunc);

		if (fs.is_open())
		{
			fs << r.text;
			fs.close();
		}
	}
	return r.status_code;
}

injector::hook_back<char(__fastcall*)(DWORD* _this, int bSlotIndex)> hbPcSaveSaveSlot;
char __fastcall PcSaveSaveSlotHook(DWORD* _this, int bSlotIndex)
{
	_asm mov bCurrentSaveSlot, eax //for some reason bSlotIndex returns dl instead of eax
	return hbPcSaveSaveSlot.fun(_this, bSlotIndex);
}

injector::hook_back<bool(__cdecl*)(int bSlotIndex)> hbCheckSlotDataValid;
bool __cdecl CheckSlotDataValidHook(int nSlotIndex)
{
	CIniReader iniReader("");
	static char* szLatestUpload = iniReader.ReadString("GTASnP.com", "LatestUpload", "");

	if (szLatestUpload[0] != 0)
	{
		wchar_t* ptr;
		if (!gvm.IsSA() && !gvm.IsIII())
		{
			ptr = pfGetText((int)TheText, "FELD_WR");
			if (backupText2[0] == 0)
			{
				bckpTxtSize2 = wcslen(ptr);
				wcsncpy(backupText2, ptr, bckpTxtSize2);
			}
		}

		if (nSlotIndex == 7)
		{
			auto status_code = DownloadSave(szLatestUpload);
			if (status_code == 200)
			{
				if (!gvm.IsSA() && !gvm.IsIII())
					wcsncpy(ptr, L"Save loaded from gtasnp.com", bckpTxtSize2);
			}
			else
			{
				if (!gvm.IsSA() && !gvm.IsIII())
					wcsncpy(ptr, L"Error downloading save file.", bckpTxtSize2);
			}
		}
		else
		{
			if (!gvm.IsSA() && !gvm.IsIII())
				wcsncpy(ptr, backupText2, bckpTxtSize2);
		}
	}
	return hbCheckSlotDataValid.fun(nSlotIndex);
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
				str.erase(0, 4); //del GTA3
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

injector::hook_back<void(__fastcall*)(DWORD* _this, int PageId)> hbMenuGotoPageHook;
void __fastcall MenuGotoPageHook(DWORD* _this, int PageId)
{
	wchar_t* ptr = pfGetText((int)TheText, "FES_SSC");
	if (backupText[0] == 0)
	{
		bckpTxtSize = wcslen(ptr);
		wcsncpy(backupText, ptr, bckpTxtSize);
	}

	if (bCurrentSaveSlot == 7) //8th
	{
		wcsncpy(ptr, SnPString, bckpTxtSize);
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&UploadSave, ptr, 0, NULL);
	}
	else
	{
		wcsncpy(ptr, backupText, bckpTxtSize);
	}

	return hbMenuGotoPageHook.fun(_this, PageId);
}

DWORD RsCameraBeginUpdateNOP()
{
	return 0;
}

void SimulateCopyrightScreen()
{
	// Simulate that the copyright screen happened
	*injector::memory_pointer(0x8D093C).get<int>() = 0;       // Previous splash index = copyright notice
	*injector::memory_pointer(0xBAB340).get<float>() -= 1000.0;// Decrease timeSinceLastScreen, so it will change immediately
	*injector::memory_pointer(0xBAB31E).get<char>() = 1;      // First Loading Splash
}

injector::hook_back<void(__cdecl*)(void)> hbFrontendIdleSA;
void __cdecl FrontendIdleHookSA()
{
	static int nTimes = 0;
	injector::MakeCALL(0x53E80E, RsCameraBeginUpdateNOP, true);

	if (++nTimes >= 2)
	{
		bool bNoLoad = (GetAsyncKeyState(VK_SHIFT) & 0xF000) != 0;
		if (!bNoLoad && nSaveNum != -1)
		{
			if (nSaveNum == 0)
				FindFiles();

			//MessageBox(0, std::to_string(nSaveNum).c_str(), 0, 0);

			if (nSaveNum != 0 && nSaveNum != 129)
			{
				static auto CheckSlotDataValid = (bool(__cdecl*)(int))0x5D1380;
				if (!CheckSlotDataValid(nSaveNum - 1))
				{
					nSaveNum = 129;
				}
			}

			static uint32_t CMenuManager = 0xBA6748;
			if (nSaveNum == 129) //NG
			{
				*injector::memory_pointer(CMenuManager + 0x5D).get<char>() = 1;
				*injector::memory_pointer(CMenuManager + 0x5C).get<char>() = 0; //menu.m_bMenuActive

			}
			else
			{
				// Make the game load automatically
				*injector::memory_pointer(CMenuManager + 0x32).get<char>() = 0;  // menu.bDeactivateMenu
				*injector::memory_pointer(CMenuManager + 0x15F).get<char>() = (nSaveNum - 1);     // menu.SaveNumber
				*injector::memory_pointer(0xB72910).get<char>() = 0;              // game.bMissionPack

				// Simulate that we came into the menu and clicked to load game
				*injector::memory_pointer(CMenuManager + 0x15D).get<char>() = 13;
				*injector::memory_pointer(CMenuManager + 0x1B3C).get<char>() = 1;
			}
			*injector::memory_pointer(0xB7CB49).get<char>() = 0; //game.m_UserPause
		}

		injector::MakeCALL(0x53E80E, 0x619450);
		injector::MakeCALL(0x53ECCB, 0x53E770);
	}

	return hbFrontendIdleSA.fun();
}

injector::hook_back<void(__fastcall*)(void*)> hbFrontendIdle;
void FrontendIdleHook(void* menuManager)
{
	_asm pushad
	static bool bMenuInit = false;
	bool bNoLoad = (GetAsyncKeyState(VK_SHIFT) & 0xF000) != 0;
	if ((!bNoLoad && nSaveNum != -1) && !bMenuInit)
	{
		if (nSaveNum == 0)
			FindFiles();

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
		if (gvm.IsIII())
			pattern = hook::pattern("? B9 ? ? ? ? C6 05 ? ? ? ? 01 E8 ? ? ? ? B9 ? ? ? ? C6"); //0x8F59D8 gta3

		static uint32_t* CMenuManager = *pattern.get(0).get<uint32_t*>(2);

		if (gvm.IsVC())
		{
			pattern = hook::pattern("80 3D ? ? ? ? 00 53 89 CB ? ? ? ? ? ? 80"); //0x498E5F
			static auto ProcessOnOffMenuOptions = (void(__fastcall*)(uint32_t* _this))(pattern.get(0).get<uint32_t>(0));
			ProcessOnOffMenuOptions(CMenuManager);
		}
		else
		{
			if (nSaveNum == 129) //NG
			{
				if (gvm.IsIII())
				{
					static auto dword_485134 = hook::pattern("C6 85 ? 01 00 00 00 E8 ? ? ? ? 6A").get(0).get<uint32_t>(2);
					injector::WriteMemory<uint8_t>(dword_485134, 0x10, true);

					auto dword_48C7F8 = hook::pattern("B9 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 43 83 FB 32");
					static auto dword_8F59D8 = *dword_48C7F8.get(0).get<uint32_t*>(1);

					struct Hook
					{
						void operator()(injector::reg_pack& regs)
						{
							regs.eax = (uint32_t)dword_8F59D8;
							injector::WriteMemory<uint8_t>(dword_485134, 0x14, true);
							injector::WriteMemory<uint8_t>((uint32_t)CMenuManager + 0x111, 0, true);
						}
					}; injector::MakeInline<Hook>(dword_48C7F8.get(0).get<uint32_t>(0), dword_48C7F8.get(0).get<uint32_t>(5));
				}

				uint32_t NewGameStart = gvm.IsIII() ? 10 : 7;
				injector::WriteMemory((uint32_t)CMenuManager + (gvm.IsIII() ? 0x548 : 0xF8), NewGameStart, true);
			}
			else
			{
				injector::WriteMemory<uint8_t>((uint32_t)CMenuManager + (gvm.IsIII() ? 0x55C : 0x100), nSaveNum - 1, true); //LastUsedSlot
				injector::WriteMemory<uint8_t>((uint32_t)CMenuManager + (gvm.IsIII() ? 0x548 : 0xF8), gvm.IsIII() ? 14 : 12, true); //currentMenuItem
			}
		}

		bMenuInit = true;
		injector::WriteMemory<uint8_t>((uint32_t)CMenuManager + (gvm.IsIII() ? 0x115 : 0x11), 1, true);
		injector::WriteMemory<uint8_t>((uint32_t)CMenuManager + (gvm.IsIII() ? 0x111 : 0x38), 0, true);
		injector::WriteMemory<uint8_t>((uint32_t)CMenuManager + (gvm.IsIII() ? 0x114 : 0x39), 1, true);
		injector::WriteMemory<uint8_t>((uint32_t)CMenuManager + (gvm.IsIII() ? 0x454 : 0x3C), 1, true);
	}

	_asm popad
	return hbFrontendIdle.fun(menuManager);
};

void III()
{
	auto pattern = hook::pattern("68 ? ? ? ? 68 ? ? ? ? 50 C7 84 24 80 00 00 00 00");
	pUserDirPath = *pattern.get(0).get<char*>(1);

	pattern = hook::pattern("C7 05 ? ? ? ? 00 00 00 00 C7 05 ? ? ? ? 00 00 00 00 E8"); //0x5811CE
	static uint32_t* dword_72CF84 = *pattern.get(10).get<uint32_t*>(12);

	struct psInitialize
	{
		void operator()(injector::reg_pack&)
		{
			injector::WriteMemory(dword_72CF84, 0, true);

			if (strncmp(szCustomUserFilesDirectory, "0", 1) != 0)
			{
				char moduleName[MAX_PATH];
				GetModuleFileName(NULL, moduleName, MAX_PATH);
				*((strrchr(moduleName, '\\')) + 1) = '\0';
				strcat(moduleName, szCustomUserFilesDirectory);
				strcpy(szCustomUserFilesDirectory, moduleName);

				auto pattern = hook::pattern("E8 ? ? ? ? 50 E8 ? ? ? ? 59 C3");
				injector::MakeCALL(pattern.get(0).get<uint32_t>(0), InitUserDirectories, true); //0x479080
				pattern = hook::pattern("E8 ? ? ? ? 50 E8 ? ? ? ? 59 E8");
				injector::MakeCALL(pattern.get(0).get<uint32_t>(0), InitUserDirectories, true); //0x5811DD
				pattern = hook::pattern("E8 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 89 C5 59 85 ED");
				injector::MakeCALL(pattern.get(1).get<uint32_t>(0), InitUserDirectories, true); //0x591EDD
			}

			if (bSkipIntro)
			{
				auto pattern = hook::pattern("C7 05 ? ? ? ? 01 00 00 00 E9");
				injector::WriteMemory<uint8_t>(pattern.get(5).get<uint32_t>(6), 0x05, true); //0x582A75
			}

			if (bDisableLoadingScreens)
			{
				auto pattern = hook::pattern("53 83 EC 50");
				injector::WriteMemory<uint8_t>(pattern.get(0).get<uint32_t>(0), 0xC3, true); //0x48D770
			}

			auto pattern = hook::pattern("E8 ? ? ? ? 83 ? ? ? ? ? ? D9 EE 74 ? 83 C4 08 DD D8 C3"); //0x48E721
			hbFrontendIdle.fun = injector::MakeCALL(pattern.get(0).get<uint32_t>(0), FrontendIdleHook).get();
		}
	}; injector::MakeInline<psInitialize>(pattern.get(10).get<uint32_t>(10), pattern.get(10).get<uint32_t>(20));

	if (bUploadSaves)
	{
		pattern = hook::pattern("8B 85 48 05 00 00 89 85 54 05 00 00 C7 85"); //0x485217
		struct GotoPageIII
		{
			void operator()(injector::reg_pack& regs)
			{
				bCurrentSaveSlot = *(uint8_t *)(regs.ebp + 0x55C);

				wchar_t* ptr = pfGetText((int)TheText, "FES_SSC");
				if (backupText[0] == 0)
				{
					bckpTxtSize = wcslen(ptr);
					wcsncpy(backupText, ptr, bckpTxtSize);
				}

				if (bCurrentSaveSlot == 7) //8th
				{
					wcsncpy(ptr, SnPString, bckpTxtSize);
					CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&UploadSave, ptr, 0, NULL);
				}
				else
				{
					wcsncpy(ptr, backupText, bckpTxtSize);
				}

				regs.eax = *(uint32_t *)(regs.ebp + 0x548);
			}
		}; injector::MakeInline<GotoPageIII>(pattern.get(1).get<uint32_t>(0), pattern.get(1).get<uint32_t>(6));
	}

	if (bDownloadSaves)
	{
		auto pattern = hook::pattern("E8 ? ? ? ? 84 C0 59 0F 84");
		hbCheckSlotDataValid.fun = injector::MakeCALL(pattern.get(4).get<uint32_t>(0), CheckSlotDataValidHook).get(); //0x48525F
	}

	if (bUploadSaves || bDownloadSaves)
	{
		pattern = hook::pattern("B9 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? C2 04 00");
		auto GetTextCall = pattern.get(0).get<uint32_t>(10);
		auto GetText = injector::GetBranchDestination(GetTextCall, true).as_int();
		pfGetText = (wchar_t *(__thiscall *)(int, char *))GetText;
		TheText = *pattern.get(0).get<uint32_t*>(1);
	}
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
				char moduleName[MAX_PATH];
				GetModuleFileName(NULL, moduleName, MAX_PATH);
				*((strrchr(moduleName, '\\')) + 1) = '\0';
				strcat(moduleName, szCustomUserFilesDirectory);
				strcpy(szCustomUserFilesDirectory, moduleName);

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
				auto pattern = hook::pattern("53 83 EC 68 ? ? ? ? ? E8");
				injector::WriteMemory<uint8_t>(pattern.get(0).get<uint32_t>(0), 0xC3, true); //0x4A69D0
			}

			auto pattern = hook::pattern("E8 ? ? ? ? 83 ? ? ? ? ? ? D9 EE 74 ? 83 C4 08 DD D8 C3"); //0x4A5C88
			hbFrontendIdle.fun = injector::MakeCALL(pattern.get(0).get<uint32_t>(0), FrontendIdleHook).get();
		}
	}; injector::MakeInline<psInitialize>(pattern.get(17).get<uint32_t>(10), pattern.get(17).get<uint32_t>(20));

	if (bUploadSaves)
	{
		auto pattern = hook::pattern("E8 ? ? ? ? B9 ? ? ? ? 88 C3 E8 ? ? ? ? 84 DB");
		hbPcSaveSaveSlot.fun = injector::MakeCALL(pattern.get(0).get<uint32_t>(0), PcSaveSaveSlotHook).get(); //0x49728C

		pattern = hook::pattern("E8 ? ? ? ? EB 4E");
		hbMenuGotoPageHook.fun = injector::MakeCALL(pattern.get(0).get<uint32_t>(0), MenuGotoPageHook).get(); //0x4972A5
	}

	if (bDownloadSaves)
	{
		auto pattern = hook::pattern("E8 ? ? ? ? 84 C0 59 74 7D");
		hbCheckSlotDataValid.fun = injector::MakeCALL(pattern.get(0).get<uint32_t>(0), CheckSlotDataValidHook).get(); //0x49730C
	}

	if (bUploadSaves || bDownloadSaves)
	{
		pattern = hook::pattern("E8 ? ? ? ? DB 05 ? ? ? ? 50 89 C3 D8 0D");
		auto GetTextCall = pattern.get(0).get<uint32_t>(0);
		auto GetText = injector::GetBranchDestination(GetTextCall, true).as_int();
		pfGetText = (wchar_t *(__thiscall *)(int, char *))GetText;
		TheText = *pattern.get(0).get<uint32_t*>(-9);
	}
}

void SA()
{
	pUserDirPath = (char*)0xC16F18;

	struct psInitialize
	{
		void operator()(injector::reg_pack&)
		{
			injector::WriteMemory(0xC8CF98, 0, true);

			if (strncmp(szCustomUserFilesDirectory, "0", 1) != 0)
			{
				char moduleName[MAX_PATH];
				GetModuleFileName(NULL, moduleName, MAX_PATH);
				*((strrchr(moduleName, '\\')) + 1) = '\0';
				strcat(moduleName, szCustomUserFilesDirectory);
				strcpy(szCustomUserFilesDirectory, moduleName);

				injector::MakeCALL(0x538860, InitUserDirectories, true);
				injector::MakeCALL(0x619075, InitUserDirectories, true);
				injector::MakeCALL(0x747470, InitUserDirectories, true);
			}

			if (bSkipIntro)
			{
				injector::MakeNOP(0x747483, 6);           // Disable gGameState = 0 setting
				injector::WriteMemory<int>(0xC8D4C0, 5);  // Put the game where the user wants (default's to the copyright screen)

				// Hook the copyright screen fading in/out and simulates that it has happened
				injector::MakeNOP(0x748C2B, 5);
				injector::MakeCALL(0x748C9A, injector::raw_ptr(SimulateCopyrightScreen));
			}

			if (bDisableLoadingScreens)
			{
				// Skip fading screen rendering
				injector::MakeJMP(0x590AE4, 0x590C9E);

				// Disable loading bar rendering
				injector::MakeNOP(0x5905B4, 5);

				// Disable loading screen rendering
				injector::MakeNOP(0x590D9F, 5);
				injector::WriteMemory<uint8_t>(0x590D9F, 0xC3, true);

				// Disable audio tune from loading screen
				injector::MakeNOP(0x748CF6, 5);
			}

			injector::MakeNOP(0x748CBD, 2);   // Let FrontentIdle run even when minimized
			hbFrontendIdleSA.fun = injector::MakeCALL(0x53ECCB, FrontendIdleHookSA).get();
		}
	}; injector::MakeInline<psInitialize>(0x74742F, 0x747439);

	if (bUploadSaves)
	{
		struct GotoPageSA
		{
			void operator()(injector::reg_pack& regs)
			{
				bCurrentSaveSlot = *(uint8_t *)(0xBA6748 + 0x15F);

				char* ptr = pfGetTextSA((int)TheText, "FES_SSC");
				if (backupText[0] == 0)
				{
					bckpTxtSize = strlen(ptr);
					strncpy((char*)backupText, ptr, bckpTxtSize);
				}

				if (bCurrentSaveSlot == 7) //8th
				{
					strncpy(ptr, SnPStringSA, bckpTxtSize);
					CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&UploadSave, ptr, 0, NULL);
				}
				else
				{
					strncpy(ptr, (char*)backupText, bckpTxtSize);
				}

				*(uint8_t *)0xBA8286 = 0;
			}
		}; injector::MakeInline<GotoPageSA>(0x578E2C, 0x578E2C + 0x7);
	}

	if (bDownloadSaves)
	{
		hbCheckSlotDataValid.fun = injector::MakeCALL(0x578EF2, CheckSlotDataValidHook).get();
	}

	if (bUploadSaves || bDownloadSaves)
	{
		pfGetTextSA = (char *(__thiscall *)(int, char *))0x6A0050;
		TheText = (uint32_t*)0xC1B340;
	}
}

DWORD WINAPI Init(LPVOID)
{
	CIniReader iniReader("");
	nSaveNum = iniReader.ReadInteger("MAIN", "LoadSlot", 0);
	bSkipIntro = iniReader.ReadInteger("MAIN", "SkipIntro", 1) != 0;
	bSkipOutro = iniReader.ReadInteger("MAIN", "SkipOutro", 1) != 0;
	bDisableLoadingScreens = iniReader.ReadInteger("MAIN", "DisableLoadingScreens", 1) != 0;
	char* sz = iniReader.ReadString("MAIN", "CustomUserFilesDirectoryInGameDir", "0");
	strcpy(szCustomUserFilesDirectory, sz);

	bUploadSaves = iniReader.ReadInteger("GTASnP.com", "UploadSaves", 1) != 0;
	bDownloadSaves = iniReader.ReadInteger("GTASnP.com", "DownloadSaves", 1) != 0;
	bCopyUrlToClipboard = iniReader.ReadInteger("GTASnP.com", "CopyUrlToClipboard", 1) != 0;

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
		III();
	else
		if (gvm.IsVC())
			VC();
		else
			if (gvm.IsSA())
				SA();

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
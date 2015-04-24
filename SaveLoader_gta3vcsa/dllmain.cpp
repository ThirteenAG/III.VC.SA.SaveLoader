#include "stdafx.h"
#include "IniReader.h"
#include "injector\injector.hpp"
#include "injector\hooking.hpp"
#include <string>
//#include <WinInet.h>
//#pragma comment(lib,"wininet.lib")
using namespace injector;

int saveNum, StartNewGame;
int LastUsedSlot, CMenuManagerStruct, LoadSave_3vc;
char currentMenuItem;

int *funcGameVersion;
int *loadingStage;
int *userdirPath;
char *isInMenu;
int *saveFileName;
char(__thiscall *LoadSave)(int, char);
int to_int(char const *s);
char* szCustomUserFilesDirectoryInGameDir;

void FindFiles()
{
	if (!saveNum)
	{
		char SFPath[MAX_PATH];
		strcpy(SFPath, (char *)userdirPath);
		strcat(SFPath, "*.b");

		WIN32_FIND_DATA fd;
		HANDLE File = FindFirstFile(SFPath, &fd);

		FILETIME LastWriteTime = fd.ftLastWriteTime;

		if (File != INVALID_HANDLE_VALUE)
		{
			do
			{
				if (CompareFileTime(&fd.ftLastWriteTime, &LastWriteTime) >= 0)
				{
					if (GetFileSize(File, NULL) >= 200820)
					{
						//MessageBox(0, 0, fd.cFileName, 0);
						LastWriteTime = fd.ftLastWriteTime;
						saveNum = to_int(fd.cFileName + 4);
					}
				}

			} while (FindNextFile(File, &fd));
			FindClose(File);
		}
	}
}

template<uintptr_t addr>
void FrontendIdleHook()
{
	using printstr_hook = injector::function_hooker<addr, void()>;
	injector::make_static_hook<printstr_hook>([](printstr_hook::func_type FrontendIdle)
	{
		static int nTimes = 0;

		FindFiles();

		// On the second processing tick, do the game start
		if (++nTimes == 2)
		{
			bool bNoLoad = (GetAsyncKeyState(VK_SHIFT) & 0xF000) != 0;
			if (bNoLoad)
			{
				return FrontendIdle();
			}

			auto& gvm = address_manager::singleton();

			if (gvm.IsSA())
			{
				if (saveNum && saveNum != 129)
				{
					WriteMemory<char>(LastUsedSlot, saveNum - 1);
					LoadSave(CMenuManagerStruct, currentMenuItem);
				}
				else
				{
					WriteMemory<char>(CMenuManagerStruct + 0x5C, 0);
					WriteMemory(StartNewGame, 0);
				}
			}
			else
			{
				if (saveNum && saveNum != 129)
				{
					WriteMemory<char>(LastUsedSlot, saveNum - 1);
					WriteMemory<char>(LoadSave_3vc, currentMenuItem);
				}
				else
				{
					WriteMemory(LoadSave_3vc, StartNewGame);

					if (gvm.IsIII())
					{
						WriteMemory<char>(CMenuManagerStruct + (0x8F5AED - 0x8F59D8), 1);
						WriteMemory<char>(CMenuManagerStruct + (0x8F5AE9 - 0x8F59D8), 0);
						WriteMemory<char>(CMenuManagerStruct + (0x8F5AEC - 0x8F59D8), 1);
						WriteMemory<char>(CMenuManagerStruct + (0x8F5E2C - 0x8F59D8), 1);
					}
					else if (gvm.IsVC())
					{
						WriteMemory<char>(CMenuManagerStruct + (0x869641 - 0x869630), 1);
						WriteMemory<char>(CMenuManagerStruct + (0x869668 - 0x869630), 0);
						WriteMemory<char>(CMenuManagerStruct + (0x869669 - 0x869630), 1);
						WriteMemory<char>(CMenuManagerStruct + (0x86966C - 0x869630), 1);
					}
				}
			}
		}

		return FrontendIdle();
	});
}

char* __cdecl InitUserDirectories()
{
	CreateDirectory(szCustomUserFilesDirectoryInGameDir, NULL);
	return szCustomUserFilesDirectoryInGameDir;
}

DWORD WINAPI Init(LPVOID param)
{
	CIniReader iniReader("");
	if ((!iniReader.ReadInteger("MAIN", "Enable", 1))) {
		return 0;
	}

	saveNum = iniReader.ReadInteger("MAIN", "LoadSlot", 0);
	bool bSkipIntro = iniReader.ReadInteger("MAIN", "SkipIntro", 1) == 1;
	bool bDisableLoadingScreens = iniReader.ReadInteger("MAIN", "DisableLoadingScreens", 1) == 1;

	szCustomUserFilesDirectoryInGameDir = iniReader.ReadString("MAIN", "CustomUserFilesDirectoryInGameDir", "");

	auto& gvm = address_manager::singleton();

	if (gvm.IsIII())
	{
		if (gvm.GetMajorVersion() == 1 && gvm.GetMinorVersion() == 0)
		{
			if (bSkipIntro)
			{
				injector::WriteMemory<char>(0x582A7B, 0x05, true);
			}

			if (bDisableLoadingScreens)
			{
				injector::WriteMemory<uint8_t>(0x48D770, 0xC3, true);
			}

			if (strncmp(szCustomUserFilesDirectoryInGameDir, "0", 1) != 0)
			{
				injector::MakeCALL(0x479080, InitUserDirectories, true); // = 0x580BB0 + 0x0  -> call    sub_580BB0
				injector::MakeCALL(0x5811DD, InitUserDirectories, true); // = 0x580BB0 + 0x0  -> call    sub_580BB0
				injector::MakeCALL(0x591EDD, InitUserDirectories, true); // = 0x580BB0 + 0x0  -> call    sub_580BB0
			}

			loadingStage = (int *)0x8F5838;
			userdirPath = (int *)0x8E28C0;
			currentMenuItem = 14;
			CMenuManagerStruct = 0x8F59D8;
			isInMenu = (char *)CMenuManagerStruct + 0x457;
			LastUsedSlot = CMenuManagerStruct + 0x55C;
			LoadSave_3vc = CMenuManagerStruct + 0x548;
			saveFileName = (int *)0x8E2CBC;
			StartNewGame = 10;
			WriteMemory<char>(0x485134, 0x10, true);
			FrontendIdleHook<(0x48E90F)>();
		}
		else
		{
			if (gvm.IsSteam())
			{
				loadingStage = (int *)0x905A2C;
				userdirPath = (int *)0x8F29B0;
				currentMenuItem = 14;
				CMenuManagerStruct = 0x905BCC;
				isInMenu = (char *)CMenuManagerStruct + 0x457;
				LastUsedSlot = CMenuManagerStruct + 0x55C;
				LoadSave_3vc = CMenuManagerStruct + 0x548;
				saveFileName = (int *)0x8F2EB0;
				StartNewGame = 10;
				WriteMemory<char>(0x485204, 0x10, true);
				FrontendIdleHook<(0x48E95F)>();//
			}
		}
	}
	else
	{
		if (gvm.IsVC())
		{
			if (gvm.GetMajorVersion() == 1 && gvm.GetMinorVersion() == 0)
			{
				if (strncmp(szCustomUserFilesDirectoryInGameDir, "0", 1) != 0)
				{
					injector::MakeCALL(0x48E020, InitUserDirectories, true); // = 0x602240 + 0x0->call    sub_602240
					injector::MakeCALL(0x61D8CA, InitUserDirectories, true); // = 0x602240 + 0x0->call    sub_602240
					injector::MakeCALL(0x601A40, InitUserDirectories, true);
					injector::MakeJMP(0x601A45, 0x601B2F, true);
				}

				if (bDisableLoadingScreens)
				{
					injector::WriteMemory<uint8_t>(0x4A69D0, 0xC3, true);
					injector::MakeJMP(0x495809, 0x49596E, true); //outro
				}

				if (bSkipIntro)
				{
					injector::WriteMemory<char>(0x5FFFAB, 0x05, true);
				}

				loadingStage = (int *)0x9B5F08;
				userdirPath = (int *)0x97509C;
				currentMenuItem = 12;
				CMenuManagerStruct = 0x869630;
				isInMenu = (char *)CMenuManagerStruct + 0x38;
				LastUsedSlot = CMenuManagerStruct + 0x100;
				LoadSave_3vc = CMenuManagerStruct + 0xF8;
				saveFileName = (int *)0x978428;
				StartNewGame = 7;
				FrontendIdleHook<(0x4A5BF2)>();
			}
			else
			{
				if (gvm.IsSteam())
				{
					loadingStage = (int *)0x9B4F10;
					userdirPath = (int *)0x9740A4;
					currentMenuItem = 12;
					CMenuManagerStruct = 0x868638;
					isInMenu = (char *)CMenuManagerStruct + 0x38;
					LastUsedSlot = CMenuManagerStruct + 0x100;
					LoadSave_3vc = CMenuManagerStruct + 0xF8;
					saveFileName = (int *)0x977430;
					StartNewGame = 7;
					FrontendIdleHook<(0x4A5AC2)>();
				}
			}
		}
		else
		{
			if (gvm.IsSA())
			{
				if (gvm.GetMajorVersion() == 1 && gvm.GetMinorVersion() == 0)
				{
					if (strncmp(szCustomUserFilesDirectoryInGameDir, "0", 1) != 0)
					{
						injector::MakeCALL(0x538860, InitUserDirectories, true); // = 0x744FB0 + 0x0->call    _InitUserDirectories; >> moved to CFileMgr::initialize
						injector::MakeCALL(0x619075, InitUserDirectories, true); // = 0x744FB0 + 0x0->call    _InitUserDirectories; >> moved to CFileMgr::initialize
						injector::MakeCALL(0x747470, InitUserDirectories, true); // = 0x744FB0 + 0x0->call    _InitUserDirectories; >> moved to CFileMgr::initialize
					}

					loadingStage = (int *)0xC8D4C0;
					LoadSave = (char(__thiscall *)(int, char)) 0x573680;
					userdirPath = (int *)0xC16F18;
					currentMenuItem = 13;
					CMenuManagerStruct = 0xBA6748;
					isInMenu = (char *)CMenuManagerStruct + 0x5C;
					LastUsedSlot = CMenuManagerStruct + 0x15F;
					saveFileName = (int *)0xC16DB8;
					StartNewGame = 0xB7CB49;
					FrontendIdleHook<(0x53ECCB)>();
				}
			}
		}
	}
	return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Init, NULL, 0, NULL);
	}
	return TRUE;
}

int to_int(char const *s)
{
	bool negate = (s[0] == '-');
	if (*s == '+' || *s == '-')
		++s;
	int result = 0;
	while (*s)
	{
		if (*s >= '0' && *s <= '9')
		{
			result = result * 10 - (*s - '0');
		}
		++s;
	}
	return negate ? result : -result;
}
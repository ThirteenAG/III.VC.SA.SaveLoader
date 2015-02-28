#include "stdafx.h"
#include "IniReader.h"
#include "injector\injector.hpp"
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
int *_saveFileName;
char(__thiscall *LoadSave)(int, char);
int to_int(char const *s);

DWORD WINAPI Init(LPVOID param)
{

    CIniReader iniReader("");
    if ((!iniReader.ReadInteger("MAIN", "Enable", 1))) {
        return 0;
    }

    auto& gvm = address_manager::singleton();

    if (gvm.IsIII())
    {
        if (gvm.GetMajorVersion() == 1 && gvm.GetMinorVersion() == 0)
        {
            loadingStage = (int *)0x8F5838;
            userdirPath = (int *)0x8E28C0;
            currentMenuItem = 14;
            CMenuManagerStruct = 0x8F59D8;
            isInMenu = (char *)CMenuManagerStruct + 0x457;
            LastUsedSlot = CMenuManagerStruct + 0x55C;
            LoadSave_3vc = CMenuManagerStruct + 0x548;
            _saveFileName = (int *)0x8E2CBC;
            StartNewGame = 10;
        }
        else
        {
            if (gvm.GetMajorVersion() == 1 && gvm.GetMinorVersion() == 1)
            {
                loadingStage = (int *)0x8F58EC;
                userdirPath = (int *)0x8E2870;
                CMenuManagerStruct = 0x8F5A8C;
                isInMenu = (char *)CMenuManagerStruct + 0x457;
                LastUsedSlot = CMenuManagerStruct + 0x55C;
                LoadSave_3vc = CMenuManagerStruct + 0x548;
                _saveFileName = (int *)0x8E2D70;
                StartNewGame = 10;
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
                    _saveFileName = (int *)0x8F2EB0;
                    StartNewGame = 10;
                }
            }
        }
    }
    else
    {
        if (gvm.IsVC())
        {
            if (gvm.GetMajorVersion() == 1 && gvm.GetMinorVersion() == 0)
            {
                loadingStage = (int *)0x9B5F08;
                userdirPath = (int *)0x97509C;
                currentMenuItem = 12;
                CMenuManagerStruct = 0x869630;
                isInMenu = (char *)CMenuManagerStruct + 0x38;
                LastUsedSlot = CMenuManagerStruct + 0x100;
                LoadSave_3vc = CMenuManagerStruct + 0xF8;
                _saveFileName = (int *)0x978428;
                StartNewGame = 7;
            }
            else
            {
                if (gvm.GetMajorVersion() == 1 && gvm.GetMinorVersion() == 1)
                {
                    loadingStage = (int *)0x9B5F10;
                    userdirPath = (int *)0x9750A4;
                    currentMenuItem = 12;
                    CMenuManagerStruct = 0x869638;
                    isInMenu = (char *)CMenuManagerStruct + 0x38;
                    LastUsedSlot = CMenuManagerStruct + 0x100;
                    LoadSave_3vc = CMenuManagerStruct + 0xF8;
                    _saveFileName = (int *)0x978430;
                    StartNewGame = 7;
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
                        _saveFileName = (int *)0x977430;
                        StartNewGame = 7;
                    }
                }
            }
        }
        else
        {
            if (gvm.IsSA())
            {
                if (gvm.GetMajorVersion() == 1 && gvm.GetMinorVersion() == 0)
                {
                    loadingStage = (int *)0xC8D4C0;
                    LoadSave = (char(__thiscall *)(int, char)) 0x573680;
                    userdirPath = (int *)0xC16F18;
                    currentMenuItem = 13;
                    CMenuManagerStruct = 0xBA6748;
                    isInMenu = (char *)CMenuManagerStruct + 0x5C;
                    LastUsedSlot = CMenuManagerStruct + 0x15F;
                    _saveFileName = (int *)0xC16DB8;
                    StartNewGame = 0xB7CB49;
                }
            }
        }
    }

    while (*loadingStage < 7 || !*isInMenu)
    {
        Sleep(20);
        if (GetAsyncKeyState(VK_SHIFT))
        {
            return 0;
        }
    }
    
    TCHAR SFPath[MAX_PATH];
    std::strcpy(SFPath, (char *)userdirPath);
    std::strcat(SFPath, "*.b");

    saveNum = iniReader.ReadInteger("MAIN", "LoadSlot", 0);

    if (!saveNum)
    {
        WIN32_FIND_DATA info;
        HANDLE file = FindFirstFile(SFPath, &info);

        if (file != INVALID_HANDLE_VALUE) {
            struct FileInfo {
                HANDLE h;
                WIN32_FIND_DATA info;
            } newest;

            newest.h = file;
            newest.info = info;

            while (FindNextFile(file, &info)) {
                if (CompareFileTime(&info.ftLastWriteTime, &newest.info.ftLastWriteTime) > 0) {
                    newest.h = file;
                    newest.info = info;
                }
            }

            DWORD dwFileSize = GetFileSize(newest.h, NULL);
            if (dwFileSize >= (int)200820)
            {
                std::strcpy(SFPath, newest.info.cFileName);
                saveNum = to_int(SFPath + 4);
            }
            FindClose(file);
        }
    }

    if (gvm.IsSA())
    {
        do
        {
            if (saveNum && saveNum != 129)
            {
                Sleep(100); //fastloader.asi compatibility
                WriteMemory(LastUsedSlot, saveNum - 1);
                LoadSave(CMenuManagerStruct, currentMenuItem);
            }
            else
            {
                Sleep(100);
                WriteMemory(CMenuManagerStruct + 0x5C, 0);
                WriteMemory(StartNewGame, 0);
            }
        } while (*loadingStage < 8);
    } 
    else
    {
        do
        {
            if (saveNum && saveNum != 129)
            {
                WriteMemory(LoadSave_3vc, currentMenuItem);
            }
            else
            {
                WriteMemory(LoadSave_3vc, StartNewGame);

                if (gvm.IsIII())
                {
                    WriteMemory(CMenuManagerStruct + (0x8F5AED - 0x8F59D8), 1);
                    WriteMemory(CMenuManagerStruct + (0x8F5AE9 - 0x8F59D8), 0);
                    WriteMemory(CMenuManagerStruct + (0x8F5AEC - 0x8F59D8), 1);
                    WriteMemory(CMenuManagerStruct + (0x8F5E2C - 0x8F59D8), 1);
                }
                else if (gvm.IsVC())
                {
                    WriteMemory(CMenuManagerStruct + (0x869641 - 0x869630), 1);
                    WriteMemory(CMenuManagerStruct + (0x869668 - 0x869630), 0);
                    WriteMemory(CMenuManagerStruct + (0x869669 - 0x869630), 1);
                    WriteMemory(CMenuManagerStruct + (0x86966C - 0x869630), 1);
                }
            }
        } while (*loadingStage < 8);
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
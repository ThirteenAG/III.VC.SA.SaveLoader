#include "stdafx.h"
#include "CPatch.h"
#include <stdio.h>
#include <string>
#include <windows.h>
using namespace std;

#define gGameVersion (*(unsigned int *)0x601048)
#define GTA_3_1_0     0x3A83126F
#define GTA_3_1_1     0x3F8CCCCD
#define GTA_3_STEAM     0x47BDA5
#define GTA_VC_1_0     0x53FF1B8B
#define GTA_VC_STEAM     0xF04F883
#define GTA_VC_1_1     0x783EE8
#define GTA_SA_1_0     0xCE8B168B
#define GTA_SA_1_1     0x0
#define GTA_SA_STEAM     0x1

HANDLE HndThread;

int saveNum;
int to_int(char const *s);
int currentMenuItem;

int *funcGameVersion;
int *loadingStage;
int *userdirPath;
char *isInMenu;
int LastUsedSlot;
int CMenuManagerStruct;
int LoadSave_3vc;
char (__thiscall *LoadSave)(int, char);

DWORD WINAPI Thread(LPVOID param)
{
		switch(gGameVersion)
		{
			case GTA_3_1_0:
			loadingStage = (int *) 0x8F5838;
			userdirPath = (int *) 0x8E28C0;
			currentMenuItem = 14;
			CMenuManagerStruct = 0x8F59D8;
			isInMenu = (char *) CMenuManagerStruct+0x457;
			LastUsedSlot = CMenuManagerStruct+0x55C;
			LoadSave_3vc = CMenuManagerStruct+0x548;
			break;

			case GTA_3_1_1:
			loadingStage = (int *) 0x8F58EC;
			userdirPath = (int *) 0x8E2870;
			currentMenuItem = 14;
			CMenuManagerStruct = 0x8F5A8C;
			isInMenu = (char *) CMenuManagerStruct+0x457;
			LastUsedSlot = CMenuManagerStruct+0x55C;
			LoadSave_3vc = CMenuManagerStruct+0x548;
			break;

			case GTA_3_STEAM:
			loadingStage = (int *) 0x905A2C;
			userdirPath = (int *) 0x8F29B0;
			currentMenuItem = 14;
			CMenuManagerStruct = 0x905BCC;
			isInMenu = (char *) CMenuManagerStruct+0x457;
			LastUsedSlot = CMenuManagerStruct+0x55C;
			LoadSave_3vc = CMenuManagerStruct+0x548;
			break;

			case GTA_VC_1_0:
			loadingStage = (int *) 0x9B5F08;
			userdirPath = (int *) 0x97509C;
			currentMenuItem = 12;
			CMenuManagerStruct = 0x869630;
			isInMenu = (char *) CMenuManagerStruct+0x38;
			LastUsedSlot = CMenuManagerStruct+0x100;
			LoadSave_3vc = CMenuManagerStruct+0xF8;
			break;

			case GTA_VC_1_1:
			loadingStage = (int *) 0x9B5F10;
			userdirPath = (int *) 0x9750A4;
			currentMenuItem = 12;
			CMenuManagerStruct = 0x869638;
			isInMenu = (char *) CMenuManagerStruct+0x38;
			LastUsedSlot = CMenuManagerStruct+0x100;
			LoadSave_3vc = CMenuManagerStruct+0xF8;
			break;

			case GTA_VC_STEAM:
			loadingStage = (int *) 0x9B4F10;
			userdirPath = (int *) 0x9740A4;
			currentMenuItem = 12;
			CMenuManagerStruct = 0x868638;
			isInMenu = (char *) CMenuManagerStruct+0x38;
			LastUsedSlot = CMenuManagerStruct+0x100;
			LoadSave_3vc = CMenuManagerStruct+0xF8;
			break;	

			case GTA_SA_1_0:
			loadingStage = (int *) 0xC8D4C0;
			LoadSave = (char (__thiscall *)(int, char)) 0x573680;
			userdirPath = (int *) 0xC16F18;
			currentMenuItem = 13;
			CMenuManagerStruct = 0xBA6748;
			isInMenu = (char *) CMenuManagerStruct+0x5C;
			LastUsedSlot = CMenuManagerStruct+0x15F;
			break;	

			case GTA_SA_1_1:
				return 0;
			break;

			case GTA_SA_STEAM:
				return 0;
			break;
		}



while (*loadingStage < 7 || !*isInMenu)
{
Sleep(0);
		if(GetAsyncKeyState(VK_SHIFT))
		{
		return 0;
		}
}


TCHAR SFPath[MAX_PATH + 1];
strcpy(SFPath, (char *)userdirPath);
strcat(SFPath, "*.b");
//MessageBox(0, SFPath, "test", 1);

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

		strcpy(SFPath, newest.info.cFileName);
		saveNum = to_int(SFPath+4); // function parses GTA3sf string and returns wrong result.
        FindClose(file);
    }

		if (saveNum) 
		{
			do {
			CPatch::SetChar(LastUsedSlot, saveNum-1);

				if(LoadSave) 
				{
					Sleep(100);
				LoadSave(CMenuManagerStruct, currentMenuItem);
				} else {
				CPatch::SetChar(LoadSave_3vc, currentMenuItem);
				}

			   } 
			while (*loadingStage < 8);
		}

	return 0;
}




BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if(reason==DLL_PROCESS_ATTACH)
    {
		HndThread = CreateThread(0,0,(LPTHREAD_START_ROUTINE)&Thread,NULL,0,NULL);
    }
    return TRUE;
}


int to_int(char const *s)
{
     bool negate = (s[0] == '-');
     if ( *s == '+' || *s == '-' ) 
          ++s;
     int result = 0;
     while(*s)
     {
          if ( *s >= '0' && *s <= '9' )
          {
              result = result * 10  - (*s - '0');  //assume negative number
          }
          ++s;
     }
     return negate ? result : -result; //-result is positive!
}
#include "stdafx.h"
#include "CPatch.h"
#include <stdio.h>
#include <string>
#include "snip_str.h"
#include <tchar.h>
#include <Windows.h>
#include <WinInet.h>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include "CIniReader\IniReader.h"
using namespace std;
#pragma comment(lib,"wininet.lib")

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
int StartNewGame;
int *_saveFileName;
int LastUsedSlot;
int CMenuManagerStruct;
int LoadSave_3vc;
char (__thiscall *LoadSave)(int, char);
int SnP_Upload(int);
int SnP_Download(int);

char *LatestSavefromSnP;
int SnP_enabled;

void GenerateKey(int vk, BOOL bExtended);
void GenerateKey_down(int vk, BOOL bExtended);
void GenerateKey_up(int vk, BOOL bExtended);

char *latest_uploaded;

DWORD WINAPI Thread(LPVOID param)
{

  CIniReader iniReader("SaveLoader_gta3vcsa.ini");
		if((!iniReader.ReadInteger("MAIN", "Enable", 0))) {
			return 0;
		}
	
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
			_saveFileName = (int *) 0x8E2CBC;
			StartNewGame = 10;
			break;

			case GTA_3_1_1:
			loadingStage = (int *) 0x8F58EC;
			userdirPath = (int *) 0x8E2870;
			CMenuManagerStruct = 0x8F5A8C;
			isInMenu = (char *) CMenuManagerStruct+0x457;
			LastUsedSlot = CMenuManagerStruct+0x55C;
			LoadSave_3vc = CMenuManagerStruct+0x548;
			_saveFileName = (int *) 0x8E2D70;
			StartNewGame = 10;
			break;

			case GTA_3_STEAM:
			loadingStage = (int *) 0x905A2C;
			userdirPath = (int *) 0x8F29B0;
			currentMenuItem = 14;
			CMenuManagerStruct = 0x905BCC;
			isInMenu = (char *) CMenuManagerStruct+0x457;
			LastUsedSlot = CMenuManagerStruct+0x55C;
			LoadSave_3vc = CMenuManagerStruct+0x548;
			_saveFileName = (int *) 0x8F2EB0;
			StartNewGame = 10;
			break;

			case GTA_VC_1_0:
			loadingStage = (int *) 0x9B5F08;
			userdirPath = (int *) 0x97509C;
			currentMenuItem = 12;
			CMenuManagerStruct = 0x869630;
			isInMenu = (char *) CMenuManagerStruct+0x38;
			LastUsedSlot = CMenuManagerStruct+0x100;
			LoadSave_3vc = CMenuManagerStruct+0xF8;
			_saveFileName = (int *) 0x978428;
			StartNewGame = 7;
			break;

			case GTA_VC_1_1:
			loadingStage = (int *) 0x9B5F10;
			userdirPath = (int *) 0x9750A4;
			currentMenuItem = 12;
			CMenuManagerStruct = 0x869638;
			isInMenu = (char *) CMenuManagerStruct+0x38;
			LastUsedSlot = CMenuManagerStruct+0x100;
			LoadSave_3vc = CMenuManagerStruct+0xF8;
			_saveFileName = (int *) 0x978430;
			StartNewGame = 7;
			break;

			case GTA_VC_STEAM:
			loadingStage = (int *) 0x9B4F10;
			userdirPath = (int *) 0x9740A4;
			currentMenuItem = 12;
			CMenuManagerStruct = 0x868638;
			isInMenu = (char *) CMenuManagerStruct+0x38;
			LastUsedSlot = CMenuManagerStruct+0x100;
			LoadSave_3vc = CMenuManagerStruct+0xF8;
			_saveFileName = (int *) 0x977430;
			StartNewGame = 7;
			break;	

			case GTA_SA_1_0:
			loadingStage = (int *) 0xC8D4C0;
			LoadSave = (char (__thiscall *)(int, char)) 0x573680;
			userdirPath = (int *) 0xC16F18;
			currentMenuItem = 13;
			CMenuManagerStruct = 0xBA6748;
			isInMenu = (char *) CMenuManagerStruct+0x5C;
			LastUsedSlot = CMenuManagerStruct+0x15F;
			_saveFileName = (int *) 0xC16DB8;
			StartNewGame = 0xB7CB49;
			break;	

			case GTA_SA_1_1:
				return 0;
			break;

			case GTA_SA_STEAM:
				return 0;
			break;
		}

SnP_enabled = iniReader.ReadInteger("SaveNPlay", "Enable", 0);
LatestSavefromSnP = iniReader.ReadString("SaveNPlay", "Latest", "");


while (*loadingStage < 7 || !*isInMenu)
{
Sleep(20);
		if(GetAsyncKeyState(VK_SHIFT))
		{
		return 0;
		}
/*		if (SnP_enabled && LatestSavefromSnP && GetAsyncKeyState(VK_F9))
			{
				char* tempPointer3;
				char tempbuf2[255];
				tempPointer3 = strrchr(LatestSavefromSnP, '/');
				memcpy(tempbuf2, tempPointer3, strlen(tempPointer3));
				memset(LatestSavefromSnP, 0x00, strlen(LatestSavefromSnP));
				strcat(LatestSavefromSnP, "file");
				strcat(LatestSavefromSnP, tempbuf2);
				//MessageBox(0, LatestSavefromSnP, "test", 1);
				SnP_Download(8);
			}*/
}


TCHAR SFPath[MAX_PATH + 1];
strcpy(SFPath, (char *)userdirPath);
strcat(SFPath, "*.b");
//MessageBox(0, SFPath, "test", 1);

//SnP_Upload(8);


	saveNum = iniReader.ReadInteger("MAIN", "LoadSlot", 0);
	if(!saveNum) 
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
		if (dwFileSize >= (int)200820)  //broken!!
		{
			//MessageBox(0, "2", "test", 1);
			strcpy(SFPath, newest.info.cFileName); 
			saveNum = to_int(SFPath + 4); // function parses GTA3sf string and returns wrong result.
		}
		FindClose(file);
    }
	}

		if (saveNum && saveNum != 129)  //loading save
		{
			int timerA;
			do {
			CPatch::SetChar(LastUsedSlot, saveNum-1);
				if(LoadSave) {
				Sleep(100);
				CPatch::SetChar(LastUsedSlot, saveNum-1);
				LoadSave(CMenuManagerStruct, currentMenuItem);
				} else {
				CPatch::SetChar(LoadSave_3vc, currentMenuItem);
				}
				++timerA;
				if (timerA > 500)
					break;
			   }
			while (*loadingStage < 8);
		}

	if(saveNum == 129) 
	{
		Sleep(20);
		CPatch::SetChar(LoadSave_3vc, StartNewGame);
		Sleep(20);
		GenerateKey_down(VK_DOWN, TRUE);
		Sleep(20);
		GenerateKey_down(VK_RETURN, TRUE);
		Sleep(20);
		GenerateKey_up(VK_DOWN, TRUE);
		GenerateKey_up(VK_RETURN, TRUE);
	}

// SnP upload

		while (true) 
		{
			Sleep(1);

				if(strpbrk((char *)_saveFileName, "sf8.b") != NULL) {
					Sleep (111);
					
					CPatch::FillWithZeroes((int)_saveFileName, 25);
					SnP_Upload(8);
				}

				if (SnP_enabled && LatestSavefromSnP && GetAsyncKeyState(VK_F9))
				{
					char* tempPointer3;
					char tempbuf2[255];
					tempPointer3 = strrchr(LatestSavefromSnP, '/');
					memcpy(tempbuf2, tempPointer3, strlen(tempPointer3));
					memset(LatestSavefromSnP, 0x00, strlen(LatestSavefromSnP));
					strcat(LatestSavefromSnP, "file");
					strcat(LatestSavefromSnP, tempbuf2);
					//MessageBox(0, LatestSavefromSnP, "test", 1);
					SnP_Download(8);

					int timerB;
					do {
						CPatch::SetChar(LastUsedSlot, saveNum - 1);
						if (LoadSave) {
							Sleep(100);
							CPatch::SetChar(LastUsedSlot, saveNum - 1);
							LoadSave(CMenuManagerStruct, currentMenuItem);
						}
						else {
							CPatch::SetChar(LoadSave_3vc, currentMenuItem);
						}
						++timerB;
						if (timerB > 500)
							break;
					} while (*loadingStage < 8);

				}
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


int SnP_Upload(int slotNumber)
{
    HINTERNET hInet = ::InternetOpen(
          _T("My User Agent")
        , INTERNET_OPEN_TYPE_PRECONFIG // get proxy from IE settings
        , NULL
        , NULL
        , 0);

    HINTERNET hConn = ::InternetConnect(
          hInet
        , _T("gtasnp.com")
        , INTERNET_DEFAULT_HTTP_PORT
        , NULL
        , NULL
        , INTERNET_SERVICE_HTTP
        , 0
        , NULL
        );

    LPCTSTR accept[] = {_T("text/html"), NULL};
    HINTERNET hReq = ::HttpOpenRequest(
          hConn
        , _T("POST")
        , _T("/upload")
        , NULL
        , _T("http://gtasnp.com/") // referrer
        , accept
        ,   INTERNET_FLAG_NO_CACHE_WRITE |
            INTERNET_FLAG_NO_COOKIES |
            INTERNET_FLAG_NO_UI |
            INTERNET_FLAG_PRAGMA_NOCACHE |
            INTERNET_FLAG_RELOAD
        , NULL
        );

    #define BOUNDARY "--Boundary"
	
	TCHAR FILE_PATH[MAX_PATH + 1];
	TCHAR HTML_PATH[MAX_PATH + 1];
	_tcscpy(FILE_PATH, (char *)userdirPath);
	_stprintf(FILE_PATH, "%s%d", FILE_PATH, slotNumber);
	_tcscpy(HTML_PATH, FILE_PATH);
	_tcscat(HTML_PATH, "-gtaSnP.html");
	_tcscat(FILE_PATH, ".b");



	TCHAR *FILE_NAME;
	FILE_NAME = _tcsrchr(FILE_PATH,'\\');

	//MessageBox(0, FILE_PATH, "test", 1);

    std::ostringstream contentStrm(std::ios_base::out | std::ios_base::binary);
    contentStrm
        << "--"BOUNDARY"\r\n"
        << "Content-Disposition: form-data; name=\"file\"; filename=\""
		<< FILE_NAME+1
		<< "\"\r\n"
        << "Content-Type: application/octet-stream\r\n\r\n";

    contentStrm << std::ifstream(FILE_PATH, std::ios_base::in | std::ios_base::binary | std::ios_base::app).rdbuf();

    contentStrm << "\r\n--"BOUNDARY"--\r\n";
    std::string content = contentStrm.str();
    contentStrm.str(std::string()); // clear memory inside ostringstream

    const TCHAR headers[] = _T("Content-Type:multipart/form-data; boundary=")_T(BOUNDARY);
    BOOL res = ::HttpSendRequest(
          hReq
        , headers
        , ARRAYSIZE(headers)
        , const_cast<char*>(content.c_str())
        , content.size()
        );

    std::vector<char> buf;
    std::ofstream responseFile(HTML_PATH, std::ios_base::app);
	
    DWORD bytesAvail = 0;
    while(true)
    {
        res = ::InternetQueryDataAvailable(
              hReq
            , &bytesAvail
            , 0
            , 0
            );
        if (!res || 0 == bytesAvail)
        {
            break;
        }

        buf.resize(bytesAvail);

        res = ::InternetReadFile(
              hReq
            , &buf[0]
            , buf.size()
            , &bytesAvail
            );
		
        buf.resize(bytesAvail);

		latest_uploaded = stristr(&buf[0], "URL:");
		if (latest_uploaded != NULL) 
		{
			char latest_buf[50];
			char* tempPointer2;
			strncpy(latest_buf, latest_uploaded+60, 40);
			tempPointer2 = strrchr(latest_buf, '"');
			*tempPointer2 = '\0';
			tempPointer2 = strchr(latest_buf, '"');
			//MessageBox(0, tempPointer2 + 1, "2", 0);
			CIniWriter iniWriter("SaveLoader_gta3vcsa.ini");
			iniWriter.WriteString("SaveNPlay", "Latest", tempPointer2 + 1);
		}



        std::copy(buf.begin(), buf.end(), std::ostreambuf_iterator<char>(responseFile));
    }

    responseFile.close();
       
    ::InternetCloseHandle(hReq);
    ::InternetCloseHandle(hConn);
    ::InternetCloseHandle(hInet);

    return 0;
}


int SnP_Download(int slotNumber)
{
    HINTERNET hInet = ::InternetOpen(
          _T("My User Agent")
        , INTERNET_OPEN_TYPE_PRECONFIG // get proxy from IE settings
        , NULL
        , NULL
        , 0);

    HINTERNET hConn = ::InternetConnect(
          hInet
        , _T("gtasnp.com")
        , INTERNET_DEFAULT_HTTP_PORT
        , NULL
        , NULL
        , INTERNET_SERVICE_HTTP
        , 0
        , NULL
        );
	
    LPCTSTR accept[] = {_T("text/html"), NULL};
    HINTERNET hReq = ::HttpOpenRequest(
          hConn
        , _T("GET")
        , _T(LatestSavefromSnP)
        , NULL
        , _T("http://gtasnp.com/") // referrer
        , accept
        ,   INTERNET_FLAG_NO_CACHE_WRITE |
            INTERNET_FLAG_NO_COOKIES |
            INTERNET_FLAG_NO_UI |
            INTERNET_FLAG_PRAGMA_NOCACHE |
            INTERNET_FLAG_RELOAD
        , NULL
        );

    #define BOUNDARY "--Boundary"
	
	TCHAR FILE_PATH[MAX_PATH + 1];
	_tcscpy(FILE_PATH, (char *)userdirPath);
	_stprintf(FILE_PATH, "%s%d", FILE_PATH, slotNumber);
	_tcscat(FILE_PATH, ".b");

	TCHAR *FILE_NAME;
	FILE_NAME = _tcsrchr(FILE_PATH,'\\');

	//MessageBox(0, FILE_NAME+1, "test", 1);
	std::remove(FILE_PATH);

    std::ostringstream contentStrm(std::ios_base::out | std::ios_base::binary);
    contentStrm
        << "--"BOUNDARY"\r\n"
        << "Content-Disposition: attachment; filename=\""
		<< FILE_NAME+1
		<< "\"\r\n"
        << "Content-Type: application/octet-stream\r\n\r\n";

    contentStrm << std::ifstream(FILE_PATH, std::ios_base::in | std::ios_base::binary).rdbuf();

    contentStrm << "\r\n--"BOUNDARY"--\r\n";
    std::string content = contentStrm.str();
    contentStrm.str(std::string()); // clear memory inside ostringstream

    const TCHAR headers[] = _T("Content-Type:application/octet-stream; boundary=")_T(BOUNDARY);
    BOOL res = ::HttpSendRequest(
          hReq
        , headers
        , ARRAYSIZE(headers)
        , const_cast<char*>(content.c_str())
        , content.size()
        );

    std::vector<char> buf;
    std::ofstream responseFile(FILE_PATH, std::ofstream::app | std::ofstream::binary);
	

    DWORD bytesAvail = 0;
    while(true)
    {
        res = ::InternetQueryDataAvailable(
              hReq
            , &bytesAvail
            , 0
            , 0
            );
        if (!res || 0 == bytesAvail)
        {
            break;
        }

        buf.resize(bytesAvail);

        res = ::InternetReadFile(
              hReq
            , &buf[0]
            , buf.size()
            , &bytesAvail
            );

        buf.resize(bytesAvail);

        std::copy(buf.begin(), buf.end(), std::ostreambuf_iterator<char>(responseFile));
    }
	Sleep(100);
    responseFile.close();
        

    ::InternetCloseHandle(hReq);
    ::InternetCloseHandle(hConn);
    ::InternetCloseHandle(hInet);

    return 0;
}
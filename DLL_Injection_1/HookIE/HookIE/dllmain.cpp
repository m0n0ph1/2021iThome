#include "pch.h"
#include <Windows.h>
#include "MinHook.h"
#include <WinInet.h>
#include <fstream>
#include <stdio.h>

using namespace std;

#pragma comment(lib, "libMinHook.x86.lib")

// HttpSendRequestW ����ƭ쫬�A�i�H�q MSDN �d��
typedef BOOL (WINAPI* HTTPSENDREQUESTW)(HINTERNET, LPCWSTR, DWORD, LPVOID, DWORD);
HTTPSENDREQUESTW fpHttpSendRequestW = NULL;

// �N��T�g�J debug.txt �ɮפ�
void DebugLog(wstring str) {
    wofstream file;
    file.open("C:\\debug.txt", std::wofstream::out | std::wofstream::app);
    if (file) file << str << endl << endl;
    file.close();
}

// «�� DetourHttpSendRequestW �����
BOOL WINAPI DetourHttpSendRequestW(HINTERNET hRequest,
    LPCWSTR   lpszHeaders,
    DWORD     dwHeadersLength,
    LPVOID    lpOptional,
    DWORD     dwOptionalLength) {
    // �ĤG�ӰѼƬO Request Header�A�⥦�g�J�ɮפ�
    if (lpszHeaders) {
        DebugLog(lpszHeaders);
    }

    // �ĥ|�ӰѼƳq�`�O POST�BPUT ���ѼơA�⥦�g�J�ɮפ�
    if (lpOptional) {
        wchar_t  ws[1000];
        swprintf(ws, 1000, L"%hs", (char *)lpOptional);
        DebugLog(ws);
    }

    // �I�s�쥻�� HttpSendRequestW
    return fpHttpSendRequestW(hRequest, lpszHeaders, dwHeadersLength, lpOptional, dwOptionalLength);
}

int hook() {
    wchar_t log[256];

    // 1. ���o wininet.dll �� Handle
    HINSTANCE hDLL = LoadLibrary(L"wininet.dll");
    if (!hDLL) {
        DebugLog(L"LoadLibrary wininet.dll failed\n");
        return 1;
    }

    // 2. �q wininet.dll ��X HttpSendRequestW ����}
    void* HttpSendRequestW = (void*)GetProcAddress(hDLL, "HttpSendRequestW");
    if (!HttpSendRequestW) {
        DebugLog(L"GetProcAddress HttpSendRequestW failed\n");
        return 1;
    }

    // 3. Hook HttpSendRequestW�A�N��«�令�ڭ̦ۤv�w�q����ơC
    //    �w�q����Ƥ��A�|��ĤG�ӰѼƻP�ĥ|�ӰѼƼg���ɮפ��A�M��I�s�쥻�� HtppSendRequestW
    if (MH_Initialize() != MH_OK){
        DebugLog(L"MH_Initialize failed\n");
        return 1;
    }
    int status = MH_CreateHook(HttpSendRequestW, &DetourHttpSendRequestW, reinterpret_cast<LPVOID*>(&fpHttpSendRequestW));
    if (status != MH_OK) {
        swprintf_s(log, L"MH_CreateHook failed: Error %d\n", status);
        DebugLog(log);
        return 1;
    }

    // 4. �ҥ� Hook
    status = MH_EnableHook(HttpSendRequestW);
    if (status != MH_OK) {
        swprintf_s(log, L"MH_EnableHook failed: Error %d\n", status);
        DebugLog(log);
        return 1;
    }

    return 0;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     ) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH: {
            hook();
        }
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}
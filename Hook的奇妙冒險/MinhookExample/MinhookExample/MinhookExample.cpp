#include <Windows.h>
#include "MinHook.h"

#if defined _M_X64
#pragma comment(lib, "libMinHook.x64.lib")
#elif defined _M_IX86
#pragma comment(lib, "libMinHook.x86.lib")
#endif

// 1. �w�q�n Hook ����ơA�]�������n�⥦�令�ۤv����ơA�ҥH�̦n�Ѽƥi�H�����쥻����ơC
typedef int (WINAPI* MESSAGEBOXW)(HWND, LPCWSTR, LPCWSTR, UINT);
MESSAGEBOXW fpMessageBoxW = NULL;

int WINAPI DetourMessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType) {
	return fpMessageBoxW(hWnd, L"Hooked\n", lpCaption, uType);
}

int main() {
	// 2. �N�ؼШ�ƪ���T�O�U�ӡA���n Hook ���e�m�@�~�C
	if (MH_Initialize() != MH_OK) {
		return 1;
	}
	if (MH_CreateHook(&MessageBoxW, &DetourMessageBoxW, reinterpret_cast<LPVOID*>(&fpMessageBoxW)) != MH_OK) {
		return 1;
	}

	// 3. �ҥ� Hook�A�N�ؼШ�ƫe�X Byte �ﱼ�Ajmp ��ۤv�w�q����ơC
	if (MH_EnableHook(&MessageBoxW) != MH_OK)
	{
		return 1;
	}

	// ���եؼШ�ƳQ�ﱼ�ᦳ�S���ܦ��ۤv�w�q����ơA�o�����ӭn���X Hooked
	MessageBoxW(NULL, L"Not hooked\n", L"MinHook Example", MB_OK);

	// 4. ���� Hook�A��ؼШ�Ƨ�^��
	if (MH_DisableHook(&MessageBoxW) != MH_OK) {
		return 1;
	}

	// ���եؼШ�Ʀ��S���Q��^�ӡA�o�����ӭn���X Not hooked
	MessageBoxW(NULL, L"Not hooked\n", L"MinHook Example", MB_OK);

	if (MH_Uninitialize() != MH_OK) {
		return 1;
	}
	return 0;
}
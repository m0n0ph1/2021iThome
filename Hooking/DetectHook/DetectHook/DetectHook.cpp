#include <windows.h>
#include <string>
#include <psapi.h>

int main(int argc, char* argv[]) {
    // �}�ҥؼ� Process (iexplore.exe �� pid)
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, 13952);
    if (!hProcess) {
        printf("OpenProcess failed: error %d\n", GetLastError());
        return 1;
    }

    // �� EnumProcessModules ���o�Ҧ��� Module Handle
    HMODULE hMods[1024], hModule = NULL;
    DWORD cbNeeded;
    if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)) {
        for (int i = 0; i < (int)(cbNeeded / sizeof(HMODULE)); i++) {
            TCHAR szModPathName[MAX_PATH] = { 0 };

            // �� GetModuleFileNameEx ���o�ثe�� Module Name
            if (GetModuleFileNameEx(hProcess, hMods[i], szModPathName, sizeof(szModPathName) / sizeof(TCHAR))) {
                // �P�_�O���O�ؼ� (WININET)�A�O���ܴN�O���U��
                std::wstring sMod = szModPathName;
                if (sMod.find(L"WININET") != std::string::npos) {
                    hModule = hMods[i];
                }
            }
            else {
                printf("GetModuleFileNameEx failed: error %d\n", GetLastError());
                return NULL;
            }
        }
    }
    else {
        printf("EnumProcessModulesEx failed: error %d\n", GetLastError());
        return 1;
    }
    if (hModule == NULL) {
        printf("Cannot find target module\n");
        return 1;
    }

    // �� GetModuleInformation ���o Module ��T
    MODULEINFO lpmodinfo;
    if (!GetModuleInformation(hProcess, hModule, &lpmodinfo, sizeof(MODULEINFO))) {
        printf("GetModuleInformation failed: error %d\n", GetLastError());
        return 1;
    }
    /* �H�W�O���i�P�ǧQ (�W) �����e */

    /* �H�U�O���i�P�ǧQ (�U) �����e */
    // 1. �� wininet.dll �ɮ� Map ��ثe�� Process ��
    // ���o�ɮת� Handle
    HANDLE hFile = CreateFile(L"C:\\Windows\\SysWOW64\\wininet.dll", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        printf("CreateFile failed: error %d\n", GetLastError());
        return NULL;
    }

    // �إ� Mapping ����
    HANDLE file_map = CreateFileMapping(hFile, NULL, PAGE_READONLY | SEC_IMAGE, 0, 0, L"KernelMap");
    if (!file_map) {
        printf("CreateFileMapping failed: error %d\n", GetLastError());
        return NULL;
    }

    // ���ɮפ��e�����e�� Process ��
    LPVOID file_image = MapViewOfFile(file_map, FILE_MAP_READ, 0, 0, 0);
    if (file_image == 0) {
        printf("MapViewOfFile failed: error %d\n", GetLastError());
        return NULL;
    }

    // 2. Ū�� wininet.dll �ɮת� PE ���c�A���o HttpSendRequestW �� RVA
    DWORD RVA = 0;

    // ���o IMAGE_DOS_HEADER ���c��A���ۤ@�����L Header�A�����X IMAGE_EXPORT_DIRECTORY
    PIMAGE_DOS_HEADER pDos_hdr = (PIMAGE_DOS_HEADER)file_image;
    PIMAGE_NT_HEADERS pNt_hdr = (PIMAGE_NT_HEADERS)((char*)file_image + pDos_hdr->e_lfanew);
    IMAGE_OPTIONAL_HEADER opt_hdr = pNt_hdr->OptionalHeader;
    IMAGE_DATA_DIRECTORY exp_entry = opt_hdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    PIMAGE_EXPORT_DIRECTORY pExp_dir = (PIMAGE_EXPORT_DIRECTORY)((char*)file_image + exp_entry.VirtualAddress);

    // �q IMAGE_EXPORT_DIRECTORY ��X Function Table�BOrdinal Table�BName Table
    DWORD* func_table = (DWORD*)((char*)file_image + pExp_dir->AddressOfFunctions);
    WORD* ord_table = (WORD*)((char*)file_image + pExp_dir->AddressOfNameOrdinals);
    DWORD* name_table = (DWORD*)((char*)file_image + pExp_dir->AddressOfNames);

    // �� Name Table �j���� HttpSendRequestW�A����z�L Function Table�BOrdinal Table ���o RVA
    for (int i = 0; i < (int)pExp_dir->NumberOfNames; i++) {
        if (strcmp("HttpSendRequestW", (const char*)file_image + (DWORD)name_table[i]) == 0) {
            RVA = (DWORD)func_table[ord_table[i]];
        }
    }
    if (!RVA) {
        printf("Failed to find target function\n");
    }

    // 3. ����ɮ׻P iexplore.exe �� HttpSendRequestW �O�_�ۦP
    // �� ReadProcessMemory Ū�� iexplore.exe �� HttpSendRequestW ��ƪ��e 5 Bytes
    TCHAR* lpBuffer = new TCHAR[6]{ 0 };
    if (!ReadProcessMemory(hProcess, (LPCVOID)((DWORD)lpmodinfo.lpBaseOfDll + RVA), lpBuffer, 5, NULL)) {
        printf("ReadProcessMemory failed: error %d\n", GetLastError());
        return -1;
    }

    // �� memcmp ����ɮשM�O���骺 HttpSendRequestW �@���@��
    if (memcmp((LPVOID)((DWORD)file_image + RVA), (LPVOID)((DWORD)lpBuffer), 5) == 0) {
        printf("Not Hook\n");
        return 0;
    }
    else {
        printf("Hook\n");
        return 1;
    }

    return 0;
}
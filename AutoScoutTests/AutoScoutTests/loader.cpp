#include <Windows.h>

#include "loader.h"

// Parse reloc table
void ldr_reloc(LPVOID pModule, PIMAGE_NT_HEADERS pImageNtHeader)
{
	DWORD dwRelocSize = pImageNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
	DWORD dwRelocAddr = pImageNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;

	if (dwRelocAddr == 0 || dwRelocSize == 0)
		return;	// no reloc table here!

	LPBYTE lpPtr = CALC_OFFSET(LPBYTE, pModule, dwRelocAddr);

	//std::cout << "sizeof(base_relocation_block): " << sizeof(base_relocation_block) << std::endl;
	//std::cout << "sizeof(base_relocation_entry): " << sizeof(base_relocation_entry) << std::endl;

	while(dwRelocSize > 0)
	{
		base_relocation_block_t block;

		memcpy(&block, lpPtr, sizeof(base_relocation_block_t));

		dwRelocSize -= block.BlockSize;

		lpPtr += sizeof(base_relocation_block_t);

		//std::cout << "Block: " << std::hex << block.PageRVA << std::endl;
		//std::cout << " Size: " << std::hex << block.BlockSize << std::endl;

		block.BlockSize -= 8;

		while(block.BlockSize)
		{
			base_relocation_entry_t entry;

			memcpy(&entry, lpPtr, sizeof(WORD));
						
			//__asm mov ecx, entry.offset;
			LPDWORD ptrOffset = CALC_OFFSET(LPDWORD, pModule, block.PageRVA + entry.offset);
			DWORD dwOldValue = *ptrOffset;
			
			DWORD dwNewValue = dwOldValue -
				pImageNtHeader->OptionalHeader.ImageBase +
				(DWORD) pModule;

			LPWORD ptrHighOffset = CALC_OFFSET_DISP(LPWORD, pModule, block.PageRVA + entry.offset, 2);
			LPWORD ptrLowOffset = CALC_OFFSET_DISP(LPWORD, pModule, block.PageRVA + entry.offset, 0);

			WORD wLowNewOffset = (WORD) ((DWORD) pModule & 0xffff);
			WORD wHighNewOffset = (WORD) (((DWORD) pModule & 0xffff0000) >> 16);

			switch(entry.type)
			{
// The base relocation is skipped. This type can be used to pad a block.
				case IMAGE_REL_BASED_ABSOLUTE:
						//std::cout << "Unsupported" << std::endl;
					break;
// The base relocation adds the high 16 bits of the difference to the 16-bit field at offset. The 16-bit field represents the high value of a 32-bit word.
				case IMAGE_REL_BASED_HIGH: 
						//*ptrHighOffset = *ptrHighOffset - wHighNewOffset;
					break;
// The base relocation adds the low 16 bits of the difference to the 16-bit field at offset. The 16-bit field represents the low half of a 32-bit word. 
				case IMAGE_REL_BASED_LOW:
					break;
// The base relocation applies all 32 bits of the difference to the 32-bit field at offset
				case IMAGE_REL_BASED_HIGHLOW:
						*ptrOffset = dwNewValue;
					break;
// The base relocation adds the high 16 bits of the difference to the 16bit field at offset. The 16-bit field represents the high value of a 32-bit word.
// The low 16 bits of the 32-bit value are stored in the 16-bit word that follows this base relocation. This means that this base relocation occupies two slots.
				case IMAGE_REL_BASED_HIGHADJ:
					break;
// The base relocation applies the difference to the 64-bit field at offset.
				case IMAGE_REL_BASED_DIR64:
					break;
			}

			// FIX ENTRY++

			lpPtr += sizeof(base_relocation_entry);
			block.BlockSize -= 2;
		}
	}
}

typedef HMODULE (WINAPI *LoadLibraryA_p)(LPCSTR strDllName);

void ldr_importdir(LPVOID pModule, PIMAGE_NT_HEADERS pImageNtHeader)
{
	DWORD dwIatSize = pImageNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size;
	DWORD dwIatAddr = pImageNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
	
	// no import directory here!
	if (dwIatAddr == 0)
		return;

	PIMAGE_IMPORT_DESCRIPTOR pImportDescriptor = CALC_OFFSET(PIMAGE_IMPORT_DESCRIPTOR, pModule, dwIatAddr);
	
	while(pImportDescriptor)
	{
		if (pImportDescriptor->FirstThunk == 0)
		{
			pImportDescriptor = NULL;
			continue;
		}
		
		LPDWORD pImportLookupTable = CALC_OFFSET(LPDWORD, pModule, pImportDescriptor->FirstThunk);
		LPCSTR lpModName = CALC_OFFSET(LPCSTR, pModule, pImportDescriptor->Name);

		LoadLibraryA_p fpLoadLibraryA = (LoadLibraryA_p) GetProcAddress(LoadLibrary(L"kernel32"), "LoadLibraryA");
		HMODULE hMod = fpLoadLibraryA(lpModName);
		//HMODULE hMod = LoadLibraryA(lpModName);
		
		if (hMod != NULL)
			while(*pImportLookupTable != 0x00)
			{
				if ((*pImportLookupTable & IMAGE_ORDINAL_FLAG) != 0x00)
				{	// IMPORT BY ORDINAL
					DWORD pOrdinalValue = *(CALC_OFFSET(LPDWORD, pImportLookupTable, 0)) & 0x0000ffff;
					*pImportLookupTable = (DWORD) GetProcAddress(hMod, (LPCSTR) pOrdinalValue);
					// SOSTITUISCE EXITPROCESS CON EXITTHREAD 
					if (*pImportLookupTable == (DWORD)GetProcAddress(fpLoadLibraryA("kernel32"), "ExitProcess"))
						*pImportLookupTable = (DWORD)ExitThread;
				}
				else
				{	// IMPORT BY NAME
					LPCSTR lpProcName = CALC_OFFSET_DISP(LPCSTR, pModule, (*pImportLookupTable), 2);	// adding two bytes
					*pImportLookupTable = (DWORD) GetProcAddress(hMod, lpProcName);
					// SOSTITUISCE EXITPROCESS CON EXITTHREAD
					if (*pImportLookupTable == (DWORD)GetProcAddress(fpLoadLibraryA("kernel32"), "ExitProcess"))
						*pImportLookupTable = (DWORD)ExitThread;
				}
				pImportLookupTable++;		
			}
		pImportDescriptor++;
		
	}
	
}


ULONG ldr_exportdir(HMODULE hModule)
{
	
	ULONG pFunction = NULL;
	PIMAGE_DOS_HEADER pImageDosHeader = (PIMAGE_DOS_HEADER) hModule;
	PIMAGE_NT_HEADERS pImageNtHeaders = CALC_OFFSET(PIMAGE_NT_HEADERS, hModule, pImageDosHeader->e_lfanew);
	PIMAGE_DATA_DIRECTORY pExportDir = &pImageNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];

	if (pExportDir->Size == 0 || pExportDir->VirtualAddress == 0)
	{	// this module have no export directory)
		return pFunction;
	}
	
	// Processing export directory table
	PIMAGE_EXPORT_DIRECTORY pExportDirectory = (PIMAGE_EXPORT_DIRECTORY) ((PBYTE)pExportDir->VirtualAddress + (ULONG)hModule);

	//DWORD dwOrdinalBase = pExportDirectory->Base;

	// Fixing pointer with BASE
	pExportDirectory->AddressOfNames += DWORD(hModule);
	pExportDirectory->AddressOfFunctions += DWORD(hModule);
	pExportDirectory->AddressOfNameOrdinals += DWORD(hModule);

	// Fixing pointers of names and functions
	LPDWORD ptrFunctions = (LPDWORD) pExportDirectory->AddressOfFunctions;
	LPDWORD ptrNames = (LPDWORD) pExportDirectory->AddressOfNames;

	for(DWORD i = 0; i < pExportDirectory->NumberOfNames; i++)
	{
		ptrFunctions[i] += (DWORD) hModule;
		ptrNames[i] += (DWORD) hModule;

		pFunction = ptrFunctions[i];
	}
	
	return pFunction;
}

BOOL MemoryLoader(LPBYTE lpRawBuffer)
{
	LPVOID lpAddress = NULL;
	DWORD header_size = 0;
	IMAGE_DOS_HEADER dos_header;
	IMAGE_NT_HEADERS32 pe_header;
	
	if (lpRawBuffer != NULL)
	{
		memcpy(&dos_header, lpRawBuffer, sizeof(dos_header));	// get DOS HEADER
		if (dos_header.e_magic != IMAGE_DOS_SIGNATURE || dos_header.e_lfanew == 0)
		{	// invalid MZ signature
			return FALSE;
		}

		memcpy(&pe_header, CALC_OFFSET(LPVOID, lpRawBuffer, dos_header.e_lfanew), sizeof(pe_header));
		if (pe_header.Signature != IMAGE_NT_SIGNATURE)
		{	// invalid PE signature
			return FALSE;
		}

		lpAddress = VirtualAlloc(NULL, pe_header.OptionalHeader.SizeOfImage, MEM_COMMIT, PAGE_READWRITE);	// allocate image
		if (lpAddress == NULL)
		{	// wrong image size or insufficient memory!
			return FALSE;
		}

		header_size = dos_header.e_lfanew + 
			pe_header.FileHeader.SizeOfOptionalHeader + 
			sizeof(pe_header.FileHeader) + 4;

		IMAGE_SECTION_HEADER section;
		LPVOID lpBufferPtr = CALC_OFFSET(LPVOID, lpRawBuffer, header_size);
		memcpy(&section, lpBufferPtr, sizeof(section));
		
		// now first section is in memory?!?!?

		memcpy(lpAddress, lpRawBuffer, section.PointerToRawData);	// loading PE header in memory!
		PIMAGE_SECTION_HEADER sections = CALC_OFFSET(PIMAGE_SECTION_HEADER, lpAddress, header_size);
		for(USHORT i = 0; i < pe_header.FileHeader.NumberOfSections; i++, sections++)
		{
			LPVOID lpSectionBuffer = CALC_OFFSET(LPVOID, lpAddress, sections->VirtualAddress);
			// raw copy ..
			// @TODO: PointerToRawData can be 0 for uninitialized sections like SizeOfRawData
			memcpy(lpSectionBuffer, CALC_OFFSET(LPVOID, lpRawBuffer, sections->PointerToRawData), sections->SizeOfRawData);
		}
	}
	DWORD ignore = 0;

	// section initialized!
	// @TODO: relocations
	ldr_reloc(lpAddress, &pe_header);

	// @TODO: IAT
	ldr_importdir((HMODULE) lpAddress, &pe_header);

	PIMAGE_SECTION_HEADER sections = CALC_OFFSET(PIMAGE_SECTION_HEADER, lpAddress, header_size);

	for(USHORT i = 0; i < pe_header.FileHeader.NumberOfSections; i++, sections++)
	{
		LPVOID lpSectionBuffer = CALC_OFFSET(LPVOID, lpAddress, sections->VirtualAddress);
	
		if ((sections->Characteristics & IMAGE_SCN_MEM_EXECUTE) == IMAGE_SCN_MEM_EXECUTE)
		{	// set +X to page!
			
			VirtualProtect(lpSectionBuffer, sections->Misc.VirtualSize, PAGE_EXECUTE_READWRITE, &ignore);
		}
	}
	// at end set +R section
	VirtualProtect(lpAddress, header_size, PAGE_READONLY, &ignore);


	CALC_OFFSET(LPVOID, lpAddress, pe_header.OptionalHeader.AddressOfEntryPoint);
	MAIN ptrMain = (MAIN)CALC_OFFSET(LPVOID, lpAddress, pe_header.OptionalHeader.AddressOfEntryPoint);
	ptrMain((HINSTANCE)lpAddress, NULL, "", 0xa);
	
	return TRUE;
}






#include <Windows.h>


#define CALC_OFFSET(type, ptr, offset) (type) (((ULONG64) ptr) + offset)
#define CALC_OFFSET_DISP(type, base, offset, disp) (type)((DWORD)(base) + (DWORD)(offset) + disp)
#define CALC_DISP(type, offset, ptr) (type) (((ULONG64) offset) - (ULONG64) ptr)


typedef int (WINAPI *MAIN)(HINSTANCE, HINSTANCE, LPSTR, int);

typedef struct base_relocation_block
{
	DWORD PageRVA;
	DWORD BlockSize;
} base_relocation_block_t;

typedef struct base_relocation_entry
{
	WORD offset : 12;
	WORD type : 4;
} base_relocation_entry_t;


void ldr_reloc(LPVOID pModule, PIMAGE_NT_HEADERS pImageNtHeader);
void ldr_importdir(LPVOID pModule, PIMAGE_NT_HEADERS pImageNtHeader);
ULONG ldr_exportdir(HMODULE hModule);
BOOL MemoryLoader(LPBYTE lpRawBuffer);
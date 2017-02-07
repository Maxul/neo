#include "stdafx.h"

#define DRIVER_NAME "vmos"

#define FILE_DEVICE_WINIO 0x00008010

#define WINIO_IOCTL_INDEX 0x810

#define IOCTL_WINIO_MAPPHYSTOLIN \
	CTL_CODE(FILE_DEVICE_WINIO, WINIO_IOCTL_INDEX, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_WINIO_UNMAPPHYSADDR \
	CTL_CODE(FILE_DEVICE_WINIO, WINIO_IOCTL_INDEX + 1, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct
{
	DWORD64 dwPhysMemSizeInBytes;
	DWORD64 pvPhysAddress;
	DWORD64 PhysicalMemoryHandle;
	DWORD64 pvPhysMemLin;
	DWORD64 pvPhysSection;
} tagPhysStruct;

extern CHAR szWinIoDriverPath[MAX_PATH];
extern BOOL g_Is64BitOS;
extern HANDLE hDriver;
extern BOOL IsWinIoInitialized;

PBYTE _stdcall MapPhysToLin(tagPhysStruct *PhysStruct);
BOOL _stdcall UnmapPhysicalMemory(tagPhysStruct *PhysStruct);

BOOL __stdcall InitializeWinIo();
void _stdcall ShutdownWinIo();

BOOL _stdcall GetPhysLong(PBYTE pbPhysAddr, PDWORD pdwPhysVal);
BOOL _stdcall SetContiguousPhysLong(DWORD pbPhysAddr, DWORD dwPhysVal, PDWORD pdwLength);
BOOL _stdcall GetContiguousPhysLong(DWORD pbPhysAddr, PDWORD pdwLength);

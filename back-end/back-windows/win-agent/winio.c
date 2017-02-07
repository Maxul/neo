#include "winio.h"

BOOL _stdcall InstallWinIoDriver(LPCSTR pszWinIoDriverPath, BOOL IsDemandLoaded);
BOOL _stdcall RemoveWinIoDriver();
BOOL _stdcall StartWinIoDriver();
BOOL _stdcall StopWinIoDriver();

typedef UINT(WINAPI* GETSYSTEMWOW64DIRECTORY)(LPTSTR, UINT);

BOOL Is64BitOS()
{
#ifdef _WIN64
	return TRUE;
#else
	GETSYSTEMWOW64DIRECTORY getSystemWow64Directory;
	HMODULE hKernel32;
	TCHAR Wow64Directory[32767];

	hKernel32 = GetModuleHandle(TEXT("kernel32.dll"));
	if (hKernel32 == NULL)
	{
		return FALSE;
	}

	getSystemWow64Directory = (GETSYSTEMWOW64DIRECTORY)GetProcAddress(hKernel32, "GetSystemWow64DirectoryW");

	if (getSystemWow64Directory == NULL)
	{
		return FALSE;
	}

	if ((getSystemWow64Directory(Wow64Directory, _countof(Wow64Directory)) == 0) &&
		(GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)) {
		return FALSE;
	}
	return TRUE;
#endif
}

BOOL GetDriverPath()
{
	HANDLE fileHandle;

	if (GetCurrentDirectoryA(sizeof(szWinIoDriverPath), szWinIoDriverPath) == 0) {
		return FALSE;
	}

	if (FAILED(StringCbCatA(szWinIoDriverPath, sizeof(szWinIoDriverPath), "\\"DRIVER_NAME".sys"))) {
		return FALSE;
	}

	if ((fileHandle = CreateFileA(szWinIoDriverPath, GENERIC_READ, 0, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE) {
		return FALSE;
	}

	if (fileHandle) {
		CloseHandle(fileHandle);
	}

	return TRUE;
}


BOOL _stdcall InstallWinIoDriver(LPCSTR pszWinIoDriverPath, BOOL IsDemandLoaded)
{
	SC_HANDLE hSCManager;
	SC_HANDLE hService;

	// Remove any previous instance of the driver
	RemoveWinIoDriver();

	hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

	if (hSCManager)
	{
		// Install the driver
		hService = CreateServiceA(
			hSCManager,
			DRIVER_NAME,
			DRIVER_NAME,
			SERVICE_ALL_ACCESS,
			SERVICE_KERNEL_DRIVER,
			(IsDemandLoaded == TRUE) ? SERVICE_DEMAND_START : SERVICE_SYSTEM_START,
			SERVICE_ERROR_NORMAL,
			pszWinIoDriverPath,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL);

		CloseServiceHandle(hSCManager);

		if (hService == NULL)
			return FALSE;
	}
	else
		return FALSE;

	CloseServiceHandle(hService);

	return TRUE;
}

BOOL _stdcall RemoveWinIoDriver()
{
	SC_HANDLE hSCManager;
	SC_HANDLE hService;
	LPQUERY_SERVICE_CONFIG pServiceConfig;
	DWORD dwBytesNeeded;
	DWORD cbBufSize;
	BOOL bResult;

	StopWinIoDriver();

	hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

	if (!hSCManager)
	{
		return FALSE;
	}

	hService = OpenServiceA(hSCManager, DRIVER_NAME, SERVICE_ALL_ACCESS);
	CloseServiceHandle(hSCManager);

	if (!hService)
	{
		return FALSE;
	}

	bResult = QueryServiceConfig(hService, NULL, 0, &dwBytesNeeded);

	if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
	{
		cbBufSize = dwBytesNeeded;
		pServiceConfig = (LPQUERY_SERVICE_CONFIG)malloc(cbBufSize);
		bResult = QueryServiceConfig(hService, pServiceConfig, cbBufSize, &dwBytesNeeded);

		if (!bResult)
		{
			free(pServiceConfig);
			CloseServiceHandle(hService);
			return bResult;
		}

		// If service is set to load automatically, don't delete it!
		if (pServiceConfig->dwStartType == SERVICE_DEMAND_START)
		{
			bResult = DeleteService(hService);
		}
	}

	CloseServiceHandle(hService);

	return bResult;
}

BOOL _stdcall StartWinIoDriver()
{
	SC_HANDLE hSCManager;
	SC_HANDLE hService;
	BOOL bResult;

	hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

	if (hSCManager)
	{
		hService = OpenServiceA(hSCManager, DRIVER_NAME, SERVICE_ALL_ACCESS);

		CloseServiceHandle(hSCManager);

		if (hService)
		{
			bResult = StartService(hService, 0, NULL) || GetLastError() == ERROR_SERVICE_ALREADY_RUNNING;

			CloseServiceHandle(hService);
		}
		else
			return FALSE;
	}
	else
		return FALSE;

	return bResult;
}

BOOL _stdcall StopWinIoDriver()
{
	SC_HANDLE hSCManager;
	SC_HANDLE hService;
	SERVICE_STATUS ServiceStatus;
	BOOL bResult;

	hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

	if (hSCManager)
	{
		hService = OpenServiceA(hSCManager, DRIVER_NAME, SERVICE_ALL_ACCESS);

		CloseServiceHandle(hSCManager);

		if (hService)
		{
			bResult = ControlService(hService, SERVICE_CONTROL_STOP, &ServiceStatus);

			CloseServiceHandle(hService);
		}
		else
			return FALSE;
	}
	else
		return FALSE;

	return bResult;
}

PBYTE _stdcall MapPhysToLin(tagPhysStruct *PhysStruct)
{
	DWORD dwBytesReturned;

	if (!IsWinIoInitialized)
		return FALSE;

	if (!DeviceIoControl(hDriver, (DWORD)IOCTL_WINIO_MAPPHYSTOLIN, PhysStruct,
		sizeof(tagPhysStruct), PhysStruct, sizeof(tagPhysStruct), &dwBytesReturned, NULL))
	{
		return NULL;
	}

	return (PBYTE)PhysStruct->pvPhysMemLin;
}

BOOL _stdcall UnmapPhysicalMemory(tagPhysStruct *PhysStruct)
{
	DWORD dwBytesReturned;

	if (!IsWinIoInitialized)
	{
		return FALSE;
	}

	if (!DeviceIoControl(hDriver, (DWORD)IOCTL_WINIO_UNMAPPHYSADDR, PhysStruct,
		sizeof(tagPhysStruct), NULL, 0, &dwBytesReturned, NULL))
	{
		return FALSE;
	}

	return TRUE;
}

BOOL __stdcall InitializeWinIo()
{
	BOOL bResult;

	g_Is64BitOS = Is64BitOS();

	hDriver = CreateFileA("\\\\.\\"DRIVER_NAME,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	// If the driver is not running, install it

	if (hDriver == INVALID_HANDLE_VALUE)
	{
		GetDriverPath();
		printf_s("%s\n", szWinIoDriverPath, MAX_PATH);

		bResult = InstallWinIoDriver(szWinIoDriverPath, FALSE);

		if (!bResult)
			return FALSE;

		bResult = StartWinIoDriver();

		if (!bResult)
			return FALSE;

		hDriver = CreateFileA("\\\\.\\"DRIVER_NAME,
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);

		if (hDriver == INVALID_HANDLE_VALUE)
			return FALSE;
	}

	IsWinIoInitialized = TRUE;

	return TRUE;
}

void _stdcall ShutdownWinIo()
{
	if (hDriver != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hDriver);
	}

	RemoveWinIoDriver();

	IsWinIoInitialized = FALSE;
}

// 设置指定地址数据
BOOL _stdcall SetPhysLong(PBYTE pbPhysAddr, DWORD dwPhysVal)
{
	PDWORD pdwLinAddr;
	tagPhysStruct PhysStruct;

	if (!IsWinIoInitialized)
		return FALSE;

	if (g_Is64BitOS)
	{
		PhysStruct.pvPhysAddress = (DWORD64)pbPhysAddr;
	}
	else
	{
		// Avoid sign extension issues
		PhysStruct.pvPhysAddress = (DWORD64)(DWORD32)pbPhysAddr;
	}

	PhysStruct.dwPhysMemSizeInBytes = 4;

	pdwLinAddr = (PDWORD)MapPhysToLin(&PhysStruct);

	if (pdwLinAddr == NULL)
		return FALSE;

	*pdwLinAddr = dwPhysVal;

	UnmapPhysicalMemory(&PhysStruct);

	return TRUE;
}

// 查询指定地址数据
BOOL _stdcall GetPhysLong(PBYTE pbPhysAddr, PDWORD pdwPhysVal)
{
	PDWORD pdwLinAddr;
	tagPhysStruct PhysStruct;

	if (!IsWinIoInitialized)
		return FALSE;

	if (g_Is64BitOS)
	{
		PhysStruct.pvPhysAddress = (DWORD64)pbPhysAddr;
	}
	else
	{
		// Avoid sign extension issues
		PhysStruct.pvPhysAddress = (DWORD64)(DWORD32)pbPhysAddr;
	}

	PhysStruct.dwPhysMemSizeInBytes = 4;

	pdwLinAddr = (PDWORD)MapPhysToLin(&PhysStruct);

	if (pdwLinAddr == NULL)
		return FALSE;

	*pdwPhysVal = *pdwLinAddr;

	UnmapPhysicalMemory(&PhysStruct);

	return TRUE;
}

// 设置指定连续地址数值
BOOL _stdcall SetContiguousPhysLong(DWORD pbPhysAddr, DWORD dwPhysVal, PDWORD pdwLength)
{
	PDWORD pdwLinAddr;
	tagPhysStruct PhysStruct;

	if (!IsWinIoInitialized)
		return FALSE;

	for (BYTE i = 0; i < *pdwLength; i++) {
		if (g_Is64BitOS)
		{
			PhysStruct.pvPhysAddress = (DWORD64)(pbPhysAddr + i);
		}
		else
		{
			// Avoid sign extension issues
			PhysStruct.pvPhysAddress = (DWORD64)(DWORD32)(pbPhysAddr + i);
		}
		PhysStruct.dwPhysMemSizeInBytes = 4;

		if ((pdwLinAddr = (PDWORD)MapPhysToLin(&PhysStruct)) == NULL)
			return FALSE;
		else {
			*pdwLinAddr = dwPhysVal;
		}
	}

	UnmapPhysicalMemory(&PhysStruct);

	return TRUE;
}

// 查询指定连续地址数值
BOOL _stdcall GetContiguousPhysLong(DWORD pbPhysAddr, PDWORD pdwLength)
{
	PDWORD pdwLinAddr;
	tagPhysStruct PhysStruct;

	if (!IsWinIoInitialized)
		return FALSE;

	for (BYTE i = 0; i < *pdwLength; i++) {
		if (g_Is64BitOS)
		{
			PhysStruct.pvPhysAddress = (DWORD64)(pbPhysAddr + i);
		}
		else
		{
			// Avoid sign extension issues
			PhysStruct.pvPhysAddress = (DWORD64)(DWORD32)(pbPhysAddr + i);
		}
		PhysStruct.dwPhysMemSizeInBytes = 4;

		if ((pdwLinAddr = (PDWORD)MapPhysToLin(&PhysStruct)) == NULL)
			return FALSE;
		else {
			if ((i % 16) == 0)
				printf("\n0X%08X  ", pbPhysAddr + i); // 打印物理地址
			else if (i % 4 == 0)
				printf("\t");
			printf(" %02X", (BYTE)*pdwLinAddr);
		}
	}

	UnmapPhysicalMemory(&PhysStruct);

	return TRUE;
}


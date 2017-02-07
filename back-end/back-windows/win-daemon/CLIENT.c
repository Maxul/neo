#include <ntddk.h> // what we need to tame a driver

// 32768-65535 are reserved for use by customers.
#define FILE_DEVICE_CLIENT 0x00008010

// 2048-4095 are reserved for customers.
#define CLIENT_IOCTL_INDEX 0x810

#define IOCTL_CLIENT_MAPPHYSTOLIN \
	CTL_CODE(FILE_DEVICE_CLIENT, CLIENT_IOCTL_INDEX, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_CLIENT_UNMAPPHYSADDR \
	CTL_CODE(FILE_DEVICE_CLIENT, CLIENT_IOCTL_INDEX + 1, METHOD_BUFFERED, FILE_ANY_ACCESS)

#pragma pack(push)
#pragma pack(1)

struct tagPhysStruct
{
	DWORD64 dwPhysMemSizeInBytes;	// 一次打开空间大小
	DWORD64 pvPhysAddress;			// 欲打开的物理起始地址
	DWORD64 PhysicalMemoryHandle;	// 打开内内核区域句柄
	DWORD64 pvPhysMemLin;			// 物理地址对应的线性（虚拟）地址
	DWORD64 pvPhysSection;			// 
};

// 一共分享10MB内核空间
struct KernelMem
{
	char bSize[10 * 1024 * 1024];
};

#pragma pack(pop)

// Function definition section
// -----------------------------------------------------------------
DRIVER_INITIALIZE DriverEntry;
NTSTATUS WinIoDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
void WinIoUnload(IN PDRIVER_OBJECT DriverObject);
NTSTATUS UnmapPhysicalMemory(HANDLE PhysicalMemoryHandle, PVOID pPhysMemLin, PVOID PhysSection);
NTSTATUS MapPhysicalMemoryToLinearSpace(PVOID pPhysAddress,
	SIZE_T PhysMemSizeInBytes,
	PVOID *ppPhysMemLin,
	HANDLE *pPhysicalMemoryHandle,
	PVOID *ppPhysSection);

// -----------------------------------------------------------------

#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, DriverEntry )
#pragma alloc_text( PAGE, WinIoDispatch)
#pragma alloc_text( PAGE, WinIoUnload)
#pragma alloc_text( PAGE, UnmapPhysicalMemory)
#pragma alloc_text( PAGE, MapPhysicalMemoryToLinearSpace)
#endif // ALLOC_PRAGMA

PCHAR pMem;
VOID KFree()
{
	if (pMem != NULL)
	{
		MmFreeContiguousMemory(pMem);
		KdPrint(("释放 分配内存 成功"));
	}
}

void KMalloc(int memSize)
{
	// 获取物理地址所对应的虚拟地址，并向其中写入连续内存的基址
	struct tagPhysStruct AllocatePhysStruct;
	AllocatePhysStruct.pvPhysAddress = 0x0000;
	AllocatePhysStruct.dwPhysMemSizeInBytes = 16;

	MapPhysicalMemoryToLinearSpace(
		(ULONG_PTR)AllocatePhysStruct.pvPhysAddress,
		(SIZE_T)AllocatePhysStruct.dwPhysMemSizeInBytes,
		(PVOID *)&AllocatePhysStruct.pvPhysMemLin,
		(HANDLE *)&AllocatePhysStruct.PhysicalMemoryHandle,
		(PVOID *)&AllocatePhysStruct.pvPhysSection);
	if (*(INT *)(AllocatePhysStruct.pvPhysMemLin + 2) != 0xFF53F000)
	{
		KdPrint(("分配内存 已存在 0x%0X", *(INT *)(AllocatePhysStruct.pvPhysMemLin + 2)));
		UnmapPhysicalMemory((HANDLE)AllocatePhysStruct.PhysicalMemoryHandle,
			(PVOID)AllocatePhysStruct.pvPhysMemLin,
			(PVOID)AllocatePhysStruct.pvPhysSection);
		return;
	}

	PHYSICAL_ADDRESS phyHighAddr;
	phyHighAddr.QuadPart = 0xFFFFFFFFFFFFFFFF;	// 最大的可用的物理地址

	// 申请连续内核物理空间
	pMem = MmAllocateContiguousMemory(memSize, phyHighAddr);

	PHYSICAL_ADDRESS PhysAddress;				// 物理地址
	if (pMem != NULL)							// 虚拟地址分配成功
	{
		PhysAddress = MmGetPhysicalAddress(pMem); // 返回虚拟地址对应的物理地址

		KdPrint(("分配内核 连续空间 地址为：0x%0X", PhysAddress.QuadPart));
		KdPrint(("分配内核 连续空间 大小为：0x%0X", memSize));

		memset(pMem, 0XFF, memSize);

		// 以下为qemu特例，告诉server I'm ready.
		*(PVOID *)(AllocatePhysStruct.pvPhysMemLin + 0) = 0x23; // [0]=>'#'
		*(PVOID *)(AllocatePhysStruct.pvPhysMemLin + 1) = 0x2A; // [1]=>'*'

		// 将基址写入物理地址为0x0002处
		*(PVOID *)(AllocatePhysStruct.pvPhysMemLin + 2) = PhysAddress.QuadPart;
		KdPrint(("分配内存 成功"));
	}
	else
	{
		pMem = NULL;
		KdPrint(("分配内存 失败"));
	}

	UnmapPhysicalMemory((HANDLE)AllocatePhysStruct.PhysicalMemoryHandle,
		(PVOID)AllocatePhysStruct.pvPhysMemLin,
		(PVOID)AllocatePhysStruct.pvPhysSection);
}

// 驱动主入口

NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(RegistryPath);

	UNICODE_STRING  DeviceNameUnicodeString;
	UNICODE_STRING  DeviceLinkUnicodeString;
	NTSTATUS        ntStatus;
	PDEVICE_OBJECT  DeviceObject = NULL;

	KdPrint(("驱动启动"));

	RtlInitUnicodeString(&DeviceNameUnicodeString, L"\\Device\\vmos");

	ntStatus = IoCreateDevice(DriverObject,
		0, // 无设备扩展
		&DeviceNameUnicodeString,
		FILE_DEVICE_CLIENT,
		0,
		FALSE,
		&DeviceObject);

	if (NT_SUCCESS(ntStatus))
	{
		DriverObject->MajorFunction[IRP_MJ_CREATE] =
			DriverObject->MajorFunction[IRP_MJ_CLOSE] =
			DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = WinIoDispatch;

		DriverObject->DriverUnload = WinIoUnload;

		// 设备链接名，用于用户空间打开

		RtlInitUnicodeString(&DeviceLinkUnicodeString, L"\\DosDevices\\vmos");

		ntStatus = IoCreateSymbolicLink(&DeviceLinkUnicodeString, &DeviceNameUnicodeString);

		if (!NT_SUCCESS(ntStatus))
		{
			KdPrint(("ERROR: 无法创建 符号链接。"));
			IoDeleteDevice(DeviceObject);
		}

	}
	else
	{
		KdPrint(("ERROR: 设备创建 失败。"));
	}

	KMalloc(sizeof(struct KernelMem));

	KdPrint(("驱动生成"));

	return ntStatus;
}


// Process the IRPs sent to this device

NTSTATUS WinIoDispatch(IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	PIO_STACK_LOCATION IrpStack;
	ULONG              dwInputBufferLength;
	ULONG              dwOutputBufferLength;
	ULONG              dwIoControlCode;
	PVOID              pvIOBuffer;
	NTSTATUS           ntStatus;
	struct             tagPhysStruct PhysStruct;
	struct             tagPhysStruct32 *pPhysStruct32 = NULL;

	KdPrint(("进入 处理 函数"));

	// 初始化为默认值

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;

	IrpStack = IoGetCurrentIrpStackLocation(Irp);

	// 获取输入/输出缓冲区地址和长度

	pvIOBuffer = Irp->AssociatedIrp.SystemBuffer;
	dwInputBufferLength = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
	dwOutputBufferLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

	switch (IrpStack->MajorFunction)
	{
	case IRP_MJ_CREATE:

		KdPrint(("收到 创建 信号"));

		KMalloc(sizeof(struct KernelMem));

		break;

	case IRP_MJ_CLOSE:

		KdPrint(("收到 关闭 信号"));
		
		// KFree();

		break;

	case IRP_MJ_DEVICE_CONTROL:

		KdPrint(("收到 控制 信号"));

		dwIoControlCode = IrpStack->Parameters.DeviceIoControl.IoControlCode;

		switch (dwIoControlCode)
		{

		case IOCTL_CLIENT_MAPPHYSTOLIN:

			KdPrint(("打开 映射 信号"));

			if (dwInputBufferLength)
			{
				memcpy(&PhysStruct, pvIOBuffer, dwInputBufferLength);

				ntStatus = MapPhysicalMemoryToLinearSpace(
					(PVOID)PhysStruct.pvPhysAddress,
					(SIZE_T)PhysStruct.dwPhysMemSizeInBytes,
					(PVOID *)&PhysStruct.pvPhysMemLin,
					(HANDLE *)&PhysStruct.PhysicalMemoryHandle,
					(PVOID *)&PhysStruct.pvPhysSection);

				if (NT_SUCCESS(ntStatus))
				{
					memcpy(pvIOBuffer, &PhysStruct, dwInputBufferLength);
					Irp->IoStatus.Information = dwInputBufferLength;
				}

				Irp->IoStatus.Status = ntStatus;
			}
			else
				Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;

			break;

		case IOCTL_CLIENT_UNMAPPHYSADDR:

			KdPrint(("关闭 映射 信号"));

			if (dwInputBufferLength)
			{
				memcpy(&PhysStruct, pvIOBuffer, dwInputBufferLength);

				ntStatus = UnmapPhysicalMemory((HANDLE)PhysStruct.PhysicalMemoryHandle, (PVOID)PhysStruct.pvPhysMemLin, (PVOID)PhysStruct.pvPhysSection);

				Irp->IoStatus.Status = ntStatus;
			}
			else
				Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;

			break;

		default:

			KdPrint(("ERROR: 未知信号"));

			Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;

			break;
		}

		break;
	}

	ntStatus = Irp->IoStatus.Status;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	KdPrint(("离开 处理 函数"));

	return ntStatus;
}

// Delete the associated device and return

void WinIoUnload(IN PDRIVER_OBJECT DriverObject)
{
	UNICODE_STRING DeviceLinkUnicodeString;
	NTSTATUS ntStatus;

	KdPrint(("进入 卸载 函数"));

	RtlInitUnicodeString(&DeviceLinkUnicodeString, L"\\DosDevices\\vmos");

	ntStatus = IoDeleteSymbolicLink(&DeviceLinkUnicodeString);

	if (NT_SUCCESS(ntStatus))
	{
		IoDeleteDevice(DriverObject->DeviceObject);
	}
	else
	{
		KdPrint(("ERROR: 无法 删除 符号连接"));
	}

	KdPrint(("离开 卸载 函数"));
}

// 物理地址映射函数
NTSTATUS MapPhysicalMemoryToLinearSpace(PVOID pPhysAddress,
	SIZE_T PhysMemSizeInBytes,
	PVOID *ppPhysMemLin,
	HANDLE *pPhysicalMemoryHandle,
	PVOID *ppPhysSection)
{
	UNICODE_STRING     PhysicalMemoryUnicodeString;
	OBJECT_ATTRIBUTES  ObjectAttributes;
	PHYSICAL_ADDRESS   ViewBase;
	NTSTATUS           ntStatus;
	PHYSICAL_ADDRESS   pStartPhysAddress;
	PHYSICAL_ADDRESS   pEndPhysAddress;
	BOOLEAN            Result1, Result2;
	ULONG              IsIOSpace;
	unsigned char     *pbPhysMemLin = NULL;

	KdPrint(("进入 映射 函数"));

	RtlInitUnicodeString(&PhysicalMemoryUnicodeString, L"\\Device\\PhysicalMemory");

	InitializeObjectAttributes(&ObjectAttributes,
		&PhysicalMemoryUnicodeString,
		OBJ_CASE_INSENSITIVE,
		(HANDLE)NULL,
		(PSECURITY_DESCRIPTOR)NULL);

	*pPhysicalMemoryHandle = NULL;
	*ppPhysSection = NULL;

	ntStatus = ZwOpenSection(pPhysicalMemoryHandle, SECTION_ALL_ACCESS, &ObjectAttributes);

	if (NT_SUCCESS(ntStatus))
	{

		ntStatus = ObReferenceObjectByHandle(*pPhysicalMemoryHandle,
			SECTION_ALL_ACCESS,
			(POBJECT_TYPE)NULL,
			KernelMode,
			ppPhysSection,
			(POBJECT_HANDLE_INFORMATION)NULL);

		if (NT_SUCCESS(ntStatus))
		{
			pStartPhysAddress.QuadPart = (ULONGLONG)(ULONG_PTR)pPhysAddress;

			pEndPhysAddress.QuadPart = pStartPhysAddress.QuadPart + PhysMemSizeInBytes;

			IsIOSpace = 0;

			Result1 = HalTranslateBusAddress(1, 0, pStartPhysAddress, &IsIOSpace, &pStartPhysAddress);

			IsIOSpace = 0;

			Result2 = HalTranslateBusAddress(1, 0, pEndPhysAddress, &IsIOSpace, &pEndPhysAddress);

			if (Result1 && Result2)
			{
				// Let ZwMapViewOfSection pick a linear address

				PhysMemSizeInBytes = (SIZE_T)pEndPhysAddress.QuadPart - (SIZE_T)pStartPhysAddress.QuadPart;

				ViewBase = pStartPhysAddress;

				ntStatus = ZwMapViewOfSection(*pPhysicalMemoryHandle,
					(HANDLE)-1,
					&pbPhysMemLin,
					0L,
					PhysMemSizeInBytes,
					&ViewBase,
					&PhysMemSizeInBytes,
					ViewShare,
					0,
					PAGE_READWRITE | PAGE_NOCACHE);

				// If the physical memory is already mapped with a different caching attribute, try again
				if (ntStatus == STATUS_CONFLICTING_ADDRESSES)
				{
					ntStatus = ZwMapViewOfSection(*pPhysicalMemoryHandle,
						(HANDLE)-1,
						&pbPhysMemLin,
						0L,
						PhysMemSizeInBytes,
						&ViewBase,
						&PhysMemSizeInBytes,
						ViewShare,
						0,
						PAGE_READWRITE);
				}

				if (!NT_SUCCESS(ntStatus))
					KdPrint(("ERROR: ZwMapViewOfSection failed"));
				else
				{
					pbPhysMemLin += pStartPhysAddress.QuadPart - ViewBase.QuadPart;
					*ppPhysMemLin = pbPhysMemLin;
				}
			}
			else
				KdPrint(("ERROR: HalTranslateBusAddress failed"));
		}
		else
			KdPrint(("ERROR: ObReferenceObjectByHandle failed"));
	}
	else
		KdPrint(("ERROR: ZwOpenSection failed"));

	if (!NT_SUCCESS(ntStatus))
		ZwClose(*pPhysicalMemoryHandle);

	KdPrint(("离开 映射 函数"));

	return ntStatus;
}


NTSTATUS UnmapPhysicalMemory(HANDLE PhysicalMemoryHandle, PVOID pPhysMemLin, PVOID pPhysSection)
{
	NTSTATUS ntStatus;

	KdPrint(("进入 取消映射 函数"));

	ntStatus = ZwUnmapViewOfSection((HANDLE)-1, pPhysMemLin);

	if (!NT_SUCCESS(ntStatus))
		KdPrint(("ERROR: UnmapViewOfSection 失败"));

	if (pPhysSection)
		ObDereferenceObject(pPhysSection);

	ZwClose(PhysicalMemoryHandle);

	KdPrint(("离开 取消映射 函数"));

	return ntStatus;
}

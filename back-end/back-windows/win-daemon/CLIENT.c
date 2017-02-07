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
	DWORD64 dwPhysMemSizeInBytes;	// һ�δ򿪿ռ��С
	DWORD64 pvPhysAddress;			// ���򿪵�������ʼ��ַ
	DWORD64 PhysicalMemoryHandle;	// �����ں�������
	DWORD64 pvPhysMemLin;			// �����ַ��Ӧ�����ԣ����⣩��ַ
	DWORD64 pvPhysSection;			// 
};

// һ������10MB�ں˿ռ�
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
		KdPrint(("�ͷ� �����ڴ� �ɹ�"));
	}
}

void KMalloc(int memSize)
{
	// ��ȡ�����ַ����Ӧ�������ַ����������д�������ڴ�Ļ�ַ
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
		KdPrint(("�����ڴ� �Ѵ��� 0x%0X", *(INT *)(AllocatePhysStruct.pvPhysMemLin + 2)));
		UnmapPhysicalMemory((HANDLE)AllocatePhysStruct.PhysicalMemoryHandle,
			(PVOID)AllocatePhysStruct.pvPhysMemLin,
			(PVOID)AllocatePhysStruct.pvPhysSection);
		return;
	}

	PHYSICAL_ADDRESS phyHighAddr;
	phyHighAddr.QuadPart = 0xFFFFFFFFFFFFFFFF;	// ���Ŀ��õ������ַ

	// ���������ں�����ռ�
	pMem = MmAllocateContiguousMemory(memSize, phyHighAddr);

	PHYSICAL_ADDRESS PhysAddress;				// �����ַ
	if (pMem != NULL)							// �����ַ����ɹ�
	{
		PhysAddress = MmGetPhysicalAddress(pMem); // ���������ַ��Ӧ�������ַ

		KdPrint(("�����ں� �����ռ� ��ַΪ��0x%0X", PhysAddress.QuadPart));
		KdPrint(("�����ں� �����ռ� ��СΪ��0x%0X", memSize));

		memset(pMem, 0XFF, memSize);

		// ����Ϊqemu����������server I'm ready.
		*(PVOID *)(AllocatePhysStruct.pvPhysMemLin + 0) = 0x23; // [0]=>'#'
		*(PVOID *)(AllocatePhysStruct.pvPhysMemLin + 1) = 0x2A; // [1]=>'*'

		// ����ַд�������ַΪ0x0002��
		*(PVOID *)(AllocatePhysStruct.pvPhysMemLin + 2) = PhysAddress.QuadPart;
		KdPrint(("�����ڴ� �ɹ�"));
	}
	else
	{
		pMem = NULL;
		KdPrint(("�����ڴ� ʧ��"));
	}

	UnmapPhysicalMemory((HANDLE)AllocatePhysStruct.PhysicalMemoryHandle,
		(PVOID)AllocatePhysStruct.pvPhysMemLin,
		(PVOID)AllocatePhysStruct.pvPhysSection);
}

// ���������

NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(RegistryPath);

	UNICODE_STRING  DeviceNameUnicodeString;
	UNICODE_STRING  DeviceLinkUnicodeString;
	NTSTATUS        ntStatus;
	PDEVICE_OBJECT  DeviceObject = NULL;

	KdPrint(("��������"));

	RtlInitUnicodeString(&DeviceNameUnicodeString, L"\\Device\\vmos");

	ntStatus = IoCreateDevice(DriverObject,
		0, // ���豸��չ
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

		// �豸�������������û��ռ��

		RtlInitUnicodeString(&DeviceLinkUnicodeString, L"\\DosDevices\\vmos");

		ntStatus = IoCreateSymbolicLink(&DeviceLinkUnicodeString, &DeviceNameUnicodeString);

		if (!NT_SUCCESS(ntStatus))
		{
			KdPrint(("ERROR: �޷����� �������ӡ�"));
			IoDeleteDevice(DeviceObject);
		}

	}
	else
	{
		KdPrint(("ERROR: �豸���� ʧ�ܡ�"));
	}

	KMalloc(sizeof(struct KernelMem));

	KdPrint(("��������"));

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

	KdPrint(("���� ���� ����"));

	// ��ʼ��ΪĬ��ֵ

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;

	IrpStack = IoGetCurrentIrpStackLocation(Irp);

	// ��ȡ����/�����������ַ�ͳ���

	pvIOBuffer = Irp->AssociatedIrp.SystemBuffer;
	dwInputBufferLength = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
	dwOutputBufferLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

	switch (IrpStack->MajorFunction)
	{
	case IRP_MJ_CREATE:

		KdPrint(("�յ� ���� �ź�"));

		KMalloc(sizeof(struct KernelMem));

		break;

	case IRP_MJ_CLOSE:

		KdPrint(("�յ� �ر� �ź�"));
		
		// KFree();

		break;

	case IRP_MJ_DEVICE_CONTROL:

		KdPrint(("�յ� ���� �ź�"));

		dwIoControlCode = IrpStack->Parameters.DeviceIoControl.IoControlCode;

		switch (dwIoControlCode)
		{

		case IOCTL_CLIENT_MAPPHYSTOLIN:

			KdPrint(("�� ӳ�� �ź�"));

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

			KdPrint(("�ر� ӳ�� �ź�"));

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

			KdPrint(("ERROR: δ֪�ź�"));

			Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;

			break;
		}

		break;
	}

	ntStatus = Irp->IoStatus.Status;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	KdPrint(("�뿪 ���� ����"));

	return ntStatus;
}

// Delete the associated device and return

void WinIoUnload(IN PDRIVER_OBJECT DriverObject)
{
	UNICODE_STRING DeviceLinkUnicodeString;
	NTSTATUS ntStatus;

	KdPrint(("���� ж�� ����"));

	RtlInitUnicodeString(&DeviceLinkUnicodeString, L"\\DosDevices\\vmos");

	ntStatus = IoDeleteSymbolicLink(&DeviceLinkUnicodeString);

	if (NT_SUCCESS(ntStatus))
	{
		IoDeleteDevice(DriverObject->DeviceObject);
	}
	else
	{
		KdPrint(("ERROR: �޷� ɾ�� ��������"));
	}

	KdPrint(("�뿪 ж�� ����"));
}

// �����ַӳ�亯��
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

	KdPrint(("���� ӳ�� ����"));

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

	KdPrint(("�뿪 ӳ�� ����"));

	return ntStatus;
}


NTSTATUS UnmapPhysicalMemory(HANDLE PhysicalMemoryHandle, PVOID pPhysMemLin, PVOID pPhysSection)
{
	NTSTATUS ntStatus;

	KdPrint(("���� ȡ��ӳ�� ����"));

	ntStatus = ZwUnmapViewOfSection((HANDLE)-1, pPhysMemLin);

	if (!NT_SUCCESS(ntStatus))
		KdPrint(("ERROR: UnmapViewOfSection ʧ��"));

	if (pPhysSection)
		ObDereferenceObject(pPhysSection);

	ZwClose(PhysicalMemoryHandle);

	KdPrint(("�뿪 ȡ��ӳ�� ����"));

	return ntStatus;
}

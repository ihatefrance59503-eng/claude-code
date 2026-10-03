// this source is from NewReality a discord server with 1500+ sources: discord.gg/newreality
#pragma once
#include <ntdef.h>
#include <ntifs.h>
#include <ntddk.h>
#include <windef.h>
#include <ntstrsafe.h>
#include <ntimage.h>
enum requests
{
	REQUEST_PEB,
	REQUEST_PID,
	REQUEST_MODULEBASE,
	REQUEST_READ,
	REQUEST_WRITE
};

struct request
{
	uintptr_t key;
	uintptr_t request;
	uintptr_t address;
	uintptr_t PID;
	const char* processName;
	const char* moduleName;
	void* pBuffer;
	uintptr_t size_of_buffer;
};


typedef struct _RTL_PROCESS_MODULE_INFORMATION
{
	HANDLE Section;
	PVOID MappedBase;
	PVOID ImageBase;
	ULONG ImageSize;
	ULONG Flags;
	USHORT LoadOrderIndex;
	USHORT InitOrderIndex;
	USHORT LoadCount;
	USHORT OffsetToFileName;
	UCHAR  FullPathName[256];
} RTL_PROCESS_MODULE_INFORMATION, * PRTL_PROCESS_MODULE_INFORMATION;

typedef struct _RTL_PROCESS_MODULES
{
	ULONG NumberOfModules;
	RTL_PROCESS_MODULE_INFORMATION Modules[1];
} RTL_PROCESS_MODULES, * PRTL_PROCESS_MODULES;

typedef struct _WRITE_STRUCT
{
	int special;

	bool read;
	bool write;
	bool request_base;
	bool new_base;
	int target_pid;
	void* base_address;
	void* address;
	void* buffer;
	void* output;
	void* process_base;

	size_t size;

	const char* mod_name;
}WRITE_STRUCT;


typedef union _virt_addr_t
{
	void* value;
	struct
	{
		uintptr_t offset : 12;
		uintptr_t pt_index : 9;
		uintptr_t pd_index : 9;
		uintptr_t pdpt_index : 9;
		uintptr_t pml4_index : 9;
		uintptr_t reserved : 16;
	};
} virt_addr_t, * pvirt_addr_t;
typedef struct _MI_ACTIVE_PFN
{
	union
	{
		struct
		{
			struct /* bitfield */
			{
				/* 0x0000 */ unsigned __int64 Tradable : 1; /* bit position: 0 */
				/* 0x0000 */ unsigned __int64 NonPagedBuddy : 43; /* bit position: 1 */
			}; /* bitfield */
		} /* size: 0x0008 */ Leaf;
		struct
		{
			struct /* bitfield */
			{
				/* 0x0000 */ unsigned __int64 Tradable : 1; /* bit position: 0 */
				/* 0x0000 */ unsigned __int64 WsleAge : 3; /* bit position: 1 */
				/* 0x0000 */ unsigned __int64 OldestWsleLeafEntries : 10; /* bit position: 4 */
				/* 0x0000 */ unsigned __int64 OldestWsleLeafAge : 3; /* bit position: 14 */
				/* 0x0000 */ unsigned __int64 NonPagedBuddy : 43; /* bit position: 17 */
			}; /* bitfield */
		} /* size: 0x0008 */ PageTable;
		/* 0x0000 */ unsigned __int64 EntireActiveField;
	}; /* size: 0x0008 */
} MI_ACTIVE_PFN, * PMI_ACTIVE_PFN; /* size: 0x0008 */

typedef struct _MMPTE_HARDWARE
{
	struct /* bitfield */
	{
		/* 0x0000 */ unsigned __int64 Valid : 1; /* bit position: 0 */
		/* 0x0000 */ unsigned __int64 Dirty1 : 1; /* bit position: 1 */
		/* 0x0000 */ unsigned __int64 Owner : 1; /* bit position: 2 */
		/* 0x0000 */ unsigned __int64 WriteThrough : 1; /* bit position: 3 */
		/* 0x0000 */ unsigned __int64 CacheDisable : 1; /* bit position: 4 */
		/* 0x0000 */ unsigned __int64 Accessed : 1; /* bit position: 5 */
		/* 0x0000 */ unsigned __int64 Dirty : 1; /* bit position: 6 */
		/* 0x0000 */ unsigned __int64 LargePage : 1; /* bit position: 7 */
		/* 0x0000 */ unsigned __int64 Global : 1; /* bit position: 8 */
		/* 0x0000 */ unsigned __int64 CopyOnWrite : 1; /* bit position: 9 */
		/* 0x0000 */ unsigned __int64 Unused : 1; /* bit position: 10 */
		/* 0x0000 */ unsigned __int64 Write : 1; /* bit position: 11 */
		/* 0x0000 */ unsigned __int64 PageFrameNumber : 40; /* bit position: 12 */
		/* 0x0000 */ unsigned __int64 ReservedForSoftware : 4; /* bit position: 52 */
		/* 0x0000 */ unsigned __int64 WsleAge : 4; /* bit position: 56 */
		/* 0x0000 */ unsigned __int64 WsleProtection : 3; /* bit position: 60 */
		/* 0x0000 */ unsigned __int64 NoExecute : 1; /* bit position: 63 */
	}; /* bitfield */
} MMPTE_HARDWARE, * PMMPTE_HARDWARE; /* size: 0x0008 */

typedef struct _MMPTE_PROTOTYPE
{
	struct /* bitfield */
	{
		/* 0x0000 */ unsigned __int64 Valid : 1; /* bit position: 0 */
		/* 0x0000 */ unsigned __int64 DemandFillProto : 1; /* bit position: 1 */
		/* 0x0000 */ unsigned __int64 HiberVerifyConverted : 1; /* bit position: 2 */
		/* 0x0000 */ unsigned __int64 ReadOnly : 1; /* bit position: 3 */
		/* 0x0000 */ unsigned __int64 SwizzleBit : 1; /* bit position: 4 */
		/* 0x0000 */ unsigned __int64 Protection : 5; /* bit position: 5 */
		/* 0x0000 */ unsigned __int64 Prototype : 1; /* bit position: 10 */
		/* 0x0000 */ unsigned __int64 Combined : 1; /* bit position: 11 */
		/* 0x0000 */ unsigned __int64 Unused1 : 4; /* bit position: 12 */
		/* 0x0000 */ __int64 ProtoAddress : 48; /* bit position: 16 */
	}; /* bitfield */
} MMPTE_PROTOTYPE, * PMMPTE_PROTOTYPE; /* size: 0x0008 */

typedef struct _MMPTE_SOFTWARE
{
	struct /* bitfield */
	{
		/* 0x0000 */ unsigned __int64 Valid : 1; /* bit position: 0 */
		/* 0x0000 */ unsigned __int64 PageFileReserved : 1; /* bit position: 1 */
		/* 0x0000 */ unsigned __int64 PageFileAllocated : 1; /* bit position: 2 */
		/* 0x0000 */ unsigned __int64 ColdPage : 1; /* bit position: 3 */
		/* 0x0000 */ unsigned __int64 SwizzleBit : 1; /* bit position: 4 */
		/* 0x0000 */ unsigned __int64 Protection : 5; /* bit position: 5 */
		/* 0x0000 */ unsigned __int64 Prototype : 1; /* bit position: 10 */
		/* 0x0000 */ unsigned __int64 Transition : 1; /* bit position: 11 */
		/* 0x0000 */ unsigned __int64 PageFileLow : 4; /* bit position: 12 */
		/* 0x0000 */ unsigned __int64 UsedPageTableEntries : 10; /* bit position: 16 */
		/* 0x0000 */ unsigned __int64 ShadowStack : 1; /* bit position: 26 */
		/* 0x0000 */ unsigned __int64 Unused : 5; /* bit position: 27 */
		/* 0x0000 */ unsigned __int64 PageFileHigh : 32; /* bit position: 32 */
	}; /* bitfield */
} MMPTE_SOFTWARE, * PMMPTE_SOFTWARE; /* size: 0x0008 */

typedef struct _MMPTE_TIMESTAMP
{
	struct /* bitfield */
	{
		/* 0x0000 */ unsigned __int64 MustBeZero : 1; /* bit position: 0 */
		/* 0x0000 */ unsigned __int64 Unused : 3; /* bit position: 1 */
		/* 0x0000 */ unsigned __int64 SwizzleBit : 1; /* bit position: 4 */
		/* 0x0000 */ unsigned __int64 Protection : 5; /* bit position: 5 */
		/* 0x0000 */ unsigned __int64 Prototype : 1; /* bit position: 10 */
		/* 0x0000 */ unsigned __int64 Transition : 1; /* bit position: 11 */
		/* 0x0000 */ unsigned __int64 PageFileLow : 4; /* bit position: 12 */
		/* 0x0000 */ unsigned __int64 Reserved : 16; /* bit position: 16 */
		/* 0x0000 */ unsigned __int64 GlobalTimeStamp : 32; /* bit position: 32 */
	}; /* bitfield */
} MMPTE_TIMESTAMP, * PMMPTE_TIMESTAMP; /* size: 0x0008 */

typedef struct _MMPTE_TRANSITION
{
	struct /* bitfield */
	{
		/* 0x0000 */ unsigned __int64 Valid : 1; /* bit position: 0 */
		/* 0x0000 */ unsigned __int64 Write : 1; /* bit position: 1 */
		/* 0x0000 */ unsigned __int64 Spare : 1; /* bit position: 2 */
		/* 0x0000 */ unsigned __int64 IoTracker : 1; /* bit position: 3 */
		/* 0x0000 */ unsigned __int64 SwizzleBit : 1; /* bit position: 4 */
		/* 0x0000 */ unsigned __int64 Protection : 5; /* bit position: 5 */
		/* 0x0000 */ unsigned __int64 Prototype : 1; /* bit position: 10 */
		/* 0x0000 */ unsigned __int64 Transition : 1; /* bit position: 11 */
		/* 0x0000 */ unsigned __int64 PageFrameNumber : 40; /* bit position: 12 */
		/* 0x0000 */ unsigned __int64 Unused : 12; /* bit position: 52 */
	}; /* bitfield */
} MMPTE_TRANSITION, * PMMPTE_TRANSITION; /* size: 0x0008 */

typedef struct _MMPTE_SUBSECTION
{
	struct /* bitfield */
	{
		/* 0x0000 */ unsigned __int64 Valid : 1; /* bit position: 0 */
		/* 0x0000 */ unsigned __int64 Unused0 : 3; /* bit position: 1 */
		/* 0x0000 */ unsigned __int64 SwizzleBit : 1; /* bit position: 4 */
		/* 0x0000 */ unsigned __int64 Protection : 5; /* bit position: 5 */
		/* 0x0000 */ unsigned __int64 Prototype : 1; /* bit position: 10 */
		/* 0x0000 */ unsigned __int64 ColdPage : 1; /* bit position: 11 */
		/* 0x0000 */ unsigned __int64 Unused1 : 3; /* bit position: 12 */
		/* 0x0000 */ unsigned __int64 ExecutePrivilege : 1; /* bit position: 15 */
		/* 0x0000 */ __int64 SubsectionAddress : 48; /* bit position: 16 */
	}; /* bitfield */
} MMPTE_SUBSECTION, * PMMPTE_SUBSECTION; /* size: 0x0008 */

typedef struct _MMPTE_LIST
{
	struct /* bitfield */
	{
		/* 0x0000 */ unsigned __int64 Valid : 1; /* bit position: 0 */
		/* 0x0000 */ unsigned __int64 OneEntry : 1; /* bit position: 1 */
		/* 0x0000 */ unsigned __int64 filler0 : 2; /* bit position: 2 */
		/* 0x0000 */ unsigned __int64 SwizzleBit : 1; /* bit position: 4 */
		/* 0x0000 */ unsigned __int64 Protection : 5; /* bit position: 5 */
		/* 0x0000 */ unsigned __int64 Prototype : 1; /* bit position: 10 */
		/* 0x0000 */ unsigned __int64 Transition : 1; /* bit position: 11 */
		/* 0x0000 */ unsigned __int64 filler1 : 16; /* bit position: 12 */
		/* 0x0000 */ unsigned __int64 NextEntry : 36; /* bit position: 28 */
	}; /* bitfield */
} MMPTE_LIST, * PMMPTE_LIST; /* size: 0x0008 */

typedef struct _MMPTE
{
	union
	{
		union
		{
			/* 0x0000 */ unsigned __int64 Long;
			/* 0x0000 */ volatile unsigned __int64 VolatileLong;
			/* 0x0000 */ struct _MMPTE_HARDWARE Hard;
			/* 0x0000 */ struct _MMPTE_PROTOTYPE Proto;
			/* 0x0000 */ struct _MMPTE_SOFTWARE Soft;
			/* 0x0000 */ struct _MMPTE_TIMESTAMP TimeStamp;
			/* 0x0000 */ struct _MMPTE_TRANSITION Trans;
			/* 0x0000 */ struct _MMPTE_SUBSECTION Subsect;
			/* 0x0000 */ struct _MMPTE_LIST List;
		}; /* size: 0x0008 */
	} /* size: 0x0008 */ u;
} MMPTE, * PMMPTE; /* size: 0x0008 */

typedef struct _MIPFNBLINK
{
	union
	{
		struct /* bitfield */
		{
			/* 0x0000 */ unsigned __int64 Blink : 40; /* bit position: 0 */
			/* 0x0000 */ unsigned __int64 NodeBlinkLow : 19; /* bit position: 40 */
			/* 0x0000 */ unsigned __int64 TbFlushStamp : 3; /* bit position: 59 */
			/* 0x0000 */ unsigned __int64 PageBlinkDeleteBit : 1; /* bit position: 62 */
			/* 0x0000 */ unsigned __int64 PageBlinkLockBit : 1; /* bit position: 63 */
		}; /* bitfield */
		struct /* bitfield */
		{
			/* 0x0000 */ unsigned __int64 ShareCount : 62; /* bit position: 0 */
			/* 0x0000 */ unsigned __int64 PageShareCountDeleteBit : 1; /* bit position: 62 */
			/* 0x0000 */ unsigned __int64 PageShareCountLockBit : 1; /* bit position: 63 */
		}; /* bitfield */
		/* 0x0000 */ unsigned __int64 EntireField;
		/* 0x0000 */ volatile __int64 Lock;
		struct /* bitfield */
		{
			/* 0x0000 */ unsigned __int64 LockNotUsed : 62; /* bit position: 0 */
			/* 0x0000 */ unsigned __int64 DeleteBit : 1; /* bit position: 62 */
			/* 0x0000 */ unsigned __int64 LockBit : 1; /* bit position: 63 */
		}; /* bitfield */
	}; /* size: 0x0008 */
} MIPFNBLINK, * PMIPFNBLINK; /* size: 0x0008 */

typedef struct _MMPFNENTRY1
{
	struct /* bitfield */
	{
		/* 0x0000 */ unsigned char PageLocation : 3; /* bit position: 0 */
		/* 0x0000 */ unsigned char WriteInProgress : 1; /* bit position: 3 */
		/* 0x0000 */ unsigned char Modified : 1; /* bit position: 4 */
		/* 0x0000 */ unsigned char ReadInProgress : 1; /* bit position: 5 */
		/* 0x0000 */ unsigned char CacheAttribute : 2; /* bit position: 6 */
	}; /* bitfield */
} MMPFNENTRY1, * PMMPFNENTRY1; /* size: 0x0001 */

typedef struct _MMPFNENTRY3
{
	struct /* bitfield */
	{
		/* 0x0000 */ unsigned char Priority : 3; /* bit position: 0 */
		/* 0x0000 */ unsigned char OnProtectedStandby : 1; /* bit position: 3 */
		/* 0x0000 */ unsigned char InPageError : 1; /* bit position: 4 */
		/* 0x0000 */ unsigned char SystemChargedPage : 1; /* bit position: 5 */
		/* 0x0000 */ unsigned char RemovalRequested : 1; /* bit position: 6 */
		/* 0x0000 */ unsigned char ParityError : 1; /* bit position: 7 */
	}; /* bitfield */
} MMPFNENTRY3, * PMMPFNENTRY3; /* size: 0x0001 */

typedef struct _MI_PFN_ULONG5
{
	union
	{
		/* 0x0000 */ unsigned long EntireField;
		struct
		{
			struct /* bitfield */
			{
				/* 0x0000 */ unsigned long NodeBlinkHigh : 21; /* bit position: 0 */
				/* 0x0000 */ unsigned long NodeFlinkMiddle : 11; /* bit position: 21 */
			}; /* bitfield */
		} /* size: 0x0004 */ StandbyList;
		struct
		{
			/* 0x0000 */ unsigned char ModifiedListBucketIndex : 4; /* bit position: 0 */
		} /* size: 0x0001 */ MappedPageList;
		struct
		{
			struct /* bitfield */
			{
				/* 0x0000 */ unsigned char AnchorLargePageSize : 2; /* bit position: 0 */
				/* 0x0000 */ unsigned char Spare1 : 6; /* bit position: 2 */
			}; /* bitfield */
			/* 0x0001 */ unsigned char ViewCount;
			/* 0x0002 */ unsigned short Spare2;
		} /* size: 0x0004 */ Active;
	}; /* size: 0x0004 */
} MI_PFN_ULONG5, * PMI_PFN_ULONG5; /* size: 0x0004 */

typedef struct _MMPFN
{
	union
	{
		/* 0x0000 */ struct _LIST_ENTRY ListEntry;
		/* 0x0000 */ struct _RTL_BALANCED_NODE TreeNode;
		struct
		{
			union
			{
				union
				{
					/* 0x0000 */ struct _SINGLE_LIST_ENTRY NextSlistPfn;
					/* 0x0000 */ void* Next;
					struct /* bitfield */
					{
						/* 0x0000 */ unsigned __int64 Flink : 40; /* bit position: 0 */
						/* 0x0000 */ unsigned __int64 NodeFlinkLow : 24; /* bit position: 40 */
					}; /* bitfield */
					/* 0x0000 */ struct _MI_ACTIVE_PFN Active;
				}; /* size: 0x0008 */
			} /* size: 0x0008 */ u1;
			union
			{
				/* 0x0008 */ struct _MMPTE* PteAddress;
				/* 0x0008 */ unsigned __int64 PteLong;
			}; /* size: 0x0008 */
			/* 0x0010 */ struct _MMPTE OriginalPte;
		}; /* size: 0x0018 */
	}; /* size: 0x0018 */
	/* 0x0018 */ struct _MIPFNBLINK u2;
	union
	{
		union
		{
			struct
			{
				/* 0x0020 */ unsigned short ReferenceCount;
				/* 0x0022 */ struct _MMPFNENTRY1 e1;
				/* 0x0023 */ struct _MMPFNENTRY3 e3;
			}; /* size: 0x0004 */
			struct
			{
				/* 0x0020 */ unsigned short ReferenceCount;
			} /* size: 0x0002 */ e2;
			struct
			{
				/* 0x0020 */ unsigned long EntireField;
			} /* size: 0x0004 */ e4;
		}; /* size: 0x0004 */
	} /* size: 0x0004 */ u3;
	/* 0x0024 */ struct _MI_PFN_ULONG5 u5;
	union
	{
		union
		{
			struct /* bitfield */
			{
				/* 0x0028 */ unsigned __int64 PteFrame : 40; /* bit position: 0 */
				/* 0x0028 */ unsigned __int64 ResidentPage : 1; /* bit position: 40 */
				/* 0x0028 */ unsigned __int64 Unused1 : 1; /* bit position: 41 */
				/* 0x0028 */ unsigned __int64 Unused2 : 1; /* bit position: 42 */
				/* 0x0028 */ unsigned __int64 Partition : 10; /* bit position: 43 */
				/* 0x0028 */ unsigned __int64 FileOnly : 1; /* bit position: 53 */
				/* 0x0028 */ unsigned __int64 PfnExists : 1; /* bit position: 54 */
				/* 0x0028 */ unsigned __int64 NodeFlinkHigh : 5; /* bit position: 55 */
				/* 0x0028 */ unsigned __int64 PageIdentity : 3; /* bit position: 60 */
				/* 0x0028 */ unsigned __int64 PrototypePte : 1; /* bit position: 63 */
			}; /* bitfield */
			/* 0x0028 */ unsigned __int64 EntireField;
		}; /* size: 0x0008 */
	} /* size: 0x0008 */ u4;
} MMPFN, * PMMPFN; /* size: 0x0030 */




typedef enum _SYSTEM_INFORMATION_CLASS
{
	SystemBasicInformation = 0x0,
	SystemProcessorInformation = 0x1,
	SystemPerformanceInformation = 0x2,
	SystemTimeOfDayInformation = 0x3,
	SystemPathInformation = 0x4,
	SystemProcessInformation = 0x5,
	SystemCallCountInformation = 0x6,
	SystemDeviceInformation = 0x7,
	SystemProcessorPerformanceInformation = 0x8,
	SystemFlagsInformation = 0x9,
	SystemCallTimeInformation = 0xa,
	SystemModuleInformation = 0xb,
	SystemLocksInformation = 0xc,
	SystemStackTraceInformation = 0xd,
	SystemPagedPoolInformation = 0xe,
	SystemNonPagedPoolInformation = 0xf,
	SystemHandleInformation = 0x10,
	SystemObjectInformation = 0x11,
	SystemPageFileInformation = 0x12,
	SystemVdmInstemulInformation = 0x13,
	SystemVdmBopInformation = 0x14,
	SystemFileCacheInformation = 0x15,
	SystemPoolTagInformation = 0x16,
	SystemInterruptInformation = 0x17,
	SystemDpcBehaviorInformation = 0x18,
	SystemFullMemoryInformation = 0x19,
	SystemLoadGdiDriverInformation = 0x1a,
	SystemUnloadGdiDriverInformation = 0x1b,
	SystemTimeAdjustmentInformation = 0x1c,
	SystemSummaryMemoryInformation = 0x1d,
	SystemMirrorMemoryInformation = 0x1e,
	SystemPerformanceTraceInformation = 0x1f,
	SystemObsolete0 = 0x20,
	SystemExceptionInformation = 0x21,
	SystemCrashDumpStateInformation = 0x22,
	SystemKernelDebuggerInformation = 0x23,
	SystemContextSwitchInformation = 0x24,
	SystemRegistryQuotaInformation = 0x25,
	SystemExtendServiceTableInformation = 0x26,
	SystemPrioritySeperation = 0x27,
	SystemVerifierAddDriverInformation = 0x28,
	SystemVerifierRemoveDriverInformation = 0x29,
	SystemProcessorIdleInformation = 0x2a,
	SystemLegacyDriverInformation = 0x2b,
	SystemCurrentTimeZoneInformation = 0x2c,
	SystemLookasideInformation = 0x2d,
	SystemTimeSlipNotification = 0x2e,
	SystemSessionCreate = 0x2f,
	SystemSessionDetach = 0x30,
	SystemSessionInformation = 0x31,
	SystemRangeStartInformation = 0x32,
	SystemVerifierInformation = 0x33,
	SystemVerifierThunkExtend = 0x34,
	SystemSessionProcessInformation = 0x35,
	SystemLoadGdiDriverInSystemSpace = 0x36,
	SystemNumaProcessorMap = 0x37,
	SystemPrefetcherInformation = 0x38,
	SystemExtendedProcessInformation = 0x39,
	SystemRecommendedSharedDataAlignment = 0x3a,
	SystemComPlusPackage = 0x3b,
	SystemNumaAvailableMemory = 0x3c,
	SystemProcessorPowerInformation = 0x3d,
	SystemEmulationBasicInformation = 0x3e,
	SystemEmulationProcessorInformation = 0x3f,
	SystemExtendedHandleInformation = 0x40,
	SystemLostDelayedWriteInformation = 0x41,
	SystemBigPoolInformation = 0x42,
	SystemSessionPoolTagInformation = 0x43,
	SystemSessionMappedViewInformation = 0x44,
	SystemHotpatchInformation = 0x45,
	SystemObjectSecurityMode = 0x46,
	SystemWatchdogTimerHandler = 0x47,
	SystemWatchdogTimerInformation = 0x48,
	SystemLogicalProcessorInformation = 0x49,
	SystemWow64SharedInformationObsolete = 0x4a,
	SystemRegisterFirmwareTableInformationHandler = 0x4b,
	SystemFirmwareTableInformation = 0x4c,
	SystemModuleInformationEx = 0x4d,
	SystemVerifierTriageInformation = 0x4e,
	SystemSuperfetchInformation = 0x4f,
	SystemMemoryListInformation = 0x50,
	SystemFileCacheInformationEx = 0x51,
	SystemThreadPriorityClientIdInformation = 0x52,
	SystemProcessorIdleCycleTimeInformation = 0x53,
	SystemVerifierCancellationInformation = 0x54,
	SystemProcessorPowerInformationEx = 0x55,
	SystemRefTraceInformation = 0x56,
	SystemSpecialPoolInformation = 0x57,
	SystemProcessIdInformation = 0x58,
	SystemErrorPortInformation = 0x59,
	SystemBootEnvironmentInformation = 0x5a,
	SystemHypervisorInformation = 0x5b,
	SystemVerifierInformationEx = 0x5c,
	SystemTimeZoneInformation = 0x5d,
	SystemImageFileExecutionOptionsInformation = 0x5e,
	SystemCoverageInformation = 0x5f,
	SystemPrefetchPatchInformation = 0x60,
	SystemVerifierFaultsInformation = 0x61,
	SystemSystemPartitionInformation = 0x62,
	SystemSystemDiskInformation = 0x63,
	SystemProcessorPerformanceDistribution = 0x64,
	SystemNumaProximityNodeInformation = 0x65,
	SystemDynamicTimeZoneInformation = 0x66,
	SystemCodeIntegrityInformation = 0x67,
	SystemProcessorMicrocodeUpdateInformation = 0x68,
	SystemProcessorBrandString = 0x69,
	SystemVirtualAddressInformation = 0x6a,
	SystemLogicalProcessorAndGroupInformation = 0x6b,
	SystemProcessorCycleTimeInformation = 0x6c,
	SystemStoreInformation = 0x6d,
	SystemRegistryAppendString = 0x6e,
	SystemAitSamplingValue = 0x6f,
	SystemVhdBootInformation = 0x70,
	SystemCpuQuotaInformation = 0x71,
	SystemNativeBasicInformation = 0x72,
	SystemErrorPortTimeouts = 0x73,
	SystemLowPriorityIoInformation = 0x74,
	SystemBootEntropyInformation = 0x75,
	SystemVerifierCountersInformation = 0x76,
	SystemPagedPoolInformationEx = 0x77,
	SystemSystemPtesInformationEx = 0x78,
	SystemNodeDistanceInformation = 0x79,
	SystemAcpiAuditInformation = 0x7a,
	SystemBasicPerformanceInformation = 0x7b,
	SystemQueryPerformanceCounterInformation = 0x7c,
	SystemSessionBigPoolInformation = 0x7d,
	SystemBootGraphicsInformation = 0x7e,
	SystemScrubPhysicalMemoryInformation = 0x7f,
	SystemBadPageInformation = 0x80,
	SystemProcessorProfileControlArea = 0x81,
	SystemCombinePhysicalMemoryInformation = 0x82,
	SystemEntropyInterruptTimingInformation = 0x83,
	SystemConsoleInformation = 0x84,
	SystemPlatformBinaryInformation = 0x85,
	SystemThrottleNotificationInformation = 0x86,
	SystemHypervisorProcessorCountInformation = 0x87,
	SystemDeviceDataInformation = 0x88,
	SystemDeviceDataEnumerationInformation = 0x89,
	SystemMemoryTopologyInformation = 0x8a,
	SystemMemoryChannelInformation = 0x8b,
	SystemBootLogoInformation = 0x8c,
	SystemProcessorPerformanceInformationEx = 0x8d,
	SystemSpare0 = 0x8e,
	SystemSecureBootPolicyInformation = 0x8f,
	SystemPageFileInformationEx = 0x90,
	SystemSecureBootInformation = 0x91,
	SystemEntropyInterruptTimingRawInformation = 0x92,
	SystemPortableWorkspaceEfiLauncherInformation = 0x93,
	SystemFullProcessInformation = 0x94,
	SystemKernelDebuggerInformationEx = 0x95,
	SystemBootMetadataInformation = 0x96,
	SystemSoftRebootInformation = 0x97,
	SystemElamCertificateInformation = 0x98,
	SystemOfflineDumpConfigInformation = 0x99,
	SystemProcessorFeaturesInformation = 0x9a,
	SystemRegistryReconciliationInformation = 0x9b,
	MaxSystemInfoClass = 0x9c,
} SYSTEM_INFORMATION_CLASS,
* PSYSTEM_INFORMATION_CLASS;
#define SECURITY_FLAG 0x1E2E3F
ULONGLONG m_stored_dtb;
PEPROCESS save_process;

#define PAGE_OFFSET_SIZE 12
static const UINT64 PMASK = (~0xfull << 8) & 0xfffffffffull;
UINT64 cached_pml4e1[512];
typedef struct Image
{
	INT32 Security;
	INT32 ProcessID;
	ULONGLONG* Address;
} Image, * Image_;

typedef struct ReadWrite
{
	INT32 Security;
	INT32 ProcessID;
	ULONGLONG Address;
	ULONGLONG Buffer;
	ULONGLONG Size;
	BOOLEAN Write;
	BOOLEAN EAC;
} ReadWrite, * ReadWrite_;

typedef struct Dirbase
{
	INT32 Security;
	INT32 ProcessID;
	bool* Operation;
} Dirbase, * Dirbase_;

typedef struct DTB
{
	INT32 Security;
	INT32 ProcessID;
	bool* Operation;
	ULONGLONG Address;
} DTB, * DTB_;



typedef struct MOUSE_INIT
{
	INT32 Security;
	INT32 Reserved;       // placeholder for alignment
	ULONGLONG DeviceHandle;
	ULONGLONG Callback;
	BOOLEAN Initialized;
	BOOLEAN EAC;          // optional / placeholder
} MOUSE_INIT, * MOUSE_INIT_;

typedef struct MOUSE_MOVE
{
	INT32 X;
	INT32 Y;
	UINT16 ButtonFlags;
	BOOLEAN EAC;          // optional / placeholder
	ULONGLONG Reserved1;  // alignment
	ULONGLONG Reserved2;  // alignment
} MOUSE_MOVE, * MOUSE_MOVE_;

typedef struct _MOUSE_INPUT_DATA {

	//
	// Unit number.  E.g., for \Device\PointerPort0  the unit is '0',
	// for \Device\PointerPort1 the unit is '1', and so on.
	//

	USHORT UnitId;

	//
	// Indicator flags.
	//

	USHORT Flags;

	//
	// The transition state of the mouse buttons.
	//

	union {
		ULONG Buttons;
		struct {
			USHORT  ButtonFlags;
			USHORT  ButtonData;
		};
	};


	//
	// The raw state of the mouse buttons.
	//

	ULONG RawButtons;

	//
	// The signed relative or absolute motion in the X direction.
	//

	LONG LastX;

	//
	// The signed relative or absolute motion in the Y direction.
	//

	LONG LastY;

	//
	// Device-specific additional information for the event.
	//

	ULONG ExtraInformation;

} MOUSE_INPUT_DATA, * PMOUSE_INPUT_DATA;
typedef struct MOUSE
{
	long X;
	long Y;
	unsigned short ButtonFlags;
}MOUSEREQUEST, * MOUSE_;

typedef VOID
(*MouseClassServiceCallback)(
	PDEVICE_OBJECT DeviceObject,
	PMOUSE_INPUT_DATA InputDataStart,
	PMOUSE_INPUT_DATA InputDataEnd,
	PULONG InputDataConsumed
	);

typedef struct _MOUSE_OBJECT
{
	PDEVICE_OBJECT MouseDevice;
	MouseClassServiceCallback ServiceCallback;
} MOUSE_OBJECT, * PMOUSE_OBJECT;

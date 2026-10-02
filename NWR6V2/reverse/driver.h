#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <thread>
#include <iostream>
#include <functional>
#include <chrono>
#include <mutex>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include "driverdefs.h"

// WinDrvMgr IOCTL codes — must match the driver's IoCodes.h
#define IOCTL_GET_BASE          CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2A2, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define IOCTL_GET_CR3           CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2A3, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define IOCTL_READ_MEMORY       CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2A4, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define IOCTL_WRITE_MEMORY      CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2A5, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define IOCTL_PATTERN_SCAN      CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2A6, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define IOCTL_MOUSE_MOVE        CTL_CODE(FILE_DEVICE_UNKNOWN, 0x3A1, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)

// Legacy IOCTLs kept for compilation — these are NOT supported by WinDrvMgr
#define IOCTL_GET_PEB           CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_ALLOC_MEMORY      CTL_CODE(FILE_DEVICE_UNKNOWN, 0x804, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_FREE_MEMORY       CTL_CODE(FILE_DEVICE_UNKNOWN, 0x805, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_PROTECT_MEMORY    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x806, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_QUERY_PROCESS     CTL_CODE(FILE_DEVICE_UNKNOWN, 0x808, METHOD_BUFFERED, FILE_ANY_ACCESS)

// WinDrvMgr request structs — must match driver's Read.h / Write.h / Base.h
// Default alignment (no pack) — driver doesn't use pack(1), sizeof(base_new)=16
struct KReadRequest    { uint64_t Address; uint64_t Buffer; uint64_t Size; };
struct KWriteRequest   { uint64_t Address; uint64_t Buffer; uint64_t Size; };
struct KBaseRequest    { int32_t  ProcessId; /* 4 bytes padding */ uint64_t BaseAddress; };

// Legacy structs kept for compilation (not used by WinDrvMgr)
struct KPebRequest     { uint32_t ProcessId; uint64_t PebAddress; };
struct KAllocRequest   { uint32_t ProcessId; uint64_t Address; uint64_t Size; uint32_t AllocationType; uint32_t Protect; };
struct KFreeRequest    { uint32_t ProcessId; uint64_t Address; uint64_t Size; uint32_t FreeType; };
struct KProtectRequest { uint32_t ProcessId; uint64_t Address; uint64_t Size; uint32_t NewProtect; uint32_t OldProtect; };
struct KQueryProcessRequest { uint32_t ProcessId; uint64_t Wow64; uint64_t PebAddress; uint64_t MainModuleBase; wchar_t ImagePath[512]; };
struct KPhysRequest    { uint64_t PhysicalAddress; uint64_t Buffer; uint64_t Size; };
struct KApcRequest     { uint32_t ProcessId; uint32_t ThreadId; uint64_t Routine; uint64_t Argument; };
struct KGetThreadRequest { uint32_t ProcessId; uint32_t ThreadId; };
struct KCreateThreadRequest { uint32_t ProcessId; uint64_t StartAddress; uint64_t Argument; uint32_t ThreadId; };
struct KHijackThreadRequest { uint32_t ProcessId; uint32_t ThreadId; uint64_t TrampolineVA; uint64_t OrigRipOffset; uint64_t OutOriginalRip; };

static std::mutex g_driverMtx;

class Driver
{
public:
    UINT ProcessId;

    Driver() : ProcessId(0), m_handle(INVALID_HANDLE_VALUE), m_lastError(0) {}
    ~Driver() { Close(); }

    const bool Init(const BOOL PhysicalMode = FALSE) {
        (void)PhysicalMode;
        printf("[DRV] Init() called, PhysicalMode=%d\n", PhysicalMode);

        auto& nt = GetNtApi();
        printf("[DRV] NtApi initialized: NtCreateFile=%p, NtDeviceIoControlFile=%p\n",
            (void*)nt.fnNtCreateFile, (void*)nt.fnNtDeviceIoControlFile);

        printf("[DRV] Trying NtCreateFile on \\\\Device\\\\WinDrvMgr...\n");
        m_handle = nt.OpenDevice(L"\\Device\\WinDrvMgr");
        printf("[DRV] NtCreateFile result: handle=0x%p\n", m_handle);

        if (m_handle == INVALID_HANDLE_VALUE) {
            printf("[DRV] NtCreateFile failed, trying CreateFileW on \\\\.\\\\WinDrvMgr...\n");
            m_handle = CreateFileW(
                L"\\\\.\\WinDrvMgr",
                GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                nullptr, OPEN_EXISTING, 0, nullptr);
            printf("[DRV] CreateFileW result: handle=0x%p (err=%lu)\n", m_handle, GetLastError());
        }

        bool success = m_handle != INVALID_HANDLE_VALUE;
        printf("[DRV] Init() %s\n", success ? "SUCCESS" : "FAILED");
        return success;
    }

    void Close() {
        if (m_handle != INVALID_HANDLE_VALUE) {
            GetNtApi().fnNtClose(m_handle);
            m_handle = INVALID_HANDLE_VALUE;
        }
    }

    bool IsOpen() const { return m_handle != INVALID_HANDLE_VALUE; }
    DWORD LastError() const { return m_lastError; }

    const bool Attach(DWORD ProcessID) {
        this->ProcessId = ProcessID;
        return true;
    }

    NTSTATUS ReadProcessMemory(uint64_t src, void* dest, uint32_t size) {
        std::scoped_lock lock(g_driverMtx);
        if (m_handle == INVALID_HANDLE_VALUE) return -1;
        // WinDrvMgr read struct: { address, buffer, size }
        // The driver writes directly to 'buffer' (user-mode address)
        KReadRequest req{};
        req.Address = src;
        req.Buffer  = (uint64_t)dest;
        req.Size    = size;
        ULONG ret = 0;
        // Pass outBuf=nullptr — driver writes to req.Buffer directly
        bool ok = Ioctl(IOCTL_READ_MEMORY, &req, (ULONG)sizeof(req), nullptr, 0, &ret);
        m_lastError = ok ? 0 : GetLastError();
        static int readCount = 0;
        static int failCount = 0;
        readCount++;
        if (!ok) failCount++;
        if (readCount <= 5 || (failCount > 0 && readCount <= 20)) {
            printf("[DRV] ReadMemory(PID=%u, addr=0x%llX, sz=%u) = %s (ret=%lu)\n",
                ProcessId, (unsigned long long)src, size, ok ? "OK" : "FAIL", ret);
        }
        return ok ? 0 : -1;
    }

    NTSTATUS WriteProcessMemory(PVOID src, PVOID dest, DWORD size) {
        std::scoped_lock lock(g_driverMtx);
        if (m_handle == INVALID_HANDLE_VALUE) return -1;
        // WinDrvMgr write struct: { address, buffer, size }
        // The driver reads from 'buffer' (user-mode address)
        KWriteRequest req{};
        req.Address = (uint64_t)dest;
        req.Buffer  = (uint64_t)src;
        req.Size    = size;
        ULONG ret = 0;
        bool ok = Ioctl(IOCTL_WRITE_MEMORY, &req, (ULONG)sizeof(req), nullptr, 0, &ret);
        m_lastError = ok ? 0 : GetLastError();
        return ok ? 0 : -1;
    }

    const uint64_t GetModuleBase(const wchar_t* ModuleName = L"", uint64_t* outModSize = nullptr) {
        std::scoped_lock lock(g_driverMtx);
        if (m_handle == INVALID_HANDLE_VALUE) {
            printf("[DRV] GetModuleBase called but driver not open!\n");
            return 0;
        }
        // WinDrvMgr base struct: { process_id, BaseAddress }
        // Driver looks up process by PID, sets TargetPID, returns section base address
        KBaseRequest req{};
        req.ProcessId = (int32_t)ProcessId;
        ULONG ret = 0;
        bool ok = Ioctl(IOCTL_GET_BASE, &req, sizeof(req), &req, sizeof(req), &ret);
        m_lastError = ok ? 0 : GetLastError();
        // WinDrvMgr doesn't return module size — caller must handle
        if (outModSize) *outModSize = 0;
        printf("[DRV] GetModuleBase(PID=%u, module='%ls') = %s, base=0x%llX (ioctl ret=%lu, err=%lu)\n",
            ProcessId, ModuleName ? ModuleName : L"(null)",
            ok ? "OK" : "FAIL",
            (unsigned long long)req.BaseAddress,
            ret, m_lastError);
        return ok ? req.BaseAddress : 0;
    }

    // WinDrvMgr does not support GetPeb — stub that returns 0
    uint64_t GetPeb() {
        printf("[DRV] GetPeb NOT supported by WinDrvMgr driver\n");
        return 0;
    }

    // WinDrvMgr does not support AllocMemory — stub that returns 0
    uint64_t AllocMemory(size_t size = 0,
                         uint32_t allocType = 0,
                         uint32_t protect   = 0,
                         uint64_t base      = 0) {
        (void)size; (void)allocType; (void)protect; (void)base;
        printf("[DRV] AllocMemory NOT supported by WinDrvMgr driver\n");
        return 0;
    }

    // WinDrvMgr does not support FreeMemory — stub that returns false
    bool FreeMemory(uint64_t address = 0, size_t size = 0, uint32_t freeType = 0) {
        (void)address; (void)size; (void)freeType;
        printf("[DRV] FreeMemory NOT supported by WinDrvMgr driver\n");
        return false;
    }

    // WinDrvMgr does not support ProtectMemory — stub that returns failure
    bool ProtectMemory(uint64_t address, size_t size,
                       uint32_t newProtect, uint32_t* oldProtect = nullptr) {
        (void)address; (void)size; (void)newProtect;
        printf("[DRV] ProtectMemory NOT supported by WinDrvMgr driver\n");
        if (oldProtect) *oldProtect = 0;
        return false;
    }

    // WinDrvMgr does not support QueryProcess — stub that returns false
    bool QueryProcess(KQueryProcessRequest& out) {
        (void)out;
        printf("[DRV] QueryProcess NOT supported by WinDrvMgr driver\n");
        return false;
    }

    const UINT GetProcessThreadNumByID(DWORD dwPID)
    {
        HANDLE hProcessSnap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hProcessSnap == INVALID_HANDLE_VALUE)
            return 0;

        PROCESSENTRY32 pe32 = { 0 };
        pe32.dwSize = sizeof(pe32);
        BOOL bRet = ::Process32First(hProcessSnap, &pe32);
        while (bRet)
        {
            if (pe32.th32ProcessID == dwPID)
            {
                ::CloseHandle(hProcessSnap);
                return pe32.cntThreads;
            }
            bRet = ::Process32Next(hProcessSnap, &pe32);
        }
        ::CloseHandle(hProcessSnap);
        return 0;
    }

    NTSTATUS DecryptCR3(uint64_t baseAddress) {
        std::scoped_lock lock(g_driverMtx);
        if (m_handle == INVALID_HANDLE_VALUE) return -1;
        struct { uint64_t BaseAddress; uint64_t OutAddress; } req{};
        req.BaseAddress = baseAddress;
        req.OutAddress = (uint64_t)&m_cr3Value;
        ULONG ret = 0;
        bool ok = Ioctl(IOCTL_GET_CR3, &req, sizeof(req), nullptr, 0, &ret);
        m_lastError = ok ? 0 : GetLastError();
        printf("[DRV] DecryptCR3(base=0x%llX) = %s, cr3=0x%llX\n",
            (unsigned long long)baseAddress, ok ? "OK" : "FAIL",
            (unsigned long long)m_cr3Value);
        return ok ? 0 : -1;
    }

    HANDLE Handle() const { return m_handle; }

private:
    HANDLE m_handle;
    DWORD  m_lastError;
    uint64_t m_cr3Value = 0;

    bool Ioctl(ULONG code,
               void* inBuf, ULONG inLen,
               void* outBuf, ULONG outLen,
               ULONG* ret = nullptr)
    {
        return GetNtApi().Ioctl(m_handle, code, inBuf, inLen, outBuf, outLen, ret);
    }
};

Driver* driver = new Driver;

template <typename T>
T read(const uintptr_t address)
{
    T buffer{};
    driver->ReadProcessMemory(address, &buffer, sizeof(T));
    return buffer;
}

template <typename T>
T write(const uintptr_t address, T buffer)
{
    driver->WriteProcessMemory((PVOID)&buffer, (PVOID)address, sizeof(T));
    return buffer;
}

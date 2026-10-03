#include "include/common.h"
#include "dbg_misc.h"

ULONG h_DbgPrompt(PCCH Prompt, PCH Response, ULONG Length) {
    auto HostPrompt = UcPtr(Prompt);
    Logger::Log("{MAG}\tDbgPrompt: \"%s\"{RESET}\n", HostPrompt ? HostPrompt : "(null)");
    return 0;
}

NTSTATUS h_KdChangeOption(ULONG Option, ULONG InBufferBytes, PVOID InBuffer, ULONG OutBufferBytes, PVOID OutBuffer, PULONG OutBufferNeeded) {
    return 0xC0000354; // STATUS_DEBUGGER_INACTIVE
}

NTSTATUS h_KdSystemDebugControl(int Command, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, PULONG ReturnLength,
    /*KPROCESSOR_MODE*/ int PreviousMode) {
    Logger::Log("{MAG}\tKdSystemDebugControl: cmd=%d inLen=%u outLen=%u{RESET}\n", Command, InputBufferLength, OutputBufferLength);
    auto HostRetLen = UcPtr(ReturnLength);
    if (HostRetLen) *HostRetLen = 0;
    auto HostOutBuf = UcPtr(OutputBuffer);
    if (HostOutBuf && OutputBufferLength > 0)
        memset(HostOutBuf, 0, OutputBufferLength);
    return 0;
}

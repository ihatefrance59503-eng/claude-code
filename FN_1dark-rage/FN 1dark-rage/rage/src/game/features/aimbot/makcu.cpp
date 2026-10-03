#include "makcu.hpp"
#include <cstdio>
#include <string>
#include <mutex>

namespace makcu {

    static HANDLE          g_handle        = INVALID_HANDLE_VALUE;
    static std::string     g_configured_port;   // user override via set_com_port
    static std::mutex      g_mtx;
    static constexpr DWORD BAUD = CBR_115200;

    // --------------------------------------------------------------
    // Low-level: open COM port and configure 115200 8N1 no flow.
    // --------------------------------------------------------------
    static HANDLE open_port(const std::string& port) {
        // "\\\\.\\COMx" form works for COM10+ too
        std::string full = "\\\\.\\" + port;

        HANDLE h = CreateFileA(
            full.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            0,                    // exclusive
            nullptr,
            OPEN_EXISTING,
            0,                    // no FILE_FLAG_OVERLAPPED — synchronous I/O
            nullptr);
        if (h == INVALID_HANDLE_VALUE)
            return INVALID_HANDLE_VALUE;

        DCB dcb = {};
        dcb.DCBlength = sizeof(dcb);
        if (!GetCommState(h, &dcb)) { CloseHandle(h); return INVALID_HANDLE_VALUE; }
        dcb.BaudRate = BAUD;
        dcb.ByteSize = 8;
        dcb.Parity   = NOPARITY;
        dcb.StopBits = ONESTOPBIT;
        dcb.fBinary  = TRUE;
        dcb.fParity  = FALSE;
        dcb.fOutxCtsFlow     = FALSE;
        dcb.fOutxDsrFlow     = FALSE;
        dcb.fDtrControl      = DTR_CONTROL_DISABLE;
        dcb.fRtsControl      = RTS_CONTROL_DISABLE;
        dcb.fOutX = dcb.fInX = FALSE;
        if (!SetCommState(h, &dcb)) { CloseHandle(h); return INVALID_HANDLE_VALUE; }

        COMMTIMEOUTS to = {};
        to.ReadIntervalTimeout         = 50;
        to.ReadTotalTimeoutConstant    = 50;
        to.ReadTotalTimeoutMultiplier  = 10;
        to.WriteTotalTimeoutConstant   = 50;
        to.WriteTotalTimeoutMultiplier = 10;
        SetCommTimeouts(h, &to);

        PurgeComm(h, PURGE_RXCLEAR | PURGE_TXCLEAR);
        return h;
    }

    // --------------------------------------------------------------
    // Write a command string; returns true if all bytes went out.
    // --------------------------------------------------------------
    static bool write_raw(const char* buf, DWORD len) {
        if (g_handle == INVALID_HANDLE_VALUE) return false;
        DWORD written = 0;
        BOOL  ok = WriteFile(g_handle, buf, len, &written, nullptr);
        return ok && written == len;
    }

    static bool write_cmd(const char* fmt, int a, int b) {
        char buf[64];
        int n = sprintf_s(buf, sizeof(buf), fmt, a, b);
        if (n <= 0) return false;
        return write_raw(buf, (DWORD)n);
    }

    static bool write_cmd1(const char* fmt, int a) {
        char buf[64];
        int n = sprintf_s(buf, sizeof(buf), fmt, a);
        if (n <= 0) return false;
        return write_raw(buf, (DWORD)n);
    }

    // --------------------------------------------------------------
    // Probe: send km.version() and look for a known-good response.
    // MAKCU firmware returns something like "KMBOX_NET_v1.9.1\r\n".
    // We just check for "KMBOX" prefix (case-insensitive) or any
    // non-empty line within the read timeout — real MAKCU answers
    // in <50ms, dead ports answer with nothing.
    // --------------------------------------------------------------
    static bool probe(HANDLE h) {
        const char cmd[] = "km.version()\r\n";
        DWORD w = 0;
        if (!WriteFile(h, cmd, (DWORD)(sizeof(cmd) - 1), &w, nullptr))
            return false;

        Sleep(60);

        char  resp[128] = {};
        DWORD r = 0;
        if (!ReadFile(h, resp, sizeof(resp) - 1, &r, nullptr))
            return false;
        if (r < 3)
            return false;

        // uppercase first few chars, check for KMBOX / MAKCU prefix
        for (DWORD i = 0; i < r && i < 8; i++) {
            char c = resp[i];
            if (c >= 'a' && c <= 'z') c -= 32;
            resp[i] = c;
        }
        return (strstr(resp, "KMBOX") != nullptr) ||
               (strstr(resp, "MAKCU") != nullptr) ||
               (strstr(resp, "KM"   ) != nullptr);
    }

    // --------------------------------------------------------------
    // Public API
    // --------------------------------------------------------------
    void set_com_port(const std::string& port) {
        std::lock_guard<std::mutex> lk(g_mtx);
        g_configured_port = port;
    }

    bool initialize() {
        std::lock_guard<std::mutex> lk(g_mtx);

        if (g_handle != INVALID_HANDLE_VALUE)
            return true;

        // 1. explicit override wins
        if (!g_configured_port.empty()) {
            g_handle = open_port(g_configured_port);
            if (g_handle != INVALID_HANDLE_VALUE)
                return true;
        }

        // 2. scan COM1..COM30 and probe each
        for (int i = 1; i <= 30; i++) {
            char port[16];
            sprintf_s(port, sizeof(port), "COM%d", i);
            HANDLE h = open_port(port);
            if (h == INVALID_HANDLE_VALUE) continue;

            if (probe(h)) {
                g_handle = h;
                return true;
            }
            CloseHandle(h);
        }

        return false;
    }

    void shutdown() {
        std::lock_guard<std::mutex> lk(g_mtx);
        if (g_handle != INVALID_HANDLE_VALUE) {
            CloseHandle(g_handle);
            g_handle = INVALID_HANDLE_VALUE;
        }
    }

    bool connected() {
        std::lock_guard<std::mutex> lk(g_mtx);
        return g_handle != INVALID_HANDLE_VALUE;
    }

    void move(int dx, int dy) {
        std::lock_guard<std::mutex> lk(g_mtx);
        write_cmd("km.move(%d,%d)\r\n", dx, dy);
    }

    void left(bool down) {
        std::lock_guard<std::mutex> lk(g_mtx);
        write_cmd1("km.left(%d)\r\n", down ? 1 : 0);
    }

    void right(bool down) {
        std::lock_guard<std::mutex> lk(g_mtx);
        write_cmd1("km.right(%d)\r\n", down ? 1 : 0);
    }

    void middle(bool down) {
        std::lock_guard<std::mutex> lk(g_mtx);
        write_cmd1("km.middle(%d)\r\n", down ? 1 : 0);
    }

    void wheel(int delta) {
        std::lock_guard<std::mutex> lk(g_mtx);
        write_cmd1("km.wheel(%d)\r\n", delta);
    }
}

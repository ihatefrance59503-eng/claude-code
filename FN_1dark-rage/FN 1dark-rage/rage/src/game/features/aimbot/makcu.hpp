#pragma once
#include <windows.h>
#include <string>

// ----------------------------------------------------------------------
// MAKCU — hardware HID mouse controller over USB-serial (CH343).
//
// From Fortnite/EAC's perspective, mouse input from MAKCU is identical
// to a real USB mouse: it enumerates as HID, generates genuine HID
// packets through the kernel input stack, no software-level signatures.
//
// This replaces the kernel-side mouse injection (mouse.hpp — commented
// out) and the previous aimbot write to player_controller rotation.
//
// Protocol (km.net-style text commands, CRLF-terminated):
//   km.move(x,y)   relative mouse move (int, int)
//   km.left(n)     1 = press,  0 = release
//   km.right(n)    same
//   km.middle(n)   same
//   km.wheel(d)    wheel delta
//   km.version()   returns firmware string — used for auto-detect probe
// ----------------------------------------------------------------------

namespace makcu {

    // Auto-detects MAKCU on COM1..COM30, falls back to a configured port.
    // Returns true on success. Idempotent — safe to call again to retry.
    bool initialize();

    // Close serial handle. Idempotent.
    void shutdown();

    // True if connected and the handle is live.
    bool connected();

    // Explicitly set the COM port before initialize() to skip auto-scan.
    void set_com_port(const std::string& port);

    // Relative mouse move via km.move(dx, dy).
    void move(int dx, int dy);

    // Button state via km.left(1/0), km.right(1/0), km.middle(1/0).
    void left(bool down);
    void right(bool down);
    void middle(bool down);

    // Wheel scroll via km.wheel(delta).
    void wheel(int delta);
}

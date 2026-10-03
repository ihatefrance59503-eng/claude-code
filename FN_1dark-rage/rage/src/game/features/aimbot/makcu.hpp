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
    bool initialize();
    void shutdown();
    bool connected();
    void set_com_port(const std::string& port);

    void move(int dx, int dy);
    void left(bool down);
    void right(bool down);
    void middle(bool down);
    void wheel(int delta);
}

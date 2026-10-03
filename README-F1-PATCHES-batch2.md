# F1 batch 2 — safe-ext integration

Four patched files + one new MAKCU module. All writes to Fortnite's address space are now gone from the aimbot path. Aim output is hardware HID via MAKCU.

## pull command

```cmd
cd C:\Users\tbmke\Desktop\F1
git fetch origin f1-patches
git checkout origin/f1-patches -- FN_1driver/nigctl/core/major/major.hpp FN_1driver/nigctl/core/functions/functions.hpp FN_1dark-rage/rage/src/game/features/aimbot/makcu.hpp FN_1dark-rage/rage/src/game/features/aimbot/makcu.cpp FN_1dark-rage/rage/src/game/features/aimbot/aimbot.cpp
```

## files in batch 2

### `FN_1driver/nigctl/core/major/major.hpp`
- **ProbeForRead / ProbeForWrite** on every user pointer before kernel deref. BSOD-from-userland is gone.
- **Caller validation**: `PsGetProcessImageFileName` check against `safe-rage.exe`. The old `0x1E2E3F` magic was decorative. Non-matching callers get `STATUS_ACCESS_DENIED`.
- Whole dispatch body wrapped in `__try/__except`.

### `FN_1driver/nigctl/core/functions/functions.hpp`
- **`read_handler` loops across page boundaries.** Was truncating silently at 4KB — reads crossing a page returned partial data with `STATUS_SUCCESS`. Now iterates until `total_size == 0`.
- **`get_cr3` returns a status explicitly** instead of falling off the end.
- **`write_phyiscal` typo renamed to `write_physical`** throughout.
- `read_physical` pulled out as free function so clean.hpp and read_handler share it.
- Junk-ops stripped.

### `FN_1dark-rage/rage/src/game/features/aimbot/makcu.hpp` (new)
- `initialize()` auto-scans COM1..COM30, probes with `km.version()`.
- `move(dx, dy)`, `left/right/middle(down)`, `wheel(delta)`.
- `set_com_port("COM5")` to pin a port if auto-scan is slow.

### `FN_1dark-rage/rage/src/game/features/aimbot/makcu.cpp` (new)
- Opens `\\.\COMx` at 115200 8N1, synchronous I/O.
- Mutex-guarded writes for multi-thread callers.

### `FN_1dark-rage/rage/src/game/features/aimbot/aimbot.cpp` (rewritten)
- **silent_aim block REMOVED** (writes to CameraManager + weapon aim pitch limits).
- **player_controller rotation write REMOVED** (`player_controller + 0x26f0`).
- All target scoring, FOV, smoothing, weapon tuning preserved.
- Final aim goes through `makcu::move(imx, imy)` instead of writing to Fortnite memory.
- Lazy MAKCU init on first tick, one toast on success/failure.

## verify in KEVLAR before shipping

```cmd
KEVLAR.exe nigctl.sys --diag --modreads > kevlar-batch2.log
```

- No new unmapped-memory / unhandled-exception lines vs batch 1
- `read_handler` paths exercise multiple page chunks on size > 4KB
- `major::io_controller` rejects non-safe-rage callers (test by renaming the usermode exe)

## what's left after batch 2

- `settings.hpp` cleanup (strip `exploits::*` namespace)
- Device name randomization per build (currently fixed `WinKernelInterface`)
- `communcation.hpp` cleanup — drop mouse IOCTLs (dead) and the Write flag (safe-ext never writes)
- `settings::aimbot::makcu_port` field to override auto-scan

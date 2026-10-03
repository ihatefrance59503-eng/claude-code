# F1 patches branch

This branch sits on top of `main` but ADDS files at F1-structure paths.
Use `git checkout` to grab specific files into your local F1 folder.

## Pull commands (run in cmd from Desktop\F1)

```cmd
cd C:\Users\tbmke\Desktop\F1
git init
git remote add origin https://github.com/ihatefrance59503-eng/claude-code.git
git fetch origin f1-patches
git checkout origin/f1-patches -- FN_1driver/nigctl/core/creation/creation.hpp
git checkout origin/f1-patches -- FN_1driver/nigctl/core/clean/clean.hpp
git checkout origin/f1-patches -- FN_1dark-rage/rage/src/render/render.cpp
```

If F1 is already a git repo pointed somewhere else:
```cmd
cd C:\Users\tbmke\Desktop\F1
git remote add patches https://github.com/ihatefrance59503-eng/claude-code.git
git fetch patches f1-patches
git checkout patches/f1-patches -- FN_1driver/nigctl/core/creation/creation.hpp FN_1driver/nigctl/core/clean/clean.hpp FN_1dark-rage/rage/src/render/render.cpp
```

For subsequent batches:
```cmd
git fetch origin f1-patches
git checkout origin/f1-patches -- <path from new batch>
```

## What's in batch 1

- `FN_1driver/nigctl/core/creation/creation.hpp` — KEVLAR_BUILD gate on all four `clean::*` calls, rollback on IoCreateDevice/IoCreateSymbolicLink failure, junk-ops stripped
- `FN_1driver/nigctl/core/clean/clean.hpp` — `CleanMmu` iterative rewrite (was unguarded tail recursion), junk-ops stripped
- `FN_1dark-rage/rage/src/render/render.cpp` — Icecream Screen Recorder as primary overlay target (`Qt5151QWindowIcon` + `recorder.exe` process check via EnumWindows), Discord Overlay fallback

## Build note

Add `KEVLAR_BUILD` to your KEVLAR emu config's preprocessor defines ONLY.
Live/release builds MUST omit it or the stealth cleans won't run.

#pragma once
#include <windows.h>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <tuple>
#include <chrono>
#include "driver.h"

struct AimTarget {
    float x, y, z;
    bool valid;
};

extern bool IsValidAddr(uint64_t p);

// ─── Bone constants ──────────────────────────────────────────────────────────

constexpr uint64_t kBoneStride = 0x20;   // real game: bone entries are 0x20 bytes

enum BoneId {
    BONE_HEAD = 0, BONE_NECK, BONE_SPINE,
    BONE_L_SHOULDER, BONE_L_ELBOW, BONE_L_HAND,
    BONE_R_SHOULDER, BONE_R_ELBOW, BONE_R_HAND,
    BONE_L_HIP, BONE_L_KNEE, BONE_L_ANKLE, BONE_L_FOOT,
    BONE_R_HIP, BONE_R_KNEE, BONE_R_ANKLE, BONE_R_FOOT,
    BONE_COUNT
};

static const std::pair<int, int> kBoneConnections[] = {
    { BONE_HEAD, BONE_NECK }, { BONE_NECK, BONE_SPINE },
    { BONE_NECK, BONE_L_SHOULDER }, { BONE_L_SHOULDER, BONE_L_ELBOW }, { BONE_L_ELBOW, BONE_L_HAND },
    { BONE_NECK, BONE_R_SHOULDER }, { BONE_R_SHOULDER, BONE_R_ELBOW }, { BONE_R_ELBOW, BONE_R_HAND },
    { BONE_SPINE, BONE_L_HIP }, { BONE_L_HIP, BONE_L_KNEE }, { BONE_L_KNEE, BONE_L_ANKLE }, { BONE_L_ANKLE, BONE_L_FOOT },
    { BONE_SPINE, BONE_R_HIP }, { BONE_R_HIP, BONE_R_KNEE }, { BONE_R_KNEE, BONE_R_ANKLE }, { BONE_R_ANKLE, BONE_R_FOOT },
};
constexpr int kNumConnections = (int)(sizeof(kBoneConnections) / sizeof(kBoneConnections[0]));

struct RefBone { int bone; float x, y, z; float radius; };
static const RefBone kReferencePose[] = {
    { BONE_HEAD,         0.00f, 0.00f, 1.62f, 0.22f },
    { BONE_NECK,         0.00f, 0.00f, 1.45f, 0.20f },
    { BONE_SPINE,        0.00f, 0.00f, 1.05f, 0.22f },
    { BONE_L_SHOULDER,  -0.19f, 0.00f, 1.40f, 0.22f },
    { BONE_L_ELBOW,     -0.26f, 0.00f, 1.14f, 0.24f },
    { BONE_L_HAND,      -0.28f, 0.10f, 0.92f, 0.26f },
    { BONE_R_SHOULDER,   0.19f, 0.00f, 1.40f, 0.22f },
    { BONE_R_ELBOW,      0.26f, 0.00f, 1.14f, 0.24f },
    { BONE_R_HAND,       0.28f, 0.10f, 0.92f, 0.26f },
    { BONE_L_HIP,       -0.10f, 0.00f, 0.92f, 0.20f },
    { BONE_L_KNEE,      -0.11f, 0.00f, 0.50f, 0.22f },
    { BONE_L_ANKLE,     -0.11f, 0.00f, 0.10f, 0.20f },
    { BONE_L_FOOT,      -0.11f, 0.12f, 0.03f, 0.20f },
    { BONE_R_HIP,        0.10f, 0.00f, 0.92f, 0.20f },
    { BONE_R_KNEE,       0.11f, 0.00f, 0.50f, 0.22f },
    { BONE_R_ANKLE,      0.11f, 0.00f, 0.10f, 0.20f },
    { BONE_R_FOOT,       0.11f, 0.12f, 0.03f, 0.20f },
};

// Current-era (Aug 2026) semantic bone indices into the 0x40 matrix array,
// from the working UnknownCheats implementation ("sigs work on all qbs").
static const int kCurBoneIds[BONE_COUNT] = {
    0,    // BONE_HEAD
    7,    // BONE_NECK
    28,   // BONE_SPINE
    6,    // BONE_L_SHOULDER
    5,    // BONE_L_ELBOW
    3,    // BONE_L_HAND
    12,   // BONE_R_SHOULDER
    10,   // BONE_R_ELBOW
    9,    // BONE_R_HAND
    23,   // BONE_L_HIP
    20,   // BONE_L_KNEE
    21,   // BONE_L_ANKLE
    19,   // BONE_L_FOOT
    29,   // BONE_R_HIP
    26,   // BONE_R_KNEE
    27,   // BONE_R_ANKLE
    25,   // BONE_R_FOOT
};

// ─── Diagnostics ─────────────────────────────────────────────────────────────

struct SkelDiag {
    uint64_t entity = 0;
    int calls = 0;
    int stage = -1;            // 0 no-ent 1 no-comp 2 scan-fail 3 found
    uint64_t comp = 0;
    uint64_t boneBase = 0;
    int fmt = -1;              // entry format (0..3)
    int count = 0;             // validated bone entries
    int bestRun = 0;           // best consecutive run seen during discovery
    int slot = -1;             // component slot offset that yielded the array
    int rot = -1;              // rotation amount used
    int layoutMode = -1;       // 0=cached 1=fallback-map 2=solved
    int layoutHits = 0;
    float layoutErr = 0.f;
    int bonesOut = 0;
    int healthProbes = 0;
    int healthVal = -1;
};
static SkelDiag g_skelDiag;
static uint64_t g_skelDiagNextPrintUs = 0;

static uint64_t NowMicros() {
    static auto start = std::chrono::high_resolution_clock::now();
    auto now = std::chrono::high_resolution_clock::now();
    return (uint64_t)std::chrono::duration_cast<std::chrono::microseconds>(now - start).count();
}

static void SkelDiagPrint(bool force = false) {
    const uint64_t now = NowMicros();
    if (!force && now < g_skelDiagNextPrintUs) return;
    g_skelDiagNextPrintUs = now + 1000000;  // 1/sec
    const SkelDiag& d = g_skelDiag;
    printf("[SKEL] ent=0x%llX calls=%d stage=%d comp=0x%llX base=0x%llX fmt=%d count=%d bestRun=%d slot=+0x%X rot=%d lay=%d(hits=%d err=%.2f) out=%d hp=%d(probes=%d)\n",
        (unsigned long long)d.entity, d.calls, d.stage,
        (unsigned long long)d.comp, (unsigned long long)d.boneBase, d.fmt, d.count, d.bestRun,
        (unsigned)d.slot, d.rot, d.layoutMode, d.layoutHits, d.layoutErr, d.bonesOut,
        d.healthVal, d.healthProbes);
}

// ─── ReadRaw (wraps driver) ─────────────────────────────────────────────────
// The driver's kernel copy may not survive an unmapped source page, so every
// read is gated on the target's COMMITTED regions (enumerated with
// VirtualQueryEx, rebuilt every few seconds). This keeps the brute-force
// discovery — which decodes pointers out of raw pawn memory — from ever
// handing the driver an address that isn't mapped.
static std::vector<std::pair<uint64_t, uint64_t>> g_committed;   // {base, size}
static uint64_t g_committedBuiltMs = 0;
static std::mutex g_committedMtx;

static void BuildCommittedRanges() {
    extern DWORD processID;
    std::vector<std::pair<uint64_t, uint64_t>> ranges;
    HANDLE h = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, processID);
    if (h) {
        MEMORY_BASIC_INFORMATION mbi = {};
        uintptr_t addr = 0;
        while (VirtualQueryEx(h, (LPCVOID)addr, &mbi, sizeof(mbi))) {
            uintptr_t next = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
            if (mbi.State == MEM_COMMIT && mbi.RegionSize > 0) {
                ranges.push_back({ (uint64_t)(uintptr_t)mbi.BaseAddress, (uint64_t)mbi.RegionSize });
            }
            if (next <= addr) break;
            addr = next;
        }
        CloseHandle(h);
    }
    std::lock_guard<std::mutex> lk(g_committedMtx);
    g_committed = std::move(ranges);
    g_committedBuiltMs = NowMicros() / 1000;
}

// True when [addr, addr+size) lies inside committed target memory.
static bool InCommitted(uint64_t addr, uint64_t size) {
    std::lock_guard<std::mutex> lk(g_committedMtx);
    if (g_committed.empty()) return false;
    if (size == 0) return true;
    const uint64_t end = addr + (uint64_t)size;
    size_t lo = 0, hi = g_committed.size();
    while (lo < hi) {
        size_t mid = (lo + hi) / 2;
        if (g_committed[mid].first + g_committed[mid].second <= addr) lo = mid + 1;
        else hi = mid;
    }
    if (lo >= g_committed.size()) return false;
    const auto& r = g_committed[lo];
    if (addr < r.first) return false;
    uint64_t covered = r.first + r.second;
    if (covered >= end) return true;
    // May span adjacent committed regions (rare); a hole rejects.
    for (size_t i = lo + 1; i < g_committed.size() && covered < end; ++i) {
        if (g_committed[i].first > covered) return false;
        covered = g_committed[i].first + g_committed[i].second;
        if (covered >= end) return true;
    }
    return covered >= end;
}

static bool SkelReadRaw(uint64_t address, void* out, size_t size) {
    if (!address || !IsValidAddr(address)) return false;
    const uint64_t nowMs = NowMicros() / 1000;
    if (g_committed.empty() || nowMs - g_committedBuiltMs > 3000) {
        BuildCommittedRanges();
    }
    if (!InCommitted(address, size)) return false;
    return driver->ReadProcessMemory(address, out, (uint32_t)size) == 0;
}

template <class T> static inline T SkelRead(uint64_t address) {
    T v{};
    SkelReadRaw(address, &v, sizeof(T));
    return v;
}

static inline bool SkelValidPtr(uint64_t p) {
    return p > 0x10000 && p < 0x7FFFFFFFFFFF;
}

static inline uint64_t Rol64(uint64_t v, int c) {
    c &= 63;
    return c ? ((v << c) | (v >> (64 - c))) : v;
}

static inline uint64_t Ror64(uint64_t v, int c) {
    c &= 63;
    return c ? ((v >> c) | (v << (64 - c))) : v;
}

static std::mutex g_skelMutex;

// ─── Health probe ───────────────────────────────────────────────────────────
// R6's health offsets move every patch and are not publicly documented for this
// build; we probe the actor chain and clamp to the game's 0..120 range. If the
// chain misses we fall back to the previous default (100) so the bar still draws.

static int ReadActorHealth(uint64_t pawn) {
    g_skelDiag.healthProbes++;
    if (!SkelValidPtr(pawn)) return 100;
    uint64_t actor = SkelRead<uint64_t>(pawn + 0x18);
    if (!SkelValidPtr(actor)) return 100;
    uint64_t info = SkelRead<uint64_t>(actor + 0xD8);
    if (!SkelValidPtr(info)) return 100;
    uint64_t comp = SkelRead<uint64_t>(info + 0x8);
    if (!SkelValidPtr(comp)) return 100;
    int hp = SkelRead<int>(comp + 0x1BC);
    if (hp < 0) hp = 0;
    if (hp > 120) hp = 100;
    g_skelDiag.healthVal = hp;
    return hp;
}

// ─── Bone array discovery ───────────────────────────────────────────────────
// Reverse-engineered from the game's own skeleton resolver (RVA 0x524E360):
//   comp  = compArr[compIdx]                       (compArr at entity+0xE8, compIdx byte at entity+0x1F9)
//   enc   = [comp + 0x48]                          (encrypted bone-array pointer)
//   base  = rol((enc ^ 0xFC05DFCE63069C47) - 0xD9BBBBF1F8EB5D96, cl)    (cl = per-session rot)
//   bone[i] = [base + 0x20*i]                      (0x20-stride entries)
// Every pointer on this build goes through per-build MBA decryption and the
// layout/rotations move every patch, so instead of hardcoding we brute-force
// the entity's reachable pointer graph with the known decrypt variants and
// accept only a self-validating bone array (>=8 consecutive plausible entries).

enum BoneFmt {
    BFMT_QUATPOS = 0,   // quat(x,y,z,w)@0x00 + pos(x,y,z)@0x10  (0x20 stride)
    BFMT_POSQUAT,       // pos(x,y,z)@0x00 (+ quat/pad after)    (0x20 stride)
    BFMT_POSONLY,       // pos(x,y,z)@0x00, no requirement on the rest (0x20 stride)
    BFMT_MATRIX,        // 4x4 row-major matrix @0x00, translation in row 3 (0x20 stride, legacy guess)
    BFMT_MATRIX40,      // CURRENT ERA: 0x40-stride matrix, pos @+0x30, live-flag 1.0f @+0x3C
    BFMT_COUNT
};

// Stride for a format. Current-era bone data is 0x40-stride matrices.
static int StrideForFmt(int fmt) {
    return (fmt == BFMT_MATRIX40) ? 0x40 : 0x20;
}

// Decrypt constants lifted from the game's skeleton resolver (RVA 0x524E360):
//   boneBase = (ror([comp+0x48], cl) + 0xD9BBBBF1F8EB5D96) ^ 0xFC05DFCE63069C47
// and add-decrypted pointers at [comp+0x10] / [comp+0x18].
static const uint64_t kRorC1 = 0xD9BBBBF1F8EB5D96ULL;   // ror-decrypt add constant
static const uint64_t kRorC2 = 0xFC05DFCE63069C47ULL;   // ror-decrypt xor constant
static const uint64_t kAddDec = 0x4355C7CD1D8CB6E6ULL;  // add-decrypt (comp+0x10)
static const uint64_t kAddDec2 = 0x837E1EAF3E816BD9ULL; // add-decrypt (comp+0x18)

static bool FloatSane(float v, float maxAbs) {
    return std::isfinite(v) && fabsf(v) < maxAbs;
}

// Interpret one 0x20 entry; returns true and fills `out` if it looks like a
// bone position in the given format.
static bool EntryToPos(const uint8_t* buf, int fmt, Vec3& out) {
    float f[16];
    memcpy(f, buf, 64);
    switch (fmt) {
    case BFMT_QUATPOS: {
        // quat at +0x00 (unit-ish), pos at +0x10
        float qx = f[0], qy = f[1], qz = f[2], qw = f[3];
        float len = sqrtf(qx*qx + qy*qy + qz*qz + qw*qw);
        if (!(len > 0.4f && len < 2.5f)) return false;
        if (!FloatSane(f[4], 100000.f) || !FloatSane(f[5], 100000.f) || !FloatSane(f[6], 10000.f)) return false;
        out = { f[4], f[5], f[6] };
        return true;
    }
    case BFMT_POSQUAT: {
        // pos at +0x00, quat at +0x10
        if (!FloatSane(f[0], 100000.f) || !FloatSane(f[1], 100000.f) || !FloatSane(f[2], 10000.f)) return false;
        float qx = f[4], qy = f[5], qz = f[6], qw = f[7];
        float len = sqrtf(qx*qx + qy*qy + qz*qz + qw*qw);
        if (!(len > 0.4f && len < 2.5f)) return false;
        out = { f[0], f[1], f[2] };
        return true;
    }
    case BFMT_POSONLY: {
        if (!FloatSane(f[0], 100000.f) || !FloatSane(f[1], 100000.f) || !FloatSane(f[2], 10000.f)) return false;
        out = { f[0], f[1], f[2] };
        return true;
    }
    case BFMT_MATRIX: {
        // row-major: translation in m[3][0..2], m[3][3] == 1
        if (FloatSane(f[12], 100000.f) && FloatSane(f[13], 100000.f) && FloatSane(f[14], 10000.f) &&
            f[15] > 0.9f && f[15] < 1.1f) {
            out = { f[12], f[13], f[14] };
            return true;
        }
        // column-major: translation in m[0..2][3], m[3][3] == 1
        if (FloatSane(f[3], 100000.f) && FloatSane(f[7], 100000.f) && FloatSane(f[11], 10000.f) &&
            f[15] > 0.9f && f[15] < 1.1f) {
            out = { f[3], f[7], f[11] };
            return true;
        }
        return false;
    }
    case BFMT_MATRIX40: {
        // Current era (2026): 0x40-stride 4x4 matrix; translation at +0x30,
        // live-flag ~1.0f at +0x3C (0.1..1.1 acceptable per the working cheat).
        if (!(FloatSane(f[12], 10000.f) && FloatSane(f[13], 10000.f) && FloatSane(f[14], 10000.f)))
            return false;
        if (!(f[15] > 0.05f && f[15] < 1.15f)) return false;
        out = { f[12], f[13], f[14] };
        return true;
    }
    }
    return false;
}

// Parse `stride`-strided entries from a raw buffer (already read). Returns the
// number of consecutive valid entries. Writes translations into out_pos.
static int ParseBoneBuf(const uint8_t* buf, int maxEntries, int fmt, int stride, Vec3* out_pos, int maxOut) {
    int n = 0;
    for (int i = 0; i < maxEntries; i++) {
        Vec3 p{};
        if (!EntryToPos(buf + (size_t)i * (size_t)stride, fmt, p)) {
            if (i < 4) return 0;   // first few must be valid
            break;
        }
        if (out_pos && n < maxOut) out_pos[n] = p;
        n++;
    }
    // A real rig spans a humanoid height; guard against accidental hits on
    // uniform data (like a color LUT) by requiring some vertical or lateral spread.
    if (n >= 8 && out_pos) {
        float minZ = 1e9f, maxZ = -1e9f, minX = 1e9f, maxX = -1e9f, minY = 1e9f, maxY = -1e9f;
        for (int i = 0; i < n; i++) {
            minZ = fminf(minZ, out_pos[i].z); maxZ = fmaxf(maxZ, out_pos[i].z);
            minX = fminf(minX, out_pos[i].x); maxX = fmaxf(maxX, out_pos[i].x);
            minY = fminf(minY, out_pos[i].y); maxY = fmaxf(maxY, out_pos[i].y);
        }
        float span = fmaxf(maxZ - minZ, fmaxf(maxX - minX, maxY - minY));
        if (span < 0.35f) return 0;
    }
    return n;
}

// Try `base` as a 0x20-strided bone array (single batched driver read).
// Returns the number of consecutive valid entries (0 = not a bone array).
// When `rootHint` is given, the array must be a plausible rig for that actor:
// either world-space positions within ~8m of the root, or character-local
// positions within a few metres of the origin (outLocal reports which). This
// rejects map-geometry / vertex buffers that look like bone arrays otherwise.
// `outRaw` (optional) receives the raw parsed run length regardless of the gate.
static int TryBoneArray(uint64_t base, int fmt, Vec3* out_pos, int maxOut,
                        const Vec3* rootHint, bool* outLocal, int* outRaw) {
    if (outLocal) *outLocal = false;
    if (outRaw) *outRaw = 0;
    if (!SkelValidPtr(base)) return 0;
    // Bone arrays live on the heap; reject module/static ranges.
    if (base >= 0x7FF000000000ULL && base < 0x7FFFFFFFF000ULL) return 0;
    const int stride = StrideForFmt(fmt);
    constexpr int kMaxEntries = 96;
    uint8_t buf[(size_t)kMaxEntries * 0x40];
    if (!SkelReadRaw(base, buf, (size_t)kMaxEntries * (size_t)stride)) return 0;
    // Parse into the caller's buffer, or an internal scratch when they only
    // want the count/validity (e.g. the known-combo fast path).
    Vec3 scratch[96];
    Vec3* pos = out_pos ? out_pos : scratch;
    int maxP = out_pos ? maxOut : kMaxEntries;
    int n = ParseBoneBuf(buf, kMaxEntries, fmt, stride, pos, maxP);
    if (outRaw) *outRaw = n;
    if (n < 4) return 0;

    if (rootHint) {
        // Examine the first up-to-24 entries.
        int m = n < 24 ? n : 24;
        float minX = 1e9f, maxX = -1e9f, minY = 1e9f, maxY = -1e9f, minZ = 1e9f, maxZ = -1e9f;
        for (int i = 0; i < m; i++) {
            minX = fminf(minX, pos[i].x); maxX = fmaxf(maxX, pos[i].x);
            minY = fminf(minY, pos[i].y); maxY = fmaxf(maxY, pos[i].y);
            minZ = fminf(minZ, pos[i].z); maxZ = fmaxf(maxZ, pos[i].z);
        }
        float spanX = maxX - minX, spanY = maxY - minY;
        float maxAbsXY = fmaxf(fmaxf(fabsf(minX), fabsf(maxX)), fmaxf(fabsf(minY), fabsf(maxY)));
        float maxAbsZ = fmaxf(fabsf(minZ), fabsf(maxZ));
        bool localSpace = maxAbsXY < 4.f && maxAbsZ < 4.f;
        // World-space check: centroid of the entries must sit near the actor root.
        float cx = 0.f, cy = 0.f, cz = 0.f;
        for (int i = 0; i < m; i++) { cx += pos[i].x; cy += pos[i].y; cz += pos[i].z; }
        cx /= (float)m; cy /= (float)m; cz /= (float)m;
        float dxy = sqrtf((cx - rootHint->x) * (cx - rootHint->x) + (cy - rootHint->y) * (cy - rootHint->y));
        float dz = fabsf(cz - rootHint->z);
        bool nearRoot = dxy < 8.f && dz < 8.f;
        if (!localSpace && !nearRoot) return 0;
        if (outLocal) *outLocal = localSpace && !nearRoot;
        // A real rig is a spread-out set of points, not a flat line or column.
        if (spanX < 0.2f && spanY < 0.2f) return 0;
    }
    return n;
}

// Decrypt variants for a candidate slot value `v` (from the disassembled
// resolver). Returns the array base candidate, 0 if none looks like a pointer.
static uint64_t DecryptBonePtr(uint64_t v, int& rotOut) {
    // 1) raw pointer
    if (SkelValidPtr(v)) { rotOut = -1; return v; }
    // 2) add-decrypted (enc = base + K)
    uint64_t a = v + kAddDec;
    if (SkelValidPtr(a)) { rotOut = -1; return a; }
    uint64_t b = v - kAddDec;
    if (SkelValidPtr(b)) { rotOut = -1; return b; }
    // 3) ror-decrypt (from the resolver): base = (ror(enc, cl) + C1) ^ C2
    for (int cl = 0; cl < 256; cl++) {
        uint64_t base = (Ror64(v, cl) + kRorC1) ^ kRorC2;
        if (SkelValidPtr(base)) { rotOut = cl; return base; }
    }
    // 4) xor-decrypt with the same constants
    uint64_t c = v ^ kRorC2;
    if (SkelValidPtr(c)) { rotOut = -1; return c; }
    uint64_t d = v ^ kRorC1;
    if (SkelValidPtr(d)) { rotOut = -1; return d; }
    return 0;
}

// Per-entity cached result: boneBase, format, count, slot, rot.
struct BoneChain {
    uint64_t base = 0;
    int fmt = -1;
    int count = 0;
    uint32_t slot = 0;
    int rot = -1;
    bool local = false;    // entries are character-local (need root anchoring)
    bool failed = false;   // scanned and found nothing (cache the failure)
    int stride = 0x20;     // entry stride (0x40 for current-era matrix arrays)
};
static std::unordered_map<uint64_t, BoneChain> g_boneChain;
static std::unordered_map<uint64_t, uint64_t> g_boneScanMs;
// Known-good combo from any entity (all operators share the rig layout): used as
// a fast-path hint for other entities.
static uint32_t g_knownSlot = 0;
static int g_knownRot = -1;
static int g_knownFmt = -1;
static bool g_knownCombo = false;

static BoneChain* GetCachedChain(uint64_t entity) {
    auto it = g_boneChain.find(entity);
    return it == g_boneChain.end() ? nullptr : &it->second;
}

// Read the actor root position (plain +0x50/+0x60, same as ReadActorOrigin).
static bool ReadSkelRoot(uint64_t actor, Vec3& out);

// Fast path: try the known (slot, rot, fmt) combo for this entity's component.
static bool TryKnownCombo(uint64_t comp, const Vec3* root, bool haveRoot, BoneChain& out) {
    if (!g_knownCombo || !SkelValidPtr(comp)) return false;
    uint64_t v = SkelRead<uint64_t>(comp + g_knownSlot);
    if (!v) return false;
    uint64_t base = 0;
    if (g_knownRot >= 0) {
        base = (Ror64(v, g_knownRot) + 0xD9BBBBF1F8EB5D96ULL) ^ 0xFC05DFCE63069C47ULL;
    } else {
        base = v;
    }
    if (!SkelValidPtr(base)) return false;
    bool local = false;
    int n = TryBoneArray(base, g_knownFmt, nullptr, 0, haveRoot ? root : nullptr, &local, nullptr);
    if (n < 8) return false;
    out = { base, g_knownFmt, n, g_knownSlot, g_knownRot, local, false, StrideForFmt(g_knownFmt) };
    return true;
}

// Full discovery for one entity: build the reachable pointer pool, try every
// slot value with every decrypt variant, keep the best self-validating array.
static bool DiscoverBoneChain(uint64_t entity, const SkelXrefInfo& xref, BoneChain& out) {
    out = {};
    if (!SkelValidPtr(entity)) { g_skelDiag.stage = 0; return false; }

    // Resolve the component the way the game does: compArr[compIdx].
    uint64_t comp = 0;
    uint64_t arr = xref.compArrOff ? SkelRead<uint64_t>(entity + xref.compArrOff) : 0;
    if (SkelValidPtr(arr)) {
        uint8_t idx = xref.compIdxOff ? SkelRead<uint8_t>(entity + xref.compIdxOff) : 0xFF;
        if (idx != 0xFF && idx < 64) {
            comp = SkelRead<uint64_t>(arr + (uint64_t)idx * 8);
        }
    }
    g_skelDiag.comp = comp;
    if (!SkelValidPtr(comp)) {
        // Fall back to scanning the entity itself (some rigs put the pointer
        // directly in the pawn object).
        comp = entity;
        g_skelDiag.comp = comp;
        g_skelDiag.stage = 1;
    } else {
        g_skelDiag.stage = 1;
    }

    // Candidate slot pool: qwords from comp, entity, and the comp sub-object
    // (the resolver reads [comp+0x18] and dereferences it too). Deduped so the
    // overlapping windows don't re-test the same (obj, off) pairs.
    uint8_t buf[0x1000];
    std::vector<std::pair<uint64_t, uint64_t>> slots;   // (objectBase, offset)
    std::unordered_set<uint64_t> slotSeen;
    auto addSlots = [&](uint64_t obj, size_t window) {
        if (!SkelValidPtr(obj)) return;
        if (window > sizeof(buf)) window = sizeof(buf);
        if (!SkelReadRaw(obj, buf, window)) return;
        for (size_t i = 0; i < window / 8; i++) {
            uint64_t q = 0;
            memcpy(&q, buf + i * 8, 8);
            if (!q || q == obj) continue;
            const uint64_t key = obj ^ ((uint64_t)i * 8 * 0x9E3779B97F4A7C15ULL);
            if (!slotSeen.insert(key).second) continue;
            slots.push_back({ obj, (uint64_t)i * 8 });
        }
    };
    addSlots(comp, 0x1000);
    if (comp != entity) addSlots(entity, 0x1000);
    // The pawn's animated skeleton data lives DEEP inside the actor object
    // (runtime proof: per-pawn quats at entity+0x104B0). The game writes the
    // per-frame 0x40 bone matrices into the pawn region too (skeleton func
    // RVA 0x524FB30: 0x40-stride matrix write, pos@+0x30, flag@+0x3C=1.0f),
    // so sweep the whole pawn body for pointer slots that decrypt to a
    // valid matrix array. Batched reads, one-time per entity (cached).
    for (uint64_t off = 0x1000; off < 0x14000; off += 0x1000) {
        addSlots(entity + off, 0x1000);
    }

    // Add-decrypted sub-objects the resolver touches ([comp+0x10] / [comp+0x18]
    // are stored as ptr + const on this build).
    uint64_t subA = SkelRead<uint64_t>(comp + 0x10) + kAddDec;
    uint64_t subB = SkelRead<uint64_t>(comp + 0x18) + kAddDec2;
    if (SkelValidPtr(subA) && subA != comp && subA != entity) addSlots(subA, 0x800);
    if (SkelValidPtr(subB) && subB != comp && subB != entity && subB != subA) addSlots(subB, 0x800);

    // Every component in the array (the resolver dispatches on component type,
    // so the bone data may live in a sibling component).
    if (SkelValidPtr(arr)) {
        uint64_t comps[64]{};
        if (SkelReadRaw(arr, comps, sizeof(comps))) {
            for (int i = 0; i < 64; i++) {
                uint64_t c = comps[i];
                if (!SkelValidPtr(c) || c == comp || c == entity) continue;
                addSlots(c, 0x1000);
                uint64_t sb = SkelRead<uint64_t>(c + 0x18) + kAddDec2;
                if (SkelValidPtr(sb) && sb != c) addSlots(sb, 0x1000);
            }
        }
    }

    // One-time calibration probe: what actually sits at the resolver's slot
    // ([comp+0x48] per the dump) and does the ror-decrypt produce anything?
    static bool s_probe2 = false;
    if (!s_probe2) {
        s_probe2 = true;
        printf("[SKEL-PROBE2] comp=0x%llX slots near +0x48:\n", (unsigned long long)comp);
        for (uint32_t o = 0x40; o <= 0x68; o += 8) {
            uint64_t v = SkelRead<uint64_t>(comp + o);
            printf("[SKEL-PROBE2]  +0x%02X = 0x%llX%s\n", o, (unsigned long long)v,
                SkelValidPtr(v) ? "  <-- ptr" : "");
            int bestCl = -1; uint64_t bestBase = 0;
            for (int cl = 0; cl < 256; cl++) {
                uint64_t b = (Ror64(v, cl) + kRorC1) ^ kRorC2;
                if (SkelValidPtr(b)) { bestCl = cl; bestBase = b; break; }
            }
            if (bestCl >= 0) {
                printf("[SKEL-PROBE2]     ror-decrypt cl=%d -> 0x%llX\n", bestCl,
                    (unsigned long long)bestBase);
            }
        }
    }

    Vec3 root{};
    const bool haveRoot = ReadSkelRoot(entity, root);
    {
        static bool s_step = false;
        if (!s_step) { s_step = true;
            printf("[SKEL-STEP] 0/4 discover: comp=0x%llX slots=%zu\n",
                (unsigned long long)comp, slots.size());
        }
    }

    int bestRun = 0;
    int tries = 0;
    // Current-era matrix arrays (0x40 stride, flag @+0x3C) are the real bone
    // data: scan for them FIRST, and prefer any found matrix over every other
    // format (the 0x20 quat arrays are only the game's rotation input).
    const int fmtOrder[BFMT_COUNT] = { BFMT_MATRIX40, BFMT_QUATPOS, BFMT_POSQUAT, BFMT_POSONLY, BFMT_MATRIX };
    BoneChain bestChain40;
    int score40 = 0;
    BoneChain bestChainOther;
    int scoreOther = 0;
    for (const auto& [obj, off] : slots) {
        uint64_t v = SkelRead<uint64_t>(obj + off);
        if (!v) continue;
        int rot = -1;
        uint64_t base = DecryptBonePtr(v, rot);
        if (!base) continue;
        for (int f = 0; f < BFMT_COUNT; f++) {
            if (tries >= 2000) break;  // budget: never let one discovery spin
            tries++;
            const int fmt = fmtOrder[f];
            Vec3 tmp[96];
            bool local = false;
            int raw = 0;
            int n = TryBoneArray(base, fmt, tmp, 96, haveRoot ? &root : nullptr, &local, &raw);
            if (raw > bestRun) bestRun = raw;   // raw run regardless of proximity
            if (n <= 0) continue;
            int score = n * 100 + (local ? 100 : 0) + (fmt == BFMT_MATRIX40 ? 400 : 0);
            if (fmt == BFMT_MATRIX40) {
                if (score > score40) {
                    score40 = score;
                    bestChain40 = { base, fmt, n, (uint32_t)off, rot, local, false, StrideForFmt(fmt) };
                }
            } else if (score > scoreOther) {
                scoreOther = score;
                bestChainOther = { base, fmt, n, (uint32_t)off, rot, local, false, StrideForFmt(fmt) };
            }
        }
    }
    g_skelDiag.bestRun = bestRun;
    // A current-era matrix array wins outright when one validates.
    BoneChain bestChain;
    if (score40 > 0 && bestChain40.count >= 8) {
        bestChain = bestChain40;
    } else if (scoreOther > 0 && bestChainOther.count >= 8) {
        bestChain = bestChainOther;
    } else {
        g_skelDiag.stage = 2;
        return false;
    }
    out = bestChain;
    g_knownSlot = bestChain.slot;
    g_knownRot = bestChain.rot;
    g_knownFmt = bestChain.fmt;
    g_knownCombo = true;
    g_skelDiag.stage = 3;
    g_skelDiag.boneBase = bestChain.base;
    g_skelDiag.fmt = bestChain.fmt;
    g_skelDiag.count = bestChain.count;
    g_skelDiag.slot = (int)bestChain.slot;
    g_skelDiag.rot = bestChain.rot;
    printf("[SKEL] BONE ARRAY FOUND: comp%s+0x%X rot=%d base=0x%llX fmt=%d entries=%d local=%d\n",
        comp != entity ? "" : "=ent", (unsigned)bestChain.slot, bestChain.rot,
        (unsigned long long)bestChain.base, bestChain.fmt, bestChain.count, (int)bestChain.local);
    return true;
}

// One-time calibration dump: raw bytes of the first entries of the accepted
// array so the true entry layout (quat? pos? matrix? header?) can be identified.
static void DumpRawBones(uint64_t base, int fmt) {
    static bool dumped40 = false;
    static bool dumped20 = false;
    if (fmt == BFMT_MATRIX40) { if (dumped40) return; dumped40 = true; }
    else { if (dumped20) return; dumped20 = true; }
    const int stride = StrideForFmt(fmt);
    // fmt=4 entries are 0x40 bytes: print all 16 floats so +0x30 (pos) and
    // +0x3C (flag) are visible. Other formats stay at 8 floats (entry-aligned).
    const int nf = (fmt == BFMT_MATRIX40) ? 16 : 8;
    printf("[BONE-RAW] base=0x%llX fmt=%d stride=0x%X — first 16 entries, %d floats each:\n",
        (unsigned long long)base, fmt, stride, nf);
    for (int i = 0; i < 16; i++) {
        float f[16];
        if (!SkelReadRaw(base + (size_t)i * (size_t)stride, f, sizeof(float) * (size_t)nf)) break;
        printf("[BONE-RAW] %2d:", i);
        for (int j = 0; j < nf; j++) {
            if (j == 8) printf(" |");
            printf(" %12.4f", f[j]);
        }
        printf("\n");
    }
}

// Read the actor root position (plain +0x50/+0x60, same as ReadActorOrigin).
static bool ReadSkelRoot(uint64_t actor, Vec3& out) {
    if (!SkelValidPtr(actor)) return false;
    for (int i = 0; i < 2; i++) {
        Vec3 v = SkelRead<Vec3>(actor + (i ? 0x60 : 0x50));
        if (FloatSane(v.x, 500000.f) && FloatSane(v.y, 500000.f) && FloatSane(v.z, 500000.f) &&
            (fabsf(v.x) > 2.f || fabsf(v.y) > 2.f || fabsf(v.z) > 2.f)) {
            out = v;
            return true;
        }
    }
    return false;
}

// ─── Layout solver: map game bone indices to our BONE_* slots ───────────────

struct SkelLayout { int idx[BONE_COUNT]; uint32_t count; bool swapYZ = false; };
static std::unordered_map<uint64_t, SkelLayout> g_layoutByEntity;
constexpr float kAcceptError = 0.60f;

static void ComputePoseStats(const Vec3* cand, int ncand, Vec3& centroid, float& scale) {
    centroid = { 0.f, 0.f, 0.f };
    scale = 1.f;
    if (ncand <= 0) return;
    for (int i = 0; i < ncand; ++i) {
        centroid.x += cand[i].x; centroid.y += cand[i].y; centroid.z += cand[i].z;
    }
    centroid.x /= (float)ncand; centroid.y /= (float)ncand; centroid.z /= (float)ncand;

    float refSpan = 0.f;
    for (int i = 0; i < BONE_COUNT; ++i) {
        if (kReferencePose[i].bone == BONE_HEAD) {
            for (int j = 0; j < BONE_COUNT; ++j) {
                if (kReferencePose[j].bone == BONE_L_HIP || kReferencePose[j].bone == BONE_R_HIP) {
                    float dx = kReferencePose[i].x - kReferencePose[j].x;
                    float dy = kReferencePose[i].y - kReferencePose[j].y;
                    float dz = kReferencePose[i].z - kReferencePose[j].z;
                    refSpan = sqrtf(dx*dx + dy*dy + dz*dz);
                    break;
                }
            }
            break;
        }
    }
    if (refSpan < 0.1f) return;

    std::vector<float> ratios;
    for (int a = 0; a < ncand; ++a) {
        for (int b = a + 1; b < ncand; ++b) {
            float dx = cand[a].x - cand[b].x;
            float dy = cand[a].y - cand[b].y;
            float dz = cand[a].z - cand[b].z;
            float d = sqrtf(dx*dx + dy*dy + dz*dz);
            if (d < 0.01f) continue;
            ratios.push_back(d / refSpan);
        }
    }
    if (ratios.empty()) return;
    std::sort(ratios.begin(), ratios.end());
    scale = ratios[ratios.size() / 2];
    if (scale < 1e-3f || scale > 1e3f) scale = 1.f;
}

// Axis mapping: the game's per-pawn arrays are y-up and character-centered
// (torso at +y, feet at -y), while kReferencePose is z-up. A rotation-only
// search can never bridge an axis-order mismatch, so we try all 24 proper
// axis-aligned remaps (6 orderings x 4 sign patterns) before the yaw/pitch/
// roll fine-search.
struct AxisMap { int src[3]; float sign[3]; };
static void BuildAxisMaps(AxisMap* out) {
    static const int perms[6][3] = {
        { 0, 1, 2 }, { 0, 2, 1 }, { 1, 0, 2 }, { 1, 2, 0 }, { 2, 0, 1 }, { 2, 1, 0 } };
    static const int parity[6] = { 1, -1, -1, 1, 1, -1 };   // permutation sign
    int n = 0;
    for (int p = 0; p < 6 && n < 24; ++p) {
        for (int s = 0; s < 8 && n < 24; ++s) {
            const float sg[3] = { (s & 1) ? -1.f : 1.f, (s & 2) ? -1.f : 1.f, (s & 4) ? -1.f : 1.f };
            if (sg[0] * sg[1] * sg[2] * parity[p] < 0.f) continue;  // proper rotations only
            AxisMap m;
            for (int i = 0; i < 3; ++i) { m.src[i] = perms[p][i]; m.sign[i] = sg[i]; }
            out[n++] = m;
        }
    }
}

// Try one orientation; returns hits. `idxOut` gets the bone->candidate mapping.
static int SolveOneOrientation(const Vec3* cand, int ncand, const Vec3& centroid,
                               float scale, const AxisMap& am,
                               float yaw, float pitch, float roll,
                               bool flipX, int idxOut[BONE_COUNT], float* errOut) {
    const float cy = cosf(yaw), sy = sinf(yaw);
    const float cp = cosf(pitch), sp = sinf(pitch);
    const float cr = cosf(roll), sr = sinf(roll);

    int idx[BONE_COUNT];
    bool used[256] = {};
    float err = 0.f;
    int hits = 0;
    for (int i = 0; i < BONE_COUNT; ++i) idx[i] = -1;

    for (const RefBone& rb : kReferencePose) {
        const float rv[3] = { rb.x, rb.y, rb.z };
        float rx = am.sign[0] * rv[am.src[0]];
        float ry = am.sign[1] * rv[am.src[1]];
        float rz = am.sign[2] * rv[am.src[2]];
        // roll around z
        float tx = rx * cr - ry * sr; float ty = rx * sr + ry * cr; rx = tx; ry = ty;
        // pitch around x
        ty = ry * cp - rz * sp; float tz = ry * sp + rz * cp; ry = ty; rz = tz;
        // yaw around y (the operator's local frame can face any way)
        tx = rx * cy + rz * sy; tz = -rx * sy + rz * cy; rx = tx; rz = tz;
        if (flipX) rx = -rx;

        const Vec3 want{ centroid.x + rx * scale, centroid.y + ry * scale, centroid.z + rz * scale };
        const float rad = rb.radius * scale;

        int pick = -1;
        float pick_d = rad;
        for (int c = 0; c < ncand && c < 256; ++c) {
            if (used[c]) continue;
            float dx = cand[c].x - want.x, dy = cand[c].y - want.y, dz = cand[c].z - want.z;
            const float d = sqrtf(dx*dx + dy*dy + dz*dz);
            if (d < pick_d) { pick_d = d; pick = c; }
        }
        if (pick < 0) continue;
        used[pick] = true;
        idx[rb.bone] = pick;
        err += pick_d / (scale > 1e-3f ? scale : 1.f);
        ++hits;
    }
    if (errOut) *errOut = err / (hits ? hits : 1);
    memcpy(idxOut, idx, sizeof(idx));
    return hits;
}

static int SolveLayout(const Vec3* cand, int ncand, int out[BONE_COUNT], float& out_err) {
    Vec3 centroid;
    float scale = 1.f;
    ComputePoseStats(cand, ncand, centroid, scale);

    AxisMap maps[24];
    BuildAxisMaps(maps);

    int best_hits = 0;
    float best_err = 1e9f;
    int best[BONE_COUNT]{};

    constexpr int kYaw = 12, kPitch = 3, kRoll = 3;
    static const float kPitchVals[kPitch] = { -0.35f, 0.f, 0.35f };
    static const float kRollVals[kRoll] = { -0.35f, 0.f, 0.35f };
    for (int m = 0; m < 24; ++m) {
        for (int flip = 0; flip < 2; ++flip) {
            for (int q = 0; q < kYaw; ++q) {
                const float yaw = q * (6.2831853f / kYaw);
                for (int p = 0; p < kPitch; ++p) {
                    for (int r = 0; r < kRoll; ++r) {
                        int idx[BONE_COUNT];
                        float err = 0.f;
                        int hits = SolveOneOrientation(cand, ncand, centroid, scale, maps[m],
                                                       yaw, kPitchVals[p], kRollVals[r], flip != 0, idx, &err);
                        if (hits > best_hits || (hits == best_hits && err < best_err)) {
                            best_hits = hits;
                            best_err = err;
                            memcpy(best, idx, sizeof(best));
                            if (hits >= BONE_COUNT && err < 0.2f) goto done;   // perfect fit, stop
                        }
                    }
                }
            }
        }
    }
done:
    memcpy(out, best, sizeof(best));
    out_err = best_err;
    return best_hits;
}

// Cheap geometry sanity check on solved bones: a real rig has the head clearly
// above a foot and roughly stacked vertically. Keeps map geometry / quat
// components-as-positions from ever drawing.
static bool HumanoidGate(const Vec3* w, uint32_t mask) {
    if (!(mask & (1u << BONE_HEAD)) || !(mask & (1u << BONE_L_FOOT))) return false;
    float dz = w[BONE_HEAD].z - w[BONE_L_FOOT].z;
    float dx = w[BONE_HEAD].x - w[BONE_L_FOOT].x;
    float dy = w[BONE_HEAD].y - w[BONE_L_FOOT].y;
    float dxy = sqrtf(dx*dx + dy*dy);
    return dz > 0.5f && dxy < 5.f;
}

// ─── Current-era rig mapping + world placement ──────────────────────────────
// Ported from the working Aug-2026 UnknownCheats skeleton (UC 768419). Bone
// matrix positions are LOCAL (rig origin at the feet, unrotated); we map
// semantic bones to array indices by matching reference poses in local space,
// then rotate the whole rig by the actor's world-rotation quaternion and
// anchor it at the actor root.

struct RotFrame { float m[9]; bool have; };

// Auto-discover the actor's world-rotation quaternion. The working cheat reads
// it from the CHARACTER component (a different compArr entry than the skeleton
// component) around +0x640..0x680 — so we scan the component, every component-
// array entry, and the entity for a unit quaternion in that window. The found
// OFFSET is cached per component so the per-frame cost drops to one read.
static const uint32_t kRotCandOffs[] = { 0x630, 0x638, 0x640, 0x648, 0x650, 0x658,
                                         0x660, 0x668, 0x670, 0x678, 0x680, 0x688 };
static const uint32_t kRotNotSearched = 0xFFFFFFFF;   // sentinel: never scanned yet
// Per-component cache: (object that holds the quat, offset into it).
static std::unordered_map<uint64_t, std::pair<uint64_t, uint32_t>> g_rotByComp;

static bool QuatToFrame(const float* q, RotFrame& F) {
    float len = sqrtf(q[0]*q[0] + q[1]*q[1] + q[2]*q[2] + q[3]*q[3]);
    if (!(len > 0.9f && len < 1.1f)) return false;
    if (!(std::isfinite(q[0]) && std::isfinite(q[1]) && std::isfinite(q[2]) && std::isfinite(q[3]))) return false;
    float qn[4] = { q[0]/len, q[1]/len, q[2]/len, q[3]/len };
    const float xx = qn[0]*qn[0], yy = qn[1]*qn[1], zz = qn[2]*qn[2];
    const float xy = qn[0]*qn[1], xz = qn[0]*qn[2], yz = qn[1]*qn[2];
    const float wx = qn[3]*qn[0], wy = qn[3]*qn[1], wz = qn[3]*qn[2];
    F.m[0] = 1.f - 2.f*(yy + zz); F.m[1] = 2.f*(xy - wz);  F.m[2] = 2.f*(xz + wy);
    F.m[3] = 2.f*(xy + wz);       F.m[4] = 1.f - 2.f*(xx + zz); F.m[5] = 2.f*(yz - wx);
    F.m[6] = 2.f*(xz - wy);       F.m[7] = 2.f*(yz + wx);  F.m[8] = 1.f - 2.f*(xx + yy);
    F.have = true;
    return true;
}

static bool TryObjRot(uint64_t obj, uint32_t& offOut, RotFrame& F) {
    if (!SkelValidPtr(obj)) return false;
    for (uint32_t off : kRotCandOffs) {
        float q[4] = {};
        if (!SkelReadRaw(obj + off, q, sizeof(q))) continue;
        if (QuatToFrame(q, F)) { offOut = off; return true; }
    }
    return false;
}

static RotFrame FindRotFrame(uint64_t comp, uint64_t entity, uint64_t arr) {
    RotFrame F{};
    if (!SkelValidPtr(comp)) comp = entity;
    // Fast path: cached (object, offset) for this component (one read per frame).
    auto it = g_rotByComp.find(comp);
    if (it != g_rotByComp.end()) {
        if (it->second.second == kRotNotSearched) return F;   // scanned, nothing
        float q[4] = {};
        if (SkelReadRaw(it->second.first + it->second.second, q, sizeof(q)) && QuatToFrame(q, F))
            return F;
        g_rotByComp.erase(it);   // went stale — rescan below
    }
    // Slow path: scan the component, every compArr entry, then the entity.
    uint32_t off = 0;
    RotFrame tmp{};
    uint64_t foundObj = 0;
    if (TryObjRot(comp, off, tmp)) { foundObj = comp; F = tmp; }
    if (!foundObj && SkelValidPtr(arr)) {
        for (int i = 0; i < 64; i++) {
            uint64_t c = SkelRead<uint64_t>(arr + (uint64_t)i * 8);
            if (!SkelValidPtr(c) || c == comp || c == entity) continue;
            if (TryObjRot(c, off, tmp)) { foundObj = c; F = tmp; break; }
        }
    }
    if (!foundObj && entity != comp && TryObjRot(entity, off, tmp)) { foundObj = entity; F = tmp; }
    if (!foundObj) {
        g_rotByComp[comp] = { 0, kRotNotSearched };   // remember failure
        return F;
    }
    g_rotByComp[comp] = { foundObj, off };   // quat re-read each frame from this spot
    return F;
}

// 180° flip switch for field calibration (some builds render backwards).
static const bool kSkelYawFlip = false;

static inline Vec3 TransformByRot(const Vec3& p, const RotFrame& F) {
    if (!F.have) return p;
    float x = p.x * F.m[0] + p.y * F.m[1] + p.z * F.m[2];
    float y = p.x * F.m[3] + p.y * F.m[4] + p.z * F.m[5];
    float z = p.x * F.m[6] + p.y * F.m[7] + p.z * F.m[8];
    if (kSkelYawFlip) { x = -x; y = -y; }
    return { x, y, z };
}

// Reference poses (local coords, z-up, feet at z≈0) — from the working cheat.
struct RefPoseBone { float x, y, z; };
static const RefPoseBone kTplStandA[BONE_COUNT] = {
    { 0.005f, 0.154f, 1.602f }, { -0.050f, -0.044f, 1.354f }, { -0.129f, -0.139f, 0.955f },
    { -0.223f, 0.035f, 1.345f }, { -0.344f, 0.132f, 1.066f }, { -0.139f, 0.362f, 1.105f },
    { 0.084f, -0.146f, 1.389f }, { 0.224f, -0.151f, 1.090f }, { 0.181f, 0.166f, 1.046f },
    { -0.202f, -0.142f, 0.884f }, { -0.197f, 0.197f, 0.558f }, { -0.295f, 0.128f, 0.301f },
    { -0.341f, 0.058f, 0.065f }, { -0.075f, -0.204f, 0.904f }, { 0.210f, -0.094f, 0.524f },
    { 0.206f, -0.254f, 0.314f }, { 0.182f, -0.408f, 0.076f },
};
static const RefPoseBone kTplStandRifle[BONE_COUNT] = {
    { 0.00f, 0.16f, 1.60f }, { -0.08f, -0.07f, 1.37f }, { -0.08f, -0.20f, 0.90f },
    { -0.23f, 0.07f, 1.35f }, { -0.15f, 0.31f, 1.12f }, { 0.02f, 0.48f, 1.21f },
    { 0.05f, -0.20f, 1.38f }, { 0.20f, -0.07f, 1.11f }, { 0.11f, 0.25f, 1.18f },
    { -0.21f, 0.01f, 0.75f }, { -0.23f, 0.17f, 0.57f }, { -0.31f, 0.12f, 0.30f },
    { -0.34f, 0.07f, 0.06f }, { 0.05f, -0.17f, 0.74f }, { 0.16f, -0.14f, 0.53f },
    { 0.18f, -0.28f, 0.31f }, { 0.18f, -0.41f, 0.06f },
};
static const RefPoseBone kTplCrouch[BONE_COUNT] = {
    { 0.03f, 0.07f, 1.07f }, { -0.06f, -0.16f, 0.86f }, { -0.129f, -0.312f, 0.473f },
    { -0.21f, -0.04f, 0.80f }, { -0.13f, 0.20f, 0.57f }, { 0.04f, 0.36f, 0.68f },
    { 0.08f, -0.29f, 0.90f }, { 0.18f, -0.21f, 0.59f }, { 0.12f, 0.12f, 0.66f },
    { -0.209f, -0.319f, 0.368f }, { -0.19f, 0.21f, 0.49f }, { -0.23f, 0.02f, 0.28f },
    { -0.25f, -0.14f, 0.06f }, { -0.117f, -0.432f, 0.383f }, { 0.26f, -0.05f, 0.46f },
    { 0.17f, -0.23f, 0.29f }, { 0.06f, -0.39f, 0.06f },
};
static const float kTol[BONE_COUNT] = {
    0.20f, 0.20f, 0.20f, 0.25f, 0.30f, 0.35f, 0.25f, 0.30f, 0.35f,
    0.25f, 0.30f, 0.30f, 0.35f, 0.25f, 0.30f, 0.30f, 0.35f,
};

// Greedy match of one template; returns hits and fills out[] (bone -> index).
// Allocation-free: fixed match buffer (17 refs x 96 candidates max) so this
// hot per-frame path can never throw or allocate.
static int MatchTemplate(const Vec3* cand, int ncand, const RefPoseBone* refs, int out[BONE_COUNT]) {
    struct M { int ri; int ci; float d; };
    M matches[17 * 96];
    int nMatch = 0;
    for (int ri = 0; ri < BONE_COUNT; ri++) {
        for (int c = 0; c < ncand; c++) {
            float dx = cand[c].x - refs[ri].x;
            float dy = cand[c].y - refs[ri].y;
            float dz = cand[c].z - refs[ri].z;
            // Arms: weight y less (large pose variation when aiming).
            float wy = (ri >= BONE_L_SHOULDER && ri <= BONE_R_HAND) ? 0.8f : 1.f;
            float d = sqrtf(dx*dx + dy*dy*wy + dz*dz);
            if (d < kTol[ri] && nMatch < (int)(sizeof(matches) / sizeof(matches[0])))
                matches[nMatch++] = { ri, c, d };
        }
    }
    std::sort(matches, matches + nMatch,
              [](const M& a, const M& b) { return a.d < b.d; });
    bool usedRef[BONE_COUNT] = {}, usedCand[96] = {};
    int tmp[BONE_COUNT];
    for (int i = 0; i < BONE_COUNT; i++) tmp[i] = -1;
    int cnt = 0;
    for (int i = 0; i < nMatch; i++) {
        const M& m = matches[i];
        if (usedRef[m.ri] || usedCand[m.ci]) continue;
        usedRef[m.ri] = true; usedCand[m.ci] = true;
        tmp[m.ri] = m.ci; cnt++;
    }
    memcpy(out, tmp, sizeof(tmp));
    return cnt;
}

// Limb sanity: base -> mid -> tip with sane segment lengths (no folding).
static bool ChainOK(const Vec3* cand, int b, int m, int t, float maxSeg) {
    if (b < 0 || m < 0 || t < 0) return false;
    auto D = [&](int a, int c) {
        float dx = cand[a].x - cand[c].x, dy = cand[a].y - cand[c].y, dz = cand[a].z - cand[c].z;
        return sqrtf(dx*dx + dy*dy + dz*dz);
    };
    float dbm = D(b, m), dmt = D(m, t), dbt = D(b, t);
    if (dbm >= dbt) return false;
    if (dbm > maxSeg || dmt > maxSeg) return false;
    if (dbm + dmt > dbt * 2.2f) return false;
    return true;
}

// Full local-space mapping over all templates + both Y-up/Z-up orientations.
// Returns hits; fills out[]. `cand` is swapped in place when the Y-up
// interpretation wins, so the indices stay valid for the caller.
static int MapRigLayoutLocal(Vec3* cand, int ncand, int out[BONE_COUNT], bool& swapYZ) {
    swapYZ = false;
    const RefPoseBone* kTemplates[3] = { kTplStandA, kTplStandRifle, kTplCrouch };
    const int chains[4][3] = {
        { BONE_L_SHOULDER, BONE_L_ELBOW, BONE_L_HAND },
        { BONE_R_SHOULDER, BONE_R_ELBOW, BONE_R_HAND },
        { BONE_L_HIP, BONE_L_KNEE, BONE_L_ANKLE },
        { BONE_R_HIP, BONE_R_KNEE, BONE_R_ANKLE },
    };
    const float kSeg[4] = { 0.45f, 0.45f, 0.50f, 0.50f };

    auto solveOne = [&](const Vec3* pts, int outTmp[BONE_COUNT]) {
        int h = 0;
        for (int t = 0; t < 3; t++) {
            int tmp[BONE_COUNT];
            int th = MatchTemplate(pts, ncand, kTemplates[t], tmp);
            if (th > h) { h = th; memcpy(outTmp, tmp, sizeof(tmp)); }
        }
        for (int ci = 0; ci < 4; ci++) {
            int b = outTmp[chains[ci][0]], m = outTmp[chains[ci][1]], t = outTmp[chains[ci][2]];
            if (b < 0 || m < 0 || t < 0) continue;
            if (!ChainOK(pts, b, m, t, kSeg[ci])) {
                if (ChainOK(pts, b, t, m, kSeg[ci])) {
                    std::swap(outTmp[chains[ci][1]], outTmp[chains[ci][2]]);
                } else {
                    for (int i = 0; i < BONE_COUNT; i++) outTmp[i] = -1;
                    return 0;
                }
            }
        }
        return h;
    };

    int best[BONE_COUNT];
    int bestHits = solveOne(cand, best);

    Vec3 sw[96];
    for (int i = 0; i < ncand; i++) sw[i] = { cand[i].x, cand[i].z, cand[i].y };
    int bestSw[BONE_COUNT];
    int swHits = solveOne(sw, bestSw);

    if (swHits > bestHits) {
        for (int i = 0; i < ncand; i++) cand[i] = sw[i];
        memcpy(out, bestSw, sizeof(bestSw));
        swapYZ = true;
        return swHits;
    }
    memcpy(out, best, sizeof(best));
    return bestHits;
}

// ─── Main bone reading function ──────────────────────────────────────────────

static bool GetBones(uint64_t entity, uint64_t registryBase,
    const SkelXrefInfo& xref, Vec3 out_world[BONE_COUNT], uint32_t& out_mask) {
    (void)registryBase;
    std::lock_guard<std::mutex> lk(g_skelMutex);
    out_mask = 0;
    g_skelDiag.entity = entity;
    g_skelDiag.calls++;

    if (!SkelValidPtr(entity)) { g_skelDiag.stage = 0; return false; }

    // Resolve the component for the fast path.
    uint64_t comp = 0;
    uint64_t arr = xref.compArrOff ? SkelRead<uint64_t>(entity + xref.compArrOff) : 0;
    if (SkelValidPtr(arr)) {
        uint8_t idx = xref.compIdxOff ? SkelRead<uint8_t>(entity + xref.compIdxOff) : 0xFF;
        if (idx != 0xFF && idx < 64) comp = SkelRead<uint64_t>(arr + (uint64_t)idx * 8);
    }
    if (!SkelValidPtr(comp)) comp = entity;
    g_skelDiag.comp = comp;

    Vec3 root{};
    const bool haveRoot = ReadSkelRoot(entity, root);

    BoneChain* cached = GetCachedChain(entity);
    BoneChain chain;
    if (cached && !cached->failed) {
        chain = *cached;
    } else if (cached && cached->failed) {
        // Failure cached — retry the scan occasionally (the array may not have
        // been created yet when we first looked).
        const uint64_t nowMs = NowMicros() / 1000;
        auto lastIt = g_boneScanMs.find(entity);
        if (lastIt == g_boneScanMs.end() || nowMs - lastIt->second > 30000) {
            g_boneScanMs[entity] = nowMs;
            if (DiscoverBoneChain(entity, xref, chain)) {
                g_boneChain[entity] = chain;
            } else {
                SkelDiagPrint();
                return false;
            }
        } else {
            SkelDiagPrint();
            return false;
        }
    } else {
        // Never scanned: try the fast path first, then discover (throttled).
        const uint64_t nowMs = NowMicros() / 1000;
        auto lastIt = g_boneScanMs.find(entity);
        if (lastIt != g_boneScanMs.end() && nowMs - lastIt->second < 5000) {
            SkelDiagPrint();
            return false;   // throttle: one scan per entity per 2s
        }
        g_boneScanMs[entity] = nowMs;
        if (TryKnownCombo(comp, haveRoot ? &root : nullptr, haveRoot, chain)) {
            g_boneChain[entity] = chain;
        } else if (DiscoverBoneChain(entity, xref, chain)) {
            g_boneChain[entity] = chain;
        } else {
            g_boneChain[entity] = { 0, -1, 0, 0, -1, false, true };   // cache failure
            SkelDiagPrint();
            return false;
        }
    }

    g_skelDiag.boneBase = chain.base;
    g_skelDiag.fmt = chain.fmt;
    g_skelDiag.count = chain.count;
    g_skelDiag.slot = (int)chain.slot;
    g_skelDiag.rot = chain.rot;
    g_skelDiag.stage = 3;

    DumpRawBones(chain.base, chain.fmt);

    // Read the entries (single batched driver read). For current-era matrix
    // arrays the parsed positions are LOCAL (rig origin at the feet).
    Vec3 cand[96];
    bool localOut = false;
    int ncand = TryBoneArray(chain.base, chain.fmt, cand, 96, nullptr, &localOut, nullptr);
    if (ncand < 8) {
        // Array vanished (round transition?) — drop the cache and retry later.
        g_boneChain.erase(entity);
        SkelDiagPrint();
        return false;
    }
    if (ncand > (int)(sizeof(cand) / sizeof(cand[0]))) ncand = (int)(sizeof(cand) / sizeof(cand[0]));
    {
        static bool s_step = false;
        if (!s_step) { s_step = true;
            printf("[SKEL-STEP] 1/4 read ok: ncand=%d fmt=%d local=%d root=(%.1f,%.1f,%.1f)\n",
                ncand, chain.fmt, (int)chain.local, root.x, root.y, root.z);
        }
    }

    // Mapping (bone -> array index), cached per entity.
    int idx[BONE_COUNT];
    auto cachedLayout = g_layoutByEntity.find(entity);
    if (cachedLayout != g_layoutByEntity.end() && cachedLayout->second.count == (uint32_t)ncand) {
        memcpy(idx, cachedLayout->second.idx, sizeof(idx));
        g_skelDiag.layoutMode = 0;
        if (cachedLayout->second.swapYZ) {
            for (int i = 0; i < ncand; i++) cand[i] = { cand[i].x, cand[i].z, cand[i].y };
        }
    } else {
        // Local-space reference-pose matching (current era). `cand` may be
        // swapped in place (Y-up -> Z-up) so the indices stay valid.
        bool swapYZ = false;
        int hits = MapRigLayoutLocal(cand, ncand, idx, swapYZ);
        g_skelDiag.layoutHits = hits;
        if (hits < 6 || idx[BONE_HEAD] < 0) {
            // Fall back to the current-era fixed bone indices.
            g_skelDiag.layoutMode = 1;
            for (int b = 0; b < BONE_COUNT; ++b) {
                int gi = kCurBoneIds[b];
                idx[b] = (gi < ncand) ? gi : -1;
            }
        } else {
            g_skelDiag.layoutMode = 2;
        }
        SkelLayout l; memcpy(l.idx, idx, sizeof(l.idx)); l.count = (uint32_t)ncand; l.swapYZ = swapYZ;
        g_layoutByEntity[entity] = l;
    }
    {
        static bool s_step = false;
        if (!s_step) { s_step = true;
            printf("[SKEL-STEP] 2/4 map ok: mode=%d hits=%d idx0=%d idx7=%d idx28=%d\n",
                g_skelDiag.layoutMode, g_skelDiag.layoutHits, idx[0], idx[1], idx[2]);
        }
    }

    // Rotate the local rig by the actor's world rotation, then anchor at root.
    const RotFrame F = FindRotFrame(comp, entity, arr);
    out_mask = 0;
    for (int b = 0; b < BONE_COUNT; ++b) {
        int gi = idx[b];
        if (gi < 0 || gi >= ncand) continue;
        Vec3 p = cand[gi];
        if (chain.local && haveRoot) {
            const Vec3 r = TransformByRot(p, F);
            p = { r.x + root.x, r.y + root.y, r.z + root.z };
        }
        out_world[b] = p;
        out_mask |= (1u << b);
    }
    g_skelDiag.bonesOut = (int)__popcnt64(out_mask);
    {
        static bool s_step = false;
        if (!s_step) { s_step = true;
            printf("[SKEL-STEP] 3/4 placed ok: mask=%X rot=%d head=(%.1f,%.1f,%.1f)\n",
                (unsigned)out_mask, (int)F.have,
                out_world[BONE_HEAD].x, out_world[BONE_HEAD].y, out_world[BONE_HEAD].z);
        }
    }

    // One-time dump of the mapping, to verify index semantics in-game.
    {
        static bool s_idxDumped = false;
        if (!s_idxDumped && out_mask) {
            s_idxDumped = true;
            printf("[BONE-IDX] ncand=%d mapping:", ncand);
            for (int b = 0; b < BONE_COUNT; ++b) printf(" %d=%d", b, idx[b]);
            printf("\n");
        }
    }

    // Geometry sanity gate: a real rig has the head clearly above a foot and
    // roughly stacked vertically. Rejects garbage so nothing bogus can draw.
    bool sane = false;
    if ((out_mask & (1u << BONE_HEAD)) && (out_mask & (1u << BONE_L_FOOT))) {
        float dz = out_world[BONE_HEAD].z - out_world[BONE_L_FOOT].z;
        float dx = out_world[BONE_HEAD].x - out_world[BONE_L_FOOT].x;
        float dy = out_world[BONE_HEAD].y - out_world[BONE_L_FOOT].y;
        float dxy = sqrtf(dx*dx + dy*dy);
        if (dz > 0.6f && dxy < 4.f) sane = true;
    }
    if (!sane) {
        // Not a humanoid rig — cache the failure so we don't hammer the driver
        // with repeated discoveries (this was the source of the lag).
        g_boneChain[entity] = { 0, -1, 0, 0, -1, false, true };
        g_skelDiag.bonesOut = 0;
        out_mask = 0;
        g_skelDiag.layoutMode = -2;   // rejected by geometry gate
        SkelDiagPrint();
        return false;
    }

    SkelDiagPrint();
    return out_mask != 0;
}

// ─── Simple hitbox fallback (for aimbot when bones unavailable) ──────────────

static AimTarget GetAimPosition(uint64_t entity, float rootX, float rootY, float rootZ, int hitboxSel) {
    AimTarget t = {};
    t.x = rootX;
    t.y = rootY;
    t.valid = true;
    switch (hitboxSel) {
    case 0: t.z = rootZ + 1.6f; break;
    case 1: t.z = rootZ + 1.45f; break;
    case 2: t.z = rootZ + 1.2f; break;
    case 3: t.z = rootZ + 0.85f; break;
    case 4: t.z = rootZ + 0.2f; break;
    default: t.z = rootZ + 1.6f; break;
    }
    return t;
}

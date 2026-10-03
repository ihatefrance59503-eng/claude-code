#pragma once
#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <cstdarg>
#include <cmath>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <chrono>
struct Vec3 { float x, y, z; };

#include "driver.h"
#include "r6_scanner.h"
#include "Imgui/imgui.h"
#include "skeleton_emu.h"
#include "antitamper.h"
#include "operator_esp.h"
struct Matrix4x4 { float m[16]; };
enum class ActorStatus { VALID, DEAD_1, DEAD_2, TEAM, LOCAL, INVALID };

struct OverlayVertex {
    uint64_t instance;
    Vec3 position;
    ActorStatus status;
    bool isPlayer;
    float distance;
    Vec3 screenPos;
    bool onScreen;
    bool hasBones;
    char operatorName[32];
    int  hp;
    uint8_t stencilByte4;
    float lastRenderTime;
};

struct RenderSyncEntry {
    Vec3 position{};
    Vec3 smoothed_position{};
    uint64_t filter_byte = 0;
    std::chrono::steady_clock::time_point last_seen{};
    std::chrono::steady_clock::time_point position_time{};
    bool confirmed_player = false; // set true once IsActiveStencil ever fires
};

extern bool fillbox;
extern float boxThickness;
extern float snaplineThickness;
extern float espBoxColor[4];
extern float espSnaplineColor[4];
extern float espDistanceColor[4];
extern float filledBoxColor[4];
extern float espSkeletonColor[4];
extern float espSkeletonThickness;
extern bool rainbowMode;
extern bool rainbowBox;
extern bool rainbowSnaplines;

extern ImU32 GetBoxColor(float offset);
extern ImU32 GetSnaplineColor(float offset);
extern ImU32 ColorToU32(const float* col);

static FILE* g_log = nullptr;
static void DBG(const char* fmt, ...) {
    if (!g_log) g_log = fopen("C:\\r6_render_calib.log", "w");
    va_list a;
    if (g_log) { va_start(a,fmt); vfprintf(g_log,fmt,a); fprintf(g_log,"\n"); fflush(g_log); va_end(a); }
    va_start(a,fmt); vprintf(fmt,a); printf("\n"); va_end(a);
}

static uint64_t g_projectionAddr = 0;
static uint64_t g_frameSyncAddr = 0;
static uint64_t g_imageBase = 0;
static uint64_t g_ShellPage = 0;
static uint64_t g_RingAddr = 0;
uint64_t g_RegistryBase = 0;
static bool     g_frameSyncActive = false;
static uint8_t  g_OrigBytes[32] = {};
static int      g_PatchLen = 0;
static size_t   g_ShellSize = 0;
static DWORD    g_frameSyncStart = 0;
static bool     g_syncComplete = false;
static constexpr DWORD COLLECT_MS = 499;

static float g_latestRenderTime = 0.0f;

static std::vector<OverlayVertex> g_vertexBuffer;
static std::mutex g_Mtx;
static int g_vtxCount = 0, g_activeVtx = 0;
static std::unordered_set<uint64_t> g_capturedFrames;
static std::mutex g_frameMtx;
static std::atomic<uint64_t> g_totalFrames{0};
static uint64_t g_ReadIdx = 0;
static constexpr size_t RING_SZ = 256;
static uint64_t g_RoundPtr = 0;
static bool g_RoundFound = false;

static DWORD g_lastEntityUpdate = 0;
static constexpr DWORD ENTITY_UPDATE_INTERVAL = 0;


static std::unordered_map<uint64_t, RenderSyncEntry> g_syncMap;
static std::mutex g_syncMapMtx;
static constexpr auto k_syncMaxAge = std::chrono::seconds(30);
static constexpr float k_viewportHeight = 1.72f;
static constexpr float k_minRenderDist = 0.1f;

static bool DrvProtect(uint64_t a, size_t s, uint32_t p, uint32_t* old) {
    HANDLE hP = OpenProcess(PROCESS_ALL_ACCESS, FALSE, driver->ProcessId);
    if (!hP) return false;
    DWORD oldProt = 0;
    BOOL ok = VirtualProtectEx(hP, (LPVOID)a, s, p, &oldProt);
    CloseHandle(hP);
    if (old) *old = oldProt;
    return ok != 0;
}
static bool DrvWriteRaw(const void* src, uint64_t dst, size_t sz) {
    // Try user-mode WriteProcessMemory first (more reliable for shellcode writes)
    HANDLE hP = OpenProcess(PROCESS_ALL_ACCESS, FALSE, driver->ProcessId);
    if (hP) {
        SIZE_T written = 0;
        BOOL ok = WriteProcessMemory(hP, (LPVOID)dst, src, sz, &written);
        CloseHandle(hP);
        if (ok && written == sz) return true;
        printf("[HOOK] WriteProcessMemory failed (err=%lu, written=%zu)\n", GetLastError(), written);
    }
    // Fallback to driver write
    return driver->WriteProcessMemory((PVOID)src, (PVOID)dst, (DWORD)sz) == 0;
}
static bool DrvWriteExec(const void* src, uint64_t dst, size_t sz) {
    uint32_t old = 0;
    DrvProtect(dst, sz, PAGE_EXECUTE_READWRITE, &old);
    bool ok = DrvWriteRaw(src, dst, sz);
    uint32_t tmp = 0;
    DrvProtect(dst, sz, old ? old : PAGE_EXECUTE_READ, &tmp);
    return ok;
}

static int FindBoundary(const uint8_t* c, int minB) {
    int p = 0;
    while (p < 32) {
        uint8_t b = c[p];
        bool rex = (b >= 0x40 && b <= 0x4F);
        if (rex) { p++; b = c[p]; }
        if (b >= 0x50 && b <= 0x5F) { p++; if (p >= minB) return p; continue; }
        if (b == 0x90 || b == 0xCC || b == 0xC3) { p++; if (p >= minB) return p; continue; }
        if (b==0x83||b==0x81||b==0x89||b==0x8B||b==0x8D||b==0x01||b==0x29||
            b==0x31||b==0x33||b==0x39||b==0x3B||b==0x85||b==0x87) {
            uint8_t m = c[p+1]; uint8_t mod = (m>>6)&3; uint8_t rm = m&7;
            p += 2;
            if (mod!=3) { if (rm==4) p++; if (mod==0&&rm==5) p+=4; else if (mod==1) p++; else if (mod==2) p+=4; }
            if (b==0x83) p++; else if (b==0x81) p+=4;
            if (p >= minB) return p; continue;
        }
        if (b == 0x0F) { p++; uint8_t m2 = c[p+1]; uint8_t mod=(m2>>6)&3; uint8_t rm=m2&7;
            p += 2; if (mod!=3) { if(rm==4)p++; if(mod==0&&rm==5)p+=4; else if(mod==1)p++; else if(mod==2)p+=4; }
            if (p >= minB) return p; continue; }
        p++; if (p >= minB) return p;
    }
    return p;
}

static Matrix4x4 QueryProjectionMatrix() { return g_projectionAddr ? read<Matrix4x4>(g_projectionAddr+0x250) : Matrix4x4{}; }
static Vec3 QueryCameraOrigin() { return g_projectionAddr ? read<Vec3>(g_projectionAddr+0x190) : Vec3{}; }
static bool W2S(const Vec3& w, Vec3& s, int W, int H) {
    if (!g_projectionAddr) return false;
    Matrix4x4 v=QueryProjectionMatrix();
    float ww=v.m[3]*w.x+v.m[7]*w.y+v.m[11]*w.z+v.m[15];
    if (ww<0.001f) return false;
    s.x=(W*.5f)*(w.x*v.m[0]+w.y*v.m[4]+w.z*v.m[8]+v.m[12])/ww+W*.5f;
    s.y=-(H*.5f)*(w.x*v.m[1]+w.y*v.m[5]+w.z*v.m[9]+v.m[13])/ww+H*.5f;    s.z=ww; return s.x>=0&&s.y>=0&&s.x<=W&&s.y<=H;
}

static bool IsEntityInFrontOfCamera(const Vec3& entityPos) {
    if (!g_projectionAddr) return true;
    Vec3 cam = QueryCameraOrigin();
    Matrix4x4 v = QueryProjectionMatrix();
    float fwdX = v.m[2], fwdY = v.m[6], fwdZ = v.m[10];
    float dx = entityPos.x - cam.x, dy = entityPos.y - cam.y, dz = entityPos.z - cam.z;
    float dot = dx * fwdX + dy * fwdY + dz * fwdZ;
    return dot > 0.0f;
}


#include "weather_fx.h"

static bool IsValidAddr(uint64_t p){return p>0x10000ULL&&p<0x7FFFFFFFFFFFULL;}

static bool ValidateWorldCoord(Vec3 v){
    if (!std::isfinite(v.x) || !std::isfinite(v.y) || !std::isfinite(v.z))
        return false;
    if(fabsf(v.x)>=500000.f||fabsf(v.y)>=500000.f||fabsf(v.z)>=500000.f)
        return false;
    int s=(fabsf(v.x)>2.f?1:0)+(fabsf(v.y)>2.f?1:0)+(fabsf(v.z)>2.f?1:0);
    return s>=2;
}

static bool ValidatePtr(uint64_t e){
    if(!IsValidAddr(e))return false;if(!IsValidAddr(read<uint64_t>(e)))return false;
    int id=read<int>(e+0x1C);return id>0&&id<1000;
}
static uint64_t ReadStencilBuffer(uint64_t e){if(!IsValidAddr(e))return 0;return read<uint64_t>(e+0xB8);}
static uint8_t StencilByte4(uint64_t fb){return(uint8_t)((fb>>32)&0xFF);}
static uint8_t StencilByte3(uint64_t fb){return(uint8_t)((fb>>24)&0xFF);}
// Stencil class list — a single constant goes stale silently after any update.
// All three are currently live; 0x448/0x548 are the post-update primary classes.
static const uint32_t kPlayerStencils[] = { 0x148, 0x448, 0x548, 0x2C8 };
static bool IsActiveStencil(uint64_t fb) {
    const uint32_t cls = (uint32_t)((fb >> 52) & 0xFFF);
    for (uint32_t s : kPlayerStencils) if (cls == s) return true;
    return false;
}
static bool IsActiveViewport(uint64_t e) { return IsActiveStencil(ReadStencilBuffer(e)); }
static bool IsClearedStencil(uint64_t fb){
    uint8_t b4=StencilByte4(fb);
    return b4==0x84||b4==0x82||b4==0x80;
}
static bool IsStencilCleared(uint64_t e){
    uint64_t fb=ReadStencilBuffer(e);
    if(IsClearedStencil(fb))return true;
    uint64_t bf=read<uint64_t>(e+0xB0);
    if(((bf>>24)&0xFF)==0xFA)return true;
    return false;
}
static bool IsSharedStencil(uint64_t fb){uint8_t b=StencilByte4(fb);return b==0x00||b==0x02;}
static bool IsSharedViewport(uint64_t e){return IsSharedStencil(ReadStencilBuffer(e));}
static bool ValidateDepthStencil(uint64_t fb){
    
    
    
    float fmask = _rcal_get_filter_mask();
    if (fmask < 0.5f) {
        
        return IsSharedStencil(fb) && !IsClearedStencil(fb);
    }
    if(IsClearedStencil(fb))return false;
    if(StencilByte4(fb)==0x00)return false;
    if(IsSharedStencil(fb))return false;
    return true;
}
static bool ValidateStencilMask(uint64_t e){
    return ValidateDepthStencil(ReadStencilBuffer(e));
}

static inline uint64_t mba_dec(uint64_t x) {
    return (x & 0xFFFFFFFFFFFFULL) ^ (0x100010001ULL * ((x >> 48) & 0xFFFFULL));
}

static bool TryPlainPos(uint64_t actor, Vec3& out) {
    // +0xA0 is the confirmed position offset on the current build (probe data Oct 2026)
    // +0x50/0x60 kept as legacy fallback
    uint32_t offsets[] = { 0xA0, 0xA8, 0xB0, 0x50, 0x60 };
    for (uint32_t off : offsets) {
        Vec3 v = read<Vec3>(actor + off);
        if (!ValidateWorldCoord(v)) continue;
        out = v;
        return true;
    }
    // UE4 actor: RootComponent pointer chain -> RelativeLocation
    // actor+0x198 -> USceneComponent; component+0x128 -> world location
    uint32_t rootOffs[] = { 0x198, 0x1A8 };
    uint32_t locOffs[]  = { 0x128, 0x11C, 0x130, 0x140 };
    for (uint32_t ro : rootOffs) {
        uint64_t comp = read<uint64_t>(actor + ro);
        if (!comp || !IsValidAddr(comp)) continue;
        for (uint32_t lo : locOffs) {
            Vec3 v = read<Vec3>(comp + lo);
            if (!ValidateWorldCoord(v)) continue;
            out = v;
            return true;
        }
    }
    return false;
}

static bool TryEncryptedPos(uint64_t actor, Vec3& out) {
    uint32_t roots[] = { 0x20, 0x30 };
    for (int ri = 0; ri < 2; ri++) {
        uint64_t slot = read<uint64_t>(actor + roots[ri]);
        if (!slot || !IsValidAddr(slot)) continue;

        uint64_t val = read<uint64_t>(slot);
        if (!val) continue;

        uint64_t d = 0;
        bool ok = true;
        for (int step = 0; step < 4; ++step) {
            d = mba_dec(val);
            if (step == 3) break;
            if (!d || !IsValidAddr(d)) { ok = false; break; }
            val = read<uint64_t>(d);
            if (!val) { ok = false; break; }
        }
        if (!ok || !d || !IsValidAddr(d)) continue;

        uint32_t node_offsets[] = { 0x30, 0x00 };
        for (int ni = 0; ni < 2; ni++) {
            Vec3 v = read<Vec3>(d + node_offsets[ni]);
            if (!ValidateWorldCoord(v)) continue;
            out = v;
            return true;
        }
    }
    return false;
}

// Visibility check: is entity in front of camera and within FOV?
// This is a necessary condition - entity can't be visible if it's behind the camera
static bool IsEntityInFOV(const Vec3& entityPos, const Vec3& camPos, const Matrix4x4& viewProj, int W, int H) {
    // Check if entity is in front of camera (dot product with camera forward)
    // Camera forward is derived from the view matrix
    Vec3 toEntity = {entityPos.x - camPos.x, entityPos.y - camPos.y, entityPos.z - camPos.z};
    // Camera forward is typically -Z in view space, which maps to columns of view matrix
    float fwdX = viewProj.m[2];
    float fwdY = viewProj.m[6];
    float fwdZ = viewProj.m[10];
    float dot = toEntity.x * fwdX + toEntity.y * fwdY + toEntity.z * fwdZ;
    if (dot < 0.0f) return false; // behind camera
    // Also check if entity projects to screen (already have this info from W2S)
    return true;
}

// ReadActorHealth lives in skeleton_emu.h (reference chain pawn+0x18->+0xD8->+0x8->+0x1BC).

static bool ReadActorOrigin(uint64_t actor, Vec3& out) {
    if (!actor || !IsValidAddr(actor)) return false;

    uint16_t flag_5e = read<uint16_t>(actor + 0x5E);
    uint16_t flag_6e = read<uint16_t>(actor + 0x6E);

    bool enc_5e = flag_5e == 0;
    bool enc_6e = flag_6e == 0;
    bool want_encrypted = enc_5e || enc_6e;
    bool want_plain = (flag_5e != 0) && (flag_6e != 0);

    if (want_encrypted) {
        if (TryEncryptedPos(actor, out)) return true;
        return false;
    }

    if (want_plain) {
        if (TryPlainPos(actor, out)) return true;
    }

    if (TryEncryptedPos(actor, out)) return true;
    return TryPlainPos(actor, out);
}

static Vec3 ResolveViewportOrigin(uint64_t e) {
    Vec3 pos{};
    if (ReadActorOrigin(e, pos))
        return pos;
    return {};
}

static void SyncFrameState(RenderSyncEntry& entry, const Vec3& new_pos, std::chrono::steady_clock::time_point now) {
    if (!ValidateWorldCoord(new_pos))
        return;
    entry.position = new_pos;
    entry.position_time = now;
    entry.smoothed_position = new_pos;
}

static Vec3 InterpolateFrameCoord(RenderSyncEntry& entry, float frame_dt) {
    (void)frame_dt;
    if (!ValidateWorldCoord(entry.position))
        return {};
    return entry.position;
}

static void FlushSyncBuffer() {
    std::lock_guard<std::mutex> lock(g_syncMapMtx);
    g_syncMap.clear();
    FlushShaderSigCache();
}

static int ReadRound() {
    if (!g_RoundPtr) return -1;
    uint64_t b=read<uint64_t>(g_RoundPtr); if(!IsValidAddr(b))return -1;
    uint64_t p1=read<uint64_t>(b+0x40); if(!IsValidAddr(p1))return -1;
    uint64_t p2=read<uint64_t>(p1+0x48); if(!IsValidAddr(p2))return -1;
    uint64_t p3=read<uint64_t>(p2+0x78); if(!IsValidAddr(p3))return -1;
    uint64_t p4=read<uint64_t>(p3+0x18); if(!IsValidAddr(p4))return -1;
    uint64_t p5=read<uint64_t>(p4+0x90); if(!IsValidAddr(p5))return -1;
    uint64_t p6=read<uint64_t>(p5+0x38); if(!IsValidAddr(p6))return -1;
    int st=read<int>(p6+0x348); return (st>=0&&st<=5)?st:-1;
}
static void FindRound() {
    if (g_RoundFound||!g_textCache.valid) return;
    const uint8_t* t=g_textCache.data.data(); size_t sz=(size_t)g_textCache.textSize;
    uint64_t tb=g_textCache.textBase;
    for (size_t i=0;i+15<sz;i++) {
        if(t[i]!=0xE8) continue; if((t[i+12]&0xF0)!=0x40) continue;
        int32_t r=*(int32_t*)&t[i+8]; uint64_t c=tb+i+12+(int64_t)r;
        if(!IsValidAddr(c)) continue;
        uint64_t b=read<uint64_t>(c); if(!b||!IsValidAddr(b)) continue;
        uint64_t p1=read<uint64_t>(b+0x40); if(!IsValidAddr(p1)) continue;
        uint64_t p2=read<uint64_t>(p1+0x48); if(!IsValidAddr(p2)) continue;
        uint64_t p3=read<uint64_t>(p2+0x78); if(!IsValidAddr(p3)) continue;
        uint64_t p4=read<uint64_t>(p3+0x18); if(!IsValidAddr(p4)) continue;
        uint64_t p5=read<uint64_t>(p4+0x90); if(!IsValidAddr(p5)) continue;
        uint64_t p6=read<uint64_t>(p5+0x38); if(!IsValidAddr(p6)) continue;
        int st=read<int>(p6+0x348);
        if(st>=0&&st<=5) { g_RoundPtr=c; g_RoundFound=true; return; }
    }
}

static void PollFrameRing() {
    if (!g_RingAddr) return;
    uint64_t wi=read<uint64_t>(g_RingAddr);
    static uint64_t s_lastWi = 0;
    if (wi != s_lastWi) { printf("[RING] write_idx=%llu (+%llu new)\n", (unsigned long long)wi, (unsigned long long)(wi - s_lastWi)); s_lastWi = wi; }
    int passed=0, badAddr=0, badPtr=0;
    while (g_ReadIdx < wi) {
        uint64_t idx=g_ReadIdx&(RING_SZ-1);
        uint64_t ep=read<uint64_t>(g_RingAddr+0x10+idx*8);
        if (!IsValidAddr(ep)) { badAddr++; }
        else if (!ValidatePtr(ep)) { badPtr++; printf("[RING]   FAIL ValidatePtr ep=0x%llX vtable=0x%llX id=%d\n",(unsigned long long)ep,(unsigned long long)read<uint64_t>(ep),read<int>(ep+0x1C)); }
        else { std::lock_guard<std::mutex> l(g_frameMtx); g_capturedFrames.insert(ep); g_totalFrames++; passed++; }
        g_ReadIdx++;
    }
    if (passed||badAddr||badPtr) printf("[RING] passed=%d badAddr=%d badPtr=%d total_captured=%llu\n", passed,badAddr,badPtr,(unsigned long long)g_totalFrames);
}

static bool AttachFrameSync() {
    if (g_frameSyncActive||!g_frameSyncAddr||!g_ShellPage) { printf("[HOOK] SKIP: active=%d addr=%llX shell=%llX\n", g_frameSyncActive, (unsigned long long)g_frameSyncAddr, (unsigned long long)g_ShellPage); return false; }
    printf("[HOOK] Attaching frame sync... addr=0x%llX shell=0x%llX\n", (unsigned long long)g_frameSyncAddr, (unsigned long long)g_ShellPage);
    for (int i=0;i<32;i++) g_OrigBytes[i]=read<uint8_t>(g_frameSyncAddr+i);
    g_PatchLen = FindBoundary(g_OrigBytes, 14);
    printf("[HOOK] PatchLen=%d\n", g_PatchLen);
    if (g_PatchLen<14||g_PatchLen>30) { printf("[HOOK] BAD PatchLen!\n"); return false; }
    uint8_t sc[80]={}; int p=0;
    sc[p++]=0x50; sc[p++]=0x52;
    sc[p++]=0x48; sc[p++]=0xB8;
    *(uint64_t*)&sc[p]=g_RingAddr; p+=8;
    sc[p++]=0x48; sc[p++]=0x8B; sc[p++]=0x10;
    sc[p++]=0x0F; sc[p++]=0xB6; sc[p++]=0xD2;
    sc[p++]=0x48; sc[p++]=0x89; sc[p++]=0x4C; sc[p++]=0xD0; sc[p++]=0x10;
    sc[p++]=0x48; sc[p++]=0xFF; sc[p++]=0x00;
    sc[p++]=0x5A; sc[p++]=0x58;
    memcpy(&sc[p],g_OrigBytes,g_PatchLen); p+=g_PatchLen;
    sc[p++]=0xFF; sc[p++]=0x25; sc[p++]=0; sc[p++]=0; sc[p++]=0; sc[p++]=0;
    *(uint64_t*)&sc[p]=g_frameSyncAddr+g_PatchLen; p+=8;
    g_ShellSize=p;
    printf("[HOOK] Writing shellcode (%d bytes) to 0x%llX...\n", p, (unsigned long long)g_ShellPage);
    if (!DrvWriteRaw(sc, g_ShellPage, p)) { printf("[HOOK] DrvWriteRaw shellcode FAILED\n"); return false; }
    printf("[HOOK] Shellcode written OK\n");
    uint32_t old=0; DrvProtect(g_ShellPage, 4096, PAGE_EXECUTE_READWRITE, &old);
    printf("[HOOK] DrvProtect result: old=0x%X\n", old);
    uint64_t zero=0; DrvWriteRaw(&zero, g_RingAddr, 8); g_ReadIdx=0;
    printf("[HOOK] Writing hook jump to 0x%llX...\n", (unsigned long long)g_frameSyncAddr);
    uint8_t hook[32]={};
    hook[0]=0xFF; hook[1]=0x25; hook[2]=0; hook[3]=0; hook[4]=0; hook[5]=0;
    *(uint64_t*)&hook[6]=g_ShellPage;
    for(int i=14;i<g_PatchLen;i++) hook[i]=0x90;
    if (!DrvWriteExec(hook, g_frameSyncAddr, g_PatchLen)) { printf("[HOOK] DrvWriteExec hook FAILED\n"); return false; }
    g_frameSyncActive=true; g_frameSyncStart=GetTickCount();
    printf("[HOOK] INSTALLED - .text patched (will restore in %dms)\n", COLLECT_MS);
    return true;
}

static bool DetachFrameSync() {
    if (!g_frameSyncActive) return false;
    if (!DrvWriteExec(g_OrigBytes, g_frameSyncAddr, g_PatchLen)) return false;
    g_frameSyncActive=false;
    printf("[HOOK] REMOVED - .text restored after %dms\n", GetTickCount()-g_frameSyncStart);
return true;
}


static constexpr int TRAIL_MAX_ENTITIES = 32;
static constexpr int TRAIL_MAX_POINTS = 200;
struct TrailBuffer {
    uint64_t entityId;
    Vec3 points[TRAIL_MAX_POINTS];
    int count;
    int writeIdx;
    DWORD lastUpdate;
};
static TrailBuffer g_trailBuffers[TRAIL_MAX_ENTITIES] = {};
static int g_trailBufCount = 0;

static TrailBuffer* AllocTrailBuffer(uint64_t entityId) {
    for (int i = 0; i < g_trailBufCount; i++)
        if (g_trailBuffers[i].entityId == entityId) return &g_trailBuffers[i];
    if (g_trailBufCount < TRAIL_MAX_ENTITIES) {
        TrailBuffer* t = &g_trailBuffers[g_trailBufCount++];
        t->entityId = entityId;
        t->count = 0;
        t->writeIdx = 0;
        t->lastUpdate = 0;
        return t;
    }
    int oldest = 0; DWORD oldestT = UINT_MAX;
    for (int i = 0; i < TRAIL_MAX_ENTITIES; i++)
        if (g_trailBuffers[i].lastUpdate < oldestT) { oldestT = g_trailBuffers[i].lastUpdate; oldest = i; }
    TrailBuffer* t = &g_trailBuffers[oldest];
    t->entityId = entityId;
    t->count = 0;
    t->writeIdx = 0;
    return t;
}

static void AppendTrailSample(TrailBuffer* t, Vec3 pos) {
    extern int trailUpdateMs;
    DWORD now = GetTickCount();
    if (now - t->lastUpdate < (DWORD)trailUpdateMs) return;
    t->lastUpdate = now;
    t->points[t->writeIdx] = pos;
    t->writeIdx = (t->writeIdx + 1) % TRAIL_MAX_POINTS;
    if (t->count < TRAIL_MAX_POINTS) t->count++;
}

static bool InitRenderPipeline(uint64_t base, uint64_t size) {
    g_imageBase=base;
    auto secs=GetPESections(base);
    if(secs.empty()) return false;
    if(!CacheTextSection(base,secs)) return false;
    auto calls=FindEntityFunctionCalls(base);
    for(auto& c:calls) if(c.hasTestAlAl){g_frameSyncAddr=c.targetVA;break;}
    if(!g_frameSyncAddr&&!calls.empty()) g_frameSyncAddr=calls[0].targetVA;
    if(!g_frameSyncAddr) return false;
    HANDLE hProc=OpenProcess(PROCESS_ALL_ACCESS,FALSE,driver->ProcessId);
    if(!hProc) { printf("[!] OpenProcess failed (err=%lu)\n",GetLastError()); return false; }
    g_ShellPage=(uint64_t)VirtualAllocEx(hProc,NULL,8192,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    if(!g_ShellPage) { printf("[!] VirtualAllocEx failed (err=%lu)\n",GetLastError()); CloseHandle(hProc); return false; }
    CloseHandle(hProc);
    // Touch every page via driver write to force page tables to populate
    uint8_t zero[8]={0};
    for(size_t i=0;i<8192;i+=4096) {
        driver->WriteProcessMemory(zero,(PVOID)(g_ShellPage+i),8);
    }
    printf("[+] ShellPage=0x%llX (pages populated)\n",(unsigned long long)g_ShellPage);
    g_RingAddr=g_ShellPage+0x100;
    FindRound();
    g_projectionAddr=ScanForViewTrans(base,size);
    if (ScanSkelXref(base)) {
        printf("[R6] Skeleton xref: compIdx=+0x%X compArr=+0x%X func=0x%llX\n",
            g_SkelXref.compIdxOff, g_SkelXref.compArrOff, (unsigned long long)g_SkelXref.skelFuncVA);
        DumpSkelFuncOffsets(base);   // print the offsets the game's own resolver uses
    } else {
        printf("[R6] Skeleton xref not found\n");
    }
    CreateThread(NULL, 0, [](LPVOID) -> DWORD { ScanSidewards(); return 0; }, NULL, 0, NULL);

    printf("[R6] Position system: ReadActorOrigin (encrypted+plain, driver read)\n");
    return true;
}

static void ShutdownRenderPipeline() {
    RestoreSidewards();
    if(g_frameSyncActive) DetachFrameSync();
    FlushSyncBuffer();
    if(g_log){fclose(g_log);g_log=nullptr;}
}

static void PollSyncBuffer(int W, int H, int maxD) {
    DWORD now_tick = GetTickCount();
    if (now_tick - g_lastEntityUpdate < ENTITY_UPDATE_INTERVAL) return;
    g_lastEntityUpdate = now_tick;
    g_latestRenderTime = 0.0f; // Reset for this frame

    std::lock_guard<std::mutex> lk(g_Mtx);
    g_vertexBuffer.clear(); g_vtxCount = 0; g_activeVtx = 0;

    static DWORD s_posDbg = 0;
    bool posDbg = (now_tick - s_posDbg > 5000);
    if (posDbg) s_posDbg = now_tick;
    if (!g_RoundFound) FindRound();
    int rs = g_RoundFound ? ReadRound() : 3;

    static int s_lastRoundState = -1;
    extern bool sidewardsEnabled;
    extern float sidewardsValue;

    bool isGameplay = (rs == 2 || rs == 3);
    bool wasGameplay = (s_lastRoundState == 2 || s_lastRoundState == 3);
    bool newRound = (isGameplay && !wasGameplay) || (rs == 2 && s_lastRoundState == 3);

    if (newRound) {
        printf("[ROUND] New round (rs=%d, prev=%d), waiting before hook...\\n", rs, s_lastRoundState);
        CreateThread(NULL, 0, [](LPVOID) -> DWORD { ScanSidewards(); return 0; }, NULL, 0, NULL);
        if (sidewardsEnabled && g_Sidewards.found) {
            SetSidewardsValue(sidewardsValue);
        }
        g_syncComplete = false;
        FlushSyncBuffer();
        { std::lock_guard<std::mutex> l(g_frameMtx); g_capturedFrames.clear(); }
    }

    if (!isGameplay && wasGameplay) {
        g_syncComplete = false;
        FlushSyncBuffer();
        { std::lock_guard<std::mutex> l(g_frameMtx); g_capturedFrames.clear(); }
    }

    s_lastRoundState = rs;

    static DWORD s_hookDelayStart = 0;
    static DWORD s_hookDelayMs = 0;
    static DWORD s_lastRecapture = 0;

    if (newRound) {
        s_hookDelayStart = now_tick;
        s_hookDelayMs = 4000 + (rand() % 2001);
        s_lastRecapture = 0;
        printf("[HOOK] Delay %dms before patching\\n", s_hookDelayMs);
    }

    // Periodically re-capture entities (for modes like Terrorist Hunt where enemies respawn)
    if (g_syncComplete && isGameplay && (now_tick - s_lastRecapture > 5000)) {
        g_syncComplete = false;
        s_hookDelayStart = now_tick;
        s_hookDelayMs = 1000;
        s_lastRecapture = now_tick;
    }

    bool delayPassed = (s_hookDelayStart > 0 && (now_tick - s_hookDelayStart) >= s_hookDelayMs);
    if (!g_frameSyncActive && !g_syncComplete && isGameplay && delayPassed) {
        AttachFrameSync();
        s_hookDelayStart = 0;
    }
    if (g_frameSyncActive) { PollFrameRing(); if (GetTickCount() - g_frameSyncStart > COLLECT_MS) { DetachFrameSync(); g_syncComplete = true; } }

    std::vector<uint64_t> cap;
    {
        std::lock_guard<std::mutex> l(g_frameMtx);
        for (auto it = g_capturedFrames.begin(); it != g_capturedFrames.end(); ++it) {
            if (IsValidAddr(*it)) cap.push_back(*it);
        }
    }

    Vec3 cam = QueryCameraOrigin();
    auto now = std::chrono::steady_clock::now();
    float frame_dt = 1.f / 60.f;
    {

        static DWORD s_prevTick = 0;
        if (s_prevTick != 0) {
            float raw = (float)(now_tick - s_prevTick) / 1000.f;
            if (raw > 0.f && raw < 0.25f) frame_dt = raw;
        }
        s_prevTick = now_tick;
    }


    {
        std::lock_guard<std::mutex> clock(g_syncMapMtx);


        static DWORD s_stencilDbg = 0;
        bool stencilDbg = (now_tick - s_stencilDbg > 3000) && !cap.empty();
        if (stencilDbg) s_stencilDbg = now_tick;
        int stPass=0, stFail=0;
        for (uint64_t ea : cap) {
            uint64_t fb = ReadStencilBuffer(ea);
            uint32_t cls = (uint32_t)((fb >> 52) & 0xFFF);
            if (!IsActiveStencil(fb) && !ValidateDepthStencil(fb)) {
                if (stencilDbg) printf("[STENCIL] FAIL ea=0x%llX fb=0x%llX cls=0x%X\n",(unsigned long long)ea,(unsigned long long)fb,cls);
                stFail++;
                auto ex = g_syncMap.find(ea);
                if (ex != g_syncMap.end() && ex->second.confirmed_player) {
                    // Stencil temporarily 0 (game culled from renderer) — keep tracking
                    Vec3 position{};
                    if (ReadActorOrigin(ea, position) && (position.x != 0.f || position.y != 0.f || position.z != 0.f))
                        SyncFrameState(ex->second, position, now);
                    ex->second.last_seen = now;
                } else {
                    g_syncMap.erase(ea);
                }
                continue;
            }
            stPass++;

            auto& entry = g_syncMap[ea];
            if (IsActiveStencil(fb)) entry.confirmed_player = true;
            entry.filter_byte = fb;
            entry.last_seen = now;

            Vec3 position{};
            if (ReadActorOrigin(ea, position))
                SyncFrameState(entry, position, now);
        }


        if (stencilDbg && !cap.empty()) printf("[STENCIL] cap=%zu pass=%d fail=%d syncMap=%zu\n", cap.size(), stPass, stFail, g_syncMap.size());
        for (auto it = g_syncMap.begin(); it != g_syncMap.end(); ) {
            auto age = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second.last_seen);
            if (age > k_syncMaxAge) {
                it = g_syncMap.erase(it);
                continue;
            }
            ++it;
        }


        static DWORD s_renderDbgTick = 0;
        bool renderDbg = (now_tick - s_renderDbgTick > 4000) && !g_syncMap.empty();
        if (renderDbg) s_renderDbgTick = now_tick;

        for (auto it = g_syncMap.begin(); it != g_syncMap.end(); ++it) {
            uint64_t ea = it->first;
            auto& entry = it->second;

            if (!entry.confirmed_player && !IsActiveStencil(entry.filter_byte) && !ValidateDepthStencil(entry.filter_byte)) {
                if (renderDbg) printf("[RENDER-SKIP] ea=0x%llX GATE cp=%d fb=0x%llX\n", (unsigned long long)ea, entry.confirmed_player?1:0, (unsigned long long)entry.filter_byte);
                continue;
            }

            Vec3 draw_pos = entry.position;
            if (!ValidateWorldCoord(draw_pos)) {
                if (renderDbg) {
                    printf("[RENDER-SKIP] ea=0x%llX COORD pos=(%.2f,%.2f,%.2f)\n", (unsigned long long)ea, draw_pos.x, draw_pos.y, draw_pos.z);
                    // probe raw floats around common position offsets to help locate the right one
                    for (uint32_t off : {0x50u, 0x60u, 0x70u, 0x80u, 0x90u, 0xA0u, 0xA8u, 0xB0u, 0x100u, 0x110u, 0x120u, 0x128u, 0x130u, 0x140u}) {
                        Vec3 v = read<Vec3>(ea + off);
                        if (std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z)&&(fabsf(v.x)>2.f||fabsf(v.y)>2.f||fabsf(v.z)>2.f)&&fabsf(v.x)<50000.f&&fabsf(v.y)<50000.f&&fabsf(v.z)<50000.f)
                            printf("[POS-PROBE] ea=0x%llX off=+0x%X (%.1f,%.1f,%.1f)\n", (unsigned long long)ea, off, v.x, v.y, v.z);
                    }
                    // also follow RootComponent pointer chain
                    for (uint32_t ro : {0x198u, 0x1A8u}) {
                        uint64_t comp = read<uint64_t>(ea + ro);
                        if (!comp || !IsValidAddr(comp)) continue;
                        for (uint32_t lo : {0x11Cu, 0x128u, 0x130u, 0x140u, 0x150u}) {
                            Vec3 v = read<Vec3>(comp + lo);
                            if (std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z)&&(fabsf(v.x)>2.f||fabsf(v.y)>2.f||fabsf(v.z)>2.f)&&fabsf(v.x)<50000.f&&fabsf(v.y)<50000.f&&fabsf(v.z)<50000.f)
                                printf("[POS-PROBE] ea=0x%llX root+0x%X comp+0x%X (%.1f,%.1f,%.1f)\n", (unsigned long long)ea, ro, lo, v.x, v.y, v.z);
                        }
                    }
                }
                continue;
            }

            float d = sqrtf(
                (draw_pos.x - cam.x) * (draw_pos.x - cam.x) +
                (draw_pos.y - cam.y) * (draw_pos.y - cam.y) +
                (draw_pos.z - cam.z) * (draw_pos.z - cam.z));

            if (d < k_minRenderDist || d >(float)maxD) {
                if (renderDbg) printf("[RENDER-SKIP] ea=0x%llX DIST d=%.1f\n", (unsigned long long)ea, d);
                continue;
            }

            Vec3 sp = {};
            bool on = W2S(draw_pos, sp, W, H);

            OverlayVertex e = {};
            e.instance = ea;
            e.position = draw_pos;
            e.status = IsClearedStencil(entry.filter_byte) ? ActorStatus::DEAD_1 : ActorStatus::VALID;
            e.isPlayer = IsActiveStencil(entry.filter_byte);
            e.distance = d;
            e.screenPos = sp;
            e.onScreen = on;
            e.hasBones = false;
            e.hp = ReadActorHealth(ea);
            e.stencilByte4 = StencilByte4(entry.filter_byte);
            e.lastRenderTime = 0.0f;
            e.operatorName[0] = '\0';


            const char* opName = ResolveShaderLabel(ea);
            if (opName) {
                strncpy(e.operatorName, opName, sizeof(e.operatorName) - 1);
                e.operatorName[sizeof(e.operatorName) - 1] = '\0';
            }

            if (posDbg && g_vtxCount < 3) {
                printf("[POS] entity=0x%llX pos=(%.1f,%.1f,%.1f) smooth=(%.1f,%.1f,%.1f) dist=%.0f on=%d\n",
                    (unsigned long long)ea,
                    entry.position.x, entry.position.y, entry.position.z,
                    draw_pos.x, draw_pos.y, draw_pos.z,
                    d, on ? 1 : 0);
            }

            // Health calibration probe: print what ReadActorHealth returns (2 sec cadence)
            static DWORD s_hpDbg = 0;
            if (now_tick - s_hpDbg > 2000 && g_vtxCount < 2) {
                s_hpDbg = now_tick;
                printf("[HP] entity=0x%llX stencil=0x%02X hp=%d\n",
                    (unsigned long long)ea, e.stencilByte4, e.hp);
            }

            g_vertexBuffer.push_back(e);
            g_vtxCount++;
            if (e.isPlayer) g_activeVtx++;
        }
    }
}

static void FlushOverlayPipeline(bool box, bool corner, bool line, bool dist, int visDist,
                            bool trail, bool skeleton) {
    int W = GetSystemMetrics(SM_CXSCREEN), H = GetSystemMetrics(SM_CYSCREEN);
    PollSyncBuffer(W, H, visDist);
    std::lock_guard<std::mutex> lk(g_Mtx);
    ImDrawList* dl = ImGui::GetOverlayDrawList();
    if (!dl) return;

    // Integrity gate (antitamper.h): unless the attribution + its render
    // heartbeat + the code hash all verify, ESP and aimbot are disabled.
    if (!_rcal_features_enabled()) {
        box = corner = line = dist = trail = skeleton = false;
    }

    extern float boxThickness;
    extern float snaplineThickness;
    extern float espBoxColor[4];
    extern float espSnaplineColor[4];
    extern float espDistanceColor[4];
    extern float filledBoxColor[4];
    extern float espTrailColor[4];
    extern float trailThickness;
    extern int trailLength;
    extern bool fillbox;
    extern bool lineheadesp;
    extern bool rainbowMode;
    extern bool rainbowBox;
    extern bool rainbowSnaplines;
    extern bool rainbowTrail;

    extern bool espTeamCheck;
    extern bool espDeathCheck;
    for (auto& e : g_vertexBuffer) {
        if (!e.onScreen) continue;
        // Team check: hide teammates from ESP (byte4 0x00=local, 0x02=team)
        if (espTeamCheck && e.isPlayer && (e.stencilByte4 == 0x00 || e.stencilByte4 == 0x02))
            continue;
        float sx = e.screenPos.x, sy = e.screenPos.y;
        float entOffset = (float)(e.instance & 0xFF) / 255.0f;

        ImU32 boxCol = GetBoxColor(entOffset);
        ImU32 snapCol = GetSnaplineColor(entOffset);

        // Head position: prefer the skeleton head bone (exact height incl. crouch).
        // Only attempted when the skeleton feature is on, and the bone-chain discovery
        // is gated so a failed discovery can't hammer the driver every frame.
        Vec3 headPos = {e.position.x, e.position.y, e.position.z + k_viewportHeight};
        if (skeleton) {
            Vec3 bones[BONE_COUNT]{};
            uint32_t mask = 0;
            if (GetBones(e.instance, g_RegistryBase, g_SkelXref, bones, mask) && (mask & (1u << BONE_HEAD))) {
                headPos = bones[BONE_HEAD];
                e.hasBones = true;
            }
        }
        Vec3 headScr = {};
        bool headOn = W2S(headPos, headScr, W, H);

        // Box geometry shared by the box, health bar and labels.
        float boxTop = sy, boxBot = sy, boxLeft = sx, boxRight = sx;
        float boxW = 0.f, boxH = 0.f;
        bool haveBox = false;
        if (headOn) {
            boxH = fabsf(sy - headScr.y);
            if (boxH >= 4.f) {
                boxW = boxH * 0.5f;
                boxTop = fminf(sy, headScr.y);
                boxBot = fmaxf(sy, headScr.y);
                boxLeft = sx - boxW * 0.5f;
                boxRight = sx + boxW * 0.5f;
                haveBox = true;
            }
        }

        if (box && haveBox) {
            const float lw = boxThickness;
            const float olw = lw + 2.0f;                       // outline slightly thicker
            const ImU32 olCol = IM_COL32(0, 0, 0, 200);        // dark outline for contrast

            if (fillbox) {
                dl->AddRectFilled({boxLeft, boxTop}, {boxRight, boxBot}, ColorToU32(filledBoxColor));
            }

            if (corner) {
                const float c = boxW * 0.28f;
                const float cv = boxH * 0.18f;
                // Outline pass
                dl->AddLine({boxLeft, boxTop}, {boxLeft + c, boxTop}, olCol, olw);
                dl->AddLine({boxLeft, boxTop}, {boxLeft, boxTop + cv}, olCol, olw);
                dl->AddLine({boxRight, boxTop}, {boxRight - c, boxTop}, olCol, olw);
                dl->AddLine({boxRight, boxTop}, {boxRight, boxTop + cv}, olCol, olw);
                dl->AddLine({boxLeft, boxBot}, {boxLeft + c, boxBot}, olCol, olw);
                dl->AddLine({boxLeft, boxBot}, {boxLeft, boxBot - cv}, olCol, olw);
                dl->AddLine({boxRight, boxBot}, {boxRight - c, boxBot}, olCol, olw);
                dl->AddLine({boxRight, boxBot}, {boxRight, boxBot - cv}, olCol, olw);
                // Colour pass
                dl->AddLine({boxLeft, boxTop}, {boxLeft + c, boxTop}, boxCol, lw);
                dl->AddLine({boxLeft, boxTop}, {boxLeft, boxTop + cv}, boxCol, lw);
                dl->AddLine({boxRight, boxTop}, {boxRight - c, boxTop}, boxCol, lw);
                dl->AddLine({boxRight, boxTop}, {boxRight, boxTop + cv}, boxCol, lw);
                dl->AddLine({boxLeft, boxBot}, {boxLeft + c, boxBot}, boxCol, lw);
                dl->AddLine({boxLeft, boxBot}, {boxLeft, boxBot - cv}, boxCol, lw);
                dl->AddLine({boxRight, boxBot}, {boxRight - c, boxBot}, boxCol, lw);
                dl->AddLine({boxRight, boxBot}, {boxRight, boxBot - cv}, boxCol, lw);
            } else {
                dl->AddRect({boxLeft, boxTop}, {boxRight, boxBot}, olCol, 0, 0, olw);
                dl->AddRect({boxLeft, boxTop}, {boxRight, boxBot}, boxCol, 0, 0, lw);
            }
        }

        if (line) {
            extern int snaplineOrigin;
            ImVec2 from;
            if (snaplineOrigin == 0) from = ImVec2((float)W/2, 0);           
            else if (snaplineOrigin == 1) from = ImVec2((float)W/2, (float)H/2); 
            else from = ImVec2((float)W/2, (float)H);                        
            dl->AddLine(from, {sx, sy}, snapCol, snaplineThickness);
        }

        if (lineheadesp && headOn) {
            dl->AddLine({sx, sy}, {(float)headScr.x, (float)headScr.y}, IM_COL32(255,255,0,200), 1.0f);
        }

        if (dist) {
            char dt[32]; snprintf(dt, 32, "%.0fm", e.distance);
            const float tx = haveBox ? boxRight + 6.0f : sx - 10.0f;
            const float ty = haveBox ? boxBot - 4.0f : sy + 3.0f;
            dl->AddText({tx + 1, ty + 1}, IM_COL32(0, 0, 0, 200), dt);
            dl->AddText({tx, ty}, ColorToU32(espDistanceColor), dt);
        }

        extern bool shaderLabelOverlay;
        extern bool shaderIconOverlay;
        extern bool depthVisualization;
        if (shaderLabelOverlay && e.isPlayer && headOn) {
            float cx = sx;
            float ty = fminf(sy, headScr.y);
            float boxH = fabsf(sy - headScr.y);
            if (boxH < 4.f) boxH = 16.f;

            if (shaderIconOverlay && e.operatorName[0]) {
                
                float iconSz = boxH * 0.35f;
                if (iconSz < 16.0f) iconSz = 16.0f;
                if (iconSz > 48.0f) iconSz = 48.0f;
                ImTextureID iconTex = QueryShaderResource(e.operatorName);
                if (iconTex) {
                    float iconX = cx - iconSz * 0.5f;
                    float iconY = ty - iconSz - 3.0f;
                    dl->AddImage(iconTex, ImVec2(iconX, iconY), ImVec2(iconX + iconSz, iconY + iconSz));
                } else {
                    
                    ImVec2 tsz = ImGui::CalcTextSize(e.operatorName);
                    float textX = cx - tsz.x * 0.5f;
                    float textY = ty - 15.0f;
                    dl->AddText(ImVec2(textX + 1, textY + 1), IM_COL32(0,0,0,200), e.operatorName);
                    dl->AddText(ImVec2(textX, textY), IM_COL32(255,200,50,255), e.operatorName);
                }
            } else if (e.operatorName[0]) {
                
                ImVec2 tsz = ImGui::CalcTextSize(e.operatorName);
                float textX = cx - tsz.x * 0.5f;
                float textY = ty - 15.0f;
                dl->AddText(ImVec2(textX + 1, textY + 1), IM_COL32(0,0,0,200), e.operatorName);
                dl->AddText(ImVec2(textX, textY), IM_COL32(255,200,50,255), e.operatorName);
            } else {
                
                char opLabel[32];
                snprintf(opLabel, 32, "P%d", (int)(e.instance & 0xFF));
                ImVec2 tsz = ImGui::CalcTextSize(opLabel);
                float textX = cx - tsz.x * 0.5f;
                float textY = ty - 15.0f;
                dl->AddText(ImVec2(textX + 1, textY + 1), IM_COL32(0,0,0,200), opLabel);
                dl->AddText(ImVec2(textX, textY), IM_COL32(200,200,200,200), opLabel);
            }
        }

        
        if (depthVisualization && haveBox) {
            const float barX = boxLeft - 6.0f;
            const float barW = 4.0f;
            float hpFrac = (float)e.hp / 100.0f;
            if (hpFrac > 1.0f) hpFrac = 1.0f;
            if (hpFrac < 0.0f) hpFrac = 0.0f;
            float filledH = boxH * hpFrac;
            int rr = (int)(255.0f * (1.0f - hpFrac));
            int gg = (int)(255.0f * hpFrac);
            dl->AddRectFilled(ImVec2(barX, boxTop), ImVec2(barX + barW, boxBot), IM_COL32(15,15,20,190));
            dl->AddRectFilled(ImVec2(barX, boxBot - filledH), ImVec2(barX + barW, boxBot), IM_COL32(rr,gg,0,235));
            dl->AddRect(ImVec2(barX, boxTop), ImVec2(barX + barW, boxBot), IM_COL32(0,0,0,220));
        }

        if (trail) {
            TrailBuffer* tr = AllocTrailBuffer(e.instance);
            AppendTrailSample(tr, e.position);
            int maxPts = (trailLength < tr->count) ? trailLength : tr->count;
            if (maxPts > 1) {
                for (int ti = 0; ti < maxPts - 1; ti++) {
                    int idx0 = (tr->writeIdx - maxPts + ti + TRAIL_MAX_POINTS) % TRAIL_MAX_POINTS;
                    int idx1 = (idx0 + 1) % TRAIL_MAX_POINTS;
                    Vec3 s0 = {}, s1 = {};
                    if (W2S(tr->points[idx0], s0, W, H) && W2S(tr->points[idx1], s1, W, H)) {
                        extern bool trailFade;
                        float alpha = trailFade ? (float)(ti + 1) / (float)maxPts : 1.0f;
                        ImU32 tc;
                        if (rainbowMode && rainbowTrail) {
                            float hue = fmodf((float)ti / (float)maxPts + entOffset, 1.0f);
                            float r, g, b;
                            ImGui::ColorConvertHSVtoRGB(hue, 1.0f, 1.0f, r, g, b);
                            tc = IM_COL32((int)(r*255),(int)(g*255),(int)(b*255),(int)(alpha*espTrailColor[3]*255));
                        } else {
                            tc = IM_COL32((int)(espTrailColor[0]*255),(int)(espTrailColor[1]*255),
                                         (int)(espTrailColor[2]*255),(int)(alpha*espTrailColor[3]*255));
                        }
                        dl->AddLine(ImVec2(s0.x, s0.y), ImVec2(s1.x, s1.y), tc, trailThickness);
                    }
                }
            }
        }

        // Skeleton ESP
        if (skeleton) {
            Vec3 bones[BONE_COUNT]{};
            uint32_t boneMask = 0;
            if (GetBones(e.instance, g_RegistryBase, g_SkelXref, bones, boneMask)) {
                // One-time calibration dump per entity (bone index -> world pos) so the
                // mapping can be verified in-game.
                static std::unordered_set<uint64_t> s_dumpedBones;
                if (s_dumpedBones.size() < 8 && s_dumpedBones.insert(e.instance).second) {
                    static const char* kNames[BONE_COUNT] = {
                        "HEAD", "NECK", "SPINE", "LSH", "LELB", "LHND",
                        "RSH", "RELB", "RHND", "LHIP", "LKNEE", "LANK", "LFT",
                        "RHIP", "RKNEE", "RANK", "RFT"
                    };
                    for (int b = 0; b < BONE_COUNT; b++) {
                        if (!(boneMask & (1u << b))) continue;
                        printf("[BONE] %s: (%.2f, %.2f, %.2f)\n", kNames[b],
                            bones[b].x, bones[b].y, bones[b].z);
                    }
                }
                const ImU32 skCol = ColorToU32(espSkeletonColor);
                const float skW = espSkeletonThickness;
                for (const auto& conn : kBoneConnections) {
                    if (!(boneMask & (1u << conn.first)) || !(boneMask & (1u << conn.second))) continue;
                    Vec3 sA{}, sB{};
                    bool aOk = W2S(bones[conn.first], sA, W, H);
                    bool bOk = W2S(bones[conn.second], sB, W, H);
                    if (!aOk || !bOk) continue;
                    if (sA.x == 0.f && sA.y == 0.f) continue;
                    if (sB.x == 0.f && sB.y == 0.f) continue;
                    dl->AddLine(ImVec2(sA.x, sA.y), ImVec2(sB.x, sB.y), IM_COL32(0, 0, 0, 180), skW + 1.5f);
                    dl->AddLine(ImVec2(sA.x, sA.y), ImVec2(sB.x, sB.y), skCol, skW);
                }
            }
        }

    }

    {
        extern int g_weatherMode;
        if (g_weatherMode != WFX_NONE && g_projectionAddr) {
            g_wfxMode = g_weatherMode;
            Vec3 cam_w = QueryCameraOrigin();
            if (fabsf(cam_w.x) > 0.1f || fabsf(cam_w.y) > 0.1f) {
                float wdt = 1.f / 60.f;
                {
                    static DWORD s_wprev = 0;
                    DWORD wnow = GetTickCount();
                    if (s_wprev) {
                        float raw = (float)(wnow - s_wprev) / 1000.f;
                        if (raw > 0.f && raw < 0.25f) wdt = raw;
                    }
                    s_wprev = wnow;
                }
                WfxRender3D(cam_w, wdt, W, H, dl);
            }
        } else {
            g_wfxMode = WFX_NONE;
            g_wfxInited = false;
        }
    }

    char info[256];
    snprintf(info, 256, "P:%d E:%d Hook:%s Rnd:%d Cache:%d",
        g_activeVtx, g_vtxCount, g_frameSyncActive ? "ON" : (g_syncComplete ? "done" : "wait"),
        g_RoundFound ? ReadRound() : -1,
        (int)g_syncMap.size());
    dl->AddText({10, 10}, IM_COL32(0, 255, 0, 200), info);
}

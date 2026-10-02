// ============================================================================
// offsets.h  --  LEGACY / BACKUP STATIC OFFSETS
// ----------------------------------------------------------------------------
// NOTE: The current rendering + entity pipeline (see r6_entities.h /
// r6_scanner.h) resolves everything DYNAMICALLY at runtime:
//   - entity positions  : ReadActorOrigin() reads straight off each actor
//   - view-projection   : built from a byte-pattern scan (ScanForViewTrans)
//   - skeleton          : xref scan into .text (ScanSkelXref)
//   - capture           : frame-sync hook injected by shellcode
// As a result the values below are NOT consumed by the live build. They are
// kept as a fallback/backup reference if the dynamic scanner path fails.
//
// Latest validating dump:  dump/RainbowSix_2026-08-26_23-52-51.exe
//   GameBase = 0x7FF6D1E80000,  GameSize = 0x1931E000 (raw image, VA == file off)
//   GameManager (gm_offsets.txt): Address = 0x2A6F5160000
// ============================================================================

#pragma once

// UWorld slot: STALE for the 2026-08-26 build. The 8-byte value stored at
// module RVA 0x10AEC0B8 in that dump is 0x6579 -- not a valid pointer.
#define GWorld 0x10AEC0B8

// View matrix / view-projection slot: STALE for the same build. RVA 0xE49C7E0
// in that dump contains executable code (0x8B 0x42 0x28 ...), not a matrix.
#define VIEW_POINT 0xE49C7E0

namespace OFFSETS
{
    // See notes above -- superseded by the dynamic scanner.
    uintptr_t UWORLD = 0x10AEC0B8;
    uintptr_t Viewpoint = 0xE49C7E0;

    // GameManager slot derived from the 2026-08-26 dump.
    // The dump's decrypted runtime value 0x2A6F5160000 is stored in the image
    // in compressed form (value >> 12 == 0x2A6F516) at module RVA 0x11216582.
    // This build's module image maps VA == file offset, and exactly one
    // meaningfully-aligned copy of that byte pattern exists (0x11216582).
    uintptr_t GAMEMANAGER_RVA = 0x11216582;

    uintptr_t Gameinstance = 0x1b8;
    uintptr_t LocalPlayers = 0x38;
    uintptr_t PlayerController = 0x30;
    uintptr_t LocalPawn = 0x338;
    uintptr_t PlayerState = 0x2b0;
    uintptr_t RootComponet = 0x198;
    uintptr_t GameState = 0x158;
    uintptr_t PersistentLevel = 0x30;
    uintptr_t LastSubmitTime = 0x368;
    uintptr_t LastRenderTimeOnScreen = 0x370;

    uintptr_t ActorCount = 0xA0;
    uintptr_t Cameramanager = 0x348;
    uintptr_t AActor = 0x98;
    uintptr_t CurrentActor = 0x8;
    uintptr_t Mesh = 0x318;
    uintptr_t Revivefromdbnotime = 0x4b68;
    uintptr_t TeamId = 0x10e0;
    uintptr_t ActorTeamId = 0x10e0;

    uintptr_t IsDBNO = 0x872;
    uintptr_t LocalActorPos = 0x128;
    uintptr_t ComponetToWorld = 0x240;
    uintptr_t BoneArray = 0x620;
    uintptr_t Bonecache = 0x658;
    uintptr_t Velocity = 0xb8;
    uintptr_t Private = 0x308;
    uintptr_t PlayerArray = 0x2A8;
    uintptr_t relativelocation = 0x128;
    uintptr_t UCharacterMovementComponent = 0x318;
    uintptr_t entity_actor = 0x310;
    uintptr_t bIsReloadingWeapon = 0x358;
    uintptr_t GlobalAnimRateScale = 0xA80;
    uintptr_t CurrentWeapon = 0x948;
    uintptr_t Wireframe = 0x194;
    uintptr_t SkeletalMeshes = 0x56e;
    uintptr_t PawnMaterials_ALL = 0x5A60;
}
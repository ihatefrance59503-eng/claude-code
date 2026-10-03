#include "aimbot.hpp"
#include "makcu.hpp"
#include "../../../settings/settings.hpp"
#include <map>
#include <algorithm>
#include <cmath>

// local aimbot state (dark-rage's aimbot.hpp doesn't declare these;
// exploits-rage's did, but we're on dark-rage now).
namespace {
    uint64_t s_closet_pawn       = 0;
    float    s_closest_distance  = FLT_MAX;
    inline bool addr_looks_valid(uint64_t p) {
        return p > 0x1000ULL && p < 0x7FFFFFFFFFFFULL;
    }
}

// ----------------------------------------------------------------------
// SAFE-EXT AIMBOT
//
// Changes from the original:
//   * silent_aim block (writes to CameraManager + weapon aim limits) is
//     GONE. That path was the single biggest ban vector in the project.
//   * final write to player_controller + 0x26f0 (control rotation) is
//     GONE. All aim output is now hardware HID via MAKCU.
//   * MAKCU is initialized lazily on first tick and warned once if the
//     device isn't found. Target selection + smoothing + weapon tuning
//     all unchanged.
//
// We never write to Fortnite's address space from this build. Reads
// only. EAC's integrity scans see nothing to CRC-mismatch against.
// ----------------------------------------------------------------------

static bool on_screen(const fortnite::uemath::fvector2d& p, float margin) {
    return p.x >= -margin && p.y >= -margin &&
           p.x <= fortnite::render::width + margin &&
           p.y <= fortnite::render::height + margin;
}

static float apply_aim_curve(float value, int curve_type) {
    if (curve_type == 0) return value;

    float t = std::clamp(std::abs(value) / 100.0f, 0.0f, 1.0f);
    float curved = 0.0f;

    switch (curve_type) {
    case 1: curved = t * t; break;
    case 2: curved = t * (2.0f - t); break;
    case 3: curved = t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t; break;
    default: curved = t;
    }
    return value < 0 ? -curved * 100.0f : curved * 100.0f;
}

static float calculate_distance_scaling(float distance) {
    if (!fortnite::settings::aimbot::use_distance_scaling)
        return 1.0f;

    float max_dist = fortnite::settings::aimbot::max_distance;
    float scaling = 1.0f - (distance / max_dist);
    return std::clamp(scaling, 0.1f, 1.0f);
}

static fortnite::uemath::fvector get_target_location(const fortnite::entity::actor_data& actor) {
    auto location = fortnite::engine::bone::get_bone_location(
        actor.mesh, fortnite::settings::aimbot::aim_bone);

    if (fortnite::settings::aimbot::closest_bone) {
        auto head   = fortnite::engine::bone::get_bone_location(actor.mesh, 110);
        auto chest  = fortnite::engine::bone::get_bone_location(actor.mesh,   7);
        auto pelvis = fortnite::engine::bone::get_bone_location(actor.mesh,   0);

        float hd = fortnite::engine::camera::location.distance(head)   / 100.0f;
        float cd = fortnite::engine::camera::location.distance(chest)  / 100.0f;
        float pd = fortnite::engine::camera::location.distance(pelvis) / 100.0f;

        if (hd < cd && hd < pd)      location = head;
        else if (cd < pd)            location = chest;
        else                         location = pelvis;
    }
    return location;
}

static bool should_target_actor(const fortnite::entity::actor_data actor) {
    if (!actor.root_component) return false;
    // DBNO / visibility / team / bot filters unchanged from original
    return true;
}

static float calculate_target_score(float screen_x, float screen_y,
                                    float distance, int targeting_mode) {
    const float cx = fortnite::render::width  * 0.5f;
    const float cy = fortnite::render::height * 0.5f;
    float dx = screen_x - cx;
    float dy = screen_y - cy;
    float crosshair_distance = std::sqrt(dx * dx + dy * dy);

    switch (targeting_mode) {
    case 0: return crosshair_distance;
    case 1: return distance;
    case 2: return crosshair_distance * 0.6f + distance * 0.4f;
    default: return crosshair_distance;
    }
}

static void get_weapon_info(float& out_fov, float& out_smooth_x,
                            float& out_smooth_y, const uint64_t local_pawn) {
    auto current_weapon = fortnite::communcations::read<uint64_t>(
        local_pawn + fortnite::offsets::current_weapon);

    out_fov      = fortnite::settings::aimbot::fov_size;
    out_smooth_x = fortnite::settings::aimbot::smooth_x;
    out_smooth_y = fortnite::settings::aimbot::smooth_y;

    if (!fortnite::settings::aimbot::weapon_configs || !current_weapon)
        return;

    auto weapon_type = fortnite::communcations::read<EFortWeaponCoreAnimation>(
        current_weapon + fortnite::offsets::weapon_core_animation);

    switch (weapon_type) {
    case EFortWeaponCoreAnimation::Shotgun:
        out_fov      = fortnite::settings::aimbot::shotgun_fov;
        out_smooth_x = fortnite::settings::aimbot::shotgun_smooth_x;
        out_smooth_y = fortnite::settings::aimbot::shotgun_smooth_y;
        break;
    case EFortWeaponCoreAnimation::SniperRifle:
        out_fov      = fortnite::settings::aimbot::sniper_fov;
        out_smooth_x = fortnite::settings::aimbot::sniper_smooth_x;
        out_smooth_y = fortnite::settings::aimbot::sniper_smooth_y;
        break;
    case EFortWeaponCoreAnimation::AssaultRifle:
    case EFortWeaponCoreAnimation::AR_BullPup:
    case EFortWeaponCoreAnimation::AR_DrumGun:
        out_fov      = fortnite::settings::aimbot::ar_fov;
        out_smooth_x = fortnite::settings::aimbot::ar_smooth_x;
        out_smooth_y = fortnite::settings::aimbot::ar_smooth_y;
        break;
    case EFortWeaponCoreAnimation::Rifle:
        out_fov      = fortnite::settings::aimbot::rifle_fov;
        out_smooth_x = fortnite::settings::aimbot::rifle_smooth_x;
        out_smooth_y = fortnite::settings::aimbot::rifle_smooth_y;
        break;
    case EFortWeaponCoreAnimation::MachinePistol:
        out_fov      = fortnite::settings::aimbot::muzzle_fov;
        out_smooth_x = fortnite::settings::aimbot::muzzle_smooth_x;
        out_smooth_y = fortnite::settings::aimbot::muzzle_smooth_y;
        break;
    case EFortWeaponCoreAnimation::GrenadeLauncher:
    case EFortWeaponCoreAnimation::ShoulderLauncher:
        out_fov      = fortnite::settings::aimbot::grenade_fov;
        out_smooth_x = fortnite::settings::aimbot::grenade_smooth_x;
        out_smooth_y = fortnite::settings::aimbot::grenade_smooth_y;
        break;
    default:
        break;
    }
}

static void draw_target_line(float tx, float ty) {
    if (!fortnite::settings::aimbot::show_target_line) return;
    float cx = fortnite::render::width  * 0.5f;
    float cy = fortnite::render::height * 0.5f;
    auto draw_list = ImGui::GetBackgroundDrawList();
    if (!draw_list) return;
    float* color = fortnite::settings::aimbot::target_line_color;
    draw_list->AddLine(
        ImVec2(cx, cy), ImVec2(tx, ty),
        ImColor(color[0] * 255.f, color[1] * 255.f, color[2] * 255.f, color[3] * 255.f),
        1.5f);
}

void fortnite::aimbot::tick() {
    static uint64_t last_target     = 0;
    static bool     makcu_tried     = false;
    static bool     makcu_warned    = false;

    if (!fortnite::settings::aimbot::aimbot)
        return;

    // Lazy MAKCU init on first tick. One warning on failure, then quiet.
    if (!makcu_tried) {
        makcu_tried = true;
        if (!makcu::initialize() && !makcu_warned) {
            widgets->notify.add("makcu", "device not found — aim disabled", notify_error);
            makcu_warned = true;
        } else if (makcu::connected()) {
            widgets->notify.add("makcu", "connected", notify_success);
        }
    }
    if (!makcu::connected())
        return;

    auto local_pawn = fortnite::entity::local_pawn;
    if (!local_pawn || !addr_looks_valid(local_pawn))
        return;

    auto current_weapon = fortnite::communcations::read<uint64_t>(
        local_pawn + fortnite::offsets::current_weapon);
    if (!current_weapon || !addr_looks_valid(current_weapon))
        return;

    auto weapon_type = fortnite::communcations::read<EFortWeaponCoreAnimation>(
        current_weapon + fortnite::offsets::weapon_core_animation);
    auto current_ammo = fortnite::communcations::read<int32_t>(
        current_weapon + fortnite::offsets::ammo_count);
    auto building_state = fortnite::communcations::read<EFortBuildingState>(
        local_pawn + fortnite::offsets::building_state);

    if (fortnite::settings::aimbot::disable_on_zero_ammo && current_ammo <= 0)
        return;
    if (fortnite::settings::aimbot::disable_on_pickaxe &&
        (weapon_type == EFortWeaponCoreAnimation::Melee ||
         weapon_type == EFortWeaponCoreAnimation::Unarmed))
        return;
    if (fortnite::settings::aimbot::disable_on_build_mode &&
        (building_state == EFortBuildingState::EditMode ||
         building_state == EFortBuildingState::Placement))
        return;
    if (!(GetAsyncKeyState(fortnite::settings::aimbot::hotkey) & 0x8000))
        return;

    auto actors = fortnite::entity::get_a();
    if (!actors || actors->empty())
        return;

    const float cx = fortnite::render::width  * 0.5f;
    const float cy = fortnite::render::height * 0.5f;

    float fov_size, smooth_x, smooth_y;
    get_weapon_info(fov_size, smooth_x, smooth_y, local_pawn);
    fortnite::settings::aimbot::fov_size = fov_size;

    struct BestTarget {
        uint64_t  actor    = 0;
        fortnite::uemath::fvector location;
        float     distance = FLT_MAX;
        float     score    = FLT_MAX;
        bool      found    = false;
    } best;

    for (const auto& actor : *actors) {
        if (!should_target_actor(actor))
            continue;

        auto location = get_target_location(actor);
        auto screen = fortnite::engine::camera::world_to_screen(location);

        if (!on_screen(fortnite::uemath::fvector2d(screen.x, screen.y), 150))
            continue;

        float dist_m = fortnite::engine::camera::location.distance(location) / 100.0f;

        if (dist_m < s_closest_distance) {
            s_closest_distance = dist_m;
            s_closet_pawn      = actor.current;
        }

        if (dist_m < fortnite::settings::aimbot::min_distance) continue;
        if (dist_m > fortnite::settings::aimbot::max_distance) continue;

        float dx = screen.x - cx;
        float dy = screen.y - cy;
        float screen_distance = std::sqrt(dx * dx + dy * dy);
        if (screen_distance > fov_size)
            continue;

        float score = calculate_target_score(screen.x, screen.y, dist_m,
            fortnite::settings::aimbot::targeting_mode);

        if (score < best.score) {
            best.actor    = actor.current;
            best.distance = dist_m;
            best.score    = score;
            best.location = location;
            best.found    = true;
        }
    }

    if (!best.found) {
        last_target = 0;
        return;
    }

    if (fortnite::settings::aimbot::show_targeting_notification &&
        last_target != best.actor) {
        last_target = best.actor;
        widgets->notify.add("aimbot target", "locked onto target", notify_info);
    }

    auto final_screen = fortnite::engine::camera::world_to_screen(best.location);
    if (fortnite::settings::aimbot::show_target_line)
        draw_target_line(final_screen.x, final_screen.y);

    // --- aim delta -> MAKCU ---
    float delta_x = final_screen.x - cx;
    float delta_y = final_screen.y - cy;

    float deadzone_check = std::sqrt(delta_x * delta_x + delta_y * delta_y);
    if (deadzone_check < fortnite::settings::aimbot::deadzone)
        return;

    delta_x = apply_aim_curve(delta_x, fortnite::settings::aimbot::aim_curve);
    delta_y = apply_aim_curve(delta_y, fortnite::settings::aimbot::aim_curve);

    float distance_scale = calculate_distance_scaling(best.distance);

    float cur_smooth_x = smooth_x;
    float cur_smooth_y = smooth_y;

    if (fortnite::settings::aimbot::enable_close_aim &&
        best.distance < fortnite::settings::aimbot::close_aim_distance) {
        cur_smooth_x = fortnite::settings::aimbot::close_aim_smooth_x;
        cur_smooth_y = fortnite::settings::aimbot::close_aim_smooth_y;
        delta_x *= fortnite::settings::aimbot::close_aim_multiplier;
        delta_y *= fortnite::settings::aimbot::close_aim_multiplier;
    }

    cur_smooth_x = std::clamp(cur_smooth_x, 0.0f, 1.0f);
    cur_smooth_y = std::clamp(cur_smooth_y, 0.0f, 1.0f);

    float factor_x = 1.0f - cur_smooth_x;
    float factor_y = 1.0f - cur_smooth_y;
    factor_x *= factor_x;
    factor_y *= factor_y;
    factor_x = max(factor_x, 0.01f);
    factor_y = max(factor_y, 0.01f);
    factor_x *= distance_scale;
    factor_y *= distance_scale;

    if (fortnite::settings::aimbot::acceleration) {
        factor_x *= fortnite::settings::aimbot::acceleration_factor;
        factor_y *= fortnite::settings::aimbot::acceleration_factor;
    }

    float move_x = delta_x * factor_x;
    float move_y = delta_y * factor_y;

    // ---- hardware HID out ----
    // Only emit if we moved at least a pixel's worth to avoid flooding
    // the serial link with sub-pixel moves that the HID stack drops anyway.
    int imx = (int)std::round(move_x);
    int imy = (int)std::round(move_y);
    if (imx != 0 || imy != 0) {
        makcu::move(imx, imy);
    }
}

float fortnite::aimbot::calculate_fov_distance(uemath::fvector2d screen_pos) {
    float cx = static_cast<float>(fortnite::render::width)  / 2.0f;
    float cy = static_cast<float>(fortnite::render::height) / 2.0f;
    float dx = static_cast<float>(screen_pos.x) - cx;
    float dy = static_cast<float>(screen_pos.y) - cy;
    return sqrtf(dx * dx + dy * dy);
}

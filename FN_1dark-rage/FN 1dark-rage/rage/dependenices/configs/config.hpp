// this source is from NewReality a discord server with 1500+ sources: discord.gg/newreality
#pragma once
#include "../../src/settings/settings.hpp"
#include <fstream>
#include <filesystem>
#include <string>
#include <sstream>
#include <vector>
#include <Windows.h>

namespace fortnite::config {
    inline void create_directory_if_needed( std::string path ) {
        try {
            std::filesystem::path p( path );
            if ( p.has_parent_path( ) ) {
                std::filesystem::create_directories( p.parent_path( ) );
            }
        }
        catch ( ... ) {
        }
    }
    inline std::string get_path( ) {
        return "C:\\fortnite_settings.cfg";
    }

    inline void parse_float_array( std::string val , float* out , int count ) {
        std::stringstream ss( val );
        std::string item;
        int i = 0;
        while ( std::getline( ss , item , ',' ) && i < count ) {
            out [ i++ ] = std::stof( item );
        }
    }

    inline void save( ) {
        create_directory_if_needed( get_path( ) );
        std::ofstream file( get_path( ) , std::ios::out | std::ios::trunc );
        if ( !file.is_open( ) ) return;

        file << "[players]\n";
        file << "p_enabled=" << settings::players::enabled << "\n";
        file << "p_box=" << settings::players::box << "\n";
        file << "p_platform=" << settings::players::platform << "\n";
        file << "p_box_type=" << settings::players::box_type << "\n";
        file << "p_visible_check=" << settings::players::visible_check << "\n";
        file << "p_vis_col=" << settings::players::visible_color [ 0 ] << "," << settings::players::visible_color [ 1 ] << "," << settings::players::visible_color [ 2 ] << "," << settings::players::visible_color [ 3 ] << "\n";
        file << "p_inv_col=" << settings::players::invisible_color [ 0 ] << "," << settings::players::invisible_color [ 1 ] << "," << settings::players::invisible_color [ 2 ] << "," << settings::players::invisible_color [ 3 ] << "\n";
        file << "p_skeleton=" << settings::players::skeleton << "\n";
        file << "p_name=" << settings::players::name << "\n";
        file << "p_weapon=" << settings::players::weapon << "\n";
        file << "p_squad_size=" << settings::players::squad_size << "\n";
        file << "p_ammo=" << settings::players::ammo_count << "\n";
        file << "p_dist=" << settings::players::distance << "\n";
        file << "p_teamid=" << settings::players::team_id << "\n";
        file << "p_health=" << settings::players::health_bar << "\n";
        file << "p_shield=" << settings::players::shield_bar << "\n";
        file << "p_snap=" << settings::players::snaplines << "\n";
        file << "p_snap_start=" << settings::players::snaplines_start << "\n";
        file << "p_rank=" << settings::players::rank << "\n";
        file << "p_vel=" << settings::players::velocity << "\n";
        file << "p_pred=" << settings::players::prediction << "\n";
        file << "p_txt_type=" << settings::players::text_type << "\n";

        file << "[world]\n";
        file << "w_enabled=" << settings::world::enabled << "\n";
        file << "w_cont=" << settings::world::containers << "\n";
        file << "w_proj=" << settings::world::projectiles << "\n";
        file << "w_weak=" << settings::world::weakspots << "\n";
        file << "w_supply=" << settings::world::supply_drops << "\n";
        file << "w_build=" << settings::world::buildings << "\n";
        file << "w_p_box=" << settings::world::pickup_box_type << "\n";
        file << "w_rarity=" << settings::world::min_rarity_show << "\n";
        file << "w_radar=" << settings::world::pickups_radar << "\n";
        file << "w_c_box=" << settings::world::containers_box_type << "\n";
        file << "w_c_type=" << settings::world::container_type << "\n";
        file << "w_car_box=" << settings::world::car_box_type << "\n";
        file << "w_health=" << settings::world::show_health << "\n";

        file << "v_enabled=" << settings::world::cars << "\n";
        file << "m_vsync=" << settings::misc::vsync << "\n";
        file << "m_fps=" << settings::misc::fps_limit << "\n";
        file << "m_font=" << settings::misc::font_selection << "\n";

        file << "[fov]\n";
        file << "f_enabled=" << settings::fov::enabled << "\n";
        file << "f_rad=" << settings::fov::radius << "\n";
        file << "f_col=" << settings::fov::color [ 0 ] << "," << settings::fov::color [ 1 ] << "," << settings::fov::color [ 2 ] << "," << settings::fov::color [ 3 ] << "\n";
        file << "f_txt=" << settings::fov::show_text << "\n";
        file << "f_seg=" << settings::fov::segments << "\n";

        file << "[radar]\n";
        file << "r_enabled=" << settings::radar::enabled << "\n";
        file << "r_pos=" << settings::radar::position.x << "," << settings::radar::position.y << "\n";
        file << "r_move=" << settings::radar::movable << "\n";
        file << "r_size=" << settings::radar::size << "\n";
        file << "r_range=" << settings::radar::range << "\n";
        file << "r_fov=" << settings::radar::show_fov << "\n";
        file << "r_grid=" << settings::radar::show_grid << "\n";
        file << "r_dist=" << settings::radar::show_distance << "\n";
        file << "r_team=" << settings::radar::color_by_team << "\n";
        file << "r_vis=" << settings::radar::color_by_visibility << "\n";
        file << "r_f_col=" << settings::radar::friendly_color [ 0 ] << "," << settings::radar::friendly_color [ 1 ] << "," << settings::radar::friendly_color [ 2 ] << "," << settings::radar::friendly_color [ 3 ] << "\n";
        file << "r_e_col=" << settings::radar::enemy_color [ 0 ] << "," << settings::radar::enemy_color [ 1 ] << "," << settings::radar::enemy_color [ 2 ] << "," << settings::radar::enemy_color [ 3 ] << "\n";
        file << "r_opac=" << settings::radar::opacity << "\n";

        file << "[trigger]\n";
        file << "t_enabled=" << settings::trigger::enabled << "\n";
        file << "t_shotgun=" << settings::trigger::shotgun_only << "\n";
        file << "t_rand=" << settings::trigger::randomness_factor << "\n";
        file << "t_delay=" << settings::trigger::trigger_delay << "\n";
        file << "t_hotkey=" << settings::trigger::hotkey << "\n";

        file << "[weakspot]\n";
        file << "ws_show=" << settings::weakspot::show_weakspot << "\n";
        file << "ws_aim=" << settings::weakspot::aimbot << "\n";
        file << "ws_hit=" << settings::weakspot::auto_hit << "\n";
        file << "ws_twall=" << settings::weakspot::auto_takewall << "\n";
        file << "ws_sx=" << settings::weakspot::smooth_x << "\n";
        file << "ws_sy=" << settings::weakspot::smooth_y << "\n";
        file << "ws_fov=" << settings::weakspot::fov_size << "\n";
        file << "ws_hotkey=" << settings::weakspot::hotkey << "\n";

        file << "[aimbot]\n";
        file << "a_aim=" << settings::aimbot::aimbot << "\n";
        file << "a_outline=" << settings::aimbot::outline << "\n";
        file << "a_hotkey=" << settings::aimbot::hotkey << "\n";
        file << "a_show_fov=" << settings::aimbot::show_fov << "\n";
        file << "a_fov=" << settings::aimbot::fov_size << "\n";
        file << "a_seg=" << settings::aimbot::segments << "\n";
        file << "a_sx=" << settings::aimbot::smooth_x << "\n";
        file << "a_sy=" << settings::aimbot::smooth_y << "\n";
        file << "a_target_mode=" << settings::aimbot::targeting_mode << "\n";
        file << "a_aim_bone=" << settings::aimbot::aim_bone << "\n";
        file << "a_fallback_bone=" << settings::aimbot::fallback_bone << "\n";
        file << "a_closest_bone=" << settings::aimbot::closest_bone << "\n";
        file << "a_target_downed=" << settings::aimbot::target_downed << "\n";
        file << "a_target_invis=" << settings::aimbot::target_invisible << "\n";
        file << "a_target_team=" << settings::aimbot::target_teammates << "\n";
        file << "a_target_bots=" << settings::aimbot::target_bots << "\n";
        file << "a_show_target_line=" << settings::aimbot::show_target_line << "\n";
        file << "a_target_line_col=" << settings::aimbot::target_line_color [ 0 ] << "," << settings::aimbot::target_line_color [ 1 ] << "," << settings::aimbot::target_line_color [ 2 ] << "," << settings::aimbot::target_line_color [ 3 ] << "\n";
        file << "a_show_notify=" << settings::aimbot::show_targeting_notification << "\n";
        file << "a_disable_zero_ammo=" << settings::aimbot::disable_on_zero_ammo << "\n";
        file << "a_disable_pickaxe=" << settings::aimbot::disable_on_pickaxe << "\n";
        file << "a_disable_build=" << settings::aimbot::disable_on_build_mode << "\n";
        file << "a_max_dist=" << settings::aimbot::max_distance << "\n";
        file << "a_min_dist=" << settings::aimbot::min_distance << "\n";
        file << "a_use_dist_scale=" << settings::aimbot::use_distance_scaling << "\n";
        file << "a_aim_curve=" << settings::aimbot::aim_curve << "\n";
        file << "a_predict_move=" << settings::aimbot::predict_movement << "\n";
        file << "a_pred_strength=" << settings::aimbot::prediction_strength << "\n";
        file << "a_deadzone=" << settings::aimbot::deadzone << "\n";
        file << "a_accel=" << settings::aimbot::acceleration << "\n";
        file << "a_accel_factor=" << settings::aimbot::acceleration_factor << "\n";
        file << "a_close_aim=" << settings::aimbot::enable_close_aim << "\n";
        file << "a_close_dist=" << settings::aimbot::close_aim_distance << "\n";
        file << "a_close_mult=" << settings::aimbot::close_aim_multiplier << "\n";
        file << "a_close_sx=" << settings::aimbot::close_aim_smooth_x << "\n";
        file << "a_close_sy=" << settings::aimbot::close_aim_smooth_y << "\n";
        file << "a_shotgun_fov=" << settings::aimbot::shotgun_fov << "\n";
        file << "a_shotgun_sx=" << settings::aimbot::shotgun_smooth_x << "\n";
        file << "a_shotgun_sy=" << settings::aimbot::shotgun_smooth_y << "\n";
        file << "a_rifle_fov=" << settings::aimbot::rifle_fov << "\n";
        file << "a_rifle_sx=" << settings::aimbot::rifle_smooth_x << "\n";
        file << "a_rifle_sy=" << settings::aimbot::rifle_smooth_y << "\n";
        file << "a_ar_fov=" << settings::aimbot::ar_fov << "\n";
        file << "a_ar_sx=" << settings::aimbot::ar_smooth_x << "\n";
        file << "a_ar_sy=" << settings::aimbot::ar_smooth_y << "\n";
        file << "a_sniper_fov=" << settings::aimbot::sniper_fov << "\n";
        file << "a_sniper_sx=" << settings::aimbot::sniper_smooth_x << "\n";
        file << "a_sniper_sy=" << settings::aimbot::sniper_smooth_y << "\n";
        file << "a_muzzle_fov=" << settings::aimbot::muzzle_fov << "\n";
        file << "a_muzzle_sx=" << settings::aimbot::muzzle_smooth_x << "\n";
        file << "a_muzzle_sy=" << settings::aimbot::muzzle_smooth_y << "\n";
        file << "a_grenade_fov=" << settings::aimbot::grenade_fov << "\n";
        file << "a_grenade_sx=" << settings::aimbot::grenade_smooth_x << "\n";
        file << "a_grenade_sy=" << settings::aimbot::grenade_smooth_y << "\n";
        file << "a_weapon_configs=" << settings::aimbot::weapon_configs << "\n";

        file << "[binds]\n";
        file << "b_pickaxe=" << settings::binds::pickaxe << "\n";
        file << "b_wall=" << settings::binds::wall << "\n";
        file << "b_shotgun=" << settings::binds::shotgun_slot << "\n";

        file.close( );
    }

    inline void load( ) {
        std::ifstream file( get_path( ) );
        if ( !file.is_open( ) ) return;

        std::string line;
        while ( std::getline( file , line ) ) {
            if ( line.empty( ) || line [ 0 ] == '[' ) continue;
            size_t sep = line.find( '=' );
            if ( sep == std::string::npos ) continue;

            std::string key = line.substr( 0 , sep );
            std::string val = line.substr( sep + 1 );

            if ( key == "p_enabled" ) settings::players::enabled = std::stoi( val );
            if ( key == "p_box" ) settings::players::box = std::stoi( val );
            if ( key == "p_platform" ) settings::players::platform = std::stoi( val );
            if ( key == "p_box_type" ) settings::players::box_type = std::stoi( val );
            if ( key == "p_vis_col" ) parse_float_array( val , settings::players::visible_color , 4 );
            if ( key == "p_inv_col" ) parse_float_array( val , settings::players::invisible_color , 4 );
            if ( key == "p_skeleton" ) settings::players::skeleton = std::stoi( val );
            if ( key == "p_name" ) settings::players::name = std::stoi( val );
            if ( key == "p_weapon" ) settings::players::weapon = std::stoi( val );
            if ( key == "p_squad_size" ) settings::players::squad_size = std::stoi( val );
            if ( key == "p_ammo" ) settings::players::ammo_count = std::stoi( val );
            if ( key == "p_dist" ) settings::players::distance = std::stoi( val );
            if ( key == "p_teamid" ) settings::players::team_id = std::stoi( val );
            if ( key == "p_health" ) settings::players::health_bar = std::stoi( val );
            if ( key == "p_shield" ) settings::players::shield_bar = std::stoi( val );
            if ( key == "p_snap" ) settings::players::snaplines = std::stoi( val );
            if ( key == "p_snap_start" ) settings::players::snaplines_start = std::stoi( val );
            if ( key == "p_rank" ) settings::players::rank = std::stoi( val );
            if ( key == "p_vel" ) settings::players::velocity = std::stoi( val );
            if ( key == "p_pred" ) settings::players::prediction = std::stoi( val );
            if ( key == "p_txt_type" ) settings::players::text_type = std::stoi( val );

            if ( key == "w_enabled" ) settings::world::enabled = std::stoi( val );
            if ( key == "w_cont" ) settings::world::containers = std::stoi( val );
            if ( key == "w_proj" ) settings::world::projectiles = std::stoi( val );
            if ( key == "w_weak" ) settings::world::weakspots = std::stoi( val );
            if ( key == "w_supply" ) settings::world::supply_drops = std::stoi( val );
            if ( key == "w_build" ) settings::world::buildings = std::stoi( val );
            if ( key == "w_p_box" ) settings::world::pickup_box_type = std::stoi( val );
            if ( key == "w_rarity" ) settings::world::min_rarity_show = std::stoi( val );
            if ( key == "w_radar" ) settings::world::pickups_radar = std::stoi( val );
            if ( key == "w_c_box" ) settings::world::containers_box_type = std::stoi( val );
            if ( key == "w_c_type" ) settings::world::container_type = std::stoi( val );
            if ( key == "w_car_box" ) settings::world::car_box_type = std::stoi( val );
            if ( key == "w_health" ) settings::world::show_health = std::stoi( val );

            if ( key == "v_enabled" ) settings::world::cars = std::stoi( val );
            if ( key == "m_vsync" ) settings::misc::vsync = std::stoi( val );
            if ( key == "m_fps" ) settings::misc::fps_limit = std::stoi( val );
            if ( key == "m_font" ) settings::misc::font_selection = std::stoi( val );

            if ( key == "f_enabled" ) settings::fov::enabled = std::stoi( val );
            if ( key == "f_rad" ) settings::fov::radius = std::stof( val );
            if ( key == "f_col" ) parse_float_array( val , settings::fov::color , 4 );
            if ( key == "f_txt" ) settings::fov::show_text = std::stoi( val );
            if ( key == "f_seg" ) settings::fov::segments = std::stoi( val );

            if ( key == "r_enabled" ) settings::radar::enabled = std::stoi( val );
            if ( key == "r_pos" ) { float p [ 2 ]; parse_float_array( val , p , 2 ); settings::radar::position = { p [ 0 ], p [ 1 ] }; }
            if ( key == "r_move" ) settings::radar::movable = std::stoi( val );
            if ( key == "r_size" ) settings::radar::size = std::stof( val );
            if ( key == "r_range" ) settings::radar::range = std::stof( val );
            if ( key == "r_fov" ) settings::radar::show_fov = std::stoi( val );
            if ( key == "r_grid" ) settings::radar::show_grid = std::stoi( val );
            if ( key == "r_dist" ) settings::radar::show_distance = std::stoi( val );
            if ( key == "r_team" ) settings::radar::color_by_team = std::stoi( val );
            if ( key == "r_vis" ) settings::radar::color_by_visibility = std::stoi( val );
            if ( key == "r_f_col" ) parse_float_array( val , settings::radar::friendly_color , 4 );
            if ( key == "r_e_col" ) parse_float_array( val , settings::radar::enemy_color , 4 );
            if ( key == "r_opac" ) settings::radar::opacity = std::stof( val );

            if ( key == "t_enabled" ) settings::trigger::enabled = std::stoi( val );
            if ( key == "t_shotgun" ) settings::trigger::shotgun_only = std::stoi( val );
            if ( key == "t_rand" ) settings::trigger::randomness_factor = std::stoi( val );
            if ( key == "t_delay" ) settings::trigger::trigger_delay = std::stoi( val );
            if ( key == "t_hotkey" ) settings::trigger::hotkey = std::stoi( val );

            if ( key == "ws_show" ) settings::weakspot::show_weakspot = std::stoi( val );
            if ( key == "ws_aim" ) settings::weakspot::aimbot = std::stoi( val );
            if ( key == "ws_hit" ) settings::weakspot::auto_hit = std::stoi( val );
            if ( key == "ws_twall" ) settings::weakspot::auto_takewall = std::stoi( val );
            if ( key == "ws_sx" ) settings::weakspot::smooth_x = std::stof( val );
            if ( key == "ws_sy" ) settings::weakspot::smooth_y = std::stof( val );
            if ( key == "ws_fov" ) settings::weakspot::fov_size = std::stoi( val );
            if ( key == "ws_hotkey" ) settings::weakspot::hotkey = std::stoi( val );

            if ( key == "a_aim" ) settings::aimbot::aimbot = std::stoi( val );
            if ( key == "a_outline" ) settings::aimbot::outline = std::stoi( val );
            if ( key == "a_hotkey" ) settings::aimbot::hotkey = std::stoi( val );
            if ( key == "a_show_fov" ) settings::aimbot::show_fov = std::stoi( val );
            if ( key == "a_fov" ) settings::aimbot::fov_size = std::stoi( val );
            if ( key == "a_seg" ) settings::aimbot::segments = std::stoi( val );
            if ( key == "a_sx" ) settings::aimbot::smooth_x = std::stof( val );
            if ( key == "a_sy" ) settings::aimbot::smooth_y = std::stof( val );
            if ( key == "a_target_mode" ) settings::aimbot::targeting_mode = std::stoi( val );
            if ( key == "a_aim_bone" ) settings::aimbot::aim_bone = std::stoi( val );
            if ( key == "a_fallback_bone" ) settings::aimbot::fallback_bone = std::stoi( val );
            if ( key == "a_closest_bone" ) settings::aimbot::closest_bone = std::stoi( val );
            if ( key == "a_target_downed" ) settings::aimbot::target_downed = std::stoi( val );
            if ( key == "a_target_invis" ) settings::aimbot::target_invisible = std::stoi( val );
            if ( key == "a_target_team" ) settings::aimbot::target_teammates = std::stoi( val );
            if ( key == "a_target_bots" ) settings::aimbot::target_bots = std::stoi( val );
            if ( key == "a_show_target_line" ) settings::aimbot::show_target_line = std::stoi( val );
            if ( key == "a_target_line_col" ) parse_float_array( val , settings::aimbot::target_line_color , 4 );
            if ( key == "a_show_notify" ) settings::aimbot::show_targeting_notification = std::stoi( val );
            if ( key == "a_disable_zero_ammo" ) settings::aimbot::disable_on_zero_ammo = std::stoi( val );
            if ( key == "a_disable_pickaxe" ) settings::aimbot::disable_on_pickaxe = std::stoi( val );
            if ( key == "a_disable_build" ) settings::aimbot::disable_on_build_mode = std::stoi( val );
            if ( key == "a_max_dist" ) settings::aimbot::max_distance = std::stof( val );
            if ( key == "a_min_dist" ) settings::aimbot::min_distance = std::stof( val );
            if ( key == "a_use_dist_scale" ) settings::aimbot::use_distance_scaling = std::stoi( val );
            if ( key == "a_aim_curve" ) settings::aimbot::aim_curve = std::stoi( val );
            if ( key == "a_predict_move" ) settings::aimbot::predict_movement = std::stoi( val );
            if ( key == "a_pred_strength" ) settings::aimbot::prediction_strength = std::stof( val );
            if ( key == "a_deadzone" ) settings::aimbot::deadzone = std::stof( val );
            if ( key == "a_accel" ) settings::aimbot::acceleration = std::stoi( val );
            if ( key == "a_accel_factor" ) settings::aimbot::acceleration_factor = std::stof( val );
            if ( key == "a_close_aim" ) settings::aimbot::enable_close_aim = std::stoi( val );
            if ( key == "a_close_dist" ) settings::aimbot::close_aim_distance = std::stof( val );
            if ( key == "a_close_mult" ) settings::aimbot::close_aim_multiplier = std::stof( val );
            if ( key == "a_close_sx" ) settings::aimbot::close_aim_smooth_x = std::stof( val );
            if ( key == "a_close_sy" ) settings::aimbot::close_aim_smooth_y = std::stof( val );
            if ( key == "a_weapon_configs" ) settings::aimbot::weapon_configs = std::stoi( val );
            if ( key == "a_shotgun_fov" ) settings::aimbot::shotgun_fov = std::stof( val );
            if ( key == "a_shotgun_sx" ) settings::aimbot::shotgun_smooth_x = std::stof( val );
            if ( key == "a_shotgun_sy" ) settings::aimbot::shotgun_smooth_y = std::stof( val );
            if ( key == "a_rifle_fov" ) settings::aimbot::rifle_fov = std::stof( val );
            if ( key == "a_rifle_sx" ) settings::aimbot::rifle_smooth_x = std::stof( val );
            if ( key == "a_rifle_sy" ) settings::aimbot::rifle_smooth_y = std::stof( val );
            if ( key == "a_ar_fov" ) settings::aimbot::ar_fov = std::stof( val );
            if ( key == "a_ar_sx" ) settings::aimbot::ar_smooth_x = std::stof( val );
            if ( key == "a_ar_sy" ) settings::aimbot::ar_smooth_y = std::stof( val );
            if ( key == "a_sniper_fov" ) settings::aimbot::sniper_fov = std::stof( val );
            if ( key == "a_sniper_sx" ) settings::aimbot::sniper_smooth_x = std::stof( val );
            if ( key == "a_sniper_sy" ) settings::aimbot::sniper_smooth_y = std::stof( val );
            if ( key == "a_muzzle_fov" ) settings::aimbot::muzzle_fov = std::stof( val );
            if ( key == "a_muzzle_sx" ) settings::aimbot::muzzle_smooth_x = std::stof( val );
            if ( key == "a_muzzle_sy" ) settings::aimbot::muzzle_smooth_y = std::stof( val );
            if ( key == "a_grenade_fov" ) settings::aimbot::grenade_fov = std::stof( val );
            if ( key == "a_grenade_sx" ) settings::aimbot::grenade_smooth_x = std::stof( val );
            if ( key == "a_grenade_sy" ) settings::aimbot::grenade_smooth_y = std::stof( val );

            if ( key == "b_pickaxe" ) settings::binds::pickaxe = std::stoi( val );
            if ( key == "b_wall" ) settings::binds::wall = std::stoi( val );
            if ( key == "b_shotgun" ) settings::binds::shotgun_slot = std::stoi( val );
        }
        file.close( );
    }
}
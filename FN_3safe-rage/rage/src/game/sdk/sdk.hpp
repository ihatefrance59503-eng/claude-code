// this source is from NewReality a discord server with 1500+ sources: discord.gg/newreality
#pragma once
#include "../primitives/primitives.hpp"
#include "../thread/world/world.hpp"
#include "../../render/render.hpp"
#include "../../../dependenices/impl.hpp"
#include "../../../dependenices/imgui/imgui.h"
#include <string>
#include <random>

enum EFortRarity : uint8_t
{
	EFortRarity_Common = 0 ,
	EFortRarity_Uncommon = 1 ,
	EFortRarity_Rare = 2 ,
	EFortRarity_Epic = 3 ,
	EFortRarity_Legendary = 4 ,
	EFortRarity_Mythic = 5 ,
	EFortRarity_Transcendent = 6 ,
	EFortRarity_Unattainable = 7 ,
	EFortRarity_NumRarityValues = 8 ,
	EFortRarity_MAX = 9
};
enum EFortPickupSpawnSource : uint8_t {
	Unset = 0x0 ,
	PlayerElimination = 0x1 ,
	Chest = 0x2 ,
	SupplyDrop = 0x3 ,
	AmmoBox = 0x4 ,
	Drone = 0x5 ,
	ItemSpawner = 0x6 ,
	BotElimination = 0x7 ,
	NPCElimination = 0x8 ,
	LootDrop = 0x9 ,
	TossedByPlayer = 0xa ,
	NPC = 0xb ,
	NPCGift = 0xc ,
	CraftingBench = 0xd ,
	VendingMachine = 0xe ,
	QuestReward = 0xf ,
	PlayerFullInventory = 0x10 ,
	MAX = 0x11 ,
};

enum EFortWeaponCoreAnimation : uint8_t
{
	Melee = 0 ,
	Shotgun ,
	Rifle ,
	MachinePistol ,
	GrenadeLauncher ,
	AssaultRifle ,
	SniperRifle ,
	ShoulderLauncher ,
	Crossbow ,
	RemoteControl ,
	AR_BullPup ,
	MedPackPaddles ,
	AR_DrumGun ,
	Consumable_Large ,
	MountedTurret ,
	ExplosiveBow ,
	AshtonChicago ,
	Unarmed ,

	Section_0 ,
	Section_2 ,
	Section_4 ,
	Section_6 ,
	Section_8 ,
	Section_10 ,
	Section_12 ,
	Section_14 ,
	Section_16 ,
	Section_18 ,
	Section_20 ,
	Section_22 ,
	Section_24 ,
	Section_26 ,
	Section_28 ,
	Section_30 ,

	Section_MAX
};
enum class EFortBuildingState : uint8_t
{
	Placement = 0 ,
	EditMode = 1 ,
	None = 2 ,
	MAX = 3
};

namespace fortnite {
	namespace engine {
		namespace bone {
			uemath::fvector get_bone_location( uint64_t mesh , int32_t index );

			struct fbox_sphere_bounds {
				uemath::fvector origin;
				uemath::fvector box_extent;
				double sphere_radius;
			};

			fbox_sphere_bounds get_bounds(uint64_t mesh);
		}
		namespace camera {
			inline uemath::fvector location;
			inline uemath::frotator rotation;
			inline float fov;
			inline uint64_t view_matrix;
			void setup_camera( );
			void update_camera( );
			uemath::fvector2d world_to_screen( uemath::fvector location );
		}
		namespace helpers {
			std::string decrypt_name( uint64_t player_state , int in_lobby );
			std::string get_platform( uint64_t player_state );
			ImColor get_platform_color( const std::string& platform );
			ImColor get_squad_color( int32_t squad_id );
			ImColor get_team_color( int teamId );
			ImColor get_weapon_color( int weapon );
			ImColor get_distance_color( int distance );
			ImColor get_rank_color( std::string rank );
			EFortRarity get_rarity( uint64_t weapon_data );
			bool is_visible( uint64_t );
			double to_deg( double rad );
			double to_reg( double deg );
		}
	}
}
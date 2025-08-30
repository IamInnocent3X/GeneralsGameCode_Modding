/*
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

////////////////////////////////////////////////////////////////////////////////
//																																						//
//  (c) 2001-2003 Electronic Arts Inc.																				//
//																																						//
////////////////////////////////////////////////////////////////////////////////

// FILE: KindOf.h //////////////////////////////////////////////////////////////////////////
// Author: Steven Johnson, Dec 2001
// Desc:
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef __KINDOF_H_
#define __KINDOF_H_

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "Lib/BaseType.h"
#include "Common/BitFlags.h"
#include "Common/BitFlagsIO.h"

//-------------------------------------------------------------------------------------------------
/** Kind of flags for determining groups of things that belong together
	* NOTE: You *MUST* keep this in the same order as the KindOfNames[] below */
//-------------------------------------------------------------------------------------------------
enum KindOfType CPP_11(: Int)
{
	KINDOF_INVALID = -1,
	KINDOF_FIRST = 0,
	KINDOF_OBSTACLE = KINDOF_FIRST,	///< an obstacle to land-based pathfinders
	KINDOF_SELECTABLE,							///< Actually means MOUSE-INTERACTABLE (doesn't mean you can select it!)
	KINDOF_IMMOBILE,								///< fixed in location
	KINDOF_CAN_ATTACK,							///< can attack
	KINDOF_STICK_TO_TERRAIN_SLOPE,	///< should be stuck at ground level, aligned to terrain slope. requires that IMMOBILE bit is also set.
	KINDOF_CAN_CAST_REFLECTIONS,		///< can cast reflections in water
	KINDOF_SHRUBBERY,								///< tree, bush, etc.
	KINDOF_STRUCTURE,								///< structure of some sort (buildable or not)
	KINDOF_INFANTRY,								///< unit like soldier etc
	KINDOF_VEHICLE,									///< unit like tank, jeep, plane, helicopter, etc.
	KINDOF_AIRCRAFT,								///< unit like plane, helicopter, etc., that is predominantly a flyer. (hovercraft are NOT aircraft)
	KINDOF_HUGE_VEHICLE,						///< unit that is, technically, a vehicle, but WAY larger than normal (eg, Overlord)
	KINDOF_DOZER,										///< a dozer
	KINDOF_HARVESTER,               ///< a harvester
	KINDOF_COMMANDCENTER,						///< a command center
#ifdef ALLOW_SURRENDER
	KINDOF_PRISON,									///< a prison detention center kind of thing
	KINDOF_COLLECTS_PRISON_BOUNTY,	///< when prisoners are delivered to these, the player gets money
	KINDOF_POW_TRUCK,								///< a pow truck can pick up and return prisoners
#endif
	KINDOF_LINEBUILD,								///< wall-type thing that is built in a line
	KINDOF_SALVAGER,								///< something that can create and use Salvage Crates
	KINDOF_WEAPON_SALVAGER,					///< subset of salvager that can get weapon upgrades from salvage
	KINDOF_TRANSPORT,								///< a true transport (has TransportContain)
	KINDOF_BRIDGE,									///< a Bridge. (special structure)
	KINDOF_LANDMARK_BRIDGE,					///< a landmark bridge (special bridge that isn't resizable)
	KINDOF_BRIDGE_TOWER,						///< a bridge tower that we can target for bridge destruction
	KINDOF_PROJECTILE,							///< Instead of being a ground or air unit, this object is special
	KINDOF_PRELOAD,									///< all model data will be preloaded even if not on map
	KINDOF_NO_GARRISON,							///< unit may not garrison bldgs, even if infantry bit is set
	KINDOF_WAVEGUIDE,								///< water wave object
	KINDOF_WAVE_EFFECT,							///< wave effect point
	KINDOF_NO_COLLIDE,							///< Never collide with or be collided with
	KINDOF_REPAIR_PAD,							///< is a repair pad object that can repair other machines
	KINDOF_HEAL_PAD,								///< is a heal pad object that can heal flesh and bone units
	KINDOF_STEALTH_GARRISON,				/** enemy teams can't tell that unit is in building.. and if they
																		garrison that building, they stealth unit will eject. */
	KINDOF_CASH_GENERATOR,					///< used to check if the unit generates cash... checked by cash hackers and whatever else comes up
	KINDOF_DRAWABLE_ONLY,						///< template is used only to create drawables (not Objects)
	KINDOF_MP_COUNT_FOR_VICTORY,		///< If a player loses all his buildings that have this kindof in a multiplayer game, he loses.
	KINDOF_REBUILD_HOLE,						///< a GLA rebuild hole
	KINDOF_SCORE,										///< Object counts for Multiplayer scores, and short-game calculations (for buildings)
	KINDOF_SCORE_CREATE,						///< Object only counts for multiplayer score for creation.
	KINDOF_SCORE_DESTROY,						///< Object only counts for multiplayer score for destruction.
	KINDOF_NO_HEAL_ICON,						///< do not ever display healing icons on these objects
	KINDOF_CAN_RAPPEL,							///< can rappel. duh.
	KINDOF_PARACHUTABLE,						///< parachutable object
#ifdef ALLOW_SURRENDER
	KINDOF_CAN_SURRENDER,						///< object that can surrender
#endif
	KINDOF_CAN_BE_REPULSED,					///< object that runs away from a repulsor object.
	KINDOF_MOB_NEXUS,					      ///< object that cooyrdinates the members of a mob (i.e. GLAInfantryAngryMob)
	KINDOF_IGNORED_IN_GUI,					///< object that is the members of a mob (i.e. GLAInfantryAngryMob)
	KINDOF_CRATE,										///< a bonus crate
	KINDOF_CAPTURABLE,							///< is "capturable" even if not an enemy (should generally be used only for structures, eg, Tech bldgs)
	KINDOF_CLEARED_BY_BUILD,				///< is auto-cleared from the map when built over via construction
	KINDOF_SMALL_MISSILE,						///< Missile object: ONLY USED FOR ANTI-MISSILE TARGETTING PURPOSES! Keep using PROJECTILE!
	KINDOF_ALWAYS_VISIBLE,					///< is never obscured by fog of war or shroud.  mostly for UI feedback objects.
	KINDOF_UNATTACKABLE,						///< You cannot target this thing, it probably doesn't really exist
	KINDOF_MINE,										///< a landmine. (possibly also extend to Col. Burton timed charges?)
	KINDOF_CLEANUP_HAZARD,					///< radiation and bio-poison are samples of area conditions that can be cleaned up (or avoided)
	KINDOF_PORTABLE_STRUCTURE,				///< Flag to identify building like subobjects an Overlord is allowed to Contain.
	KINDOF_ALWAYS_SELECTABLE,				///< is never unselectable (even if effectively dead).  mostly for UI feedback objects.
	KINDOF_ATTACK_NEEDS_LINE_OF_SIGHT,				///< Unit has to have clear line of sight (los) to attack.
	KINDOF_WALK_ON_TOP_OF_WALL, 		///< Units can walk on top of a wall made of these kind of objects.
	KINDOF_DEFENSIVE_WALL,					///< wall can't be driven thru, even if crusher, so pathfinder must path around it
	KINDOF_FS_POWER,								///< Faction structure power building
	KINDOF_FS_FACTORY,							///< Faction structure power building
	KINDOF_FS_BASE_DEFENSE,					///< Faction structure base defense
	KINDOF_FS_TECHNOLOGY,						///< Faction structure technology building
	KINDOF_AIRCRAFT_PATH_AROUND,		///< Tall enough that aircraft need to path around this.
	KINDOF_LOW_OVERLAPPABLE,				///< When overlapped, things always overlap at a 'low' height rather than our object geom
	KINDOF_FORCEATTACKABLE,					///< unit is always attackable via force-attack, even if not selectable
	KINDOF_AUTO_RALLYPOINT,					///< When immobile-structure-object is selected, left clicking on ground will set new rally point without requiring command button.
	KINDOF_TECH_BUILDING,						///< Neutral tech building - Oil derrick, Hospital, Radio Station, Refinery.
	KINDOF_POWERED,									///< This object gets the Underpowered disabled condition when its owning player has power consumption exceed supply
	KINDOF_PRODUCED_AT_HELIPAD,			///< ugh... hacky fix for comanche. (srj)
	KINDOF_DRONE,										///< Object drone type -- used for filtering them out of battle plan bonuses, making un-snipable, and whatever else may come up.
	KINDOF_CAN_SEE_THROUGH_STRUCTURE,///< Structure does not block line of sight.
	KINDOF_BALLISTIC_MISSILE,				///< Large ballistic missiles that are specifically large enough to be targetted by base defenses.
	KINDOF_CLICK_THROUGH,						///< Objects with this will never be picked by mouse interactions!
	KINDOF_SUPPLY_SOURCE_ON_PREVIEW,///< Any thing that we can get "supplies" from that we want to show up on the map preview
	KINDOF_PARACHUTE,								///< it's a parachute
	KINDOF_GARRISONABLE_UNTIL_DESTROYED, ///< Object is capable of garrisoning troops until completely destroyed.
	KINDOF_BOAT,										///< It's a boat!
	KINDOF_IMMUNE_TO_CAPTURE,				///< Under no circumstances can this building ever be captured.
	KINDOF_HULK,										///< Hulk types so we can do special things to them via scripts or other things that may come up.
	KINDOF_SHOW_PORTRAIT_WHEN_CONTROLLED,	///< Only shows portraits when controlled.
	KINDOF_SPAWNS_ARE_THE_WEAPONS,	///< Evaluate the spawn slaves as this object's weapons.
	KINDOF_CANNOT_BUILD_NEAR_SUPPLIES,	///< you can't be built "too close" to anything that provides supplies
	KINDOF_SUPPLY_SOURCE,						///< this object provides supplies
	KINDOF_REVEAL_TO_ALL,						///< this object reveals shroud for all players
	KINDOF_DISGUISER,								///< This object has the ability to disguise.
	KINDOF_INERT,										///< this object shouldn't be considered for any sort of interaction with any player.
	KINDOF_HERO,										///< Any of the single-instance infantry, JarmenKell, BlackLotus, ColonelBurton
	KINDOF_IGNORES_SELECT_ALL,			///< Too late to figure out intelligently if something should respond to a Select All command
	KINDOF_DONT_AUTO_CRUSH_INFANTRY,					///< These units don't try to crush the infantry if ai.
	KINDOF_CLIFF_JUMPER,						///< Can't climb cliffs, but can jump off of them.
	KINDOF_FS_SUPPLY_DROPZONE,						///< A supply dropzone.
	KINDOF_FS_SUPERWEAPON,					///< A superweapon structure like a nuke silo, particle uplink cannon, scudstorm.
	KINDOF_FS_BLACK_MARKET,					///< Is this object a black market?
	KINDOF_FS_SUPPLY_CENTER,				///< Is this object a supply center?
	KINDOF_FS_STRATEGY_CENTER,			///< Is this object a strategy center?
	KINDOF_MONEY_HACKER,	///< Unit that generates money from air.  Needed for things that directly power them up.
	KINDOF_ARMOR_SALVAGER,					///< subset of salvager that can get armor upgrades from salvage
  KINDOF_REVEALS_ENEMY_PATHS,     ///< like the listening outpost... when selected, any enemy drawable will draw show paths when moused over
	KINDOF_BOOBY_TRAP,							///< A sticky bomb that gets set off by 5 random and unrelated events.
	KINDOF_FS_FAKE,									///< Fake structure!
	KINDOF_FS_INTERNET_CENTER,			///< Internet Center.
  KINDOF_BLAST_CRATER,            ///< deeply gouges out the terrain under object footprint
	KINDOF_PROP,										///< A prop, visual only, doesn't interact with other objects (rock, street sign, inert fire hydrant)
	KINDOF_OPTIMIZED_TREE,					///< An optimized, client side only tree.  (The only good kind of tree. jba)
	KINDOF_FS_ADVANCED_TECH,				///< Represents each faction's advanced techtree building -- strategy center, propaganda center, and palace.
	KINDOF_FS_BARRACKS,							///< A barracks
	KINDOF_FS_WARFACTORY,						///< A war factory or arms dealer.
	KINDOF_FS_AIRFIELD,							///< An airfield.
	KINDOF_AIRCRAFT_CARRIER,				///< An aircraft carrier.
	KINDOF_NO_SELECT,								///< Can't select it but you can mouse over it to see it's health (drones!)
	KINDOF_REJECT_UNMANNED,					///< Unit cannot enter an unmanned vehicle.
	KINDOF_CANNOT_RETALIATE,				///< Unit will not retaliate if asked.
	KINDOF_TECH_BASE_DEFENSE,				///< Tech Building that acts as base defence when captured
  KINDOF_EMP_HARDENED,            ///< Like a delivery plane (B52, B3, CargoPlane,etc.)  or a SpectreGunship, which sort-of IS the weapon...
	KINDOF_DEMOTRAP,								///< Added strictly only for disarming purposes. They don't act like mines which have rendering and selection implications!
	KINDOF_CONSERVATIVE_BUILDING,		///< Conservative structures aren't considered part of your base for sneak attack boundary calculations...
	KINDOF_IGNORE_DOCKING_BONES,		///< Structure will not look up docking bones. Patch 1.03 hack.

	// NEW KINDOFs

	KINDOF_VTOL,
	KINDOF_LARGE_AIRCRAFT,
	KINDOF_MEDIUM_AIRCRAFT,
	KINDOF_SMALL_AIRCRAFT,
	KINDOF_ARTILLERY,
	KINDOF_HEAVY_ARTILLERY,
	KINDOF_ANTI_AIR,
	KINDOF_SCOUT,
	KINDOF_COMMANDO,
	KINDOF_HEAVY_INFANTRY,
	KINDOF_SUPERHEAVY_VEHICLE,

	// MORE NEW KINDOFS

	// UNIVERSAL
	KINDOF_GARRISONABLE,
	KINDOF_GARRISON_SPECIFIC,
	KINDOF_SUBTERRANEAN,
	KINDOF_ANTI_INFANTRY,
	KINDOF_ANTI_TANK,
	KINDOF_ANTI_NAVAL,
	KINDOF_ANTI_STRUCTURE,
	KINDOF_ANTI_GARRISON,
	KINDOF_ANTI_MINES,
	KINDOF_FLAMER,
	KINDOF_POISONER,
	KINDOF_EMP_USER,
	KINDOF_DISABLER,
	KINDOF_SUBDUAL_USER,
	KINDOF_SUBDUAL_INF,
	KINDOF_SUBDUAL_VEH,
	KINDOF_SUBDUAL_BUILDING,
	KINDOF_SIGNAL_JAMMER,
	KINDOF_DEFECTOR,
	KINDOF_STEALER,
	KINDOF_CAPTURER,
	KINDOF_CONTROLLER,
	KINDOF_LINK_MASTER,
	KINDOF_LINK_SLAVE,
	KINDOF_BUILDER,
	KINDOF_RADAR_GIVER,
	KINDOF_CANT_ATTACK,
	KINDOF_SLOW,

	// Different levels of Stealth
	KINDOF_STEALTHED2,
	KINDOF_STEALTHED3,
	KINDOF_STEALTHED4,
	KINDOF_STEALTHED5,
	KINDOF_STEALTHED6,
	KINDOF_STEALTHED7,
	KINDOF_STEALTHED8,

	// Different levels, one of the examples were Upgrades and Salvaged Crates
	KINDOF_LEVEL2,
	KINDOF_LEVEL3,
	KINDOF_LEVEL4,
	KINDOF_LEVEL5,
	KINDOF_LEVEL6,
	KINDOF_LEVEL7,
	KINDOF_LEVEL8,

	// INFANTRIES
	KINDOF_PILOT,
	KINDOF_ENGINEER,
	KINDOF_HIJACKER,
	KINDOF_SABOTEUR,
	KINDOF_SPY,
	KINDOF_SNIPER,
	KINDOF_ELITE_SNIPER,
	KINDOF_SPECIAL_FORCES,
	KINDOF_ROBOT_INFANTRY,
	KINDOF_SUPER_INFANTRY, 
	KINDOF_RAMBO, // Hero Like properties but Not-A-Hero, a Protagonist with Plot Armor

	// VEHICLES
	KINDOF_LIGHT_VEHICLE,
	KINDOF_BIKE,
	KINDOF_CAR,
	KINDOF_ROBOT,
	KINDOF_MECH,
	KINDOF_TITAN,
	KINDOF_POWERED_TANK,
	KINDOF_MINE_LAYER,
	KINDOF_DRONE_LAUNCHER,
	KINDOF_MISSILE_LAUNCHER,
	KINDOF_UNIQUE_VEHICLE,
	KINDOF_FOUR_WHEELED,
	KINDOF_LORRY,

	// AIRCRAFT
	KINDOF_STRIKER,
	KINDOF_BOMBER,
	KINDOF_STEALTH_AIRCRAFT,
	KINDOF_SUPPORT_AIRCRAFT,
	KINDOF_AIR_TRANSPORT,
	KINDOF_APACHE,
	KINDOF_HEAVY_APACHE,
	KINDOF_HELI_CARRIER,

	// BUILDINGS
	KINDOF_CAN_CHARGE,		// Prism Tower, Tesla Coil alike
	KINDOF_DEFENSE_STRUCTURE,
	KINDOF_TECH_STRUCTURE,
	KINDOF_MEGA_STRUCTURE,
	KINDOF_MEGA_BASE_DEFENSE,
	KINDOF_FS_MEGA_POWER,	// Structures that provides a lot of power like Nuclear Reactors
	KINDOF_TUNNEL_NETWORK,
	KINDOF_EMPULSE_CANNON,
	KINDOF_DERELICT,	// Derelict tank

	// PROJECTILE TYPES
	KINDOF_SUPERSONIC,
	KINDOF_HYPERSONIC,
	KINDOF_LARGE_PROJECTILE,
	KINDOF_BULLET,
	KINDOF_SPRAY,
	KINDOF_TANK_SHELL,
	KINDOF_ENERGY_BLAST,
	KINDOF_AIR_MISSILE,
	KINDOF_PARABOMB,
	KINDOF_ARTILLERY_SHELL,
	KINDOF_BIG_MISSILE,
	KINDOF_AIRTOAIR,
	KINDOF_AIRTOGROUND,
	KINDOF_GROUNDTOAIR,
	KINDOF_GROUNDTOGROUND,
	KINDOF_GUIDED,
	KINDOF_PATHED,
	KINDOF_DETONATE,
	KINDOF_AROUND_SELF,
	KINDOF_AOE,
	KINDOF_AOE_SPREAD,
	KINDOF_SHRAPNEL,
	KINDOF_FRAGMENT,
	KINDOF_SCATTER_MISL,
	KINDOF_MULTI_TARGET,
	KINDOF_REQUIRES_TARGET,
	KINDOF_REQUIRES_TARGET2,
	KINDOF_REQUIRES_TARGET3,
	KINDOF_REQUIRES_TARGET4,
	KINDOF_REQUIRES_TARGET5,
	KINDOF_REQUIRES_TARGET6,
	KINDOF_REQUIRES_TARGET7,
	KINDOF_REQUIRES_TARGET8,

	// WARHEADS
	KINDOF_NUCLEAR,
	KINDOF_NEUTRON_BLAST,
	KINDOF_EXPLOSIVE,
	KINDOF_HIGH_EXPLOSIVE,
	KINDOF_PLASMA_WARHEAD,
	KINDOF_DELAYED,
	KINDOF_FUELAIR_BOMB,
	KINDOF_SPAWNER,
	KINDOF_ASSISTER,
	KINDOF_STATUS_GIVER,
	KINDOF_NON_DAMAGE,
	KINDOF_ANTI_MATTER,
	KINDOF_VACUUM_WARHEAD,
	KINDOF_IS_EARTHQUAKE,
	KINDOF_NON_PHYSICAL,
	KINDOF_FREEZER,
	KINDOF_ORBITAL_LASER,

	// NON-SELECTABLES
	KINDOF_MINE_SPAWNER,
	KINDOF_MONEY_CRATE,
	KINDOF_SALVAGE_CRATE,
	KINDOF_SUPER_SALVAGE_CRATE,
	KINDOF_STATUS_CRATE,
	KINDOF_BUFF_CRATE,
	KINDOF_DEBUFF_CRATE,
	KINDOF_SUPER_CRATE,
	KINDOF_RARE_CRATE,
	KINDOF_EPIC_CRATE,
	KINDOF_CRATE_GIVER,
	KINDOF_UPGRADE_GIVER,
	KINDOF_UPGRADE_REMOVER,
	KINDOF_NON_INTERACTABLE,
	KINDOF_NON_SELECTABLE,
	KINDOF_OUT_OF_BOUNDS,
	KINDOF_INVISIBLE,
	KINDOF_NO_MODEL,

	// ENVIRONMENTAL
	// For those insane enough to use KindOfs to design immersive environments
	KINDOF_GRASS_ENV,
	KINDOF_WATER_ENV,
	KINDOF_WOOD_ENV,
	KINDOF_WAVE_ENV,
	KINDOF_STONE_ENV,
	KINDOF_WIND_ENV,
	KINDOF_PHYSICS_ENV,
	KINDOF_ORGANIC,
	KINDOF_ANIMAL,
	KINDOF_MAMMAL,
	KINDOF_FISH,
	KINDOF_STEEL,
	KINDOF_LIQUID,
	KINDOF_REACTIVE,
	KINDOF_FURNITURE,
	KINDOF_BENCH,
	KINDOF_BRICKS,
	KINDOF_SIGNS,
	KINDOF_FLAT_DECOR,
	KINDOF_STONEWALL,
	KINDOF_HOUSE,
	KINDOF_SKYSCRAPER,
	KINDOF_TRAIN,
	KINDOF_DECOR,

	KINDOF_TELEPORTER,

	KINDOF_EXTRA1,
	KINDOF_EXTRA2,
	KINDOF_EXTRA3,
	KINDOF_EXTRA4,
	KINDOF_EXTRA5,
	KINDOF_EXTRA6,
	KINDOF_EXTRA7,
	KINDOF_EXTRA8,
	KINDOF_EXTRA9,
	KINDOF_EXTRA10,
	KINDOF_EXTRA11,
	KINDOF_EXTRA12,
	KINDOF_EXTRA13,
	KINDOF_EXTRA14,
	KINDOF_EXTRA15,
	KINDOF_EXTRA16,


	KINDOF_COUNT										// total number of kindofs

};

typedef BitFlags<KINDOF_COUNT>	KindOfMaskType;

#define MAKE_KINDOF_MASK(k) KindOfMaskType(KindOfMaskType::kInit, (k))

inline Bool TEST_KINDOFMASK(const KindOfMaskType& m, KindOfType t)
{
	return m.test(t);
}

inline Bool TEST_KINDOFMASK_ANY(const KindOfMaskType& m, const KindOfMaskType& mask)
{
	return m.anyIntersectionWith(mask);
}

inline Bool TEST_KINDOFMASK_MULTI(const KindOfMaskType& m, const KindOfMaskType& mustBeSet, const KindOfMaskType& mustBeClear)
{
	return m.testSetAndClear(mustBeSet, mustBeClear);
}

inline Bool KINDOFMASK_ANY_SET(const KindOfMaskType& m)
{
	return m.any();
}

inline void CLEAR_KINDOFMASK(KindOfMaskType& m)
{
	m.clear();
}

inline void SET_ALL_KINDOFMASK_BITS(KindOfMaskType& m)
{
	m.clear();
	m.flip();
}

inline void FLIP_KINDOFMASK(KindOfMaskType& m)
{
	m.flip();
}

// defined in Common/System/Kindof.cpp
extern KindOfMaskType KINDOFMASK_NONE;	// inits to all zeroes
extern KindOfMaskType KINDOFMASK_FS;		// Initializes all FS types for faction structures.
void initKindOfMasks();

#endif	// __KINDOF_H_


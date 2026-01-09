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

// FILE: Weapon.cpp ///////////////////////////////////////////////////////////////////////////////
// Author: Colin Day, November 2001
// Desc:   Weapon descriptions
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#define DEFINE_DEATH_NAMES
#define DEFINE_WEAPONBONUSCONDITION_NAMES
#define DEFINE_WEAPONBONUSFIELD_NAMES
#define DEFINE_WEAPONCOLLIDEMASK_NAMES
#define DEFINE_WEAPONAFFECTSMASK_NAMES
#define DEFINE_WEAPONRELOAD_NAMES
#define DEFINE_WEAPONPREFIRE_NAMES
#define DEFINE_PROTECTION_NAMES
#define DEFINE_MAGNET_FORMULA_NAMES

#include "Common/crc.h"
#include "Common/CRCDebug.h"
#include "Common/GameAudio.h"
#include "Common/GameState.h"
#include "Common/INI.h"
#include "Common/PerfTimer.h"
#include "Common/Player.h"
#include "Common/ThingFactory.h"
#include "Common/ThingTemplate.h"
#include "Common/Xfer.h"

#include "GameClient/Drawable.h"
#include "GameClient/FXList.h"
#include "GameClient/InGameUI.h"
#include "GameClient/ParticleSys.h"
#include "GameClient/TintStatus.h"
#include "GameClient/GameClient.h"

#include "GameLogic/Damage.h"
#include "GameLogic/ExperienceTracker.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/AIPathfind.h"
#include "GameLogic/Module/BehaviorModule.h"
#include "GameLogic/Module/BodyModule.h"
#include "GameLogic/Module/ContainModule.h"
#include "GameLogic/Module/LaserUpdate.h"
#include "GameLogic/Module/UpdateModule.h"
#include "GameLogic/Module/SpecialPowerCompletionDie.h"
#include "GameLogic/Module/AssaultTransportAIUpdate.h"
#include "GameLogic/Object.h"
#include "GameLogic/ObjectCreationList.h"
#include "GameLogic/PartitionManager.h"
#include "GameLogic/Weapon.h"

#include "GameLogic/Module/AIUpdate.h"
#include "GameLogic/Module/AssistedTargetingUpdate.h"
#include "GameLogic/Module/ProjectileStreamUpdate.h"
#include "GameLogic/Module/PhysicsUpdate.h"
#include "GameLogic/Module/LifetimeUpdate.h"
#include "GameLogic/TerrainLogic.h"

#define RATIONALIZE_ATTACK_RANGE
#define ATTACK_RANGE_IS_2D

#ifdef ATTACK_RANGE_IS_2D
	const DistanceCalculationType ATTACK_RANGE_CALC_TYPE = FROM_BOUNDINGSPHERE_2D;
#else
	const DistanceCalculationType ATTACK_RANGE_CALC_TYPE = FROM_BOUNDINGSPHERE_3D;
#endif


// damage is ALWAYS 3d
const DistanceCalculationType DAMAGE_RANGE_CALC_TYPE = FROM_BOUNDINGSPHERE_3D;

//-------------------------------------------------------------------------------------------------
static void parsePerVetLevelAsciiString( INI* ini, void* /*instance*/, void * store, const void* /*userData*/ )
{
	AsciiString* s = (AsciiString*)store;
	VeterancyLevel v = (VeterancyLevel)INI::scanIndexList(ini->getNextToken(), TheVeterancyNames);
	s[v] = ini->getNextAsciiString();
}

//-------------------------------------------------------------------------------------------------
static void parseAllVetLevelsAsciiString( INI* ini, void* /*instance*/, void * store, const void* /*userData*/ )
{
	AsciiString* s = (AsciiString*)store;
	AsciiString a = ini->getNextAsciiString();
	for (Int i = LEVEL_FIRST; i <= LEVEL_LAST; ++i)
		s[i] = a;
}

//-------------------------------------------------------------------------------------------------
static void parsePerVetLevelFXList( INI* ini, void* /*instance*/, void * store, const void* /*userData*/ )
{
	typedef const FXList* ConstFXListPtr;
	ConstFXListPtr* s = (ConstFXListPtr*)store;
	VeterancyLevel v = (VeterancyLevel)INI::scanIndexList(ini->getNextToken(), TheVeterancyNames);
	const FXList* fx = NULL;
	INI::parseFXList(ini, NULL, &fx, NULL);
	s[v] = fx;
}

//-------------------------------------------------------------------------------------------------
static void parseAllVetLevelsFXList( INI* ini, void* /*instance*/, void * store, const void* /*userData*/ )
{
	typedef const FXList* ConstFXListPtr;
	ConstFXListPtr* s = (ConstFXListPtr*)store;
	const FXList* fx = NULL;
	INI::parseFXList(ini, NULL, &fx, NULL);
	for (Int i = LEVEL_FIRST; i <= LEVEL_LAST; ++i)
		s[i] = fx;
}

//-------------------------------------------------------------------------------------------------
static void parsePerVetLevelPSys( INI* ini, void* /*instance*/, void * store, const void* /*userData*/ )
{
	typedef const ParticleSystemTemplate* ConstParticleSystemTemplatePtr;
	ConstParticleSystemTemplatePtr* s = (ConstParticleSystemTemplatePtr*)store;
	VeterancyLevel v = (VeterancyLevel)INI::scanIndexList(ini->getNextToken(), TheVeterancyNames);
	ConstParticleSystemTemplatePtr pst = NULL;
	INI::parseParticleSystemTemplate(ini, NULL, &pst, NULL);
	s[v] = pst;
}

//-------------------------------------------------------------------------------------------------
static void parseAllVetLevelsPSys( INI* ini, void* /*instance*/, void * store, const void* /*userData*/ )
{
	typedef const ParticleSystemTemplate* ConstParticleSystemTemplatePtr;
	ConstParticleSystemTemplatePtr* s = (ConstParticleSystemTemplatePtr*)store;
	ConstParticleSystemTemplatePtr pst = NULL;
	INI::parseParticleSystemTemplate(ini, NULL, &pst, NULL);
	for (Int i = LEVEL_FIRST; i <= LEVEL_LAST; ++i)
		s[i] = pst;
}

//-------------------------------------------------------------------------------------------------
static void parsePerVetLevelBool( INI* ini, void* /*instance*/, void * store, const void* /*userData*/ )
{
	Bool* s = (Bool*)store;
	VeterancyLevel v = (VeterancyLevel)INI::scanIndexList(ini->getNextToken(), TheVeterancyNames);
	s[v] = INI::scanBool(ini->getNextToken());
}

//-------------------------------------------------------------------------------------------------
static void parseAllVetLevelsBool( INI* ini, void* /*instance*/, void * store, const void* /*userData*/ )
{
	Bool* s = (Bool*)store;
	Bool a = INI::scanBool(ini->getNextToken());
	for (Int i = LEVEL_FIRST; i <= LEVEL_LAST; ++i)
		s[i] = a;
}

//-------------------------------------------------------------------------------------------------
static void parsePerVetLevelReal( INI* ini, void* /*instance*/, void * store, const void* /*userData*/ )
{
	Real* s = (Real*)store;
	VeterancyLevel v = (VeterancyLevel)INI::scanIndexList(ini->getNextToken(), TheVeterancyNames);
	s[v] = INI::scanReal(ini->getNextToken());
}

//-------------------------------------------------------------------------------------------------
static void parseAllVetLevelsReal( INI* ini, void* /*instance*/, void * store, const void* /*userData*/ )
{
	Real* s = (Real*)store;
	Real a = INI::scanReal(ini->getNextToken());
	for (Int i = LEVEL_FIRST; i <= LEVEL_LAST; ++i)
		s[i] = a;
}

//-------------------------------------------------------------------------------------------------
static void parsePerVetLevelTintStatus( INI* ini, void* /*instance*/, void * store, const void* /*userData*/ )
{
	TintStatus* s = (TintStatus*)store;
	VeterancyLevel v = (VeterancyLevel)INI::scanIndexList(ini->getNextToken(), TheVeterancyNames);
	TintStatus t = TINT_STATUS_INVALID;
	TintStatusFlags::parseSingleBitFromINI(ini, NULL, &t, NULL);
	s[v] = t;
}

//-------------------------------------------------------------------------------------------------
static void parseAllVetLevelsTintStatus( INI* ini, void* /*instance*/, void * store, const void* /*userData*/ )
{
	TintStatus* s = (TintStatus*)store;
	TintStatus t = TINT_STATUS_INVALID;
	TintStatusFlags::parseSingleBitFromINI(ini, NULL, &t, NULL);
	for (Int i = LEVEL_FIRST; i <= LEVEL_LAST; ++i)
		s[i] = t;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC DATA ////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
WeaponStore *TheWeaponStore = NULL;					///< the weapon store definition


///////////////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE DATA ///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
const FieldParse WeaponTemplate::TheWeaponTemplateFieldParseTable[] =
{

	{ "PrimaryDamage",						INI::parseReal,													NULL,							offsetof(WeaponTemplate, m_primaryDamage) },
	{ "PrimaryDamageRadius",			INI::parseReal,													NULL,							offsetof(WeaponTemplate, m_primaryDamageRadius) },
	{ "SecondaryDamage",					INI::parseReal,													NULL,							offsetof(WeaponTemplate, m_secondaryDamage) },
	{ "SecondaryDamageRadius",		INI::parseReal,													NULL,							offsetof(WeaponTemplate, m_secondaryDamageRadius) },
	{ "ShockWaveAmount",					INI::parseReal,													NULL,							offsetof(WeaponTemplate, m_shockWaveAmount) },
	{ "ShockWaveRadius",					INI::parseReal,													NULL,							offsetof(WeaponTemplate, m_shockWaveRadius) },
	{ "ShockWaveTaperOff",				INI::parseReal,													NULL,							offsetof(WeaponTemplate, m_shockWaveTaperOff) },
	{ "AttackRange",							INI::parseReal,													NULL,							offsetof(WeaponTemplate, m_attackRange) },
	{ "MinimumAttackRange",				INI::parseReal,													NULL,							offsetof(WeaponTemplate, m_minimumAttackRange) },
	{ "RequestAssistRange",				INI::parseReal,													NULL,							offsetof(WeaponTemplate, m_requestAssistRange) },
	{ "AcceptableAimDelta",				INI::parseAngleReal,										NULL,							offsetof(WeaponTemplate, m_aimDelta) },
	{ "ScatterRadius",						INI::parseReal,													NULL,							offsetof(WeaponTemplate, m_scatterRadius) },
	{ "ScatterTargetScalar",			INI::parseReal,													NULL,							offsetof(WeaponTemplate, m_scatterTargetScalar) },
	{ "ScatterRadiusVsInfantry",	INI::parseReal,													NULL,							offsetof( WeaponTemplate, m_infantryInaccuracyDist ) },
	{ "DamageType",								DamageTypeFlags::parseSingleBitFromINI,	NULL,							offsetof(WeaponTemplate, m_damageType) },
	{ "DamageStatusType",					ObjectStatusMaskType::parseSingleBitFromINI,	NULL,				offsetof(WeaponTemplate, m_damageStatusType) },
	{ "DeathType",								INI::parseIndexList,										TheDeathNames,		offsetof(WeaponTemplate, m_deathType) },
	{ "WeaponSpeed",							INI::parseVelocityReal,									NULL,							offsetof(WeaponTemplate, m_weaponSpeed) },
	{ "MinWeaponSpeed",						INI::parseVelocityReal,									NULL,							offsetof(WeaponTemplate, m_minWeaponSpeed) },
	{ "ScaleWeaponSpeed",					INI::parseBool,													NULL,							offsetof(WeaponTemplate, m_isScaleWeaponSpeed) },
	{ "WeaponRecoil",							INI::parseAngleReal,										NULL,							offsetof(WeaponTemplate, m_weaponRecoil) },
	{ "MinTargetPitch",						INI::parseAngleReal,										NULL,							offsetof(WeaponTemplate, m_minTargetPitch) },
	{ "MaxTargetPitch",						INI::parseAngleReal,										NULL,							offsetof(WeaponTemplate, m_maxTargetPitch) },
	{ "RadiusDamageAngle",				INI::parseAngleReal,										NULL,							offsetof(WeaponTemplate, m_radiusDamageAngle) },
	{ "ProjectileObject",					INI::parseAsciiString,									NULL,							offsetof(WeaponTemplate, m_projectileName) },
	{ "FireSound",								INI::parseAudioEventRTS,								NULL,							offsetof(WeaponTemplate, m_fireSound) },
	{ "FireSoundLoopTime",				INI::parseDurationUnsignedInt,					NULL,							offsetof(WeaponTemplate, m_fireSoundLoopTime) },
	{ "FireFX",											parseAllVetLevelsFXList,							NULL,							offsetof(WeaponTemplate, m_fireFXs) },
	{ "ProjectileDetonationFX",			parseAllVetLevelsFXList,							NULL,							offsetof(WeaponTemplate, m_projectileDetonateFXs) },
	{ "FireOCL",										parseAllVetLevelsAsciiString,					NULL,							offsetof(WeaponTemplate, m_fireOCLNames) },
	{ "ProjectileDetonationOCL",		parseAllVetLevelsAsciiString,					NULL,							offsetof(WeaponTemplate, m_projectileDetonationOCLNames) },
	{ "ProjectileExhaust",					parseAllVetLevelsPSys,								NULL,							offsetof(WeaponTemplate, m_projectileExhausts) },
	{ "VeterancyFireFX",										parsePerVetLevelFXList,				NULL,							offsetof(WeaponTemplate, m_fireFXs) },
	{ "VeterancyProjectileDetonationFX",		parsePerVetLevelFXList,				NULL,							offsetof(WeaponTemplate, m_projectileDetonateFXs) },
	{ "VeterancyFireOCL",										parsePerVetLevelAsciiString,	NULL,							offsetof(WeaponTemplate, m_fireOCLNames) },
	{ "VeterancyProjectileDetonationOCL",		parsePerVetLevelAsciiString,	NULL,							offsetof(WeaponTemplate, m_projectileDetonationOCLNames) },
	{ "VeterancyProjectileExhaust",					parsePerVetLevelPSys,					NULL,							offsetof(WeaponTemplate, m_projectileExhausts) },
	{ "ClipSize",									INI::parseInt,													NULL,							offsetof(WeaponTemplate, m_clipSize) },
	{ "ContinuousFireOne",				INI::parseInt,													NULL,							offsetof(WeaponTemplate, m_continuousFireOneShotsNeeded) },
	{ "ContinuousFireTwo",				INI::parseInt,													NULL,							offsetof(WeaponTemplate, m_continuousFireTwoShotsNeeded) },
	{ "ContinuousFireCoast",			INI::parseDurationUnsignedInt,					NULL,							offsetof(WeaponTemplate, m_continuousFireCoastFrames) },
 	{ "AutoReloadWhenIdle",				INI::parseDurationUnsignedInt,					NULL,							offsetof(WeaponTemplate, m_autoReloadWhenIdleFrames) },
	{ "ClipReloadTime",						INI::parseDurationUnsignedInt,					NULL,							offsetof(WeaponTemplate, m_clipReloadTime) },
	{ "DelayBetweenShots",				WeaponTemplate::parseShotDelay,					NULL,							0 },
	{ "ShotsPerBarrel",						INI::parseInt,													NULL,							offsetof(WeaponTemplate, m_shotsPerBarrel) },
	{ "DamageDealtAtSelfPosition",INI::parseBool,													NULL,							offsetof(WeaponTemplate, m_damageDealtAtSelfPosition) },
	{ "RadiusDamageAffects",			INI::parseBitString32,	TheWeaponAffectsMaskNames,				offsetof(WeaponTemplate, m_affectsMask) },
	{ "ProjectileCollidesWith",		INI::parseBitString32,	TheWeaponCollideMaskNames,				offsetof(WeaponTemplate, m_collideMask) },
	{ "AntiAirborneVehicle",			INI::parseBitInInt32,										(void*)WEAPON_ANTI_AIRBORNE_VEHICLE,	offsetof(WeaponTemplate, m_antiMask) },
	{ "AntiGround",								INI::parseBitInInt32,										(void*)WEAPON_ANTI_GROUND,						offsetof(WeaponTemplate, m_antiMask) },
	{ "AntiProjectile",						INI::parseBitInInt32,										(void*)WEAPON_ANTI_PROJECTILE,				offsetof(WeaponTemplate, m_antiMask) },
	{ "AntiSmallMissile",					INI::parseBitInInt32,										(void*)WEAPON_ANTI_SMALL_MISSILE,			offsetof(WeaponTemplate, m_antiMask) },
	{ "AntiMine",									INI::parseBitInInt32,										(void*)WEAPON_ANTI_MINE,							offsetof(WeaponTemplate, m_antiMask) },
	{ "AntiParachute",						INI::parseBitInInt32,										(void*)WEAPON_ANTI_PARACHUTE,					offsetof(WeaponTemplate, m_antiMask) },
	{ "AntiAirborneInfantry",			INI::parseBitInInt32,										(void*)WEAPON_ANTI_AIRBORNE_INFANTRY, offsetof(WeaponTemplate, m_antiMask) },
	{ "AntiBallisticMissile",			INI::parseBitInInt32,										(void*)WEAPON_ANTI_BALLISTIC_MISSILE, offsetof(WeaponTemplate, m_antiMask) },
	{ "AutoReloadsClip",					INI::parseIndexList,										TheWeaponReloadNames,							offsetof(WeaponTemplate, m_reloadType) },
	{ "ProjectileStreamName",			INI::parseAsciiString,									NULL,							offsetof(WeaponTemplate, m_projectileStreamName) },
	{ "LaserName",								INI::parseAsciiString,									NULL,							offsetof(WeaponTemplate, m_laserName) },
	{ "LaserBoneName",						INI::parseAsciiString,									NULL,							offsetof(WeaponTemplate, m_laserBoneName) },
	{ "WeaponBonus",							WeaponTemplate::parseWeaponBonusSet,		NULL,							0 },
	{ "CustomWeaponBonus",						WeaponTemplate::parseCustomWeaponBonusSet,		NULL,							0 },
	{ "HistoricBonusTime",				INI::parseDurationUnsignedInt,					NULL,							offsetof(WeaponTemplate, m_historicBonusTime) },
	{ "HistoricBonusRadius",			INI::parseReal,													NULL,							offsetof(WeaponTemplate, m_historicBonusRadius) },
	{ "HistoricBonusCount",				INI::parseInt,													NULL,							offsetof(WeaponTemplate, m_historicBonusCount) },
	{ "HistoricBonusWeapon",			INI::parseWeaponTemplate,								NULL,							offsetof(WeaponTemplate, m_historicBonusWeapon) },
	{ "LeechRangeWeapon",					INI::parseBool,													NULL,							offsetof(WeaponTemplate, m_leechRangeWeapon) },
	{ "ScatterTarget",						WeaponTemplate::parseScatterTarget,			NULL,							0 },
	{ "CapableOfFollowingWaypoints", INI::parseBool,											NULL,							offsetof(WeaponTemplate, m_capableOfFollowingWaypoint) },
	{ "ShowsAmmoPips",						INI::parseBool,													NULL,							offsetof(WeaponTemplate, m_isShowsAmmoPips) },
	{ "AllowAttackGarrisonedBldgs", INI::parseBool,												NULL,							offsetof(WeaponTemplate, m_allowAttackGarrisonedBldgs) },
	{ "PlayFXWhenStealthed",			INI::parseBool,													NULL,							offsetof(WeaponTemplate, m_playFXWhenStealthed) },
	{ "PreAttackDelay",						INI::parseDurationUnsignedInt,					NULL,							offsetof( WeaponTemplate, m_preAttackDelay ) },
	{ "PreAttackType",						INI::parseIndexList,										TheWeaponPrefireNames, offsetof(WeaponTemplate, m_prefireType) },
	{ "ContinueAttackRange",			INI::parseReal,													NULL,							offsetof(WeaponTemplate, m_continueAttackRange) },
	{ "SuspendFXDelay",						INI::parseDurationUnsignedInt,					NULL,							offsetof(WeaponTemplate, m_suspendFXDelay) },
	{ "MissileCallsOnDie",			INI::parseBool,													NULL,							offsetof(WeaponTemplate, m_dieOnDetonate) },
	{ "ScatterTargetAligned", INI::parseBool, NULL, offsetof(WeaponTemplate, m_scatterTargetAligned) },
	{ "ScatterTargetRandomOrder", INI::parseBool, NULL, offsetof(WeaponTemplate, m_scatterTargetRandom) },
	{ "ScatterTargetRandomAngle", INI::parseBool, NULL, offsetof(WeaponTemplate, m_scatterTargetRandomAngle) },
	{ "ScatterTargetMinScalar", INI::parseReal, NULL, offsetof(WeaponTemplate, m_scatterTargetMinScalar) },
	{ "ScatterTargetCenteredAtShooter", INI::parseBool, NULL, offsetof(WeaponTemplate, m_scatterTargetCenteredAtShooter) },
	{ "ScatterTargetResetTime", INI::parseDurationUnsignedInt, NULL, offsetof(WeaponTemplate, m_scatterTargetResetTime) },
	{ "ScatterTargetResetRecenter", INI::parseBool, NULL, offsetof(WeaponTemplate, m_scatterTargetResetRecenter) },
	{ "PreAttackFX", parseAllVetLevelsFXList, NULL,	offsetof(WeaponTemplate, m_preAttackFXs) },
	{ "VeterancyPreAttackFX", parsePerVetLevelFXList, NULL, offsetof(WeaponTemplate, m_preAttackFXs) },
	{ "PreAttackFXDelay",						INI::parseDurationUnsignedInt,					NULL,							offsetof(WeaponTemplate, m_preAttackFXDelay) },
	{ "ContinuousLaserLoopTime",				INI::parseDurationUnsignedInt,					NULL,							offsetof(WeaponTemplate, m_continuousLaserLoopTime) },
	{ "LaserGroundTargetHeight",				INI::parseReal,					NULL,							offsetof(WeaponTemplate, m_laserGroundTargetHeight) },
	{ "LaserGroundUnitTargetHeight",				INI::parseReal,					NULL,					offsetof(WeaponTemplate, m_laserGroundUnitTargetHeight) },
	{ "ScatterOnWaterSurface", INI::parseBool, NULL, offsetof(WeaponTemplate, m_scatterOnWaterSurface) },
	
	// New Features
	{ "CustomDamageType",						INI::parseAsciiString,	NULL,							offsetof(WeaponTemplate, m_customDamageType) },
	{ "CustomDamageStatusType",					INI::parseAsciiString,	NULL,							offsetof(WeaponTemplate, m_customDamageStatusType) },
	{ "CustomDeathType",						INI::parseAsciiString,	NULL,							offsetof(WeaponTemplate, m_customDeathType) },

	{ "WeaponIsFlame",							INI::parseBool,			NULL,							offsetof(WeaponTemplate, m_isFlame) },
	{ "WeaponFlameCollidesWithBurning",			INI::parseBool,			NULL,							offsetof(WeaponTemplate, m_projectileCollidesWithBurn) },
	{ "WeaponIsPoison",							INI::parseBool,			NULL,							offsetof(WeaponTemplate, m_isPoison) },
	{ "WeaponPoisonMuzzleFlashesGarrison",		INI::parseBool,			NULL,							offsetof(WeaponTemplate, m_poisonMuzzleFlashesGarrison) },
	{ "WeaponIsDisarm",							INI::parseBool,			NULL,							offsetof(WeaponTemplate, m_isDisarm) },
	{ "WeaponKillsGarrison",					INI::parseBool,			NULL,							offsetof(WeaponTemplate, m_killsGarrison) },
	{ "WeaponKillsGarrisonAmount",				INI::parseInt,			NULL,							offsetof(WeaponTemplate, m_killsGarrisonAmount) },

	{ "WeaponUseSpecificVoice",					INI::parseAsciiString,	NULL,							offsetof(WeaponTemplate, m_playSpecificVoice) },
	{ "OverrideDamageTypeFX",					DamageTypeFlags::parseSingleBitFromINI,	NULL,			offsetof(WeaponTemplate, m_damageFXOverride) },

	{ "WeaponDoStatusDamageType",				parseAllVetLevelsBool,			NULL,					offsetof(WeaponTemplate, m_doStatusDamage) },
	{ "VeterancyWeaponDoStatusDamageType",		parsePerVetLevelBool,			NULL,					offsetof(WeaponTemplate, m_doStatusDamage) },
	{ "WeaponStatusDuration",					INI::parseReal,			NULL, 							offsetof(WeaponTemplate, m_statusDuration) },
	{ "WeaponStatusDurationDamageCorrelation",	INI::parseBool,			NULL,							offsetof(WeaponTemplate, m_statusDurationTypeCorrelate) },
	{ "WeaponStatusTintStatus",					parseAllVetLevelsTintStatus,				NULL,		offsetof(WeaponTemplate, m_tintStatus) },
	{ "VeterancyWeaponStatusTintStatus",		parsePerVetLevelTintStatus,					NULL,		offsetof(WeaponTemplate, m_tintStatus) },
	{ "WeaponStatusCustomTintStatus",			parseAllVetLevelsAsciiString,				NULL, 		offsetof(WeaponTemplate, m_customTintStatus) },
	{ "VeterancyWeaponStatusCustomTintStatus",	parsePerVetLevelAsciiString,				NULL, 		offsetof(WeaponTemplate, m_customTintStatus) },

	{ "WeaponIsSubdual",						parseAllVetLevelsBool,			NULL,					offsetof(WeaponTemplate, m_isSubdual) },
	{ "VeterancyWeaponIsSubdual",				parsePerVetLevelBool,			NULL,					offsetof(WeaponTemplate, m_isSubdual) },
	{ "SubdualDealsNormalDamage",				parseAllVetLevelsBool,			NULL,					offsetof(WeaponTemplate, m_subdualDealsNormalDamage) },
	{ "VeterancySubdualDealsNormalDamage",		parsePerVetLevelBool,			NULL,					offsetof(WeaponTemplate, m_subdualDealsNormalDamage) },
	{ "SubdualDamageMultiplier",				parseAllVetLevelsReal,			NULL,					offsetof(WeaponTemplate, m_subdualDamageMultiplier) },
	{ "VeterancySubdualDamageMultiplier",		parsePerVetLevelReal,			NULL,					offsetof(WeaponTemplate, m_subdualDamageMultiplier) },
	{ "SubdualForbiddenKindOf",					KindOfMaskType::parseFromINI,	NULL, 					offsetof( WeaponTemplate, m_subdualForbiddenKindOf ) },

	{ "FiringTrackerStatusTrigger",				ObjectStatusMaskType::parseSingleBitFromINIVector,	NULL,		offsetof(WeaponTemplate, m_firingTrackerStatusTrigger) },	
	{ "FiringTrackerBonusConditionType",		INI::parseIndexList,	TheWeaponBonusNames, 				offsetof(WeaponTemplate, m_firingTrackerBonusConditionGive ) },
	{ "FiringTrackerCustomStatusTrigger",				INI::parseAsciiStringVector,	NULL,			offsetof(WeaponTemplate, m_firingTrackerCustomStatusTrigger) },
	{ "FiringTrackerCustomBonusConditionType",			INI::parseQuotedAsciiString,	NULL,			offsetof(WeaponTemplate, m_firingTrackerCustomBonusConditionGive ) },

	{ "WeaponNotAbsoluteKill",						INI::parseBool,			NULL,							offsetof(WeaponTemplate, m_notAbsoluteKill) },
	
	{ "SubduedProjectileNoDamage",					INI::parseBool,			NULL,							offsetof(WeaponTemplate, m_subduedProjectileNoDamage) },
	{ "SubdualMissileAttractor",					INI::parseBool,			NULL,							offsetof(WeaponTemplate, m_isMissileAttractor) },
	{ "ShielderProtectionType",						INI::parseProtectionTypeFlags,	NULL, 					offsetof(WeaponTemplate, m_protectionTypes) },
	{ "IgnoresShielder",							INI::parseBool,			NULL,							offsetof(WeaponTemplate, m_isShielderImmune) },

	{ "SubdualCustomType",							INI::parseAsciiString,	NULL,							offsetof(WeaponTemplate, m_subdualCustomType) },
	{ "CustomSubdualTintStatus",					parseAllVetLevelsTintStatus,		NULL,				offsetof(WeaponTemplate, m_customSubdualTint) },
	{ "CustomSubdualCustomTintStatus",				parseAllVetLevelsAsciiString,		NULL, 				offsetof(WeaponTemplate, m_customSubdualCustomTint) },
	{ "CustomSubdualHasDisable",					parseAllVetLevelsBool,			NULL,					offsetof(WeaponTemplate, m_customSubdualHasDisable) },
	{ "CustomSubdualHasDisableProjectile",			parseAllVetLevelsBool,			NULL,					offsetof(WeaponTemplate, m_customSubdualHasDisableProjectiles) },
	{ "CustomSubdualClearOnTrigger",				parseAllVetLevelsBool,			NULL,					offsetof(WeaponTemplate, m_customSubdualClearOnTrigger) },
	{ "CustomSubdualDoStatus",						parseAllVetLevelsBool,			NULL,					offsetof(WeaponTemplate, m_customSubdualDoStatus) },
	{ "CustomSubdualOCL",							parseAllVetLevelsAsciiString,	NULL,					offsetof(WeaponTemplate, m_customSubdualOCLNames) },
	{ "CustomSubdualDisableType",					DisabledMaskType::parseSingleBitFromINI, NULL,			offsetof(WeaponTemplate, m_customSubdualDisableType ) },
	{ "CustomSubdualDisableTint",					TintStatusFlags::parseSingleBitFromINI,		NULL,		offsetof(WeaponTemplate, m_customSubdualDisableTint) },
	{ "CustomSubdualDisableCustomTint",				INI::parseAsciiString,			NULL, 					offsetof(WeaponTemplate, m_customSubdualDisableCustomTint) },
	{ "CustomSubdualRemoveSubdualTintOnDisable",	INI::parseBool,					NULL,					offsetof(WeaponTemplate, m_customSubdualRemoveSubdualTintOnDisable) },
	{ "CustomSubdualDisableSound",					INI::parseAsciiString,			NULL, 					offsetof(WeaponTemplate, m_customSubdualDisableSound) },
	{ "CustomSubdualDisableRemoveSound",			INI::parseAsciiString,			NULL, 					offsetof(WeaponTemplate, m_customSubdualDisableRemoveSound) },
	
	{ "VeterancyCustomSubdualTintStatus",			parsePerVetLevelTintStatus,			NULL,				offsetof(WeaponTemplate, m_customSubdualTint) },
	{ "VeterancyCustomSubdualCustomTintStatus",		parsePerVetLevelAsciiString,		NULL, 				offsetof(WeaponTemplate, m_customSubdualCustomTint) },
	{ "VeterancyCustomSubdualHasDisable",			parsePerVetLevelBool,			NULL,					offsetof(WeaponTemplate, m_customSubdualHasDisable) },
	{ "VeterancyCustomSubdualHasDisableProjectile",	parsePerVetLevelBool,			NULL,					offsetof(WeaponTemplate, m_customSubdualHasDisableProjectiles) },
	{ "VeterancyCustomSubdualClearOnTrigger",		parsePerVetLevelBool,			NULL,					offsetof(WeaponTemplate, m_customSubdualClearOnTrigger) },
	{ "VeterancyCustomSubdualDoStatus",				parsePerVetLevelBool,			NULL,					offsetof(WeaponTemplate, m_customSubdualDoStatus) },
	{ "VeterancyCustomSubdualOCL",					parsePerVetLevelAsciiString,	NULL,					offsetof(WeaponTemplate, m_customSubdualOCLNames) },

	{ "ShockWaveUseCenter",							INI::parseBool,					NULL,					offsetof(WeaponTemplate, m_shockWaveUseCenter) },
	{ "ShockWaveRespectsCenter",					INI::parseBool,					NULL,					offsetof(WeaponTemplate, m_shockWaveRespectsCenter) },
	{ "ShockWaveAffectsAirborne",					INI::parseBool,					NULL,					offsetof(WeaponTemplate, m_shockWaveAffectsAirborne) },
	{ "ShockWavePullsAirborne",						INI::parseBool,					NULL,					offsetof(WeaponTemplate, m_shockWavePullsAirborne) },
	
	{ "MagnetAmount",								INI::parseReal,					NULL,					offsetof(WeaponTemplate, m_magnetAmount) },
	{ "MagnetInfantryAmount",						INI::parseReal,					NULL,					offsetof(WeaponTemplate, m_magnetInfantryAmount) },
	{ "MagnetTaperOffDistance",						INI::parseReal,					NULL,					offsetof(WeaponTemplate, m_magnetTaperOffDistance) },
	{ "MagnetTaperOffRatio",						INI::parseReal,					NULL,					offsetof(WeaponTemplate, m_magnetTaperOffRatio) },
	{ "MagnetTaperOnDistance",						INI::parseReal,					NULL,					offsetof(WeaponTemplate, m_magnetTaperOnDistance) },
	{ "MagnetTaperOnRatio",							INI::parseReal,					NULL,					offsetof(WeaponTemplate, m_magnetTaperOnRatio) },
	{ "MagnetLiftHeight",							INI::parseReal,					NULL,					offsetof(WeaponTemplate, m_magnetLiftHeight) },
	{ "MagnetLiftHeightSecond",						INI::parseReal,					NULL,					offsetof(WeaponTemplate, m_magnetLiftHeightSecond) },
	{ "MagnetLiftForce",							INI::parseReal,					NULL,					offsetof(WeaponTemplate, m_magnetLiftForce) },
	{ "MagnetLiftForceToHeight",					INI::parseReal,					NULL,					offsetof(WeaponTemplate, m_magnetLiftForceToHeight) },
	{ "MagnetLiftForceToHeightSecond",				INI::parseReal,					NULL,					offsetof(WeaponTemplate, m_magnetLiftForceToHeightSecond) },
	{ "MagnetMaxLiftHeight",						INI::parseReal,					NULL,					offsetof(WeaponTemplate, m_magnetMaxLiftHeight) },
	{ "MagnetLevitationHeight",						INI::parseReal,					NULL,					offsetof(WeaponTemplate, m_magnetLevitationHeight) },
	{ "MagnetMinDistance",							INI::parseReal,					NULL,					offsetof(WeaponTemplate, m_magnetMinDistance) },
	{ "MagnetMaxDistance",							INI::parseReal,					NULL,					offsetof(WeaponTemplate, m_magnetMaxDistance) },
	{ "MagnetLinearDistanceCalc",					INI::parseBool,					NULL,					offsetof(WeaponTemplate, m_magnetLinearDistanceCalc) },
	{ "MagnetNoAirborne",							INI::parseBool,					NULL,					offsetof(WeaponTemplate, m_magnetNoAirborne) },
	{ "MagnetAirborneZForce",						INI::parseReal,					NULL,					offsetof(WeaponTemplate, m_magnetAirborneZForce) },
	{ "MagnetAirborneAffectedByYaw",				INI::parseBool,					NULL,					offsetof(WeaponTemplate, m_magnetAirboneAffectedByYaw) },
	{ "MagnetUseCenter",							INI::parseBool,					NULL,					offsetof(WeaponTemplate, m_magnetUseCenter) },
	{ "MagnetRespectsCenter",						INI::parseBool,					NULL,					offsetof(WeaponTemplate, m_magnetRespectsCenter) },
	{ "MagnetFormula",								INI::parseIndexList,			TheMagnetFormulaNames,	offsetof(WeaponTemplate, m_magnetFormula) },

	{ "MinDamageAltitude",							INI::parseReal,					NULL,					offsetof(WeaponTemplate, m_minDamageHeight) },
	{ "MaxDamageAltitude",							INI::parseReal,					NULL,					offsetof(WeaponTemplate, m_maxDamageHeight) },
	{ "MinTargetAltitude",							INI::parseReal,					NULL,					offsetof(WeaponTemplate, m_minTargetHeight) },
	{ "MaxTargetAltitude",							INI::parseReal,					NULL,					offsetof(WeaponTemplate, m_maxTargetHeight) },
	{ "PriorityWithinAttackRange",					INI::parseInt,					NULL,					offsetof(WeaponTemplate, m_attackRangePriority) },
	{ "PriorityOutsideAttackRange",					INI::parseInt,					NULL,					offsetof(WeaponTemplate, m_outsideAttackRangePriority) },

	{ "TriggeredBy",								INI::parseAsciiStringVector, 	NULL, 					offsetof(WeaponTemplate, m_activationUpgradeNames ) },
	{ "ConflictsWith",								INI::parseAsciiStringVector, 	NULL, 					offsetof(WeaponTemplate, m_conflictingUpgradeNames ) },
	{ "RequiresAllTriggers", 						INI::parseBool, 				NULL, 					offsetof(WeaponTemplate, m_requiresAllTriggers ) },
	{ "RequiredStatus",								ObjectStatusMaskType::parseFromINI,	NULL, 				offsetof(WeaponTemplate, m_requiredStatus ) },
	{ "ForbiddenStatus",							ObjectStatusMaskType::parseFromINI,	NULL, 				offsetof(WeaponTemplate, m_forbiddenStatus ) },
	{ "RequiredCustomStatus",						INI::parseAsciiStringVector, 	NULL, 					offsetof(WeaponTemplate, m_requiredCustomStatus ) },
	{ "ForbiddenCustomStatus",						INI::parseAsciiStringVector, 	NULL, 					offsetof(WeaponTemplate, m_forbiddenCustomStatus ) },

	{ "AttackCursorName",							INI::parseAsciiString,			NULL, 					offsetof(WeaponTemplate, m_cursorName ) },
	{ "ForceAttackObjectCursorName",				INI::parseAsciiString,			NULL, 					offsetof(WeaponTemplate, m_forceAttackObjectCursorName ) },
	{ "ForceAttackGroundCursorName",				INI::parseAsciiString,			NULL, 					offsetof(WeaponTemplate, m_forceAttackGroundCursorName ) },
	{ "InvalidCursorName",							INI::parseAsciiString,       	NULL, 					offsetof(WeaponTemplate, m_invalidCursorName ) },

	{ "DamagesSelfOnly",							INI::parseBool,			NULL,							offsetof(WeaponTemplate, m_damagesSelfOnly) },

	{ "RejectKeys",									INI::parseAsciiStringVector,			NULL,			offsetof(WeaponTemplate, m_rejectKeys) },

	{ "ClearsParasite",								INI::parseBool,			NULL,							offsetof(WeaponTemplate, m_clearsParasite) },
	{ "ClearsParasiteKeys",							INI::parseAsciiStringVector,			NULL,			offsetof(WeaponTemplate, m_clearsParasiteKeys) },

	{ "ROFPenaltyWhenMoving",						INI::parsePercentToReal,			NULL,				offsetof(WeaponTemplate, m_rofMovingPenalty) },
	{ "ROFMovingScalesWithMovingSpeed",				INI::parseBool,			NULL,							offsetof(WeaponTemplate, m_rofMovingScales) },
	{ "ROFMovingScalingSpeedMax",					INI::parseVelocityReal,		NULL,						offsetof(WeaponTemplate, m_rofMovingMaxSpeedCount) },

	{ "InvulnerabilityDuration",					INI::parseDurationUnsignedInt,			NULL,			offsetof(WeaponTemplate, m_invulnerabilityDuration) },

	{ "ShrapnelCount",				INI::parseInt,													NULL,							offsetof(WeaponTemplate, m_shrapnelBonusCount) },
	{ "ShrapnelWeapon",			INI::parseWeaponTemplate,								NULL,							offsetof(WeaponTemplate, m_shrapnelBonusWeapon) },
	{ "ShrapnelDoesNotRequireVictim",				INI::parseBool,					NULL,							offsetof(WeaponTemplate, m_shrapnelDoesNotRequireVictim) },
	{ "ShrapnelCanTargetStealth",				INI::parseBool,					NULL,							offsetof(WeaponTemplate, m_shrapnelIgnoresStealthStatus) },
	{ "ShrapnelTargetMask",			INI::parseBitString32,	TheWeaponAffectsMaskNames,				offsetof(WeaponTemplate, m_shrapnelAffectsMask) },

	{ "IsRailgun",								INI::parseBool,					NULL,							offsetof(WeaponTemplate, m_isRailgun) },
	{ "RailgunIsLinear",						INI::parseBool,					NULL,							offsetof(WeaponTemplate, m_railgunIsLinear) },
	{ "RailgunUsesSecondaryDamage",				INI::parseBool,					NULL,							offsetof(WeaponTemplate, m_railgunUsesSecondaryDamage) },
	{ "RailgunPiercesBehind",					INI::parseBool,					NULL,							offsetof(WeaponTemplate, m_railgunPiercesBehind) },
	{ "RailgunPierceAmount",					INI::parseInt,					NULL,							offsetof(WeaponTemplate, m_railgunPierceAmount) },
	{ "RailgunRadius",							INI::parseReal,					NULL,							offsetof(WeaponTemplate, m_railgunRadius) },
	{ "RailgunRadiusCheckPerDistance",			INI::parseReal,					NULL,							offsetof(WeaponTemplate, m_railgunRadiusCheckPerDistance) },
	{ "RailgunInfantryRadius",					INI::parseReal,					NULL,							offsetof(WeaponTemplate, m_railgunInfantryRadius) },
	{ "RailgunExtraDistance",					INI::parseReal,					NULL,							offsetof(WeaponTemplate, m_railgunExtraDistance) },
	{ "RailgunMaxDistance",						INI::parsePositiveNonZeroReal,	NULL,							offsetof(WeaponTemplate, m_railgunMaxDistance) },
	{ "RailgunDamageType",						DamageTypeFlags::parseSingleBitFromINI,	NULL,							offsetof(WeaponTemplate, m_railgunDamageType) },
	{ "RailgunDeathType",						INI::parseIndexList,										TheDeathNames,		offsetof(WeaponTemplate, m_railgunDeathType) },
	{ "RailgunOverrideDamageTypeFX",			DamageTypeFlags::parseSingleBitFromINI,	NULL,			offsetof(WeaponTemplate, m_railgunDamageFXOverride) },
	{ "RailgunCustomDamageType",				INI::parseAsciiString,			NULL,							offsetof(WeaponTemplate, m_railgunCustomDamageType) },
	{ "RailgunCustomDeathType",					INI::parseAsciiString,			NULL,							offsetof(WeaponTemplate, m_railgunCustomDeathType) },
	{ "RailgunFX",								parseAllVetLevelsFXList,		NULL,							offsetof(WeaponTemplate, m_railgunFXs) },
	{ "RailgunOCL",								parseAllVetLevelsAsciiString,	NULL,							offsetof(WeaponTemplate, m_railgunOCLNames) },
	{ "RailgunVeterancyFX",						parsePerVetLevelFXList,			NULL,							offsetof(WeaponTemplate, m_railgunFXs) },
	{ "RailgunVeterancyOCL",					parsePerVetLevelAsciiString,	NULL,							offsetof(WeaponTemplate, m_railgunOCLNames) },

	{ "UserBypassLineOfSight",					INI::parseBool,					NULL,							offsetof(WeaponTemplate, m_weaponBypassLineOfSight) },
	{ "UserIgnoresObstacles",					INI::parseBool,					NULL,							offsetof(WeaponTemplate, m_weaponIgnoresObstacles) },

	/*{ "IsMindControl",				INI::parseBool,													NULL,							offsetof(WeaponTemplate, m_isMindControl) },
	{ "MindControlRadius",				INI::parseBool,					NULL,							offsetof(WeaponTemplate, m_mindControlRadius) },
	{ "MindControlCount",				INI::parseInt,					NULL,							offsetof(WeaponTemplate, m_mindControlCount) },
	{ "MindControlTurnsNeutral",			INI::parseBool,								NULL,							offsetof(WeaponTemplate, m_mindControlTurnsNeutral) },
	{ "MindControlIsPermanent",				INI::parseBool,					NULL,							offsetof(WeaponTemplate, m_mindControlIsPermanent) },
	{ "MindControlDefectDuration",			INI::parseDurationUnsignedInt,						NULL,							offsetof(WeaponTemplate, m_mindControlDefectDuration) },
	{ "MindControlKey",				INI::parseAsciiString,					NULL,				offsetof(WeaponTemplate, m_mindControlKey) },
	{ "MindControlAnimTemplate",				INI::parseAsciiString,					NULL,				offsetof(WeaponTemplate, m_mindControlAnimTemplate) },

	{ "CustomSubdualDoMindControl",				INI::parseBool,					NULL,							offsetof(WeaponTemplate, m_customSubdualDoMindControl) },

	{ "BreaksMindControl",				INI::parseBool,					NULL,							offsetof(WeaponTemplate, m_customSubdualDoMindControl) },
	{ "BreaksMindControlKey",				INI::parseAsciiStringVector,					NULL,				offsetof(WeaponTemplate, m_customSubdualDoMindControl) },*/

	{ NULL,												NULL,																		NULL,							0 }

};

///////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS ///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------------------------------
WeaponTemplate::WeaponTemplate() : m_nextTemplate(NULL)
{

	m_name													= "NoNameWeapon";
	m_nameKey												= NAMEKEY_INVALID;
	m_primaryDamage									= 0.0f;
	m_primaryDamageRadius						= 0.0f;
	m_secondaryDamage								= 0.0f;
	m_secondaryDamageRadius					= 0.0f;
	m_attackRange										= 0.0f;
	m_minimumAttackRange						= 0.0f;
	m_requestAssistRange						= 0.0f;
	m_aimDelta											= 0.0f;
	m_scatterRadius									= 0.0f;
	m_scatterTargetScalar						= 0.0f;
	m_shockWaveAmount								= 0.0f;
	m_shockWaveRadius								= 0.0f;
	m_shockWaveTaperOff							= 0.0f;
	m_damageType										= DAMAGE_EXPLOSION;
	m_deathType											= DEATH_NORMAL;
	m_weaponSpeed										= 999999.0f;	// effectively instant
	m_minWeaponSpeed								= 999999.0f;	// effectively instant
	m_isScaleWeaponSpeed						= FALSE;
	m_weaponRecoil									= 0.0f;		// no recoil
	m_minTargetPitch								= -PI;
	m_maxTargetPitch								= PI;
	m_radiusDamageAngle							= PI;	// PI each way, so full circle
	m_projectileName.clear();					// no projectile
	m_projectileTmpl								= NULL;
	for (Int i = LEVEL_FIRST; i <= LEVEL_LAST; ++i)
	{
		m_fireOCLNames[i].clear();
		m_projectileDetonationOCLNames[i].clear();
		m_projectileExhausts[i]					= NULL;
		m_fireOCLs[i]										= NULL;
		m_projectileDetonationOCLs[i]		= NULL;
		m_fireFXs[i]										= NULL;
		m_projectileDetonateFXs[i]			= NULL;

		m_doStatusDamage[i] = FALSE;
		m_tintStatus[i] = TINT_STATUS_INVALID;
		m_customTintStatus[i] = NULL;

		m_isSubdual[i] = FALSE;
		m_subdualDealsNormalDamage[i] = FALSE;
		m_subdualDamageMultiplier[i] = 1.0f;

		m_customSubdualCustomTint[i] = NULL;
		m_customSubdualTint[i] = TINT_STATUS_GAINING_SUBDUAL_DAMAGE;
		m_customSubdualHasDisable[i] = TRUE;
		m_customSubdualHasDisableProjectiles[i] = TRUE;
		m_customSubdualClearOnTrigger[i] = FALSE;
		m_customSubdualDoStatus[i] = FALSE;
		m_customSubdualOCLNames[i].clear();
		m_customSubdualOCLs[i] = NULL;

		m_railgunOCLNames[i].clear();
		m_railgunOCLs[i] = NULL;
		m_railgunFXs[i] = NULL;
	}
	m_damageDealtAtSelfPosition			= false;
	m_affectsMask										= (WEAPON_AFFECTS_ALLIES | WEAPON_AFFECTS_ENEMIES | WEAPON_AFFECTS_NEUTRALS);
	// most projectile weapons don't want to collide with nontargeted enemies/allies or trees...
	m_collideMask										= (WEAPON_COLLIDE_STRUCTURES);
	m_reloadType										= AUTO_RELOAD;
	m_prefireType										= PREFIRE_PER_SHOT;
	m_clipSize											= 0;
	m_continuousFireOneShotsNeeded	= INT_MAX;
	m_continuousFireTwoShotsNeeded	= INT_MAX;
	m_continuousFireCoastFrames			= 0;
 	m_autoReloadWhenIdleFrames			= 0;
	m_clipReloadTime								= 0;
	m_minDelayBetweenShots					= 0;
	m_maxDelayBetweenShots					= 0;
	m_fireSoundLoopTime							= 0;
	m_continuousLaserLoopTime = 0;
	m_extraBonus										= NULL;
	m_shotsPerBarrel								= 1;
	m_antiMask											= WEAPON_ANTI_GROUND;	// but not air or projectile.
	m_projectileStreamName.clear();
	m_laserName.clear();
	m_laserBoneName.clear();
	m_historicBonusTime							= 0;
	m_historicBonusCount						= 0;
	m_historicBonusRadius						= 0;
	m_historicBonusWeapon						= NULL;
	m_shrapnelBonusCount						= 0;
	m_shrapnelBonusWeapon						= NULL;
	m_shrapnelDoesNotRequireVictim				= FALSE;
	m_shrapnelIgnoresStealthStatus				= FALSE;
	m_shrapnelAffectsMask						= (WEAPON_AFFECTS_ENEMIES | WEAPON_AFFECTS_NEUTRALS);
	m_leechRangeWeapon							= FALSE;
	m_capableOfFollowingWaypoint		= FALSE;
	m_isShowsAmmoPips								= FALSE;
	m_allowAttackGarrisonedBldgs		= FALSE;
	m_playFXWhenStealthed						= FALSE;
	m_preAttackDelay								= 0;
	m_continueAttackRange						= 0.0f;
	m_infantryInaccuracyDist				= 0.0f;
	m_damageStatusType							= OBJECT_STATUS_NONE;
	m_suspendFXDelay								= 0;
	m_dieOnDetonate						= FALSE;
	m_scatterTargetAligned = FALSE;
	m_scatterTargetRandom = TRUE;
	m_scatterTargetRandomAngle = FALSE;
	m_scatterTargetMinScalar = 0;
	m_scatterTargetCenteredAtShooter = FALSE;
	m_scatterTargetResetTime = 0;
	m_preAttackFXDelay = 6; // Non-Zero default! 6 frames = 200ms. This should be a good base value to avoid spamming
	m_laserGroundUnitTargetHeight = 10; // Default Height offset
	m_scatterOnWaterSurface = false;
	m_historicDamageTriggerId = 0;
	m_customDamageType						= NULL;
	m_customDamageStatusType				= NULL;
	m_customDeathType						= NULL;
	m_isFlame 								= FALSE;
	m_projectileCollidesWithBurn			= FALSE;
	m_isPoison 								= FALSE;
	m_poisonMuzzleFlashesGarrison			= FALSE;
	m_isDisarm 								= FALSE;
	m_killsGarrison 						= FALSE;
	m_killsGarrisonAmount 					= 0;
	m_playSpecificVoice						= NULL;
	m_damageFXOverride						= DAMAGE_UNRESISTABLE;
	m_statusDuration 						= 0.0f;
	//m_doStatusDamage 						= FALSE;
	m_statusDurationTypeCorrelate 			= FALSE;
	//m_tintStatus							= TINT_STATUS_INVALID;
	//m_customTintStatus					= NULL;

	//m_isSubdual = FALSE;
	//m_subdualDealsNormalDamage = FALSE;
	//m_subdualDamageMultiplier = 1.0f;	
	m_subdualForbiddenKindOf.clear();
	m_firingTrackerStatusTrigger.clear();
	m_firingTrackerBonusConditionGive = WEAPONBONUSCONDITION_INVALID;
	m_firingTrackerCustomStatusTrigger.clear();
	m_firingTrackerCustomBonusConditionGive = NULL;
	m_notAbsoluteKill = FALSE;
	m_clearsParasite = FALSE;
	m_clearsParasiteKeys.clear();

	m_isMissileAttractor = FALSE;
	m_subduedProjectileNoDamage = FALSE;

	m_damagesSelfOnly = FALSE;

	m_subdualCustomType = NULL;
	m_customSubdualRemoveSubdualTintOnDisable = FALSE;
	m_customSubdualDisableType = DISABLED_SUBDUED;
	m_customSubdualDisableTint = TINT_STATUS_INVALID;
	m_customSubdualDisableCustomTint = NULL;
	m_customSubdualDisableSound = NULL;
	m_customSubdualDisableRemoveSound = NULL;

	m_shockWaveUseCenter = FALSE;
	m_shockWaveRespectsCenter = FALSE;
	m_shockWaveAffectsAirborne = FALSE;
	m_shockWavePullsAirborne = FALSE;

	m_magnetAmount = 0.0f;
	m_magnetInfantryAmount = 0.0f;
	m_magnetTaperOffDistance = 0.0f;
	m_magnetTaperOffRatio = 1.0f;
	m_magnetTaperOnDistance = 0.0f;
	m_magnetTaperOnRatio = 1.0f;
	m_magnetLiftHeight = 0.0f;
	m_magnetLiftHeightSecond = 0.0f;
	m_magnetLiftForce = 1.0f;
	m_magnetLiftForceToHeight = 1.0f;
	m_magnetLiftForceToHeightSecond = 1.0f;
	m_magnetMaxLiftHeight = 0.0f;
	m_magnetLevitationHeight = 0.0f;
	m_magnetMinDistance = 0.0f;
	m_magnetMaxDistance = 0.0f;
	m_magnetLinearDistanceCalc = FALSE;
	m_magnetNoAirborne = FALSE;
	m_magnetAirborneZForce = 0.0f;
	m_magnetAirboneAffectedByYaw = FALSE;
	m_magnetUseCenter = FALSE;
	m_magnetRespectsCenter = TRUE;
	m_magnetFormula = MAGNET_STATIC;

	m_protectionTypes = DEATH_TYPE_FLAGS_ALL;

	m_isShielderImmune = FALSE;

	m_minDamageHeight = 0.0f;
	m_maxDamageHeight = 0.0f;
	m_minTargetHeight = 0.0f;
	m_maxTargetHeight = 0.0f;
	m_attackRangePriority = 0;
	m_outsideAttackRangePriority = 0;

	m_activationUpgradeNames.clear();
	m_conflictingUpgradeNames.clear();
	m_requiredCustomStatus.clear();
	m_forbiddenCustomStatus.clear();
	m_requiresAllTriggers = false;

	m_cursorName = NULL;
	m_forceAttackObjectCursorName = NULL;
	m_forceAttackGroundCursorName = NULL;
	m_invalidCursorName = NULL;

	m_rejectKeys.clear();

	m_rofMovingPenalty = 0.0f;
	m_rofMovingScales = FALSE;
	m_rofMovingMaxSpeedCount = 0.0f;

	m_isRailgun = FALSE;
	m_railgunUsesSecondaryDamage = FALSE;
	m_railgunIsLinear = TRUE;
	m_railgunPiercesBehind = FALSE;
	m_railgunPierceAmount = -1;
	m_railgunRadius = 0.0f;
	m_railgunRadiusCheckPerDistance = 0.0f;
	m_railgunInfantryRadius = 0.0f;
	m_railgunExtraDistance = 0.0f;
	m_railgunMaxDistance = 0.0f;
	m_railgunDamageType = DAMAGE_NUM_TYPES;
	m_railgunDeathType = DEATH_NONE;
	m_railgunDamageFXOverride = DAMAGE_UNRESISTABLE;
	m_railgunCustomDamageType = NULL;
	m_railgunCustomDeathType = NULL;

	m_weaponBypassLineOfSight = FALSE;
	m_weaponIgnoresObstacles = FALSE;

	m_invulnerabilityDuration = 0;
}

//-------------------------------------------------------------------------------------------------
WeaponTemplate::~WeaponTemplate()
{
	deleteInstance(m_nextTemplate);

	// delete any extra-bonus that's present
	deleteInstance(m_extraBonus);
}

// ------------------------------------------------------------------------------------------------
void WeaponTemplate::reset( void )
{
	m_historicDamage.clear();
}

void WeaponTemplate::copy_from(const WeaponTemplate& other) {
	//Backup nextTemplate, name and namekey
	WeaponTemplate* nextTempl = this->m_nextTemplate;
	AsciiString name = this->m_name;
	NameKeyType nameKey = this->m_nameKey;

	// take all values from other
	*this = other;

	this->m_nextTemplate = nextTempl;
	this->m_name = name;
	this->m_nameKey = nameKey;
}

//-------------------------------------------------------------------------------------------------
/*static*/ void WeaponTemplate::parseWeaponBonusSet( INI* ini, void *instance, void * /*store*/, const void* /*userData*/ )
{
	WeaponTemplate* self = (WeaponTemplate*)instance;

	if (!self->m_extraBonus)
		self->m_extraBonus = newInstance(WeaponBonusSet);

	self->m_extraBonus->parseWeaponBonusSet(ini);
}

//-------------------------------------------------------------------------------------------------
/*static*/ void WeaponTemplate::parseCustomWeaponBonusSet( INI* ini, void *instance, void * /*store*/, const void* /*userData*/ )
{
	WeaponTemplate* self = (WeaponTemplate*)instance;

	if (!self->m_extraBonus)
		self->m_extraBonus = newInstance(WeaponBonusSet);

	self->m_extraBonus->parseCustomWeaponBonusSet(ini);
}

//-------------------------------------------------------------------------------------------------
/*static*/ void WeaponTemplate::parseScatterTarget( INI* ini, void *instance, void * /*store*/, const void* /*userData*/ )
{
	// Accept multiple listings of Coord2D's.
	WeaponTemplate* self = (WeaponTemplate*)instance;

	Coord2D target;
	target.x = 0;
	target.y = 0;
	INI::parseCoord2D( ini, NULL, &target, NULL );

	self->m_scatterTargets.push_back(target);
}

//-------------------------------------------------------------------------------------------------
/*static*/ void WeaponTemplate::parseShotDelay( INI* ini, void *instance, void * /*store*/, const void* /*userData*/ )
{
	// This smart parser allows both a single number for traditional delay, and a labeled pair of numbers for a delay range
	WeaponTemplate* self = (WeaponTemplate*)instance;
	static const char *MIN_LABEL = "Min";
	static const char *MAX_LABEL = "Max";

	const char* token = ini->getNextTokenOrNull(ini->getSepsColon());

	if( stricmp(token, MIN_LABEL) == 0 )
	{
		// Two entry min/max
		self->m_minDelayBetweenShots = INI::scanInt(ini->getNextToken(ini->getSepsColon()));
		token = ini->getNextTokenOrNull(ini->getSepsColon());
		if( stricmp(token, MAX_LABEL) != 0 )
		{
			// Messed up double entry
			self->m_maxDelayBetweenShots = self->m_minDelayBetweenShots;
		}
		else
			self->m_maxDelayBetweenShots = INI::scanInt(ini->getNextToken(ini->getSepsColon()));
	}
	else
	{
		// single entry, as in no label so the first token is just a number
		self->m_minDelayBetweenShots = INI::scanInt(token);
		self->m_maxDelayBetweenShots = self->m_minDelayBetweenShots;
	}

	// No matter what we have now, we want to convert it to frames from msec.
	// ShotDelay used to use parseDurationUnsignedInt, and we are expanding on that.
	self->m_minDelayBetweenShots = ceilf(ConvertDurationFromMsecsToFrames((Real)self->m_minDelayBetweenShots));
	self->m_maxDelayBetweenShots = ceilf(ConvertDurationFromMsecsToFrames((Real)self->m_maxDelayBetweenShots));

}

//-------------------------------------------------------------------------------------------------
void WeaponTemplate::postProcessLoad()
{
	if (!TheThingFactory)
	{
		DEBUG_CRASH(("you must call this after TheThingFactory is inited"));
		return;
	}

	if (m_projectileName.isEmpty())
	{
		m_projectileTmpl = NULL;
	}
	else
	{
		m_projectileTmpl = TheThingFactory->findTemplate(m_projectileName);
		DEBUG_ASSERTCRASH(m_projectileTmpl, ("projectile %s not found!",m_projectileName.str()));
	}

	for (Int i = LEVEL_FIRST; i <= LEVEL_LAST; ++i)
	{
		// And the OCL if there is one
		if (m_fireOCLNames[i].isEmpty())
		{
			m_fireOCLs[i] = NULL;
		}
		else
		{
			m_fireOCLs[i] = TheObjectCreationListStore->findObjectCreationList(m_fireOCLNames[i].str() );
			DEBUG_ASSERTCRASH(m_fireOCLs[i], ("OCL %s not found in a weapon!",m_fireOCLNames[i].str()));
		}
		m_fireOCLNames[i].clear();

		// And the other OCL if there is one
		if (m_projectileDetonationOCLNames[i].isEmpty() )
		{
			m_projectileDetonationOCLs[i] = NULL;
		}
		else
		{
			m_projectileDetonationOCLs[i] = TheObjectCreationListStore->findObjectCreationList(m_projectileDetonationOCLNames[i].str() );
			DEBUG_ASSERTCRASH(m_projectileDetonationOCLs[i], ("OCL %s not found in a weapon!",m_projectileDetonationOCLNames[i].str()));
		}
		m_projectileDetonationOCLNames[i].clear();

		if (m_customSubdualOCLNames[i].isEmpty() )
		{
			m_customSubdualOCLs[i] = NULL;
		}
		else
		{
			m_customSubdualOCLs[i] = TheObjectCreationListStore->findObjectCreationList(m_customSubdualOCLNames[i].str() );
			DEBUG_ASSERTCRASH(m_customSubdualOCLs[i], ("OCL %s not found in a weapon!",m_customSubdualOCLNames[i].str()));
		}
		m_customSubdualOCLNames[i].clear();

		if (m_railgunOCLNames[i].isEmpty())
		{
			m_railgunOCLs[i] = NULL;
		}
		else
		{
			m_railgunOCLs[i] = TheObjectCreationListStore->findObjectCreationList(m_railgunOCLNames[i].str() );
			DEBUG_ASSERTCRASH(m_railgunOCLs[i], ("OCL %s not found in a weapon!",m_railgunOCLNames[i].str()));
		}
		m_railgunOCLNames[i].clear();
	}

}

//-------------------------------------------------------------------------------------------------
Real WeaponTemplate::getAttackRange(const WeaponBonus& bonus) const
{
#ifdef RATIONALIZE_ATTACK_RANGE
	// Note - undersize by 1/4 of a pathfind cell, so that the goal is not teetering on the edge
	// of firing range.  jba.
	const Real UNDERSIZE = PATHFIND_CELL_SIZE_F*0.25f;
	Real r = m_attackRange * bonus.getField(WeaponBonus::RANGE) - UNDERSIZE;
	if (r < 0.0f) r = 0.0f;
	return r;
#else
// fudge this a little to account for pathfinding roundoff & such
	const Real ATTACK_RANGE_FUDGE = 1.05f;
	return m_attackRange * bonus.getField(WeaponBonus::RANGE) * ATTACK_RANGE_FUDGE;
#endif
}

//-------------------------------------------------------------------------------------------------
Real WeaponTemplate::getMinimumAttackRange() const
{
#ifdef RATIONALIZE_ATTACK_RANGE
	// Note - undersize by 1/4 of a pathfind cell, so that the goal is not teetering on the edge
	// of firing range.  jba.
	const Real UNDERSIZE = PATHFIND_CELL_SIZE_F*0.25f;
	Real r = m_minimumAttackRange - UNDERSIZE;
	if (r < 0.0f) r = 0.0f;
	return r;
#else
	return m_minimumAttackRange;
#endif
}

//-------------------------------------------------------------------------------------------------
Real WeaponTemplate::getUnmodifiedAttackRange() const
{
	return m_attackRange;
}

//-------------------------------------------------------------------------------------------------
Int WeaponTemplate::getDelayBetweenShots(const WeaponBonus& bonus) const
{
	// yes, divide, not multiply; the larger the rate-of-fire bonus, the shorter
	// we want the delay time to be.
	Int delayToUse;
	if( m_minDelayBetweenShots == m_maxDelayBetweenShots )
		delayToUse = m_minDelayBetweenShots; // Random number thing doesn't like this case
	else
		delayToUse = GameLogicRandomValue( m_minDelayBetweenShots, m_maxDelayBetweenShots );

	Real bonusROF = bonus.getField(WeaponBonus::RATE_OF_FIRE);
	//CRCDEBUG_LOG(("WeaponTemplate::getDelayBetweenShots() - min:%d max:%d val:%d, bonusROF=%g/%8.8X",
		//m_minDelayBetweenShots, m_maxDelayBetweenShots, delayToUse, bonusROF, AS_INT(bonusROF)));

	return REAL_TO_INT_FLOOR(delayToUse / bonusROF);
}

//-------------------------------------------------------------------------------------------------
Int WeaponTemplate::getClipReloadTime(const WeaponBonus& bonus) const
{
	// yes, divide, not multiply; the larger the rate-of-fire bonus, the shorter
	// we want the reload time to be.
	return REAL_TO_INT_FLOOR(m_clipReloadTime / bonus.getField(WeaponBonus::RATE_OF_FIRE));
}

//-------------------------------------------------------------------------------------------------
Int WeaponTemplate::getPreAttackDelay( const WeaponBonus& bonus ) const
{
	return m_preAttackDelay * bonus.getField( WeaponBonus::PRE_ATTACK );
}

//-------------------------------------------------------------------------------------------------
Real WeaponTemplate::getPrimaryDamage(const WeaponBonus& bonus) const
{
	return m_primaryDamage * bonus.getField(WeaponBonus::DAMAGE);
}

//-------------------------------------------------------------------------------------------------
Real WeaponTemplate::getPrimaryDamageRadius(const WeaponBonus& bonus) const
{
	return m_primaryDamageRadius * bonus.getField(WeaponBonus::RADIUS);
}

//-------------------------------------------------------------------------------------------------
Real WeaponTemplate::getSecondaryDamage(const WeaponBonus& bonus) const
{
	return m_secondaryDamage * bonus.getField(WeaponBonus::DAMAGE);
}

//-------------------------------------------------------------------------------------------------
Real WeaponTemplate::getSecondaryDamageRadius(const WeaponBonus& bonus) const
{
	return m_secondaryDamageRadius * bonus.getField(WeaponBonus::RADIUS);
}

Real WeaponTemplate::getArmorBonus(const WeaponBonus& bonus) const 
{
	return bonus.getField(WeaponBonus::ARMOR); 
}

//-------------------------------------------------------------------------------------------------
Bool WeaponTemplate::isContactWeapon() const
{
#ifdef RATIONALIZE_ATTACK_RANGE
	// Note - undersize by 1/4 of a pathfind cell, so that the goal is not teetering on the edge
	// of firing range.  jba.
	const Real UNDERSIZE = PATHFIND_CELL_SIZE_F*0.25f;
	return (m_attackRange - UNDERSIZE) < PATHFIND_CELL_SIZE_F;
#else
// fudge this a little to account for pathfinding roundoff & such
	const Real ATTACK_RANGE_FUDGE = 1.05f;
	return m_attackRange * ATTACK_RANGE_FUDGE < PATHFIND_CELL_SIZE_F;
#endif
}

//-------------------------------------------------------------------------------------------------
Real WeaponTemplate::estimateWeaponTemplateDamage(
	const Object *sourceObj,
	const Object *victimObj,
	const Coord3D* victimPos,
	const WeaponBonus& bonus
) const
{
	if (sourceObj == NULL || (victimObj == NULL && victimPos == NULL))
	{
		DEBUG_CRASH(("bad args to estimate"));
		return 0.0f;
	}

	const Real damageAmount = getPrimaryDamage(bonus);
	if ( victimObj == NULL )
	{
		return damageAmount;
	}

	DamageType damageType = getDamageType();
	DeathType deathType = getDeathType();

	AsciiString customDamageType = getCustomDamageType();

	if ( victimObj->isKindOf(KINDOF_SHRUBBERY) )
	{
		if (deathType == DEATH_BURNED)
		{
			// this is just a nonzero value, to ensure we can target shrubbery with flame weapons, regardless...
			return 1.0f;
		}
		else
		{
			return 0.0f;
		}
	}


  // hmm.. must be shooting a firebase or such, if there is noone home to take the bullet, return 0!
  if ( victimObj->isKindOf( KINDOF_STRUCTURE) && damageType == DAMAGE_SNIPER )
  {
    if ( victimObj->getContain() )
    {
      if ( victimObj->getContain()->getContainCount() == 0 )
        return 0.0f;
    }
  }


	if ( damageType == DAMAGE_SURRENDER || m_allowAttackGarrisonedBldgs )
	{
		ContainModuleInterface* contain = victimObj->getContain();
		if( contain && contain->getContainCount() > 0 && contain->isGarrisonable() && !contain->isImmuneToClearBuildingAttacks() )
		{
			// this is just a nonzero value, to ensure we can target garrisoned things with surrender weapons, regardless...
			return 1.0f;
		}
	}

	if( damageType == DAMAGE_DISARM )
	{
		if( victimObj->isKindOf( KINDOF_MINE ) || victimObj->isKindOf( KINDOF_BOOBY_TRAP ) || victimObj->isKindOf( KINDOF_DEMOTRAP ) )
		{
			// this is just a nonzero value, to ensure we can target mines with disarm weapons, regardless...
			return 1.0f;
		}

		// Units that get disabled by Chrono damage cannot be attacked
		if (victimObj->isDisabledByType(DISABLED_CHRONO) &&
			!(damageType == DAMAGE_CHRONO_GUN || damageType == DAMAGE_CHRONO_UNRESISTABLE)) {
			return 0.0;
		}

	}
	if( damageType == DAMAGE_DEPLOY && !victimObj->isAirborneTarget() )
	{
		return 1.0f;
	}

	//@todo Kris need to examine the DAMAGE_HACK type for damage estimation purposes.
	//Likely this damage type will have threat implications that won't properly be dealt with until resolved.

	DamageInfoInput damageInfo;
	damageInfo.m_damageType = damageType;
	damageInfo.m_deathType = deathType;
	damageInfo.m_sourceID = sourceObj->getID();
	damageInfo.m_amount = damageAmount;
	damageInfo.m_customDamageType = customDamageType;
	return victimObj->estimateDamage(damageInfo);
}

//-------------------------------------------------------------------------------------------------
Bool WeaponTemplate::shouldProjectileCollideWith(
	const Object* projectileLauncher,
	const Object* projectile,
	const Object* thingWeCollidedWith,
	ObjectID intendedVictimID,	// could be INVALID_ID for a position-shot
	ObjectID shrapnelLaunchID
) const
{
 	if (!projectile || !thingWeCollidedWith)
 		return false;

	// if it's our intended victim, we want to collide with it, regardless of any other considerations.
	if (intendedVictimID == thingWeCollidedWith->getID())
		return true;

	// if we are launched from a shrapnel, do not collide with the current victim 
	if (shrapnelLaunchID != INVALID_ID && shrapnelLaunchID == thingWeCollidedWith->getID())
	{
		return false;
	}

 	if (projectileLauncher != NULL)
 	{

 		// Don't hit your own launcher, ever.
 		if (projectileLauncher == thingWeCollidedWith)
 			return false;

 		// If our launcher is inside something, and that something is 'thingWeCollidedWith' we won't collide
 		const Object *launcherContainedBy = projectileLauncher->getContainedBy();
 		if( launcherContainedBy == thingWeCollidedWith )
 			return false;

 	}

	// never bother burning already-burned things. (srj)
	if (!getProjectileCollidesWithBurn())
	{
		if (getDamageType() == DAMAGE_FLAME || getDamageType() == DAMAGE_PARTICLE_BEAM || getIsFlame() == TRUE)
		{
			if (thingWeCollidedWith->testStatus(OBJECT_STATUS_BURNED))
			{
				return false;
			}
		}
	}

	// horrible special case for airplanes sitting on airfields: the projectile might
	// "collide" with the airfield's (invisible) collision geometry when a resting plane
	// is targeted. we don't want this. special case it:
	if (thingWeCollidedWith->isKindOf(KINDOF_FS_AIRFIELD))
	{
		//
		// ok, so if we are an airfield, and our intended victim has a reserved space
		// with us, it "belongs" to us and collisions intended for it should not detonate
		// as a result of colliding with us.
		//
		// notes:
		//	-- we have already established that "thingWeCollidedWith" is not the intended victim (above)
		//	-- this does NOT verify that the plane is actually parked at the airfield; it might be elsewhere
		//			(but if it is, it's highly unlikely that this sort of collision could occur)
		//
		for (BehaviorModule** i = thingWeCollidedWith->getBehaviorModules(); *i; ++i)
		{
			ParkingPlaceBehaviorInterface* pp = (*i)->getParkingPlaceBehaviorInterface();
			if (pp != NULL && pp->hasReservedSpace(intendedVictimID))
				return false;
		}
	}

	// if something has a Sneaky Target offset, it is momentarily immune to being hit...
	// normally this shouldn't happen, but occasionally can by accident. so avoid it. (srj)
	const AIUpdateInterface* ai = thingWeCollidedWith->getAI();
	if (ai != NULL && ai->getSneakyTargetingOffset(NULL))
	{
		return false;
	}

	Int requiredMask = 0;

	Relationship r = projectile->getRelationship(thingWeCollidedWith);
	if (r == ALLIES) requiredMask = WEAPON_COLLIDE_ALLIES;
	else if (r == ENEMIES) requiredMask = WEAPON_COLLIDE_ENEMIES;

	if (thingWeCollidedWith->isKindOf(KINDOF_STRUCTURE))
	{
		if (thingWeCollidedWith->getControllingPlayer() == projectile->getControllingPlayer())
			requiredMask |= WEAPON_COLLIDE_CONTROLLED_STRUCTURES;
		else
			requiredMask |= WEAPON_COLLIDE_STRUCTURES;
	}
	if (thingWeCollidedWith->isKindOf(KINDOF_SHRUBBERY))					requiredMask |= WEAPON_COLLIDE_SHRUBBERY;
	if (thingWeCollidedWith->isKindOf(KINDOF_PROJECTILE))					requiredMask |= WEAPON_COLLIDE_PROJECTILE;
	if (thingWeCollidedWith->getTemplate()->getFenceWidth() > 0)	requiredMask |= WEAPON_COLLIDE_WALLS;
	if (thingWeCollidedWith->isKindOf(KINDOF_SMALL_MISSILE))			requiredMask |= WEAPON_COLLIDE_SMALL_MISSILES;			//All missiles are also projectiles!
	if (thingWeCollidedWith->isKindOf(KINDOF_BALLISTIC_MISSILE))	requiredMask |= WEAPON_COLLIDE_BALLISTIC_MISSILES;	//All missiles are also projectiles!

	// if any in requiredMask are present in collidemask, do the collision. (srj)
	if ((getProjectileCollideMask() & requiredMask) != 0)
		return true;

	//DEBUG_LOG(("Rejecting projectile collision between %s and %s!",projectile->getTemplate()->getName().str(),thingWeCollidedWith->getTemplate()->getName().str()));
	return false;
}

//-------------------------------------------------------------------------------------------------
static void clipToTerrainExtent(Coord3D& approachTargetPos)
{
	Region3D bounds;
	TheTerrainLogic->getExtent(&bounds);
	if (approachTargetPos.x < bounds.lo.x+PATHFIND_CELL_SIZE_F) {
		approachTargetPos.x = bounds.lo.x+PATHFIND_CELL_SIZE_F;
	}
	if (approachTargetPos.y < bounds.lo.y+PATHFIND_CELL_SIZE_F) {
		approachTargetPos.y = bounds.lo.y+PATHFIND_CELL_SIZE_F;
	}
	if (approachTargetPos.x > bounds.hi.x-PATHFIND_CELL_SIZE_F) {
		approachTargetPos.x = bounds.hi.x-PATHFIND_CELL_SIZE_F;
	}
	if (approachTargetPos.y > bounds.hi.y-PATHFIND_CELL_SIZE_F) {
		approachTargetPos.y = bounds.hi.y-PATHFIND_CELL_SIZE_F;
	}
}

//-------------------------------------------------------------------------------------------------
UnsignedInt WeaponTemplate::fireWeaponTemplate
(
	const Object *sourceObj,
	WeaponSlotType wslot,
	Int specificBarrelToUse,
	Object *victimObj,
	const Coord3D* victimPos,
	const WeaponBonus& bonus,
	Bool isProjectileDetonation,
	Bool ignoreRanges,
	Weapon *firingWeapon,
	ObjectID* projectileID,
	Bool inflictDamage,
	const Coord3D* launchPos,
	ObjectID shrapnelLaunchID
) const
{

	//-extraLogging
	#if defined(RTS_DEBUG)
		AsciiString targetStr;
		if( TheGlobalData->m_extraLogging )
		{
			if( victimObj )
				targetStr.format( "%s", victimObj->getTemplate()->getName().str() );
			else if( victimPos )
				targetStr.format( "%d,%d,%d", victimPos->x, victimPos->y, victimPos->z );
			else
				targetStr.format( "SELF." );

			DEBUG_LOG( ("%d - WeaponTemplate::fireWeaponTemplate() begin - %s attacking %s",
				TheGameLogic->getFrame(), sourceObj->getTemplate()->getName().str(), targetStr.str() ) );
		}
	#endif
	//end -extraLogging

	//CRCDEBUG_LOG(("WeaponTemplate::fireWeaponTemplate() from %s", DescribeObject(sourceObj).str()));
	DEBUG_ASSERTCRASH(specificBarrelToUse >= 0, ("specificBarrelToUse should no longer be -1"));

	if (sourceObj == NULL || (victimObj == NULL && victimPos == NULL))
	{
		//-extraLogging
		#if defined(RTS_DEBUG)
			if( TheGlobalData->m_extraLogging )
				DEBUG_LOG( ("FAIL 1 (sourceObj %d == NULL || (victimObj %d == NULL && victimPos %d == NULL)", sourceObj != 0, victimObj != 0, victimPos != 0) );
		#endif
		//end -extraLogging

		return 0;
	}

	DEBUG_ASSERTCRASH((m_primaryDamage > 0)  ||  (victimObj == NULL), ("You can't really shoot a zero damage weapon at an Object.") );

	ObjectID sourceID = sourceObj->getID();
	const Coord3D* sourcePos = launchPos ? launchPos : sourceObj->getPosition();

	Real distSqr;
	ObjectID victimID;
	TBridgeAttackInfo info;
	Coord3D victimPosStorage;
	if (victimObj)
	{
		DEBUG_ASSERTLOG(sourceObj != victimObj, ("*** firing weapon at self -- is this really what you want?"));
		victimPos = victimObj->getPosition();
		victimID = victimObj->getID();

		Coord3D sneakyOffset;
		const AIUpdateInterface* ai = victimObj->getAI();
		if (ai != NULL && ai->getSneakyTargetingOffset(&sneakyOffset))
		{
			victimPosStorage = *victimPos;
			victimPosStorage.x += sneakyOffset.x;
			victimPosStorage.y += sneakyOffset.y;
			victimPosStorage.z += sneakyOffset.z;

			victimPos = &victimPosStorage;
			// for a sneaky offset, we always target a position rather than an object
			victimObj = NULL;
			victimID = INVALID_ID;
			distSqr = ThePartitionManager->getDistanceSquared(sourceObj, victimPos, ATTACK_RANGE_CALC_TYPE);
		}
		else
		{
			if (victimObj->isKindOf(KINDOF_BRIDGE))
			{
				// Bridges are kind of oddball - they have 2 target points at either end.
				TheTerrainLogic->getBridgeAttackPoints(victimObj, &info);
				distSqr = ThePartitionManager->getDistanceSquared( sourceObj, &info.attackPoint1, ATTACK_RANGE_CALC_TYPE );
				victimPos = &info.attackPoint1;
 				Real distSqr2 = ThePartitionManager->getDistanceSquared( sourceObj, &info.attackPoint2, ATTACK_RANGE_CALC_TYPE );
				if (distSqr > distSqr2)
				{
					// Try the other one.
					distSqr = distSqr2;
					victimPos = &info.attackPoint2;
				}
			}
			else
			{
				distSqr = ThePartitionManager->getDistanceSquared(sourceObj, victimObj, ATTACK_RANGE_CALC_TYPE);
			}
		}
	}
	else
	{
		victimID = INVALID_ID;
		distSqr = ThePartitionManager->getDistanceSquared(sourceObj, victimPos, ATTACK_RANGE_CALC_TYPE);
	}

//	DEBUG_LOG(("WeaponTemplate::fireWeaponTemplate: firing weapon %s (source=%s, victim=%s)",
//		m_name.str(),sourceObj->getTemplate()->getName().str(),victimObj?victimObj->getTemplate()->getName().str():"NULL"));

	//Only perform this check if the weapon isn't a leech range weapon (which can have unlimited range!)
	if( !ignoreRanges && !isLeechRangeWeapon() )
	{
		Real attackRangeSqr = sqr(getAttackRange(bonus));
		if (distSqr > attackRangeSqr)
		{
			//DEBUG_ASSERTCRASH(distSqr < 5*5 || distSqr < attackRangeSqr*1.2f, ("*** victim is out of range (%f vs %f) of this weapon -- why did we attempt to fire?",sqrtf(distSqr),sqrtf(attackRangeSqr)));

			//-extraLogging
			#if defined(RTS_DEBUG)
				if( TheGlobalData->m_extraLogging )
					DEBUG_LOG( ("FAIL 2 (distSqr %.2f > attackRangeSqr %.2f)", distSqr, attackRangeSqr ) );
			#endif
			//end -extraLogging

			return 0;
		}
	}

	if (!ignoreRanges)
	{
		Real minAttackRangeSqr = sqr(getMinimumAttackRange());
#ifdef RATIONALIZE_ATTACK_RANGE
		if (distSqr < minAttackRangeSqr && !isProjectileDetonation)
#else
		if (distSqr < minAttackRangeSqr-0.5f && !isProjectileDetonation)
#endif
		{
			DEBUG_ASSERTCRASH(distSqr > minAttackRangeSqr*0.8f, ("*** victim is closer than min attack range (%f vs %f) of this weapon -- why did we attempt to fire?",sqrtf(distSqr),sqrtf(minAttackRangeSqr)));

			//-extraLogging
			#if defined(RTS_DEBUG)
				if( TheGlobalData->m_extraLogging )
					DEBUG_LOG( ("FAIL 3 (distSqr %.2f< minAttackRangeSqr %.2f - 0.5f && !isProjectileDetonation %d)", distSqr, minAttackRangeSqr, isProjectileDetonation ) );
			#endif
			//end -extraLogging

			return 0;
		}
	}

	const ThingTemplate *projectileTemplate = getProjectileTemplate();
	
	// New feature, similar to Black Hole Armor.
	Object* retarget = NULL;
	ObjectID ShielderID = INVALID_ID;

	if(!getIsShielderImmune() && !isProjectileDetonation && !isLeechRangeWeapon() && victimObj && victimObj->testCustomStatus("SHIELDED_TARGET"))
	{
		ProtectionTypeFlags ShieldedType = victimObj->getShieldByTargetType();
		Bool hasProtection = false;
		if(projectileTemplate)
			hasProtection = !isProjectileDetonation && getProtectionTypeFlag(ShieldedType, PROTECTION_PROJECTILES);
		else
			hasProtection = firingWeapon->isLaser() ? getProtectionTypeFlag(ShieldedType, PROTECTION_LASER) : getProtectionTypeFlag(ShieldedType, PROTECTION_BULLETS);

		if(hasProtection)
		{
			ShielderID = victimObj->getShieldByTargetID();
			if(ShielderID != INVALID_ID)
			{
				retarget = TheGameLogic->findObjectByID(ShielderID);
				if(retarget && (retarget->isEffectivelyDead() || retarget->isDestroyed()) )
					retarget = NULL;
			}
		}
	}
	Object *curTarget = retarget ? retarget : victimObj;
	Coord3D targetedPos = *victimPos;
	Bool hasBodyForTargetAiming = FALSE;

	if( TheGlobalData->m_dynamicTargeting && curTarget )
	{
		Coord3D targetCoord = *sourceObj->getCurrentTargetCoord();
		hasBodyForTargetAiming = TRUE;
		if( (TheGameLogic->getFrame() >= firingWeapon->getLastShotFrame() + 3*LOGICFRAMES_PER_SECOND) ||
		 	(sourceObj->getLastVictimID() != curTarget->getID()) ||
			(fabs(targetCoord.x) < WWMATH_EPSILON &&
			fabs(targetCoord.y) < WWMATH_EPSILON &&
			fabs(targetCoord.z) < WWMATH_EPSILON )
		  )
		{
			// if we're airborne and too close, just head for the opposite side.
			Coord3D dir;
			targetedPos = *curTarget->getPosition();
			dir.x = sourcePos->x-targetedPos.x;
			dir.y = sourcePos->y-targetedPos.y;
			dir.z = sourcePos->z-targetedPos.z;
			if (sourceObj->isAboveTerrain())
			{
				// Don't do a 180 degree turn.
				Real angle = atan2(-dir.y, -dir.x);
				Real relAngle = sourceObj->getOrientation()- angle;
				if (relAngle>2*PI) relAngle -= 2*PI;
				if (relAngle<-2*PI) relAngle += 2*PI;
				if (fabs(relAngle)<PI/2) {
					dir.x = -dir.x;
					dir.y = -dir.y;
					dir.z = -dir.z;
				}
			}

			Real targetRadius = curTarget->getGeometryInfo().getBoundingCircleRadius();
			Real targetHeight = curTarget->getGeometryInfo().getMaxHeightAbovePosition();

			Real distance = dir.length();
			Real shrukenDistance = distance * 0.33;
			Real adjustedHeight = sourceObj->isAboveTerrain() ? targetHeight : min(shrukenDistance, targetHeight);

			Real angle = atan2(dir.y, dir.x);
			Real deviateAngle = 0.39f * adjustedHeight / targetHeight;
			Real targetAngle = angle + GameLogicRandomValueReal(-deviateAngle,deviateAngle); // About +-45 degrees deviation

			PathfindLayerEnum sourceLayer = sourceObj->getLayer();
			PathfindLayerEnum targetLayer = curTarget->getLayer();
			Bool isStructure = curTarget->isKindOf(KINDOF_STRUCTURE) ? TRUE : FALSE;
			Real dz;
			if(isStructure)
				dz = GameLogicRandomValueReal(adjustedHeight * 0.25f, adjustedHeight * 0.7f);
			else if(targetLayer > sourceLayer)
				dz = GameLogicRandomValueReal(0.0f, adjustedHeight * 0.5f);
			else if(sourceLayer > targetLayer)
				dz = GameLogicRandomValueReal(adjustedHeight * 0.5f, adjustedHeight);
			else
				dz = GameLogicRandomValueReal(0.0f, adjustedHeight);

			targetHeight *= isStructure ? 0.7f : 0.8f;
			Real targetRatio = isStructure ? ((targetHeight + dz + dz * 0.5f) / (targetHeight + targetHeight + targetHeight * 0.5f)) : (dz / targetHeight);
			targetRatio = min(0.6f, 1.0f - targetRatio);
			if(firingWeapon->hasProjectileStream())
			{
				if(isStructure)
					targetRadius *= targetRatio;
				else
					targetRadius *= curTarget->isKindOf(KINDOF_INFANTRY) ? min(0.1f, targetRatio) : min(0.33f, targetRatio);

				dz = 0.0f;
			}
			else if(projectileTemplate != NULL)
			{
				if(isStructure)
				{
					dz *= targetRatio; // Structures doesn't check for Z axis when applying collisions(?), so best to lower it to relatable values.
					if(m_scatterRadius == 0.0f)
						targetRatio *= 1.0f + min(0.8f, 0.03f * targetRadius / PI); // Formula is ( 1 / (Circumference (2 * PI * r) / radius^2 (r * r)) ) * 0.06f, so that's the simplified formula
				}
				else if(curTarget->isKindOf(KINDOF_INFANTRY))
				{
					targetRatio = min(0.1f, targetRatio);
					dz = 0.0f; // Don't aim for headshots or body shots
				}
				targetRadius *= targetRatio;
			}
			else
			{
				targetRadius *= curTarget->isKindOf(KINDOF_INFANTRY) ? min(0.1f, targetRatio) : targetRatio;
			}

			Real dx = Cos(targetAngle) * targetRadius;
			Real dy = Sin(targetAngle) * targetRadius;

			targetedPos.x = targetedPos.x + dx;
			targetedPos.y = targetedPos.y + dy;
			targetedPos.z = targetedPos.z + dz;
			///@todo - make sure we can get to the approach position.
			clipToTerrainExtent(targetedPos);

			targetCoord.x = targetedPos.x - victimPos->x;
			targetCoord.y = targetedPos.y - victimPos->y;
			targetCoord.z = targetedPos.z - victimPos->z;

			TheGameLogic->findObjectByID(sourceID)->setCurrentTargetCoord(&targetCoord);
		}
		else
		{
			Real targetHeight = curTarget->getGeometryInfo().getMaxHeightAbovePosition();
			if(projectileTemplate == NULL && !firingWeapon->isLaser() && !sourceObj->isKindOf(KINDOF_INFANTRY) && distSqr > 0)
			{
				Real adjustedHeight = targetHeight;
				if(!sourceObj->isAboveTerrain() && distSqr < 4 * targetHeight * targetHeight)
				{
					Real distance = sqrt(distSqr);
					Real shrukenDistance = distance * 0.5f;
					adjustedHeight = min(shrukenDistance, targetHeight);
				}
				Real dz = 0.25f*adjustedHeight;
				if(targetCoord.z > dz)
				{
					Real max_dz = min(dz, 0.5f*adjustedHeight - targetCoord.z);
					dz = GameLogicRandomValueReal(-dz, max_dz);
				}
				else
				{
					Real min_dz = max(-dz, targetCoord.z - 0.25f*adjustedHeight);
					dz = GameLogicRandomValueReal(min_dz, dz);
				}
				targetCoord.z += dz;
			}
			targetedPos.x += targetCoord.x;
			targetedPos.y += targetCoord.y;
			targetedPos.z += targetCoord.z;
		}

	}

	// We start from the FX

	// call this even if FXList is null, because this also handles stuff like Gun Barrel Recoil
	if (sourceObj && sourceObj->getDrawable())
	{
		Coord3D targetPos;
		if( curTarget && !hasBodyForTargetAiming )
		{
			curTarget->getGeometryInfo().getCenterPosition( *curTarget->getPosition(), targetPos );
		}
		else
		{
			// IamInnocent - Removed inline from this function
			//targetPos.set( &targetedPos );
			targetPos = targetedPos;
		}
		Real reAngle = getWeaponRecoilAmount();
		Real reDir = reAngle != 0.0f ? (atan2(targetPos.y - sourcePos->y, targetPos.x - sourcePos->x)) : 0.0f;
		VeterancyLevel v = sourceObj->getVeterancyLevel();
		const FXList* fx = isProjectileDetonation ? getProjectileDetonateFX(v) : getFireFX(v);

		if ( TheGameLogic->getFrame() < firingWeapon->getSuspendFXFrame() )
			fx = NULL;

		StealthUpdate *stealth = sourceObj->getStealth();

		Bool handled;
		Bool isDisguisedAndCheckIfNeedOffset = stealth && stealth->isDisguisedAndCheckIfNeedOffset();
		Drawable *currentDraw = isDisguisedAndCheckIfNeedOffset ? stealth->getDrawableTemplateWhileDisguised() : sourceObj->getDrawable();
		if(isDisguisedAndCheckIfNeedOffset)
		{
			if(currentDraw)
			{
				Weapon::setFirePositionForDrawable(sourceObj, currentDraw, wslot, specificBarrelToUse);
				if(TheGlobalData->m_useEfficientDrawableScheme)
					TheGameClient->informClientNewDrawable(currentDraw);
			}
		}

		// TheSuperHackers @todo: Remove hardcoded KINDOF_MINE check and apply PlayFXWhenStealthed = Yes to the mine weapons instead.

		if (!sourceObj->isLogicallyVisible()									// if user watching cannot see us
			&& !sourceObj->isKindOf(KINDOF_MINE)								// and not a mine (which always do the FX, even if hidden)...
			&& !isPlayFXWhenStealthed()													// and not a weapon marked to playwhenstealthed
			)
		{
			handled = TRUE;		// then let's just pretend like we did the fx by returning true
		}
		else if( !launchPos )
		{
			if(isDisguisedAndCheckIfNeedOffset)
			{
				handled = !currentDraw || currentDraw->handleWeaponFireFX(wslot,
																															specificBarrelToUse,
																															fx,
																															getWeaponSpeed(),
																															0.0f,
																															0.0f,
																															&targetPos,
																															getPrimaryDamageRadius(bonus)
																															);

				// IamInnocent - WARNING: The function below is Very nuclear and will cause Crashes (point being do proceed with caution if use)
				// Note: The Barrels for the Drawable MUST Match the current Drawable or else it will cause crashes... (Ofcourse)
				/// Update - Updated regarding Usage, even with Failsaves implemented, should inspect for errors as this function is VERY Volatile. Drawables should all be on Client side and have no affect on Game Logic
				if(handled)
				{
					// IamInnocent - technical fix for Disguised Drawables using original offsets for handling Recoil and Muzzles
					/// Maximum barrel allow for drawables using different templates while using its Original Drawable amount of barrels is the Drawable's Barrel Count - 1
					Int barrelCount = specificBarrelToUse;
					if(sourceObj->getTemplate() != sourceObj->getDrawable()->getTemplate() && currentDraw->getTemplate() == sourceObj->getTemplate())
					{
						Int maxBarrelCount = stealth->getBarrelCountDisguisedTemplate(wslot);
						// IamInnocent - added to check to prevent crashes if the barrelCount has more barrels than the current drawable
						while(barrelCount > maxBarrelCount)
						{
							if(maxBarrelCount <= 0)
								barrelCount = 0;
							else
								barrelCount -= maxBarrelCount;
						}
						maxBarrelCount = max(0, maxBarrelCount-1);
						barrelCount = min(barrelCount, maxBarrelCount);
					}
					sourceObj->getDrawable()->handleWeaponFireRecoil(wslot, barrelCount, reAngle, reDir, fx != NULL, FALSE);
				}
			}
			else
			{
				handled = currentDraw->handleWeaponFireFX(wslot,
																															specificBarrelToUse,
																															fx,
																															getWeaponSpeed(),
																															reAngle,
																															reDir,
																															&targetPos,
																															getPrimaryDamageRadius(bonus)
																															);
			}
		}

		if (handled == false && fx != NULL)
		{
			// bah. just play it at the drawable's pos.
			//DEBUG_LOG(("*** WeaponFireFX not fully handled by the client"));
			const Coord3D* where = isContactWeapon() ? &targetPos : (launchPos ? launchPos : currentDraw->getPosition());
			FXList::doFXPos(fx, where, currentDraw->getTransformMatrix(), getWeaponSpeed(), &targetPos, getPrimaryDamageRadius(bonus));
		}
	}

	// Now do the FireOCL if there is one
	if( sourceObj )
	{
		VeterancyLevel v = sourceObj->getVeterancyLevel();
		const ObjectCreationList *oclToUse = isProjectileDetonation ? getProjectileDetonationOCL(v) : getFireOCL(v);
		if( oclToUse )
			ObjectCreationList::create( oclToUse, sourceObj, NULL );
	}

	Coord3D projectileDestination = targetedPos; //Need to copy this, as we have a pointer to their actual position
	Real scatterRadius = 0.0f;
	if( m_scatterRadius > 0.0f || m_infantryInaccuracyDist > 0.0f && victimObj && victimObj->isKindOf( KINDOF_INFANTRY ) && !retarget )
	{
		projectileDestination = *victimPos;
		// This weapon scatters, so clear the victimObj, as we are no longer shooting it directly,
		// and find a random point within the radius to shoot at as victimPos
		scatterRadius = m_scatterRadius;

		// if it's an object, aim at the center, not the ground part (srj)
		PathfindLayerEnum targetLayer = LAYER_GROUND;
		if( victimObj )
		{
			if( victimObj->isKindOf( KINDOF_STRUCTURE ) )
			{
				if(TheGlobalData->m_dynamicTargeting)
					projectileDestination = targetedPos;
				else
					victimObj->getGeometryInfo().getCenterPosition(*victimObj->getPosition(), projectileDestination);
			}
			if( m_infantryInaccuracyDist > 0.0f && victimObj->isKindOf( KINDOF_INFANTRY ) )
			{
				//If we are firing a weapon that is considered inaccurate against infantry, then add it to
				//the scatter radius!
				scatterRadius += m_infantryInaccuracyDist;
			}
			targetLayer = victimObj->getLayer();
		}

		//victimObj = NULL; // his position is already in victimPos, if he existed

		//Randomize the scatter radius (sometimes it can be more accurate than others)
		scatterRadius = GameLogicRandomValueReal( 0, scatterRadius );
		Real scatterAngleRadian = GameLogicRandomValueReal( 0, 2*PI );

		Coord3D firingOffset;
		firingOffset.zero();
		firingOffset.x = scatterRadius * Cos( scatterAngleRadian );
		firingOffset.y = scatterRadius * Sin( scatterAngleRadian );

		projectileDestination.x += firingOffset.x;
		projectileDestination.y += firingOffset.y;

		//What's suddenly become crucially important is to FIRE at the ground at this location!!!
		//If we aim for the center point of our target and miss, the shot will go much farther than
		//we expect!
		// srj sez: we should actually fire at the layer the victim is on, if possible, in case it is on a bridge...

		if (targetLayer == LAYER_GROUND && firingWeapon->getTemplate()->isScatterOnWaterSurface()) {
				Real waterZ;
				Real terrainZ;
				TheTerrainLogic->isUnderwater(projectileDestination.x, projectileDestination.y, &waterZ, &terrainZ);
				projectileDestination.z = std::max(waterZ, terrainZ);
		}
		else {
			projectileDestination.z = TheTerrainLogic->getLayerHeight(projectileDestination.x, projectileDestination.y, targetLayer);
		}
	}

	if (projectileTemplate == NULL || isProjectileDetonation)
	{
		// see if we need to be called back at a later point to deal the damage.
		Coord3D v;
		v.x = victimPos->x - sourcePos->x;
		v.y = victimPos->y - sourcePos->y;
		v.z = victimPos->z - sourcePos->z;
		// don't round the result; we WANT a fractional-frame-delay in this case.
		Real delayInFrames = (v.length() / getWeaponSpeed());

		ObjectID damageID = getDamageDealtAtSelfPosition() ? INVALID_ID : victimID;
		if(retarget)
		{
			damageID = ShielderID;
		}

		if( firingWeapon->isLaser() )
		{
			if( scatterRadius <= getPrimaryDamageRadius( bonus ) || scatterRadius <= getSecondaryDamageRadius( bonus ) )
			{
				//The laser is close enough to damage the object, so just hit it directly. Some victim objects
				//adjust the laser's position to prevent it from hitting the ground.
				if( curTarget )
				{
					if(TheGlobalData->m_dynamicTargeting)
						projectileDestination = targetedPos;
					else
						projectileDestination.set( curTarget->getPosition() );
				}
				if (firingWeapon->getContinuousLaserLoopTime() > 0)
					firingWeapon->handleContinuousLaser(sourceObj, curTarget, &projectileDestination);
				else
					firingWeapon->createLaser(sourceObj, curTarget, &projectileDestination, launchPos);
			}
			else
			{
				//We are missing our intended target, so now we want to aim at the ground at the projectile offset.
				damageID = INVALID_ID;
				if (firingWeapon->getContinuousLaserLoopTime() > 0)
					firingWeapon->handleContinuousLaser(sourceObj, NULL, &projectileDestination, launchPos);
				else
					firingWeapon->createLaser(sourceObj, NULL, &projectileDestination, launchPos);
			}

			// Handle Detonation OCL
			Coord3D targetPos; // We need a better position to match the visual laser;
			//targetPos.set(&projectileDestination); // IamInnocent - Removed inline from this function
			targetPos = projectileDestination;

			if (curTarget) {
				if (!curTarget->isKindOf(KINDOF_PROJECTILE) && !curTarget->isAirborneTarget()) {
					//Targets are positioned on the ground, so raise the beam up so we're not shooting their feet.
					//Projectiles are a different story, target their exact position.
					targetPos.z += m_laserGroundUnitTargetHeight;
				}
			}
			else { // We target the ground
				targetPos.z += m_laserGroundTargetHeight;
			}

			VeterancyLevel vet = sourceObj->getVeterancyLevel();
			const ObjectCreationList* detOCL = getProjectileDetonationOCL(vet);
			Real laserAngle = atan2(v.y, v.x);  //TODO: check if this should be inverted
			if (detOCL) {
				//TODO: should we consider a proper 3D matrix?
				ObjectCreationList::create(detOCL, sourceObj, &targetPos, NULL, laserAngle);
			}
			// Handle Detonation FX
			const FXList* fx = getProjectileDetonateFX(vet);
			if (fx != NULL) {
				Matrix3D laserMtx;
				Vector3 pos(sourcePos->x, sourcePos->y, sourcePos->z);
				Vector3 dir(v.x, v.y, v.z);
				dir.Normalize(); //This is fantastically crucial for calling buildTransformMatrix!!!!!
				laserMtx.buildTransformMatrix(pos, dir);
				FXList::doFXPos(fx, &targetPos, &laserMtx, 0.0f, NULL, getPrimaryDamageRadius(bonus));
			}

			if( inflictDamage )
			{
				dealDamageInternal( sourceID, damageID, &projectileDestination, bonus, isProjectileDetonation, wslot, specificBarrelToUse );
			}
			return TheGameLogic->getFrame();
		}

		const Coord3D* damagePos = getDamageDealtAtSelfPosition() ? sourcePos : &targetedPos; //victimPos;
		if (delayInFrames < 1.0f)
		{
			// go ahead and do it now
			//DEBUG_LOG(("WeaponTemplate::fireWeaponTemplate: firing weapon immediately!"));
			if( inflictDamage )
			{
				dealDamageInternal(sourceID, damageID, damagePos, bonus, isProjectileDetonation, wslot, specificBarrelToUse );
			}

			//-extraLogging
			#if defined(RTS_DEBUG)
				if( TheGlobalData->m_extraLogging )
					DEBUG_LOG( ("EARLY 4 (delayed damage applied now)") );
			#endif
			//end -extraLogging


			return TheGameLogic->getFrame();
		}
		else
		{
			UnsignedInt when = 0;
			if( TheWeaponStore && inflictDamage )
			{
				UnsignedInt delayInWholeFrames = REAL_TO_INT_CEIL(delayInFrames);
				when = TheGameLogic->getFrame() + delayInWholeFrames;
				//DEBUG_LOG(("WeaponTemplate::fireWeaponTemplate: firing weapon in %d frames (= %d)!", delayInWholeFrames,when));
				TheWeaponStore->setDelayedDamage(this, damagePos, when, sourceID, damageID, bonus);
			}

			//-extraLogging
			#if defined(RTS_DEBUG)
				if( TheGlobalData->m_extraLogging )
					DEBUG_LOG( ("EARLY 5 (delaying damage applied until frame %d)", when ) );
			#endif
			//end -extraLogging


			return when;
		}
	}
	else	// must be a projectile
	{
		Player *owningPlayer = sourceObj->getControllingPlayer(); //Need to know so missiles don't collide with firer
		Object *projectile = TheThingFactory->newObject( projectileTemplate, owningPlayer->getDefaultTeam() );
		projectile->setProducer(sourceObj);

		//If the player has battle plans (America Strategy Center), then apply those bonuses
		//to this object if applicable. Internally it validates certain kinds of objects.
		//When projectiles are created, weapon bonuses such as damage may get applied.
		if( owningPlayer->getNumBattlePlansActive() > 0 )
		{
			owningPlayer->applyBattlePlanBonusesForObject( projectile );
		}


		//Store the project ID in the object as the last projectile fired!
		if (projectileID)
			*projectileID = projectile->getID();

		// Notify special power tracking
		SpecialPowerCompletionDie *die = sourceObj->findSpecialPowerCompletionDie();
		if (die)
		{
			die->notifyScriptEngine();
			die = projectile->findSpecialPowerCompletionDie();
			if (die)
			{
				die->setCreator(INVALID_ID);
			}
		}
		else
		{
			die = projectile->findSpecialPowerCompletionDie();
			if (die)
			{
				die->setCreator(sourceObj->getID());
			}
		}

		//firingWeapon->newProjectileFired( sourceObj, projectile, victimObj, victimPos, launchPos );//The actual logic weapon needs to know this was created.
		firingWeapon->newProjectileFired( sourceObj, projectile, curTarget, &targetedPos, launchPos );//The actual logic weapon needs to know this was created.

		ProjectileUpdateInterface* pui = NULL;
		for (BehaviorModule** u = projectile->getBehaviorModules(); *u; ++u)
		{
			if ((pui = (*u)->getProjectileUpdateInterface()) != NULL)
				break;
		}
		if (pui)
		{
			if(launchPos)
			{
				Coord3D tmp = *launchPos;
				Real groundHeight = TheTerrainLogic->getGroundHeight(tmp.x, tmp.y);

				if (groundHeight + 2.0f >= tmp.z)
				{
					tmp.z = 99999.0f;
					PathfindLayerEnum newLayer = TheTerrainLogic->getHighestLayerForDestination(&tmp);
					projectile->setLayer(newLayer);

					// ensure we are slightly above the bridge, to account for fudge & sloppy art
					PathfindLayerEnum testLayer = TheTerrainLogic->getHighestLayerForDestination(&tmp);
					const Real FUDGE = 2.0f;
					tmp.z = TheTerrainLogic->getLayerHeight(tmp.x, tmp.y, testLayer) + FUDGE;
				}

				projectile->setPosition(&tmp);
				
				projectile->setOrientation(atan2(projectileDestination.y, projectileDestination.x));

				if(shrapnelLaunchID != INVALID_ID)
					pui->setShrapnelLaunchID(shrapnelLaunchID);
			}

			if(pui->projectileShouldDetonateOnGround())
			{
				Coord3D TargetCoord = *sourceObj->getCurrentTargetCoord();
				if(TargetCoord.z != 0.0f)
				{
					TargetCoord.z = 0.0f;
					TheGameLogic->findObjectByID(sourceID)->setCurrentTargetCoord(&TargetCoord);
				}
			}

			VeterancyLevel v = sourceObj->getVeterancyLevel();
			if( scatterRadius > 0.0f )
			{
				//With a scatter radius, don't follow the victim (overriding the intent).
				pui->projectileLaunchAtObjectOrPosition( NULL, &projectileDestination, sourceObj, wslot, specificBarrelToUse, this, m_projectileExhausts[v], launchPos );
			}
			else
			{
				pui->projectileLaunchAtObjectOrPosition(curTarget, &projectileDestination, sourceObj, wslot, specificBarrelToUse, this, m_projectileExhausts[v], launchPos );
			}
		}
		else
		{
			//DEBUG_CRASH(("Projectiles should implement ProjectileUpdateInterface!"));
			// actually, this is ok, for things like Firestorm.... (srj)
			projectile->setPosition(&projectileDestination);
		}

		//If we're launching a missile at a unit with valid countermeasures, then communicate it
		//if( projectile->isKindOf( KINDOF_SMALL_MISSILE ) && victimObj && victimObj->hasCountermeasures() )
		if( projectile && curTarget && curTarget->hasCountermeasuresExpanded(projectile) )
		{
			const AIUpdateInterface *ai = curTarget->getAI();
			//Only allow jets not currently supersonic to launch countermeasures
			if( ai && ai->getCurLocomotorSetType() != LOCOMOTORSET_SUPERSONIC )
			{
				//This function will determine now whether or not the fired projectile will be diverted to
				//an available decoy flare.
				curTarget->reportMissileForCountermeasures( projectile );
			}

		}
		//-extraLogging
		#if defined(RTS_DEBUG)
			if( TheGlobalData->m_extraLogging )
				DEBUG_LOG( ("DONE") );
		#endif
		//end -extraLogging

		return 0;
	}
}
//-------------------
void WeaponTemplate::createPreAttackFX
(
	const Object* sourceObj,
	WeaponSlotType wslot,
	Int specificBarrelToUse,
	const Object* victimObj,
	const Coord3D* victimPos
	//const WeaponBonus& bonus,
	//Weapon *firingWeapon,
) const
{
	// PLAY PRE ATTACK FX
	VeterancyLevel v = sourceObj->getVeterancyLevel();
	const FXList* fx = getPreAttackFX(v);

	if (fx) {
		Coord3D targetPos;
		if (victimObj)
		{
			victimObj->getGeometryInfo().getCenterPosition(*victimObj->getPosition(), targetPos);
		}
		else if (victimPos)
		{
			targetPos.set(victimPos);
		}

		/*DEBUG_LOG((">>> INFO - creating PRE_ATTACK FX for '%s' with victim '%s' and pos '(%f, %f, %f)' \n",
			sourceObj->getTemplate()->getName().str(),
			victimObj ? victimObj->getTemplate()->getName().str() : "None",
			targetPos.x, targetPos.y, targetPos.z));*/

		StealthUpdate *stealth = sourceObj->getStealth();

		Bool handled = false;
		Bool isDisguisedAndCheckIfNeedOffset = stealth && stealth->isDisguisedAndCheckIfNeedOffset();
		Drawable *currentDraw = isDisguisedAndCheckIfNeedOffset ? stealth->getDrawableTemplateWhileDisguised() : sourceObj->getDrawable();

		if(isDisguisedAndCheckIfNeedOffset)
		{
			if(!currentDraw)
				return; // Sanity

			Weapon::setFirePositionForDrawable(sourceObj, currentDraw, wslot, specificBarrelToUse);
			if(TheGlobalData->m_useEfficientDrawableScheme)
				TheGameClient->informClientNewDrawable(currentDraw);

			handled = currentDraw->handleWeaponPreAttackFX(wslot,
				specificBarrelToUse,
				fx,
				getWeaponSpeed(),
				0.0f, //TODO: Enable recoil stats if we want to have PreAttack specific recoil amount
				0.0f,
				&targetPos,
				0.0f
			);

			// IamInnocent - HOPEFULLY there are no desyncs with this feature
			if(fx && handled)
			{
				// IamInnocent - technical fix for Disguised Drawables using original offsets for handling Recoil and Muzzles
				/// Maximum barrel allow for drawables using different templates while using its Original Drawable amount of barrels is the Drawable's Barrel Count - 1
				Int barrelCount = specificBarrelToUse;
				if(sourceObj->getTemplate() != sourceObj->getDrawable()->getTemplate() && currentDraw->getTemplate() == sourceObj->getTemplate())
				{
					Int maxBarrelCount = stealth->getBarrelCountDisguisedTemplate(wslot);
					// IamInnocent - added to check to prevent crashes if the barrelCount has more barrels than the current drawable
					while(barrelCount > maxBarrelCount)
					{
						if(maxBarrelCount <= 0)
							barrelCount = 0;
						else
							barrelCount -= maxBarrelCount;
					}
					maxBarrelCount = max(0, maxBarrelCount-1);
					barrelCount = min(barrelCount, maxBarrelCount);
				}
				sourceObj->getDrawable()->handleWeaponFireRecoil(wslot, specificBarrelToUse, 0.0f, 0.0f, FALSE, TRUE);
			}
		}
		else
		{
			handled = currentDraw->handleWeaponPreAttackFX(wslot,
				specificBarrelToUse,
				fx,
				getWeaponSpeed(),
				0.0f, //TODO: Enable recoil stats if we want to have PreAttack specific recoil amount
				0.0f,
				&targetPos,
				0.0f
			);
		}

		if (handled == false && fx != NULL)
		{
			const Coord3D* where = isContactWeapon() ? &targetPos : currentDraw->getPosition();
			FXList::doFXPos(fx, where, currentDraw->getTransformMatrix(), getWeaponSpeed(), &targetPos, 0.0f);
		}
	}
}
//-------------------------------------------------------------------------------------------------
#if RETAIL_COMPATIBLE_CRC
void WeaponTemplate::trimOldHistoricDamage() const
{
	UnsignedInt expirationDate = TheGameLogic->getFrame() - TheGlobalData->m_historicDamageLimit;
	while (!m_historicDamage.empty())
	{
		HistoricWeaponDamageInfo& h = m_historicDamage.front();
		if (h.frame <= expirationDate)
		{
			m_historicDamage.pop_front();
			continue;
		}
		else
		{
			// since they are in strict chronological order,
			// stop as soon as we get to a nonexpired one
			break;
		}
	}
}
#else
void WeaponTemplate::trimOldHistoricDamage() const
{
	if (m_historicDamage.empty())
		return;

	const UnsignedInt currentFrame = TheGameLogic->getFrame();
	const UnsignedInt expirationFrame = currentFrame - m_historicBonusTime;

	HistoricWeaponDamageList::iterator it = m_historicDamage.begin();

	while (it != m_historicDamage.end())
	{
		if (it->frame <= expirationFrame)
			it = m_historicDamage.erase(it);
		else
			break;
	}
}
#endif

//-------------------------------------------------------------------------------------------------
void WeaponTemplate::trimTriggeredHistoricDamage() const
{
	HistoricWeaponDamageList::iterator it = m_historicDamage.begin();

	while (it != m_historicDamage.end())
	{
		if (it->triggerId == m_historicDamageTriggerId)
			it = m_historicDamage.erase(it);
		else
			++it;
	}
}

//-------------------------------------------------------------------------------------------------
static Bool is2DDistSquaredLessThan(const Coord3D& a, const Coord3D& b, Real distSqr)
{
	Real da = sqr(a.x - b.x) + sqr(a.y - b.y);
	return da <= distSqr;
}

//-------------------------------------------------------------------------------------------------
// Copied from WeaponSet.cpp
//-------------------------------------------------------------------------------------------------
static Int getVictimAntiMask(const Object* victim)
{
	if (victim->isKindOf(KINDOF_SMALL_MISSILE))
	{
		//All missiles are also projectiles!
		return WEAPON_ANTI_SMALL_MISSILE;
	}
	else if (victim->isKindOf(KINDOF_BALLISTIC_MISSILE))
	{
		return WEAPON_ANTI_BALLISTIC_MISSILE;
	}
	else if (victim->isKindOf(KINDOF_PROJECTILE))
	{
		return WEAPON_ANTI_PROJECTILE;
	}
	else if (victim->isKindOf(KINDOF_MINE) || victim->isKindOf(KINDOF_DEMOTRAP))
	{
		return WEAPON_ANTI_MINE | WEAPON_ANTI_GROUND;
	}
	else if (victim->isAirborneTarget())
	{
		if (victim->isKindOf(KINDOF_VEHICLE))
		{
			return WEAPON_ANTI_AIRBORNE_VEHICLE;
		}
		else if (victim->isKindOf(KINDOF_INFANTRY))
		{
			return WEAPON_ANTI_AIRBORNE_INFANTRY;
		}
		else if (victim->isKindOf(KINDOF_PARACHUTE))
		{
			return WEAPON_ANTI_PARACHUTE;
		}
		else if (!victim->isKindOf(KINDOF_UNATTACKABLE))
		{
			DEBUG_CRASH(("Object %s is being targetted as airborne, but is not infantry, nor vehicle. Is this legit? -- tell Kris", victim->getName().str()));
		}
		return 0;
	}
	else
	{
		return WEAPON_ANTI_GROUND;
	}
}

//-------------------------------------------------------------------------------------------------
#if RETAIL_COMPATIBLE_CRC
void WeaponTemplate::processHistoricDamage(const Object* source, const Coord3D* pos) const
{
	//
	/** @todo We need to rewrite the historic stuff ... if you fire 5 missiles, and the 5th,
	// one creates a firestorm ... and then half a second later another volley of 5 missiles
	// come in, the second wave of 5 missiles would all do a historic weapon, making 5 more
	// firestorms (CBD) */
	//

	if( m_historicBonusCount > 0 && m_historicBonusWeapon != this && (!m_shrapnelBonusWeapon || m_shrapnelBonusWeapon != this))
	{
		trimOldHistoricDamage();

		Real radSqr = m_historicBonusRadius * m_historicBonusRadius;
		Int count = 0;
		UnsignedInt frameNow = TheGameLogic->getFrame();
		UnsignedInt oldestThatWillCount = frameNow - m_historicBonusTime; // Anything before this frame is "more than two seconds ago" eg
		for( HistoricWeaponDamageList::const_iterator it = m_historicDamage.begin(); it != m_historicDamage.end(); ++it )
		{
			if( it->frame >= oldestThatWillCount &&
					is2DDistSquaredLessThan( *pos, it->location, radSqr ) )
			{
				// This one is close enough in time and distance, so count it. This is tracked by template since it applies
				// across units, so don't try to clear historicDamage on success in here.
				++count;
			}
		}

		if( count >= m_historicBonusCount - 1 )	// minus 1 since we include ourselves implicitly
		{
		  TheWeaponStore->createAndFireTempWeapon(m_historicBonusWeapon, source, pos);

			/** @todo E3 hack! Clear the list for now to make sure we don't have multiple firestorms
				* remove this when the branches merge back into one.  What is causing the
				* multiple firestorms, who is to say ... this is a plug, not a fix! */
			m_historicDamage.clear();

		}
		else
		{

			// add AFTER checking for historic stuff
			m_historicDamage.push_back( HistoricWeaponDamageInfo(frameNow, *pos) );

		}

	}
}
#else
void WeaponTemplate::processHistoricDamage(const Object* source, const Coord3D* pos) const
{
	if( m_historicBonusCount > 0 && m_historicBonusWeapon != this && (!m_shrapnelBonusWeapon || m_shrapnelBonusWeapon != this))
	{
		trimOldHistoricDamage();

		++m_historicDamageTriggerId;

		const Int requiredCount = m_historicBonusCount - 1; // minus 1 since we include ourselves implicitly
		if (m_historicDamage.size() >= requiredCount)
		{
			const Real radSqr = m_historicBonusRadius * m_historicBonusRadius;
			Int count = 0;

			for (HistoricWeaponDamageList::iterator it = m_historicDamage.begin(); it != m_historicDamage.end(); ++it)
			{
				if (is2DDistSquaredLessThan(*pos, it->location, radSqr))
				{
					// This one is close enough in time and distance, so count it. This is tracked by template since it applies
					// across units, so don't try to clear historicDamage on success in here.
					it->triggerId = m_historicDamageTriggerId;

					if (++count == requiredCount)
					{
						TheWeaponStore->createAndFireTempWeapon(m_historicBonusWeapon, source, pos);
						trimTriggeredHistoricDamage();
						return;
					}
				}
			}
		}

		// add AFTER checking for historic stuff
		m_historicDamage.push_back(HistoricWeaponDamageInfo(TheGameLogic->getFrame(), *pos));
	}
}
#endif

static Bool testValidForAttack(const Object* victim, Int antiMask, ObjectID primaryID, Relationship r)
{
	// Sanity
	if( !victim )
		return FALSE;

	// Don't check for dead or removed targets
	if( victim->isDestroyed() || victim->isEffectivelyDead() )
		return FALSE;
	
	// Victim is not attackable, don't do anything
	if( victim->isKindOf( KINDOF_UNATTACKABLE ) )
		return FALSE;

	// Victim is valid for attacking
	if( victim->isKindOf(KINDOF_VEHICLE) ||
		victim->isKindOf(KINDOF_AIRCRAFT) ||
		victim->isKindOf(KINDOF_STRUCTURE) ||
		victim->isKindOf(KINDOF_INFANTRY) ||
		victim->isKindOf(KINDOF_MINE) ||
		victim->isKindOf(KINDOF_SHRUBBERY) ||
		victim->isKindOf(KINDOF_PARACHUTE)
	  )
	  return TRUE;
	
	// Victim is Projectile
	Int targetAntiMask = 0;
	if (victim->isKindOf(KINDOF_SMALL_MISSILE))
	{
		//All missiles are also projectiles!
		targetAntiMask = WEAPON_ANTI_SMALL_MISSILE;
	}
	else if (victim->isKindOf(KINDOF_BALLISTIC_MISSILE))
	{
		targetAntiMask = WEAPON_ANTI_BALLISTIC_MISSILE;
	}
	else if (victim->isKindOf(KINDOF_PROJECTILE))
	{
		targetAntiMask = WEAPON_ANTI_PROJECTILE;
	}

	if( targetAntiMask > 0 )
	{
		// If the projectile is our main target or the projectile is not an ally, do Shrapnel
		if( (antiMask & targetAntiMask) != 0 && (victim->getID() == primaryID || r != ALLIES) )
			return TRUE;
	}

	return FALSE;
}

//-------------------------------------------------------------------------------------------------
void WeaponTemplate::dealDamageInternal(ObjectID sourceID, ObjectID victimID, const Coord3D *pos, const WeaponBonus& bonus, Bool isProjectileDetonation, WeaponSlotType wslot, Int specificBarrelToUse) const
{
	if (sourceID == 0)	// must have a source
		return;

	if (victimID == 0 && pos == NULL)	// must have some sort of destination
		return;

	Object *source = TheGameLogic->findObjectByID(sourceID);	// might be null...

	processHistoricDamage(source, pos);

//DEBUG_LOG(("WeaponTemplate::dealDamageInternal: dealing damage %s at frame %d",m_name.str(),TheGameLogic->getFrame()));

	Bool doShrapnel = FALSE;
	ObjectID shrapnelVictimID = INVALID_ID;

	// if there's a specific victim, use it's pos (overriding the value passed in)
	Object *primaryVictim = victimID ? TheGameLogic->findObjectByID(victimID) : NULL;	// might be null...
	if (primaryVictim)
	{
		pos = primaryVictim->getPosition();
	}

	VeterancyLevel v = LEVEL_REGULAR;

	if(source)
		v = source->getVeterancyLevel();

	DamageType damageType = getDamageType();
	DeathType deathType = getDeathType();
	ObjectStatusTypes damageStatusType = getDamageStatusType();
	AsciiString customDamageType = getCustomDamageType();
	AsciiString customDamageStatusType = getCustomDamageStatusType();
	AsciiString customDeathType = getCustomDeathType();
	Bool IsFlame = getIsFlame();
	Bool ProjectileCollidesWithBurn = getProjectileCollidesWithBurn();
	Bool IsPoison = getIsPoison();
	Bool PoisonMuzzleFlashesGarrison = getPoisonMuzzleFlashesGarrison();
	Bool IsDisarm = getIsDisarm();
	Bool KillsGarrison = getKillsGarrison();
	Int KillsGarrisonAmount = getKillsGarrisonAmount();
	AsciiString SpecificVoice = PlaySpecificVoice();
	DamageType DamageFXOverride = getDamageFXOverride();
	Real StatusDuration = getStatusDuration();
	Bool DoStatusDamage = getDoStatusDamage(v);
	Bool StatusDurationTypeCorrelate = getStatusDurationTypeCorrelate();
	TintStatus TintStatusType = getTintStatusType(v);
	AsciiString CustomTintStatusType = getCustomTintStatusType(v);
	Bool IsSubdual = getIsSubdual(v);
	Bool subdualDealsNormalDamage = getSubdualDealsNormalDamage(v);
	Real subdualDamageMultiplier = getSubdualDamageMultiplier(v);
	KindOfMaskType subdualForbiddenKindOf = getSubdualForbiddenKindOf();
	Bool IsNotAbsoluteKill = getIsNotAbsoluteKill();
	Bool ClearsParasite = getClearsParasite();
	std::vector<AsciiString> ClearsParasiteKeys = getClearsParasiteKeys();
	Bool IsMissileAttractor = getIsMissileAttractor();
	Bool SubdueProjectileNoDamage = getSubdueProjectileNoDamage();
	AsciiString SubdualCustomType = getSubdualCustomType();
	AsciiString CustomSubdualCustomTint = getCustomSubdualCustomTint(v);
	TintStatus CustomSubdualTint = getCustomSubdualTint(v);
	Bool CustomSubdualHasDisable = getCustomSubdualHasDisable(v);
	Bool CustomSubdualHasDisableProjectiles = getCustomSubdualHasDisableProjectiles(v);
	Bool CustomSubdualClearOnTrigger = getCustomSubdualClearOnTrigger(v);
	Bool CustomSubdualDoStatus = getCustomSubdualDoStatus(v);
	const ObjectCreationList *CustomSubdualOCL = getCustomSubdualOCL(v);
	DisabledType CustomSubdualDisableType = getCustomSubdualDisableType();
	Bool CustomSubdualRemoveSubdualTintOnDisable =  getCustomSubdualRemoveSubdualTintOnDisable();
	TintStatus CustomSubdualDisableTint = getCustomSubdualDisableTint();
	AsciiString CustomSubdualDisableCustomTint = getCustomSubdualDisableCustomTint();
	AsciiString CustomSubdualDisableSound =  getCustomSubdualDisableSound();
	AsciiString CustomSubdualDisableRemoveSound =  getCustomSubdualDisableRemoveSound();
	ProtectionTypeFlags ProtectionTypes = getProtectionTypes();
	Real MinDamageHeight = getMinDamageHeight();
	Real MaxDamageHeight = getMaxDamageHeight();
	Bool DamagesSelfOnly = getDamagesSelfOnly();
	UnsignedInt InvulnerabilityDuration = getInvulnerabilityDuration();

	const Real CLOSE_ENOUGH = 1.0f;
	
	if (getProjectileTemplate() == NULL || isProjectileDetonation)
	{
		// Railgun Variables
		Bool isRailgun = getIsRailgun();
		Int railgunAmount = getRailgunPierceAmount();
		Bool checkForRailgunOnly = false;
		DamageType railgunDamageType = getRailgunDamageType() == DAMAGE_NUM_TYPES ? damageType : getRailgunDamageType();
		DeathType railgunDeathType = getRailgunDeathType() == DEATH_NONE ? deathType : getRailgunDeathType();
		DamageType railgunDamageFXOverride = getRailgunDamageFXOverride() == DAMAGE_UNRESISTABLE ? DamageFXOverride : getRailgunDamageFXOverride();
		AsciiString railgunCustomDamageType = getRailgunCustomDamageType().isEmpty() ? customDamageType : getRailgunCustomDamageType();
		AsciiString railgunCustomDeathType = getRailgunCustomDeathType().isEmpty() ? customDeathType : getRailgunCustomDeathType();

		SimpleObjectIterator *iter;
		Object *curVictim;
		Real curVictimDistSqr;

		Real primaryRadius = getPrimaryDamageRadius(bonus);
		Real secondaryRadius = getSecondaryDamageRadius(bonus);
		Real primaryDamage = getPrimaryDamage(bonus);
		Real secondaryDamage = getSecondaryDamage(bonus);
		Int affects = getAffectsMask();

		DEBUG_ASSERTCRASH(secondaryRadius >= primaryRadius || secondaryRadius == 0.0f, ("secondary radius should be >= primary radius (or zero)"));

		Real primaryRadiusSqr = sqr(primaryRadius);
		Real radius = max(primaryRadius, secondaryRadius);
		if (radius > 0.0f)
		{
			iter = ThePartitionManager->iterateObjectsInRange(pos, radius, DAMAGE_RANGE_CALC_TYPE);
			curVictim = iter->firstWithNumeric(&curVictimDistSqr);
		}
		else
		{
			//DEBUG_ASSERTCRASH(primaryVictim != NULL, ("weapons without radii should always pass in specific victims"));
			// check against victimID rather than primaryVictim, since we may have targeted a legitimate victim
			// that got killed before the damage was dealt... (srj)
			//DEBUG_ASSERTCRASH(victimID != 0, ("weapons without radii should always pass in specific victims"));
			iter = NULL;
			curVictim = primaryVictim;
			curVictimDistSqr = 0.0f;

			if( affects & WEAPON_KILLS_SELF )
			{
				DamageInfo damageInfo;
				damageInfo.in.m_damageType = damageType;
				damageInfo.in.m_deathType = deathType;
				damageInfo.in.m_sourceID = sourceID;
				damageInfo.in.m_sourcePlayerMask = 0;
				damageInfo.in.m_damageStatusType = damageStatusType;
				damageInfo.in.m_playSpecificVoice = SpecificVoice;
				damageInfo.in.m_damageFXOverride = DamageFXOverride;
				damageInfo.in.m_statusDuration = StatusDuration;
				damageInfo.in.m_doStatusDamage = DoStatusDamage;
				damageInfo.in.m_statusDurationTypeCorrelate = StatusDurationTypeCorrelate;
				damageInfo.in.m_tintStatus = TintStatusType;
				damageInfo.in.m_customTintStatus = CustomTintStatusType;
				damageInfo.in.m_notAbsoluteKill = IsNotAbsoluteKill;
				damageInfo.in.m_clearsParasite = ClearsParasite;
				damageInfo.in.m_clearsParasiteKeys = ClearsParasiteKeys;
				damageInfo.in.m_customDamageType = customDamageType;
				damageInfo.in.m_customDamageStatusType = customDamageStatusType;
				damageInfo.in.m_customDeathType = customDeathType;
				damageInfo.in.m_amount = HUGE_DAMAGE_AMOUNT;
				source->attemptDamage( &damageInfo );

				privateDoShrapnel(sourceID, victimID, pos);
				return;
			}
		}
		MemoryPoolObjectHolder hold(iter);

		for (;; curVictim = iter ? iter->nextWithNumeric(&curVictimDistSqr) : NULL)
		{
			// IamInnocent - Check for Railgun targets within LOS
			if(curVictim == NULL)
			{
				// Do nothing if we only damage ourselves
				if(DamagesSelfOnly || affects == WEAPON_AFFECTS_SELF)
					break;
				
				// Refresh the checklist for Railgun
				if(isRailgun)
				{
					Coord3D railgunDirection;
					Bool isRailgunLinear = getRailgunIsLinear();
					Bool RailgunPiercesBehind = getRailgunPiercesBehind();
					Real RailgunExtraDistance = getRailgunExtraDistance();
					Real RailgunMaxDistance = getRailgunMaxDistance();
					Real extraHeight = 0.0f;
					Coord3D srcPos, posOther;
					
					if (source)
					{
						srcPos = *source->getPosition();
						if(isRailgunLinear)
						{
							StealthUpdate *stealth = source->getStealth();
							extraHeight = srcPos.z;

							if(stealth && stealth->isDisguisedAndCheckIfNeedOffset())
								stealth->getDrawableTemplateWhileDisguised()->getWeaponFireOffset(wslot, specificBarrelToUse, &srcPos);
							else
								source->getDrawable()->getWeaponFireOffset(wslot, specificBarrelToUse, &srcPos);

							extraHeight = srcPos.z - extraHeight;
						}
					}
					else
					{
						// If there is no source, we do nothing and break;
						break;
					}

					// Set the railgun targeting position
					if ( primaryVictim )
					{
						posOther = *primaryVictim->getPosition();
						if(isRailgunLinear)
							posOther.z += extraHeight;
					}
					else
					{
						posOther = *pos;
					}

					if(RailgunExtraDistance != 0)
					{
						// Extend (or Reduce) the targeting distance based on the configuration
						railgunDirection = posOther;
						railgunDirection.sub( &srcPos );

						Real railgunAngle = atan2(railgunDirection.y, railgunDirection.x);
						posOther.x += Cos(railgunAngle) * RailgunExtraDistance;
						posOther.y += Sin(railgunAngle) * RailgunExtraDistance;
					}
					
					if(RailgunMaxDistance)
					{
						// If there is any Max Distance configured, then calculate whether it has reached the Max Distance
						railgunDirection = posOther;
						railgunDirection.sub( &srcPos );
						Real railgunDist = railgunDirection.length();
						if(railgunDist > RailgunMaxDistance)
						{
							// Set the new position based on the direction of the Vector
							Real ReductionRatio = (railgunDist - RailgunMaxDistance) / railgunDist;
							posOther.x -= (posOther.x - srcPos.x) * ReductionRatio;
							posOther.y -= (posOther.y - srcPos.y) * ReductionRatio;
							posOther.z -= (posOther.z - srcPos.z) * ReductionRatio;
						}
					}

					isRailgun = FALSE; // Only do this once
					checkForRailgunOnly = TRUE;

					IterOrderType order = railgunAmount > 0 ? ITER_SORTED_NEAR_TO_FAR : ITER_FASTEST;

					iter = ThePartitionManager->iterateObjectsAlongLine(source, &srcPos, &posOther, getRailgunRadius(), getRailgunInfantryRadius(), getRailgunRadiusCheckPerDistance(), getRailgunFX(v), getRailgunOCL(v), DAMAGE_RANGE_CALC_TYPE, RailgunPiercesBehind, NULL, order);
					curVictim = iter->firstWithNumeric(&curVictimDistSqr);

					// If nothing to check, do nothing and return
					if(curVictim == NULL)
						break;
				}
				else
					break;
			}

			// Check for Railgun Only
			if( checkForRailgunOnly )
			{
				// Don't deal damage against the primary victim
				if( primaryVictim && primaryVictim->getID() == curVictim->getID() )
					continue;
			}
			
			Bool killSelf = false;
			if (source != NULL)
			{
				// Special, only deal damage to self
				if(DamagesSelfOnly && source != curVictim)
				{
					continue;
				}
				
				// anytime something is designated as the "primary victim" (ie, the direct target
				// of the weapon), we ignore all the "affects" flags.
				if (curVictim != primaryVictim)
				{
					if( (affects & WEAPON_KILLS_SELF) && source == curVictim )
					{
						killSelf = true;
					}
					else if(!DamagesSelfOnly)
					{

						// should object ever be allowed to damage themselves? methinks not...
						// exception: a few weapons allow this (eg, for suicide bombers).
						if( (affects & WEAPON_AFFECTS_SELF) == 0 )
						{
							// Remember that source is a missile for some units, and they don't want to injure them'selves' either
							if( source == curVictim || source->getProducerID() == curVictim->getID() )
							{
								//DEBUG_LOG(("skipping damage done to SELF..."));
								continue;
							}
						}
						
						//IamInnocent - Fix declaring RadiusDamageAffects = SELF solely not checking for Self and needed to be declared together with ALLIES 3/11/2025
						if( affects == WEAPON_AFFECTS_SELF )
						{
							if( !(source == curVictim || source->getProducerID() == curVictim->getID()) )
								continue;
						}

						if( affects & WEAPON_DOESNT_AFFECT_SIMILAR )
						{
							//This means we probably are affecting allies, but don't want to kill nearby members that are the same type as us.
							//A good example are a group of terrorists blowing themselves up. We don't want to cause a domino effect that kills
							//all of them.
							if( source->getTemplate()->isEquivalentTo(curVictim->getTemplate()) && source->getRelationship( curVictim ) == ALLIES )
							{
								continue;
							}
						}

						if ((affects & WEAPON_DOESNT_AFFECT_AIRBORNE) != 0 && curVictim->isSignificantlyAboveTerrain())
						{
							continue;
						}

						/*
							The idea here is: if its our ally(/enemies), AND it's not the direct target, AND the weapon doesn't
							do radius-damage to allies(/enemies)... skip it.
						*/
						Relationship r = curVictim->getRelationship(source);
						Int requiredMask;
						if (r == ALLIES)
							requiredMask = WEAPON_AFFECTS_ALLIES;
						else if (r == ENEMIES)
							requiredMask = WEAPON_AFFECTS_ENEMIES;
						else /* r == NEUTRAL */
							requiredMask = WEAPON_AFFECTS_NEUTRALS;

						if( affects == WEAPON_AFFECTS_SELF )
						{
							requiredMask = WEAPON_AFFECTS_SELF;
						}

						if( !(affects & requiredMask) )
						{
							//Skip if we aren't affected by this weapon.
							continue;
						}

					}
				}
			}

			// Minus the Railgun Amount
			if(checkForRailgunOnly && testValidForAttack(curVictim, getAntiMask(), INVALID_ID, NEUTRAL) && railgunAmount-- == 0)
				break;

			DamageInfo damageInfo;
			damageInfo.in.m_damageType = checkForRailgunOnly ? railgunDamageType : damageType;
			damageInfo.in.m_deathType = checkForRailgunOnly ? railgunDeathType : deathType;
			damageInfo.in.m_sourceID = sourceID;
			damageInfo.in.m_sourcePlayerMask = 0;
			damageInfo.in.m_damageStatusType = damageStatusType;

			damageInfo.in.m_customDamageType = checkForRailgunOnly ? railgunCustomDamageType : customDamageType;
			damageInfo.in.m_customDamageStatusType = customDamageStatusType;
			damageInfo.in.m_customDeathType = checkForRailgunOnly ? railgunCustomDeathType : customDeathType;

			damageInfo.in.m_isFlame = IsFlame;
			damageInfo.in.m_projectileCollidesWithBurn = ProjectileCollidesWithBurn;
			damageInfo.in.m_isPoison = IsPoison;
			damageInfo.in.m_poisonMuzzleFlashesGarrison = PoisonMuzzleFlashesGarrison;
			damageInfo.in.m_isDisarm = IsDisarm;
			damageInfo.in.m_killsGarrison = KillsGarrison;
			damageInfo.in.m_killsGarrisonAmount = KillsGarrisonAmount;
			damageInfo.in.m_playSpecificVoice = SpecificVoice;
			damageInfo.in.m_damageFXOverride = checkForRailgunOnly ? railgunDamageFXOverride : DamageFXOverride;

			damageInfo.in.m_statusDuration = StatusDuration;
			damageInfo.in.m_doStatusDamage = DoStatusDamage;
			damageInfo.in.m_statusDurationTypeCorrelate = StatusDurationTypeCorrelate;
			damageInfo.in.m_tintStatus = TintStatusType;
			damageInfo.in.m_customTintStatus = CustomTintStatusType;

			damageInfo.in.m_isSubdual = IsSubdual;
			damageInfo.in.m_subdualDealsNormalDamage = subdualDealsNormalDamage;
			damageInfo.in.m_subdualDamageMultiplier = subdualDamageMultiplier;
			damageInfo.in.m_subdualForbiddenKindOf = subdualForbiddenKindOf;

			damageInfo.in.m_notAbsoluteKill = IsNotAbsoluteKill;
			damageInfo.in.m_clearsParasite = ClearsParasite;
			damageInfo.in.m_clearsParasiteKeys = ClearsParasiteKeys;

			damageInfo.in.m_isMissileAttractor = IsMissileAttractor;
			damageInfo.in.m_subduedProjectileNoDamage = SubdueProjectileNoDamage;

			damageInfo.in.m_subdualCustomType = SubdualCustomType;
			damageInfo.in.m_customSubdualCustomTint = CustomSubdualCustomTint;
			damageInfo.in.m_customSubdualTint = CustomSubdualTint;
			damageInfo.in.m_customSubdualHasDisable = CustomSubdualHasDisable;
			damageInfo.in.m_customSubdualHasDisableProjectiles = CustomSubdualHasDisableProjectiles;
			damageInfo.in.m_customSubdualClearOnTrigger = CustomSubdualClearOnTrigger;
			damageInfo.in.m_customSubdualDoStatus = CustomSubdualDoStatus;
			damageInfo.in.m_customSubdualOCL = CustomSubdualOCL;
			damageInfo.in.m_customSubdualDisableType = CustomSubdualDisableType;
			damageInfo.in.m_customSubdualDisableTint = CustomSubdualDisableTint;
			damageInfo.in.m_customSubdualDisableCustomTint = CustomSubdualDisableCustomTint;
			damageInfo.in.m_customSubdualRemoveSubdualTintOnDisable = CustomSubdualRemoveSubdualTintOnDisable;
			damageInfo.in.m_customSubdualDisableSound = CustomSubdualDisableSound;
			damageInfo.in.m_customSubdualDisableRemoveSound = CustomSubdualDisableRemoveSound;

			damageInfo.in.m_protectionTypes = ProtectionTypes;

			damageInfo.in.m_minDamageHeight = MinDamageHeight;
			damageInfo.in.m_maxDamageHeight = MaxDamageHeight;
			
			Coord3D damageDirection;
			damageDirection.zero();
			if( curVictim && source )
			{
				damageDirection.set( curVictim->getPosition() );
				damageDirection.sub( source->getPosition() );
			}

			Real allowedAngle = getRadiusDamageAngle();
			if( allowedAngle < PI )
			{
				if( curVictim == NULL  ||  source == NULL )
					continue; // We are directional damage, but can't figure out our direction.  Just bail.

				// People can only be hit in a cone oriented as the firer is oriented
				Vector3 sourceVector = source->getTransformMatrix()->Get_X_Vector();
				Vector3 damageVector(damageDirection.x, damageDirection.y, damageDirection.z);
				sourceVector.Normalize();
				damageVector.Normalize();

				// These are now normalized, so the dot productis actually the Cos of the angle they form
				// A smaller Cos would mean a more obtuse angle
				if( Vector3::Dot_Product(sourceVector, damageVector) < Cos(allowedAngle) )
					continue;// Too far to the side, can't hurt them.
			}

			// Invulnerable mechanic from OCL.
			// Doesn't actually make you invulnerable, but makes your relationship considered ALLIES for targeting instead
			if(InvulnerabilityDuration > 0)
				curVictim->goInvulnerable(InvulnerabilityDuration);

			// Grab the vector between the source object causing the damage and the victim in order that we can
			// simulate a shockwave pushing objects around
			damageInfo.in.m_shockWaveAmount = m_shockWaveAmount;
			if (damageInfo.in.m_shockWaveAmount != 0.0f)
			{
				// Calculate the vector of the shockwave
				Coord3D shockWaveVector = damageDirection;
				
				if(damageInfo.in.m_shockWaveAmount < 0)
				{
					damageInfo.in.m_shockWaveAmount *= -1;
					if(m_shockWaveUseCenter)
					{
						shockWaveVector.set( pos );
						shockWaveVector.sub( curVictim->getPosition() );
					}
					else if(source)
					{
						shockWaveVector.set( source->getPosition() );
						shockWaveVector.sub( curVictim->getPosition() );
					}
				}
				else if(m_shockWaveUseCenter)
				{
					shockWaveVector.set( curVictim->getPosition() );
					shockWaveVector.sub( pos );
				}

				if(m_shockWaveRespectsCenter)
				{
					if(m_shockWaveUseCenter)
					{
						if(damageInfo.in.m_shockWaveAmount > shockWaveVector.length())
							damageInfo.in.m_shockWaveAmount = shockWaveVector.length();
					}
					else
					{
						Coord3D centerVector;

						centerVector.set( curVictim->getPosition() );
						centerVector.sub( pos );

						if(damageInfo.in.m_shockWaveAmount > centerVector.length())
							damageInfo.in.m_shockWaveAmount = centerVector.length();
					}

					/*if(fabs(centerVector.x) < damageInfo.in.m_shockWaveAmount)
						shockWaveVector.x = centerVector.x / damageInfo.in.m_shockWaveAmount;
					if(fabs(centerVector.y) < damageInfo.in.m_shockWaveAmount)
						shockWaveVector.y = centerVector.y / damageInfo.in.m_shockWaveAmount;
					if(fabs(centerVector.z) < damageInfo.in.m_shockWaveAmount)
						shockWaveVector.z = centerVector.z / damageInfo.in.m_shockWaveAmount;*/
				}

				// Guard against zero vector. Make vector stright up if that is the case
				if (fabs(shockWaveVector.x) < WWMATH_EPSILON &&
						fabs(shockWaveVector.y) < WWMATH_EPSILON &&
						fabs(shockWaveVector.z) < WWMATH_EPSILON)
				{
					shockWaveVector.z = 1.0f;
				}

				// Populate the damge information with the shockwave information
				damageInfo.in.m_shockWaveVector = shockWaveVector;
				damageInfo.in.m_shockWaveRadius = m_shockWaveRadius;
				damageInfo.in.m_shockWaveTaperOff = m_shockWaveTaperOff;
				damageInfo.in.m_shockWaveAffectsAirborne = m_shockWaveAffectsAirborne;
				damageInfo.in.m_shockWavePullsAirborne = m_shockWavePullsAirborne;
			}

			if (m_magnetInfantryAmount && curVictim && curVictim->isKindOf(KINDOF_INFANTRY))
				damageInfo.in.m_magnetAmount = m_magnetInfantryAmount;
			else
				damageInfo.in.m_magnetAmount = m_magnetAmount;
			if (damageInfo.in.m_magnetAmount != 0.0f && curVictim && 
				  curVictim->getAIUpdateInterface() && 
				  curVictim->getAIUpdateInterface()->getCurLocomotor() &&
				  (!curVictim->isAirborneTarget() || !m_magnetNoAirborne ))
			{
				Real distance;
				Coord3D magnetVector = damageDirection;

				if(m_magnetLinearDistanceCalc)
				{
					if(m_magnetUseCenter)
						distance = ThePartitionManager->getDistanceSquared(curVictim, pos, ATTACK_RANGE_CALC_TYPE);
					else
						distance = ThePartitionManager->getDistanceSquared(source, curVictim->getPosition(), ATTACK_RANGE_CALC_TYPE);
					distance = sqrt(distance);
				}
				else
				{
					if(m_magnetUseCenter)
					{
						magnetVector.set( curVictim->getPosition() );
						magnetVector.sub( pos );
					}
					distance = magnetVector.length();
				}

				if(distance > m_magnetMinDistance && ( !m_magnetMaxDistance || distance < m_magnetMaxDistance ))
				{
					// Populate the damge information with the magnet information
					if(m_magnetTaperOffDistance > 0 && distance > m_magnetTaperOffDistance)
					{
						Real ratio = max(0.0f, Real(1.0f - ( distance - m_magnetTaperOffDistance ) * ( m_magnetTaperOffRatio / m_magnetTaperOffDistance ) ));
						
						damageInfo.in.m_magnetAmount *= ratio;
					}
					else if(m_magnetTaperOnDistance > 0 && distance < m_magnetTaperOnDistance)
					{
						Real ratio = max(0.0f, Real(1.0f + ( m_magnetTaperOnDistance - distance ) * ( m_magnetTaperOnRatio / m_magnetTaperOnDistance ) ));
						
						damageInfo.in.m_magnetAmount *= ratio;
					}

					if(damageInfo.in.m_magnetAmount != 0.0f)
					{
						if(damageInfo.in.m_magnetAmount < 0)
						{
							damageInfo.in.m_magnetAmount *= -1;
							if(m_magnetUseCenter)
							{
								magnetVector.set( pos );
								magnetVector.sub( curVictim->getPosition() );
							}
							else if( source )
							{
								magnetVector.set( source->getDrawable()->getPosition() );
								magnetVector.sub( curVictim->getPosition() );
							}
						}
						else if( !m_magnetUseCenter && source )
						{
							magnetVector.set( curVictim->getPosition() );
							magnetVector.sub( source->getDrawable()->getPosition() );
						}
						
						if(m_magnetRespectsCenter)
						{
							if(damageInfo.in.m_magnetAmount > magnetVector.length())
								damageInfo.in.m_magnetAmount = magnetVector.length();
						}

						// Guard against zero vector. Make vector stright up if that is the case
						if ( fabs(magnetVector.x) < WWMATH_EPSILON &&
								fabs(magnetVector.y) < WWMATH_EPSILON &&
								fabs(magnetVector.z) < WWMATH_EPSILON)
						{
							magnetVector.z = 1.0f;
						}

						damageInfo.in.m_magnetVector = magnetVector;
						damageInfo.in.m_magnetLiftHeight = m_magnetLiftHeight;
						damageInfo.in.m_magnetLiftHeightSecond = m_magnetLiftHeightSecond;
						damageInfo.in.m_magnetLiftForce = m_magnetLiftForce;
						damageInfo.in.m_magnetLiftForceToHeight = m_magnetLiftForceToHeight;
						damageInfo.in.m_magnetLiftForceToHeightSecond = m_magnetLiftForceToHeightSecond;
						damageInfo.in.m_magnetMaxLiftHeight = m_magnetMaxLiftHeight;
						damageInfo.in.m_magnetAirboneAffectedByYaw = m_magnetAirboneAffectedByYaw;
						damageInfo.in.m_magnetFormula = m_magnetFormula;
						damageInfo.in.m_magnetLevitationHeight = m_magnetLevitationHeight;
						damageInfo.in.m_magnetAirborneZForce = m_magnetAirborneZForce;
					}
				}
				else
				{
					damageInfo.in.m_magnetAmount = 0.0f;
					damageInfo.in.m_magnetLevitationHeight = m_magnetLevitationHeight;
				}
			}

      if (source && source->getControllingPlayer()) {
				damageInfo.in.m_sourcePlayerMask = source->getControllingPlayer()->getPlayerMask();
			}
			// note, don't bother with damage multipliers here...
			// that's handled internally by the attemptDamage() method.
			if(checkForRailgunOnly)
				damageInfo.in.m_amount = getRailgunUsesSecondaryDamage() ? secondaryDamage : primaryDamage;
			else
				damageInfo.in.m_amount = (curVictimDistSqr <= primaryRadiusSqr) ? primaryDamage : secondaryDamage;

			if( killSelf )
			{
				//Deal enough damage to kill yourself. I thought about getting the current health and applying
				//enough unresistable damage to die... however it's possible that we have different types of
				//deaths based on damage type and/or the possibility to resist certain damage types and
				//surviving -- so instead, I'm blindly inflicting a very high value of the intended damage type.
				damageInfo.in.m_amount = HUGE_DAMAGE_AMOUNT;
				//BodyModuleInterface* body = curVictim->getBodyModule();
				//if( body )
				//{
				//	Real curVictimHealth = curVictim->getBodyModule()->getHealth();
				//	damageInfo.in.m_amount = __max( damageInfo.in.m_amount, curVictimHealth );
				//}
			}

			// if the damage-dealer is a projectile, designate the damage as done by its launcher, not the projectile.
			// this is much more useful for the AI...
			if (source && source->isKindOf(KINDOF_PROJECTILE))
			{
				for (BehaviorModule** u = source->getBehaviorModules(); *u; ++u)
				{
					ProjectileUpdateInterface* pui = (*u)->getProjectileUpdateInterface();
					if (pui != NULL)
					{
						damageInfo.in.m_sourceID = pui->projectileGetLauncherID();
						break;
					}
				}
			}

			// Check whether we do Shrapnel
			if( !doShrapnel &&
				m_shrapnelBonusWeapon &&
				m_shrapnelBonusCount > 0 &&
				curVictimDistSqr < CLOSE_ENOUGH &&
				testValidForAttack(curVictim, getAntiMask(), primaryVictim ? primaryVictim->getID() : INVALID_ID, source->getRelationship(curVictim))
			  )
			{
				doShrapnel = TRUE;
				if(shrapnelVictimID == INVALID_ID)
					shrapnelVictimID = curVictim->getID();
			}

			curVictim->attemptDamage(&damageInfo);
			//DEBUG_ASSERTLOG(damageInfo.out.m_noEffect, ("WeaponTemplate::dealDamageInternal: dealt to %s %08lx: attempted %f, actual %f (%f)",
			//	curVictim->getTemplate()->getName().str(),curVictim,
			//	damageInfo.in.m_amount, damageInfo.out.m_actualDamageDealt, damageInfo.out.m_actualDamageClipped));
		}

		if(doShrapnel || m_shrapnelDoesNotRequireVictim)
			privateDoShrapnel(sourceID, shrapnelVictimID, pos);
	}
	else
	{
		DEBUG_CRASH(("projectile weapons should never get dealDamage called directly"));
	}
}

//-------------------------------------------------------------------------------------------------
void WeaponTemplate::privateDoShrapnel(ObjectID sourceID, ObjectID victimID, const Coord3D *pos) const
{
	// Sanity, don't do Shrapnel
	if(!m_shrapnelBonusWeapon || m_shrapnelBonusCount <= 0)
		return;

	// Don't do shrapnel from self or historic weapons
	if( m_shrapnelBonusWeapon == this || (m_historicBonusWeapon && m_historicBonusWeapon == this) )
		return;

	Object *source = TheGameLogic->findObjectByID(sourceID);	// might be null...

	// Don't do anything if source doesn't exist
	if (source == NULL)
		return;

	// if there's a specific victim, use it's pos (overriding the value passed in)
	//Object *primaryVictim = victimID ? TheGameLogic->findObjectByID(victimID) : NULL;	// might be null...

	// Shrapnel Bonus Weapon
	WeaponBonus bonus;
	std::vector<AsciiString> dummy;
	m_shrapnelBonusWeapon->private_computeBonus(source, 0, bonus, dummy);
	Real range = m_shrapnelBonusWeapon->getAttackRange(bonus);

	Int affects = m_shrapnelAffectsMask;
	std::vector<ObjectID> victimIDVec;

	PartitionFilterSameMapStatus filterMapStatus(source);
	PartitionFilterStealthedAndUndetected filterStealth(source, false);
	PartitionFilterAlive filterAlive;

	// Leaving this here commented out to show that I need to reach valid contents of invalid transports.
	// So these checks are on an individual basis, not in the Partition query
	//	PartitionFilterAcceptByKindOf filterKindof(data->m_requiredAffectKindOf,data->m_forbiddenAffectKindOf);
	PartitionFilter *filters[4];
	Int numFilters = 0;
	filters[numFilters++] = &filterMapStatus;
	filters[numFilters++] = &filterAlive;
	if(!m_shrapnelIgnoresStealthStatus)
		filters[numFilters++] = &filterStealth;
	filters[numFilters] = NULL;

	// scan objects in our region
	ObjectIterator *iter = ThePartitionManager->iterateObjectsInRange( pos, 
																	range, 
																	DAMAGE_RANGE_CALC_TYPE, 
																	filters );
	MemoryPoolObjectHolder hold( iter );
	for( Object *curVictim = iter->first(); curVictim != NULL; curVictim = iter->next() )
	{
		// Don't check for the primary victim as target
		if (victimID != INVALID_ID && curVictim->getID() == victimID)
			continue;

		// Special, only deal damage to self
		if(m_shrapnelBonusWeapon->getDamagesSelfOnly())
		{
			if(source->getID() != curVictim->getID())
				continue;
		}
		
		// anytime something is designated as the "primary victim" (ie, the direct target
		// of the weapon), we ignore all the "affects" flags.
		else
		{
			WeaponAntiMaskType targetAntiMask = (WeaponAntiMaskType)getVictimAntiMask( curVictim );
			if( curVictim->isKindOf( KINDOF_UNATTACKABLE ) ||
				curVictim->testStatus(OBJECT_STATUS_MASKED) ||
				curVictim->testStatus(OBJECT_STATUS_NO_ATTACK_FROM_AI) ||
				(m_shrapnelBonusWeapon->getAntiMask() & targetAntiMask) == 0 || 
				m_shrapnelBonusWeapon->estimateWeaponTemplateDamage(source, curVictim, curVictim->getPosition(), bonus) == 0.0f )
			{
				continue;
			}

			BodyModuleInterface* body = curVictim->getBodyModule();
			if( body && body->cantBeKilled())
				continue;
			
			// should object ever be allowed to damage themselves? methinks not...
			// exception: a few weapons allow this (eg, for suicide bombers).
			if( (affects & WEAPON_AFFECTS_SELF) == 0 )
			{
				// Remember that source is a missile for some units, and they don't want to injure them'selves' either
				if( source == curVictim || source->getProducerID() == curVictim->getID() )
				{
					//DEBUG_LOG(("skipping damage done to SELF..."));
					continue;
				}
			}
			
			//IamInnocent - Fix declaring RadiusDamageAffects = SELF solely not checking for Self and needed to be declared together with ALLIES 3/11/2025
			if( affects == WEAPON_AFFECTS_SELF )
			{
				if( !(source == curVictim || source->getProducerID() == curVictim->getID()) )
					continue;
			}

			if( affects & WEAPON_DOESNT_AFFECT_SIMILAR )
			{
				//This means we probably are affecting allies, but don't want to kill nearby members that are the same type as us.
				//A good example are a group of terrorists blowing themselves up. We don't want to cause a domino effect that kills
				//all of them.
				if( source->getTemplate()->isEquivalentTo(curVictim->getTemplate()) && source->getRelationship( curVictim ) == ALLIES )
				{
					continue;
				}
			}

			if ((affects & WEAPON_DOESNT_AFFECT_AIRBORNE) != 0 && curVictim->isSignificantlyAboveTerrain())
			{
				continue;
			}

			/*
				The idea here is: if its our ally(/enemies), AND it's not the direct target, AND the weapon doesn't
				do radius-damage to allies(/enemies)... skip it.
			*/
			Relationship r = curVictim->getRelationship(source);
			Int requiredMask;
			if (r == ALLIES)
				requiredMask = WEAPON_AFFECTS_ALLIES;
			else if (r == ENEMIES)
				requiredMask = WEAPON_AFFECTS_ENEMIES;
			else /* r == NEUTRAL */
				requiredMask = WEAPON_AFFECTS_NEUTRALS;

			if( affects == WEAPON_AFFECTS_SELF )
			{
				requiredMask = WEAPON_AFFECTS_SELF;
			}

			if( !(affects & requiredMask) )
			{
				//Skip if we aren't affected by this weapon.
				continue;
			}
		}

		victimIDVec.push_back(curVictim->getID());
	}

	Coord3D spawnPos;
	spawnPos.set( getProjectileTemplate() != NULL ? source->getPosition() : pos );

	for(int i = 0; i < m_shrapnelBonusCount; i++)
	{
		if(!victimIDVec.empty())
		{
			Int randomValue = GameLogicRandomValue(0, victimIDVec.size() - 1);
			Object* target = TheGameLogic->findObjectByID(victimIDVec[randomValue]);

			TheWeaponStore->createAndFireTempWeaponOnSpot(m_shrapnelBonusWeapon, source, target, &spawnPos, victimID);

			for( std::vector<ObjectID>::const_iterator it_ID = victimIDVec.begin(); it_ID != victimIDVec.end();)
			{
				if((*it_ID) == victimIDVec[randomValue])
				{
					it_ID = victimIDVec.erase(it_ID);
					break;
				}
				++it_ID;
			}
		}
		else
		{
			Real angle = GameLogicRandomValueReal( 0, 2*PI );
			Coord3D targetPos;
			targetPos.set( pos );
			targetPos.x += range * Cos(angle);
			targetPos.y += range * Sin(angle);
			targetPos.z = TheTerrainLogic->getGroundHeight(targetPos.x, targetPos.y);

			TheWeaponStore->createAndFireTempWeaponOnSpot(m_shrapnelBonusWeapon, source, &targetPos, &spawnPos, victimID);
		}
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
WeaponStore::WeaponStore()
{
}

//-------------------------------------------------------------------------------------------------
WeaponStore::~WeaponStore()
{
	deleteAllDelayedDamage();

	for (size_t i = 0; i < m_weaponTemplateVector.size(); i++)
	{
		WeaponTemplate* wt = m_weaponTemplateVector[i];
		deleteInstance(wt);
	}
	m_weaponTemplateVector.clear();
	for (WeaponTemplateMap::iterator it = m_weaponTemplateHashMap.begin(); it != m_weaponTemplateHashMap.end(); ++it)
	{
		WeaponTemplate* wt = it->second;
		if (wt)
			deleteInstance(wt);
	}
	m_weaponTemplateHashMap.clear();
}

//-------------------------------------------------------------------------------------------------
void WeaponStore::handleProjectileDetonation(const WeaponTemplate* wt, const Object *source, const Coord3D* pos, WeaponBonusConditionFlags extraBonusFlags, const std::vector<AsciiString>& extraBonusCustomFlags, Bool inflictDamage)
{
	Weapon* w = TheWeaponStore->allocateNewWeapon(wt, PRIMARY_WEAPON);
	w->loadAmmoNow(source);
	w->fireProjectileDetonationWeapon( source, pos, extraBonusFlags, extraBonusCustomFlags, inflictDamage);
	deleteInstance(w);
}

//-------------------------------------------------------------------------------------------------
void WeaponStore::createAndFireTempWeapon(const WeaponTemplate* wt, const Object *source, const Coord3D* pos)
{
	if (wt == NULL)
		return;
	Weapon* w = TheWeaponStore->allocateNewWeapon(wt, PRIMARY_WEAPON);
	w->loadAmmoNow(source);
	w->fireWeapon(source, pos);
	deleteInstance(w);
}

//-------------------------------------------------------------------------------------------------
void WeaponStore::createAndFireTempWeapon(const WeaponTemplate* wt, const Object *source, Object *target)
{
	//CRCDEBUG_LOG(("WeaponStore::createAndFireTempWeapon() for %s", DescribeObject(source)));
	if (wt == NULL)
		return;
	Weapon* w = TheWeaponStore->allocateNewWeapon(wt, PRIMARY_WEAPON);
	w->loadAmmoNow(source);
	w->fireWeapon(source, target);
	deleteInstance(w);
}

//-------------------------------------------------------------------------------------------------
void WeaponStore::createAndFireTempWeaponOnSpot(const WeaponTemplate* wt, const Object *source, const Coord3D* pos, const Coord3D* sourcePos, ObjectID shrapnelLaunchID)
{
	if (wt == NULL)
		return;
	Weapon* w = TheWeaponStore->allocateNewWeapon(wt, PRIMARY_WEAPON);
	w->loadAmmoNow(source);
	w->fireWeaponOnSpot(source, pos, NULL, sourcePos, shrapnelLaunchID);
	deleteInstance(w);
}

//-------------------------------------------------------------------------------------------------
void WeaponStore::createAndFireTempWeaponOnSpot(const WeaponTemplate* wt, const Object *source, Object *target, const Coord3D* sourcePos, ObjectID shrapnelLaunchID)
{
	//CRCDEBUG_LOG(("WeaponStore::createAndFireTempWeapon() for %s", DescribeObject(source)));
	if (wt == NULL)
		return;
	Weapon* w = TheWeaponStore->allocateNewWeapon(wt, PRIMARY_WEAPON);
	w->loadAmmoNow(source);
	w->fireWeaponOnSpot(source, target, NULL, sourcePos, shrapnelLaunchID);
	deleteInstance(w);
}

//-------------------------------------------------------------------------------------------------
const WeaponTemplate *WeaponStore::findWeaponTemplate( const AsciiString& name ) const
{
	if (name.compareNoCase("None") == 0)
		return NULL;
	const WeaponTemplate * wt = findWeaponTemplatePrivate( TheNameKeyGenerator->nameToKey( name ) );
	DEBUG_ASSERTCRASH(wt != NULL, ("Weapon %s not found!",name));
	return wt;
}

//-------------------------------------------------------------------------------------------------
const WeaponTemplate *WeaponStore::findWeaponTemplate( const char* name ) const
{
	if (stricmp(name, "None") == 0)
		return NULL;
	const WeaponTemplate * wt = findWeaponTemplatePrivate( TheNameKeyGenerator->nameToKey( name ) );
	DEBUG_ASSERTCRASH(wt != NULL, ("Weapon %s not found!",name));
	return wt;
}

//-------------------------------------------------------------------------------------------------
WeaponTemplate *WeaponStore::findWeaponTemplatePrivate( NameKeyType key ) const
{
	// search weapon list for name
	/*for (size_t i = 0; i < m_weaponTemplateVector.size(); i++)
		if( m_weaponTemplateVector[ i ]->getNameKey() == key )
			return m_weaponTemplateVector[i];*/
	WeaponTemplateMap::const_iterator it = m_weaponTemplateHashMap.find(key);
	if(it != m_weaponTemplateHashMap.end())
		return it->second;

	return NULL;

}

//-------------------------------------------------------------------------------------------------
WeaponTemplate *WeaponStore::newWeaponTemplate(AsciiString name)
{

	// sanity
	if(name.isEmpty())
		return NULL;

	// allocate a new weapon
	WeaponTemplate *wt = newInstance(WeaponTemplate);
	wt->m_name = name;
	wt->m_nameKey = TheNameKeyGenerator->nameToKey( name );
	//m_weaponTemplateVector.push_back(wt);
	m_weaponTemplateHashMap[wt->m_nameKey] = wt;

	return wt;
}

//-------------------------------------------------------------------------------------------------
WeaponTemplate *WeaponStore::newOverride(WeaponTemplate *weaponTemplate)
{
	if (!weaponTemplate)
		return NULL;

	// allocate a new weapon
	WeaponTemplate *wt = newInstance(WeaponTemplate);
	(*wt) = (*weaponTemplate);
	(wt)->friend_setNextTemplate(weaponTemplate);

	return wt;
}

//-------------------------------------------------------------------------------------------------
void WeaponStore::update()
{
	for (std::list<WeaponDelayedDamageInfo>::iterator ddi = m_weaponDDI.begin(); ddi != m_weaponDDI.end(); )
	{
		UnsignedInt curFrame = TheGameLogic->getFrame();
		if (curFrame >= ddi->m_delayDamageFrame)
		{
			// we never do projectile-detonation-damage via this code path.
			const Bool isProjectileDetonation = false;
			ddi->m_delayedWeapon->dealDamageInternal(ddi->m_delaySourceID, ddi->m_delayIntendedVictimID, &ddi->m_delayDamagePos, ddi->m_bonus, isProjectileDetonation);
			ddi = m_weaponDDI.erase(ddi);
		}
		else
		{
			++ddi;
		}
	}
}

//-------------------------------------------------------------------------------------------------
void WeaponStore::deleteAllDelayedDamage()
{
	m_weaponDDI.clear();
}

// ------------------------------------------------------------------------------------------------
void WeaponStore::resetWeaponTemplates( void )
{

	for (size_t i = 0; i < m_weaponTemplateVector.size(); i++)
	{
		WeaponTemplate* wt = m_weaponTemplateVector[i];
		wt->reset();
	}
	for (WeaponTemplateMap::iterator it = m_weaponTemplateHashMap.begin(); it != m_weaponTemplateHashMap.end(); ++it)
	{
		WeaponTemplate* wt = it->second;
		wt->reset();
	}

}

//-------------------------------------------------------------------------------------------------
void WeaponStore::reset()
{
	// clean up any overriddes.
	for (size_t i = 0; i < m_weaponTemplateVector.size(); ++i)
	{
		WeaponTemplate *wt = m_weaponTemplateVector[i];
		if (wt->isOverride())
		{
			WeaponTemplate *override = wt;
			wt = wt->friend_clearNextTemplate();
			deleteInstance(override);
		}
	}
	for (WeaponTemplateMap::iterator it = m_weaponTemplateHashMap.begin(); it != m_weaponTemplateHashMap.end(); ++it)
	{
		WeaponTemplate* wt = it->second;
		if (wt->isOverride())
		{
			WeaponTemplate *override = wt;
			wt = wt->friend_clearNextTemplate();
			deleteInstance(override);
		}
	}

	deleteAllDelayedDamage();
	resetWeaponTemplates();
}

//-------------------------------------------------------------------------------------------------
void WeaponStore::setDelayedDamage(const WeaponTemplate *weapon, const Coord3D* pos, UnsignedInt whichFrame, ObjectID sourceID, ObjectID victimID, const WeaponBonus& bonus)
{
	WeaponDelayedDamageInfo wi;
	wi.m_delayedWeapon = weapon;
	wi.m_delayDamagePos = *pos;
	wi.m_delayDamageFrame = whichFrame;
	wi.m_delaySourceID = sourceID;
	wi.m_delayIntendedVictimID = victimID;
	wi.m_bonus = bonus;
	m_weaponDDI.push_back(wi);
}

//-------------------------------------------------------------------------------------------------
void WeaponStore::postProcessLoad()
{
	if (!TheThingFactory)
	{
		DEBUG_CRASH(("you must call this after TheThingFactory is inited"));
		return;
	}

	for (size_t i = 0; i < m_weaponTemplateVector.size(); i++)
	{
		WeaponTemplate* wt = m_weaponTemplateVector[i];
		if (wt)
			wt->postProcessLoad();
	}
	for (WeaponTemplateMap::iterator it = m_weaponTemplateHashMap.begin(); it != m_weaponTemplateHashMap.end(); ++it)
	{
		WeaponTemplate* wt = it->second;
		if (wt)
			wt->postProcessLoad();
	}

}

//-------------------------------------------------------------------------------------------------
/*static*/ void WeaponStore::parseWeaponTemplateDefinition(INI* ini)
{
	AsciiString name;

	// read the weapon name
	const char* c = ini->getNextToken();
	name.set(c);

	// find existing item if present
	WeaponTemplate *weapon = TheWeaponStore->findWeaponTemplatePrivate( TheNameKeyGenerator->nameToKey( name ) );
	if (weapon)
	{
		if (ini->getLoadType() == INI_LOAD_CREATE_OVERRIDES)
			weapon = TheWeaponStore->newOverride(weapon);
		else
		{
			DEBUG_CRASH(("Weapon '%s' already exists, but OVERRIDE not specified", c));
			return;
		}

	}
	else
	{
		// no item is present, create a new one
		weapon = TheWeaponStore->newWeaponTemplate(name);
	}

	// parse the ini weapon definition
	ini->initFromINI(weapon, weapon->getFieldParse());

	if (weapon->m_projectileName.isNone())
		weapon->m_projectileName.clear();

	//DEBUG_LOG(("Finished Parsing Weapon: '%s'", name.str()));

#if defined(RTS_DEBUG)
	if (!weapon->getFireSound().getEventName().isEmpty() && weapon->getFireSound().getEventName().compareNoCase("NoSound") != 0)
	{
		DEBUG_ASSERTCRASH(TheAudio->isValidAudioEvent(&weapon->getFireSound()), ("Invalid FireSound %s in Weapon '%s'.", weapon->getFireSound().getEventName().str(), weapon->getName().str()));
	}
#endif

}


//-------------------------------------------------------------------------------------------------
/*static*/ void WeaponStore::parseWeaponExtendTemplateDefinition(INI* ini)
{
	AsciiString name;
	AsciiString parent;

	// read the weapon name
	const char* c = ini->getNextToken();
	name.set(c);

	// read the parent name
	const char* c2 = ini->getNextToken();
	parent.set(c2);

	// find parent if present
	WeaponTemplate* parentWeapon = TheWeaponStore->findWeaponTemplatePrivate(TheNameKeyGenerator->nameToKey(parent));
	if (parentWeapon)
	{

		// find existing item if present
		WeaponTemplate* weapon = TheWeaponStore->findWeaponTemplatePrivate(TheNameKeyGenerator->nameToKey(name));
		if (weapon)
		{
			if (ini->getLoadType() == INI_LOAD_CREATE_OVERRIDES)
				weapon = TheWeaponStore->newOverride(weapon);
			else
			{
				DEBUG_CRASH(("Weapon '%s' already exists, but OVERRIDE not specified", c));
				return;
			}

		}
		else
		{
			// no item is present, create a new one
			weapon = TheWeaponStore->newWeaponTemplate(name);
		}

		//copy from parent
		weapon->copy_from(*parentWeapon);

		// parse the ini weapon definition
		ini->initFromINI(weapon, weapon->getFieldParse());

		if (weapon->m_projectileName.isNone())
			weapon->m_projectileName.clear();

#if defined(RTS_DEBUG)
		if (!weapon->getFireSound().getEventName().isEmpty() && weapon->getFireSound().getEventName().compareNoCase("NoSound") != 0)
		{
			DEBUG_ASSERTCRASH(TheAudio->isValidAudioEvent(&weapon->getFireSound()), ("Invalid FireSound %s in Weapon '%s'.", weapon->getFireSound().getEventName().str(), weapon->getName().str()));
		}
#endif

	}
	else
	{
		DEBUG_CRASH(("Weapon '%s' cannot extend parrent '%s' as it not exists", c, c2));
	}

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
Weapon::Weapon(const WeaponTemplate* tmpl, WeaponSlotType wslot)
{
	// Weapons start empty; you must reload before use.
	// (however, there is no delay for reloading the first time.)
	m_template = tmpl;
	m_wslot = wslot;
	m_status = OUT_OF_AMMO;
	m_ammoInClip = 0;
	m_whenWeCanFireAgain = 0;
	m_whenPreAttackFinished = 0;
	m_whenLastReloadStarted = 0;
	m_projectileStreamID = INVALID_ID;
	m_leechWeaponRangeActive = false;
	m_pitchLimited = (m_template->getMinTargetPitch() > -PI || m_template->getMaxTargetPitch() < PI);
	m_maxShotCount = NO_MAX_SHOTS_LIMIT;
	m_curBarrel = 0;
	m_numShotsForCurBarrel = 	m_template->getShotsPerBarrel();
	m_lastFireFrame = 0;
	m_suspendFXFrame = TheGameLogic->getFrame() + m_template->getSuspendFXDelay();
	m_scatterTargetsAngle = 0;
	m_nextPreAttackFXFrame = 0;
	m_continuousLaserID = INVALID_ID;
	m_bonusRefObjID = INVALID_ID;
}

//-------------------------------------------------------------------------------------------------
Weapon::Weapon(const Weapon& that)
{
	// Weapons lose all ammo when copied.
	this->m_template = that.m_template;
	this->m_wslot = that.m_wslot;
	this->m_status = OUT_OF_AMMO;
	this->m_ammoInClip = 0;
	this->m_whenPreAttackFinished = 0;
	this->m_whenLastReloadStarted = 0;
	this->m_whenWeCanFireAgain = 0;
	this->m_projectileStreamID = INVALID_ID;
	this->m_leechWeaponRangeActive = false;
	this->m_pitchLimited = (m_template->getMinTargetPitch() > -PI || m_template->getMaxTargetPitch() < PI);
	this->m_maxShotCount = NO_MAX_SHOTS_LIMIT;
	this->m_curBarrel = 0;
	this->m_numShotsForCurBarrel = m_template->getShotsPerBarrel();
	this->m_lastFireFrame = 0;
	this->m_suspendFXFrame = that.getSuspendFXFrame();
	this->m_nextPreAttackFXFrame = 0;
	this->m_continuousLaserID = INVALID_ID;
	this->m_bonusRefObjID = INVALID_ID;
}

//-------------------------------------------------------------------------------------------------
Weapon& Weapon::operator=(const Weapon& that)
{
	if (this != &that)
	{
		// Weapons lose all ammo when copied.
		this->m_template = that.m_template;
		this->m_wslot = that.m_wslot;
		this->m_status = OUT_OF_AMMO;
		this->m_ammoInClip = 0;
		this->m_whenPreAttackFinished = 0;
		this->m_whenLastReloadStarted = 0;
		this->m_whenWeCanFireAgain = 0;
		this->m_leechWeaponRangeActive = false;
		this->m_pitchLimited = (m_template->getMinTargetPitch() > -PI || m_template->getMaxTargetPitch() < PI);
		this->m_maxShotCount = NO_MAX_SHOTS_LIMIT;
		this->m_curBarrel = 0;
		this->m_lastFireFrame = 0;
		this->m_suspendFXFrame = that.getSuspendFXFrame();
		this->m_numShotsForCurBarrel = m_template->getShotsPerBarrel();
		this->m_projectileStreamID = INVALID_ID;
		this->m_nextPreAttackFXFrame = 0;
		this->m_continuousLaserID = INVALID_ID;
	}
	return *this;
}

//-------------------------------------------------------------------------------------------------
Weapon::~Weapon()
{
}

//-------------------------------------------------------------------------------------------------
// DEBUG
static void debug_printWeaponBonus(WeaponBonus* bonus, AsciiString name) {
	const char* bonusNames[] = {
		"DAMAGE",
		"RADIUS",
		"RANGE",
		"RATE_OF_FIRE",
		"PRE_ATTACK",
		"ARMOR"
	};
	DEBUG_LOG((">>> Weapon bonus for '%s':\n", name.str()));
	for (int i = 0; i < WeaponBonus::FIELD_COUNT; i++) {
		DEBUG_LOG((">>> -- '%s' : %f\n", bonusNames[i], bonus->getField(static_cast<WeaponBonus::Field>(i))));
	}
}

//-------------------------------------------------------------------------------------------------
void Weapon::computeBonus(const Object *source, WeaponBonusConditionFlags extraBonusFlags, WeaponBonus& bonus, const std::vector<AsciiString>& extraBonusCustomFlags) const
{
	// TODO: Do we need this eventually?
	const Object* bonusRefObj = NULL;
	if (m_bonusRefObjID != INVALID_ID) {
		bonusRefObj = TheGameLogic->findObjectByID(m_bonusRefObjID);
	}
	else {
		bonusRefObj = source;
	}

	bonus.clear();
	WeaponBonusConditionFlags flags = bonusRefObj->getWeaponBonusCondition();
	std::vector<AsciiString> customFlags = bonusRefObj->getCustomWeaponBonusCondition();
	//CRCDEBUG_LOG(("Weapon::computeBonus() - flags are %X for %s", flags, DescribeObject(source).str()));
	flags |= extraBonusFlags;

	Int flagSize = customFlags.size();
	for( std::vector<AsciiString>::const_iterator it = extraBonusCustomFlags.begin(); it != extraBonusCustomFlags.end(); ++it )
	{
		if(!checkWithinStringVecSize((*it), customFlags, flagSize))
			customFlags.push_back(*it);
	}
	
	if (bonusRefObj->getContainedBy())
	{
		// We may be able to add in our container's flags
		const ContainModuleInterface *theirContain = bonusRefObj->getContainedBy()->getContain();
		if( theirContain && theirContain->isWeaponBonusPassedToPassengers() )
		{
			flags |= theirContain->getWeaponBonusPassedToPassengers();

			// CustomFlags
			std::vector<AsciiString> unitCustomFlags = theirContain->getCustomWeaponBonusPassedToPassengers();

			if(!unitCustomFlags.empty())
			{
				flagSize = customFlags.size();
				for( std::vector<AsciiString>::const_iterator it2 = unitCustomFlags.begin(); it2 != unitCustomFlags.end(); ++it2 )
				{
					if(!checkWithinStringVecSize((*it2), unitCustomFlags, flagSize))
						customFlags.push_back(*it2);
				}
			}
		}
	}

	if (TheGlobalData->m_weaponBonusSet)
		TheGlobalData->m_weaponBonusSet->appendBonuses(flags, bonus, customFlags);
	const WeaponBonusSet* extra = m_template->getExtraBonus();
	if (extra)
		extra->appendBonuses(flags, bonus, customFlags);
}

//-------------------------------------------------------------------------------------------------
void WeaponTemplate::private_computeBonus(const Object *source, WeaponBonusConditionFlags extraBonusFlags, WeaponBonus& bonus, const std::vector<AsciiString>& extraBonusCustomFlags) const
{
	// Sanity
	if(source == NULL)
		return;

	bonus.clear();
	WeaponBonusConditionFlags flags = source->getWeaponBonusCondition();
	std::vector<AsciiString> customFlags = source->getCustomWeaponBonusCondition();
	//CRCDEBUG_LOG(("Weapon::computeBonus() - flags are %X for %s", flags, DescribeObject(source).str()));
	flags |= extraBonusFlags;

	Int flagSize = customFlags.size();
	for( std::vector<AsciiString>::const_iterator it = extraBonusCustomFlags.begin(); it != extraBonusCustomFlags.end(); ++it )
	{
		if(!checkWithinStringVecSize((*it), customFlags, flagSize))
			customFlags.push_back(*it);
	}

	if (source->getContainedBy())
	{
		// We may be able to add in our container's flags
		const ContainModuleInterface *theirContain = source->getContainedBy()->getContain();
		if( theirContain && theirContain->isWeaponBonusPassedToPassengers() )
		{
			flags |= theirContain->getWeaponBonusPassedToPassengers();

			// CustomFlags
			std::vector<AsciiString> unitCustomFlags = theirContain->getCustomWeaponBonusPassedToPassengers();

			if(!unitCustomFlags.empty())
			{
				flagSize = customFlags.size();
				for( std::vector<AsciiString>::const_iterator it2 = unitCustomFlags.begin(); it2 != unitCustomFlags.end(); ++it2 )
				{
					if(!checkWithinStringVecSize((*it2), unitCustomFlags, flagSize))
						customFlags.push_back(*it2);
				}
			}
		}
	}

	if (TheGlobalData->m_weaponBonusSet)
		TheGlobalData->m_weaponBonusSet->appendBonuses(flags, bonus, customFlags);
	const WeaponBonusSet* extra = getExtraBonus();
	if (extra)
		extra->appendBonuses(flags, bonus, customFlags);
}

//-------------------------------------------------------------------------------------------------
void Weapon::loadAmmoNow(const Object *sourceObj)
{
	WeaponBonus bonus;
	std::vector<AsciiString> dummy;
	computeBonus(sourceObj, 0, bonus, dummy);
	reloadWithBonus(sourceObj, bonus, true);
}

//-------------------------------------------------------------------------------------------------
void Weapon::reloadAmmo(const Object *sourceObj)
{

	WeaponBonus bonus;
	std::vector<AsciiString> dummy;
	computeBonus(sourceObj, 0, bonus, dummy);
	reloadWithBonus(sourceObj, bonus, false);
}

//-------------------------------------------------------------------------------------------------
Int Weapon::getClipReloadTime(const Object *source) const
{
	WeaponBonus bonus;
	std::vector<AsciiString> dummy;
	computeBonus(source, 0, bonus, dummy);
	return m_template->getClipReloadTime(bonus);
}

//-------------------------------------------------------------------------------------------------
void Weapon::setClipPercentFull(Real percent, Bool allowReduction)
{
	if (m_template->getClipSize() == 0)
		return;

	Int ammo = REAL_TO_INT_FLOOR(m_template->getClipSize() * percent);
	if (ammo > m_ammoInClip || (allowReduction && ammo < m_ammoInClip))
	{
		m_ammoInClip = ammo;
		m_status = m_ammoInClip ? OUT_OF_AMMO : READY_TO_FIRE;
		//CRCDEBUG_LOG(("Weapon::setClipPercentFull() just set m_status to %d (ammo in clip is %d)", m_status, m_ammoInClip));
		m_whenLastReloadStarted = TheGameLogic->getFrame();
		m_whenWeCanFireAgain = m_whenLastReloadStarted;
		//CRCDEBUG_LOG(("Just set m_whenWeCanFireAgain to %d in Weapon::setClipPercentFull", m_whenWeCanFireAgain));
		rebuildScatterTargets();
	}
}

//-------------------------------------------------------------------------------------------------
void Weapon::rebuildScatterTargets(Bool recenter/* = false*/)
{
	m_scatterTargetsUnused.clear();
	Int scatterTargetsCount = m_template->getScatterTargetsVector().size();
	if (scatterTargetsCount)
	{
		if (recenter && m_ammoInClip > 0 && m_ammoInClip < m_template->getClipSize() && m_template->getClipSize() > 0) {
			// Recenter case is relevant when we have "sweep" target set up in a line around the target.
			// When we reset, we want to keep the next shots around the target, which would be the
			// indices in the center of the list.
			UnsignedInt startIndex = REAL_TO_INT_FLOOR((m_template->getClipSize() - m_ammoInClip) / 2);
			for (Int targetIndex = startIndex + m_ammoInClip - 1; targetIndex >= startIndex; targetIndex--) {
				m_scatterTargetsUnused.push_back(targetIndex);
			}
			// TODO: Crash
			// Is there any need to fill up the rest of the targets?
		}
		else {
			// When I reload, I need to rebuild the list of ScatterTargets to shoot at.
			for (Int targetIndex = scatterTargetsCount - 1; targetIndex >= 0; targetIndex--)
				m_scatterTargetsUnused.push_back(targetIndex);
		}


		if (m_template->isScatterTargetRandomAngle()) {
			m_scatterTargetsAngle = GameLogicRandomValueReal(0, PI * 2);
		}
	}
}

//-------------------------------------------------------------------------------------------------
void Weapon::reloadWithBonus(const Object *sourceObj, const WeaponBonus& bonus, Bool loadInstantly)
{
	if (m_template->getClipSize() > 0
			&& m_ammoInClip == m_template->getClipSize()
			&& !sourceObj->isReloadTimeShared())
		return;	// don't restart our reload delay.

	m_ammoInClip = m_template->getClipSize();
	if (m_ammoInClip <= 0)
		m_ammoInClip = 0x7fffffff;	// 0 == unlimited (or effectively so)

	m_status = RELOADING_CLIP;
	Real reloadTime = loadInstantly ? 0 : m_template->getClipReloadTime(bonus);

	if(getROFMovingPenalty() != 0.0f)
		reloadTime = m_template->calcROFForMoving(sourceObj, reloadTime);

	m_whenLastReloadStarted = TheGameLogic->getFrame();
	m_whenWeCanFireAgain = m_whenLastReloadStarted + reloadTime;
	//CRCDEBUG_LOG(("Just set m_whenWeCanFireAgain to %d in Weapon::reloadWithBonus 1", m_whenWeCanFireAgain));

			// if we are sharing reload times
			// go through other weapons in weapon set
			// set their m_whenWeCanFireAgain to this guy's delay
			// set their m_status to this guy's status
	if (sourceObj->isReloadTimeShared())
	{
		for (Int wt = 0; wt<WEAPONSLOT_COUNT; wt++)
		{
			Weapon *weapon = sourceObj->getWeaponInWeaponSlot((WeaponSlotType)wt);
			if (weapon)
			{
				weapon->setPossibleNextShotFrame(m_whenWeCanFireAgain);
				weapon->setLastReloadStartedFrame(m_whenLastReloadStarted);  // This might actually be right to use here
				//CRCDEBUG_LOG(("Just set m_whenWeCanFireAgain to %d in Weapon::reloadWithBonus 2", m_whenWeCanFireAgain));
				weapon->setStatus(RELOADING_CLIP);
			}
		}
	}

	rebuildScatterTargets();
}

//-------------------------------------------------------------------------------------------------
void Weapon::onWeaponBonusChange(const Object *source)
{
	// We are concerned with our reload times being off if our ROF just changed.

	WeaponBonus bonus;
	std::vector<AsciiString> dummy;
	computeBonus(source, 0, bonus, dummy); // The middle arg is for projectiles to inherit damage bonus from launcher

	Int newDelay;
	Bool needUpdate = FALSE;

	if( getStatus() == RELOADING_CLIP )
	{
		newDelay = m_template->getClipReloadTime(bonus);
		needUpdate = TRUE;
	}
	else if( getStatus() == BETWEEN_FIRING_SHOTS )
	{
		newDelay = m_template->getDelayBetweenShots(bonus);
		needUpdate = TRUE;
	}

	if(getROFMovingPenalty() != 0.0f)
	{
		newDelay = m_template->calcROFForMoving(source, newDelay);
	}

	if( needUpdate )
	{
		m_whenLastReloadStarted = TheGameLogic->getFrame();
		m_whenWeCanFireAgain = m_whenLastReloadStarted + newDelay;

		if (source->isReloadTimeShared())
		{
			for (Int wt = 0; wt<WEAPONSLOT_COUNT; wt++)
			{
				Weapon *weapon = source->getWeaponInWeaponSlot((WeaponSlotType)wt);
				if (weapon)
				{
					weapon->setPossibleNextShotFrame(m_whenWeCanFireAgain);
					weapon->setStatus(RELOADING_CLIP);
				}
			}
		}
	}
}

//-------------------------------------------------------------------------------------------------
Bool Weapon::computeApproachTarget(const Object *source, const Object *target, const Coord3D *pos, Real angleOffset, Coord3D& approachTargetPos) const
{
	// compute unit direction vector from us to our victim
	const Coord3D *targetPos;
	Coord3D dir;
	if (target)
	{
		targetPos = target->getPosition();
		ThePartitionManager->getVectorTo( target, source, ATTACK_RANGE_CALC_TYPE, dir );
	}
	else if (pos)
	{
		targetPos = pos;
		ThePartitionManager->getVectorTo( source, pos, ATTACK_RANGE_CALC_TYPE, dir );
		// Flip the vector to get from source to pos.
		dir.x = -dir.x;
		dir.y = -dir.y;
		dir.z = -dir.z;
	}
	else
	{
		DEBUG_CRASH(("error"));
		approachTargetPos.zero();
		return false;
	}

	Real dist = dir.length();
	Real minAttackRange = m_template->getMinimumAttackRange();
	if (minAttackRange > PATHFIND_CELL_SIZE_F && dist < minAttackRange)
	{
		// We aret too close, so move away from the target.
		DEBUG_ASSERTCRASH((minAttackRange<0.9f*getAttackRange(source)), ("Min attack range is too near attack range."));
		// Recompute dir, cause if the bounding spheres touch, it will be 0.
		Coord3D srcPos = *source->getPosition();
		dir.x = srcPos.x-targetPos->x;
		dir.y = srcPos.y-targetPos->y;
#ifdef ATTACK_RANGE_IS_2D
		dir.z = 0.0f;
#else
		dir.z = srcPos.z-targetPos->z;
#endif
		dir.normalize();

		// if we're airborne and too close, just head for the opposite side.
		if (source->isAboveTerrain())
		{
			// Don't do a 180 degree turn.
			Real angle = atan2(-dir.y, -dir.x);
			Real relAngle = source->getOrientation()- angle;
			if (relAngle>2*PI) relAngle -= 2*PI;
			if (relAngle<-2*PI) relAngle += 2*PI;
			if (fabs(relAngle)<PI/2) {
				dir.x = -dir.x;
				dir.y = -dir.y;
				dir.z = -dir.z;
			}
		}

		if (angleOffset != 0.0f)
		{
			Real angle = atan2(dir.y, dir.x);
			dir.x = (Real)Cos(angle + angleOffset);
			dir.y = (Real)Sin(angle + angleOffset);
		}

		// select a spot along the line between us, halfway between the min & max range.
		Real attackRange = (getAttackRange(source) + minAttackRange)/2.0f;
#ifdef ATTACK_RANGE_IS_2D
		if (target)
			attackRange += target->getGeometryInfo().getBoundingCircleRadius();
		attackRange += source->getGeometryInfo().getBoundingCircleRadius();
#else
		if (target)
			attackRange += target->getGeometryInfo().getBoundingSphereRadius();
		attackRange += source->getGeometryInfo().getBoundingSphereRadius();
#endif
		approachTargetPos.x = attackRange * dir.x + targetPos->x;
		approachTargetPos.y = attackRange * dir.y + targetPos->y;
		approachTargetPos.z = attackRange * dir.z + targetPos->z;
		///@todo - make sure we can get to the approach position.
		clipToTerrainExtent(approachTargetPos);
		return false;
	}

	const Real FUDGE = 0.001f;
	if (dist < FUDGE)
	{
		// we're close enough!
		approachTargetPos = *source->getPosition();
		return true;
	}
	else
	{
		if (isContactWeapon())
		{
			// Weapon is basically a contact weapon, like a car bomb.  The approach target logic
			// has been modified to let it approach the object, so just return the target position.	jba.
			approachTargetPos = *targetPos;
			return false;
		}

		dir.x /= dist;
		dir.y /= dist;
		dir.z /= dist;

		if (angleOffset != 0.0f)
		{
			Real angle = atan2(dir.y, dir.x);
			dir.x = (Real)Cos(angle + angleOffset);
			dir.y = (Real)Sin(angle + angleOffset);
		}

		// select a spot along the line between us, in range of our weapon
		const Real ATTACK_RANGE_APPROACH_FUDGE = 0.9f;
		Real attackRange = getAttackRange(source) * ATTACK_RANGE_APPROACH_FUDGE;
		approachTargetPos.x = attackRange * dir.x + targetPos->x;
		approachTargetPos.y = attackRange * dir.y + targetPos->y;
		approachTargetPos.z = attackRange * dir.z + targetPos->z;

		if (source->getAI() && source->getAI()->isAircraftThatAdjustsDestination()) {
			// Adjust the target so that we are not stacked atop another aircraft.
			TheAI->pathfinder()->adjustTargetDestination(source, target, pos, this, &approachTargetPos);
		}

		return false;
	}
}

//-------------------------------------------------------------------------------------------------
Bool WeaponTemplate::passRequirements(const Object *source) const
{
	std::vector<AsciiString> activationUpgrades = getActivationUpgradeNames();
	std::vector<AsciiString> conflictingUpgrades = getConflictingUpgradeNames();
	Bool RequiresAllTriggers = getRequiresAllTriggers();
	ObjectStatusMaskType statusRequired = getRequiredStatus();
	ObjectStatusMaskType statusForbidden = getForbiddenStatus();
	std::vector<AsciiString> customStatusRequired = getCustomStatusRequired();
	std::vector<AsciiString> customStatusForbidden = getCustomStatusForbidden();

	if(!activationUpgrades.empty())
	{
		Bool gotUpgrade = FALSE;
		std::vector<AsciiString>::const_iterator it_a;
		for( it_a = activationUpgrades.begin(); it_a != activationUpgrades.end(); it_a++)
		{
			gotUpgrade = FALSE;
			const UpgradeTemplate* ut = TheUpgradeCenter->findUpgrade( *it_a );
			if( !ut )
			{
				DEBUG_CRASH(("An upgrade module references '%s', which is not an Upgrade", it_a->str()));
				throw INI_INVALID_DATA;
			}
			if ( ut->getUpgradeType() == UPGRADE_TYPE_PLAYER )
			{
				if(source->getControllingPlayer()->hasUpgradeComplete(ut))
				{
					gotUpgrade = TRUE;
					if(!RequiresAllTriggers)
						break;
				}
			}
			else if( source->hasUpgrade(ut) )
			{
				gotUpgrade = TRUE;
				if(!RequiresAllTriggers)
					break;
			}
		}
		if(!gotUpgrade)
			return FALSE;
	}

	if(!conflictingUpgrades.empty())
	{
		std::vector<AsciiString>::const_iterator it_c;
		for( it_c = conflictingUpgrades.begin(); it_c != conflictingUpgrades.end(); it_c++)
		{
			const UpgradeTemplate* ut = TheUpgradeCenter->findUpgrade( *it_c );
			if( !ut )
			{
				DEBUG_CRASH(("An upgrade module references '%s', which is not an Upgrade", it_c->str()));
				throw INI_INVALID_DATA;
			}
			if ( ut->getUpgradeType() == UPGRADE_TYPE_PLAYER )
			{
				if(source->getControllingPlayer()->hasUpgradeComplete(ut))
					return FALSE;
			}
			else if( source->hasUpgrade(ut) )
			{
				return FALSE;
			}
		}
	}

	//We need all required status or else we fail
	// If we have any requirements
	if(statusRequired.any() && !source->getStatusBits().testForAll(statusRequired))
		return FALSE;

	//If we have any forbidden statii, then fail
	if(source->getStatusBits().testForAny(statusForbidden))
		return FALSE;

	if(!source->testCustomStatusForAll(customStatusRequired))
		return FALSE;

	for(std::vector<AsciiString>::const_iterator it = customStatusForbidden.begin(); it != customStatusForbidden.end(); ++it)
	{
		if(source->testCustomStatus(*it))
			return FALSE;
	}

	return TRUE;
}

//-------------------------------------------------------------------------------------------------
Int WeaponTemplate::calcROFForMoving(const Object *source, Int Delay) const
{
	if(source == NULL || source->getPhysics() == NULL || !source->getPhysics()->isMotive())
		return Delay;

	Real value = getROFMovingPenalty();

	if(getROFMovingScales() && source->getAI())
	{
		Real speed = source->getAI()->getCurLocomotorSpeed();

		Real curSpeed = min(source->getLastActualSpeed(), speed);
		Real maxSpeed = getROFMovingMaxSpeedCount() > 0 ? getROFMovingMaxSpeedCount() : speed;

		if(curSpeed < maxSpeed)
		{
			Real scalingRatio = min(1.0f, Real(curSpeed / maxSpeed));
			value *= scalingRatio;
		}
	}

	Int newDelay = Delay * (1.0f + value);
	return newDelay;
}

//-------------------------------------------------------------------------------------------------
//Special case attack range calculate that fakes moving the object (to a garrisoned point) without
//actually moving the object. This is used to help determine if a garrisoned unit not yet
//positioned can attack someone.
//-------------------------------------------------------------------------------------------------
Bool Weapon::isSourceObjectWithGoalPositionWithinAttackRange( const Object *source, const Coord3D *goalPos, const Object *target, const Coord3D *targetPos ) const
{

	Real distSqr;
	if( target )
		distSqr = ThePartitionManager->getGoalDistanceSquared( source, goalPos, target, ATTACK_RANGE_CALC_TYPE );
	else if( targetPos )
		distSqr = ThePartitionManager->getGoalDistanceSquared( source, goalPos, targetPos, ATTACK_RANGE_CALC_TYPE );
	else
		return false;

	Real attackRangeSqr = sqr( getAttackRange( source ) );
	Real minAttackRangeSqr = sqr(m_template->getMinimumAttackRange());
#ifdef RATIONALIZE_ATTACK_RANGE
	if (distSqr < minAttackRangeSqr)
#else
	if (distSqr < minAttackRangeSqr-0.5f)
#endif
	{
		return false;
	}
	return (distSqr <= attackRangeSqr);
}

//-------------------------------------------------------------------------------------------------
Bool Weapon::isWithinAttackRange(const Object *source, const Coord3D* pos) const
{
	Real distSqr = ThePartitionManager->getDistanceSquared( source, pos, ATTACK_RANGE_CALC_TYPE );
	Real attackRangeSqr = sqr(getAttackRange(source));
	Real minAttackRangeSqr = sqr(m_template->getMinimumAttackRange());
#ifdef RATIONALIZE_ATTACK_RANGE
	if (distSqr < minAttackRangeSqr)
#else
	if (distSqr < minAttackRangeSqr-0.5f)
#endif
	{
		return false;
	}
	return (distSqr <= attackRangeSqr);
}

//-------------------------------------------------------------------------------------------------
Bool Weapon::isWithinAttackRange(const Object *source, const Object *target) const
{
	Real distSqr;
	//Real scalar = computeBuffedBonus(source,target,(Int)WeaponBonus::RANGE);
	Real attackRangeSqr = sqr(getAttackRange(source));

	if( !target->isKindOf(KINDOF_BRIDGE) )
	{
		distSqr = ThePartitionManager->getDistanceSquared( source, target, ATTACK_RANGE_CALC_TYPE );
	}
	else
	{
		// Special case - bridges have two attackable points at either end.
		TBridgeAttackInfo info;
		TheTerrainLogic->getBridgeAttackPoints(target, &info);
		distSqr = ThePartitionManager->getDistanceSquared( source, &info.attackPoint1, ATTACK_RANGE_CALC_TYPE );
		if (distSqr>attackRangeSqr)
		{
			// Try the other one.
			distSqr = ThePartitionManager->getDistanceSquared( source, &info.attackPoint2, ATTACK_RANGE_CALC_TYPE );
		}
	}

	Real minAttackRangeSqr = sqr(m_template->getMinimumAttackRange());
#ifdef RATIONALIZE_ATTACK_RANGE
	if (distSqr < minAttackRangeSqr)
#else
	if (distSqr < minAttackRangeSqr-0.5f)
#endif
	{
		// too close. can't attack.
		return false;
	}

	if( distSqr <= attackRangeSqr )
	{
		// Note - only compare contact weapons with structures.  If you do the collision check
		// against vehicles, the attacker may get close enough to the vehicle to get crushed
		// before it fires its weapon.  jba.
		if( isContactWeapon() && target->isKindOf(KINDOF_STRUCTURE))
		{
			//We're close enough to fire off ranged weapons -- but in the case of contact weapons
			//we want to do a more detailed check to see if we're actually colliding with the target.
			ObjectIterator *iter = ThePartitionManager->iteratePotentialCollisions( source->getPosition(), source->getGeometryInfo(), 0.0f );
			MemoryPoolObjectHolder hold( iter );
			for( Object *them = iter->first(); them; them = iter->next() )
			{
				if( target == them )
				{
					return true;
				}
			}
			return false;
		}
		return true;
	}
	return false;
}

//-------------------------------------------------------------------------------------------------
Bool Weapon::isTooClose(const Object *source, const Object *target) const
{
	Real minAttackRange = m_template->getMinimumAttackRange();
	if (minAttackRange == 0.0f)
		return false;

	Real distSqr = ThePartitionManager->getDistanceSquared( source, target, ATTACK_RANGE_CALC_TYPE );
	if (distSqr < sqr(minAttackRange))
	{
		return true;
	}
	return false;
}

//-------------------------------------------------------------------------------------------------
Bool Weapon::isTooClose( const Object *source, const Coord3D *pos ) const
{
	Real minAttackRange = m_template->getMinimumAttackRange();
	if (minAttackRange == 0.0f)
		return false;

	Real distSqr = ThePartitionManager->getDistanceSquared( source, pos, ATTACK_RANGE_CALC_TYPE );
	if (distSqr < sqr(minAttackRange))
	{
		return true;
	}
	return false;
}

//-------------------------------------------------------------------------------------------------
Int Weapon::getWeaponPriority(const Object *source, const Object *target) const
{
	Real minAttackRange = m_template->getMinimumAttackRange();
	Real AttackRange = getAttackRange(source);

	Real distSqr = ThePartitionManager->getDistanceSquared( source, target, ATTACK_RANGE_CALC_TYPE );
	if (distSqr < sqr(minAttackRange))
	{
		return m_template->getOutsideAttackRangePriority();
	}
	else if (distSqr <= sqr(AttackRange))
	{
		return m_template->getAttackRangePriority();
	}
	return m_template->getOutsideAttackRangePriority();
}

//-------------------------------------------------------------------------------------------------
Int Weapon::getWeaponPriority( const Object *source, const Coord3D *pos ) const
{
	Real minAttackRange = m_template->getMinimumAttackRange();
	Real AttackRange = getAttackRange(source);

	Real distSqr = ThePartitionManager->getDistanceSquared( source, pos, ATTACK_RANGE_CALC_TYPE );
	if (distSqr < sqr(minAttackRange))
	{
		return m_template->getOutsideAttackRangePriority();
	}
	else if (distSqr <= sqr(AttackRange))
	{
		return m_template->getAttackRangePriority();
	}
	return m_template->getOutsideAttackRangePriority();
}


//-------------------------------------------------------------------------------------------------
Bool Weapon::isGoalPosWithinAttackRange(const Object *source, const Coord3D* goalPos, const Object *target, const Coord3D* targetPos)	const
{
	Real distSqr;
	// Note - undersize by 1/4 of a pathfind cell, so that the goal is not teetering on the edge
	// of firing range.  jba.
	// Note 2 - even with RATIONALIZE_ATTACK_RANGE, we still need to subtract 1/4 of a pathfind cell,
	// otherwise if it teters on the edge, attacks can fail.  jba.
	Real attackRangeSqr = sqr(getAttackRange(source)-(PATHFIND_CELL_SIZE_F*0.25f));

	if (target != NULL)
	{
		if (target->isKindOf(KINDOF_BRIDGE))
		{
			// Special case - bridges have two attackable points at either end.
			TBridgeAttackInfo info;
			TheTerrainLogic->getBridgeAttackPoints(target, &info);
			distSqr = ThePartitionManager->getGoalDistanceSquared( source, goalPos, &info.attackPoint1, ATTACK_RANGE_CALC_TYPE );
			if (distSqr>attackRangeSqr)
			{
				// Try the other one.
				distSqr = ThePartitionManager->getGoalDistanceSquared( source, goalPos, &info.attackPoint2, ATTACK_RANGE_CALC_TYPE );
			}
		}
		else
		{
			distSqr = ThePartitionManager->getGoalDistanceSquared( source, goalPos, target, ATTACK_RANGE_CALC_TYPE );
		}
	}
	else
	{
		distSqr = ThePartitionManager->getGoalDistanceSquared( source, goalPos, targetPos, ATTACK_RANGE_CALC_TYPE );
	}

	// Note - oversize by 1/4 of a pathfind cell, so that the goal is not teetering on the edge
	// of firing range.  jba.
	// Note 2 - even with RATIONALIZE_ATTACK_RANGE, we still need to add 1/4 of a pathfind cell,
	// otherwise if it teters on the edge, attacks can fail.  jba.
	Real minAttackRangeSqr = sqr(m_template->getMinimumAttackRange()+(PATHFIND_CELL_SIZE_F*0.25f));
#ifdef RATIONALIZE_ATTACK_RANGE
	if (distSqr < minAttackRangeSqr)
#else
	if (distSqr < minAttackRangeSqr-0.5f)
#endif
	{
		return false;
	}
	return (distSqr <= attackRangeSqr);
}

//-------------------------------------------------------------------------------------------------
Real Weapon::getPercentReadyToFire() const
{
	switch (getStatus())
	{
		case OUT_OF_AMMO:
		case PRE_ATTACK:
			return 0.0f;

		case READY_TO_FIRE:
			return 1.0f;

		case BETWEEN_FIRING_SHOTS:
		case RELOADING_CLIP:
		{
			UnsignedInt now = TheGameLogic->getFrame();
			UnsignedInt nextShot = getPossibleNextShotFrame();
			DEBUG_ASSERTCRASH(now >= m_whenLastReloadStarted, ("now >= m_whenLastReloadStarted"));
			if (now >= nextShot)
				return 1.0f;

			DEBUG_ASSERTCRASH(nextShot >= m_whenLastReloadStarted, ("nextShot >= m_whenLastReloadStarted"));
			UnsignedInt totalTime = nextShot - m_whenLastReloadStarted;
			if (totalTime == 0)
			{
				return 1.0f;
			}

			UnsignedInt timeLeft = nextShot - now;
			DEBUG_ASSERTCRASH(timeLeft <= totalTime, ("timeLeft <= totalTime"));
			UnsignedInt timeSoFar = totalTime - timeLeft;
			if (timeSoFar >= totalTime)
			{
				return 1.0f;
			}
			else
			{
				return (Real)timeSoFar / (Real)totalTime;
			}
		}
	}
	DEBUG_CRASH(("should not get here"));
	return 0.0f;
}

//-------------------------------------------------------------------------------------------------
Real Weapon::getAttackRange(const Object *source) const
{
	WeaponBonus bonus;
	std::vector<AsciiString> dummy;
	computeBonus(source, 0, bonus, dummy);
	return m_template->getAttackRange(bonus);

	//Contained objects have longer ranges.
	//const Object *container = source->getContainedBy();
	//if( container )
	//{
	//	attackRange += container->getGeometryInfo().getBoundingCircleRadius();
	//}
	//return attackRange;
}

//-------------------------------------------------------------------------------------------------
Real Weapon::getAttackDistance(const Object *source, const Object *victimObj, const Coord3D* victimPos) const
{
	Real range = getAttackRange(source);

	if (victimObj != NULL)
	{
	#ifdef ATTACK_RANGE_IS_2D
		range += source->getGeometryInfo().getBoundingCircleRadius();
		range += victimObj->getGeometryInfo().getBoundingCircleRadius();
	#else
		range += source->getGeometryInfo().getBoundingSphereRadius();
		range += victimObj->getGeometryInfo().getBoundingSphereRadius();
	#endif
	}

	return range;
}

//-------------------------------------------------------------------------------------------------
Real Weapon::estimateWeaponDamage(const Object *sourceObj, const Object *victimObj, const Coord3D* victimPos)
{
	if (!m_template)
		return 0.0f;
	
	if (sourceObj->testCustomStatus("ZERO_DAMAGE"))
		return 0.0f;

	// if the weapon is just reloading, it's ok. if it's out of ammo
	// (and won't autoreload), then we aren't gonna do any damage.
	if (getStatus() == OUT_OF_AMMO && !m_template->getAutoReloadsClip())
		return 0.0f;

	WeaponBonus bonus;
	std::vector<AsciiString> dummy;
	computeBonus(sourceObj, 0, bonus, dummy);

	return m_template->estimateWeaponTemplateDamage(sourceObj, victimObj, victimPos, bonus);
}

//-------------------------------------------------------------------------------------------------
void Weapon::newProjectileFired(const Object *sourceObj, const Object *projectile, const Object *victimObj, const Coord3D *victimPos, const Coord3D *launchPos )
{
	// If I have a stream, I need to tell it about this new guy
	if( m_template->getProjectileStreamName().isEmpty() )
		return; // nope, no streak logic to do

	Object* projectileStream = TheGameLogic->findObjectByID(m_projectileStreamID);
	if( projectileStream == NULL )
	{
		m_projectileStreamID = INVALID_ID;	// reset, since it might have been "valid" but deleted out from under us
		const ThingTemplate* pst = TheThingFactory->findTemplate(m_template->getProjectileStreamName());
		projectileStream = TheThingFactory->newObject( pst, sourceObj->getControllingPlayer()->getDefaultTeam() );
		if( projectileStream == NULL )
			return;
		m_projectileStreamID = projectileStream->getID();
	}

	//Check for projectile stream update
	static NameKeyType key_ProjectileStreamUpdate = NAMEKEY("ProjectileStreamUpdate");
	ProjectileStreamUpdate* update = (ProjectileStreamUpdate*)projectileStream->findUpdateModule(key_ProjectileStreamUpdate);
	if( update )
	{
		update->setPosition( launchPos ? launchPos : sourceObj->getPosition() );
		update->addProjectile( sourceObj->getID(), projectile->getID(), victimObj ? victimObj->getID() : INVALID_ID, victimPos );
		return;
	}

}

//-------------------------------------------------------------------------------------------------
ObjectID Weapon::createLaser( const Object *sourceObj, const Object *victimObj, const Coord3D *victimPos, const Coord3D *launchPos )
{
	const ThingTemplate* pst = TheThingFactory->findTemplate(m_template->getLaserName());
	if( !pst )
	{
		DEBUG_CRASH( ("Weapon::createLaser(). %s could not find template for its laser %s.",
			sourceObj->getTemplate()->getName().str(), m_template->getLaserName().str() ) );
		return INVALID_ID;
	}
	Object* laser = TheThingFactory->newObject( pst, sourceObj->getControllingPlayer()->getDefaultTeam() );
	if( laser == NULL )
		return INVALID_ID;

	// Give it a good basis in reality to ensure it can draw when on screen.
	laser->setPosition(launchPos ? launchPos : sourceObj->getPosition());

	//Check for laser update
	Drawable *draw = laser->getDrawable();
	if( draw )
	{
		static NameKeyType key_LaserUpdate = NAMEKEY( "LaserUpdate" );
		LaserUpdate *update = (LaserUpdate*)draw->findClientUpdateModule( key_LaserUpdate );
		if( update )
		{
			Coord3D pos = *victimPos;
			if( victimObj) {
				if (!victimObj->isKindOf(KINDOF_PROJECTILE) && !victimObj->isAirborneTarget()) {
					//Targets are positioned on the ground, so raise the beam up so we're not shooting their feet.
					//Projectiles are a different story, target their exact position.
					pos.z += getTemplate()->getLaserGroundUnitTargetHeight();
				}
			}
			else { // We target the ground
				pos.z += getTemplate()->getLaserGroundTargetHeight();
			}
			update->initLaser( launchPos ? NULL : sourceObj, victimObj, launchPos ? launchPos : sourceObj->getPosition(), &pos, m_template->getLaserBoneName() );
		}
	}

	return laser->getID();
}
//-------------------------------------------------------------------------------------------------
void Weapon::handleContinuousLaser(const Object* sourceObj, const Object* victimObj, const Coord3D* victimPos, const Coord3D *launchPos )
{
	UnsignedInt frameNow = TheGameLogic->getFrame();
	Object* continuousLaser = TheGameLogic->findObjectByID(m_continuousLaserID);

	if (m_lastFireFrame + m_template->getContinuousLaserLoopTime() < frameNow ||
		continuousLaser == NULL) {
		// We are outside the loop time, or the laser doesn't exist -> create a new laser
		ObjectID laserId = createLaser(sourceObj, victimObj, victimPos, launchPos);
		continuousLaser = TheGameLogic->findObjectByID(laserId);
		if (continuousLaser == NULL) {
			return;
		}
		m_continuousLaserID = laserId;
		return;
	}

	// We have an existing laser
	continuousLaser->setPosition(launchPos ? launchPos : sourceObj->getPosition());

	//Check for laser update
	Drawable* draw = continuousLaser->getDrawable();
	if (draw)
	{
		// Try to update lifetime of the laser
		static NameKeyType key_LifetimeUpdate = NAMEKEY("LifetimeUpdate");
		LifetimeUpdate* lt_update = (LifetimeUpdate*)continuousLaser->findUpdateModule(key_LifetimeUpdate);
		if (lt_update) {
			lt_update->resetLifetime();
		}

		static NameKeyType key_LaserUpdate = NAMEKEY("LaserUpdate");
		LaserUpdate* update = (LaserUpdate*)draw->findClientUpdateModule(key_LaserUpdate);
		if (update)
		{
			Coord3D pos = *victimPos;
			if (victimObj && !victimObj->isKindOf(KINDOF_PROJECTILE) && !victimObj->isAirborneTarget())
			{
				//Targets are positioned on the ground, so raise the beam up so we're not shooting their feet.
				//Projectiles are a different story, target their exact position.
				pos.z += 10.0f;
			}
			update->updateContinuousLaser(launchPos ? NULL : sourceObj, victimObj, launchPos ? launchPos : sourceObj->getPosition(), &pos);
		}
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
// return true if we auto-reloaded our clip after firing.
//DECLARE_PERF_TIMER(fireWeapon)
Bool Weapon::privateFireWeapon(
	const Object *sourceObj,
	Object *victimObj,
	const Coord3D* victimPos,
	Bool isProjectileDetonation,
	Bool ignoreRanges,
	WeaponBonusConditionFlags extraBonusFlags,
	ObjectID* projectileID,
	Bool inflictDamage,
	const std::vector<AsciiString>& extraBonusCustomFlags,
	const Coord3D* sourcePos,
	ObjectID shrapnelLaunchID
)
{
	//CRCDEBUG_LOG(("Weapon::privateFireWeapon() for %s", DescribeObject(sourceObj).str()));
	//USE_PERF_TIMER(fireWeapon)
	if (projectileID)
		*projectileID = INVALID_ID;

	if (!m_template)
		return false;

	// If we are a networked weapon, tell everyone nearby they might want to get in on this shot
	if( m_template->getRequestAssistRange()  &&  victimObj )
		processRequestAssistance( sourceObj, victimObj );

	//For weapon templates that have the leech range weapon flag set, it essentially grants
	//the weapon unlimited range for the remainder of the attack. While it's triggered here
	//it's the AIAttackState machine that actually uses and resets this value.
	//This makes the ASSUMPTION that it is IMPOSSIBLE TO FIRE A WEAPON WITHOUT BEING IN AN AIATTACKSTATE
	//
	// @todo srj -- this isn't a universally true assertion! eg, FireWeaponDie lets you do this easily.
	//
	if( m_template->isLeechRangeWeapon() )
	{
		setLeechRangeActive( TRUE );
	}

	bool usedDisarm = FALSE;

	//Special case damge type overrides requiring special handling.
	switch( m_template->getDamageType() )
	{
		case DAMAGE_DEPLOY:
		{
			const AIUpdateInterface *ai = sourceObj->getAI();
			if( ai )
			{
				const AssaultTransportAIInterface *atInterface = ai->getAssaultTransportAIInterface();
				if( atInterface )
				{
					atInterface->beginAssault( victimObj );
				}
			}
			break;
		}

		case DAMAGE_DISARM:
		{
			usedDisarm = true;

			if (sourceObj && victimObj)
			{
				Bool found = false;
				for (BehaviorModule** bmi = victimObj->getBehaviorModules(); *bmi; ++bmi)
				{
					LandMineInterface* lmi = (*bmi)->getLandMineInterface();
					if (lmi)
					{
						VeterancyLevel v = sourceObj->getVeterancyLevel();
						FXList::doFXPos(m_template->getFireFX(v), victimObj->getPosition(), victimObj->getTransformMatrix(), 0, victimObj->getPosition(), 0);
						lmi->disarm();
						found = true;
						break;
					}
				}

				// it's a mine, but doesn't have LandMineInterface...
				if( !found && victimObj->isKindOf( KINDOF_MINE ) || victimObj->isKindOf( KINDOF_BOOBY_TRAP ) || victimObj->isKindOf( KINDOF_DEMOTRAP ) )
				{
					VeterancyLevel v = sourceObj->getVeterancyLevel();
					FXList::doFXPos(m_template->getFireFX(v), victimObj->getPosition(), victimObj->getTransformMatrix(), 0, victimObj->getPosition(), 0);
					TheGameLogic->destroyObject( victimObj );// douse this thing before somebody gets hurt!
					found = true;
				}

				if( found )
				{
					sourceObj->getControllingPlayer()->getAcademyStats()->recordMineCleared();
				}
			}

			--m_maxShotCount;
			--m_ammoInClip;	// so we can use the delay between shots on the mine clearing weapon
			if (m_ammoInClip <= 0 && m_template->getAutoReloadsClip())
			{
				reloadAmmo(sourceObj);
				return TRUE;	// reloaded
			}
			else
			{
				return FALSE;	// did not reload
			}
		}

		case DAMAGE_HACK:
		{
			//We're using a hacker unit to hack a target. Hacking has various effects and
			//instead of inflicting damage, we are waiting for a period of time until the hack takes effect.
			//return FALSE;
		}
	}

	if(!usedDisarm && m_template->getIsDisarm() == TRUE && sourceObj && victimObj)
	{
		Bool found = false;
		for (BehaviorModule** bmi = victimObj->getBehaviorModules(); *bmi; ++bmi)
		{
			LandMineInterface* lmi = (*bmi)->getLandMineInterface();
			if (lmi)
			{
				VeterancyLevel v = sourceObj->getVeterancyLevel();
				FXList::doFXPos(m_template->getFireFX(v), victimObj->getPosition(), victimObj->getTransformMatrix(), 0, victimObj->getPosition(), 0);
				lmi->disarm();
				found = true;
				break;
			}
		}

		// it's a mine, but doesn't have LandMineInterface...
		if( !found && victimObj->isKindOf( KINDOF_MINE ) || victimObj->isKindOf( KINDOF_BOOBY_TRAP ) || victimObj->isKindOf( KINDOF_DEMOTRAP ) )
		{
			VeterancyLevel v = sourceObj->getVeterancyLevel();
			FXList::doFXPos(m_template->getFireFX(v), victimObj->getPosition(), victimObj->getTransformMatrix(), 0, victimObj->getPosition(), 0);
			TheGameLogic->destroyObject( victimObj );// douse this thing before somebody gets hurt!
			found = true;
		}

		if( found )
		{
			sourceObj->getControllingPlayer()->getAcademyStats()->recordMineCleared();
		}
	}

	WeaponBonus bonus;
	computeBonus(sourceObj, extraBonusFlags, bonus, extraBonusCustomFlags);

	// debug_printWeaponBonus(&bonus, m_template->getName());

	DEBUG_ASSERTCRASH(getStatus() != OUT_OF_AMMO, ("Hmm, firing weapon that is OUT_OF_AMMO"));
	DEBUG_ASSERTCRASH(getStatus() == READY_TO_FIRE, ("Hmm, Weapon is firing more often than should be possible"));
	DEBUG_ASSERTCRASH(m_ammoInClip > 0, ("Hmm, firing an empty weapon"));

	if (getStatus() != READY_TO_FIRE)
		return false;

	UnsignedInt now = TheGameLogic->getFrame();
	Bool reloaded = false;
	if (m_ammoInClip > 0)
	{
		// TheSuperHackers @logic-client-separation helmutbuhler 11/04/2025
		// barrelCount shouln't depend on Drawable, which belongs to client.
		Int barrelCount = sourceObj->getDrawable()->getBarrelCount(m_wslot);
		// IamInnocent - Added barrel count to change based on the disguised Object
		StealthUpdate *stealth = sourceObj->getStealth();
		if(stealth && stealth->isDisguisedAndCheckIfNeedOffset())
		{
			barrelCount = stealth->getBarrelCountWhileDisguised(m_wslot);
		}
		if (m_curBarrel >= barrelCount)
		{
			m_curBarrel = 0;
			m_numShotsForCurBarrel = m_template->getShotsPerBarrel();
		}

		if( !m_scatterTargetsUnused.empty() && !isProjectileDetonation)
		{
			// If we haven't fired for this long: reset the scatter targets.
			if (m_template->getScatterTargetResetTime() > 0) {
				UnsignedInt frameNow = TheGameLogic->getFrame();
				if (m_lastFireFrame + m_template->getScatterTargetResetTime() < frameNow) {
					rebuildScatterTargets(m_template->isScatterTargetResetRecenter());
				}
			}


			// If I have a set scatter pattern, I need to offset the target by a random pick from that pattern
			if( victimObj )
			{
				victimPos = victimObj->getPosition();
				victimObj = NULL;
			}
			Coord3D  targetPos = *victimPos; // need to copy, as this pointer is actually inside somebody potentially

			Int targetIndex = 0;
			if (m_template->isScatterTargetRandom()) {
				Int randomPick = GameLogicRandomValue(0, m_scatterTargetsUnused.size() - 1);
				targetIndex = m_scatterTargetsUnused[randomPick];
				// To erase from a vector, put the last on the one you used and pop the back.
				m_scatterTargetsUnused[randomPick] = m_scatterTargetsUnused.back();
				m_scatterTargetsUnused.pop_back();
			}
			else {
				//We actually pick from the back of the the order of targets
				targetIndex = m_scatterTargetsUnused[m_scatterTargetsUnused.size() - 1];
				m_scatterTargetsUnused.pop_back();
			}

			Real scatterTargetScalar = getScatterTargetScalar();// essentially a radius, but operates only on this scatterTarget table
			Coord2D scatterOffset = m_template->getScatterTargetsVector().at( targetIndex );

			// Scale scatter target based on range
			Real minScale = m_template->getScatterTargetMinScalar();
			if (minScale > 0.0) {
				Real minRange = m_template->getMinimumAttackRange();
				Real maxRange = m_template->getUnmodifiedAttackRange();
				Real range = sqrt(ThePartitionManager->getDistanceSquared(sourceObj, victimPos, FROM_CENTER_2D));
				Real rangeRatio = (range - minRange) / (maxRange - minRange);
				scatterTargetScalar = (rangeRatio * (scatterTargetScalar - minScale)) + minScale;
				// DEBUG_LOG((">>> Weapon: Range = %f, RangeRatio = %f, TargetScalar = %f\n", range, rangeRatio, scatterTargetScalar));
			}

			scatterOffset.x *= scatterTargetScalar;
			scatterOffset.y *= scatterTargetScalar;

			// New: align scatter pattern to shooter, and/or use a random angle
			if (m_template->isScatterTargetAligned() || m_scatterTargetsAngle != 0.0f) {
				// DEBUG_LOG((">>> Weapon: m_scatterTargetsAngle = %f\n", m_scatterTargetsAngle));

				const Coord3D srcPos = sourcePos ? *sourcePos : *sourceObj->getPosition();

				Real angle = m_scatterTargetsAngle;
				if (m_template->isScatterTargetAligned()) {
					angle += atan2(targetPos.y - srcPos.y, targetPos.x - srcPos.x);
					// angle += atan2(srcPos.y - targetPos.y, srcPos.x - targetPos.x);
				}

				Real cosA = Cos(angle);
				Real sinA = Sin(angle);
				Real scatterOffsetRotX = scatterOffset.x * cosA - scatterOffset.y * sinA;
				Real scatterOffsetRotY = scatterOffset.x * sinA + scatterOffset.y * cosA;
				scatterOffset.x = scatterOffsetRotX;
				scatterOffset.y = scatterOffsetRotY;
			}

			if (m_template->isScatterTargetCenteredAtShooter()) {
				targetPos = sourcePos ? *sourcePos : *sourceObj->getPosition();
			}

			targetPos.x += scatterOffset.x;
			targetPos.y += scatterOffset.y;

			if (m_template->isScatterOnWaterSurface()) {
				Real waterZ;
				Real terrainZ;
				TheTerrainLogic->isUnderwater(targetPos.x, targetPos.y, &waterZ, &terrainZ);
				targetPos.z = std::max(waterZ, terrainZ);
			}
			else {
				targetPos.z = TheTerrainLogic->getGroundHeight(targetPos.x, targetPos.y);
			}

			// Note AW: We have to ignore Ranges when using ScatterTargets, or else the weapon can fail in the next stage
			ignoreRanges = TRUE;
			m_template->fireWeaponTemplate(sourceObj, m_wslot, m_curBarrel, victimObj, &targetPos, bonus, isProjectileDetonation, ignoreRanges, this, projectileID, inflictDamage, sourcePos, shrapnelLaunchID );
		}
		else
		{
			m_template->fireWeaponTemplate(sourceObj, m_wslot, m_curBarrel, victimObj, victimPos, bonus, isProjectileDetonation, ignoreRanges, this, projectileID, inflictDamage, sourcePos, shrapnelLaunchID );
		}

		m_lastFireFrame = now;
		--m_ammoInClip;
		--m_maxShotCount;
		--m_numShotsForCurBarrel;
		if (m_numShotsForCurBarrel <= 0)
		{
			++m_curBarrel;
			m_numShotsForCurBarrel = m_template->getShotsPerBarrel();
		}

		if (m_ammoInClip <= 0)
		{
			if (m_template->getAutoReloadsClip())
			{
				reloadAmmo(sourceObj);
				reloaded = true;
			}
			else
			{
				m_status = OUT_OF_AMMO;
				m_whenWeCanFireAgain = 0x7fffffff;
				//CRCDEBUG_LOG(("Just set m_whenWeCanFireAgain to %d in Weapon::privateFireWeapon 1", m_whenWeCanFireAgain));
			}
		}
		else
		{
			m_status = BETWEEN_FIRING_SHOTS;
			//CRCDEBUG_LOG(("Weapon::privateFireWeapon() just set m_status to BETWEEN_FIRING_SHOTS"));
			Int delay = m_template->getDelayBetweenShots(bonus);

			if(getROFMovingPenalty() != 0.0f)
				delay = m_template->calcROFForMoving(sourceObj, delay);

			m_whenLastReloadStarted = now;
			m_whenWeCanFireAgain = now + delay;
			//CRCDEBUG_LOG(("Just set m_whenWeCanFireAgain to %d (delay is %d) in Weapon::privateFireWeapon", m_whenWeCanFireAgain, delay));

			// if we are sharing reload times
			// go through other weapons in weapon set
			// set their m_whenWeCanFireAgain to this guy's delay
			// set their m_status to this guy's status

			if ( sourceObj->isReloadTimeShared() )
			{
				for (Int wt = 0; wt<WEAPONSLOT_COUNT; wt++)
				{
					Weapon *weapon = sourceObj->getWeaponInWeaponSlot((WeaponSlotType)wt);
					if (weapon)
					{
						weapon->setPossibleNextShotFrame(m_whenWeCanFireAgain);
						//CRCDEBUG_LOG(("Just set m_whenWeCanFireAgain to %d in Weapon::privateFireWeapon 3", m_whenWeCanFireAgain));
						weapon->setStatus(BETWEEN_FIRING_SHOTS);
					}
				}
			}

		}
	}

	return reloaded;
}


//-------------------------------------------------------------------------------------------------
void Weapon::preFireWeapon( const Object *source, const Object *victim )
{
	Int delay = getPreAttackDelay(source, victim);
	if (delay > 0)
	{
		Bool allowFX = TheGameLogic->getFrame() > getNextPreAttackFXFrame();

		setStatus(PRE_ATTACK);
		setPreAttackFinishedFrame(TheGameLogic->getFrame() + delay);
		if (m_template->isLeechRangeWeapon())
		{
			setLeechRangeActive(TRUE);
		}

		if (allowFX) {

			// Fix currentBarrel
			Int curBarrel = m_curBarrel;
			Int barrelCount = source->getDrawable()->getBarrelCount(m_wslot);
			// IamInnocent - Added barrel count to change based on the disguised Object
			StealthUpdate *stealth = source->getStealth();
			if(stealth && stealth->isDisguisedAndCheckIfNeedOffset())
			{
				barrelCount = stealth->getBarrelCountWhileDisguised(m_wslot);
			}
		
			if (curBarrel >= barrelCount)
			{
				curBarrel = 0;
			}

			getTemplate()->createPreAttackFX(source, m_wslot, curBarrel, victim, NULL);
			// Add delay to avoid spamming the FX
			if (m_template->getPreAttackFXDelay() > 0) {
				setNextPreAttackFXFrame(TheGameLogic->getFrame() + m_template->getPreAttackFXDelay());
			}
		}
	}
}

//-------------------------------------------------------------------------------------------------
// Same as above but with target location instead of object
void Weapon::preFireWeapon(const Object* source, const Coord3D* pos)
{
	Int delay = getPreAttackDelay(source, NULL);
	if (delay > 0)
	{
		Bool allowFX = TheGameLogic->getFrame() > getNextPreAttackFXFrame();

		setStatus(PRE_ATTACK);
		setPreAttackFinishedFrame(TheGameLogic->getFrame() + delay);
		if (m_template->isLeechRangeWeapon())
		{
			setLeechRangeActive(TRUE);
		}

		if (allowFX) {
			// Fix currentBarrel
			Int curBarrel = m_curBarrel;
			Int barrelCount = source->getDrawable()->getBarrelCount(m_wslot);
			// IamInnocent - Added barrel count to change based on the disguised Object
			StealthUpdate *stealth = source->getStealth();
			if(stealth && stealth->isDisguisedAndCheckIfNeedOffset())
			{
				barrelCount = stealth->getBarrelCountWhileDisguised(m_wslot);
			}

			if (curBarrel >= barrelCount)
			{
				curBarrel = 0;
			}

			getTemplate()->createPreAttackFX(source, m_wslot, curBarrel, NULL, pos);
			// Add delay to avoid spamming the FX
			if (m_template->getPreAttackFXDelay() > 0) {
				setNextPreAttackFXFrame(TheGameLogic->getFrame() + m_template->getPreAttackFXDelay());
			}
		}
	}
}

//-------------------------------------------------------------------------------------------------
Bool Weapon::fireWeapon(const Object *source, Object *target, ObjectID* projectileID)
{
	if (source->testCustomStatus("AIM_NO_ATTACK"))
	{
		if (projectileID)
			*projectileID = INVALID_ID;
		return false;
	}
	std::vector<AsciiString> dummy;
	//CRCDEBUG_LOG(("Weapon::fireWeapon() for %s at %s", DescribeObject(source).str(), DescribeObject(target).str()));
	return privateFireWeapon( source, target, NULL, false, false, 0, projectileID, TRUE, dummy );
}

//-------------------------------------------------------------------------------------------------
// return true if we auto-reloaded our clip after firing.
Bool Weapon::fireWeapon(const Object *source, const Coord3D* pos, ObjectID* projectileID)
{
	if (source->testCustomStatus("AIM_NO_ATTACK"))
	{
		if (projectileID)
			*projectileID = INVALID_ID;
		return false;
	}
	std::vector<AsciiString> dummy;
	//CRCDEBUG_LOG(("Weapon::fireWeapon() for %s", DescribeObject(source).str()));
	return privateFireWeapon( source, NULL, pos, false, false, 0, projectileID, TRUE, dummy );
}

//-------------------------------------------------------------------------------------------------
Bool Weapon::fireWeaponOnSpot(const Object *source, Object *target, ObjectID* projectileID, const Coord3D* sourcePos, ObjectID shrapnelLaunchID)
{
	if (source->testCustomStatus("AIM_NO_ATTACK"))
	{
		if (projectileID)
			*projectileID = INVALID_ID;
		return false;
	}
	std::vector<AsciiString> dummy;
	//CRCDEBUG_LOG(("Weapon::fireWeapon() for %s at %s", DescribeObject(source).str(), DescribeObject(target).str()));
	return privateFireWeapon( source, target, NULL, false, false, 0, projectileID, TRUE, dummy, sourcePos, shrapnelLaunchID );
}

//-------------------------------------------------------------------------------------------------
// return true if we auto-reloaded our clip after firing.
Bool Weapon::fireWeaponOnSpot(const Object *source, const Coord3D* pos, ObjectID* projectileID, const Coord3D* sourcePos, ObjectID shrapnelLaunchID)
{
	if (source->testCustomStatus("AIM_NO_ATTACK"))
	{
		if (projectileID)
			*projectileID = INVALID_ID;
		return false;
	}
	std::vector<AsciiString> dummy;
	//CRCDEBUG_LOG(("Weapon::fireWeapon() for %s", DescribeObject(source).str()));
	return privateFireWeapon( source, NULL, pos, false, false, 0, projectileID, TRUE, dummy, sourcePos, shrapnelLaunchID );
}

//-------------------------------------------------------------------------------------------------
void Weapon::fireProjectileDetonationWeapon(const Object *source, Object *target, WeaponBonusConditionFlags extraBonusFlags, const std::vector<AsciiString>& extraBonusCustomFlags, Bool inflictDamage)
{
	//CRCDEBUG_LOG(("Weapon::fireProjectileDetonationWeapon() for %sat %s", DescribeObject(source).str(), DescribeObject(target).str()));
	privateFireWeapon( source, target, NULL, true, false, extraBonusFlags, NULL, inflictDamage, extraBonusCustomFlags);
}

//-------------------------------------------------------------------------------------------------
void Weapon::fireProjectileDetonationWeapon(const Object *source, const Coord3D* pos, WeaponBonusConditionFlags extraBonusFlags, const std::vector<AsciiString>& extraBonusCustomFlags, Bool inflictDamage)
{
	//CRCDEBUG_LOG(("Weapon::fireProjectileDetonationWeapon() for %s", DescribeObject(source).str()));
	privateFireWeapon( source, NULL, pos, true, false, extraBonusFlags, NULL, inflictDamage, extraBonusCustomFlags);
}

//-------------------------------------------------------------------------------------------------
//Currently, this function was added to allow a script to force fire a weapon,
//and immediately gain control of the weapon that was fired to give it special orders...
Object* Weapon::forceFireWeapon( const Object *source, const Coord3D *pos)
{
	//CRCDEBUG_LOG(("Weapon::forceFireWeapon() for %s", DescribeObject(source).str()));
	//Force the ammo to load instantly.
	//loadAmmoNow( source );
	//Fire the weapon at the position. Internally, it'll store the weapon projectile ID if so created.
	ObjectID projectileID = INVALID_ID;
	const Bool ignoreRange = true;
	std::vector<AsciiString> dummy;
	privateFireWeapon(source, NULL, pos, false, ignoreRange, NULL, &projectileID, TRUE, dummy );
	return TheGameLogic->findObjectByID( projectileID );
}

//-------------------------------------------------------------------------------------------------
WeaponStatus Weapon::getStatus() const
{
	UnsignedInt now = TheGameLogic->getFrame();
	if( now < m_whenPreAttackFinished )
	{
		return PRE_ATTACK;
	}
	if( now >= m_whenWeCanFireAgain )
	{
		if (m_ammoInClip > 0)
			m_status = READY_TO_FIRE;
		else
			m_status = OUT_OF_AMMO;
		//CRCDEBUG_LOG(("Weapon::getStatus() just set m_status to %d (ammo in clip is %d)", m_status, m_ammoInClip));
	}
	return m_status;
}

//-------------------------------------------------------------------------------------------------
Bool Weapon::isWithinTargetPitch(const Object *source, const Object *victim) const
{
	if (isContactWeapon() || !isPitchLimited())
		return true;

	const Coord3D* src = source->getPosition();
	const Coord3D* dst = victim->getPosition();

	const Real ACCCEPTABLE_DZ = 10.0f;
	if (fabs(dst->z - src->z) < ACCCEPTABLE_DZ)
		return true;	// always good enough if dz is small, regardless of pitch

	Real minPitch, maxPitch;
	source->getGeometryInfo().calcPitches(*src, victim->getGeometryInfo(), *dst, minPitch, maxPitch);

	// if there's any intersection between the the two pitch ranges, we're good to go.
	if ((minPitch >= m_template->getMinTargetPitch() && minPitch <= m_template->getMaxTargetPitch()) ||
			(maxPitch >= m_template->getMinTargetPitch() && maxPitch <= m_template->getMaxTargetPitch()) ||
			(minPitch <= m_template->getMinTargetPitch() && maxPitch >= m_template->getMaxTargetPitch()))
		return true;

	//DEBUG_LOG(("pitch %f-%f is out of range",rad2deg(minPitch),rad2deg(maxPitch),rad2deg(m_template->getMinTargetPitch()),rad2deg(m_template->getMaxTargetPitch())));
	return false;
}

//-------------------------------------------------------------------------------------------------
Bool Weapon::isWithinTargetHeight(const Object *victim) const
{	
	Real height;
	
	// Structures don't have altitudes
	if( victim->isKindOf( KINDOF_STRUCTURE ) )
	{
		// But they have geometrical heights
		height = victim->getGeometryInfo().getMaxHeightAbovePosition();
	}
	else
	{
		height = victim->getHeightAboveTerrain();
	}

	// Structures don't have a Max Target Height since all Structures are built on ground
	if (m_template->getMinTargetHeight() <= height && ( victim->isKindOf( KINDOF_STRUCTURE ) || !m_template->getMaxTargetHeight() || height <= m_template->getMaxTargetHeight() )) 
		return true;
	else
		return false;
}

//-------------------------------------------------------------------------------------------------
Real Weapon::getPrimaryDamageRadius(const Object *source) const
{
	WeaponBonus bonus;
	std::vector<AsciiString> dummy;
	computeBonus(source, 0, bonus, dummy);
	return m_template->getPrimaryDamageRadius(bonus);
}

//-------------------------------------------------------------------------------------------------
Bool Weapon::isDamageWeapon() const
{
	//These damage types are special attacks that don't do damage directly, even
	//if they can indirectly. These are here to prevent the UI from allowing the
	//user to mouseover a target and think it can attack it using these types.
	switch( m_template->getDamageType() )
	{
		case DAMAGE_DEPLOY:
			//Kris @todo
			//Evaluate a better way to handle this weapon type... doesn't fit being a damage weapon.
			//May want to check if cargo can attack!
			return TRUE;

		case DAMAGE_DISARM:
			return TRUE;	// hmm, can only "damage" mines, but still...

		case DAMAGE_HACK:
			return FALSE;
	}

	//Use no bonus
	WeaponBonus whoCares;
	if( m_template->getPrimaryDamage( whoCares ) > 0.0f || m_template->getSecondaryDamage( whoCares ) > 0.0f )
	{
		return TRUE;
	}

	return FALSE;
}

//-------------------------------------------------------------------------------------------------
Int Weapon::getPreAttackDelay( const Object *source, const Object *victim ) const
{
	// Look for a reason to return zero and have no delay.
	WeaponPrefireType type = m_template->getPrefireType();
	if( type == PREFIRE_PER_CLIP )
	{
		if( m_template->getClipSize() > 0  &&  m_ammoInClip < m_template->getClipSize() )
			return 0;// I only delay once a clip, and this is not the first shot
	}
	else if( type == PREFIRE_PER_ATTACK )
	{
		if( source->getNumConsecutiveShotsFiredAtTarget( victim ) > 0 )
			return 0;// I only delay once an attack, and I have already shot this guy
	}
	//else it is per shot, so it always applies

	WeaponBonus bonus;
	std::vector<AsciiString> dummy;
	computeBonus(source, 0, bonus, dummy);
	return m_template->getPreAttackDelay( bonus );
}

//-------------------------------------------------------------------------------------------------
Real Weapon::getArmorBonus(const Object *source) const
{
	WeaponBonus bonus;
	std::vector<AsciiString> dummy;
	computeBonus(source, 0, bonus, dummy);
	return m_template->getArmorBonus(bonus);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
class AssistanceRequestData
{
public:
	AssistanceRequestData();

	const Object *m_requestingObject;
	Object *m_victimObject;
	Real m_requestDistanceSquared;
};

//-------------------------------------------------------------------------------------------------
AssistanceRequestData::AssistanceRequestData()
{
	m_requestingObject = NULL;
	m_victimObject = NULL;
	m_requestDistanceSquared = 0.0f;
}

//-------------------------------------------------------------------------------------------------
static void makeAssistanceRequest( Object *requestOf, void *userData )
{
	AssistanceRequestData *requestData = (AssistanceRequestData *)userData;

	// Don't ask ourselves (can't believe I forgot this one)
	if( requestOf == requestData->m_requestingObject )
		return;

	// Only request of our kind of people
	if( !requestOf->getTemplate()->isEquivalentTo( requestData->m_requestingObject->getTemplate() ) )
		return;

	// Who are close enough
	Real distSq = ThePartitionManager->getDistanceSquared( requestOf, requestData->m_requestingObject, FROM_CENTER_2D );
	if( distSq > requestData->m_requestDistanceSquared )
		return;

	// and respond to requests
	static const NameKeyType key_assistUpdate = NAMEKEY("AssistedTargetingUpdate");
	AssistedTargetingUpdate *assistModule = (AssistedTargetingUpdate*)requestOf->findUpdateModule(key_assistUpdate);
	if( assistModule == NULL )
		return;

	// and say yes
	if( !assistModule->isFreeToAssist() )
		return;

	assistModule->assistAttack( requestData->m_requestingObject, requestData->m_victimObject );
}

//-------------------------------------------------------------------------------------------------
void Weapon::processRequestAssistance( const Object *requestingObject, Object *victimObject )
{
	// Iterate through our player's objects, and tell everyone like us within our assistance range
	// who is free to do so to assist us on this shot.
	Player *ourPlayer = requestingObject->getControllingPlayer();
	if( !ourPlayer )
		return;

	AssistanceRequestData requestData;
	requestData.m_requestingObject = requestingObject;
	requestData.m_victimObject = victimObject;
	requestData.m_requestDistanceSquared = m_template->getRequestAssistRange() * m_template->getRequestAssistRange();

	ourPlayer->iterateObjects( makeAssistanceRequest, &requestData );
}

//-------------------------------------------------------------------------------------------------
/*static*/ void Weapon::calcProjectileLaunchPosition(
	const Object* launcher,
	WeaponSlotType wslot,
	Int specificBarrelToUse,
	Matrix3D& worldTransform,
	Coord3D& worldPos
)
{
	StealthUpdate *stealth = launcher->getStealth();
	Bool isSimpleDisguise = false;
	Bool isDisguisedAndCheckIfNeedOffset = false;
	if(stealth)
	{
		isSimpleDisguise = stealth->isSimpleDisguise();
		isDisguisedAndCheckIfNeedOffset = stealth->isDisguisedAndCheckIfNeedOffset();

	}
	if( isSimpleDisguise || launcher->getContainedBy() )
	{
		// If we are in an enclosing container, our launch position is our actual position.  Yes, I am putting
		// a minor case and an oft used function, but the major case is huge and full of math.
		if(isSimpleDisguise || launcher->getContainedBy()->getContain()->isEnclosingContainerFor(launcher))
		{
			worldTransform = *launcher->getTransformMatrix();
			Vector3 tmp = worldTransform.Get_Translation();
			worldPos.x = tmp.X;
			worldPos.y = tmp.Y;
			worldPos.z = tmp.Z;
			return;
		}
	}

	Real turretAngle = 0.0f;
	Real turretPitch = 0.0f;
	const AIUpdateInterface* ai = launcher->getAIUpdateInterface();
	WhichTurretType tur = ai ? ai->getWhichTurretForWeaponSlot(wslot, &turretAngle, &turretPitch) : TURRET_INVALID;
	//CRCDEBUG_LOG(("calcProjectileLaunchPosition(): Turret %d, slot %d, barrel %d for %s", tur, wslot, specificBarrelToUse, DescribeObject(launcher).str()));

	Matrix3D attachTransform(true);
	Coord3D turretRotPos = {0.0f, 0.0f, 0.0f};
	Coord3D turretPitchPos = {0.0f, 0.0f, 0.0f};
	const Drawable* draw = isDisguisedAndCheckIfNeedOffset ? stealth->getDrawableTemplateWhileDisguised() : launcher->getDrawable();
	//CRCDEBUG_LOG(("Do we have a drawable? %d", (draw != NULL)));
	if (!draw || !draw->getProjectileLaunchOffset(wslot, specificBarrelToUse, &attachTransform, tur, &turretRotPos, &turretPitchPos))
	{
		//CRCDEBUG_LOG(("ProjectileLaunchPos %d %d not found!",wslot, specificBarrelToUse));
		DEBUG_CRASH(("ProjectileLaunchPos %d %d not found!",wslot, specificBarrelToUse));
		attachTransform.Make_Identity();
		turretRotPos.zero();
		turretPitchPos.zero();
	}
	if (tur != TURRET_INVALID)
	{
		// The attach transform is the pristine front and center position of the fire point
		// We can't read from the client, so we need to reproduce the actual point that
		// takes turn and pitch into account.
		Matrix3D turnAdjustment(1);
		Matrix3D pitchAdjustment(1);

		// To rotate about a point, move that point to 0,0, rotate, then move it back.
		// Pre rotate will keep the first twist from screwing the angle of the second pitch
		pitchAdjustment.Translate( turretPitchPos.x, turretPitchPos.y, turretPitchPos.z );
		pitchAdjustment.In_Place_Pre_Rotate_Y(-turretPitch);
		pitchAdjustment.Translate( -turretPitchPos.x, -turretPitchPos.y, -turretPitchPos.z );

		turnAdjustment.Translate( turretRotPos.x, turretRotPos.y, turretRotPos.z );
		turnAdjustment.In_Place_Pre_Rotate_Z(turretAngle);
		turnAdjustment.Translate( -turretRotPos.x, -turretRotPos.y, -turretRotPos.z );

#ifdef ALLOW_TEMPORARIES
		attachTransform = turnAdjustment * pitchAdjustment * attachTransform;
#else
		Matrix3D tmp = attachTransform;
		attachTransform.mul(turnAdjustment, pitchAdjustment);
		attachTransform.postMul(tmp);
#endif
	}

//#if defined(RTS_DEBUG)
//  Real muzzleHeight = attachTransform.Get_Z_Translation();
//  DEBUG_ASSERTCRASH( muzzleHeight > 0.001f, ("YOUR TURRET HAS A VERY LOW PROJECTILE LAUNCH POSITION, BUT FOUND A VALID BONE. DID YOU PICK THE WRONG ONE? %s", launcher->getTemplate()->getName().str()));
//#endif

  launcher->convertBonePosToWorldPos(NULL, &attachTransform, NULL, &worldTransform);

	Vector3 tmp = worldTransform.Get_Translation();
	worldPos.x = tmp.X;
	worldPos.y = tmp.Y;
	worldPos.z = tmp.Z;
}

//-------------------------------------------------------------------------------------------------
/*static*/ /*Bool Weapon::calcWeaponFirePosition(
	const Object* obj,
	const Drawable* draw,
	WeaponSlotType wslot,
	Int specificBarrelToUse,
	Matrix3D& worldTransform,
	Coord3D& worldPos
)
{
	Real turretAngle = 0.0f;
	Real turretPitch = 0.0f;
	const AIUpdateInterface* ai = obj->getAIUpdateInterface();
	WhichTurretType tur = ai ? ai->getWhichTurretForWeaponSlot(wslot, &turretAngle, &turretPitch) : TURRET_INVALID;

	Matrix3D attachTransform(true);
	Coord3D turretRotPos = {0.0f, 0.0f, 0.0f};
	Coord3D turretPitchPos = {0.0f, 0.0f, 0.0f};
	//CRCDEBUG_LOG(("Do we have a drawable? %d", (draw != NULL)));
	if (!draw || !draw->getWeaponFireOffset(wslot, specificBarrelToUse, &attachTransform, tur, &turretRotPos, &turretPitchPos))
	{
		//CRCDEBUG_LOG(("WeaponFirePos %d %d not found!",wslot, specificBarrelToUse));
		//DEBUG_CRASH(("WeaponFirePos %d %d not found!",wslot, specificBarrelToUse));
		//attachTransform.Make_Identity();
		//turretRotPos.zero();
		//turretPitchPos.zero();
		return FALSE;
	}
	if (tur != TURRET_INVALID)
	{
		// The attach transform is the pristine front and center position of the fire point
		// We can't read from the client, so we need to reproduce the actual point that
		// takes turn and pitch into account.
		Matrix3D turnAdjustment(1);
		Matrix3D pitchAdjustment(1);

		// To rotate about a point, move that point to 0,0, rotate, then move it back.
		// Pre rotate will keep the first twist from screwing the angle of the second pitch
		pitchAdjustment.Translate( turretPitchPos.x, turretPitchPos.y, turretPitchPos.z );
		pitchAdjustment.In_Place_Pre_Rotate_Y(-turretPitch);
		pitchAdjustment.Translate( -turretPitchPos.x, -turretPitchPos.y, -turretPitchPos.z );

		turnAdjustment.Translate( turretRotPos.x, turretRotPos.y, turretRotPos.z );
		turnAdjustment.In_Place_Pre_Rotate_Z(turretAngle);
		turnAdjustment.Translate( -turretRotPos.x, -turretRotPos.y, -turretRotPos.z );

#ifdef ALLOW_TEMPORARIES
		attachTransform = turnAdjustment * pitchAdjustment * attachTransform;
#else
		Matrix3D tmp = attachTransform;
		attachTransform.mul(turnAdjustment, pitchAdjustment);
		attachTransform.postMul(tmp);
#endif
	}

//#if defined(RTS_DEBUG)
//  Real muzzleHeight = attachTransform.Get_Z_Translation();
//  DEBUG_ASSERTCRASH( muzzleHeight > 0.001f, ("YOUR TURRET HAS A VERY LOW PROJECTILE LAUNCH POSITION, BUT FOUND A VALID BONE. DID YOU PICK THE WRONG ONE? %s", obj->getTemplate()->getName().str()));
//#endif

  //obj->convertBonePosToWorldPos(NULL, &attachTransform, NULL, &worldTransform);

  worldTransform = attachTransform;
	Vector3 tmp = worldTransform.Get_Translation();
	worldPos.x = tmp.X;
	worldPos.y = tmp.Y;
	worldPos.z = tmp.Z;

	return TRUE;
}
*/

// ------------------------------------------------------------------------------------------------
static const ModelConditionFlags s_allWeaponFireFlags[WEAPONSLOT_COUNT] =
{
	MAKE_MODELCONDITION_MASK5(
		MODELCONDITION_FIRING_A,
		MODELCONDITION_BETWEEN_FIRING_SHOTS_A,
		MODELCONDITION_RELOADING_A,
		MODELCONDITION_PREATTACK_A,
		MODELCONDITION_USING_WEAPON_A
	),
	MAKE_MODELCONDITION_MASK5(
		MODELCONDITION_FIRING_B,
		MODELCONDITION_BETWEEN_FIRING_SHOTS_B,
		MODELCONDITION_RELOADING_B,
		MODELCONDITION_PREATTACK_B,
		MODELCONDITION_USING_WEAPON_B
	),
	MAKE_MODELCONDITION_MASK5(
		MODELCONDITION_FIRING_C,
		MODELCONDITION_BETWEEN_FIRING_SHOTS_C,
		MODELCONDITION_RELOADING_C,
		MODELCONDITION_PREATTACK_C,
		MODELCONDITION_USING_WEAPON_C
	),
	MAKE_MODELCONDITION_MASK5(
		MODELCONDITION_FIRING_D,
		MODELCONDITION_BETWEEN_FIRING_SHOTS_D,
		MODELCONDITION_RELOADING_D,
		MODELCONDITION_PREATTACK_D,
		MODELCONDITION_USING_WEAPON_D
	),
	MAKE_MODELCONDITION_MASK5(
		MODELCONDITION_FIRING_E,
		MODELCONDITION_BETWEEN_FIRING_SHOTS_E,
		MODELCONDITION_RELOADING_E,
		MODELCONDITION_PREATTACK_E,
		MODELCONDITION_USING_WEAPON_E
	),
	MAKE_MODELCONDITION_MASK5(
		MODELCONDITION_FIRING_F,
		MODELCONDITION_BETWEEN_FIRING_SHOTS_F,
		MODELCONDITION_RELOADING_F,
		MODELCONDITION_PREATTACK_F,
		MODELCONDITION_USING_WEAPON_F
	),
	MAKE_MODELCONDITION_MASK5(
		MODELCONDITION_FIRING_G,
		MODELCONDITION_BETWEEN_FIRING_SHOTS_G,
		MODELCONDITION_RELOADING_G,
		MODELCONDITION_PREATTACK_G,
		MODELCONDITION_USING_WEAPON_G
	),
	MAKE_MODELCONDITION_MASK5(
		MODELCONDITION_FIRING_H,
		MODELCONDITION_BETWEEN_FIRING_SHOTS_H,
		MODELCONDITION_RELOADING_H,
		MODELCONDITION_PREATTACK_H,
		MODELCONDITION_USING_WEAPON_H
	)
};

//-------------------------------------------------------------------------------------------------
/*static*/ void Weapon::setFirePositionForDrawable(
	const Object* launcher,
	Drawable* draw,
	WeaponSlotType wslot,
	Int specificBarrelToUse
)
{
	draw->setModelConditionState(MODELCONDITION_ATTACKING);
	ModelConditionFlags c = launcher->getModelConditionForWeaponSlot(wslot, WSF_FIRING);
	draw->clearAndSetModelConditionFlags(s_allWeaponFireFlags[wslot], c);

	Real turretAngle = 0.0f;
	Real turretPitch = 0.0f;
	const AIUpdateInterface* ai = launcher->getAIUpdateInterface();
	WhichTurretType tur = ai ? ai->getWhichTurretForWeaponSlot(wslot, &turretAngle, &turretPitch) : TURRET_INVALID;
	//CRCDEBUG_LOG(("calcProjectileLaunchPosition(): Turret %d, slot %d, barrel %d for %s", tur, wslot, specificBarrelToUse, DescribeObject(launcher).str()));

	if (tur != TURRET_INVALID)
	{
		draw->doTurretPositioning(tur, turretAngle, turretPitch);
	}

	draw->setPosition( launcher->getPosition() );
	draw->setOrientation( launcher->getOrientation() );
	draw->updateDrawable();
}

//-------------------------------------------------------------------------------------------------
/*static*/ void Weapon::positionProjectileForLaunch(
	Object* projectile,
	const Object* launcher,
	WeaponSlotType wslot,
	Int specificBarrelToUse,
	const Coord3D* launchPos
)
{
	//CRCDEBUG_LOG(("Weapon::positionProjectileForLaunch() for %s from %s",
		//DescribeObject(projectile).str(), DescribeObject(launcher).str()));

	// if our launch vehicle is gone, destroy ourselves
	if (launcher == NULL)
	{
		TheGameLogic->destroyObject( projectile );
		return;
	}

	Matrix3D worldTransform(true);
	Coord3D worldPos;

	if(!launchPos)
		Weapon::calcProjectileLaunchPosition(launcher, wslot, specificBarrelToUse, worldTransform, worldPos);

	projectile->getDrawable()->setDrawableHidden(false);
	if(!launchPos)
	{
		projectile->setTransformMatrix(&worldTransform);
		projectile->setPosition(&worldPos);
	}
	projectile->getExperienceTracker()->setExperienceSink( launcher->getID() );

	const PhysicsBehavior* launcherPhys = launcher->getPhysics();
	PhysicsBehavior* missilePhys = projectile->getPhysics();
	if (launcherPhys && missilePhys)
	{
		if(!launchPos)
			launcherPhys->transferVelocityTo(missilePhys);
		missilePhys->setIgnoreCollisionsWith(launcher);
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void Weapon::getFiringLineOfSightOrigin(const Object* source, Coord3D& origin) const
{
	//GS 1-6-03
	// Sorry, but we have to simplify this.  If we take the actual projectile launch pos, then
	// that point can change. Take a Ranger with his gun on his shoulder.  His point is very high so
	// he clears this check and transitions to attacking.  This puts his gun at waist level and
	// now he fails this check so he transitions back.  Our height won't change.
	origin.z += source->getGeometryInfo().getMaxHeightAbovePosition();

/*
	if (m_template->getProjectileTemplate() == NULL)
	{
		// note that we want to measure from the top of the collision
		// shape, not the bottom! (most objects have eyes a lot closer
		// to their head than their feet. if we have really odd critters
		// with eye-feet, we'll need to change this assumption.)
		origin.z += source->getGeometryInfo().getMaxHeightAbovePosition();
	}
	else
	{
		Matrix3D tmp(true);
		Coord3D launchPos = {0.0f, 0.0f, 0.0f};
		calcProjectileLaunchPosition(source, m_wslot, m_curBarrel, tmp, launchPos);
		origin.x += launchPos.x - source->getPosition()->x;
		origin.y += launchPos.y - source->getPosition()->y;
		origin.z += launchPos.z - source->getPosition()->z;
	}
*/
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
Bool Weapon::isClearFiringLineOfSightTerrain(const Object* source, const Object* victim) const
{
	if(!source->hasDefaultLineOfSightEnabled() || getWeaponIgnoresObstacles() || getWeaponBypassLineOfSight())
		return true;

	Coord3D origin;
	origin = *source->getPosition();
	//CRCDEBUG_LOG(("Weapon::isClearFiringLineOfSightTerrain(Object) for %s", DescribeObject(source).str()));
	//DUMPCOORD3D(&origin);
	getFiringLineOfSightOrigin(source, origin);
	Coord3D victimPos;
	victim->getGeometryInfo().getCenterPosition( *victim->getPosition(), victimPos );
	//CRCDEBUG_LOG(("Weapon::isClearFiringLineOfSightTerrain() - victimPos is (%g,%g,%g) (%X,%X,%X)",
	//	victimPos.x, victimPos.y, victimPos.z,
	//	AS_INT(victimPos.x),AS_INT(victimPos.y),AS_INT(victimPos.z)));
	return ThePartitionManager->isClearLineOfSightTerrain(NULL, origin, NULL, victimPos);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
Bool Weapon::isClearFiringLineOfSightTerrain(const Object* source, const Coord3D& victimPos) const
{
	if(!source->hasDefaultLineOfSightEnabled() || getWeaponIgnoresObstacles() || getWeaponBypassLineOfSight())
		return true;

	Coord3D origin;
	origin = *source->getPosition();
	//CRCDEBUG_LOG(("Weapon::isClearFiringLineOfSightTerrain(Coord3D) for %s", DescribeObject(source).str()));
	//DUMPCOORD3D(&origin);
	getFiringLineOfSightOrigin(source, origin);
	return ThePartitionManager->isClearLineOfSightTerrain(NULL, origin, NULL, victimPos);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
/** Determine whether if source was at goalPos whether it would have clear line of sight. */
Bool Weapon::isClearGoalFiringLineOfSightTerrain(const Object* source, const Coord3D& goalPos, const Object* victim) const
{
	if(!source->hasDefaultLineOfSightEnabled() || getWeaponIgnoresObstacles() || getWeaponBypassLineOfSight())
		return true;

	Coord3D origin=goalPos;
	//CRCDEBUG_LOG(("Weapon::isClearGoalFiringLineOfSightTerrain(Object) for %s", DescribeObject(source).str()));
	//DUMPCOORD3D(&origin);
	getFiringLineOfSightOrigin(source, origin);
	Coord3D victimPos;
	victim->getGeometryInfo().getCenterPosition( *victim->getPosition(), victimPos );
	return ThePartitionManager->isClearLineOfSightTerrain(NULL, origin, NULL, victimPos);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
/** Determine whether if source was at goalPos whether it would have clear line of sight. */
Bool Weapon::isClearGoalFiringLineOfSightTerrain(const Object* source, const Coord3D& goalPos, const Coord3D& victimPos) const
{
	if(!source->hasDefaultLineOfSightEnabled() || getWeaponIgnoresObstacles() || getWeaponBypassLineOfSight())
		return true;

	Coord3D origin=goalPos;
	//CRCDEBUG_LOG(("Weapon::isClearGoalFiringLineOfSightTerrain(Coord3D) for %s", DescribeObject(source).str()));
	//DUMPCOORD3D(&origin);
	getFiringLineOfSightOrigin(source, origin);
	//CRCDEBUG_LOG(("Weapon::isClearFiringLineOfSightTerrain() - victimPos is (%g,%g,%g) (%X,%X,%X)",
	//	victimPos.x, victimPos.y, victimPos.z,
	//	AS_INT(victimPos.x),AS_INT(victimPos.y),AS_INT(victimPos.z)));
	return ThePartitionManager->isClearLineOfSightTerrain(NULL, origin, NULL, victimPos);
}

//-------------------------------------------------------------------------------------------------
//Kris: Patch 1.01 - November 10, 2003
//This function was added to transfer key weapon stats for Jarmen Kell to and from the bike for
//the sniper attack, so he can share the stats.
//-------------------------------------------------------------------------------------------------
void Weapon::transferNextShotStatsFrom( const Weapon &weapon )
{
	m_whenWeCanFireAgain = weapon.getPossibleNextShotFrame();
	m_whenLastReloadStarted = weapon.getLastReloadStartedFrame();
	m_status = weapon.getStatus();
}

//-------------------------------------------------------------------------------------------------
// Used for WeaponReloadSharedAcrossSets
//-------------------------------------------------------------------------------------------------
void Weapon::transferReloadStateFrom(const Weapon& weapon, Real clipPercentage/*=0.0*/)
{
	// A) Weapon is reloading (clip size > 0)
	// B) Weapon is between firing shots (any clip size)
	// C) Weapon is ready to fire, but clip is not full (

	if (weapon.getClipSize() == 0) {
		m_ammoInClip = 0x7fffffff;	// 0 == unlimited (or effectively so)
	}
	else {
		if (weapon.getStatus() == RELOADING_CLIP) {
			m_ammoInClip = weapon.getClipSize();  //Reloading means we actually are at max clip size
		}
		else {
			Int ammo = REAL_TO_INT_FLOOR(m_template->getClipSize() * clipPercentage);
			m_ammoInClip = ammo;
		}	
		//rebuildScatterTargets();
	}

	m_whenWeCanFireAgain = weapon.getPossibleNextShotFrame();
	m_whenLastReloadStarted = weapon.getLastReloadStartedFrame();
	m_status = weapon.getStatus();


	DEBUG_LOG(("Weapon::transferReloadStateFrom (now = %d): m_whenWeCanFireAgain = %d, m_whenLastReloadStarted = %d, m_status = %d", TheGameLogic->getFrame(), m_whenWeCanFireAgain, m_whenLastReloadStarted, m_status));
}


// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void Weapon::crc( Xfer *xfer )
{
#ifdef DEBUG_CRC
	AsciiString logString;
	AsciiString tmp;
	Bool doLogging = g_logObjectCRCs;
	if (doLogging)
	{
		tmp.format("CRC of weapon %s: ", m_template->getName().str());
		logString.concat(tmp);
	}
#endif // DEBUG_CRC

	AsciiString tmplName = m_template->getName();
	xfer->xferAsciiString(&tmplName);

	// slot
	xfer->xferUser( &m_wslot, sizeof( WeaponSlotType ) );
#ifdef DEBUG_CRC
	if (doLogging)
	{
		tmp.format("m_wslot %d ", m_wslot);
		logString.concat(tmp);
	}
#endif // DEBUG_CRC

	// status
	/*
	xfer->xferUser( &m_status, sizeof( WeaponStatus ) );
#ifdef DEBUG_CRC
	if (doLogging)
	{
		tmp.format("m_status %d ", m_status);
		logString.concat(tmp);
	}
#endif // DEBUG_CRC
	*/

	// ammo
	xfer->xferUnsignedInt( &m_ammoInClip );
#ifdef DEBUG_CRC
	if (doLogging)
	{
		tmp.format("m_ammoInClip %d ", m_ammoInClip);
		logString.concat(tmp);
	}
#endif // DEBUG_CRC

	// when can fire again
	xfer->xferUnsignedInt( &m_whenWeCanFireAgain );
#ifdef DEBUG_CRC
	if (doLogging)
	{
		tmp.format("m_whenWeCanFireAgain %d ", m_whenWeCanFireAgain);
		logString.concat(tmp);
	}
#endif // DEBUG_CRC

	// when pre attack finished
	xfer->xferUnsignedInt( &m_whenPreAttackFinished );
#ifdef DEBUG_CRC
	if (doLogging)
	{
		tmp.format("m_whenPreAttackFinished %d ", m_whenPreAttackFinished);
		logString.concat(tmp);
	}
#endif // DEBUG_CRC

	// m_nextPreAttackFXFrame
	xfer->xferUnsignedInt(&m_nextPreAttackFXFrame);
#ifdef DEBUG_CRC
	if (doLogging)
	{
		tmp.format("m_nextPreAttackFXFrame %d ", m_nextPreAttackFXFrame);
		logString.concat(tmp);
	}
#endif // DEBUG_CRC

	// when last reload started
	xfer->xferUnsignedInt( &m_whenLastReloadStarted );
#ifdef DEBUG_CRC
	if (doLogging)
	{
		tmp.format("m_whenLastReloadStarted %d ", m_whenLastReloadStarted);
		logString.concat(tmp);
	}
#endif // DEBUG_CRC

	// last fire frame
	xfer->xferUnsignedInt( &m_lastFireFrame );
#ifdef DEBUG_CRC
	if (doLogging)
	{
		tmp.format("m_lastFireFrame %d ", m_lastFireFrame);
		logString.concat(tmp);
	}
#endif // DEBUG_CRC

	// projectile stream object
	xfer->xferObjectID( &m_projectileStreamID );
#ifdef DEBUG_CRC
	if (doLogging)
	{
		tmp.format("projectileStreamID %d ", m_projectileStreamID);
		logString.concat(tmp);
	}
#endif // DEBUG_CRC

	// laser object (defunct)
	ObjectID laserIDUnused = INVALID_ID;
	xfer->xferObjectID( &laserIDUnused );
#ifdef DEBUG_CRC
	if (doLogging)
	{
		tmp.format("laserID %d ", laserIDUnused);
		logString.concat(tmp);
	}
#endif // DEBUG_CRC

	// max shot count
	xfer->xferInt( &m_maxShotCount );
#ifdef DEBUG_CRC
	if (doLogging)
	{
		tmp.format("m_maxShotCount %d ", m_maxShotCount);
		logString.concat(tmp);
	}
#endif // DEBUG_CRC

	// current barrel
	xfer->xferInt( &m_curBarrel );
#ifdef DEBUG_CRC
	if (doLogging)
	{
		tmp.format("m_curBarrel %d ", m_curBarrel);
		logString.concat(tmp);
	}
#endif // DEBUG_CRC

	// num shots for current barrel
	xfer->xferInt( &m_numShotsForCurBarrel );
#ifdef DEBUG_CRC
	if (doLogging)
	{
		tmp.format("m_numShotsForCurBarrel %d ", m_numShotsForCurBarrel);
		logString.concat(tmp);
	}
#endif // DEBUG_CRC

	// scatter targets unused
	UnsignedShort scatterCount = m_scatterTargetsUnused.size();
	xfer->xferUnsignedShort( &scatterCount );
#ifdef DEBUG_CRC
	if (doLogging)
	{
		tmp.format("scatterCount %d ", scatterCount);
		logString.concat(tmp);
	}
#endif // DEBUG_CRC

	Int intData;

	std::vector< Int >::const_iterator it;

	for( it = m_scatterTargetsUnused.begin(); it != m_scatterTargetsUnused.end(); ++it )
	{

		intData = *it;
		xfer->xferInt( &intData );
#ifdef DEBUG_CRC
		if (doLogging)
		{
			tmp.format("%d ", intData);
			logString.concat(tmp);
		}
#endif // DEBUG_CRC

	}

	// pitch limited
	xfer->xferBool( &m_pitchLimited );
#ifdef DEBUG_CRC
	if (doLogging)
	{
		tmp.format("m_pitchLimited %d ", m_pitchLimited);
		logString.concat(tmp);
	}
#endif // DEBUG_CRC

	// leech weapon range active
	xfer->xferBool( &m_leechWeaponRangeActive );
#ifdef DEBUG_CRC
	if (doLogging)
	{
		tmp.format("m_leechWeaponRangeActive %d ", m_leechWeaponRangeActive);
		logString.concat(tmp);
	}
#endif // DEBUG_CRC

	// scatter targets random angle
	xfer->xferReal(&m_scatterTargetsAngle);
#ifdef DEBUG_CRC
	if (doLogging)
	{
		tmp.format("m_scatterTargetsAngle %d ", m_scatterTargetsAngle);
		logString.concat(tmp);
	}
#endif // DEBUG_CRC


	// continuous laser object
	xfer->xferObjectID(&m_continuousLaserID);
#ifdef DEBUG_CRC
	if (doLogging)
	{
		CRCDEBUG_LOG(("%s", logString.str()));
		tmp.format("m_continuousLaserID %d ", m_continuousLaserID);
		logString.concat(tmp);
	}
#endif // DEBUG_CRC



#ifdef DEBUG_CRC
	if (doLogging)
	{
		CRCDEBUG_LOG(("%s", logString.str()));
	}
#endif // DEBUG_CRC


}

// ------------------------------------------------------------------------------------------------
/** Xfer
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void Weapon::xfer( Xfer *xfer )
{
	// version
	const XferVersion currentVersion = 3;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	if (version >= 2)
	{
		AsciiString tmplName = m_template->getName();
		xfer->xferAsciiString(&tmplName);
		if (xfer->getXferMode() == XFER_LOAD)
		{
			m_template = TheWeaponStore->findWeaponTemplate(tmplName);
			if (m_template == NULL)
				throw INI_INVALID_DATA;
		}
	}

	// slot
	xfer->xferUser( &m_wslot, sizeof( WeaponSlotType ) );

	// status
	xfer->xferUser( &m_status, sizeof( WeaponStatus ) );

	// ammo
	xfer->xferUnsignedInt( &m_ammoInClip );

	// when can fire again
	xfer->xferUnsignedInt( &m_whenWeCanFireAgain );

	// wehn pre attack finished
	xfer->xferUnsignedInt( &m_whenPreAttackFinished );

	// when next preAttack FX
	xfer->xferUnsignedInt( &m_nextPreAttackFXFrame);

	// when last reload started
	xfer->xferUnsignedInt( &m_whenLastReloadStarted );

	// last fire frame
	xfer->xferUnsignedInt( &m_lastFireFrame );

	// suspendFXFrame, this affects client only
	if ( version >= 3 )
		xfer->xferUnsignedInt( &m_suspendFXFrame );
	else
		m_suspendFXFrame = 0;

	// projectile stream object
	xfer->xferObjectID( &m_projectileStreamID );

	// laser object
	ObjectID laserIDUnused = INVALID_ID;
	xfer->xferObjectID( &laserIDUnused );

	// max shot count
	xfer->xferInt( &m_maxShotCount );

	// current barrel
	xfer->xferInt( &m_curBarrel );

	// num shots for current barrel
	xfer->xferInt( &m_numShotsForCurBarrel );

	// scatter targets unused
	UnsignedShort scatterCount = m_scatterTargetsUnused.size();
	xfer->xferUnsignedShort( &scatterCount );
	Int intData;
	if( xfer->getXferMode() == XFER_SAVE )
	{
		std::vector< Int >::const_iterator it;

		for( it = m_scatterTargetsUnused.begin(); it != m_scatterTargetsUnused.end(); ++it )
		{

			intData = *it;
			xfer->xferInt( &intData );

		}

	}
	else
	{

		// sanity, the scatter targets must be empty
		m_scatterTargetsUnused.clear();

		for( UnsignedShort i = 0; i < scatterCount; ++i )
		{

			xfer->xferInt( &intData );
			m_scatterTargetsUnused.push_back( intData );

		}

	}

	// pitch limited
	xfer->xferBool( &m_pitchLimited );

	// leech weapon range active
	xfer->xferBool( &m_leechWeaponRangeActive );

	// scatter targets random angle
	xfer->xferReal( &m_scatterTargetsAngle );

	// continuous laser object
	xfer->xferObjectID(&m_continuousLaserID);


}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void Weapon::loadPostProcess( void )
{
	if( m_projectileStreamID != INVALID_ID )
	{
		Object* projectileStream = TheGameLogic->findObjectByID( m_projectileStreamID );
		if( projectileStream == NULL )
		{
			m_projectileStreamID = INVALID_ID;
		}
	}

	if (m_continuousLaserID != INVALID_ID)
	{
		Object* continuousLaser = TheGameLogic->findObjectByID(m_continuousLaserID);
		if (continuousLaser == NULL)
		{
			m_continuousLaserID = INVALID_ID;
		}
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
void WeaponBonus::appendBonuses(WeaponBonus& bonus) const
{
	for (int f = 0; f < WeaponBonus::FIELD_COUNT; ++f)
	{
		bonus.m_field[f] += this->m_field[f] - 1.0f;
	}
}

//-------------------------------------------------------------------------------------------------
/*void WeaponBonus::appendBonusesCustom(const AsciiString& customStatus, WeaponBonus& bonus) const
{
	for (int f = 0; f < WeaponBonus::FIELD_COUNT; ++f)
	{
		CustomWeaponBonus field = this->m_field2[f];
		CustomWeaponBonus::const_iterator it = field.find(customStatus);

		if(it != field.end())
		{
			bonus.m_field[f] += it->second - 1.0f;
		}
	}
}*/

//-------------------------------------------------------------------------------------------------
/*static*/ void WeaponBonusSet::parseWeaponBonusSet(INI* ini, void* /*instance*/, void* store, const void* /*userData*/)
{
	WeaponBonusSet* self = (WeaponBonusSet*)store;
	self->parseWeaponBonusSet(ini);
}

//-------------------------------------------------------------------------------------------------
/*static*/ void WeaponBonusSet::parseWeaponBonusSetPtr(INI* ini, void* /*instance*/, void* store, const void* /*userData*/)
{
	WeaponBonusSet** selfPtr = (WeaponBonusSet**)store;
	(*selfPtr)->parseWeaponBonusSet(ini);
}

//-------------------------------------------------------------------------------------------------
void WeaponBonusSet::parseWeaponBonusSet(INI* ini)
{
	WeaponBonusConditionType wb = (WeaponBonusConditionType)INI::scanIndexList(ini->getNextToken(), TheWeaponBonusNames);
	WeaponBonus::Field wf = (WeaponBonus::Field)INI::scanIndexList(ini->getNextToken(), TheWeaponBonusFieldNames);
	m_bonus[wb].setField(wf, INI::scanPercentToReal(ini->getNextToken()));
}

//-------------------------------------------------------------------------------------------------
/*static*/ void WeaponBonusSet::parseCustomWeaponBonusSet(INI* ini, void* /*instance*/, void* store, const void* /*userData*/)
{
	WeaponBonusSet* self = (WeaponBonusSet*)store;
	self->parseCustomWeaponBonusSet(ini);
}

//-------------------------------------------------------------------------------------------------
/*static*/ void WeaponBonusSet::parseCustomWeaponBonusSetPtr(INI* ini, void* /*instance*/, void* store, const void* /*userData*/)
{
	WeaponBonusSet** selfPtr = (WeaponBonusSet**)store;
	(*selfPtr)->parseCustomWeaponBonusSet(ini);
}

//-------------------------------------------------------------------------------------------------
void WeaponBonusSet::parseCustomWeaponBonusSet(INI* ini)
{
	
	//WeaponBonusConditionType wb = (WeaponBonusConditionType)INI::scanIndexList(ini->getNextToken(), TheWeaponBonusNames);
	AsciiString customStatus = ini->getNextQuotedAsciiString();
	WeaponBonus::Field wf = (WeaponBonus::Field)INI::scanIndexList(ini->getNextToken(), TheWeaponBonusFieldNames);
	// So now we have a 3D array. One for checking DamageTypes, and one for checking CustomDamageTypes.
	NameKeyType namekey = TheNameKeyGenerator->nameToKey( customStatus );
	this->m_customBonus[namekey].setField(wf, INI::scanPercentToReal(ini->getNextToken()));
}

//-------------------------------------------------------------------------------------------------
void WeaponBonusSet::appendBonuses(WeaponBonusConditionFlags flags, WeaponBonus& bonus, const std::vector<AsciiString>& customFlags) const
//void WeaponBonusSet::appendBonuses(WeaponBonusConditionFlags flags, WeaponBonus& bonus) const
{
	// TO-DO: Change to Hash_Map. DONE.
	/// Reverted to vector
	for (std::vector<AsciiString>::const_iterator it = customFlags.begin(); it != customFlags.end(); ++it)
	{
		NameKeyType namekey = TheNameKeyGenerator->nameToKey( *it );
		CustomWeaponBonusMap::const_iterator it_bonus = this->m_customBonus.find(namekey);
		if( it_bonus != this->m_customBonus.end() )
			it_bonus->second.appendBonuses(bonus);
	}

	if (flags == 0)
		return;	// my, that was easy

	for (int i = 0; i < WEAPONBONUSCONDITION_COUNT; ++i)
	{
		if ((flags & (1 << i)) == 0)
			continue;

		this->m_bonus[i].appendBonuses(bonus);
	}
}


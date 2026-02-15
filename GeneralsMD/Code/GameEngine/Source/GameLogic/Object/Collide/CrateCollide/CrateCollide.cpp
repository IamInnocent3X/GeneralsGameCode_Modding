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

// FILE: CrateCollide.cpp ///////////////////////////////////////////////////////////////////////
// Author: Graham Smallwood, March 2002
// Desc:   Abstract base Class Crate Collide
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine
#include "Common/BitFlagsIO.h"
#include "Common/Player.h"
#include "Common/Xfer.h"
#include "Common/GameAudio.h"
#include "Common/MiscAudio.h"
#include "GameClient/Anim2D.h"
#include "GameClient/FXList.h"
#include "GameClient/InGameUI.h"
#include "GameClient/Drawable.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/Object.h"
#include "GameLogic/ObjectCreationList.h"
#include "GameLogic/Weapon.h"
#include "GameLogic/Module/CrateCollide.h"


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
CrateCollideModuleData::CrateCollideModuleData()
{
	m_isForbidOwnerPlayer = FALSE;
	m_isAllowNeutralPlayer = FALSE;
	m_executeAnimationDisplayTimeInSeconds = 0.0f;
	m_executeAnimationZRisePerSecond = 0.0f;
	m_executeAnimationFades = TRUE;
	m_isBuildingPickup = FALSE;
	m_isHumanOnlyPickup = FALSE;
	m_isAllowPickAboveTerrain = FALSE;
	m_executeFX = nullptr;
	m_pickupScience = SCIENCE_INVALID;
	m_targetsMask = 0;
	m_destroyOnCollide = FALSE;
	m_fxOnCollide = nullptr;
	m_oclOnCollide = nullptr;
	m_cursorName.clear();
	m_rejectKeys.clear();
	m_activationUpgradeNames.clear();
	m_conflictingUpgradeNames.clear();
	m_requiredCustomStatus.clear();
	m_forbiddenCustomStatus.clear();
	m_requiresAllTriggers = false;

	m_damagePercentageToUnit = 0.0f;
	m_destroyOnTargetDie = FALSE;
	m_destroyOnHeal = FALSE;
	m_removeOnHeal = FALSE;
	m_leechExpFromObject = FALSE;
	m_customStatusToSet.clear();
	m_customStatusToGive.clear();
	m_bonusToGive.clear();
	m_customBonusToGive.clear();
	m_customStatusToRemove.clear();
	m_customStatusToDestroy.clear();
	m_kindofany.clear();
	m_executionAnimationTemplate = AsciiString::TheEmptyString;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void CrateCollideModuleData::buildFieldParse(MultiIniFieldParse& p)
{
  ModuleData::buildFieldParse(p);

	static const FieldParse dataFieldParse[] =
	{
		{ "RequiredKindOf", KindOfMaskType::parseFromINI, nullptr, offsetof( CrateCollideModuleData, m_kindof ) },
		{ "ForbiddenKindOf", KindOfMaskType::parseFromINI, nullptr, offsetof( CrateCollideModuleData, m_kindofnot ) },
		{ "KindOf", KindOfMaskType::parseFromINI, nullptr, offsetof( CrateCollideModuleData, m_kindofany ) },
		{ "ForbidOwnerPlayer", INI::parseBool,	nullptr,	offsetof( CrateCollideModuleData, m_isForbidOwnerPlayer ) },
		{ "AllowNeutralPlayer", INI::parseBool,	nullptr,	offsetof( CrateCollideModuleData, m_isAllowNeutralPlayer ) },
		{ "BuildingPickup", INI::parseBool,	nullptr,	offsetof( CrateCollideModuleData, m_isBuildingPickup ) },
		{ "HumanOnly", INI::parseBool,	nullptr,	offsetof( CrateCollideModuleData, m_isHumanOnlyPickup ) },
		{ "AllowPickAboveTerrain", INI::parseBool,	nullptr,	offsetof( CrateCollideModuleData, m_isAllowPickAboveTerrain ) },
		{ "PickupScience", INI::parseScience,	nullptr,	offsetof( CrateCollideModuleData, m_pickupScience ) },
		{ "ExecuteFX", INI::parseFXList, nullptr, offsetof( CrateCollideModuleData, m_executeFX ) },
		{ "ExecuteAnimation", INI::parseAsciiString, nullptr, offsetof( CrateCollideModuleData, m_executionAnimationTemplate ) },
		{ "ExecuteAnimationTime", INI::parseReal, nullptr, offsetof( CrateCollideModuleData, m_executeAnimationDisplayTimeInSeconds ) },
		{ "ExecuteAnimationZRise", INI::parseReal, nullptr, offsetof( CrateCollideModuleData, m_executeAnimationZRisePerSecond ) },
		{ "ExecuteAnimationFades", INI::parseBool, nullptr, offsetof( CrateCollideModuleData, m_executeAnimationFades ) },

		{ "AffectsTargets", INI::parseBitString32, TheWeaponAffectsMaskNames, offsetof( CrateCollideModuleData, m_targetsMask) },
		{ "TriggeredBy", INI::parseAsciiStringVector, nullptr, offsetof( CrateCollideModuleData, m_activationUpgradeNames ) },
		{ "ConflictsWith", INI::parseAsciiStringVector, nullptr, offsetof( CrateCollideModuleData, m_conflictingUpgradeNames ) },
		{ "RequiresAllTriggers", INI::parseBool, nullptr, offsetof( CrateCollideModuleData, m_requiresAllTriggers ) },
		{ "RequiredStatus", ObjectStatusMaskType::parseFromINI,	nullptr, offsetof( CrateCollideModuleData, m_requiredStatus ) },
		{ "ForbiddenStatus", ObjectStatusMaskType::parseFromINI, nullptr, offsetof( CrateCollideModuleData, m_forbiddenStatus ) },
		{ "RequiredCustomStatus", INI::parseAsciiStringVector, nullptr, offsetof( CrateCollideModuleData, m_requiredCustomStatus ) },
		{ "ForbiddenCustomStatus", INI::parseAsciiStringVector, nullptr, offsetof( CrateCollideModuleData, m_forbiddenCustomStatus ) },

		{ "DeleteUserOnCollide", INI::parseBool, nullptr, offsetof( CrateCollideModuleData, m_destroyOnCollide ) },
		{ "FXOnCollide", INI::parseFXList, nullptr, offsetof( CrateCollideModuleData, m_fxOnCollide ) },
		{ "OCLOnCollide", INI::parseObjectCreationList, nullptr, offsetof( CrateCollideModuleData, m_oclOnCollide ) },

		{ "RejectKeys",				INI::parseAsciiStringVector,		nullptr, offsetof( CrateCollideModuleData, m_rejectKeys ) },
		{ "StatusToRemove",		ObjectStatusMaskType::parseFromINI,	nullptr, offsetof( CrateCollideModuleData, m_statusToRemove ) },
		{ "CustomStatusToRemove",	INI::parseAsciiStringVector, nullptr, offsetof( CrateCollideModuleData, m_customStatusToRemove ) },
		{ "StatusToDestroy",		ObjectStatusMaskType::parseFromINI,	nullptr, offsetof( CrateCollideModuleData, m_statusToDestroy ) },
		{ "CustomStatusToDestroy",	INI::parseAsciiStringVector, nullptr, offsetof( CrateCollideModuleData, m_customStatusToDestroy ) },
		{ "StatusToSet",		ObjectStatusMaskType::parseFromINI,	nullptr, offsetof( CrateCollideModuleData, m_statusToSet ) },
		{ "CustomStatusToSet",	INI::parseAsciiStringVector, nullptr, offsetof( CrateCollideModuleData, m_customStatusToSet ) },
		{ "StatusToGive",		ObjectStatusMaskType::parseFromINI,	nullptr, offsetof( CrateCollideModuleData, m_statusToGive ) },
		{ "CustomStatusToGive",	INI::parseAsciiStringVector, nullptr, offsetof( CrateCollideModuleData, m_customStatusToGive ) },
		{ "WeaponBonusToGive",		INI::parseWeaponBonusVector, nullptr, offsetof( CrateCollideModuleData, m_bonusToGive ) },
		{ "CustomWeaponBonusToGive",	INI::parseAsciiStringVector, nullptr, offsetof( CrateCollideModuleData, m_customBonusToGive ) },

		{ "LeechExpFromObject",		INI::parseBool,		nullptr, offsetof( CrateCollideModuleData, m_leechExpFromObject ) },
		{ "DamagePercentToUnit",	INI::parsePercentToReal,		nullptr, offsetof( CrateCollideModuleData, m_damagePercentageToUnit ) },
		{ "DestroyOnTargetDie",		INI::parseBool,		nullptr, offsetof( CrateCollideModuleData, m_destroyOnTargetDie ) },
		{ "DestroyOnHeal",			INI::parseBool,		nullptr, offsetof( CrateCollideModuleData, m_destroyOnHeal ) },
		{ "RemoveOnHeal",			INI::parseBool,		nullptr, offsetof( CrateCollideModuleData, m_removeOnHeal ) },

		{ "CursorName", INI::parseAsciiString, nullptr, offsetof( CrateCollideModuleData, m_cursorName ) },

		{ nullptr, nullptr, nullptr, 0 }
	};
  p.add(dataFieldParse);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
CrateCollide::CrateCollide( Thing *thing, const ModuleData* moduleData ) : CollideModule( thing, moduleData )
{

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
CrateCollide::~CrateCollide( void )
{

}

//-------------------------------------------------------------------------------------------------
/** The collide event.
	* Note that when other is nullptr it means "collide with ground" */
//-------------------------------------------------------------------------------------------------
void CrateCollide::onCollide( Object *other, const Coord3D *, const Coord3D * )
{
	const CrateCollideModuleData *modData = getCrateCollideModuleData();
	// If the crate can be picked up, perform the game logic and destroy the crate.
	if( isValidToExecute( other ) )
	{
		if( modData->m_fxOnCollide != nullptr )
		{
			FXList::doFXObj( modData->m_fxOnCollide, getObject() );
		}
		if( modData->m_oclOnCollide != nullptr )
		{
			ObjectCreationList::create( modData->m_oclOnCollide, getObject(), nullptr );
		}

		if( executeCrateBehavior( other ) )
		{
			if( modData->m_executeFX != nullptr )
			{
				// Note: We pass in other here, because the crate is owned by the neutral player, and
				// we want to do things that only the other person can see.
				FXList::doFXObj( modData->m_executeFX, other );
			}

			TheGameLogic->destroyObject( getObject() );
		} 
		else if( modData->m_destroyOnCollide == TRUE)
		{
			TheGameLogic->destroyObject( getObject() );
		}

		// play animation in the world at this spot if there is one
		if( TheAnim2DCollection && modData->m_executionAnimationTemplate.isEmpty() == FALSE && TheGameLogic->getDrawIconUI() )
		{
			Anim2DTemplate *animTemplate = TheAnim2DCollection->findTemplate( modData->m_executionAnimationTemplate );

			TheInGameUI->addWorldAnimation( animTemplate,
																			getObject()->getPosition(),
																			WORLD_ANIM_FADE_ON_EXPIRE,
																			modData->m_executeAnimationDisplayTimeInSeconds,
																			modData->m_executeAnimationZRisePerSecond );

		}

	}

}

Bool CrateCollide::executeCrateBehavior( Object *other )
{
	const CrateCollideModuleData* md = getCrateCollideModuleData();
	Object *obj = getObject();
	
	// Set the Reject Key
	if(!md->m_rejectKeys.empty())
	{
		other->setRejectKey( md->m_rejectKeys );
	}

	// Set the Statuses
	obj->setStatus( md->m_statusToSet );
	other->setStatus( md->m_statusToGive );
	for(std::vector<AsciiString>::const_iterator it = md->m_customStatusToSet.begin(); it != md->m_customStatusToSet.end(); ++it)
	{
		obj->setCustomStatus( *it );
	}
	for(std::vector<AsciiString>::const_iterator it = md->m_customStatusToGive.begin(); it != md->m_customStatusToGive.end(); ++it)
	{
		other->setCustomStatus( *it );
	}
	// Give the bonuses to the Object
	for (Int i = 0; i < md->m_bonusToGive.size(); i++) {
		other->setWeaponBonusCondition(md->m_bonusToGive[i]);
	}
	for(std::vector<AsciiString>::const_iterator it = md->m_customBonusToGive.begin(); it != md->m_customBonusToGive.end(); ++it)
	{
		other->setCustomWeaponBonusCondition( *it );
	}
	return FALSE;
}

Bool CrateCollide::revertCollideBehavior( Object *other )
{
	const CrateCollideModuleData* md = getCrateCollideModuleData();
	Object *obj = getObject();
	
	// Clear other statuses set by this module
	obj->clearStatus( md->m_statusToSet );
	for(std::vector<AsciiString>::const_iterator it = md->m_customStatusToSet.begin(); it != md->m_customStatusToSet.end(); ++it)
	{
		obj->clearCustomStatus( *it );
	}

	// If the Object doesn't exist, or is destroyed we stop here.
	if(!other || other->isDestroyed() || other->isEffectivelyDead())
		return FALSE;

	// Remove the Reject Key from the Object
	if(!md->m_rejectKeys.empty())
	{
		other->clearRejectKey( md->m_rejectKeys );
	}

	// Clear the statuses and bonuses of the Object after removed from Hijacking
	other->clearStatus( md->m_statusToGive );
	for(std::vector<AsciiString>::const_iterator it = md->m_customStatusToGive.begin(); it != md->m_customStatusToGive.end(); ++it)
	{
		other->clearCustomStatus( *it );
	}
	for (Int i = 0; i < md->m_bonusToGive.size(); i++) {
		other->clearWeaponBonusCondition(md->m_bonusToGive[i]);
	}
	for(std::vector<AsciiString>::const_iterator it = md->m_customBonusToGive.begin(); it != md->m_customBonusToGive.end(); ++it)
	{
		other->clearCustomWeaponBonusCondition( *it );
	}
	return FALSE;
}

//-------------------------------------------------------------------------------------------------
Bool CrateCollide::isValidToExecute( const Object *other ) const
{
	//The ground never picks up a crate
	if( other == nullptr )
		return FALSE;

	const CrateCollideModuleData* md = getCrateCollideModuleData();

	Relationship r = getObject()->getRelationship( other );

	if ( !( ((md->m_targetsMask & WEAPON_AFFECTS_ALLIES ) && r == ALLIES) ||
			((md->m_targetsMask & WEAPON_AFFECTS_ENEMIES ) && r == ENEMIES ) ||
			((md->m_targetsMask & WEAPON_AFFECTS_NEUTRALS ) && r == NEUTRAL ) ||
			md->m_targetsMask == 0)
	   )
		return FALSE;


	//Nothing Neutral can pick up any type of crate
	if(other->isNeutralControlled() && !md->m_isAllowNeutralPlayer)
		return FALSE;

	Bool validBuildingAttempt = md->m_isBuildingPickup && other->isKindOf( KINDOF_STRUCTURE );

	// Must be a "Unit" type thing.  Real Game Object, not just Object
	if( other->getAIUpdateInterface() == nullptr  &&  !validBuildingAttempt )// Building exception flag for Drop Zone
		return FALSE;

	// must match our kindof flags (if any)
	if ( !md->m_kindofany.any() && !other->isKindOfMulti(md->m_kindof, md->m_kindofnot) )
		return FALSE;

	if( md->m_kindofany.any() && (!other->isAnyKindOf(md->m_kindofany) || other->isAnyKindOf(md->m_kindofnot)) )
	   return FALSE;

	if( other->isEffectivelyDead() )
		return FALSE;

	// crates cannot be claimed while in the air, except by buildings
	if( getObject()->isAboveTerrain() && !(validBuildingAttempt || md->m_isAllowPickAboveTerrain))
		return FALSE;

	// TheSuperHackers @bugfix Stubbjax 09/02/2026 Prevent the crate from being collected multiple times in a single frame.
#if !RETAIL_COMPATIBLE_CRC
	if (getObject()->isDestroyed())
		return FALSE;
#endif

	if( md->m_isForbidOwnerPlayer  &&  (getObject()->getControllingPlayer() == other->getControllingPlayer()) )
		return FALSE; // Design has decreed this to not be picked up by the dead guy's team.

	if( md->m_isHumanOnlyPickup  &&  other->getControllingPlayer() && (other->getControllingPlayer()->getPlayerType() != PLAYER_HUMAN) )
		return FALSE; // Human only mission crate

	if( (md->m_pickupScience != SCIENCE_INVALID)  &&  other->getControllingPlayer()  &&  !other->getControllingPlayer()->hasScience(md->m_pickupScience) )
		return FALSE; // Science required to pick this up

	if( other->isKindOf( KINDOF_PARACHUTE ) )
		return FALSE;

	if( !md->m_rejectKeys.empty() && other->hasRejectKey(md->m_rejectKeys) )
	{
		return FALSE; // If the object already have the reject key set, return false
	}

	if( !passRequirements() )
		return FALSE;

	return TRUE;
}

//-------------------------------------------------------------------------------------------------
Bool CrateCollide::passRequirements() const
{
	const Object *source = getObject();
	const CrateCollideModuleData* md = getCrateCollideModuleData();
	std::vector<AsciiString> activationUpgrades = md->m_activationUpgradeNames;
	std::vector<AsciiString> conflictingUpgrades = md->m_conflictingUpgradeNames;
	Bool RequiresAllTriggers = md->m_requiresAllTriggers;
	ObjectStatusMaskType statusRequired = md->m_requiredStatus;
	ObjectStatusMaskType statusForbidden = md->m_forbiddenStatus;
	std::vector<AsciiString> customStatusRequired = md->m_requiredCustomStatus;
	std::vector<AsciiString> customStatusForbidden = md->m_forbiddenCustomStatus;

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


// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void CrateCollide::crc( Xfer *xfer )
{

	// extend base class
	CollideModule::crc( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Xfer Method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void CrateCollide::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	CollideModule::xfer( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void CrateCollide::loadPostProcess( void )
{

	// extend base class
	CollideModule::loadPostProcess();

}

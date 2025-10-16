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

// FILE: ActiveBody.cpp ///////////////////////////////////////////////////////////////////////////
// Author: Colin Day, November 2001
// Desc:   Active bodies have health, they can die and are affected by health
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file int the GameEngine
#include "Common/BitFlagsIO.h"
#include "Common/CRCDebug.h"
#include "Common/DamageFX.h"
#include "Common/Player.h"
#include "Common/GameState.h"
#include "Common/GlobalData.h"
#include "Common/PlayerList.h"
#include "Common/Team.h"
#include "Common/Thing.h"
#include "Common/ThingTemplate.h"
#include "Common/Xfer.h"
#include "GameClient/ControlBar.h"
#include "GameClient/Drawable.h"
#include "GameClient/InGameUI.h"
#include "GameClient/ParticleSys.h"
#include "GameLogic/AI.h"
#include "GameLogic/AIPathfind.h"
#include "GameLogic/Armor.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/Object.h"
#include "GameLogic/ObjectCreationList.h"
#include "GameLogic/Damage.h"
#include "GameLogic/PartitionManager.h"
#include "GameLogic/TerrainLogic.h"
#include "GameLogic/Weapon.h"
#include "GameLogic/Module/AIUpdate.h"
#include "GameLogic/Module/ActiveBody.h"
#include "GameLogic/Module/BridgeBehavior.h"
#include "GameLogic/Module/ContainModule.h"
#include "GameLogic/Module/DamageModule.h"
#include "GameLogic/Module/DieModule.h"



#define YELLOW_DAMAGE_PERCENT (0.25f)

// FORWARD REFERENCES /////////////////////////////////////////////////////////////////////////////

// ------------------------------------------------------------------------------------------------
/** Body particle systems are particle systems that are automatically created and attached
	* to an object as the damage state changes for that object.  We keep a list of these
	* so that when we transition from one state to another we can kill any old particle
	* systems that we need to before we create new ones */
// ------------------------------------------------------------------------------------------------
class BodyParticleSystem : public MemoryPoolObject
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( BodyParticleSystem, "BodyParticleSystem" )

public:

	ParticleSystemID m_particleSystemID;		///< the particle system ID
	BodyParticleSystem *m_next;							///< next particle system in this body module

};

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
BodyParticleSystem::~BodyParticleSystem( void )
{

}  // end ~BodyParticleSystem

///////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS ///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
static BodyDamageType calcDamageState(Real health, Real maxHealth)
{
	if (!TheGlobalData)
		return BODY_PRISTINE;

	Real ratio = health / maxHealth;

	if (ratio > TheGlobalData->m_unitDamagedThresh)
	{
		return BODY_PRISTINE;
	}
	else if (ratio > TheGlobalData->m_unitReallyDamagedThresh)
	{
		return BODY_DAMAGED;
	}
	else if (ratio > 0.0f)
	{
		return BODY_REALLYDAMAGED;
	}
	else
	{
		return BODY_RUBBLE;
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
ActiveBodyModuleData::ActiveBodyModuleData()
{
	m_maxHealth = 0;
	m_initialHealth = 0;
	m_subdualDamageCap = 0;
	m_subdualDamageHealRate = 0;
	m_subdualDamageHealAmount = 0;
	m_subdualDamageCapCustom.clear();
	m_subdualDamageHealRateCustom.clear();
	m_subdualDamageHealAmountCustom.clear();
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void ActiveBodyModuleData::buildFieldParse(MultiIniFieldParse& p)
{
  ModuleData::buildFieldParse(p);

	static const FieldParse dataFieldParse[] =
	{
		{ "MaxHealth",						INI::parseReal,						NULL,		offsetof( ActiveBodyModuleData, m_maxHealth ) },
		{ "InitialHealth",				INI::parseReal,						NULL,		offsetof( ActiveBodyModuleData, m_initialHealth ) },

		{ "SubdualDamageCap",					INI::parseReal,									NULL,		offsetof( ActiveBodyModuleData, m_subdualDamageCap ) },
		{ "SubdualDamageHealRate",		INI::parseDurationUnsignedInt,	NULL,		offsetof( ActiveBodyModuleData, m_subdualDamageHealRate ) },
		{ "SubdualDamageHealAmount",	INI::parseReal,									NULL,		offsetof( ActiveBodyModuleData, m_subdualDamageHealAmount ) },

		{ "CustomSubdualDamageCap", 		INI::parseAsciiStringWithColonVectorAppend, NULL, offsetof( ActiveBodyModuleData, m_subdualDamageCapCustom ) },
		{ "CustomSubdualDamageHealRate",	INI::parseAsciiStringWithColonVectorAppend,	NULL,		offsetof( ActiveBodyModuleData, m_subdualDamageHealRateCustom ) },
		{ "CustomSubdualDamageHealAmount",	INI::parseAsciiStringWithColonVectorAppend,	NULL,		offsetof( ActiveBodyModuleData, m_subdualDamageHealAmountCustom ) },
		{ 0, 0, 0, 0 }
	};
  p.add(dataFieldParse);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
ActiveBody::ActiveBody( Thing *thing, const ModuleData* moduleData ) :
	BodyModule(thing, moduleData),
	m_curDamageFX(NULL),
	m_curArmorSet(NULL),
	m_frontCrushed(false),
	m_backCrushed(false),
	m_lastDamageTimestamp(0xffffffff),// So we don't think we just got damaged on the first frame
	m_lastHealingTimestamp(0xffffffff),// So we don't think we just got healed on the first frame
	m_curDamageState(BODY_PRISTINE),
	m_nextDamageFXTime(0),
	m_lastDamageFXDone((DamageType)-1),
	m_lastDamageCleared(false),
	m_particleSystems(NULL),
	m_currentSubdualDamage(0),
	m_indestructible(false),
	m_damageFXOverride(false),
	m_hasBeenSubdued(false),
	m_customSubdualDisabledSound(NULL),
	m_customSubdualDisableRemovalSound(NULL),
	m_clearedSubdued(FALSE),
	m_clearedSubduedCustom(FALSE)
{
	const ActiveBodyModuleData *data = getActiveBodyModuleData();

	m_currentHealth = getActiveBodyModuleData()->m_initialHealth;
	m_prevHealth = getActiveBodyModuleData()->m_initialHealth;
	m_maxHealth = getActiveBodyModuleData()->m_maxHealth;
	m_initialHealth = getActiveBodyModuleData()->m_initialHealth;
	m_subdualDamageCap = data->m_subdualDamageCap;
	m_subdualDamageHealRate = data->m_subdualDamageHealRate;
	m_subdualDamageHealAmount = data->m_subdualDamageHealAmount;

	m_currentSubdualDamageCustom.clear();
	m_subdualDamageCapCustom.clear();
	m_subdualDamageHealRateCustom.clear();
	m_subdualDamageHealAmountCustom.clear();

	Bool parseName = TRUE;
	AsciiString customStatus = NULL;
	for(std::vector<AsciiString>::const_iterator it = data->m_subdualDamageCapCustom.begin(); it != data->m_subdualDamageCapCustom.end(); ++it)
	{
		if(parseName)
		{
			customStatus = (*it);
			parseName = FALSE;
		}
		else
		{
			m_subdualDamageCapCustom[customStatus] = INI::scanReal((*it).str());
			parseName = TRUE;
		}
	}

	parseName = TRUE;
	customStatus = NULL;
	for(std::vector<AsciiString>::const_iterator it2 = data->m_subdualDamageHealRateCustom.begin(); it2 != data->m_subdualDamageHealRateCustom.end(); ++it2)
	{
		if(parseName)
		{
			customStatus = (*it2);
			parseName = FALSE;
		}
		else
		{
			m_subdualDamageHealRateCustom[customStatus] = INI::scanUnsignedInt((*it2).str());
			parseName = TRUE;
		}
	}

	parseName = TRUE;
	customStatus = NULL;
	for(std::vector<AsciiString>::const_iterator it3 = data->m_subdualDamageHealAmountCustom.begin(); it3 != data->m_subdualDamageHealAmountCustom.end(); ++it3)
	{
		if(parseName)
		{
			customStatus = (*it3);
			parseName = FALSE;
		}
		else
		{
			m_subdualDamageHealAmountCustom[customStatus] = INI::scanReal((*it3).str());
			parseName = TRUE;
		}
	}

	// force an initially-valid armor setup
	validateArmorAndDamageFX();
	// start us in the right state
	setCorrectDamageState();

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
ActiveBody::~ActiveBody( void )
{
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void ActiveBody::onDelete( void )
{

	// delete all particle systems
	deleteAllParticleSystems();

}  // end onDelete

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void ActiveBody::setCorrectDamageState()
{
	m_curDamageState = calcDamageState(m_currentHealth, m_maxHealth);

	/// @todo srj -- bleah, this is an icky way to do it. oh well.
	if (m_curDamageState == BODY_RUBBLE && getObject()->isKindOf(KINDOF_STRUCTURE))
	{
		Real rubbleHeight = getObject()->getTemplate()->getStructureRubbleHeight();

		if (rubbleHeight <= 0.0f)
			rubbleHeight = TheGlobalData->m_defaultStructureRubbleHeight;

		/** @todo I had to change this to a Z only version to keep it from disappearing from the
			PartitionManager for a frame.  That didn't used to happen.
		*/
		getObject()->setGeometryInfoZ(rubbleHeight);

		// Have to tell pathfind as well, as rubble pathfinds differently.
		TheAI->pathfinder()->removeObjectFromPathfindMap(getObject());
		TheAI->pathfinder()->addObjectToPathfindMap(getObject());


		// here we make sure nobody collides with us, ever again...			//Lorenzen
		//THis allows projectiles shot from infantry that are inside rubble to get out of said rubble safely
		getObject()->setStatus( MAKE_OBJECT_STATUS_MASK( OBJECT_STATUS_NO_COLLISIONS ) );


	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void ActiveBody::setDamageState( BodyDamageType newState )
{
	Real ratio = 1.0f;
	if( newState == BODY_PRISTINE )
	{
		ratio = 1.0f;
	}
	else if( newState == BODY_DAMAGED )
	{
		ratio = TheGlobalData->m_unitDamagedThresh;
	}
	else if( newState == BODY_REALLYDAMAGED )
	{
		ratio = TheGlobalData->m_unitReallyDamagedThresh;
	}
	else if( newState == BODY_RUBBLE )
	{
		ratio = 0.0f;
	}
	Real desiredHealth = m_maxHealth * ratio - 1;// -1 because < not <= in calcState
	desiredHealth = max( desiredHealth, 0.0f );
	internalChangeHealth( desiredHealth - m_currentHealth );
	setCorrectDamageState();
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void ActiveBody::validateArmorAndDamageFX() const
{
	const ArmorTemplateSet* set = getObject()->getTemplate()->findArmorTemplateSet(m_curArmorSetFlags);
	DEBUG_ASSERTCRASH(set, ("findArmorSet should never return null"));
	if (set && set != m_curArmorSet)
	{
		if (set->getArmorTemplate())
		{
			m_curArmor = TheArmorStore->makeArmor(set->getArmorTemplate());
		}
		else
		{
			m_curArmor.clear();
		}
		if (!m_damageFXOverride) m_curDamageFX = set->getDamageFX();  // Only set this if override is cleared
		m_curArmorSet = set;
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
Real ActiveBody::estimateDamage( DamageInfoInput& damageInfo ) const
{
	validateArmorAndDamageFX();

	//Subdual damage can't affect you if you can't be subdued
	if( IsSubdualDamage(damageInfo.m_damageType)  &&  !canBeSubdued() )
		return 0.0f;

	if( damageInfo.m_damageType == DAMAGE_KILL_GARRISONED )
	{
		ContainModuleInterface* contain = getObject()->getContain();
		if( contain && contain->getContainCount() > 0 && contain->isGarrisonable() && !contain->isImmuneToClearBuildingAttacks() )
			return 1.0f;
		else
			return 0.0f;
	}

	if( damageInfo.m_damageType == DAMAGE_SNIPER )
	{
		if( getObject()->isKindOf( KINDOF_STRUCTURE ) && getObject()->testStatus( OBJECT_STATUS_UNDER_CONSTRUCTION ) )
		{
			//If we're a pathfinder shooting a stinger site under construction... don't. Special case code.
			return 0.0f;
		}
	}
	// Compute Armor Bonuses (from weapons) first
	Real amount = 1.0f;
	const Weapon* w = NULL;
	w = getObject()->getCurrentWeapon();
	amount = w ? w->getArmorBonus(getObject()) : 1.0f;

	// Compute Armor Bonuses based on Status Types, Weapon Bonus Conditions and Custom Weapon Bonus Conditions
	ObjectStatusMaskType objStatus = getObject()->getStatusBits();
	WeaponBonusConditionFlags objFlags = getObject()->getWeaponBonusCondition();
	ObjectCustomStatusType objCustomStatus = getObject()->getCustomStatus();
	ObjectCustomStatusType objCustomFlags = getObject()->getCustomWeaponBonusCondition();
	amount *= m_curArmor.scaleArmorBonus(objStatus, objFlags, objCustomStatus, objCustomFlags);
	
	// Compute damage according to Armor Coefficient.
	amount *= m_curArmor.adjustDamage(damageInfo.m_damageType, damageInfo.m_amount, damageInfo.m_customDamageType);
	//if(!damageInfo.m_customDamageType.isEmpty()) amount = m_curArmor.adjustDamageCustom(damageInfo.m_customDamageType, damageInfo.m_amount);

	return amount;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void ActiveBody::doDamageFX( const DamageInfo *damageInfo )
{
	DamageType damageTypeToUse = damageInfo->in.m_damageType;
	if (damageInfo->in.m_damageFXOverride != DAMAGE_UNRESISTABLE )
	{
		// Just the visual aspect of damage can be overridden in some cases.
		// Unresistable is the default to mean no override, as we are out of bits.
		damageTypeToUse = damageInfo->in.m_damageFXOverride;
	}

	if (m_curDamageFX)
	{
		UnsignedInt now = TheGameLogic->getFrame();
		if (damageTypeToUse == m_lastDamageFXDone && m_nextDamageFXTime > now)
			return;
		Object *source = TheGameLogic->findObjectByID(damageInfo->in.m_sourceID);	// might be null, I guess
		m_lastDamageFXDone = damageTypeToUse;
		m_nextDamageFXTime = now + m_curDamageFX->getDamageFXThrottleTime(damageTypeToUse, source);
		m_curDamageFX->doDamageFX(damageTypeToUse, damageInfo->out.m_actualDamageDealt, source, getObject());
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void ActiveBody::attemptDamage( DamageInfo *damageInfo )
{
	validateArmorAndDamageFX();

	// sanity
	if( damageInfo == NULL )
		return;

	if ( m_indestructible )
		return;

	// initialize these, just in case we bail out early
	damageInfo->out.m_actualDamageDealt = 0.0f;
	damageInfo->out.m_actualDamageClipped = 0.0f;

	// we cannot damage again objects that are already dead
	Object* obj = getObject();
	if( obj->isEffectivelyDead() )
		return;

	Real objHeight;
	// Structures don't have altitudes
	if( obj->isKindOf( KINDOF_STRUCTURE ) )
	{
		// But they have geometrical heights
		objHeight = obj->getGeometryInfo().getMaxHeightAbovePosition();
	}
	else
	{
		objHeight = obj->getHeightAboveTerrain();
	}

	// Structures don't have a Max Target Height since all Structures are built on ground
	if( objHeight < damageInfo->in.m_minDamageHeight || ( !obj->isKindOf( KINDOF_STRUCTURE ) && damageInfo->in.m_maxDamageHeight && damageInfo->in.m_maxDamageHeight < objHeight ))
		return;
	

	Object *damager = TheGameLogic->findObjectByID( damageInfo->in.m_sourceID );
	if( damager )
	{
		//Store the template so later if the attacking object dies, we use script conditions to look at the
		//damager's template inside evaluateTeamAttackedByType or evaluateNameAttackedByType.
		damageInfo->in.m_sourceTemplate = damager->getTemplate();
		
		//If damager is affected by Zero Damage or No Damage Status, return
		if (damager->testCustomStatus("ZERO_DAMAGE") || damager->testCustomStatus("NO_DAMAGE"))
			return;
	}

	Bool alreadyHandled = FALSE;
	Bool allowModifier = TRUE;
	Bool doDamageModules = TRUE;
	Bool adjustConditions = TRUE;
	Bool killsGarrisoned = FALSE;
	Bool isStatusDamage = FALSE;
	Bool isKilled = FALSE;
	Real amount = m_curArmor.adjustDamage(damageInfo->in.m_damageType, damageInfo->in.m_amount, damageInfo->in.m_customDamageType);
	Real realFramesToStatusFor = 0.0f;
	const Weapon* w = NULL;
	w = obj->getCurrentWeapon();
	Real armorBonus = w ? w->getArmorBonus(obj) : 1.0f;

	ObjectStatusMaskType objStatus = obj->getStatusBits();
	WeaponBonusConditionFlags objFlags = obj->getWeaponBonusCondition();
	ObjectCustomStatusType objCustomStatus = obj->getCustomStatus();
	ObjectCustomStatusType objCustomFlags = obj->getCustomWeaponBonusCondition();
	armorBonus *= m_curArmor.scaleArmorBonus(objStatus, objFlags, objCustomStatus, objCustomFlags);

	amount *= armorBonus;

	if(damageInfo->in.m_damageStatusType != OBJECT_STATUS_NONE || !damageInfo->in.m_customDamageStatusType.isEmpty())
	{
		realFramesToStatusFor = ConvertDurationFromMsecsToFrames(amount);
		if (damageInfo->in.m_statusDuration > 0) {
			realFramesToStatusFor = ConvertDurationFromMsecsToFrames(damageInfo->in.m_statusDuration);
			if (damageInfo->in.m_statusDurationTypeCorrelate) {
				realFramesToStatusFor = m_curArmor.adjustDamage(damageInfo->in.m_damageType, realFramesToStatusFor, damageInfo->in.m_customDamageType);
				realFramesToStatusFor *= armorBonus;
			}
		}
	}

	// Units that get disabled by Chrono damage cannot take damage:
	if (obj->isDisabledByType(DISABLED_CHRONO) &&
		!(damageInfo->in.m_damageType == DAMAGE_CHRONO_GUN || damageInfo->in.m_damageType == DAMAGE_CHRONO_UNRESISTABLE))
		return;

	switch( damageInfo->in.m_damageType )
	{
		case DAMAGE_HEALING:
		{
			if( !damageInfo->in.m_kill )
			{
				// Healing and Damage are separate, so this shouldn't happen
				attemptHealing( damageInfo );
			}
			return;
		}

		case DAMAGE_KILLPILOT:
		{
			// This type of damage doesn't actually damage the unit, but it does kill it's
			// pilot, in the case of a vehicle.
			if( obj->isKindOf( KINDOF_VEHICLE ) )
			{
				//Handle special case for combat bike. We actually will kill the bike by
				//forcing the rider to leave the bike. That way the bike will automatically
				//scuttle and be unusable.
				ContainModuleInterface *contain = obj->getContain();
				if( contain && contain->isRiderChangeContain() )
				{

					AIUpdateInterface *ai = obj->getAI();

					if( ai->isMoving() )
					{
						//Bike is moving, so just blow it up instead.
						if (damager)
							damager->scoreTheKill( obj );
						obj->kill();
					}
					else
					{
						//Removing the rider will scuttle the bike.
						Object *rider = *(contain->getContainedItemsList()->begin());
						ai->aiEvacuateInstantly( TRUE, CMD_FROM_AI );

						//Kill the rider.
						if (damager)
							damager->scoreTheKill( rider );
						rider->kill();
					}
				}
				else
				{
					// Make it unmanned, so units can easily check the ability to "take control of it"
					obj->setDisabled( DISABLED_UNMANNED );
					TheGameLogic->deselectObject(obj, PLAYERMASK_ALL, TRUE);

          if ( obj->getAI() )
            obj->getAI()->aiIdle( CMD_FROM_AI );

					// Convert it to the neutral team so it renders gray giving visual representation that it is unmanned.
					obj->setTeam( ThePlayerList->getNeutralPlayer()->getDefaultTeam() );
				}

				//We don't care which team sniped the vehicle... we use this information to flag whether or not
				//we captured a vehicle.
	      ThePlayerList->getNeutralPlayer()->getAcademyStats()->recordVehicleSniped();
			}
			alreadyHandled = TRUE;
			allowModifier = FALSE;
			break;
		}

		case DAMAGE_KILL_GARRISONED:
		{
			// KRIS: READ THIS!!!
			// This code is very misleading (but in a good way). One would think this is
			// an excellent place to add the hook to kill garrisoned troops. And that is
			// a correct assumption. Unfortunately, the vast majority of garrison slayings
			// are performed in DumbProjectileBehavior::projectileHandleCollision(), so my
			// hope is that this message will save you some research time!

			killsGarrisoned = TRUE;

			Int killsToMake = REAL_TO_INT_FLOOR(damageInfo->in.m_amount);
			ContainModuleInterface* contain = obj->getContain();
			if( contain && contain->getContainCount() > 0 && contain->isGarrisonable() && !contain->isImmuneToClearBuildingAttacks() )
			{
				Int numKilled = 0;

				// garrisonable buildings subvert the normal process here.
				const ContainedItemsList* items = contain->getContainedItemsList();
				if (items)
				{
					for( ContainedItemsList::const_iterator it = items->begin(); (it != items->end()) && (numKilled < killsToMake); it++ )
					{
						Object* thingToKill = *it;
						if (!thingToKill->isEffectivelyDead() )
						{
							if (damager)
								damager->scoreTheKill( thingToKill );
							thingToKill->kill();
							++numKilled;
							thingToKill->getControllingPlayer()->getAcademyStats()->recordClearedGarrisonedBuilding();
						}
					} // next contained item

				} // if items
			}	// if a garrisonable thing
			alreadyHandled = TRUE;
			allowModifier = FALSE;
			break;
		}

		case DAMAGE_STATUS:
		{
			isStatusDamage = TRUE;
			// Damage amount is msec time we set the status given in damageStatusType
			// Real realFramesToStatusFor = ConvertDurationFromMsecsToFrames(amount);
			obj->doStatusDamage( damageInfo->in.m_damageStatusType , REAL_TO_INT_CEIL(realFramesToStatusFor) , damageInfo->in.m_customDamageStatusType , damageInfo->in.m_customTintStatus , damageInfo->in.m_tintStatus );
			
			// Custom feature, similar to Black Hole Armor
			if(damageInfo->in.m_customDamageStatusType == "SHIELDED_TARGET")
				obj->setShieldByTargetID(damageInfo->in.m_sourceID, damageInfo->in.m_protectionTypes);
			
			alreadyHandled = TRUE;
			allowModifier = FALSE;
			break;
		}

		case DAMAGE_CHRONO_GUN:
		case DAMAGE_CHRONO_UNRESISTABLE:
		{
			// This handles both gaining chrono damage and recovering from it

			// Note: Should HoldTheLine or Shields apply? (Not for recovery)
			if (damageInfo->in.m_damageType != DAMAGE_CHRONO_UNRESISTABLE) {
				amount *= m_damageScalar;
			}
			
			Bool wasSubdued = isSubduedChrono();

			// Increase damage counter
			internalAddChronoDamage(amount);
			// DEBUG_LOG(("ActiveBody::attemptDamage - amount = %f, chronoDmg = %f\n", amount, getCurrentChronoDamageAmount()));
			
			// Check for disabling threshold
			Bool nowSubdued = isSubduedChrono();

			if (wasSubdued != nowSubdued)
			{
				// Enable/Disable ; Apply/Remove Visual Effects
				onSubdualChronoChange(nowSubdued);
			}

			// This will handle continuous art changes such as transparency
			getObject()->notifyChronoDamage(amount);

			// Check kill state:
			if (getCurrentChronoDamageAmount() > getMaxHealth()) {
				damageInfo->in.m_kill = TRUE;
				doDamageModules = FALSE;
				adjustConditions = FALSE;
			}
			else {
				alreadyHandled = TRUE;
			}
			allowModifier = FALSE;
		}
	}

	if(!killsGarrisoned && damageInfo->in.m_killsGarrison == TRUE)
	{
		Int killsToMake = REAL_TO_INT_FLOOR(damageInfo->in.m_amount);
		if(damageInfo->in.m_killsGarrisonAmount > 0)
		{
			killsToMake = damageInfo->in.m_killsGarrisonAmount;
		}
		ContainModuleInterface* contain = obj->getContain();
		if( contain && contain->getContainCount() > 0 && contain->isGarrisonable() && !contain->isImmuneToClearBuildingAttacks() )
		{
			Int numKilled = 0;

			// garrisonable buildings subvert the normal process here.
			const ContainedItemsList* items = contain->getContainedItemsList();
			if (items)
			{
				for( ContainedItemsList::const_iterator it = items->begin(); (it != items->end()) && (numKilled < killsToMake); it++ )
				{
					Object* thingToKill = *it;
					if (!thingToKill->isEffectivelyDead() )
					{
						if (damager)
							damager->scoreTheKill( thingToKill );
						thingToKill->kill();
						++numKilled;
						thingToKill->getControllingPlayer()->getAcademyStats()->recordClearedGarrisonedBuilding();
					}
				} // next contained item

			} // if items
		}	// if a garrisonable thing
	}

	if (!isStatusDamage && damageInfo->in.m_doStatusDamage)
	{
		obj->doStatusDamage( damageInfo->in.m_damageStatusType , REAL_TO_INT_CEIL(realFramesToStatusFor) , damageInfo->in.m_customDamageStatusType , damageInfo->in.m_customTintStatus , damageInfo->in.m_tintStatus );
		
		// Custom feature, similar to Black Hole Armor
		if(damageInfo->in.m_customDamageStatusType == "SHIELDED_TARGET")
			obj->setShieldByTargetID(damageInfo->in.m_sourceID, damageInfo->in.m_protectionTypes);
	}

	// Custom Subdual Type
	if( !damageInfo->in.m_subdualCustomType.isEmpty() )
	{
		if( !canBeSubduedCustom(damageInfo->in.m_subdualCustomType) || obj->isAnyKindOf(damageInfo->in.m_subdualForbiddenKindOf) )
		{
			if( IsSubdualDamage(damageInfo->in.m_damageType) && damageInfo->in.m_customDamageType.isEmpty() )
				return;
		}
		else 
		{
			Real Subdualamount = amount;
			if(damageInfo->in.m_subdualDamageMultiplier != 1.0f)
			{
				Subdualamount *= damageInfo->in.m_subdualDamageMultiplier;
			}
			if ( ( damageInfo->in.m_damageType != DAMAGE_SUBDUAL_UNRESISTABLE && damageInfo->in.m_damageType != DAMAGE_UNRESISTABLE ) || !damageInfo->in.m_customDamageType.isEmpty() ) {
				Subdualamount *= m_damageScalar;
			}

			SubdualCustomData subdualData;
			subdualData.damage = Subdualamount;
			subdualData.tintStatus = damageInfo->in.m_customSubdualTint;
			subdualData.customTintStatus = damageInfo->in.m_customSubdualCustomTint;
			subdualData.disableType = damageInfo->in.m_customSubdualDisableType;
			subdualData.isSubdued = FALSE;

			Bool wasSubdued = FALSE;
			Bool nowSubdued = FALSE;

			CustomSubdualCurrentDamageMap::iterator it = m_currentSubdualDamageCustom.find(damageInfo->in.m_subdualCustomType);
			if(it != m_currentSubdualDamageCustom.end())
			{
				wasSubdued = m_maxHealth <= it->second.damage ? TRUE : FALSE; //isSubduedCustom(damageInfo->in.m_subdualCustomType);

				internalAddSubdualDamageCustom(subdualData,damageInfo->in.m_subdualCustomType);

				nowSubdued = m_maxHealth <= it->second.damage ? TRUE : FALSE; //isSubduedCustom(damageInfo->in.m_subdualCustomType);

				if(damageInfo->in.m_customSubdualClearOnTrigger && nowSubdued)
				{
					it->second.damage = 0.1f;
				}

				if( it->second.disableType != subdualData.disableType && getObject()->isDisabledByType(it->second.disableType) )
				{
					wasSubdued = FALSE;

					obj->clearDisabled(it->second.disableType);
					//DEBUG_LOG(( "SubdualCustomType: %s, Changed Disabled Type, Cleared: %d, Now: %d", damageInfo->in.m_subdualCustomType.str(), it->second.disableType, subdualData.disableType ));
				}

				if( ( it->second.tintStatus != subdualData.tintStatus || it->second.customTintStatus != subdualData.customTintStatus ) && obj->getDrawable() )
				{
					wasSubdued = FALSE;

					if(!it->second.customTintStatus.isEmpty())
						getObject()->getDrawable()->clearCustomTintStatus(it->second.customTintStatus);
					else if(it->second.tintStatus > TINT_STATUS_INVALID && it->second.tintStatus < TINT_STATUS_COUNT)
						getObject()->getDrawable()->clearTintStatus(it->second.tintStatus);
				}
			}
			else
			{
				nowSubdued = m_maxHealth <= Subdualamount ? TRUE : FALSE;

				if(damageInfo->in.m_customSubdualClearOnTrigger && nowSubdued)
				{
					it->second.damage = 0.1f;
				}
				
				internalAddSubdualDamageCustom(subdualData,damageInfo->in.m_subdualCustomType);
			}

			if(damageInfo->in.m_subdualDealsNormalDamage == FALSE)
			{
				alreadyHandled = TRUE;
				allowModifier = FALSE;
			}

			if(damageInfo->in.m_customSubdualClearOnTrigger && obj->isDisabledByType(damageInfo->in.m_customSubdualDisableType) && damageInfo->in.m_customSubdualHasDisable && !obj->isKindOf(KINDOF_PROJECTILE))
				onSubdualRemovalCustom(damageInfo->in.m_customSubdualDisableType, TRUE);

			if( wasSubdued != nowSubdued )
			{
				onSubdualChangeCustom(nowSubdued, damageInfo, damageInfo->in.m_customSubdualClearOnTrigger);
			}

			if( nowSubdued )
			{
				if(damageInfo->in.m_customSubdualOCL != NULL)
					ObjectCreationList::create(damageInfo->in.m_customSubdualOCL, getObject(), NULL );

				if(damageInfo->in.m_customSubdualDoStatus)
				{
					getObject()->doStatusDamage( damageInfo->in.m_damageStatusType , REAL_TO_INT_CEIL(realFramesToStatusFor) , damageInfo->in.m_customDamageStatusType , damageInfo->in.m_customTintStatus , damageInfo->in.m_tintStatus );
				
					// Custom feature, similar to Black Hole Armor
					if(damageInfo->in.m_customDamageStatusType == "SHIELDED_TARGET")
						getObject()->setShieldByTargetID(damageInfo->in.m_sourceID, damageInfo->in.m_protectionTypes);
				}
			}

			SubdualCustomNotifyData subdualNotifyData;
			subdualNotifyData.damage = Subdualamount;
			subdualNotifyData.tintStatus = subdualData.tintStatus;
			subdualNotifyData.customTintStatus = subdualData.customTintStatus;
			subdualNotifyData.disableType = subdualData.disableType;
			subdualNotifyData.hasDisable = damageInfo->in.m_customSubdualHasDisable;
			subdualNotifyData.removeTintOnDisable = damageInfo->in.m_customSubdualRemoveSubdualTintOnDisable;
			subdualNotifyData.isSubdued = nowSubdued;
			subdualNotifyData.clearOnTrigger = damageInfo->in.m_customSubdualClearOnTrigger;
			subdualNotifyData.disableTint = damageInfo->in.m_customSubdualDisableTint;
			subdualNotifyData.disableCustomTint = damageInfo->in.m_customSubdualDisableCustomTint;

			getObject()->notifySubdualDamageCustom(subdualNotifyData, damageInfo->in.m_subdualCustomType );
		}
	}
	// Do Default Subdual Behavior
	else if( ( IsSubdualDamage(damageInfo->in.m_damageType) && damageInfo->in.m_customDamageType.isEmpty() ) || damageInfo->in.m_isSubdual == TRUE )
	{
		if( !canBeSubdued() || obj->isAnyKindOf(damageInfo->in.m_subdualForbiddenKindOf) )
		{
			if( IsSubdualDamage(damageInfo->in.m_damageType) && damageInfo->in.m_customDamageType.isEmpty() )
				return;
		}
		else 
		{
			Real Subdualamount = amount;
			if(damageInfo->in.m_subdualDamageMultiplier != 1.0f)
			{
				Subdualamount *= damageInfo->in.m_subdualDamageMultiplier;
			}
			if ( ( damageInfo->in.m_damageType != DAMAGE_SUBDUAL_UNRESISTABLE && damageInfo->in.m_damageType != DAMAGE_UNRESISTABLE ) || !damageInfo->in.m_customDamageType.isEmpty() ) {
				Subdualamount *= m_damageScalar;
			}
			
			Bool wasSubdued = isSubdued();
			internalAddSubdualDamage(Subdualamount);
			Bool nowSubdued = isSubdued();

			if(damageInfo->in.m_subdualDealsNormalDamage == FALSE)
			{
				alreadyHandled = TRUE;
				allowModifier = FALSE;
			}

			if( wasSubdued != nowSubdued )
			{
				if(damageInfo->in.m_isMissileAttractor)
					onSubdualChangeAttractor(nowSubdued, damageInfo->in.m_sourceID);
				else
					onSubdualChange(nowSubdued, damageInfo->in.m_subduedProjectileNoDamage);
			}

			getObject()->notifySubdualDamage(Subdualamount);
		}
	}

	if (allowModifier)
	{
		if( damageInfo->in.m_damageType != DAMAGE_UNRESISTABLE )
		{
			// Apply the damage scalar (extra bonuses -- like strategy center defensive battle plan)
			// And remember not to adjust unresistable damage, just like the armor code can't.
			amount *= m_damageScalar;
		}
	}

	// sanity check the damage value -- can't apply negative damage
	if( amount > 0.0f || damageInfo->in.m_kill )
	{
		BodyDamageType oldState = m_curDamageState;

		//If the object is going to die, make sure we damage all remaining health.
		if( damageInfo->in.m_kill )
		{
			amount = m_currentHealth;
		}

		if (!alreadyHandled)
		{
			// do the damage simplistic damage subtraction
			internalChangeHealth( -amount, adjustConditions);
		}

#ifdef ALLOW_SURRENDER
//*****************************************************************************************
//*****************************************************************************************
//THIS CODE HAS BEEN DISABLED FOR THE MULTIPLAYER PLAY TEST!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!**
//*****************************************************************************************
//		// if we were "killed" by surrender damage...
//		if (damageInfo->in.m_damageType == DAMAGE_SURRENDER && m_currentHealth <= 0.0f && obj->isKindOf(KINDOF_CAN_SURRENDER))
//		{
//			AIUpdateInterface* ai = obj->getAIUpdateInterface();
//			if (ai)
//			{
//				// do no damage, but make it surrender instead.
//				m_currentHealth = m_prevHealth;
//				const Object* killer = TheGameLogic->findObjectByID( damageInfo->in.m_sourceID );
//				ai->setSurrendered(killer, true);
//				return;
//			}
//		}
//*****************************************************************************************
//*****************************************************************************************
#endif

		// record the actual damage done from this, and when it happened
		damageInfo->out.m_actualDamageDealt = amount;
		damageInfo->out.m_actualDamageClipped = m_prevHealth - m_currentHealth;

		// then copy the whole DamageInfo struct for easy lookup
		// (object pointer loses scope as soon as atteptdamage's caller ends)
		// m_lastDamageTimestamp is initialized to FFFFFFFFFF, so doing a < compare is problematic.
		// jba.
		if (m_lastDamageTimestamp!=TheGameLogic->getFrame() && m_lastDamageTimestamp != TheGameLogic->getFrame()-1) {
			m_lastDamageInfo = *damageInfo;
			m_lastDamageCleared = false;
			m_lastDamageTimestamp = TheGameLogic->getFrame();
		} else {
			// Multiple damages applied in one/next frame.  We prefer the one that tells who the attacker is.
			Object *srcObj1 = TheGameLogic->findObjectByID(m_lastDamageInfo.in.m_sourceID);
			Object *srcObj2 = TheGameLogic->findObjectByID(damageInfo->in.m_sourceID);
			if (srcObj2) {
				if (srcObj1) {
					if (srcObj2->isKindOf(KINDOF_VEHICLE) || srcObj2->isKindOf(KINDOF_INFANTRY) ||
						srcObj2->isFactionStructure()) {
							m_lastDamageInfo = *damageInfo;
							m_lastDamageCleared = false;
							m_lastDamageTimestamp = TheGameLogic->getFrame();
						}
				} else {
					m_lastDamageInfo = *damageInfo;
					m_lastDamageCleared = false;
					m_lastDamageTimestamp = TheGameLogic->getFrame();
				}

			}	else {
				// no change.
			}
		}

		// Notify the player that they have been attacked by this player
		if (m_lastDamageInfo.in.m_sourceID != INVALID_ID)
		{
			Object *srcObj = TheGameLogic->findObjectByID(m_lastDamageInfo.in.m_sourceID);
			if (srcObj)
			{
				Player *srcPlayer = srcObj->getControllingPlayer();
				obj->getControllingPlayer()->setAttackedBy(srcPlayer->getPlayerIndex());
			}
		}

		// if our health has gone down then do run the damage module callback
		if( m_currentHealth < m_prevHealth && doDamageModules)
		{
			for (BehaviorModule** m = obj->getBehaviorModules(); *m; ++m)
			{
				DamageModuleInterface* d = (*m)->getDamage();
				if (!d)
					continue;

				d->onDamage( damageInfo );
			}
		}

		if (m_curDamageState != oldState && adjustConditions)
		{
			for (BehaviorModule** m = obj->getBehaviorModules(); *m; ++m)
			{
				DamageModuleInterface* d = (*m)->getDamage();
				if (!d)
					continue;

				d->onBodyDamageStateChange( damageInfo, oldState, m_curDamageState );
			}

			// @todo: This really feels like it should be in the TransitionFX lists.
			if (m_curDamageState == BODY_DAMAGED)
			{
				AudioEventRTS damaged = *obj->getTemplate()->getSoundOnDamaged();
				damaged.setObjectID(obj->getID());
				TheAudio->addAudioEvent(&damaged);
			}
			else if (m_curDamageState == BODY_REALLYDAMAGED)
			{
				AudioEventRTS reallyDamaged = *obj->getTemplate()->getSoundOnReallyDamaged();
				reallyDamaged.setObjectID(obj->getID());
				TheAudio->addAudioEvent(&reallyDamaged);
			}

		}

		// Should we play our fear sound?
		if( (m_prevHealth / m_maxHealth) > YELLOW_DAMAGE_PERCENT &&
				(m_currentHealth / m_maxHealth) < YELLOW_DAMAGE_PERCENT &&
				(m_currentHealth > 0) )
		{
			// 25% chance to play
			if (GameLogicRandomValue(0, 99) < 25)
			{
				AudioEventRTS fearSound = *obj->getTemplate()->getVoiceFear();
				fearSound.setPosition( obj->getPosition() );
				fearSound.setPlayerIndex( obj->getControllingPlayer()->getPlayerIndex() );
				TheAudio->addAudioEvent(&fearSound);
			}
		}

		// check to see if we died
		if( m_currentHealth <= 0 && m_prevHealth > 0 )
		{
			// Give our killer credit for killing us, if there is one.
			if( damager )
			{
				damager->scoreTheKill( obj );
			}

			obj->doHijackerUpdate(TRUE, FALSE, damageInfo->in.m_clearsParasite, damageInfo->in.m_sourceID );
			obj->onDie( damageInfo );

			isKilled = TRUE;
		}
	}

	if( !isKilled )
	{
		if(m_currentHealth < m_prevHealth)
			obj->doAssaultTransportHealthUpdate();
		obj->doHijackerUpdate(FALSE, FALSE, damageInfo->in.m_clearsParasite, damageInfo->in.m_sourceID );
	}

	doDamageFX(damageInfo);

	// Damaged repulsable civilians scare (repulse) other civs.	jba.
	if( TheAI->getAiData()->m_enableRepulsors )
	{
		if( obj->isKindOf( KINDOF_CAN_BE_REPULSED ) )
		{
			obj->setStatus( MAKE_OBJECT_STATUS_MASK( OBJECT_STATUS_REPULSOR ) );
		}
	}

	//Retaliate, even if I'm dead -- we'll still get my nearby friends to get revenge!!!
	//Also only retaliate if we're controlled by a human player and the thing that attacked me
	//is an enemy.
	Player *controllingPlayer = obj->getControllingPlayer();
	if( controllingPlayer && controllingPlayer->isLogicalRetaliationModeEnabled() && controllingPlayer->getPlayerType() == PLAYER_HUMAN )
	{
		if( shouldRetaliateAgainstAggressor(obj, damager))
		{
			PartitionFilterPlayerAffiliation f1( controllingPlayer, ALLOW_ALLIES, true );
			PartitionFilterOnMap filterMapStatus;
			PartitionFilter *filters[] = { &f1, &filterMapStatus, 0 };


			Real distance = TheAI->getAiData()->m_retaliateFriendsRadius + obj->getGeometryInfo().getBoundingCircleRadius();
			SimpleObjectIterator *iter = ThePartitionManager->iterateObjectsInRange( obj->getPosition(), distance, FROM_CENTER_2D, filters, ITER_FASTEST );
			MemoryPoolObjectHolder hold( iter );
			for( Object *them = iter->first(); them; them = iter->next() )
			{
				if (!shouldRetaliate(them)) {
					continue;
				}
				AIUpdateInterface *ai = them->getAI();
				if (ai==NULL) {
					continue;
				}
				//If we have AI and we're mobile, then assist!
				if( !them->isKindOf( KINDOF_IMMOBILE ) && !them->testStatus( OBJECT_STATUS_IMMOBILE) )
				{
					//But only if we can attack it!
					CanAttackResult result = them->getAbleToAttackSpecificObject( ATTACK_NEW_TARGET, damager, CMD_FROM_AI );
					if( result == ATTACKRESULT_POSSIBLE_AFTER_MOVING || result == ATTACKRESULT_POSSIBLE )
					{
						ai->aiGuardRetaliate( damager, them->getPosition(), NO_MAX_SHOTS_LIMIT, CMD_FROM_AI );
					}
				}
			}
		}
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
Bool ActiveBody::shouldRetaliateAgainstAggressor(Object *obj, Object *damager)
{
	/* This considers whether obj should invoke his friends to retaliate against damager.
		 Note that obj could be a structure, so we don't actually check whether obj will
		 retaliate, as in many cases he wouldn't. */
	if (damager==NULL) {
		return false;
	}
	if (damager->isAirborneTarget()) {
		return false; // Don't retaliate against aircraft. [8/25/2003]
	}
	if (damager->getRelationship( obj ) != ENEMIES) {
		return false; // only retaliate against enemies.
	}
	Real distSqr = ThePartitionManager->getDistanceSquared(obj, damager, FROM_BOUNDINGSPHERE_2D);
	if (distSqr > sqr(TheAI->getAiData()->m_maxRetaliateDistance)) {
		return false;
	}
	// Only human players retaliate. [8/25/2003]
	if (obj->getControllingPlayer()->getPlayerType() != PLAYER_HUMAN) {
		return false;
	}
	// Drones never retaliate. [8/25/2003]
	if (obj->isKindOf(KINDOF_DRONE)) {
		return false;
	}
	return true;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
Bool ActiveBody::shouldRetaliate(Object *obj)
{
	// Cannot retaliate objects dont. [8/25/2003]
	if (obj->isKindOf(KINDOF_CANNOT_RETALIATE)) {
		return false;
	}
	if (obj->isKindOf( KINDOF_IMMOBILE ) || obj->testStatus( OBJECT_STATUS_IMMOBILE)) {
		return false;
	}
	// Drones never retaliate [8/25/2003] except when they do [2025/09/07]
	if (obj->isKindOf(KINDOF_DRONE) && !obj->isKindOf(KINDOF_CAN_RETALIATE)) {
		return false;
	}
	// Any unit that isn't idle won't retaliate. [8/25/2003]
	if (obj->getAI()) {
		if (!obj->getAI()->isIdle()) {
			return false;
		}
	} else {
		return false; // Non-ai can't retaliate. [8/26/2003]
	}
	// Stealthed units don't retaliate unless they're detected. [8/25/2003]
	if ( obj->getStatusBits().test( OBJECT_STATUS_STEALTHED ) &&
		!obj->getStatusBits().test( OBJECT_STATUS_DETECTED ) ) {
		return false;
	}
	// If we're using an ability, don't stop. [8/25/2003]
	if (obj->testStatus(OBJECT_STATUS_IS_USING_ABILITY)) {
		return false;
	}
	return true;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void ActiveBody::attemptHealing( DamageInfo *damageInfo )
{
	validateArmorAndDamageFX();

	// sanity
	if( damageInfo == NULL )
		return;

	if( damageInfo->in.m_damageType != DAMAGE_HEALING )
	{
		// Healing and Damage are separate, so this shouldn't happen
		attemptDamage( damageInfo );
		return;
	}

	Object* obj = getObject();

	// srj sez: sorry, once yer dead, yer dead.
	// Special case for bridges, cause the system now things they're dead
	///@todo we need to figure out what has changed so we don't have to hack this (CBD 11-1-2002)
	if( obj->isKindOf( KINDOF_BRIDGE ) == FALSE &&
			obj->isKindOf( KINDOF_BRIDGE_TOWER ) == FALSE &&
			obj->isEffectivelyDead())
		return;

	// initialize these, just in case we bail out early
	damageInfo->out.m_actualDamageDealt = 0.0f;
	damageInfo->out.m_actualDamageClipped = 0.0f;

	Real amount = m_curArmor.adjustDamage(damageInfo->in.m_damageType, damageInfo->in.m_amount, damageInfo->in.m_customDamageType);

	// sanity check the damage value -- can't apply negative healing
	if( amount > 0.0f )
	{
		BodyDamageType oldState = m_curDamageState;

		// do the damage simplistic damage ADDITION
		internalChangeHealth( amount );

		// record the actual damage done from this, and when it happened
		damageInfo->out.m_actualDamageDealt = amount;
		damageInfo->out.m_actualDamageClipped = m_prevHealth - m_currentHealth;

		//then copy the whole DamageInfo struct for easy lookup
		//(object pointer loses scope as soon as atteptdamage's caller ends)
		m_lastDamageInfo = *damageInfo;
		m_lastDamageCleared = false;
		m_lastDamageTimestamp = TheGameLogic->getFrame();
		m_lastHealingTimestamp = TheGameLogic->getFrame();

		// if our health has gone UP then do run the damage module callback
		if( m_currentHealth > m_prevHealth )
		{
			for (BehaviorModule** m = obj->getBehaviorModules(); *m; ++m)
			{
				DamageModuleInterface* d = (*m)->getDamage();
				if (!d)
					continue;

				d->onHealing( damageInfo );
			}

			obj->doAssaultTransportHealthUpdate();
			obj->doHijackerUpdate(FALSE, TRUE, damageInfo->in.m_clearsParasite, damageInfo->in.m_sourceID);
		}

		if (m_curDamageState != oldState)
		{
			for (BehaviorModule** m = obj->getBehaviorModules(); *m; ++m)
			{
				DamageModuleInterface* d = (*m)->getDamage();
				if (!d)
					continue;

				d->onBodyDamageStateChange( damageInfo, oldState, m_curDamageState );
			}
		}
	}

	doDamageFX(damageInfo);
}

//-------------------------------------------------------------------------------------------------
/** Simple setting of the health value, it does *NOT* track any transition
	* states for the event of "damage" or the event of "death".  */
//-------------------------------------------------------------------------------------------------
void ActiveBody::setInitialHealth(Int initialPercent)
{

	// save the current health as the previous health
	m_prevHealth = m_currentHealth;

	Real factor = initialPercent/100.0f;
	Real newHealth = factor * m_initialHealth;

	// change the health to the requested percentage.
	internalChangeHealth(newHealth - m_currentHealth);

}

//-------------------------------------------------------------------------------------------------
/** Simple setting of the health value, it does *NOT* track any transition
	* states for the event of "damage" or the event of "death".  */
//-------------------------------------------------------------------------------------------------
void ActiveBody::setMaxHealth( Real maxHealth, MaxHealthChangeType healthChangeType )
{
	Real prevMaxHealth = m_maxHealth;
	m_maxHealth = maxHealth;
	m_initialHealth = maxHealth;

	switch( healthChangeType )
	{
		case PRESERVE_RATIO:
		{
			//400/500 (80%) + 100 becomes 480/600 (80%)
			//200/500 (40%) - 100 becomes 160/400 (40%)
			Real ratio = m_currentHealth / prevMaxHealth;
			Real newHealth = maxHealth * ratio;
			internalChangeHealth( newHealth - m_currentHealth );
			break;
		}
		case ADD_CURRENT_HEALTH_TOO:
		{
			//Add the same amount that we are adding to the max health.
			//This could kill you if max health is reduced (if we ever have that ability to add buffer health like in D&D)
			//400/500 (80%) + 100 becomes 500/600 (83%)
			//200/500 (40%) - 100 becomes 100/400 (25%)
			internalChangeHealth( maxHealth - prevMaxHealth );
			break;
		}
		case SAME_CURRENTHEALTH:
			//do nothing
			break;

		case FULLY_HEAL:
		{
			// Set current to the new Max.
			//400/500 (80%) + 100 becomes 600/600 (100%)
			//200/500 (40%) - 100 becomes 400/400 (100%)
			internalChangeHealth(m_maxHealth - m_currentHealth);
			break;
		}
	}

	//
	// when max health is getting clipped to a lower value, if our current health
	// value is now outside of the max health range we will set it back down to the
	// new cap.  Note that we are *NOT* going through any healing or damage methods here
	// and are doing a direct set
	//
	if( m_currentHealth > maxHealth )
	{
		internalChangeHealth( maxHealth - m_currentHealth );
	}

}

// ------------------------------------------------------------------------------------------------
/** Given the current damage state of the object, evaluate the visual model conditions
	* that have a visual impact on the object */
// ------------------------------------------------------------------------------------------------
void ActiveBody::evaluateVisualCondition()
{

	Drawable* draw = getObject()->getDrawable();
	if (draw)
	{
		draw->reactToBodyDamageStateChange(m_curDamageState);
	}

	//
	// destroy any particle systems that were attached to our body for the old state
	// and create new particle systems for the new state
	//
	updateBodyParticleSystems();

}

// ------------------------------------------------------------------------------------------------
/** Create up to maxSystems particle systems of type particleSystemName and attach to bones
	* specified by the bone base name.  If there are more bones than maxSystems then the
	* bones will be randomly selected */
// ------------------------------------------------------------------------------------------------
void ActiveBody::createParticleSystems( const AsciiString &boneBaseName,
																				const ParticleSystemTemplate *systemTemplate,
																				Int maxSystems )
{
	Object *us = getObject();

	// sanity
	if( systemTemplate == NULL )
		return;

	// get the bones
	enum { MAX_BONES = 16 };
	Coord3D bonePositions[ MAX_BONES ];
	Int numBones = us->getMultiLogicalBonePosition( boneBaseName.str(),
																									MAX_BONES,
																									bonePositions,
																									NULL,
																									FALSE );

	// if no bones found nothing else to do
	if( numBones == 0 )
		return;

	//
	// if we don't have enough bones to go up to maxSystems, we will change maxSystems to be
	// the number of bones we actually have (we don't want systems doubling up on bones)
	//
	if( numBones < maxSystems )
		maxSystems = numBones;

	//
	// create an array that we'll use to mark which bone positions have already been used,
	// this is necessary when we have more bones than particle systems we're going to
	// create, in which case we place the particle systems at random bone locations
	// but don't want to repeat any
	//
	Bool usedBoneIndices[ MAX_BONES ] = { FALSE };

	// create the particle systems
	const Coord3D *pos;
	for( Int i = 0; i < maxSystems; ++i )
	{

		// pick a bone index to place this particle system at
		// MDC: moving to GameLogicRandomValue.  This does not need to be synced, but having it so makes searches *so* much nicer.
		// DTEH: Moved back to GameClientRandomValue because of desync problems. July 27th 2003.
		Int boneIndex = GameClientRandomValue( 0, maxSystems - i - 1 );

		// find the actual bone location to use and mark that bone index as used
		Int count = 0;
		Int j = 0;
		for( ; j < numBones; j++ )
		{

			// ignore bone positions that have already been used
			if( usedBoneIndices[ j ] == TRUE )
				continue;

			// this spot is available, if count == boneIndex then use this index
			if( count == boneIndex )
			{

				pos = &bonePositions[ j ];
				usedBoneIndices[ j ] = TRUE;
				break;  // exit for j

			}  // end if
			else
			{

				// we won't use this index, increment count until we find a suitable index to use
				++count;

			}  // end else

		}  // end for, j

		// sanity
		DEBUG_ASSERTCRASH( j != numBones,
											 ("ActiveBody::createParticleSystems, Unable to select particle system index") );

		// create particle system here
		ParticleSystem *particleSystem = TheParticleSystemManager->createParticleSystem( systemTemplate );
		if( particleSystem )
		{

			// set the position of the particle system in local object space
			particleSystem->setPosition( pos );

			// attach particle system to object
			particleSystem->attachToObject( us );

			// create a new body particle system entry and keep this particle system in it
			BodyParticleSystem *newEntry = newInstance(BodyParticleSystem);
			newEntry->m_particleSystemID = particleSystem->getSystemID();
			newEntry->m_next = m_particleSystems;
			m_particleSystems = newEntry;

		}  // end if

	}  // end for, i

}  // end createParticleSystems

// ------------------------------------------------------------------------------------------------
/** Delete all the body particle systems */
// ------------------------------------------------------------------------------------------------
void ActiveBody::deleteAllParticleSystems( void )
{
	BodyParticleSystem *nextBodySystem;
	ParticleSystem *particleSystem;

	while( m_particleSystems )
	{

		// get this particle system
		particleSystem = TheParticleSystemManager->findParticleSystem( m_particleSystems->m_particleSystemID );
		if( particleSystem )
			particleSystem->destroy();

		// get next system in the body
		nextBodySystem = m_particleSystems->m_next;

		// destroy this entry
		deleteInstance(m_particleSystems);

		// set the body systems head to the next
		m_particleSystems = nextBodySystem;

	}  // end while

}  // end deleteAllParticleSystems

// ------------------------------------------------------------------------------------------------
/* 	This function is called on state changes only.  Body Type or Aflameness. */
// ------------------------------------------------------------------------------------------------
void ActiveBody::updateBodyParticleSystems( void )
{
	static const ParticleSystemTemplate *fireSmallTemplate   = TheParticleSystemManager->findTemplate( TheGlobalData->m_autoFireParticleSmallSystem );
	static const ParticleSystemTemplate *fireMediumTemplate  = TheParticleSystemManager->findTemplate( TheGlobalData->m_autoFireParticleMediumSystem );
	static const ParticleSystemTemplate *fireLargeTemplate   = TheParticleSystemManager->findTemplate( TheGlobalData->m_autoFireParticleLargeSystem );
	static const ParticleSystemTemplate *smokeSmallTemplate  = TheParticleSystemManager->findTemplate( TheGlobalData->m_autoSmokeParticleSmallSystem );
	static const ParticleSystemTemplate *smokeMediumTemplate = TheParticleSystemManager->findTemplate( TheGlobalData->m_autoSmokeParticleMediumSystem );
	static const ParticleSystemTemplate *smokeLargeTemplate  = TheParticleSystemManager->findTemplate( TheGlobalData->m_autoSmokeParticleLargeSystem );
	static const ParticleSystemTemplate *aflameTemplate			 = TheParticleSystemManager->findTemplate( TheGlobalData->m_autoAflameParticleSystem );
	Int countModifier;
	const ParticleSystemTemplate *fireSmall;
	const ParticleSystemTemplate *fireMedium;
	const ParticleSystemTemplate *fireLarge;
	const ParticleSystemTemplate *smokeSmall;
	const ParticleSystemTemplate *smokeMedium;
	const ParticleSystemTemplate *smokeLarge;

	//
	// when we're aflame, we use a slightly different set of particle systems that are
	// auto created that lends itself to more fire and bigger fire
	//
	if( getObject()->testStatus( OBJECT_STATUS_AFLAME ) )
	{

		fireSmall		= fireMediumTemplate;		// small fire becomes medium fire
		fireMedium	= fireLargeTemplate;		// medium fire becomes large fire
		fireLarge		= fireLargeTemplate;		// large fire stays large
		smokeSmall	= fireSmallTemplate;		// small smoke becomes small fire
		smokeMedium = fireSmallTemplate;		// medium smoke becomes small fire
		smokeLarge	= fireSmallTemplate;		// large smoke becomes small fire

		// we get to make more of them all too
		countModifier = 2;

	}  // end if
	else
	{

		// use regular templates
		fireSmall		= fireSmallTemplate;
		fireMedium	= fireMediumTemplate;
		fireLarge		= fireLargeTemplate;
		smokeSmall	= smokeSmallTemplate;
		smokeMedium = smokeMediumTemplate;
		smokeLarge	= smokeLargeTemplate;

		// we make just the normal amount of these
		countModifier = 1;

	}  // end else

	//
	// remove any particle systems we have currently in the list in favor of any new ones
	// that we're going to autopopulate ourselves with
	//
	deleteAllParticleSystems();

	//
	// create particle systems for the new body state
	//

	// small fire bones
	createParticleSystems( TheGlobalData->m_autoFireParticleSmallPrefix,
												 fireSmall, TheGlobalData->m_autoFireParticleSmallMax * countModifier );

	// medium fire bones
	createParticleSystems( TheGlobalData->m_autoFireParticleMediumPrefix,
												 fireMedium, TheGlobalData->m_autoFireParticleMediumMax * countModifier );

	// large fire bones
	createParticleSystems( TheGlobalData->m_autoFireParticleLargePrefix,
												 fireLarge, TheGlobalData->m_autoFireParticleLargeMax * countModifier );

	// small smoke bones
	createParticleSystems( TheGlobalData->m_autoSmokeParticleSmallPrefix,
												 smokeSmall, TheGlobalData->m_autoSmokeParticleSmallMax * countModifier );

	// medium smoke bones
	createParticleSystems( TheGlobalData->m_autoSmokeParticleMediumPrefix,
												 smokeMedium, TheGlobalData->m_autoSmokeParticleMediumMax * countModifier );

	// large smoke bones
	createParticleSystems( TheGlobalData->m_autoSmokeParticleLargePrefix,
												 smokeLarge, TheGlobalData->m_autoSmokeParticleLargeMax * countModifier );

	// actively on fire
	if( getObject()->testStatus( OBJECT_STATUS_AFLAME ) )
		createParticleSystems( TheGlobalData->m_autoAflameParticlePrefix,
													 aflameTemplate, TheGlobalData->m_autoAflameParticleMax * countModifier );

}  // end updatebodyParticleSystems

//-------------------------------------------------------------------------------------------------
/** Simple changing of the health value, it does *NOT* track any transition
	* states for the event of "damage" or the event of "death".  If you
	* with to kill an object and give these modules a chance to react
	* to that event use the proper damage method calls.
	* No game logic should go in here.  This is the low level math and flag maintenance.
	* Game stuff goes in attemptDamage and attemptHealing.
*/
//-------------------------------------------------------------------------------------------------
void ActiveBody::internalChangeHealth( Real delta, Bool changeModelCondition)
{
	// save the current health as the previous health
	m_prevHealth = m_currentHealth;

	// change the health by the delta, it can be positive or negative
	m_currentHealth += delta;

	// high end cap
	Real maxHealth = m_maxHealth;
	if( m_currentHealth > maxHealth )
		m_currentHealth = maxHealth;

	// low end cap
	const Real lowEndCap = 0.0f;  // low end cap for health, don't go below this
	if( m_currentHealth < lowEndCap )
		m_currentHealth = lowEndCap;

	if (changeModelCondition) {
		// recalc the damage state
		BodyDamageType oldState = m_curDamageState;
		setCorrectDamageState();

		// if our state has changed
		if (m_curDamageState != oldState)
		{

			//
			// show a visual change in the model for the damage state, we do not show visual changes
			// for damage states when things are under construction because we just don't have
			// all the art states for that during buildup animation
			//
			if (!getObject()->getStatusBits().test(OBJECT_STATUS_UNDER_CONSTRUCTION))
				evaluateVisualCondition();

		}  // end if
	}

	// mark the bit according to our health. (if our AI is dead but our health improves, it will
	// still re-flag this bit in the AIDeadState every frame.)
	getObject()->setEffectivelyDead(m_currentHealth <= 0);

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void ActiveBody::internalAddSubdualDamage( Real delta, Bool isHealing )
{
	//const ActiveBodyModuleData *data = getActiveBodyModuleData();
	if(isHealing)
	{
		m_currentSubdualDamage -= delta;
		m_currentSubdualDamage = max(0.0f, m_currentSubdualDamage);

		// The function of this feature is to Fix Tint Color Correction Bug, which is why it is assigned after the calculation.
		if(isSubdued())
			m_hasBeenSubdued = TRUE;
		
		Bool clearLater;
		
		if( getObject()->getDrawable() )
		{
			if( getObject()->getDrawable()->testTintStatus(TINT_STATUS_GAINING_SUBDUAL_DAMAGE) && m_hasBeenSubdued == FALSE )
				clearLater = TRUE;
			getObject()->getDrawable()->clearTintStatus(TINT_STATUS_GAINING_SUBDUAL_DAMAGE);
		}

		if( !getObject()->isKindOf(KINDOF_PROJECTILE) && !m_clearedSubdued)
		{
			onSubdualChange(isSubdued(), FALSE, clearLater);
		}

		//DEBUG_LOG(( "Subdual Healed, Current Subdual Damage: %f", m_currentSubdualDamage ));
	}
	else
	{
		m_currentSubdualDamage += delta;
		m_currentSubdualDamage = min(m_currentSubdualDamage, m_subdualDamageCap);

		if(!isSubdued())
			m_hasBeenSubdued = FALSE;
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void ActiveBody::internalAddChronoDamage(Real delta)
{
	// Just increment, we don't need a cap. we kill once maxHealth is reached
	//Real chronoDamageCap = m_maxHealth * 2.0;
    m_currentChronoDamage += delta;
	//m_currentChronoDamage = min(m_currentChronoDamage, chronoDamageCap);
}

void ActiveBody::setSubdualCap( Real subdualCap )
{
	m_subdualDamageCap = subdualCap;
	if(!m_subdualDamageCapCustom.empty())
	{
		Real ratio = subdualCap;
		ratio /= m_subdualDamageCap;
		for (CustomSubdualDamageMap::iterator it = m_subdualDamageCapCustom.begin(); it != m_subdualDamageCapCustom.end(); ++it )
			it->second *= ratio;
	}
} 

void ActiveBody::setSubdualHealRate( UnsignedInt subdualHealRate )
{
	m_subdualDamageHealRate = subdualHealRate;
	if(!m_subdualDamageHealRateCustom.empty())
	{
		Real ratio = subdualHealRate;
		ratio /= m_subdualDamageHealRate;
		for (CustomSubdualHealRateMap::iterator it = m_subdualDamageHealRateCustom.begin(); it != m_subdualDamageHealRateCustom.end(); ++it )
			it->second *= ratio;
	}
} 

void ActiveBody::setSubdualHealAmount( Real subdualHealAmount )
{
	m_subdualDamageHealAmount = subdualHealAmount;
	if(!m_subdualDamageHealAmountCustom.empty())
	{
		Real ratio = subdualHealAmount;
		ratio /= m_subdualDamageHealAmount;
		for (CustomSubdualDamageMap::iterator it = m_subdualDamageHealAmountCustom.begin(); it != m_subdualDamageHealAmountCustom.end(); ++it )
			it->second *= ratio;
	}
} 

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
Bool ActiveBody::canBeSubdued() const
{
	// Any body with subdue listings can be subdued.
	return m_subdualDamageCap > 0;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void ActiveBody::onSubdualChange( Bool isNowSubdued, Bool subduedProjectileNoDamage, Bool clearTintLater )
{
	if( !getObject()->isKindOf(KINDOF_PROJECTILE) )
	{
		Object *me = getObject();
		
		if( isNowSubdued )
		{
			m_clearedSubdued = FALSE;
			
			me->setDisabled(DISABLED_SUBDUED);

      ContainModuleInterface *contain = me->getContain();
      if ( contain )
        contain->orderAllPassengersToIdle( CMD_FROM_AI );

		}
		else
		{
			m_clearedSubdued = TRUE;
			
			me->clearDisabled(DISABLED_SUBDUED, clearTintLater);

			if( me->isKindOf( KINDOF_FS_INTERNET_CENTER ) )
			{
				//Kris: October 20, 2003 - Patch 1.01
				//Any unit inside an internet center is a hacker! Order
				//them to start hacking again.
				ContainModuleInterface *contain = me->getContain();
				if ( contain )
					contain->orderAllPassengersToHackInternet( CMD_FROM_AI );
			}
		}
	}
	else if( isNowSubdued )// There is no coming back from being jammed, and projectiles can't even heal, but this makes it clear.
	{
		ProjectileUpdateInterface *pui = getObject()->getProjectileUpdateInterface();
		if( pui )
		{
			pui->projectileNowJammed(subduedProjectileNoDamage);
		}
	}
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void ActiveBody::onSubdualChangeAttractor( Bool isNowSubdued, ObjectID attractorID )
{
	if( !getObject()->isKindOf(KINDOF_PROJECTILE) )
	{
		Object *me = getObject();

		if( isNowSubdued )
		{
			m_clearedSubdued = FALSE;
			
			me->setDisabled(DISABLED_SUBDUED);

      ContainModuleInterface *contain = me->getContain();
      if ( contain )
        contain->orderAllPassengersToIdle( CMD_FROM_AI );

		}
		else
		{
			m_clearedSubdued = TRUE;
			
			me->clearDisabled(DISABLED_SUBDUED);

			if( me->isKindOf( KINDOF_FS_INTERNET_CENTER ) )
			{
				//Kris: October 20, 2003 - Patch 1.01
				//Any unit inside an internet center is a hacker! Order
				//them to start hacking again.
				ContainModuleInterface *contain = me->getContain();
				if ( contain )
					contain->orderAllPassengersToHackInternet( CMD_FROM_AI );
			}
		}
	}
	else if( isNowSubdued )// There is no coming back from being jammed, and projectiles can't even heal, but this makes it clear.
	{
		ProjectileUpdateInterface *pui = getObject()->getProjectileUpdateInterface();
		if( pui )
		{
			pui->projectileNowDrawn(attractorID);
		}
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
Bool ActiveBody::canBeSubduedCustom(const AsciiString &customStatus) const
{
	CustomSubdualDamageMap::const_iterator it = m_subdualDamageCapCustom.find(customStatus);
	if(it != m_subdualDamageCapCustom.end())
		return it->second > 0;
	// Any body with subdue listings can be subdued.
	return m_subdualDamageCap > 0;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
Bool ActiveBody::isSubduedCustom(const AsciiString &customStatus) const
{
	CustomSubdualCurrentDamageMap::const_iterator it = m_currentSubdualDamageCustom.find(customStatus);
	if(it != m_currentSubdualDamageCustom.end())
		return m_maxHealth <= it->second.damage;
	return FALSE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
Bool ActiveBody::isNearSubduedRangeCustom( Real low, Real high, const AsciiString &customStatus ) const
{
	CustomSubdualCurrentDamageMap::const_iterator it = m_currentSubdualDamageCustom.find(customStatus);
	if(it != m_currentSubdualDamageCustom.end())
		return m_maxHealth > it->second.damage - low || m_maxHealth <= it->second.damage + high;
	return FALSE;
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void ActiveBody::internalAddSubdualDamageCustom( SubdualCustomData delta, const AsciiString &customStatus, Bool isHealing )
{
	CustomSubdualCurrentDamageMap::iterator it = m_currentSubdualDamageCustom.find(customStatus);
	if(it != m_currentSubdualDamageCustom.end())
	{
		if(isHealing)
		{
			it->second.damage -= delta.damage;
			it->second.damage = max(0.0f, it->second.damage);

			// The function of this feature is to Fix Tint Color Correction Bug, which is why it is assigned after the calculation.
			//if(isSubduedCustom(customStatus))
			if(m_maxHealth <= it->second.damage) // Yes.
			{
				it->second.isSubdued = TRUE;
			}

			Bool clearLater;

			if(getObject()->getDrawable() )
			{
				if(!it->second.customTintStatus.isEmpty())
				{
					if( getObject()->getDrawable()->testCustomTintStatus(it->second.customTintStatus) && it->second.isSubdued == FALSE )
						clearLater = TRUE;
					getObject()->getDrawable()->clearCustomTintStatus(it->second.customTintStatus);
				}
				else if(it->second.tintStatus > TINT_STATUS_INVALID && it->second.tintStatus < TINT_STATUS_COUNT)
				{
					if( getObject()->getDrawable()->testTintStatus(it->second.tintStatus) && it->second.isSubdued == FALSE )
						clearLater = TRUE;
					getObject()->getDrawable()->clearTintStatus(it->second.tintStatus);
				}
			}
			
			if( !getObject()->isKindOf(KINDOF_PROJECTILE) && !m_clearedSubduedCustom && m_maxHealth > it->second.damage )
			{
				onSubdualRemovalCustom(it->second.disableType, clearLater);
			}
		}
		else
		{
			it->second.damage += delta.damage;
			it->second.tintStatus = delta.tintStatus;
			it->second.customTintStatus = delta.customTintStatus;
			it->second.disableType = delta.disableType;
		}
	}
	else
	{
		m_currentSubdualDamageCustom[customStatus] = delta;
	}

	if(!isHealing)
	{
		CustomSubdualDamageMap::iterator it2 = m_subdualDamageCapCustom.find(customStatus);
		if(it2 != m_subdualDamageCapCustom.end())
		{
			m_currentSubdualDamageCustom[customStatus].damage = min(m_currentSubdualDamageCustom[customStatus].damage, it2->second);
		}
		else
		{
			m_currentSubdualDamageCustom[customStatus].damage = min(m_currentSubdualDamageCustom[customStatus].damage, m_subdualDamageCap);
		}
		if(!isSubduedCustom(customStatus))
			m_currentSubdualDamageCustom[customStatus].isSubdued = FALSE;
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void ActiveBody::onSubdualChangeCustom( Bool isNowSubdued, const DamageInfo *damageInfo, Bool dontPaintTint )
{
	if( !getObject()->isKindOf(KINDOF_PROJECTILE) )
	{
		Object *me = getObject();
		
		if(damageInfo->in.m_customSubdualHasDisable)
		{
			if( isNowSubdued )
			{
				m_clearedSubduedCustom = FALSE;
				
				if(!damageInfo->in.m_customSubdualDisableSound.isEmpty())
				{
					AudioEventRTS disableSound;
					disableSound.setEventName( damageInfo->in.m_customSubdualDisableSound );
					disableSound.setPosition( me->getPosition() );
					TheAudio->addAudioEvent( &disableSound );

					if(!damageInfo->in.m_customSubdualDisableRemoveSound.isEmpty())
					{
						m_customSubdualDisabledSound = damageInfo->in.m_customSubdualDisableSound;
						m_customSubdualDisableRemovalSound = damageInfo->in.m_customSubdualDisableRemoveSound;// For clearing later
					}
				}
				if(dontPaintTint)
				{
					me->setDisabledUntil( damageInfo->in.m_customSubdualDisableType, FOREVER, TINT_STATUS_INVALID, "DontPaint" );
				}
				else
				{
					me->setDisabledUntil( damageInfo->in.m_customSubdualDisableType, FOREVER, damageInfo->in.m_customSubdualDisableTint, damageInfo->in.m_customSubdualDisableCustomTint );
				}
				if(damageInfo->in.m_customSubdualDisableType == DISABLED_SUBDUED || 
						damageInfo->in.m_customSubdualDisableType == DISABLED_FROZEN )
				{
					ContainModuleInterface *contain = me->getContain();
					if ( contain )
						contain->orderAllPassengersToIdle( CMD_FROM_AI );
				}
			}
			else
			{
				if(!damageInfo->in.m_customSubdualDisableRemoveSound.isEmpty())
				{
					AudioEventRTS removeSound;
					removeSound.setEventName( damageInfo->in.m_customSubdualDisableRemoveSound );
					removeSound.setPosition( me->getPosition() );
					TheAudio->addAudioEvent( &removeSound );

					m_customSubdualDisableRemovalSound = NULL;

					if(!damageInfo->in.m_customSubdualDisableSound.isEmpty())
					{
						AudioEventRTS disableSound;
						disableSound.setEventName( damageInfo->in.m_customSubdualDisableSound );
						TheAudio->removeAudioEvent(disableSound.getPlayingHandle());

						m_customSubdualDisabledSound = NULL;
					}
				}
				
				me->clearDisabled(damageInfo->in.m_customSubdualDisableType);

				if(!me->isDisabled())
					m_clearedSubduedCustom = TRUE;

				if(me->isKindOf( KINDOF_FS_INTERNET_CENTER ) &&
						( damageInfo->in.m_customSubdualDisableType == DISABLED_SUBDUED || 
						damageInfo->in.m_customSubdualDisableType == DISABLED_FROZEN ) )
				{
					//Kris: October 20, 2003 - Patch 1.01
					//Any unit inside an internet center is a hacker! Order
					//them to start hacking again.
					ContainModuleInterface *contain = me->getContain();
					if ( contain )
						contain->orderAllPassengersToHackInternet( CMD_FROM_AI );
				}
			}
		}
	}
	else if( isNowSubdued && damageInfo->in.m_customSubdualHasDisableProjectiles )// There is no coming back from being jammed, and projectiles can't even heal, but this makes it clear.
	{
		ProjectileUpdateInterface *pui = getObject()->getProjectileUpdateInterface();
		if( pui )
		{
			if(damageInfo->in.m_isMissileAttractor)
				pui->projectileNowDrawn(damageInfo->in.m_sourceID);
			else
				pui->projectileNowJammed(damageInfo->in.m_subduedProjectileNoDamage);
		}
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void ActiveBody::onSubdualRemovalCustom(DisabledType SubdualDisableType, Bool clearTintLater)
{
	// Check if there's other existing custom subdual types that uses the same DISABLED_TYPE
	for(CustomSubdualCurrentDamageMap::const_iterator it = m_currentSubdualDamageCustom.begin(); it != m_currentSubdualDamageCustom.end(); ++it)
	{
		if(it->second.damage >= m_maxHealth && it->second.disableType == SubdualDisableType)
		{
			return;
		}
	}
	
	Object *me = getObject();

	if(!m_customSubdualDisableRemovalSound.isEmpty())
	{
		AudioEventRTS removeSound;
		removeSound.setEventName( m_customSubdualDisableRemovalSound );
		removeSound.setPosition( me->getPosition() );
		TheAudio->addAudioEvent( &removeSound );

		m_customSubdualDisableRemovalSound = NULL;
		
		if(!m_customSubdualDisabledSound.isEmpty())
		{
			AudioEventRTS disableSound;
			disableSound.setEventName( m_customSubdualDisabledSound );
			TheAudio->removeAudioEvent(disableSound.getPlayingHandle());

			m_customSubdualDisabledSound = NULL;
		}
	}

	me->clearDisabled(SubdualDisableType, clearTintLater);

	if(!me->isDisabled())
		m_clearedSubduedCustom = TRUE;

	if( me->isKindOf( KINDOF_FS_INTERNET_CENTER ) &&
			( SubdualDisableType == DISABLED_SUBDUED || 
			SubdualDisableType == DISABLED_FROZEN ) )
	{
		//Kris: October 20, 2003 - Patch 1.01
		//Any unit inside an internet center is a hacker! Order
		//them to start hacking again.
		ContainModuleInterface *contain = me->getContain();
		if ( contain )
			contain->orderAllPassengersToHackInternet( CMD_FROM_AI );
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void ActiveBody::onSubdualChronoChange( Bool isNowSubdued )
{
	Object *me = getObject();
		
	if( isNowSubdued )
	{
		me->setDisabled(DISABLED_CHRONO);

		// Apply Chrono Particles
		applyChronoParticleSystems();

		m_chronoDisabledSoundLoop = TheAudio->getMiscAudio()->m_chronoDisabledSoundLoop;
		m_chronoDisabledSoundLoop.setObjectID(me->getID());
		m_chronoDisabledSoundLoop.setPlayingHandle(TheAudio->addAudioEvent(&m_chronoDisabledSoundLoop));

		ContainModuleInterface *contain = me->getContain();
		if ( contain )
			contain->orderAllPassengersToIdle( CMD_FROM_AI );
		}
	else
	{
		me->clearDisabled(DISABLED_CHRONO);

		// Remove Chrono Particles, i.e. restore default particles
		updateBodyParticleSystems();

		TheAudio->removeAudioEvent(m_chronoDisabledSoundLoop.getPlayingHandle());

		if (me->isKindOf(KINDOF_FS_INTERNET_CENTER))
		{
			//Kris: October 20, 2003 - Patch 1.01
			//Any unit inside an internet center is a hacker! Order
			//them to start hacking again.
			ContainModuleInterface* contain = me->getContain();
			if (contain)
				contain->orderAllPassengersToHackInternet(CMD_FROM_AI);
		}
	}
}

// ------------------------------------------------------------------------------------------------
/* 	This function is called on state changes only.  Body Type or Aflameness. */
// ------------------------------------------------------------------------------------------------
void ActiveBody::applyChronoParticleSystems(void)
{
	deleteAllParticleSystems();

	static const ParticleSystemTemplate* chronoEffectsLargeTemplate = TheParticleSystemManager->findTemplate(TheGlobalData->m_chronoDisableParticleSystemLarge);
	static const ParticleSystemTemplate* chronoEffectsMediumTemplate = TheParticleSystemManager->findTemplate(TheGlobalData->m_chronoDisableParticleSystemMedium);
	static const ParticleSystemTemplate* chronoEffectsSmallTemplate = TheParticleSystemManager->findTemplate(TheGlobalData->m_chronoDisableParticleSystemSmall);
	
	const ParticleSystemTemplate* chronoEffects;

	// TODO: select particles
	Object* obj = getObject();

	if (obj->isKindOf(KINDOF_INFANTRY)) {
		chronoEffects = chronoEffectsSmallTemplate;
	}
	else if (obj->isKindOf(KINDOF_STRUCTURE)) {
		chronoEffects = chronoEffectsLargeTemplate;
	}
	// Use Medium as default
	else {
		chronoEffects = chronoEffectsMediumTemplate;
	}

	ParticleSystem* particleSystem = TheParticleSystemManager->createParticleSystem(chronoEffects);
	if (particleSystem)
	{
		// attach particle system to object
		particleSystem->attachToObject(obj);

		
		Real x = obj->getGeometryInfo().getMajorRadius();
		Real y;
		if (obj->getGeometryInfo().getGeomType() == GEOMETRY_BOX)
			y = obj->getGeometryInfo().getMinorRadius();
		else
			y = obj->getGeometryInfo().getMajorRadius();
		Real z = obj->getGeometryInfo().getMaxHeightAbovePosition() * 0.5;
		particleSystem->setEmissionBoxHalfSize(x, y, z);
		
		// set the position of the particle system in local object space.
		// Apparently even Zero coordinates are needed here.
		Coord3D pos = { 0, 0, 0}; // *obj->getPosition();
		pos.z += z;
		particleSystem->setPosition(&pos);

		//DEBUG_LOG((">>> applyChronoParticleSystems: pos = {%f, %f, %f}", pos.x, pos.y, pos.z));

		// Scale particle count based on size ?
		//Real size = x * y;
		//particleSystem->setBurstCountMultiplier(MAX(1.0, sqrt(size * 0.02f))); // these are somewhat tweaked right now
		//particleSystem->setBurstDelayMultiplier(MIN(5.0, sqrt(500.0f / size)));

		// create a new body particle system entry and keep this particle system in it
		BodyParticleSystem* newEntry = newInstance(BodyParticleSystem);
		newEntry->m_particleSystemID = particleSystem->getSystemID();
		newEntry->m_next = m_particleSystems;
		m_particleSystems = newEntry;

		// DEBUG_LOG(("ActiveBody::applyChronoParticleSystems - created particleSystem.\n"));
	}
	else {
		// DEBUG_LOG(("ActiveBody::applyChronoParticleSystems - Failed to create particleSystem?!\n"));
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
Bool ActiveBody::isSubduedChrono() const
{
	return (m_maxHealth * TheGlobalData->m_chronoDamageDisableThreshold) <= m_currentChronoDamage;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
Bool ActiveBody::isSubdued() const
{
	return m_maxHealth <= m_currentSubdualDamage;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
Bool ActiveBody::isNearSubduedRange( Real low, Real high ) const
{
	return m_maxHealth > m_currentSubdualDamage - low || m_maxHealth <= m_currentSubdualDamage + high;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
Real ActiveBody::getHealth() const
{
	return m_currentHealth;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BodyDamageType ActiveBody::getDamageState() const
{
	return m_curDamageState;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
Real ActiveBody::getMaxHealth() const
{
	return m_maxHealth;
}  ///< return max health

Real ActiveBody::getSubdualDamageCap() const
{
	return m_subdualDamageCap;
} 

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
UnsignedInt ActiveBody::getSubdualDamageHealRate() const
{
	return m_subdualDamageHealRate;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
Real ActiveBody::getSubdualDamageHealAmount() const
{
	return m_subdualDamageHealAmount;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
Bool ActiveBody::hasAnySubdualDamage() const
{
	return m_currentSubdualDamage > 0;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
Bool ActiveBody::hasAnySubdualDamageCustom() const
{
	for(CustomSubdualCurrentDamageMap::const_iterator it = m_currentSubdualDamageCustom.begin(); it != m_currentSubdualDamageCustom.end(); ++it)
	{
		if(it->second.damage > 0)
		{
			return TRUE;
		}
	}
	return FALSE;
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
std::vector<AsciiString> ActiveBody::getAnySubdualDamageCustom() const
{
	std::vector<AsciiString> stringVec;
	for(CustomSubdualCurrentDamageMap::const_iterator it = m_currentSubdualDamageCustom.begin(); it != m_currentSubdualDamageCustom.end(); ++it)
	{
		if(it->second.damage > 0)
		{
			stringVec.push_back(it->first);
		}
	}
	return stringVec;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
Real ActiveBody::getSubdualDamageCapCustom(const AsciiString& customStatus) const
{
	CustomSubdualDamageMap::const_iterator it = m_subdualDamageCapCustom.find(customStatus);
	if(it != m_subdualDamageCapCustom.end())
	{
		return it->second;
	}
	return m_subdualDamageCap;
} 

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
UnsignedInt ActiveBody::getSubdualDamageHealRateCustom(const AsciiString& customStatus) const
{
	CustomSubdualHealRateMap::const_iterator it = m_subdualDamageHealRateCustom.find(customStatus);
	if(it != m_subdualDamageHealRateCustom.end())
	{
		return it->second;
	}
	return m_subdualDamageHealRate;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
Real ActiveBody::getSubdualDamageHealAmountCustom(const AsciiString& customStatus) const
{
	CustomSubdualDamageMap::const_iterator it = m_subdualDamageHealAmountCustom.find(customStatus);
	if(it != m_subdualDamageHealAmountCustom.end())
	{
		return it->second;
	}
	return m_subdualDamageHealAmount;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
UnsignedInt ActiveBody::getChronoDamageHealRate() const
{
	return TheGlobalData->m_chronoDamageHealRate;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
Real ActiveBody::getChronoDamageHealAmount() const
{
	// DEBUG_LOG(("ActiveBody::getChronoDamageHealAmount() - maxHealth = %f\n", m_maxHealth));
	return m_maxHealth * TheGlobalData->m_chronoDamageHealAmount;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
Bool ActiveBody::hasAnyChronoDamage() const
{
	return m_currentChronoDamage > 0;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
Real ActiveBody::getInitialHealth() const 
{ 
	return m_initialHealth;
}  // return initial health


// ------------------------------------------------------------------------------------------------
/** Set or unset the overridable indestructible flag in the body */
// ------------------------------------------------------------------------------------------------
void ActiveBody::setIndestructible( Bool indestructible )
{

	m_indestructible = indestructible;

	// for bridges, we mirror this state on its towers
	Object *us = getObject();
	if( us->isKindOf( KINDOF_BRIDGE ) )
	{
		BridgeBehaviorInterface *bbi = BridgeBehavior::getBridgeBehaviorInterfaceFromObject( us );
		if( bbi )
		{
			Object *tower;

			// get tower
			for( Int i = 0; i < BRIDGE_MAX_TOWERS; ++i )
			{

				tower = TheGameLogic->findObjectByID( bbi->getTowerID( BridgeTowerType(i) ) );
				if( tower )
				{
					BodyModuleInterface *body = tower->getBodyModule();

					if( body )
						body->setIndestructible( indestructible );

				}  // end if

			}  // end for, i

		}  // end if

	}  // end if

}  // end setIndestructible

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void ActiveBody::onVeterancyLevelChanged( VeterancyLevel oldLevel, VeterancyLevel newLevel, Bool provideFeedback )
{
	if (oldLevel == newLevel)
		return;

	if (oldLevel < newLevel)
	{
		if( provideFeedback )
		{
			AudioEventRTS veterancyChanged;
			switch (newLevel)
			{
				case LEVEL_VETERAN:
					veterancyChanged = *getObject()->getTemplate()->getSoundPromotedVeteran();
					break;
				case LEVEL_ELITE:
					veterancyChanged = *getObject()->getTemplate()->getSoundPromotedElite();
					break;
				case LEVEL_HEROIC:
					veterancyChanged = *getObject()->getTemplate()->getSoundPromotedHero();
					break;
			}

			veterancyChanged.setObjectID(getObject()->getID());
			TheAudio->addAudioEvent(&veterancyChanged);
		}

		//Also mark the UI dirty -- incase the object is selected or contained.
		Object *obj = getObject();
		Drawable *draw = TheInGameUI->getFirstSelectedDrawable();
		if( draw )
		{
			Object *checkOwner = draw->getObject();
			if( checkOwner == obj )
			{
				//Our selected object has been promoted!
				TheControlBar->markUIDirty();
			}
			else
			{
				const Object *containedBy = obj->getContainedBy();
				if( containedBy && TheInGameUI->getSelectCount() == 1 )
				{
					Object *checkOwner = draw->getObject();
					if( checkOwner == containedBy )
					{
						//But only if the contained by object is containing me!
						TheControlBar->markUIDirty();
					}
				}
			}
		}
	}

	Real oldBonus = TheGlobalData->m_healthBonus[oldLevel];
	Real newBonus = TheGlobalData->m_healthBonus[newLevel];
	Real mult = newBonus / oldBonus;

	// get this before calling setMaxHealth, since it can clip curHealth
	//Real newHealth = m_currentHealth * mult;

	// change the max
	setMaxHealth(m_maxHealth * mult, PRESERVE_RATIO );

	// now change the cur (setMaxHealth now handles it)
	//internalChangeHealth( newHealth - m_currentHealth );

	switch (newLevel)
	{
		case LEVEL_REGULAR:
			clearArmorSetFlag(ARMORSET_VETERAN);
			clearArmorSetFlag(ARMORSET_ELITE);
			clearArmorSetFlag(ARMORSET_HERO);
			break;
		case LEVEL_VETERAN:
			setArmorSetFlag(ARMORSET_VETERAN);
			clearArmorSetFlag(ARMORSET_ELITE);
			clearArmorSetFlag(ARMORSET_HERO);
			break;
		case LEVEL_ELITE:
			clearArmorSetFlag(ARMORSET_VETERAN);
			setArmorSetFlag(ARMORSET_ELITE);
			clearArmorSetFlag(ARMORSET_HERO);
			break;
		case LEVEL_HEROIC:
			clearArmorSetFlag(ARMORSET_VETERAN);
			clearArmorSetFlag(ARMORSET_ELITE);
			setArmorSetFlag(ARMORSET_HERO);
			break;
	}
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void ActiveBody::setAflame( Bool )
{

	//
	// All this does now is act like a major body state change. It is called after Aflame has been
	// set or cleared as an Object Status
	//
	updateBodyParticleSystems();

}
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void ActiveBody::overrideDamageFX(DamageFX* damageFX)
{
	if (damageFX != NULL) {
		m_curDamageFX = damageFX;
		m_damageFXOverride = true;
	}
	else {
		m_curDamageFX = NULL;
		m_damageFXOverride = false;

		// Restore DamageFX from current armorset
		const ArmorTemplateSet* set = getObject()->getTemplate()->findArmorTemplateSet(m_curArmorSetFlags);
		if (set)
		{
			m_curDamageFX = set->getDamageFX();
		}
	}
	//DEBUG_LOG((">>>ActiveBody: overrideDamageFX - new m_curDamageFX = %d, m_damageFXOverride = %d\n",
	//	m_curDamageFX, m_damageFXOverride));
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void ActiveBody::crc( Xfer *xfer )
{

  // extend base class
	BodyModule::crc( xfer );

}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void ActiveBody::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// base class
	BodyModule::xfer( xfer );

	// current health
	xfer->xferReal( &m_currentHealth );

	xfer->xferReal( &m_subdualDamageCap );

	xfer->xferUnsignedInt( &m_subdualDamageHealRate );
	
	xfer->xferReal( &m_subdualDamageHealAmount );

	xfer->xferReal( &m_currentSubdualDamage );

	// previous health
	xfer->xferReal( &m_prevHealth );

	// max health
	xfer->xferReal( &m_maxHealth );

	// initial health
	xfer->xferReal( &m_initialHealth );

	// current damage state
	xfer->xferUser( &m_curDamageState, sizeof( BodyDamageType ) );

	// next damage fx time
	xfer->xferUnsignedInt( &m_nextDamageFXTime );

	// last damage fx done
	xfer->xferUser( &m_lastDamageFXDone, sizeof( DamageType ) );

	// last damage info
	xfer->xferSnapshot( &m_lastDamageInfo );

	// last damage timestamp
	xfer->xferUnsignedInt( &m_lastDamageTimestamp );

	// last damage timestamp
	xfer->xferUnsignedInt( &m_lastHealingTimestamp );

	// front crushed
	xfer->xferBool( &m_frontCrushed );

	// back crushed
	xfer->xferBool( &m_backCrushed );

	// last damaged cleared
	xfer->xferBool( &m_lastDamageCleared );

	// indestructible
	xfer->xferBool( &m_indestructible );

	xfer->xferBool( &m_hasBeenSubdued );
	xfer->xferBool( &m_clearedSubdued );
	xfer->xferBool( &m_clearedSubduedCustom );

	// particle system count
	BodyParticleSystem *system;
	UnsignedShort particleSystemCount = 0;
	for( system = m_particleSystems; system; system = system->m_next )
		particleSystemCount++;
	xfer->xferUnsignedShort( &particleSystemCount );

	// particle systems
	if( xfer->getXferMode() == XFER_SAVE )
	{

		// walk the particle systems
		for( system = m_particleSystems; system; system = system->m_next )
		{

			// write particle system ID
			xfer->xferUser( &system->m_particleSystemID, sizeof( ParticleSystemID ) );

		}  // end for, system

	}  // end if, save
	else
	{
		ParticleSystemID particleSystemID;

		// the list should be empty at this time
		if( m_particleSystems != NULL )
		{

			DEBUG_CRASH(( "ActiveBody::xfer - m_particleSystems should be empty, but is not" ));
			throw SC_INVALID_DATA;

		}  // end if

		// read all data elements
		BodyParticleSystem *newEntry;
		for( UnsignedShort i = 0; i < particleSystemCount; ++i )
		{

			// read particle system ID
			xfer->xferUser( &particleSystemID, sizeof( ParticleSystemID ) );

			// allocate entry and add to list
			newEntry = newInstance(BodyParticleSystem);
			newEntry->m_particleSystemID = particleSystemID;
			newEntry->m_next = m_particleSystems;  // the list will be reversed, but we don't care
			m_particleSystems = newEntry;

		}  // end for, i

	}  // end else, load

	// armor set flags
	m_curArmorSetFlags.xfer( xfer );

	// For Disable Removal
	xfer->xferAsciiString( &m_customSubdualDisabledSound );
	xfer->xferAsciiString( &m_customSubdualDisableRemovalSound );

	// Modified from Team.cpp
	CustomSubdualCurrentDamageMap::iterator customSubdualIt;
	CustomSubdualDamageMap::iterator customSubdualIt2;
	CustomSubdualHealRateMap::iterator customSubdualIt3;
	CustomSubdualDamageMap::iterator customSubdualIt4;

	UnsignedShort customSubdualCount = m_currentSubdualDamageCustom.size();
	UnsignedShort customSubdualCount2 = m_subdualDamageCapCustom.size();
	UnsignedShort customSubdualCount3 = m_subdualDamageHealRateCustom.size();
	UnsignedShort customSubdualCount4 = m_subdualDamageHealAmountCustom.size();
	
	xfer->xferUnsignedShort( &customSubdualCount );
	xfer->xferUnsignedShort( &customSubdualCount2 );
	xfer->xferUnsignedShort( &customSubdualCount3 );
	xfer->xferUnsignedShort( &customSubdualCount4 );

	AsciiString customSubdualName;
	AsciiString customSubdualName2;
	AsciiString customSubdualName3;
	AsciiString customSubdualName4;
	
	Real customSubdualAmount;
	UnsignedInt customSubdualTint;
	AsciiString customSubdualCustomTint;
	UnsignedInt customSubdualDisableType;
	Bool customHasBeenSubdued;

	Real customSubdualAmount2;
	UnsignedInt customSubdualAmount3;
	Real customSubdualAmount4;
	if( xfer->getXferMode() == XFER_SAVE )
	{

		for( customSubdualIt = m_currentSubdualDamageCustom.begin(); customSubdualIt != m_currentSubdualDamageCustom.end(); ++customSubdualIt )
		{

			customSubdualName = (*customSubdualIt).first;
			xfer->xferAsciiString( &customSubdualName );

			customSubdualAmount = (*customSubdualIt).second.damage;
			xfer->xferReal( &customSubdualAmount );

			customSubdualTint = (*customSubdualIt).second.tintStatus;
			xfer->xferUnsignedInt( &customSubdualTint );

			customSubdualCustomTint = (*customSubdualIt).second.customTintStatus;
			xfer->xferAsciiString( &customSubdualCustomTint );

			customSubdualDisableType = (*customSubdualIt).second.disableType;
			xfer->xferUnsignedInt( &customSubdualDisableType );

			customHasBeenSubdued = (*customSubdualIt).second.isSubdued;
			xfer->xferBool( &customHasBeenSubdued );

		}  // end for

		for( customSubdualIt2 = m_subdualDamageCapCustom.begin(); customSubdualIt2 != m_subdualDamageCapCustom.end(); ++customSubdualIt2 )
		{

			customSubdualName2 = (*customSubdualIt2).first;
			xfer->xferAsciiString( &customSubdualName2 );

			customSubdualAmount2 = (*customSubdualIt2).second;
			xfer->xferReal( &customSubdualAmount2 );

		}  // end for

		for( customSubdualIt3 = m_subdualDamageHealRateCustom.begin(); customSubdualIt3 != m_subdualDamageHealRateCustom.end(); ++customSubdualIt3 )
		{

			customSubdualName3 = (*customSubdualIt3).first;
			xfer->xferAsciiString( &customSubdualName3 );

			customSubdualAmount3 = (*customSubdualIt3).second;
			xfer->xferUnsignedInt( &customSubdualAmount3 );

		}  // end for

		for( customSubdualIt4 = m_subdualDamageHealAmountCustom.begin(); customSubdualIt4 != m_subdualDamageHealAmountCustom.end(); ++customSubdualIt4 )
		{

			customSubdualName4 = (*customSubdualIt4).first;
			xfer->xferAsciiString( &customSubdualName4 );

			customSubdualAmount4 = (*customSubdualIt4).second;
			xfer->xferReal( &customSubdualAmount4 );

		}  // end for


	}  // end if, save
	else
	{

		for( UnsignedShort i = 0; i < customSubdualCount; ++i )
		{

			xfer->xferAsciiString( &customSubdualName );

			xfer->xferReal( &customSubdualAmount );

			xfer->xferUnsignedInt( &customSubdualTint );

			xfer->xferAsciiString( &customSubdualCustomTint );

			xfer->xferUnsignedInt( &customSubdualDisableType );

			xfer->xferBool( &customHasBeenSubdued );

			SubdualCustomData data;
			data.damage = customSubdualAmount;
			data.tintStatus = (TintStatus)customSubdualTint;
			data.customTintStatus = customSubdualCustomTint;
			data.disableType = (DisabledType)customSubdualDisableType;
			data.isSubdued = customHasBeenSubdued;

			m_currentSubdualDamageCustom[customSubdualName] = data;
			
		}  // end for, i

		for( UnsignedShort i_2 = 0; i_2 < customSubdualCount2; ++i_2 )
		{

			xfer->xferAsciiString( &customSubdualName2 );

			xfer->xferReal( &customSubdualAmount2 );

			m_subdualDamageCapCustom[customSubdualName2] = customSubdualAmount2;
			
		}  // end for, i

		for( UnsignedShort i_3 = 0; i_3 < customSubdualCount3; ++i_3 )
		{

			xfer->xferAsciiString( &customSubdualName3 );

			xfer->xferUnsignedInt( &customSubdualAmount3 );

			m_subdualDamageHealRateCustom[customSubdualName3] = customSubdualAmount3;
			
		}  // end for, i

		for( UnsignedShort i_4 = 0; i_4 < customSubdualCount4; ++i_4 )
		{

			xfer->xferAsciiString( &customSubdualName4 );

			xfer->xferReal( &customSubdualAmount4 );

			m_subdualDamageHealAmountCustom[customSubdualName4] = customSubdualAmount4;
			
		}  // end for, i

	}  // end else load

}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void ActiveBody::loadPostProcess( void )
{

	// extend base class
	BodyModule::loadPostProcess();

}  // end loadPostProcess

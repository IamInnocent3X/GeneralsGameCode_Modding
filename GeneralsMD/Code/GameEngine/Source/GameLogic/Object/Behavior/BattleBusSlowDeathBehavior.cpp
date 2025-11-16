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

// FILE: BattleBusSlowDeathBehavior.cpp /////////////////////////////////////////////////////////////////
// Author: Graham Smallwood, June 2003
// Desc:   Death for the Battle Bus.  Can do a fake slow death before the real one if triggered intentionally
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file int the GameEngine
#include "Common/Xfer.h"

#include "GameLogic/Module/BattleBusSlowDeathBehavior.h"

#include "Common/ModelState.h"

#include "GameClient/FXList.h"

#include "GameLogic/GameLogic.h"
#include "GameLogic/Object.h"
#include "GameLogic/ObjectCreationList.h"
#include "GameLogic/Module/AIUpdate.h"
#include "GameLogic/Module/ContainModule.h"
#include "GameLogic/Module/PhysicsUpdate.h"


enum
{
	GROUND_CHECK_DELAY = 10,		///< Check for colliding with the ground only after this long, to prevent hitting on the way up
	EMPTY_HULK_CHECK_DELAY = 15 ///< Check for empty hulk every this often instead of every frame
};

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
BattleBusSlowDeathBehaviorModuleData::BattleBusSlowDeathBehaviorModuleData( void )
{

	m_fxStartUndeath = NULL;
	m_oclStartUndeath = NULL;

	m_fxHitGround = NULL;
	m_oclHitGround = NULL;

	m_throwForce = 1.0f;
	m_percentDamageToPassengers = 0.0f;
	m_emptyHulkDestructionDelay = 0;

	m_percentDamageToPassengersScale = FALSE;
	m_percentDamageToPassengersScaleRatio = 1.0f;;

	m_sleepsafteramountofdeaths = 0;
	m_modelswitchafteramountofdeaths = 0;
	m_triggerfxafteramountofdeaths = 0;
	m_triggeroclafteramountofdeaths = 0;
	m_throwForceafteramountofdeaths = 0;
	m_damageToPassengerafteramountofdeaths = 0;
	m_noPassengersAmountOfDeathTrigger = 2;

	m_multipleLivesfxStartUndeathList.clear();
	m_multipleLivesoclStartUndeathList.clear();
	m_multipleLivesfxHitGroundList.clear();
	m_multipleLivesoclHitGroundList.clear();

}  // end BattleBusSlowDeathBehaviorModuleData

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
/*static*/ void BattleBusSlowDeathBehaviorModuleData::buildFieldParse( MultiIniFieldParse &p )
{
  SlowDeathBehaviorModuleData::buildFieldParse( p );

	static const FieldParse dataFieldParse[] =
	{

		{ "FXStartUndeath",	INI::parseFXList,	NULL, offsetof( BattleBusSlowDeathBehaviorModuleData, m_fxStartUndeath ) },
		{ "OCLStartUndeath", INI::parseObjectCreationList, NULL, offsetof( BattleBusSlowDeathBehaviorModuleData, m_oclStartUndeath ) },

		{ "FXHitGround",	INI::parseFXList,	NULL, offsetof( BattleBusSlowDeathBehaviorModuleData, m_fxHitGround ) },
		{ "OCLHitGround", INI::parseObjectCreationList, NULL, offsetof( BattleBusSlowDeathBehaviorModuleData, m_oclHitGround ) },

		{ "ThrowForce", INI::parseReal, NULL, offsetof( BattleBusSlowDeathBehaviorModuleData, m_throwForce ) },
		{ "PercentDamageToPassengers", INI::parsePercentToReal, NULL, offsetof( BattleBusSlowDeathBehaviorModuleData, m_percentDamageToPassengers ) },
		{ "EmptyHulkDestructionDelay", INI::parseDurationUnsignedInt, NULL, offsetof( BattleBusSlowDeathBehaviorModuleData, m_emptyHulkDestructionDelay ) },

		{ "PercentDamageToPassengersScales", INI::parseBool, NULL, offsetof( BattleBusSlowDeathBehaviorModuleData, m_percentDamageToPassengersScale ) },
		{ "PercentDamageToPassengersScaleRatio", INI::parsePercentToReal, NULL, offsetof( BattleBusSlowDeathBehaviorModuleData, m_percentDamageToPassengersScaleRatio ) },

		{ "SleepsAfterAmountofDeaths", INI::parseInt, NULL, offsetof( BattleBusSlowDeathBehaviorModuleData, m_sleepsafteramountofdeaths ) },
		{ "ModelConditionSwitchAfterAmountofDeaths", INI::parseInt, NULL, offsetof( BattleBusSlowDeathBehaviorModuleData, m_modelswitchafteramountofdeaths ) },
		{ "TriggerFXAfterAmountofDeaths", INI::parseInt, NULL, offsetof( BattleBusSlowDeathBehaviorModuleData, m_triggerfxafteramountofdeaths ) },
		{ "TriggerOCLAfterAmountofDeaths", INI::parseInt, NULL, offsetof( BattleBusSlowDeathBehaviorModuleData, m_triggeroclafteramountofdeaths ) },
		{ "ThrowForceAfterAmountofDeaths", INI::parseInt, NULL, offsetof( BattleBusSlowDeathBehaviorModuleData, m_throwForceafteramountofdeaths ) },
		{ "PercentDamageToPassengersAfterAmountofDeaths", INI::parseInt, NULL, offsetof( BattleBusSlowDeathBehaviorModuleData, m_damageToPassengerafteramountofdeaths ) },

//		Unlike UndeadBody Module, the Trigger FX/OCL considers the FX/OCL closest to the Death Amount instead of required to re-declare it everytime.
//		Example if the Trigger Amount is: 9 3 6 7 1. If your Death Amount is 5. You would Trigger the Condition at '3', as it is less than 5 while being closest to 5.
		{ "FXStartUndeathAmountDeathsOverride", parseFXIntPair, NULL, offsetof( BattleBusSlowDeathBehaviorModuleData, m_multipleLivesfxStartUndeathList) }, 
		{ "OCLStartUndeathAmountDeathsOverride", parseOCLIntPair, NULL, offsetof( BattleBusSlowDeathBehaviorModuleData, m_multipleLivesoclStartUndeathList) }, 
		
		{ "FXHitGroundAmountDeathsOverride", parseFXIntPair, NULL, offsetof( BattleBusSlowDeathBehaviorModuleData, m_multipleLivesfxHitGroundList) }, 
		{ "OCLHitGroundAmountDeathsOverride", parseOCLIntPair, NULL, offsetof( BattleBusSlowDeathBehaviorModuleData, m_multipleLivesoclHitGroundList) }, 

		{ "DeathOnNoPassengersAmountOfDeaths", INI::parseInt, NULL, offsetof( BattleBusSlowDeathBehaviorModuleData, m_noPassengersAmountOfDeathTrigger ) },
		
		{ 0, 0, 0, 0 }

	};

  p.add( dataFieldParse );

}  // end buildFieldParse

void BattleBusSlowDeathBehaviorModuleData::parseOCLIntPair( INI* ini, void * /*instance*/, void *store, const void* /*userData*/ )
{
	IntOCL up;

	INI::parseInt(ini, NULL, &up.m_deathTrigger, NULL);
	INI::parseObjectCreationList(ini, NULL, &up.m_deathocl, NULL);

	std::vector<IntOCL>* s = (std::vector<IntOCL>*)store;
	s->push_back(up);
} 

void BattleBusSlowDeathBehaviorModuleData::parseFXIntPair( INI* ini, void * /*instance*/, void *store, const void* /*userData*/ )
{
	IntFX up;

	INI::parseInt(ini, NULL, &up.m_deathTrigger, NULL);
	INI::parseFXList(ini, NULL, &up.m_deathfx, NULL);

	std::vector<IntFX>* s = (std::vector<IntFX>*)store;
	s->push_back(up);
} 

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
BattleBusSlowDeathBehavior::BattleBusSlowDeathBehavior( Thing *thing, const ModuleData *moduleData )
										: SlowDeathBehavior( thing, moduleData )
{

	m_isRealDeath = FALSE;
	m_isInFirstDeath = FALSE;
	m_groundCheckFrame = 0;
	m_penaltyDeathFrame = 0;
	m_amountofDeaths = 0;
}  // end BattleBusSlowDeathBehavior

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
BattleBusSlowDeathBehavior::~BattleBusSlowDeathBehavior( void )
{

}  // end ~BattleBusSlowDeathBehavior

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void BattleBusSlowDeathBehavior::onDie( const DamageInfo *damageInfo )
{
	m_isRealDeath = TRUE;// Set beforehand because onDie could result in picking us to beginSlowDeath
	m_isInFirstDeath = FALSE; // and clear this incase we died while in the alternate death

	SlowDeathBehavior::onDie(damageInfo);
}  // end onDie

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void BattleBusSlowDeathBehavior::beginSlowDeath( const DamageInfo *damageInfo )
{
	Object *me = getObject();
	const BattleBusSlowDeathBehaviorModuleData * data = getBattleBusSlowDeathBehaviorModuleData();
	if( !m_isRealDeath )
	{
		// We can intercept and do our extra slow death if this is not from a real onDie
		m_isInFirstDeath = TRUE;
		m_groundCheckFrame = TheGameLogic->getFrame() + GROUND_CHECK_DELAY;

		// Check for the amount of deaths this unit encounters. Considered the time it dies during Second Life as the first Death, and every Trigger Condition 
		// does not account for death amount to trigger, (0). Setting any condition to Trigger at Multiple Lives requires the Death Amount to be at least 2 lives.
		m_amountofDeaths++;

		// First do the special effects
		if(data->m_triggerfxafteramountofdeaths<=m_amountofDeaths) FXList::doFXObj(findFX(1, m_amountofDeaths), me );
		if(data->m_triggeroclafteramountofdeaths<=m_amountofDeaths) ObjectCreationList::create(findOCL(1, m_amountofDeaths), me, NULL );

		if( me->getAI() && data->m_sleepsafteramountofdeaths <= m_amountofDeaths)
		{
			// Then stop what we were doing
			me->getAI()->aiIdle(CMD_FROM_AI);
		}

		if( me->getPhysics() && data->m_throwForceafteramountofdeaths <= m_amountofDeaths)
		{
			// Then stop physically
			me->getPhysics()->clearAcceleration();
			me->getPhysics()->scrubVelocity2D(0);

			// Then get chucked in the air
			Coord3D throwForce;
			throwForce.x = 0;
			throwForce.y = 0;
			throwForce.z = data->m_throwForce;
			me->getPhysics()->applyShock(&throwForce);
			me->getPhysics()->applyRandomRotation();
		}

		// And finally hit those inside for some damage
		if( me->getContain() && data->m_damageToPassengerafteramountofdeaths <= m_amountofDeaths) {
			Real damageratio = data->m_percentDamageToPassengersScaleRatio;
			if(data->m_percentDamageToPassengersScale) {
				damageratio = pow(damageratio, m_amountofDeaths);
			} else {
				damageratio = 1.0f;
			}
			me->getContain()->processDamageToContained(data->m_percentDamageToPassengers*damageratio);
		}
		
		setWakeFrame(getObject(), UPDATE_SLEEP_NONE);
	}
	else
	{
		// If a real death, we aren't allowed to do anything
		SlowDeathBehavior::beginSlowDeath( damageInfo );
	}

}  // end beginSlowDeath

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
UpdateSleepTime BattleBusSlowDeathBehavior::update( void )
{
	Object *me = getObject();
	const BattleBusSlowDeathBehaviorModuleData * data = getBattleBusSlowDeathBehaviorModuleData();
	const ContainModuleInterface *contain = me->getContain();

	Bool notContained = FALSE;
	if( data->m_noPassengersAmountOfDeathTrigger <= m_amountofDeaths && ( contain == NULL || ( contain != NULL && contain->getContainCount() == 0 ) ) )
	{
		notContained = TRUE;
	}

	if( m_isInFirstDeath && !notContained )
	{
		// First death means we are doing our throw up in the air conversion

		if( (m_groundCheckFrame< TheGameLogic->getFrame() )  &&  !me->isAboveTerrain() )
		{
			// Do the special FX
			if(data->m_triggerfxafteramountofdeaths<=m_amountofDeaths)FXList::doFXObj(findFX(2, m_amountofDeaths), me );
			if(data->m_triggeroclafteramountofdeaths<=m_amountofDeaths)ObjectCreationList::create(findOCL(2, m_amountofDeaths), me, NULL );
			if(data->m_modelswitchafteramountofdeaths <= m_amountofDeaths)me->setModelConditionState(MODELCONDITION_SECOND_LIFE);

			// We're done since we hit the ground
			m_isInFirstDeath = FALSE;

			if(data->m_sleepsafteramountofdeaths <= m_amountofDeaths)
			{
				// And stop us forever
				if( me->getAI() )
				{
					// Stop doing anything again (could try to move while in air)
					me->getAI()->aiIdle(CMD_FROM_AI);
				}
				if( me->getPhysics() )
				{
					// And stop physically again
					me->getPhysics()->clearAcceleration();
					me->getPhysics()->scrubVelocity2D(0);
				}
				me->setDisabled(DISABLED_HELD);

				// We can only sleep if we don't have to watch out for being empty.
				if( data->m_emptyHulkDestructionDelay == 0 )
					return UPDATE_SLEEP_FOREVER;
				else
					return UPDATE_SLEEP_NONE; // None, so we check for empty as soon as possible
			}
		}
		return UPDATE_SLEEP_NONE;// None, since we are waiting to be able to check for ground collision
	}
	else if( m_isRealDeath )
	{
		// If a real death, we aren't allowed to do anything
		return SlowDeathBehavior::update();
	}
	else
	{
		// If neither death is active, we must be awake to check for emptiness
		
		// Safety, no need to be awake if no special case to wait for
		if( contain == NULL && notContained )
			return UPDATE_SLEEP_FOREVER;

		if( m_penaltyDeathFrame != 0 )
		{
			// We have been timed for death, see if we have re-filled first
			if( contain->getContainCount() > 0 || !notContained )
			{
				m_penaltyDeathFrame = 0;
				return UPDATE_SLEEP(EMPTY_HULK_CHECK_DELAY);
			}
			else if( TheGameLogic->getFrame() > m_penaltyDeathFrame )
			{
				// No salvation, see if the timer is up to be killed.  Penalty prevents effects, Extra 4 prevents this die module from recalling.
				// Can't use Suicided death because a Terrorist actually inflicts Suicide deaths on others.
				me->kill(DAMAGE_PENALTY, DEATH_EXTRA_4);
				return UPDATE_SLEEP_FOREVER;// Forever, since we are dead and some other death module is in charge
			}
			else
			{
				return UPDATE_SLEEP(EMPTY_HULK_CHECK_DELAY);
			}
		}
		else
		{
			// We are not marked for death, so see if we should be
			if( contain->getContainCount() == 0 && notContained )
			{
				m_penaltyDeathFrame = TheGameLogic->getFrame() + data->m_emptyHulkDestructionDelay;
			}
			return UPDATE_SLEEP(EMPTY_HULK_CHECK_DELAY);// Stay awake regardless
		}
	}
}  // end update

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void BattleBusSlowDeathBehavior::crc( Xfer *xfer )
{

	// extend base class
	SlowDeathBehavior::crc( xfer );

}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void BattleBusSlowDeathBehavior::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	SlowDeathBehavior::xfer( xfer );

	xfer->xferBool( &m_isRealDeath );

	xfer->xferBool( &m_isInFirstDeath );

	xfer->xferUnsignedInt( &m_groundCheckFrame );

	xfer->xferUnsignedInt( &m_penaltyDeathFrame );

	xfer->xferInt( &m_amountofDeaths );

}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void BattleBusSlowDeathBehavior::loadPostProcess( void )
{

	// extend base class
	SlowDeathBehavior::loadPostProcess();

}  // end loadPostProcess

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
const FXList* BattleBusSlowDeathBehavior::findFX(int type, int amountofDeaths) const
{
	const BattleBusSlowDeathBehaviorModuleData * data = getBattleBusSlowDeathBehaviorModuleData();

	// The magic code that makes the triggering condition considers the OCL closests to the amount of deaths instead of requiring to redeclare everytime.
	int 							triggeringInt = 0;
	const FXList*					triggeringfx;
	// If we want to get fxStartUndeath
	if(type == 1)
	{
		if(amountofDeaths>0 && !data->m_multipleLivesfxStartUndeathList.empty())
		{
			for (std::vector<BattleBusSlowDeathBehaviorModuleData::IntFX>::const_iterator it = data->m_multipleLivesfxStartUndeathList.begin(); 
						it != data->m_multipleLivesfxStartUndeathList.end();
						++it)
			{
				if ((it->m_deathTrigger) > amountofDeaths)
				{
					continue;
				}
				else if ((it->m_deathTrigger) == amountofDeaths)
				{
					return it->m_deathfx;
				}
				else if(triggeringInt < (it->m_deathTrigger) && (it->m_deathTrigger) < amountofDeaths)
				{
					triggeringInt = it->m_deathTrigger;
					triggeringfx  = it->m_deathfx;
				}
			}
			if(triggeringInt > 0)
				return triggeringfx;
		}
		return data->m_fxStartUndeath;
	} 
	// If we want to get fxHitGround
	else if (type == 2)
	{
		if(amountofDeaths>0 && !data->m_multipleLivesfxHitGroundList.empty())
		{
			for (std::vector<BattleBusSlowDeathBehaviorModuleData::IntFX>::const_iterator it = data->m_multipleLivesfxHitGroundList.begin(); 
						it != data->m_multipleLivesfxHitGroundList.end();
						++it)
			{
				if ((it->m_deathTrigger) > amountofDeaths)
				{
					continue;
				}
				else if ((it->m_deathTrigger) == amountofDeaths)
				{
					return it->m_deathfx;
				}
				else if(triggeringInt < (it->m_deathTrigger) && (it->m_deathTrigger) < amountofDeaths)
				{
					triggeringInt = it->m_deathTrigger;
					triggeringfx  = it->m_deathfx;
				}
			}
			if(triggeringInt > 0)
				return triggeringfx;
		}
		return data->m_fxHitGround;
	}
	else
	{
		DEBUG_ASSERTCRASH(0, ("Invalid Type Error for BattleBusSlowDeathBehavior findFX. Object: %s, FXType: %d, Amount Of Deaths Triggered: %d.", getObject()->getName(), type, amountofDeaths));
	}
	return NULL;
}

const ObjectCreationList* BattleBusSlowDeathBehavior::findOCL(int type, int amountofDeaths) const
{
	const BattleBusSlowDeathBehaviorModuleData * data = getBattleBusSlowDeathBehaviorModuleData();

	// The magic code that makes the triggering condition considers the OCL closests to the amount of deaths instead of requiring to redeclare everytime.
	int 							triggeringInt = 0;
	const ObjectCreationList*		triggeringocl;
	// If we want to get oclStartUndeath
	if(type == 1)
	{
		if(amountofDeaths>0 && !data->m_multipleLivesoclStartUndeathList.empty())
		{
			for (std::vector<BattleBusSlowDeathBehaviorModuleData::IntOCL>::const_iterator it = data->m_multipleLivesoclStartUndeathList.begin(); 
						it != data->m_multipleLivesoclStartUndeathList.end();
						++it)
			{
				if ((it->m_deathTrigger) > amountofDeaths)
				{
					continue;
				}
				else if ((it->m_deathTrigger) == amountofDeaths)
				{
					return it->m_deathocl;
				}
				else if(triggeringInt < (it->m_deathTrigger) && (it->m_deathTrigger) < amountofDeaths)
				{
					triggeringInt = it->m_deathTrigger;
					triggeringocl  = it->m_deathocl;
				}
			}
			if(triggeringInt > 0)
				return triggeringocl;
		}
		return data->m_oclStartUndeath;
	} 
	// If we want to get oclHitGround
	else if (type == 2)
	{
		if(amountofDeaths>0 && !data->m_multipleLivesoclHitGroundList.empty())
		{
			for (std::vector<BattleBusSlowDeathBehaviorModuleData::IntOCL>::const_iterator it = data->m_multipleLivesoclHitGroundList.begin(); 
						it != data->m_multipleLivesoclHitGroundList.end();
						++it)
			{
				if ((it->m_deathTrigger) > amountofDeaths)
				{
					continue;
				}
				else if ((it->m_deathTrigger) == amountofDeaths)
				{
					return it->m_deathocl;
				}
				else if(triggeringInt < (it->m_deathTrigger) && (it->m_deathTrigger) < amountofDeaths)
				{
					triggeringInt = it->m_deathTrigger;
					triggeringocl  = it->m_deathocl;
				}
			}
			if(triggeringInt > 0)
				return triggeringocl;
		}
		return data->m_oclHitGround;
	}
	else
	{
		DEBUG_ASSERTCRASH(0, ("Invalid Type Error for BattleBusSlowDeathBehavior findOCL. Object: %s, OCLType: %d, Amount Of Deaths Triggered: %d.", getObject()->getName(), type, amountofDeaths));
	}
	return NULL;
}
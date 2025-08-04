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

// FILE: PoisonedBehavior.cpp /////////////////////////////////////////////////////////////////////////
// Author: Graham Smallwood, July 2002
// Desc:   Behavior that reacts to poison Damage by continuously damaging us further in an Update
///////////////////////////////////////////////////////////////////////////////////////////////////


// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file int the GameEngine

#define DEFINE_DEATH_NAMES

#include "Common/Xfer.h"
#include "GameClient/Drawable.h"
#include "GameLogic/Module/PoisonedBehavior.h"
#include "GameLogic/Damage.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/Object.h"



// tinting is all handled in drawable, now, Graham look near the bottom of Drawable::UpdateDrawable()
//static const RGBColor poisonedTint = {0.0f, 1.0f, 0.0f};

//-------------------------------------------------------------------------------------------------
PoisonedBehaviorModuleData::PoisonedBehaviorModuleData()
{
	m_poisonDamageIntervalData = 0; // How often I retake poison damage dealt me
	m_poisonDurationData = 0;				// And how long after the last poison dose I am poisoned
	m_poisonDamage = 0.0f;
	m_poisonDamageMultiplier = 1.0f;
	m_damageType = DAMAGE_POISON;
	m_damageTypeFX = DAMAGE_POISON;
	m_deathType = DEATH_NONE;
	m_damageStatusType = OBJECT_STATUS_NONE;
	m_customDamageType = NULL;
	m_customDamageStatusType = NULL;
	m_customDeathType = NULL;
	m_statusDuration = 0.0f;
	m_doStatusDamage = FALSE;
	m_statusDurationTypeCorrelate = FALSE;
	m_poisonUnpurgable = FALSE;
	m_tintStatus = TINT_STATUS_POISONED;
	m_customTintStatus = NULL;
	m_damageTypesReaction.first = setDamageTypeFlag(DAMAGE_TYPE_FLAGS_NONE, DAMAGE_POISON);
	m_damageTypesReaction.second.format("NONE");
	m_damageTypesCure.first = DAMAGE_TYPE_FLAGS_NONE;
	m_damageTypesCure.second.format("NONE");
	m_customDamageTypesReaction.clear();
	m_customDamageTypesCure.clear();
	m_requiredCustomStatus.clear();
	m_forbiddenCustomStatus.clear();

}

//-------------------------------------------------------------------------------------------------
/*static*/ void PoisonedBehaviorModuleData::buildFieldParse(MultiIniFieldParse& p) 
{

	static const FieldParse dataFieldParse[] = 
	{
		{ "PoisonDamageInterval", INI::parseDurationUnsignedInt, NULL, offsetof(PoisonedBehaviorModuleData, m_poisonDamageIntervalData) },
		{ "PoisonDuration", INI::parseDurationUnsignedInt, NULL, offsetof(PoisonedBehaviorModuleData, m_poisonDurationData) },
		
		{ "PoisonDamage", 							INI::parseReal, 						NULL, 							offsetof(PoisonedBehaviorModuleData, m_poisonDamage) },
		{ "PoisonDamageMultiplier", 				INI::parseReal, 						NULL, 							offsetof(PoisonedBehaviorModuleData, m_poisonDamageMultiplier) },
		{ "PoisonDamageType",						DamageTypeFlags::parseSingleBitFromINI,	NULL,							offsetof(PoisonedBehaviorModuleData, m_damageType) },		
		{ "PoisonDamageTypeFX",						DamageTypeFlags::parseSingleBitFromINI,	NULL,							offsetof(PoisonedBehaviorModuleData, m_damageTypeFX) },		
		{ "PoisonDeathType",						INI::parseIndexList,								TheDeathNames,		offsetof(PoisonedBehaviorModuleData, m_deathType) },		
		
		{ "PoisonCustomDamageType",					INI::parseAsciiString,							NULL,					offsetof(PoisonedBehaviorModuleData, m_customDamageType) },		
		{ "PoisonCustomDamageStatusType",			INI::parseAsciiString,							NULL,					offsetof(PoisonedBehaviorModuleData, m_customDamageStatusType) },		
		{ "PoisonCustomDeathType",					INI::parseAsciiString,							NULL,					offsetof(PoisonedBehaviorModuleData, m_customDeathType) },
		
		{ "PoisonDamageStatusType",					ObjectStatusMaskType::parseSingleBitFromINI,	NULL,					offsetof(PoisonedBehaviorModuleData, m_damageStatusType) },		
		{ "PoisonDoStatusDamageType",				INI::parseBool,								NULL,						offsetof(PoisonedBehaviorModuleData, m_doStatusDamage) },	
		{ "PoisonStatusDuration",					INI::parseReal,								NULL, 						offsetof(PoisonedBehaviorModuleData, m_statusDuration) },
		{ "PoisonStatusDurationDamageCorrelation",	INI::parseBool,								NULL,						offsetof(PoisonedBehaviorModuleData, m_statusDurationTypeCorrelate) },	
		{ "PoisonStatusTintStatus",					TintStatusFlags::parseSingleBitFromINI,		NULL,						offsetof(PoisonedBehaviorModuleData, m_tintStatus) },	
		{ "PoisonStatusCustomTintStatus",			INI::parseQuotedAsciiString,				NULL, 						offsetof(PoisonedBehaviorModuleData, m_customTintStatus) },

		{ "DamageTypesReaction", 					INI::parseDamageTypeFlagsCustom, 		NULL, 							offsetof(PoisonedBehaviorModuleData, m_damageTypesReaction) },
		{ "CustomDamageTypesReaction", 				INI::parseCustomTypes, 					NULL, 							offsetof(PoisonedBehaviorModuleData, m_customDamageTypesReaction) },	
		{ "DontCurePoisonOnHeal",					INI::parseBool,							NULL,							offsetof(PoisonedBehaviorModuleData, m_poisonUnpurgable) },
		{ "DamageTypesCurePoison", 					INI::parseDamageTypeFlagsCustom, 		NULL, 							offsetof(PoisonedBehaviorModuleData, m_damageTypesCure) },
		{ "CustomDamageTypesCurePoison", 			INI::parseCustomTypes, 					NULL, 							offsetof(PoisonedBehaviorModuleData, m_customDamageTypesCure) },	

		{ "RequiredStatus",							ObjectStatusMaskType::parseFromINI,		NULL, 							offsetof(PoisonedBehaviorModuleData, m_requiredStatus ) },
		{ "ForbiddenStatus",						ObjectStatusMaskType::parseFromINI,		NULL, 							offsetof(PoisonedBehaviorModuleData, m_forbiddenStatus ) },
		{ "RequiredCustomStatus",					INI::parseAsciiStringVector,			NULL, 							offsetof(PoisonedBehaviorModuleData, m_requiredCustomStatus ) },
		{ "ForbiddenCustomStatus",					INI::parseAsciiStringVector,			NULL, 							offsetof(PoisonedBehaviorModuleData, m_forbiddenCustomStatus ) },

		{ 0, 0, 0, 0 }
	};

  UpdateModuleData::buildFieldParse(p);
  p.add(dataFieldParse);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PoisonedBehavior::PoisonedBehavior( Thing *thing, const ModuleData* moduleData ) : UpdateModule( thing, moduleData )
{
	m_poisonDamageFrame = 0;
	m_poisonOverallStopFrame = 0;
	m_poisonDamageAmount = 0.0f;
	//DeathType dt = (DeathType)INI::scanIndexList(getPoisonedBehaviorModuleData()->m_deathTypeName.str(), TheDeathNames);
	//if(dt != DEATH_NONE)
	//	m_deathType = dt;
	//else
	m_deathType = DEATH_POISONED;
	setWakeFrame(getObject(), UPDATE_SLEEP_FOREVER);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PoisonedBehavior::~PoisonedBehavior( void )
{
}

//-------------------------------------------------------------------------------------------------
/** Damage has been dealt, this is an opportunity to react to that damage */
//-------------------------------------------------------------------------------------------------
void PoisonedBehavior::onDamage( DamageInfo *damageInfo )
{
	// @bugfix hanfield 01/08/2025 Check m_sourceID to see if we are causing damage. If we are - ignore.
	if(damageInfo->in.m_sourceID == INVALID_ID) 
		return;

	// Universal Poison Trigger
	if(damageInfo->in.m_isPoison == TRUE)
	{
		startPoisonedEffects( damageInfo ); 
		return;
	}

	const PoisonedBehaviorModuleData* d = getPoisonedBehaviorModuleData();

	// right type to cure poison?
	if(damageInfo->in.m_customDamageType.isEmpty())
	{
		if (getDamageTypeFlag(d->m_damageTypesCure.first, damageInfo->in.m_damageType))
		{
			stopPoisonedEffects();
			setWakeFrame(getObject(), UPDATE_SLEEP_FOREVER);
			return;
		}
	}
	else
	{
		if(getCustomTypeFlag(d->m_damageTypesCure.second, d->m_customDamageTypesCure, damageInfo->in.m_customDamageType))
		{
			stopPoisonedEffects();
			setWakeFrame(getObject(), UPDATE_SLEEP_FOREVER);
			return;
		}
	}

	// right type?
	if(damageInfo->in.m_customDamageType.isEmpty())
	{
		if (!getDamageTypeFlag(d->m_damageTypesReaction.first, damageInfo->in.m_damageType))
			return;
	}
	else
	{
		if(!getCustomTypeFlag(d->m_damageTypesReaction.second, d->m_customDamageTypesReaction, damageInfo->in.m_customDamageType))
			return;
	}

	const Object *obj = getObject();

	//We need all required status or else we fail
	// If we have any requirements
	if( d->m_requiredStatus.any()  &&  !obj->getStatusBits().testForAll( d->m_requiredStatus ) )
		return; 

	//If we have any forbidden statii, then fail
	if( obj->getStatusBits().testForAny( d->m_forbiddenStatus ) )
		return; 

	for(std::vector<AsciiString>::const_iterator it = d->m_requiredCustomStatus.begin(); it != d->m_requiredCustomStatus.end(); ++it)
	{
		ObjectCustomStatusType::const_iterator it2 = obj->getCustomStatus().find(*it);
		if (it2 != obj->getCustomStatus().end()) 
		{
			if(it2->second == 0)
				return;
		}
		else
		{
			return;
		}
	}

	for(std::vector<AsciiString>::const_iterator it = d->m_forbiddenCustomStatus.begin(); it != d->m_forbiddenCustomStatus.end(); ++it)
	{
		ObjectCustomStatusType::const_iterator it2 = obj->getCustomStatus().find(*it);
		if (it2 != obj->getCustomStatus().end() && it2->second == 1) 
			return;
	}

	startPoisonedEffects( damageInfo );      
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void PoisonedBehavior::onHealing( DamageInfo *damageInfo )
{
	if(getPoisonedBehaviorModuleData()->m_poisonUnpurgable == TRUE)
		return;
	
	stopPoisonedEffects();

	setWakeFrame(getObject(), UPDATE_SLEEP_FOREVER);
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
UpdateSleepTime PoisonedBehavior::update()
{
	const PoisonedBehaviorModuleData* d = getPoisonedBehaviorModuleData();
	UnsignedInt now = TheGameLogic->getFrame();

	if( m_poisonOverallStopFrame == 0 )
	{
		DEBUG_CRASH(("hmm, this should not happen"));
		return UPDATE_SLEEP_FOREVER;
		//we aren't poisoned, so nevermind
	}

	if (m_poisonDamageFrame != 0 && now >= m_poisonDamageFrame)
	{
		// If it is time to do damage, then do it and reset the damage timer
		DamageInfo damage;
		damage.in.m_amount = m_poisonDamageAmount;
		damage.in.m_sourceID = INVALID_ID;
		damage.in.m_damageType = d->m_damageType; // @bugfix hanfield 01/08/2025 Since we now check for sourceID, this damage will not cause an infinite poison loop
		damage.in.m_damageFXOverride = d->m_damageTypeFX; // Not necessary anymore, but can help to make sure proper FX are used, if template is wonky
		damage.in.m_deathType = m_deathType;

		damage.in.m_damageStatusType = d->m_damageStatusType;
		damage.in.m_customDamageType = d->m_customDamageType;
		damage.in.m_customDamageStatusType = d->m_customDamageStatusType;
		damage.in.m_customDeathType = d->m_customDeathType;
		damage.in.m_statusDuration = d->m_statusDuration;
		damage.in.m_doStatusDamage = d->m_doStatusDamage;
		damage.in.m_statusDurationTypeCorrelate = d->m_statusDurationTypeCorrelate;
		damage.in.m_tintStatus = d->m_tintStatus;
		damage.in.m_customTintStatus = d->m_customTintStatus;

		getObject()->attemptDamage( &damage );

		m_poisonDamageFrame = now + d->m_poisonDamageIntervalData;
	}

	// If we are now at zero we need to turn off our special effects... 
	// unless the poison killed us, then we continue to be a pulsating toxic pus ball
	if( m_poisonOverallStopFrame != 0 && 
			now >= m_poisonOverallStopFrame &&
			!getObject()->isEffectivelyDead())
	{
		stopPoisonedEffects();
	}

	return calcSleepTime();
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
UpdateSleepTime PoisonedBehavior::calcSleepTime()
{
	// UPDATE_SLEEP requires a count-of-frames, not an absolute-frame, so subtract 'now' 
	UnsignedInt now = TheGameLogic->getFrame();
	if (m_poisonOverallStopFrame == 0 || m_poisonOverallStopFrame == now)
		return UPDATE_SLEEP_FOREVER;
	return frameToSleepTime(m_poisonDamageFrame, m_poisonOverallStopFrame);
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void PoisonedBehavior::startPoisonedEffects( const DamageInfo *damageInfo )
{
	const PoisonedBehaviorModuleData* d = getPoisonedBehaviorModuleData();
	UnsignedInt now = TheGameLogic->getFrame();

	// We are going to take the damage dealt by the original poisoner every so often for a while.
	if(d->m_poisonDamage != 0.0f)
		m_poisonDamageAmount = d->m_poisonDamage;
	else
		m_poisonDamageAmount = damageInfo->out.m_actualDamageDealt * d->m_poisonDamageMultiplier;
	
	m_poisonOverallStopFrame = now + d->m_poisonDurationData;

	// If we are getting re-poisoned, don't reset the damage counter if running, but do set it if unset
	if( m_poisonDamageFrame != 0 )
		m_poisonDamageFrame = min( m_poisonDamageFrame, now + d->m_poisonDamageIntervalData );
	else
		m_poisonDamageFrame = now + d->m_poisonDamageIntervalData;

	if(d->m_deathType == DEATH_NONE)
		m_deathType = damageInfo->in.m_deathType;
	else
		m_deathType = d->m_deathType;

	Drawable *myDrawable = getObject()->getDrawable();
	if( myDrawable )
	{
		if(!d->m_customTintStatus.isEmpty())
		{
			myDrawable->setCustomTintStatus(d->m_customTintStatus);
		}
		else if (d->m_tintStatus > TINT_STATUS_INVALID && d->m_tintStatus < TINT_STATUS_COUNT)
		{
			myDrawable->setTintStatus( d->m_tintStatus );// Graham, It has changed, see UpdateDrawable()
		}
	}

	setWakeFrame(getObject(), calcSleepTime());
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void PoisonedBehavior::stopPoisonedEffects()
{
	m_poisonDamageFrame = 0;
	m_poisonOverallStopFrame = 0;
	m_poisonDamageAmount = 0.0f;

	Drawable *myDrawable = getObject()->getDrawable();
	if( myDrawable )
	{
		const PoisonedBehaviorModuleData* d = getPoisonedBehaviorModuleData();
		
		if(!d->m_customTintStatus.isEmpty())
		{
			myDrawable->clearCustomTintStatus();
		}
		else if (d->m_tintStatus > TINT_STATUS_INVALID && d->m_tintStatus < TINT_STATUS_COUNT)
		{
			myDrawable->clearTintStatus( d->m_tintStatus );// Graham, It has changed, see UpdateDrawable()
		}
	}
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void PoisonedBehavior::crc( Xfer *xfer )
{

	// extend base class
	UpdateModule::crc( xfer );

}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void PoisonedBehavior::xfer( Xfer *xfer )
{

	// version 
	const XferVersion currentVersion = 2;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	UpdateModule::xfer( xfer );

	// poisoned damage frame
	xfer->xferUnsignedInt( &m_poisonDamageFrame );

	// poison overall stop frame
	xfer->xferUnsignedInt( &m_poisonOverallStopFrame );

	// poison damage amount
	xfer->xferReal( &m_poisonDamageAmount );

	if (version >= 2)
	{
		xfer->xferUser(&m_deathType, sizeof(m_deathType));
	}

}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void PoisonedBehavior::loadPostProcess( void )
{

	// extend base class
	UpdateModule::loadPostProcess();

}  // end loadPostProcess

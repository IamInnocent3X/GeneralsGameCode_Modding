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

// FILE: FireWeaponWhenDamagedBehavior.cpp ///////////////////////////////////////////////////////////////////////
// Author:
// Desc:
///////////////////////////////////////////////////////////////////////////////////////////////////


// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine
#define DEFINE_SLOWDEATHPHASE_NAMES
#define DEFINE_WEAPONSLOTTYPE_NAMES  //TheWeaponSlotTypeNamesLookupList

#include "Common/Thing.h"
#include "Common/ThingTemplate.h"
#include "Common/INI.h"
#include "Common/RandomValue.h"
#include "Common/Xfer.h"
#include "Common/Player.h"
#include "GameClient/Drawable.h"
#include "GameClient/FXList.h"
#include "GameClient/InGameUI.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/Module/BodyModule.h"
#include "GameLogic/Module/FireWeaponWhenDamagedBehavior.h"
#include "GameLogic/Module/AIUpdate.h"
#include "GameLogic/Object.h"
#include "GameLogic/ObjectCreationList.h"
#include "GameLogic/Weapon.h"
#include "GameLogic/WeaponSet.h"
#include "GameClient/Drawable.h"

const Int MAX_IDX = 32;

const Real BEGIN_MIDPOINT_RATIO = 0.35f;
const Real END_MIDPOINT_RATIO = 0.65f;

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
FireWeaponWhenDamagedBehavior::FireWeaponWhenDamagedBehavior( Thing *thing, const ModuleData* moduleData ) :
	UpdateModule( thing, moduleData ),
	m_reactionWeaponPristine( nullptr ),
	m_reactionWeaponDamaged( nullptr ),
	m_reactionWeaponReallyDamaged( nullptr ),
	m_reactionWeaponRubble( nullptr ),
	m_continuousWeaponPristine( nullptr ),
	m_continuousWeaponDamaged( nullptr ),
	m_continuousWeaponReallyDamaged( nullptr ),
	m_continuousWeaponRubble( nullptr ),
	m_duration( 0 ),
	m_interval( 0 ),
	m_innate( TRUE )
{

	const FireWeaponWhenDamagedBehaviorModuleData *d = getFireWeaponWhenDamagedBehaviorModuleData();
	const Object* obj = getObject();

	WeaponSlotType wslot = (WeaponSlotType)INI::scanLookupList(d->m_weaponSlotName.str(), TheWeaponSlotTypeNamesLookupList);

	if ( d->m_reactionWeaponPristine )
	{
		m_reactionWeaponPristine				= TheWeaponStore->allocateNewWeapon(
			d->m_reactionWeaponPristine,					wslot);
		m_reactionWeaponPristine->reloadAmmo( obj );
	}
	if ( d->m_reactionWeaponDamaged )
	{
		m_reactionWeaponDamaged					= TheWeaponStore->allocateNewWeapon(
			d->m_reactionWeaponDamaged,					wslot);
		m_reactionWeaponDamaged->reloadAmmo( obj );
	}
	if ( d->m_reactionWeaponReallyDamaged )
	{
		m_reactionWeaponReallyDamaged		= TheWeaponStore->allocateNewWeapon(
			d->m_reactionWeaponReallyDamaged,		wslot);
		m_reactionWeaponReallyDamaged->reloadAmmo( obj );
	}
	if ( d->m_reactionWeaponRubble )
	{
		m_reactionWeaponRubble					= TheWeaponStore->allocateNewWeapon(
			d->m_reactionWeaponRubble,						wslot);
		m_reactionWeaponRubble->reloadAmmo( obj );
	}


	if ( d->m_continuousWeaponPristine )
	{
		m_continuousWeaponPristine			= TheWeaponStore->allocateNewWeapon(
			d->m_continuousWeaponPristine,				wslot);
		m_continuousWeaponPristine->reloadAmmo( obj );
	}
	if ( d->m_continuousWeaponDamaged )
	{
		m_continuousWeaponDamaged				= TheWeaponStore->allocateNewWeapon(
			d->m_continuousWeaponDamaged,				wslot);
		m_continuousWeaponDamaged->reloadAmmo( obj );
	}
	if ( d->m_continuousWeaponReallyDamaged )
	{
		m_continuousWeaponReallyDamaged = TheWeaponStore->allocateNewWeapon(
			d->m_continuousWeaponReallyDamaged,	wslot);
		m_continuousWeaponReallyDamaged->reloadAmmo( obj );
	}
	if ( d->m_continuousWeaponRubble )
	{
		m_continuousWeaponRubble				= TheWeaponStore->allocateNewWeapon(
			d->m_continuousWeaponRubble,					wslot);
		m_continuousWeaponRubble->reloadAmmo( obj );
	}

	//if (checkStartsActive())
	//{
	//	giveSelfUpgrade();
	//}

	if (d->m_continuousDurationPristine > 0 )
	{
		m_innate = FALSE;
	}

	if (isUpgradeActive() &&
			(d->m_continuousWeaponPristine != nullptr ||
			d->m_continuousWeaponDamaged != nullptr ||
			d->m_continuousWeaponReallyDamaged != nullptr ||
			d->m_continuousWeaponRubble != nullptr))
	{
		setWakeFrame(getObject(), UPDATE_SLEEP_NONE);
	}
	else
	{
		setWakeFrame(getObject(), UPDATE_SLEEP_FOREVER);
	}

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
FireWeaponWhenDamagedBehavior::~FireWeaponWhenDamagedBehavior()
{
	deleteInstance(m_reactionWeaponPristine);
	deleteInstance(m_reactionWeaponDamaged);
	deleteInstance(m_reactionWeaponReallyDamaged);
	deleteInstance(m_reactionWeaponRubble);

	deleteInstance(m_continuousWeaponPristine);
	deleteInstance(m_continuousWeaponDamaged);
	deleteInstance(m_continuousWeaponReallyDamaged);
	deleteInstance(m_continuousWeaponRubble);
}

//-------------------------------------------------------------------------------------------------
/** Damage has been dealt, this is an opportunity to reach to that damage */
//-------------------------------------------------------------------------------------------------
void FireWeaponWhenDamagedBehavior::onDamage( DamageInfo *damageInfo )
{
	if (!isUpgradeActive())
		return;

	const FireWeaponWhenDamagedBehaviorModuleData* d = getFireWeaponWhenDamagedBehaviorModuleData();

	// right type?
	if(damageInfo->in.m_customDamageType.isEmpty())
	{
		if (!getDamageTypeFlag(d->m_damageTypesCustom.first, damageInfo->in.m_damageType))
			return;
	}
	else
	{
		if(!getCustomTypeFlag(d->m_damageTypesCustom.second, d->m_customDamageTypes, damageInfo->in.m_customDamageType))
			return;
	}
	// right amount? (use actual [post-armor] damage dealt)
	if (damageInfo->out.m_actualDamageDealt < d->m_damageAmount)
		return;

	const Object *obj = getObject();

	//We need all required status or else we fail
	// If we have any requirements
	if( d->m_requiredStatus.any()  &&  !obj->getStatusBits().testForAll( d->m_requiredStatus ) )
		return; 

	//If we have any forbidden statii, then fail
	if( obj->getStatusBits().testForAny( d->m_forbiddenStatus ) )
		return; 

	if(!obj->testCustomStatusForAll(d->m_requiredCustomStatus))
		return;

	for(std::vector<AsciiString>::const_iterator it = d->m_forbiddenCustomStatus.begin(); it != d->m_forbiddenCustomStatus.end(); ++it)
	{
		if(obj->testCustomStatus(*it))
			return;
	}

	BodyDamageType bdt = obj->getBodyModule()->getDamageState();

	UnsignedInt now = TheGameLogic->getFrame();
	m_duration = 0;
	m_innate = TRUE;

	getObject()->setContainedPosition();

	if ( bdt == BODY_RUBBLE )
	{
		if( m_reactionWeaponRubble && m_reactionWeaponRubble->getStatus() == READY_TO_FIRE )
		{
			if(!m_reactionWeaponRubble->getTemplate()->passRequirements(obj))
				return;

			m_reactionWeaponRubble->forceFireWeapon( obj, obj->getPosition() );
		}
		if(d->m_continuousDurationRubble > 0)
		{
			m_duration =  now + d->m_continuousDurationRubble;
			m_innate = FALSE;
			setWakeFrame(getObject(), UPDATE_SLEEP_NONE);
		}

	}
	else if ( bdt == BODY_REALLYDAMAGED )
	{
		if( m_reactionWeaponReallyDamaged && m_reactionWeaponReallyDamaged->getStatus() == READY_TO_FIRE )
		{
			if(!m_reactionWeaponReallyDamaged->getTemplate()->passRequirements(obj))
				return;

			m_reactionWeaponReallyDamaged->forceFireWeapon( obj, obj->getPosition() );
		}
		if(d->m_continuousDurationReallyDamaged > 0)
		{
			m_duration = now + d->m_continuousDurationReallyDamaged;
			m_innate = FALSE;
			setWakeFrame(getObject(), UPDATE_SLEEP_NONE);
		}
	}
	else if ( bdt == BODY_DAMAGED )
	{
		if( m_reactionWeaponDamaged && m_reactionWeaponDamaged->getStatus() == READY_TO_FIRE )
		{
			if(!m_reactionWeaponDamaged->getTemplate()->passRequirements(obj))
				return;

			m_reactionWeaponDamaged->forceFireWeapon( obj, obj->getPosition() );
		}
		if(d->m_continuousDurationDamaged > 0)
		{
			m_duration = now + d->m_continuousDurationDamaged;
			m_innate = FALSE;
			setWakeFrame(getObject(), UPDATE_SLEEP_NONE);
		}
	}
	else // not damaged yet
	{
		if( m_reactionWeaponPristine && m_reactionWeaponPristine->getStatus() == READY_TO_FIRE )
		{
			if(!m_reactionWeaponPristine->getTemplate()->passRequirements(obj))
				return;

			m_reactionWeaponPristine->forceFireWeapon( obj, obj->getPosition() );
		}
		if(d->m_continuousDurationPristine > 0)
		{
			m_duration = now + d->m_continuousDurationPristine;
			m_innate = FALSE;
			setWakeFrame(getObject(), UPDATE_SLEEP_NONE);
		}
	}

}

//-------------------------------------------------------------------------------------------------
/** if object fires weapon constantly, figure out which one and do it */
//-------------------------------------------------------------------------------------------------
UpdateSleepTime FireWeaponWhenDamagedBehavior::update()
{
	if (!isUpgradeActive())
	{
		DEBUG_ASSERTCRASH(isUpgradeActive(), ("hmm, this should not be possible"));
		return UPDATE_SLEEP_FOREVER;
	}

	const FireWeaponWhenDamagedBehaviorModuleData* d = getFireWeaponWhenDamagedBehaviorModuleData();

	const Object *obj = getObject();
	BodyDamageType bdt = obj->getBodyModule()->getDamageState();

	UnsignedInt now = TheGameLogic->getFrame();

	if ( bdt == BODY_RUBBLE )
	{
		if( m_continuousWeaponRubble && m_continuousWeaponRubble->getStatus() == READY_TO_FIRE && m_continuousWeaponRubble->getTemplate()->passRequirements(obj) && ( m_innate == TRUE || m_duration > now ) && now > m_interval )
		{
			getObject()->setContainedPosition();
			m_continuousWeaponRubble->forceFireWeapon( obj, obj->getPosition() );
			WeaponBonus bonus;
			UnsignedInt delay = d->m_continuousWeaponRubble->getDelayBetweenShots( bonus );
			m_interval = now + (d->m_continuousIntervalRubble > delay ? d->m_continuousIntervalRubble : delay);
		}

	}
	else if ( bdt == BODY_REALLYDAMAGED )
	{
		if( m_continuousWeaponReallyDamaged && m_continuousWeaponReallyDamaged->getStatus() == READY_TO_FIRE && m_continuousWeaponReallyDamaged->getTemplate()->passRequirements(obj) && ( m_innate == TRUE || m_duration > now ) && now > m_interval )
		{
			getObject()->setContainedPosition();
			m_continuousWeaponReallyDamaged->forceFireWeapon( obj, obj->getPosition() );
			WeaponBonus bonus;
			UnsignedInt delay = d->m_continuousWeaponReallyDamaged->getDelayBetweenShots( bonus );
			m_interval = now + (d->m_continuousIntervalReallyDamaged > delay ? d->m_continuousIntervalReallyDamaged : delay);
		}
	}
	else if ( bdt == BODY_DAMAGED )
	{
		if( m_continuousWeaponDamaged && m_continuousWeaponDamaged->getStatus() == READY_TO_FIRE && m_continuousWeaponDamaged->getTemplate()->passRequirements(obj) && ( m_innate == TRUE || m_duration > now ) && now > m_interval )
		{
			getObject()->setContainedPosition();
			m_continuousWeaponDamaged->forceFireWeapon( obj, obj->getPosition() );
			WeaponBonus bonus;
			UnsignedInt delay = d->m_continuousWeaponDamaged->getDelayBetweenShots( bonus );
			m_interval = now + (d->m_continuousIntervalDamaged > delay ? d->m_continuousIntervalDamaged : delay);
		}
	}
	else // not damaged yet
	{
		if( m_continuousWeaponPristine && m_continuousWeaponPristine->getStatus() == READY_TO_FIRE && m_continuousWeaponPristine->getTemplate()->passRequirements(obj) && ( m_innate == TRUE || m_duration > now ) && now > m_interval )
		{
			getObject()->setContainedPosition();
			m_continuousWeaponPristine->forceFireWeapon( obj, obj->getPosition() );
			WeaponBonus bonus;
			UnsignedInt delay = d->m_continuousWeaponPristine->getDelayBetweenShots( bonus );
			m_interval = now + (d->m_continuousIntervalPristine > delay ? d->m_continuousIntervalPristine : delay);
		}
	}

	return UPDATE_SLEEP( m_interval >= now ? m_interval - now + 1 : UPDATE_SLEEP_NONE );
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void FireWeaponWhenDamagedBehavior::crc( Xfer *xfer )
{

	// extend base class
	UpdateModule::crc( xfer );

	// extend upgrade mux
	UpgradeMux::upgradeMuxCRC( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void FireWeaponWhenDamagedBehavior::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	UpdateModule::xfer( xfer );

	// extend upgrade mux
	UpgradeMux::upgradeMuxXfer( xfer );

	Bool weaponPresent;

	// reaction pristine
	weaponPresent = m_reactionWeaponPristine ? TRUE : FALSE;
	xfer->xferBool( &weaponPresent );
	if( weaponPresent )
		xfer->xferSnapshot( m_reactionWeaponPristine );

	// reaction damaged
	weaponPresent = m_reactionWeaponDamaged ? TRUE : FALSE;
	xfer->xferBool( &weaponPresent );
	if( weaponPresent )
		xfer->xferSnapshot( m_reactionWeaponDamaged );

	// reaction really damaged
	weaponPresent = m_reactionWeaponReallyDamaged ? TRUE : FALSE;
	xfer->xferBool( &weaponPresent );
	if( weaponPresent )
		xfer->xferSnapshot( m_reactionWeaponReallyDamaged );

	// reaction rubble
	weaponPresent = m_reactionWeaponRubble ? TRUE : FALSE;
	xfer->xferBool( &weaponPresent );
	if( weaponPresent )
		xfer->xferSnapshot( m_reactionWeaponRubble );

	// continuous pristine
	weaponPresent = m_continuousWeaponPristine ? TRUE : FALSE;
	xfer->xferBool( &weaponPresent );
	if( weaponPresent )
		xfer->xferSnapshot( m_continuousWeaponPristine );

	// continuous damaged
	weaponPresent = m_continuousWeaponDamaged ? TRUE : FALSE;
	xfer->xferBool( &weaponPresent );
	if( weaponPresent )
		xfer->xferSnapshot( m_continuousWeaponDamaged );

	// continuous really damaged
	weaponPresent = m_continuousWeaponReallyDamaged ? TRUE : FALSE;
	xfer->xferBool( &weaponPresent );
	if( weaponPresent )
		xfer->xferSnapshot( m_continuousWeaponReallyDamaged );

	// continuous rubble
	weaponPresent = m_continuousWeaponRubble ? TRUE : FALSE;
	xfer->xferBool( &weaponPresent );
	if( weaponPresent )
		xfer->xferSnapshot( m_continuousWeaponRubble );

	xfer->xferUnsignedInt( &m_duration );
	xfer->xferUnsignedInt( &m_interval );
	xfer->xferBool( &m_innate );

}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void FireWeaponWhenDamagedBehavior::loadPostProcess()
{

	// extend base class
	UpdateModule::loadPostProcess();

	// extend upgrade mux
	UpgradeMux::upgradeMuxLoadPostProcess();

}

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

// FILE: FireOCLAfterWeaponCooldownUpdate.cpp //////////////////////////////////////////////////////////////////////
// Author: Kris Morness, September 2002
// Desc:   This system tracks the objects status with regards to firing, and whenever the object stops
//         firing, and all the conditions are met, then it'll create the specified OCL.
///////////////////////////////////////////////////////////////////////////////////////////////////

// USER INCLUDES //////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#define DEFINE_WEAPONSLOTTYPE_NAMES  //TheWeaponSlotTypeNamesLookupList

#include "Common/Player.h"
#include "Common/ThingTemplate.h"
#include "Common/Xfer.h"

#include "GameClient/Drawable.h"
#include "GameClient/InGameUI.h"

#include "GameLogic/GameLogic.h"
#include "GameLogic/Object.h"
#include "GameLogic/ObjectCreationList.h"
#include "GameLogic/Weapon.h"
#include "GameLogic/WeaponSet.h"

#include "GameLogic/Module/FireOCLAfterWeaponCooldownUpdate.h"
#include "GameLogic/Module/LifetimeUpdate.h"
#include "GameLogic/Module/AIUpdate.h"
#include "GameLogic/Module/BodyModule.h"


//-------------------------------------------------------------------------------------------------
FireOCLAfterWeaponCooldownUpdateModuleData::FireOCLAfterWeaponCooldownUpdateModuleData()
{
	m_weaponSlot						= PRIMARY_WEAPON;
	m_minShotsRequired			= 1;
	m_oclLifetimePerSecond	= 1000;
	m_oclMaxFrames					= 1000;
	m_ocl										= nullptr;
}

//-------------------------------------------------------------------------------------------------
void FireOCLAfterWeaponCooldownUpdateModuleData::buildFieldParse(MultiIniFieldParse& p)
{
  UpdateModuleData::buildFieldParse(p);
	static const FieldParse dataFieldParse[] =
	{
		{ "WeaponSlot",						INI::parseLookupList,						TheWeaponSlotTypeNamesLookupList, offsetof( FireOCLAfterWeaponCooldownUpdateModuleData, m_weaponSlot ) },
		{ "OCL",									INI::parseObjectCreationList,		nullptr, offsetof( FireOCLAfterWeaponCooldownUpdateModuleData, m_ocl ) },
		{ "MinShotsToCreateOCL",  INI::parseUnsignedInt,					nullptr, offsetof( FireOCLAfterWeaponCooldownUpdateModuleData, m_minShotsRequired ) },
		{ "OCLLifetimePerSecond",	INI::parseUnsignedInt,					nullptr, offsetof( FireOCLAfterWeaponCooldownUpdateModuleData, m_oclLifetimePerSecond ) },
		{ "OCLLifetimeMaxCap",		INI::parseDurationUnsignedInt,	nullptr, offsetof( FireOCLAfterWeaponCooldownUpdateModuleData, m_oclMaxFrames ) },
		{ nullptr, nullptr, nullptr, 0 }
	};
  p.add(dataFieldParse);
	p.add(UpgradeMuxData::getFieldParse(), offsetof( FireOCLAfterWeaponCooldownUpdateModuleData, m_upgradeMuxData ));
}


//-------------------------------------------------------------------------------------------------
FireOCLAfterWeaponCooldownUpdate::FireOCLAfterWeaponCooldownUpdate( Thing *thing, const ModuleData *moduleData ) : UpdateModule( thing, moduleData )
{
	m_valid = false;
	m_weaponFired = false;
	m_isActive = false;
	resetStats();
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
FireOCLAfterWeaponCooldownUpdate::~FireOCLAfterWeaponCooldownUpdate( void )
{
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
Bool FireOCLAfterWeaponCooldownUpdate::checkStartsActive() const
{
	UpgradeMaskType objectMask = getObject()->getObjectCompletedUpgradeMask();
	UpgradeMaskType playerMask = getObject()->getControllingPlayer()->getCompletedUpgradeMask();
	UpgradeMaskType maskToCheck = playerMask;
	maskToCheck.set( objectMask );

	UpgradeMaskType activation, conflicting;
	getUpgradeActivationMasks(activation, conflicting);

	Bool startsActive = false;
	if(!activation.any())
	{
		startsActive = true;
	}

	if( maskToCheck.any() )
	{
		if(maskToCheck.testForAny( conflicting ))
		{
			startsActive = false;
		}
		else if(activation.any())
		{
			if(requiresAllActivationUpgrades())
				startsActive = maskToCheck.testForAll( activation );
			else
				startsActive = maskToCheck.testForAny( activation );
		}
	}

	startsActive = startsActive || getFireOCLAfterWeaponCooldownUpdateModuleData()->m_upgradeMuxData.muxDataCheckStartsActive(getObject());
	return startsActive;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void FireOCLAfterWeaponCooldownUpdate::upgradeImplementation( )
{
	// Very simple; just need to flag the Object as having the player upgrade, and the WeaponSet chooser
	// will do the work of picking the right one from ini.  This comment is as long as the code. Update: not anymore ;)

	Object *obj = getObject();

	UpgradeMaskType objectMask = obj->getObjectCompletedUpgradeMask();
	UpgradeMaskType playerMask = obj->getControllingPlayer()->getCompletedUpgradeMask();
	UpgradeMaskType maskToCheck = playerMask;
	maskToCheck.set( objectMask );

	//First make sure we have the right combination of upgrades
	Int UpgradeStatus = wouldRefreshUpgrade(maskToCheck, m_isActive);

	// If there's no Upgrade Status, do Nothing;
	if( UpgradeStatus == 0 )
	{
		return;
	}
	else if( UpgradeStatus == 1 )
	{
		// Set to apply upgrade
		m_isActive = TRUE;
	}
	else if( UpgradeStatus == 2 )
	{
		m_isActive = FALSE;
		// Remove the Upgrade Execution Status so it is treated as activation again
		setUpgradeExecuted(false);
	}

	// We stop here since we only get whether the Upgrade is Active
}

//-------------------------------------------------------------------------------------------------
UpdateSleepTime FireOCLAfterWeaponCooldownUpdate::update( void )
{
	const FireOCLAfterWeaponCooldownUpdateModuleData* data = getFireOCLAfterWeaponCooldownUpdateModuleData();
	//UpgradeMaskType activation, conflicting;
	//getUpgradeActivationMasks( activation, conflicting );
	Bool validThisFrame = true;
	Bool validToFireOCL = true;
	Object *obj = getObject();

	//Get current weapon
	Weapon *weapon = obj->getCurrentWeapon();
	if( weapon )
	{
		if( weapon->getWeaponSlot() != data->m_weaponSlot )
		{
			//Not the right weapon slot -- it's possible we switched weapons in which case it's
			//possible to fire the OCL.
			validThisFrame = false;
		}
	}
	else
	{
		//No weapon selected -- possible to switch off weapons, possible to have just
		//finished firing our weapon.
		validThisFrame = false;
	}

	//const UpgradeMaskType& objectMask = obj->getObjectCompletedUpgradeMask();
	//const UpgradeMaskType& playerMask = obj->getControllingPlayer()->getCompletedUpgradeMask();
	//UpgradeMaskType maskToCheck = playerMask;
	//maskToCheck.set( objectMask );
	// IamInnocent - changed the condition to Upgrade Implementation
	if( validThisFrame && !m_isActive ) //!testUpgradeConditions( maskToCheck ) )
	{
		//Can't use this period if this object doesn't have any of the upgrades
		validThisFrame = false;
		validToFireOCL = false;
	}

	UnsignedInt now = TheGameLogic->getFrame();
	if( validThisFrame )
	{
		//if( weapon->getLastShotFrame() == now - 1 )
		if( m_weaponFired )
		{
			m_checkFrame = weapon->getPossibleNextShotFrame();
			m_consecutiveShots++;
			if( m_consecutiveShots == 1 )
			{
				//Our first shot -- so record the frame number so we can easily calculate the
				//time we've been firing!
				m_startFrame = now;
			}
		}
		else if( m_checkFrame && m_checkFrame < now ) //weapon->getPossibleNextShotFrame() < now )
		{
			//Means we could have shot but didn't!
			m_checkFrame = 0;

			if( data->m_minShotsRequired <= m_consecutiveShots )
			{
				//We have fired enough shots to create the OCL.
				fireOCL();
			}
		}
	}
	else if( validToFireOCL )
	{
		//We've stopped being valid -- which means we may have stopped firing our weapon.
		Weapon *weapon = obj->getWeaponInWeaponSlot( data->m_weaponSlot );
		if( weapon && data->m_minShotsRequired <= m_consecutiveShots )
		{
			//We switched weapons! Fire OCL!
			fireOCL();
		}
	}

	if( validThisFrame != m_valid )
	{
		//There has been a validity change, so reset all values!
		m_valid = validThisFrame;
		resetStats();
	}

	m_weaponFired = FALSE;

	// IamInnocent - Made Sleepy
	return m_checkFrame && m_checkFrame > now ? UPDATE_SLEEP(m_checkFrame + 1 - now) : UPDATE_SLEEP_FOREVER;
	//return UPDATE_SLEEP_NONE;
}

//-------------------------------------------------------------------------------------------------
void FireOCLAfterWeaponCooldownUpdate::refreshUpdateMod(Bool weaponFired)
{
	if(weaponFired)
		m_weaponFired = TRUE;
	setWakeFrame(getObject(), UPDATE_SLEEP_NONE);
}

//-------------------------------------------------------------------------------------------------
void FireOCLAfterWeaponCooldownUpdate::resetStats()
{
	m_consecutiveShots = 0;
	m_startFrame = 0;
	m_checkFrame = 0;
}

//-------------------------------------------------------------------------------------------------
void FireOCLAfterWeaponCooldownUpdate::fireOCL()
{
	const FireOCLAfterWeaponCooldownUpdateModuleData* data = getFireOCLAfterWeaponCooldownUpdateModuleData();
	Object *obj = getObject();

	//Calculate the lifetime of the OCL.
	UnsignedInt now = TheGameLogic->getFrame();
	Real seconds = (now - m_startFrame) * SECONDS_PER_LOGICFRAME_REAL;
	seconds *= data->m_oclLifetimePerSecond * 0.001f;
	UnsignedInt oclFrames = (UnsignedInt)(seconds * LOGICFRAMES_PER_SECOND);
	oclFrames = MIN( oclFrames, data->m_oclMaxFrames );

	ObjectCreationList::create( data->m_ocl, obj, obj, oclFrames );

	resetStats();
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void FireOCLAfterWeaponCooldownUpdate::crc( Xfer *xfer )
{

	// extend base class
	UpdateModule::crc( xfer );

	// extend base class
	UpgradeMux::upgradeMuxCRC( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void FireOCLAfterWeaponCooldownUpdate::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	UpdateModule::xfer( xfer );

	// extend base class
	UpgradeMux::upgradeMuxXfer( xfer );

	// valid
	xfer->xferBool( &m_valid );

	// consecutive shots
	xfer->xferUnsignedInt( &m_consecutiveShots );

	// start frame
	xfer->xferUnsignedInt( &m_startFrame );

	// check time
	xfer->xferUnsignedInt( &m_checkFrame );

	// is active
	xfer->xferBool( &m_isActive );


}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void FireOCLAfterWeaponCooldownUpdate::loadPostProcess( void )
{

	// extend base class
	UpdateModule::loadPostProcess();

	// extend base class
	UpgradeMux::upgradeMuxLoadPostProcess();

}

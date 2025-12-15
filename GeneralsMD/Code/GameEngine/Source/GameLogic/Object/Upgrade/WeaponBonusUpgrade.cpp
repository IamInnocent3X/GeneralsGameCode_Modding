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

// FILE: WeaponBonusUpgrade.cpp /////////////////////////////////////////////////
//-----------------------------------------------------------------------------
//
//                       Electronic Arts Pacific.
//
//                       Confidential Information
//                Copyright (C) 2002 - All Rights Reserved
//
//-----------------------------------------------------------------------------
//
//	created:	May 2002
//
//	Filename: 	WeaponBonusUpgrade.cpp
//
//	author:		Chris Huybregts
//
//	purpose:
//
//-----------------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// SYSTEM INCLUDES ////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// USER INCLUDES //////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "Common/Player.h"
#include "Common/Xfer.h"
#include "GameLogic/Object.h"
#include "GameLogic/Module/WeaponBonusUpgrade.h"
#include "GameLogic/Weapon.h"

//-----------------------------------------------------------------------------
// DEFINES ////////////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// PUBLIC FUNCTIONS ///////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// PRIVATE FUNCTIONS //////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
WeaponBonusUpgrade::WeaponBonusUpgrade( Thing *thing, const ModuleData* moduleData ) : UpgradeModule( thing, moduleData )
{
	m_hasExecuted = FALSE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
WeaponBonusUpgrade::~WeaponBonusUpgrade( void )
{
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void WeaponBonusUpgrade::upgradeImplementation( )
{
	// Very simple; just need to flag the Object as having the player upgrade, and the WeaponSet chooser
	// will do the work of picking the right one from ini.  This comment is as long as the code.
	Object *obj = getObject();

	UpgradeMaskType objectMask = obj->getObjectCompletedUpgradeMask();
	UpgradeMaskType playerMask = obj->getControllingPlayer()->getCompletedUpgradeMask();
	UpgradeMaskType maskToCheck = playerMask;
	maskToCheck.set( objectMask );

	//First make sure we have the right combination of upgrades
	Int UpgradeStatus = wouldRefreshUpgrade(maskToCheck, m_hasExecuted);

	// If there's no Upgrade Status, do Nothing;
	if( UpgradeStatus == 1 )
	{
		m_hasExecuted = TRUE;
		obj->setWeaponBonusCondition( WEAPONBONUSCONDITION_PLAYER_UPGRADE );
	}
	else if( UpgradeStatus == 2 )
	{
		m_hasExecuted = FALSE;
		obj->clearWeaponBonusCondition( WEAPONBONUSCONDITION_PLAYER_UPGRADE );
		// Remove the Upgrade Execution Status so it is treated as activation again
		setUpgradeExecuted(false);
	}

}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void WeaponBonusUpgrade::crc( Xfer *xfer )
{

	// extend base class
	UpgradeModule::crc( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void WeaponBonusUpgrade::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	UpgradeModule::xfer( xfer );

	xfer->xferBool(&m_hasExecuted);

}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void WeaponBonusUpgrade::loadPostProcess( void )
{

	// extend base class
	UpgradeModule::loadPostProcess();

}

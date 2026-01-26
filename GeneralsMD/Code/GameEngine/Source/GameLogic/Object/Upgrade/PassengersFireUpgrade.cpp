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

// FILE: PassengersFireUpgrade.h /////////////////////////////////////////////////////////////////////////////
// Author: Mark Lorenzen, May 2003
// Desc:	 UpgradeModule that sets containmodules flag for passengersAllowedToFire
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "Common/Player.h"
#include "Common/Xfer.h"
#include "GameLogic/Object.h"
#include "GameLogic/Module/PassengersFireUpgrade.h"
#include "GameLogic/Module/ContainModule.h"

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PassengersFireUpgrade::PassengersFireUpgrade( Thing *thing, const ModuleData* moduleData ) : UpgradeModule( thing, moduleData )
{
	m_hasExecuted = FALSE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PassengersFireUpgrade::~PassengersFireUpgrade( void )
{
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void PassengersFireUpgrade::upgradeImplementation( )
{
	// Just need to flag the containmodule having the passengersallowedtofire true, .

  Object *obj = getObject();

  ContainModuleInterface *contain = obj->getContain();
  if ( !contain )
	return;

  const UpgradeMaskType& objectMask = obj->getObjectCompletedUpgradeMask();
  const UpgradeMaskType& playerMask = obj->getControllingPlayer()->getCompletedUpgradeMask();
  UpgradeMaskType maskToCheck = playerMask;
  maskToCheck.set( objectMask );

  //First make sure we have the right combination of upgrades
  Int UpgradeStatus = wouldRefreshUpgrade(maskToCheck, m_hasExecuted);

  // Apply the Upgrade if the Object has not yet been Upgraded
  if( UpgradeStatus == 1 )
  {
  	m_hasExecuted = TRUE;  
	contain->setPassengerAllowedToFire( TRUE );
  }
  else if( UpgradeStatus == 2 )
  {
	m_hasExecuted = FALSE;  
	contain->setPassengerAllowedToFire( FALSE );
	// Remove the Upgrade Execution Status so it is treated as activation again
  	setUpgradeExecuted(false);
  }

//	obj->setWeaponSetFlag( WEAPONSET_PLAYER_UPGRADE );

//  ContainModuleInterface *contain = obj->getContain();
//  if ( contain )
//  {
//    contain->setPassengerAllowedToFire( TRUE );
//  }
//
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void PassengersFireUpgrade::crc( Xfer *xfer )
{

	// extend base class
	UpgradeModule::crc( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void PassengersFireUpgrade::xfer( Xfer *xfer )
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
void PassengersFireUpgrade::loadPostProcess( void )
{

	// extend base class
	UpgradeModule::loadPostProcess();

}

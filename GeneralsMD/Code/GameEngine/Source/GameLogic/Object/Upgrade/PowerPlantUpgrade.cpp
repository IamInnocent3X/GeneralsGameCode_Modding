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

// FILE: PowerPlantUpgrade.cpp /////////////////////////////////////////////////////////////////////////////
// Author: Amit Kumar, August 2002
// Desc:	 Power plant upgrades
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file int the GameEngine

#include "Common/ModelState.h"
#include "Common/Player.h"
#include "Common/Xfer.h"
#include "GameClient/Drawable.h"
#include "GameClient/InGameUI.h"
#include "GameLogic/Object.h"
#include "GameLogic/Module/PowerPlantUpdate.h"
#include "GameLogic/Module/PowerPlantUpgrade.h"

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PowerPlantUpgrade::PowerPlantUpgrade( Thing *thing, const ModuleData* moduleData ) :
							UpgradeModule( thing, moduleData )
{
	m_hasExecuted = FALSE;
}  // end PowerPlantUpgrade

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PowerPlantUpgrade::~PowerPlantUpgrade( void )
{

}  // end ~PowerPlantUpgrade

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void PowerPlantUpgrade::onDelete( void )
{

	// if we haven't been upgraded there is nothing to clean up
	if( isAlreadyUpgraded() == FALSE )
		return;

	// remove the power bonus from the player
	Player *player = getObject()->getControllingPlayer();
	if( player )
		player->removePowerBonus( getObject() );

	// this upgrade module is now "not upgraded"
	setUpgradeExecuted(FALSE);

}  // end onDelete

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void PowerPlantUpgrade::onCapture( Player *oldOwner, Player *newOwner )
{

	// do nothing if we haven't upgraded yet
	if( isAlreadyUpgraded() == FALSE )
		return;

	if (getObject()->isDisabled())
		return;

	// remove power bonus from old owner
	if( oldOwner )
	{

		oldOwner->removePowerBonus( getObject() );
		setUpgradeExecuted(FALSE);

	}

	// add power bonus to the new owner
	if( newOwner )
	{

		newOwner->addPowerBonus( getObject() );
		setUpgradeExecuted(TRUE);

	}

}  // end onCapture

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void PowerPlantUpgrade::upgradeImplementation( void )
{
	Object *obj = getObject();

	UpgradeMaskType objectMask = obj->getObjectCompletedUpgradeMask();
	UpgradeMaskType playerMask = obj->getControllingPlayer()->getCompletedUpgradeMask();
	UpgradeMaskType maskToCheck = playerMask;
	maskToCheck.set( objectMask );

	//First make sure we have the right combination of upgrades
	Int UpgradeStatus = wouldRefreshUpgrade(maskToCheck);

	// Because this module does things differently, we need to take a different approach
	if( UpgradeStatus == 0 )
	{
		// If we do not have the Upgrade, yet we have not executed, do nothing
		if(!m_hasExecuted)
		{
			return;
		}
		else
		{
			// Remove the Upgrade Execution Status so it is treated as activation again
			m_hasExecuted = false;
			setUpgradeExecuted(false);
		}
	}

	Bool isApply = UpgradeStatus == 1 ? TRUE : FALSE;

	if(isApply)
	{
		// If we have yet to do the Upgrade, proceed to do the Upgrade, but if we already have the Upgrade, don't do anything.
		if(!m_hasExecuted)
			m_hasExecuted = isApply;
		else
			return;
	}

	Player *player = getObject()->getControllingPlayer();

	// add the new power production to the object
	if( player )
	{
		if(isApply)
			player->addPowerBonus(getObject());
		else
			player->removePowerBonus( getObject() );
	}


	PowerPlantUpdateInterface *ppui;
	for( BehaviorModule **umi = getObject()->getBehaviorModules(); *umi; ++umi)
	{
		ppui = (*umi)->getPowerPlantUpdateInterface();
		if( ppui )
			ppui->extendRods(isApply);
	}

}  // end upgradeImplementation

void PowerPlantUpgrade::forceRefreshMyUpgrade()
{
	upgradeImplementation();
}
// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void PowerPlantUpgrade::crc( Xfer *xfer )
{

	// extend base class
	UpgradeModule::crc( xfer );

}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void PowerPlantUpgrade::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	UpgradeModule::xfer( xfer );

	xfer->xferBool( &m_hasExecuted );

}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void PowerPlantUpgrade::loadPostProcess( void )
{

	// extend base class
	UpgradeModule::loadPostProcess();

	// Most upgrade modules have state change effects that are themselves saved.  This one is a fire and forget.
	// So we need to re-fire on load if we are turned on.
	if( isAlreadyUpgraded() )
	{
		Player *player = getObject()->getControllingPlayer();

		// add the new power production to the object
		if( player )
			player->addPowerBonus(getObject());
	}

}  // end loadPostProcess

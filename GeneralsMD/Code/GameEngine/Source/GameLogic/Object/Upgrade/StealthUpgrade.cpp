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

// FILE: StealthUpgrade.cpp /////////////////////////////////////////////////////////////////////////////
// Author: Kris Morness, May 2002
// Desc:	 An upgrade that set's the owner's OBJECT_STATUS_CAN_STEALTH Status
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#define DEFINE_STEALTHLEVEL_NAMES

#include "Common/Player.h"
#include "Common/Xfer.h"
#include "GameLogic/Module/StealthUpdate.h"
#include "GameLogic/Module/StealthUpgrade.h"
#include "GameLogic/Module/SpawnBehavior.h"
#include "GameLogic/Object.h"

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void StealthUpgradeModuleData::buildFieldParse(MultiIniFieldParse& p)
{
	UpgradeModuleData::buildFieldParse(p);

	static const FieldParse dataFieldParse[] =
	{
		{ "EnableStealth", INI::parseBool, nullptr, offsetof(StealthUpgradeModuleData, m_enableStealth) },
		{ "OverrideStealthForbiddenConditions", INI::parseBitString32, TheStealthLevelNames, offsetof(StealthUpgradeModuleData, m_stealthLevel) },
		{ 0, 0, 0, 0 }
	};
	p.add(dataFieldParse);
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
StealthUpgrade::StealthUpgrade( Thing *thing, const ModuleData* moduleData ) : UpgradeModule( thing, moduleData )
{
	m_hasExecuted = FALSE;
	m_prevStealthLevel = 0;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
StealthUpgrade::~StealthUpgrade( void )
{
}

//-------------------------------------------------------------------------------------------------
void StealthUpgrade::upgradeImplementation()
{
	// The logic that does the stealthupdate will notice this and start stealthing
	Object *me = getObject();

	UpgradeMaskType objectMask = me->getObjectCompletedUpgradeMask();
	UpgradeMaskType playerMask = me->getControllingPlayer()->getCompletedUpgradeMask();
	UpgradeMaskType maskToCheck = playerMask;
	maskToCheck.set( objectMask );

	//First make sure we have the right combination of upgrades
	Int UpgradeStatus = wouldRefreshUpgrade(maskToCheck, m_hasExecuted);

	// If there's no Upgrade Status, do Nothing;
	if( UpgradeStatus == 0 )
	{
		return;
	}

	const StealthUpgradeModuleData* d = getStealthUpgradeModuleData();

	// Get whether the Upgrade wants to Enable or Disable the Stealth
	Bool isEnable = d->m_enableStealth;

	m_hasExecuted = UpgradeStatus == 1 ? TRUE : FALSE;

	// Remove the Upgrade Execution Status so it is treated as activation again
	if(!m_hasExecuted)
	{
		setUpgradeExecuted(false);

		// Set its Given Status as Reversed, so its treated as the opposite
		isEnable = !isEnable;
	}
	
	if (d->m_stealthLevel > 0) {
		StealthUpdate* stealth = getObject()->getStealth();
		if (stealth) {  // we should always have a stealth update module
			if(m_hasExecuted) {
				m_prevStealthLevel = stealth->getStealthLevel();
				stealth->setStealthLevelOverride(d->m_stealthLevel);
			} else {
				stealth->setStealthLevelOverride(m_prevStealthLevel);
			}
		}
		return;  // Note AW: There should be no reason to enable/disable stealth if you change the stealthLevel
	}

	me->setStatus(MAKE_OBJECT_STATUS_MASK(OBJECT_STATUS_CAN_STEALTH), isEnable);

	//Grant stealth to spawns if applicable.
	if (me->isKindOf(KINDOF_SPAWNS_ARE_THE_WEAPONS))
	{
		SpawnBehaviorInterface* sbInterface = me->getSpawnBehaviorInterface();
		if (sbInterface)
		{
			sbInterface->giveSlavesStealthUpgrade(isEnable);
		}
	}
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void StealthUpgrade::crc( Xfer *xfer )
{

	// extend base class
	UpgradeModule::crc( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void StealthUpgrade::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	UpgradeModule::xfer( xfer );

	xfer->xferBool(&m_hasExecuted);

	xfer->xferUnsignedInt(&m_prevStealthLevel);

}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void StealthUpgrade::loadPostProcess( void )
{

	// extend base class
	UpgradeModule::loadPostProcess();

}

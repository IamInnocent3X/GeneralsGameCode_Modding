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

// FILE: LineOfSightModifierUpgrade.cpp ///////////////////////////////////////////////////////////
// Author: IamInnocent, December 2025
// Desc:	 An upgrade that set's the owner's requirement for checking Line Of Sight
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "Common/Player.h"
#include "Common/Xfer.h"
#include "GameLogic/Module/LineOfSightModifierUpgrade.h"
#include "GameLogic/Object.h"

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void LineOfSightModifierUpgradeModuleData::buildFieldParse(MultiIniFieldParse& p)
{
	UpgradeModuleData::buildFieldParse(p);

	static const FieldParse dataFieldParse[] =
	{
		{ "DisableNeedToCheckLineOfSight", INI::parseBool, nullptr, offsetof(LineOfSightModifierUpgradeModuleData, m_disableLOS) },
		{ "SetIgnoreObstacleViewBlock", INI::parseBool, nullptr, offsetof(LineOfSightModifierUpgradeModuleData, m_ignoreObstacleViewBlock) },
		{ 0, 0, 0, 0 }
	};
	p.add(dataFieldParse);
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
LineOfSightModifierUpgrade::LineOfSightModifierUpgrade( Thing *thing, const ModuleData* moduleData ) : UpgradeModule( thing, moduleData )
{
	m_hasExecuted = FALSE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
LineOfSightModifierUpgrade::~LineOfSightModifierUpgrade()
{
}

//-------------------------------------------------------------------------------------------------
void LineOfSightModifierUpgrade::upgradeImplementation()
{
	// The logic that does the stealthupdate will notice this and start stealthing
	Object *me = getObject();

	const UpgradeMaskType& objectMask = me->getObjectCompletedUpgradeMask();
	const UpgradeMaskType& playerMask = me->getControllingPlayer()->getCompletedUpgradeMask();
	UpgradeMaskType maskToCheck = playerMask;
	maskToCheck.set( objectMask );

	//First make sure we have the right combination of upgrades
	Int UpgradeStatus = wouldRefreshUpgrade(maskToCheck, m_hasExecuted);

	// If there's no Upgrade Status, do Nothing;
	if( UpgradeStatus == 0 )
	{
		return;
	}

	const LineOfSightModifierUpgradeModuleData* d = getLineOfSightModifierUpgradeModuleData();

	// Get whether the Upgrade wants to Enable or Disable the Stealth
	Bool isDisableLOS = d->m_disableLOS;
	Bool hasIgnoreObstacleViewBlock = d->m_ignoreObstacleViewBlock;

	m_hasExecuted = UpgradeStatus == 1 ? TRUE : FALSE;

	// Remove the Upgrade Execution Status so it is treated as activation again
	if(!m_hasExecuted)
	{
		setUpgradeExecuted(false);

		// Set its Given Status as reversed
		if(isDisableLOS)
			isDisableLOS = false;
		if(hasIgnoreObstacleViewBlock)
			hasIgnoreObstacleViewBlock = false;
	}

	me->setLineOfSightRequirementStatus(!isDisableLOS);
	me->setIgnoresObstacleForViewBlock(hasIgnoreObstacleViewBlock);
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void LineOfSightModifierUpgrade::crc( Xfer *xfer )
{

	// extend base class
	UpgradeModule::crc( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void LineOfSightModifierUpgrade::xfer( Xfer *xfer )
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
void LineOfSightModifierUpgrade::loadPostProcess()
{

	// extend base class
	UpgradeModule::loadPostProcess();

}

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

// FILE: ArmorUpgrade.cpp /////////////////////////////////////////////////
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
//	Filename: 	ArmorUpgrade.cpp
//
//	author:		Chris Brue
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

#include "Common/Xfer.h"
#include "Common/Player.h"
#include "Common/Upgrade.h"
#include "GameLogic/Object.h"
#include "GameLogic/Module/ArmorUpgrade.h"
#include "GameLogic/Module/BodyModule.h"

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
ArmorUpgradeModuleData::ArmorUpgradeModuleData(void)
{
	m_armorSetFlag = ARMORSET_PLAYER_UPGRADE;
	//m_needsParkedAircraft = FALSE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void ArmorUpgradeModuleData::buildFieldParse(MultiIniFieldParse& p)
{

	UpgradeModuleData::buildFieldParse(p);

	static const FieldParse dataFieldParse[] =
	{
		{ "ArmorSetFlag", INI::parseIndexList,	ArmorSetFlags::getBitNames(),offsetof(ArmorUpgradeModuleData, m_armorSetFlag) },
		{ "ArmorSetFlagsToClear", ArmorSetFlags::parseFromINI, nullptr, offsetof(ArmorUpgradeModuleData, m_armorSetFlagsToClear) },
		//{ "NeedsParkedAircraft", INI::parseBool, nullptr, offsetof(ArmorUpgradeModuleData, m_needsParkedAircraft) },
		{ 0, 0, 0, 0 }
	};

	p.add(dataFieldParse);

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
ArmorUpgrade::ArmorUpgrade( Thing *thing, const ModuleData* moduleData ) : UpgradeModule( thing, moduleData )
{
	m_lastTerrainDecalType = TERRAIN_DECAL_NONE;
	m_hasExecuted = FALSE;
	m_clearedArmorSetFlags.clear();
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
ArmorUpgrade::~ArmorUpgrade( void )
{
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
Bool ArmorUpgrade::attemptUpgrade(UpgradeMaskType keyMask)
{
	if (isTriggeredBy("Upgrade_AmericaChemicalSuits"))
	{
		Drawable* draw = getObject()->getDrawable();
		if (!draw) {
			return false;
		}
	}

	return UpgradeMux::attemptUpgrade(keyMask);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void ArmorUpgrade::upgradeImplementation( )
{
	// Very simple; just need to flag the Object as having the player upgrade, and the WeaponSet chooser
	// will do the work of picking the right one from ini.  This comment is as long as the code.
	
	DEBUG_LOG(("ArmorUpgrade::upgradeImplementation 0\n"));

	const ArmorUpgradeModuleData* data = getArmorUpgradeModuleData();

	Object *obj = getObject();
	if( !obj )
		return;

	DEBUG_LOG(("ArmorUpgrade::upgradeImplementation 1\n"));

	UpgradeMaskType objectMask = obj->getObjectCompletedUpgradeMask();
	UpgradeMaskType playerMask = obj->getControllingPlayer()->getCompletedUpgradeMask();
	UpgradeMaskType maskToCheck = playerMask;
	maskToCheck.set( objectMask );

	//First make sure we have the right combination of upgrades
	Int UpgradeStatus = wouldRefreshUpgrade(maskToCheck, m_hasExecuted);

	// If there's no Upgrade Status, do Nothing;
	if( UpgradeStatus == 0 )
	{
		return;
	}
	
	m_hasExecuted = TRUE;
	
	if( UpgradeStatus == 2 )
	{
		m_hasExecuted = FALSE;
		// Remove the Upgrade Execution Status so it is treated as activation again
		setUpgradeExecuted(false);
	}

	m_hasExecuted = UpgradeStatus == 1 ? TRUE : FALSE;

	BodyModuleInterface* body = obj->getBodyModule();
	if (body) {
		if( m_hasExecuted )
		{
			body->setArmorSetFlag(data->m_armorSetFlag);

			DEBUG_LOG(("ArmorUpgrade::upgradeImplementation 2 - flagsToClear = %d\n", data->m_armorSetFlagsToClear));

			if (data->m_armorSetFlagsToClear.any()) {
				// We loop over each armorset type and see if we have it set.
				// Andi: Not sure if this is cleaner solution than storing an array of flags.
				for (int i = 0; i < ARMORSET_COUNT; i++) {
					ArmorSetType type = (ArmorSetType)i;
					if (data->m_armorSetFlagsToClear.test(type)) {
						body->clearArmorSetFlag(type);
						m_clearedArmorSetFlags.set(type);
						// obj->clearWeaponSetFlag(type);
					}
				}
			}
		}
		else
		{
			if(body->testArmorSetFlag(data->m_armorSetFlag))
				body->clearArmorSetFlag(data->m_armorSetFlag);

			DEBUG_LOG(("ArmorUpgrade::upgradeImplementation, upgrade removal 2 - flagsToSet Back = %d\n", data->m_armorSetFlagsToClear));

			if (m_clearedArmorSetFlags.any()) {
				for (int i = 0; i < ARMORSET_COUNT; i++) {
					ArmorSetType type = (ArmorSetType)i;
					if (m_clearedArmorSetFlags.test(type)) {
						body->clearArmorSetFlag(type);
					}
				}
				m_clearedArmorSetFlags.clear();
			}
		}
	}

	// Unique case for AMERICA to test for upgrade to set flag
	if(isTriggeredBy("Upgrade_AmericaChemicalSuits"))
	{
		Drawable* draw = obj->getDrawable();
		if (draw) {
			if(m_hasExecuted)
			{
				m_lastTerrainDecalType = draw->getTerrainDecalType();
				draw->setTerrainDecal(TERRAIN_DECAL_CHEMSUIT);
			}
			else if( draw->getTerrainDecalType() == TERRAIN_DECAL_CHEMSUIT && !m_hasExecuted )
			{
				draw->setTerrainDecal(m_lastTerrainDecalType);
			}
		}
		/*else {
			DEBUG_LOG(("ArmorUpgrade::upgradeImplementation 3b - no draw?.\n"));
		}*/
	}

}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void ArmorUpgrade::crc( Xfer *xfer )
{

	// extend base class
	UpgradeModule::crc( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void ArmorUpgrade::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	UpgradeModule::xfer( xfer );

	m_clearedArmorSetFlags.xfer( xfer );
	//m_lastTerrainDecalType.xfer( xfer );
	xfer->xferUser( &m_lastTerrainDecalType, sizeof( m_lastTerrainDecalType ) );

	xfer->xferBool(&m_hasExecuted);

}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void ArmorUpgrade::loadPostProcess( void )
{

	// extend base class
	UpgradeModule::loadPostProcess();

}

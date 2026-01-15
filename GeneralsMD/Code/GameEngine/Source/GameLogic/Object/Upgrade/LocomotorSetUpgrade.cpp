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

// FILE: LocomotorSetUpgrade.cpp /////////////////////////////////////////////////////////////////////////////
// Author: Graham Smallwood, March 2002
// Desc:	 UpgradeModule that sets a weapon set bit for the Best Fit weapon set chooser to discover
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#define DEFINE_LOCOMOTORSET_NAMES //Gain access to TheLocomotorSetNames[]

#include "Common/Xfer.h"
#include "Common/Player.h"
#include "GameLogic/Object.h"
#include "GameLogic/Module/LocomotorSetUpgrade.h"
#include "GameLogic/Module/AIUpdate.h"

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
LocomotorSetUpgradeModuleData::LocomotorSetUpgradeModuleData(void)
{
	m_setUpgraded = TRUE;
	m_useLocomotorType = FALSE;
	m_LocomotorType = LOCOMOTORSET_INVALID;
	// m_needsParkedAircraft = FALSE;
}
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
/*static*/ void LocomotorSetUpgradeModuleData::parseLocomotorType(INI* ini, void* instance, void* store, const void* /*userData*/)
{
	const char* token = ini->getNextToken();
	if (stricmp(token, "None") != 0) {
		LocomotorSetUpgradeModuleData* self = (LocomotorSetUpgradeModuleData*)instance;
		self->m_useLocomotorType = true;
		*(LocomotorSetType*)store = (LocomotorSetType)INI::scanIndexList(token, TheLocomotorSetNames);
	}
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void LocomotorSetUpgradeModuleData::buildFieldParse(MultiIniFieldParse& p)
{

	UpgradeModuleData::buildFieldParse(p);

	static const FieldParse dataFieldParse[] =
	{
		{ "EnableUpgrade", INI::parseBool, nullptr, offsetof(LocomotorSetUpgradeModuleData, m_setUpgraded) },
		{ "ExplicitLocomotorType", LocomotorSetUpgradeModuleData::parseLocomotorType, nullptr, offsetof(LocomotorSetUpgradeModuleData, m_LocomotorType)},
		//{ "NeedsParkedAircraft", INI::parseBool, nullptr, offsetof(WeaponSetUpgradeModuleData, m_needsParkedAircraft) },
		{ 0, 0, 0, 0 }
	};

	p.add(dataFieldParse);

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
LocomotorSetUpgrade::LocomotorSetUpgrade( Thing *thing, const ModuleData* moduleData ) : UpgradeModule( thing, moduleData )
{
	m_hasExecuted = FALSE;
	m_prevLocomotorType = LOCOMOTORSET_INVALID;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
LocomotorSetUpgrade::~LocomotorSetUpgrade( void )
{
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void LocomotorSetUpgrade::upgradeImplementation( )
{
	Object* obj = getObject();
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
	else if( UpgradeStatus == 1 )
	{
		// Set to apply upgrade
		m_hasExecuted = TRUE;
	}
	else if( UpgradeStatus == 2 )
	{
		m_hasExecuted = FALSE;
		// Remove the Upgrade Execution Status so it is treated as activation again
		setUpgradeExecuted(false);
	}
	
	const LocomotorSetUpgradeModuleData* data = getLocomotorSetUpgradeModuleData();
	AIUpdateInterface* ai = obj->getAIUpdateInterface();
	if (ai) {
		if( m_hasExecuted )
		{
			m_prevLocomotorType = ai->getCurLocomotorSetType();
			if (data->m_useLocomotorType && data->m_LocomotorType != LOCOMOTORSET_NORMAL_UPGRADED) {
				ai->chooseLocomotorSet(data->m_LocomotorType);
			}
			else {
				ai->setLocomotorUpgrade(data->m_setUpgraded);
			}
		}
		else
		{
			// We only revert back if the locomotor set doesn't change after the upgrade
			if (data->m_useLocomotorType && data->m_LocomotorType != LOCOMOTORSET_NORMAL_UPGRADED) {
				if(ai->getCurLocomotorSetType() == data->m_LocomotorType)
					ai->chooseLocomotorSet(m_prevLocomotorType);
			}
			else{
				LocomotorSetType lastLocoSet = ai->getCurLocomotorSetType();
				ai->setLocomotorUpgrade(!data->m_setUpgraded);

				if(lastLocoSet != LOCOMOTORSET_NORMAL_UPGRADED)
					ai->chooseLocomotorSet(lastLocoSet);
			}
		}

	}

}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void LocomotorSetUpgrade::crc( Xfer *xfer )
{

	// extend base class
	UpgradeModule::crc( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void LocomotorSetUpgrade::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	UpgradeModule::xfer( xfer );

	// Has executed
	xfer->xferBool( &m_hasExecuted );

	// previous locomotor type
	xfer->xferUser( &m_prevLocomotorType, sizeof(LocomotorSetType) );

}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void LocomotorSetUpgrade::loadPostProcess( void )
{

	// extend base class
	UpgradeModule::loadPostProcess();

}

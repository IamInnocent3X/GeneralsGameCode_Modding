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

// FILE: ExperienceScalarUpgrade.cpp /////////////////////////////////////////////////////////////////////////////
// Author: Kris Morness, September 2002
// Desc:	 UpgradeModule that adds a scalar to the object's experience gain.
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "Common/Player.h"
#include "Common/Xfer.h"
#include "GameLogic/Object.h"
#include "GameLogic/ExperienceTracker.h"
#include "GameLogic/Module/ExperienceScalarUpgrade.h"

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
ExperienceScalarUpgradeModuleData::ExperienceScalarUpgradeModuleData( void )
{
	m_initiallyActive = false;
	m_addXPScalar = 0.0f;
	m_addXPValueScalar = 0.0f;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void ExperienceScalarUpgradeModuleData::buildFieldParse(MultiIniFieldParse& p)
{

  UpgradeModuleData::buildFieldParse( p );

	static const FieldParse dataFieldParse[] =
	{
		{ "StartsActive",	INI::parseBool, NULL, offsetof(ExperienceScalarUpgradeModuleData, m_initiallyActive) },
		{ "AddXPScalar",	INI::parseReal,		NULL, offsetof( ExperienceScalarUpgradeModuleData, m_addXPScalar ) },
		{ "AddXPValueScalar",	INI::parseReal,		NULL, offsetof( ExperienceScalarUpgradeModuleData, m_addXPValueScalar ) },
		{ 0, 0, 0, 0 }
	};

  p.add(dataFieldParse);

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
ExperienceScalarUpgrade::ExperienceScalarUpgrade( Thing *thing, const ModuleData* moduleData ) : UpgradeModule( thing, moduleData )
{
	m_hasExecuted = FALSE;
	if (getExperienceScalarUpgradeModuleData()->m_initiallyActive)
	{
		giveSelfUpgrade();
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
ExperienceScalarUpgrade::~ExperienceScalarUpgrade( void )
{
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void ExperienceScalarUpgrade::upgradeImplementation( )
{
	const ExperienceScalarUpgradeModuleData *data = getExperienceScalarUpgradeModuleData();

	//Simply add the xp scalar to the xp tracker!
	Object *obj = getObject();

	UpgradeMaskType objectMask = obj->getObjectCompletedUpgradeMask();
	UpgradeMaskType playerMask = obj->getControllingPlayer()->getCompletedUpgradeMask();
	UpgradeMaskType maskToCheck = playerMask;
	maskToCheck.set( objectMask );

	//First make sure we have the right combination of upgrades
	Int UpgradeStatus = wouldRefreshUpgrade(maskToCheck, m_hasExecuted);

	Real value, scalar;

	if( UpgradeStatus == 1 )
	{
		m_hasExecuted = TRUE;
		value = data->m_addXPScalar;
		scalar = data->m_addXPValueScalar;
	}
	else if( UpgradeStatus == 2 )
	{
		m_hasExecuted = FALSE;
		value = -data->m_addXPScalar;
		scalar = -data->m_addXPValueScalar;
		// Remove the Upgrade Execution Status so it is treated as activation again
		setUpgradeExecuted(false);
	}
	else
	{
		return;
	}

	ExperienceTracker *xpTracker = obj->getExperienceTracker();
	if( xpTracker )
	{
		xpTracker->setExperienceScalar( xpTracker->getExperienceScalar() + value );
		xpTracker->setExperienceValueScalar( xpTracker->getExperienceValueScalar() + scalar );
	}
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void ExperienceScalarUpgrade::crc( Xfer *xfer )
{

	// extend base class
	UpgradeModule::crc( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void ExperienceScalarUpgrade::xfer( Xfer *xfer )
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
void ExperienceScalarUpgrade::loadPostProcess( void )
{

	// extend base class
	UpgradeModule::loadPostProcess();

}

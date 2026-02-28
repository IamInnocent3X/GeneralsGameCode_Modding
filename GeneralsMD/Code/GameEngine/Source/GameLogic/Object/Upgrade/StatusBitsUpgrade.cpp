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

// FILE: StatusBitsUpgrade.cpp /////////////////////////////////////////////////
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
//	Filename: 	StatusBitsUpgrade.cpp
//
//	author:		Steven Johnson
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

#define DEFINE_OBJECT_STATUS_NAMES
#include "Common/Xfer.h"
#include "GameClient/Drawable.h"
#include "GameLogic/Object.h"
#include "GameLogic/Weapon.h"
#include "GameLogic/Module/StatusBitsUpgrade.h"

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
void StatusBitsUpgradeModuleData::buildFieldParse(MultiIniFieldParse& p)
{
  UpgradeModuleData::buildFieldParse(p);

	static const FieldParse dataFieldParse[] =
	{
		{ "StatusToSet",		ObjectStatusMaskType::parseFromINI,	nullptr, offsetof( StatusBitsUpgradeModuleData, m_statusToSet ) },
		{ "StatusToClear",	ObjectStatusMaskType::parseFromINI,	nullptr, offsetof( StatusBitsUpgradeModuleData, m_statusToClear ) },
		{ "CustomStatusToSet",	INI::parseAsciiStringVector, nullptr, offsetof( StatusBitsUpgradeModuleData, m_customStatusToSet ) },
		{ "CustomStatusToClear",	INI::parseAsciiStringVector, nullptr, offsetof( StatusBitsUpgradeModuleData, m_customStatusToClear ) },
		{ "BonusToSet",       INI::parseWeaponBonusVector, nullptr, offsetof( StatusBitsUpgradeModuleData, m_bonusToSet ) },
		{ "BonusToClear",       INI::parseWeaponBonusVector, nullptr, offsetof( StatusBitsUpgradeModuleData, m_bonusToClear ) },
		{ "CustomBonusToSet",	INI::parseAsciiStringVector, nullptr, offsetof( StatusBitsUpgradeModuleData, m_customBonusToSet ) },
		{ "CustomBonusToClear",	INI::parseAsciiStringVector, nullptr, offsetof( StatusBitsUpgradeModuleData, m_customBonusToClear ) },
		{ nullptr, nullptr, nullptr, 0 }
	};
  p.add(dataFieldParse);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
StatusBitsUpgrade::StatusBitsUpgrade( Thing *thing, const ModuleData* moduleData ) : UpgradeModule( thing, moduleData )
{
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
StatusBitsUpgrade::~StatusBitsUpgrade()
{
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void StatusBitsUpgrade::upgradeImplementation()
{
	const StatusBitsUpgradeModuleData* d = getStatusBitsUpgradeModuleData();
	Object *obj = getObject();	

	obj->setStatus( d->m_statusToSet );
	obj->clearStatus( d->m_statusToClear );
	obj->setCustomStatus( d->m_customStatusToSet );
	obj->clearCustomStatus( d->m_customStatusToSet );
	for (Int i = 0; i < d->m_bonusToSet.size(); i++) {
		obj->setWeaponBonusCondition(d->m_bonusToSet[i]);
	}
	for (Int i = 0; i < d->m_bonusToClear.size(); i++) {
		obj->clearWeaponBonusCondition(d->m_bonusToClear[i]);
	}
	for(std::vector<AsciiString>::const_iterator it = d->m_customBonusToSet.begin(); it != d->m_customBonusToSet.end(); ++it)
	{
		obj->setCustomWeaponBonusCondition( *it );
	}
	for(std::vector<AsciiString>::const_iterator it = d->m_customBonusToClear.begin(); it != d->m_customBonusToClear.end(); ++it)
	{
		obj->clearCustomWeaponBonusCondition( *it );
	}
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void StatusBitsUpgrade::crc( Xfer *xfer )
{

	// extend base class
	UpgradeModule::crc( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void StatusBitsUpgrade::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	UpgradeModule::xfer( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void StatusBitsUpgrade::loadPostProcess()
{

	// extend base class
	UpgradeModule::loadPostProcess();

}

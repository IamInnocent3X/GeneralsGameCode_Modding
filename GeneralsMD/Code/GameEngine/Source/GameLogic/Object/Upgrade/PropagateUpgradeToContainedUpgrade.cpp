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

// FILE: PropagateUpgradeToContainedUpgrade.cpp /////////////////////////////////
//-----------------------------------------------------------------------------
//
//	purpose:	When this upgrade triggers on a transport/container, grant the
//						configured upgrade(s) to each currently-contained passenger.
//
//-----------------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// USER INCLUDES //////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "Common/Player.h"
#include "Common/Upgrade.h"
#include "Common/Xfer.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/Object.h"
#include "GameLogic/Module/ContainModule.h"
#include "GameLogic/Module/PropagateUpgradeToContainedUpgrade.h"

//-------------------------------------------------------------------------------------------------
// Grant one upgrade to one rider, skipping riders that already have it or that have no module
// reacting to it, so we don't dirty their upgrade mask needlessly.
//-------------------------------------------------------------------------------------------------
static void tryGrantUpgradeToRider( Object *rider, const UpgradeTemplate *upgradeTemplate, Bool isGrant )
{
	if( rider == nullptr )
		return;

	if( !rider->affectedByUpgrade( upgradeTemplate ) || (isGrant && rider->hasUpgrade( upgradeTemplate )) || (!isGrant && !rider->hasUpgrade( upgradeTemplate )) )
		return;

	if(isGrant)
		rider->giveUpgrade( upgradeTemplate );
	else
		rider->removeUpgrade( upgradeTemplate );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void PropagateUpgradeToContainedUpgradeModuleData::buildFieldParse(MultiIniFieldParse& p)
{
  UpgradeModuleData::buildFieldParse(p);

	static const FieldParse dataFieldParse[] =
	{
		{ "UpgradeToPropagate",	INI::parseAsciiStringVector, nullptr, offsetof( PropagateUpgradeToContainedUpgradeModuleData, m_upgradeNamesToPropagate ) },
		{ nullptr, nullptr, nullptr, 0 }
	};
  p.add(dataFieldParse);
}

//------------------------------------------------------------------------------------------------- 
//-------------------------------------------------------------------------------------------------
PropagateUpgradeToContainedUpgrade::PropagateUpgradeToContainedUpgrade( Thing *thing, const ModuleData* moduleData ) : UpgradeModule( thing, moduleData )
{
	m_hasExecuted = FALSE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PropagateUpgradeToContainedUpgrade::~PropagateUpgradeToContainedUpgrade()
{
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void PropagateUpgradeToContainedUpgrade::upgradeImplementation( )
{
	Object *obj = getObject();

	ContainModuleInterface *contain = obj->getContain();
	if( contain == nullptr )
	{
		DEBUG_CRASH( ("PropagateUpgradeToContainedUpgrade on %s has no Contain module; nothing to propagate to.", obj->getName().str() ) );
		return;
	}

	const PropagateUpgradeToContainedUpgradeModuleData *data = getPropagateUpgradeToContainedUpgradeModuleData();

	const UpgradeMaskType& objectMask = obj->getObjectCompletedUpgradeMask();
	const UpgradeMaskType& playerMask = obj->getControllingPlayer()->getCompletedUpgradeMask();
	UpgradeMaskType maskToCheck = playerMask;
	maskToCheck.set( objectMask );

	//First make sure we have the right combination of upgrades
	Int UpgradeStatus = wouldRefreshUpgrade(maskToCheck, m_hasExecuted);

	if( UpgradeStatus == 1 )
	{
		m_hasExecuted = TRUE;
	}
	else if( UpgradeStatus == 2 )
	{
		m_hasExecuted = FALSE;

		// Remove the Upgrade Execution Status so it is treated as activation again
		setUpgradeExecuted(false);
	}
	else
	{
		return;
	}

	const ContainedItemsList *contained = contain->getContainedItemsList();

	// Some containers keep an object in a separate hidden slot that is NOT in the contained list,
	// e.g. HelixContain's portable turret (KINDOF_PORTABLE_STRUCTURE). friend_getRider() exposes it.
	Object *hiddenRider = nullptr;
	const Object *rawHiddenRider = contain->friend_getRider();
	if( rawHiddenRider != nullptr )
		hiddenRider = TheGameLogic->findObjectByID( rawHiddenRider->getID() );

	const std::vector<AsciiString> &names = data->m_upgradeNamesToPropagate;

	for( std::vector<AsciiString>::const_iterator nameIt = names.begin(); nameIt != names.end(); ++nameIt )
	{
		const UpgradeTemplate *upgradeTemplate = TheUpgradeCenter->findUpgrade( *nameIt );
		if( upgradeTemplate == nullptr )
		{
			DEBUG_CRASH( ("PropagateUpgradeToContainedUpgrade on %s can't find upgrade template %s.", obj->getName().str(), nameIt->str() ) );
			continue;
		}

		// Only OBJECT-type upgrades make sense per-rider; player upgrades belong to the whole player.
		if( upgradeTemplate->getUpgradeType() != UPGRADE_TYPE_OBJECT )
		{
			DEBUG_CRASH( ("PropagateUpgradeToContainedUpgrade on %s: upgrade %s is not an OBJECT upgrade and won't be propagated.", obj->getName().str(), nameIt->str() ) );
			continue;
		}

		if( contained != nullptr )
		{
			// Increment the iterator before granting: giveUpgrade() may run rider modules that change containment.
			for( ContainedItemsList::const_iterator it = contained->begin(); it != contained->end(); )
			{
				Object *rider = *it;
				++it;

				tryGrantUpgradeToRider( rider, upgradeTemplate, m_hasExecuted );
			}
		}

		// hiddenRider is guarded against being a duplicate of a listed rider by tryGrantUpgradeToRider's
		// hasUpgrade() check, so it is safe to also grant here.
		tryGrantUpgradeToRider( hiddenRider, upgradeTemplate, m_hasExecuted );
	}
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void PropagateUpgradeToContainedUpgrade::crc( Xfer *xfer )
{

	// extend base class
	UpgradeModule::crc( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void PropagateUpgradeToContainedUpgrade::xfer( Xfer *xfer )
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
void PropagateUpgradeToContainedUpgrade::loadPostProcess()
{

	// extend base class
	UpgradeModule::loadPostProcess();

}

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

// FILE: UpgradeDie.cpp ///////////////////////////////////////////////////////////////////////////
// Author: Kris Morness, August 2002
// Desc:  When object dies, the parent object is freed of the specified object upgrade field.
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "Common/Player.h"
#include "Common/ThingTemplate.h"
#include "Common/Upgrade.h"
#include "Common/Xfer.h"

#include "GameClient/Drawable.h"

#include "GameLogic/Module/ProductionUpdate.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/Module/UpgradeDie.h"
#include "GameLogic/Object.h"

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
UpgradeDie::UpgradeDie( Thing *thing, const ModuleData* moduleData ) : DieModule( thing, moduleData )
{
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
UpgradeDie::~UpgradeDie( void )
{
}

//-------------------------------------------------------------------------------------------------
/** The die callback. */
//-------------------------------------------------------------------------------------------------
void UpgradeDie::onDie( const DamageInfo *damageInfo )
{
	if (!isDieApplicable(damageInfo))
		return;
	//Look for the object that created me.
	Object *producer = TheGameLogic->findObjectByID( getObject()->getProducerID() );
	if( producer )
	{
		const UpgradeDieModuleData * data = getUpgradeDieModuleData();
		Player *player = producer->getControllingPlayer();
	
		if(!data->m_upgradeName.isEmpty())
		{
			//Okay, we found our parent... now look for the upgrade.
			const UpgradeTemplate *upgrade = TheUpgradeCenter->findUpgrade( data->m_upgradeName );

			if( upgrade )
			{
				//Check if it's a player Upgrade...
				if( upgrade->getUpgradeType() == UPGRADE_TYPE_PLAYER )
				{
					player->removeUpgrade( upgrade );
				}
				//We found the upgrade, now see if the parent object has it set...
				else if( producer->hasUpgrade( upgrade ) )
				{
					//Cool, now remove it.
					producer->removeUpgrade( upgrade );
				}
				else
				{
					DEBUG_ASSERTCRASH( 0, ("Object %s just died, but is trying to free upgrade %s in it's producer %s%s",
						getObject()->getTemplate()->getName().str(),
						data->m_upgradeName.str(),
						producer->getTemplate()->getName().str(),
						", which the producer doesn't have. This is used in cases where the producer builds an upgrade that can die... like ranger building scout drones.") );
				}
			}
		}
		std::vector<AsciiString> upgradeNames = data->m_upgradeNames;

		if( !upgradeNames.empty() )
		{
			std::vector<AsciiString>::const_iterator it;
			for( it = upgradeNames.begin();
						it != upgradeNames.end();
						it++)
			{
				const UpgradeTemplate* upgrade = TheUpgradeCenter->findUpgrade( *it );
				if( !upgrade )
				{
					DEBUG_CRASH(("An upgrade module references %s, which is not an Upgrade", it->str()));
					throw INI_INVALID_DATA;
				}

		//if( upgradeNames.size() > 0 )
		//{
		//	for (int i; i < upgradeNames.size() ; i++)
		//	{
		//		const UpgradeTemplate *upgrade = TheUpgradeCenter->findUpgrade( data->m_upgradeNames[i] );
				if( upgrade )
				{
					//Check if it's a player Upgrade...
					if( upgrade->getUpgradeType() == UPGRADE_TYPE_PLAYER )
					{
						player->removeUpgrade( upgrade );
					}
					//We found the upgrade, now see if the parent object has it set...
					else if( producer->hasUpgrade( upgrade ) )
					{
						//Cool, now remove it.
						producer->removeUpgrade( upgrade );
					}
					else
					{
						DEBUG_ASSERTCRASH( 0, ("Object %s just died, but is trying to free upgrade %s in it's producer %s%s",
							getObject()->getTemplate()->getName().str(),
							it->str(),
							producer->getTemplate()->getName().str(), 
							", which the producer doesn't have. This is used in cases where the producer builds an upgrade that can die... like ranger building scout drones.") );
					}
				}
			}
		}
		std::vector<AsciiString> upgradeNamesGrant = data->m_upgradeNamesGrant;
		if( !upgradeNamesGrant.empty() )
		{
			std::vector<AsciiString>::const_iterator it;
			for( it = upgradeNamesGrant.begin();
						it != upgradeNamesGrant.end();
						it++)
			{
				const UpgradeTemplate* upgradeTemplate = TheUpgradeCenter->findUpgrade( *it );
				if( !upgradeTemplate )
				{
					DEBUG_CRASH(("An upgrade module references %s, which is not an Upgrade", it->str()));
					throw INI_INVALID_DATA;
				}

		//if( upgradeNamesGrant.size() > 0 )
		//{
		//	for (int i; i < upgradeNamesGrant.size() ; i++)
		//	{
		//		const UpgradeTemplate *upgrade = TheUpgradeCenter->findUpgrade( data->m_upgradeNames[i] );
				if( !upgradeTemplate )
				{
					DEBUG_ASSERTCRASH( 0, ("Object %s just died, but is trying to give upgrade %s to it's producer %s",
						getObject()->getTemplate()->getName().str(),
						it->str(),
						producer->getTemplate()->getName().str()) );
				}
				if( upgradeTemplate->getUpgradeType() == UPGRADE_TYPE_PLAYER )
				{
					// get the player
					player->findUpgradeInQueuesAndCancelThem( upgradeTemplate );
					player->addUpgrade( upgradeTemplate, UPGRADE_STATUS_COMPLETE );
				}
				else
				{
					// Fail safe if in any other condition, for example: Undead Body, or new Future Implementations such as UpgradeDie to Give Upgrades.
					ProductionUpdateInterface *pui = producer->getProductionUpdateInterface();
					if( pui )
					{
						pui->cancelUpgrade( upgradeTemplate );
					}
					producer->giveUpgrade( upgradeTemplate );
				}
				
				player->getAcademyStats()->recordUpgrade( upgradeTemplate, TRUE );
			}
		}
	}
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void UpgradeDie::crc( Xfer *xfer )
{

	// extend base class
	DieModule::crc( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void UpgradeDie::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	DieModule::xfer( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void UpgradeDie::loadPostProcess( void )
{

	// extend base class
	DieModule::loadPostProcess();

}

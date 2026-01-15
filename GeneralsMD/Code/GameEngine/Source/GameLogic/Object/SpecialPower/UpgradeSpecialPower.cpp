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

// FILE: UpgradeSpecialPower.cpp /////////////////////////////////////////////////////////////////
// Author: Andreas W, July 25
// Desc:   Special Power will grant an upgrade to the object
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file int the GameEngine

#include "Common/Xfer.h"
#include "Common/Player.h"
#include "Common/Upgrade.h"
#include "GameLogic/Object.h"
#include "GameLogic/Module/UpgradeSpecialPower.h"
#include "GameLogic/Module/ProductionUpdate.h"

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
UpgradeSpecialPowerModuleData::UpgradeSpecialPowerModuleData(void)
{
	m_upgradeName = "";
	m_upgradeNames.clear();
	m_upgradeNamesRemove.clear();
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void UpgradeSpecialPowerModuleData::buildFieldParse(MultiIniFieldParse& p)
{
	SpecialPowerModuleData::buildFieldParse(p);

	static const FieldParse dataFieldParse[] =
	{
		{ "UpgradeToGrant", INI::parseAsciiString,	nullptr,   offsetof(UpgradeSpecialPowerModuleData, m_upgradeName) },
		{ "UpgradesToGrant", INI::parseAsciiStringVector,	nullptr,   offsetof(UpgradeSpecialPowerModuleData, m_upgradeNames) },
		{ "UpgradesToRemove", INI::parseAsciiStringVector,	nullptr,   offsetof(UpgradeSpecialPowerModuleData, m_upgradeNamesRemove) },
		{ 0, 0, 0, 0 }
	};
	p.add(dataFieldParse);

}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
UpgradeSpecialPower::UpgradeSpecialPower(Thing* thing, const ModuleData* moduleData)
	: SpecialPowerModule(thing, moduleData)
{

}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
UpgradeSpecialPower::~UpgradeSpecialPower(void)
{

}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void UpgradeSpecialPower::grantUpgrade(Object* object) {

	// get module data
	const UpgradeSpecialPowerModuleData* modData = getUpgradeSpecialPowerModuleData();
	Player* player = getObject()->getControllingPlayer();

  if(!modData->m_upgradeName.isEmpty())
  {
	const UpgradeTemplate* upgradeTemplate = TheUpgradeCenter->findUpgrade(modData->m_upgradeName);
	if (!upgradeTemplate)
	{
		DEBUG_ASSERTCRASH(0, ("UpgradeSpecialPower for %s can't find upgrade template %s.", getObject()->getName(), modData->m_upgradeName));
		return;
	}

	if (upgradeTemplate->getUpgradeType() == UPGRADE_TYPE_PLAYER)
	{
		// get the player
		player->addUpgrade(upgradeTemplate, UPGRADE_STATUS_COMPLETE);
	}
	else
	{
		object->giveUpgrade(upgradeTemplate);
	}

	player->getAcademyStats()->recordUpgrade(upgradeTemplate, TRUE);

  }

	std::vector<AsciiString> upgradeNames = modData->m_upgradeNames;

	if( !upgradeNames.empty() )
	{
		std::vector<AsciiString>::const_iterator it;
		for( it = upgradeNames.begin();
					it != upgradeNames.end();
					it++)
		{
			const UpgradeTemplate* upgradeTemplate = TheUpgradeCenter->findUpgrade( *it );
			if( !upgradeTemplate )
			{
				DEBUG_CRASH(("An upgrade module references %s, which is not an Upgrade", it->str()));
				throw INI_INVALID_DATA;
			}

	//if( upgradeNames.size() > 0 )
	//{
	//	for (int i; i < upgradeNames.size() ; i++)
	//	{
	//		const UpgradeTemplate *upgrade = TheUpgradeCenter->findUpgrade( getUpgradeDieModuleData()->upgradeNames[i] );
			if( !upgradeTemplate )
			{
				DEBUG_ASSERTCRASH( 0, ("GrantUpdateCreate for %s can't find upgrade template %s.", getObject()->getName(), it->str() ) );
				return;
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
				ProductionUpdateInterface *pui = getObject()->getProductionUpdateInterface();
				if( pui )
				{
					pui->cancelUpgrade( upgradeTemplate );
				}
				getObject()->giveUpgrade( upgradeTemplate );
			}
			
			player->getAcademyStats()->recordUpgrade( upgradeTemplate, TRUE );
		}
	}

	std::vector<AsciiString> upgradeNamesRemove = modData->m_upgradeNamesRemove;

	if( !upgradeNamesRemove.empty() )
	{
		std::vector<AsciiString>::const_iterator it;
		for( it = upgradeNamesRemove.begin();
					it != upgradeNamesRemove.end();
					it++)
		{
			const UpgradeTemplate* upgrade = TheUpgradeCenter->findUpgrade( *it );
			if( !upgrade )
			{
				DEBUG_CRASH(("An upgrade module references %s, which is not an Upgrade", it->str()));
				throw INI_INVALID_DATA;
			}

	//if( m_upgradeNames.size() > 0 )
	//{
	//	for (int i; i < m_upgradeNames.size() ; i++)
	//	{
	//		const UpgradeTemplate *upgrade = TheUpgradeCenter->findUpgrade( getUpgradeDieModuleData()->m_upgradeNames[i] );
			if( upgrade )
			{
				//Check if it's a player Upgrade...
				if( upgrade->getUpgradeType() == UPGRADE_TYPE_PLAYER )
				{
					player->removeUpgrade( upgrade );
				}
				//We found the upgrade, now see if the parent object has it set...
				else if( getObject()->hasUpgrade( upgrade ) )
				{
					//Cool, now remove it.
					getObject()->removeUpgrade( upgrade );
				}
				else
				{
					DEBUG_ASSERTCRASH( 0, ("Object %s just created, but is trying to remove upgrade %s",
						getObject()->getTemplate()->getName().str(),
						it->str() ) );
				}
			}
		}
	}

}


// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void UpgradeSpecialPower::doSpecialPower(UnsignedInt commandOptions)
{
	if (getObject()->isDisabled())
		return;

	// call the base class action cause we are *EXTENDING* functionality
	SpecialPowerModule::doSpecialPower(commandOptions);

	// Grant the upgrade
	grantUpgrade(getObject());
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void UpgradeSpecialPower::doSpecialPowerAtObject(Object* obj, UnsignedInt commandOptions)
{
	if (getObject()->isDisabled())
		return;

	// call the base class action cause we are *EXTENDING* functionality
	SpecialPowerModule::doSpecialPowerAtObject(obj, commandOptions);

	// Grant the upgrade
	grantUpgrade(obj);
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void UpgradeSpecialPower::crc(Xfer* xfer)
{

	// extend base class
	SpecialPowerModule::crc(xfer);

}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
	// ------------------------------------------------------------------------------------------------
void UpgradeSpecialPower::xfer(Xfer* xfer)
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion(&version, currentVersion);

	// extend base class
	SpecialPowerModule::xfer(xfer);

}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void UpgradeSpecialPower::loadPostProcess(void)
{

	// extend base class
	SpecialPowerModule::loadPostProcess();

}

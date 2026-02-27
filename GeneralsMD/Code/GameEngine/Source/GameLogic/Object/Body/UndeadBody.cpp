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

// FILE: UndeadBody.cpp ////////////////////////////////////////////////////////////////////////
// Author: Graham Smallwood, June 2003
// Desc:	 First death is intercepted and sets flags and setMaxHealth.  Second death is handled normally.
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine
#include "Common/Player.h"
#include "Common/Xfer.h"
#include "GameLogic/Module/ProductionUpdate.h"
#include "GameLogic/Module/UndeadBody.h"

#include "GameLogic/Object.h"
#include "GameLogic/Module/SlowDeathBehavior.h"

// PUBLIC FUNCTIONS ///////////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------------------------------
void UndeadBodyModuleData::buildFieldParse(MultiIniFieldParse& p)
{
  ActiveBodyModuleData::buildFieldParse(p);
	static const FieldParse dataFieldParse[] =
	{
		{ "SecondLifeMaxHealth",			INI::parseReal,	nullptr,		offsetof( UndeadBodyModuleData, m_secondLifeMaxHealth ) },
		{ "SecondLifeUpgradesToGrant",		INI::parseAsciiStringVector,							nullptr, offsetof( UndeadBodyModuleData, m_secondLifeUpgradeNames ) },
		{ "SecondLifeUpgradesToRemove",		INI::parseAsciiStringVector,							nullptr, offsetof( UndeadBodyModuleData, m_secondLifeUpgradeRemoveNames ) },
		{ "SecondLifeSubdualCap",			INI::parseReal,	nullptr,		offsetof( UndeadBodyModuleData, m_secondLifeSubdualCap ) },
		{ "SecondLifeSubdualHealRate",			INI::parseUnsignedInt,	nullptr,		offsetof( UndeadBodyModuleData, m_secondLifeSubdualHealRate ) },
		{ "SecondLifeSubdualHealAmount",			INI::parseReal,	nullptr,		offsetof( UndeadBodyModuleData, m_secondLifeSubdualHealAmount ) },
		{ "SecondLifeArmorSetSwitchAfterLivesAmount",	INI::parseInt,	nullptr,		offsetof( UndeadBodyModuleData, m_secondLifeArmorSetSwitch ) },
		{ "MultipleLives",					INI::parseInt,	nullptr,		offsetof( UndeadBodyModuleData, m_multipleLives ) },
		{ "MultipleLivesMaxHealthRatio",	INI::parsePercentToReal,	nullptr,		offsetof( UndeadBodyModuleData, m_multipleLivesMaxHealthRatio ) },
		{ "MultipleLivesMaxHealthRatioScale",	INI::parseBool,	nullptr,		offsetof( UndeadBodyModuleData, m_multipleLivesMaxHealthRatioScale ) },
		{ "MultipleLivesOverrideTrigger",			INI::parseIntVector,				nullptr,		offsetof( UndeadBodyModuleData, m_multipleLivesOverrideTrigger ) },
		{ "MultipleLivesMaxHealthVariationOverride",		INI::parseRealVector,				nullptr,		offsetof( UndeadBodyModuleData, m_multipleLivesMaxHealthOverride ) },
		{ "MultipleLivesSubdualCapVariationOverride",		INI::parseRealVector,				nullptr,		offsetof( UndeadBodyModuleData, m_multipleLivesSubdualCapOverride ) },
		{ "MultipleLivesSubdualHealRateVariationOverride",		INI::parseUnsignedIntVector,		nullptr,		offsetof( UndeadBodyModuleData, m_multipleLivesSubdualHealRate ) },
		{ "MultipleLivesSubdualHealAmountVariationOverride",		INI::parseRealVector,				nullptr,		offsetof( UndeadBodyModuleData, m_multipleLivesSubdualHealAmount ) },
		{ "MultipleLivesUpgradesToGrant", 		INI::parseAsciiStringWithColonVectorAppend, nullptr, offsetof( UndeadBodyModuleData, m_multipleLivesUpgradeList) }, 
		{ "MultipleLivesUpgradesToRemove", 		INI::parseAsciiStringWithColonVectorAppend, nullptr, offsetof( UndeadBodyModuleData, m_multipleLivesUpgradeRemoveList) }, 
		{ nullptr, nullptr, nullptr, 0 }
	};
  p.add(dataFieldParse);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
UndeadBodyModuleData::UndeadBodyModuleData()
{
	m_secondLifeMaxHealth = 1;
	m_secondLifeSubdualCap = 0;
	m_secondLifeSubdualHealRate = 0;
	m_secondLifeSubdualHealAmount = 0;
	m_secondLifeUpgradeNames.clear();
	m_secondLifeUpgradeRemoveNames.clear();
	m_secondLifeArmorSetSwitch = 0;
	m_multipleLives = 0;
	m_multipleLivesMaxHealthRatio = 1.0f;
	m_multipleLivesMaxHealthRatioScale = FALSE;
	m_multipleLivesOverrideTrigger.clear();
	m_multipleLivesMaxHealthOverride.clear();
	m_multipleLivesSubdualCapOverride.clear();
	m_multipleLivesSubdualHealRate.clear();
	m_multipleLivesSubdualHealAmount.clear();
	m_multipleLivesUpgradeList.clear();
	m_multipleLivesUpgradeRemoveList.clear();
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
UndeadBody::UndeadBody( Thing *thing, const ModuleData* moduleData )
						 : ActiveBody( thing, moduleData )
{
	m_isSecondLife = FALSE;
	m_currentMultipleLives = getUndeadBodyModuleData()->m_multipleLives;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
UndeadBody::~UndeadBody( void )
{

}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void UndeadBody::attemptDamage( DamageInfo *damageInfo )
{
	// If we are on our first life, see if this damage will kill us.  If it will, bind it to one hitpoint
	// remaining, then go ahead and take it.
	Bool shouldStartSecondLife = FALSE;

	if( ( damageInfo->in.m_damageType != DAMAGE_UNRESISTABLE || damageInfo->in.m_notAbsoluteKill == TRUE ) 
			&& (!m_isSecondLife || m_currentMultipleLives > 0)
#if RETAIL_COMPATIBLE_CRC || PRESERVE_RETAIL_BEHAVIOR
			&& damageInfo->in.m_amount >= getHealth()
#else
			// TheSuperHackers @bugfix Stubbjax 20/09/2025 Battle Buses now correctly apply damage modifiers when calculating lethal damage
			&& estimateDamage(damageInfo->in) >= getHealth()
#endif
			&& IsHealthDamagingDamage(damageInfo->in.m_damageType)
			)
	{
		damageInfo->in.m_amount = min( damageInfo->in.m_amount, getHealth() - 1 );
		shouldStartSecondLife = TRUE;
		if(m_isSecondLife) m_currentMultipleLives--;
	}

	ActiveBody::attemptDamage(damageInfo);

	// After we take it (which allows for damaging special effects), we will do our modifications to the body module
	if( shouldStartSecondLife )
		startSecondLife(damageInfo);
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void UndeadBody::startSecondLife(DamageInfo *damageInfo)
{
	const UndeadBodyModuleData *data = getUndeadBodyModuleData();

	// Grant Upgrades function when starting Second Life
	if( !m_isSecondLife && !data->m_secondLifeUpgradeNames.empty() )
	{
		std::vector<AsciiString>::const_iterator it;
		for( it = data->m_secondLifeUpgradeNames.begin();
					it != data->m_secondLifeUpgradeNames.end();
					it++)
		{
			const UpgradeTemplate *upgradeTemplate = TheUpgradeCenter->findUpgrade( *it );
			if( !upgradeTemplate )
			{
				DEBUG_CRASH( ("SecondLifeUpgradeToGrant for %s can't find upgrade template %s.", getObject()->getName(), it->str() ) );
				return;
			}

			Player *player = getObject()->getControllingPlayer();
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
	
	// Remove Upgrades function when starting Second Life
	if( !m_isSecondLife && !data->m_secondLifeUpgradeRemoveNames.empty() )
	{
		std::vector<AsciiString>::const_iterator it;
		for( it = data->m_secondLifeUpgradeRemoveNames.begin();
					it != data->m_secondLifeUpgradeRemoveNames.end();
					it++)
		{
			const UpgradeTemplate* upgrade = TheUpgradeCenter->findUpgrade( *it );
			if( !upgrade )
			{
				DEBUG_CRASH(("An upgrade module references %s, which is not an Upgrade", it->str()));
				throw INI_INVALID_DATA;
			}
			if( upgrade )
			{
				//Check if it's a player Upgrade...
				if( upgrade->getUpgradeType() == UPGRADE_TYPE_PLAYER )
				{
					Player *player = getObject()->getControllingPlayer();
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
					DEBUG_CRASH( ("Object %s just Triggered Undead Body but is trying to remove upgrade %s",
						getObject()->getName(),
						it->str() ) );
				}
			}
		}
	}

	// Flag module as no longer intercepting damage
	m_isSecondLife = TRUE;

	// Modify ActiveBody's max health and initial health
	setMaxHealth(data->m_secondLifeMaxHealth, FULLY_HEAL);
	if(data->m_secondLifeSubdualCap>0) setSubdualCap(data->m_secondLifeSubdualCap);
	if(data->m_secondLifeSubdualHealRate>0)setSubdualHealRate(data->m_secondLifeSubdualHealRate);
	if(data->m_secondLifeSubdualHealAmount>0)setSubdualHealAmount(data->m_secondLifeSubdualHealAmount);
	
	//Customize to use multiple lives, not just two.
	if(m_currentMultipleLives < data->m_multipleLives) {

		//Whether there is adjustment to the Max Health Ratio per Revive.
		if(data->m_multipleLivesMaxHealthRatio != 1.0f)
		{
			Real healthFactor = data->m_multipleLivesMaxHealthRatio;
			if(data->m_multipleLivesMaxHealthRatioScale == TRUE){
				healthFactor = pow(data->m_multipleLivesMaxHealthRatio, (data->m_multipleLives - m_currentMultipleLives));
			}
			setMaxHealth(data->m_secondLifeMaxHealth*healthFactor, FULLY_HEAL);
		} 
		else 
		{
			setMaxHealth(data->m_secondLifeMaxHealth, FULLY_HEAL);
		}
		
		// Determine we want to override the heal value for the current amount of lives use for reviving.
		Int healArray = -1;

		if( !data->m_multipleLivesOverrideTrigger.empty() )
		{
			for(int x = 0; x < data->m_multipleLivesOverrideTrigger.size(); x++)
			{
				if(data->m_multipleLivesOverrideTrigger[x] <= 0) 
				{
					DEBUG_CRASH(("Invalid MultipleLivesOverrideTrigger value: %d at array %d.", data->m_multipleLivesMaxHealthOverride[x], x));
					throw INI_INVALID_DATA;
				}
				if(data->m_multipleLivesOverrideTrigger[x] == data->m_multipleLives - m_currentMultipleLives)
				{
					healArray = x;
					break;
				}
			}
		}
		if(healArray >= 0)
		{
			/*if(data->m_multipleLivesMaxHealthOverride.empty())
			{
				DEBUG_CRASH( ("MultipleLivesMaxHealthVariationOverride is empty while MultipleLivesOverrideTrigger is at Life: %d.", healArray) );
			}*/
			if(!data->m_multipleLivesMaxHealthOverride.empty() && data->m_multipleLivesMaxHealthOverride.size() > healArray && data->m_multipleLivesMaxHealthOverride[healArray]>0)
			{
				setMaxHealth(data->m_multipleLivesMaxHealthOverride[healArray], FULLY_HEAL);
			}
			if(!data->m_multipleLivesSubdualCapOverride.empty() && data->m_multipleLivesSubdualCapOverride.size() > healArray && data->m_multipleLivesSubdualCapOverride[healArray]>=0)
			{
				setSubdualCap(data->m_multipleLivesSubdualCapOverride[healArray]);
			}
			if(!data->m_multipleLivesSubdualHealRate.empty() && data->m_multipleLivesSubdualHealRate.size() > healArray && data->m_multipleLivesSubdualHealRate[healArray]>0)
			{
				setSubdualHealRate(data->m_multipleLivesSubdualHealRate[healArray]);
			}
			if(!data->m_multipleLivesSubdualCapOverride.empty() && data->m_multipleLivesSubdualHealAmount.size() > healArray && data->m_multipleLivesSubdualHealAmount[healArray]>0)
			{
				setSubdualHealAmount(data->m_multipleLivesSubdualHealAmount[healArray]);
			}
		}
		if( !data->m_multipleLivesUpgradeList.empty() )
		{
			std::vector<AsciiString>::const_iterator it;
			Int value;
			for( it = data->m_multipleLivesUpgradeList.begin();
						it != data->m_multipleLivesUpgradeList.end();
						it++)
			{
				const char* getChars = it->str();
				if(isdigit(*getChars)){
					if (sscanf( getChars, "%d", &value ) != 1)
					{
						DEBUG_CRASH( ("MultipleLivesUpgradesToGrant Value isn't a valid digit: %s.", it->str()) );
						throw INI_INVALID_DATA;
					}
					if(value < 0 || value > 65535)
					{
						DEBUG_CRASH( ("MultipleLivesUpgradesToGrant Value is invalid: %d.", value) );
						throw INI_INVALID_DATA;
					}
				}
				else if(value > 0 && data->m_multipleLives - m_currentMultipleLives == value)
				{
					const UpgradeTemplate* upgradeTemplate = TheUpgradeCenter->findUpgrade( *it );
					if( !upgradeTemplate )
					{
						DEBUG_CRASH(("An upgrade module references %s, which is not an Upgrade", it->str()));
						throw INI_INVALID_DATA;
					}

					Player *player = getObject()->getControllingPlayer();
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
		}
		if( !data->m_multipleLivesUpgradeRemoveList.empty() )
		{
			std::vector<AsciiString>::const_iterator it;
			Int value;
			for( it = data->m_multipleLivesUpgradeRemoveList.begin();
						it != data->m_multipleLivesUpgradeRemoveList.end();
						it++)
			{
				const char* getChars = it->str();
				if(isdigit(*getChars)){
					if (sscanf( getChars, "%d", &value ) != 1)
					{
						DEBUG_CRASH( ("MultipleLivesUpgradesToGrant Value isn't a valid digit: %s.", it->str()) );
						throw INI_INVALID_DATA;
					}
					if(value < 0 || value > 65535)
					{
						DEBUG_CRASH( ("MultipleLivesUpgradesToGrant Value is invalid: %d.", value) );
						throw INI_INVALID_DATA;
					}
				}
				else if(value > 0 && data->m_multipleLives - m_currentMultipleLives == value)
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
							Player *player = getObject()->getControllingPlayer();
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
							DEBUG_CRASH( ("Object %s just Triggered Undead Body for Multiple Lives: %d, but is trying to remove upgrade %s",
								getObject()->getName(),
								value,
								it->str() ) );
						}
					}
				}
			}
		}
	}
	// Set Armor set flag to use second life armor
	if(data->m_secondLifeArmorSetSwitch == 0 || data->m_multipleLives - m_currentMultipleLives == data->m_secondLifeArmorSetSwitch)
		setArmorSetFlag(ARMORSET_SECOND_LIFE);

	// Fire the Slow Death module.  The fact that this is not the result of an onDie will cause the special behavior
	Int total = 0;
	BehaviorModule** update = getObject()->getBehaviorModules();
	for( ; *update; ++update )
	{
		SlowDeathBehaviorInterface* sdu = (*update)->getSlowDeathBehaviorInterface();
		if (sdu != nullptr  && sdu->isDieApplicable(damageInfo) )
		{
			total += sdu->getProbabilityModifier( damageInfo );
		}
	}
	DEBUG_ASSERTCRASH(total > 0, ("Hmm, this is wrong"));


	// this returns a value from 1...total, inclusive
	Int roll = GameLogicRandomValue(1, total);

	for( update = getObject()->getBehaviorModules(); *update; ++update)
	{
		SlowDeathBehaviorInterface* sdu = (*update)->getSlowDeathBehaviorInterface();
		if (sdu != nullptr && sdu->isDieApplicable(damageInfo))
		{
			roll -= sdu->getProbabilityModifier( damageInfo );
			if (roll <= 0)
			{
				sdu->beginSlowDeath(damageInfo);
				return;
			}
		}
	}

}


// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void UndeadBody::crc( Xfer *xfer )
{

	// extend base class
	ActiveBody::crc( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void UndeadBody::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	ActiveBody::xfer( xfer );

	xfer->xferBool(&m_isSecondLife);
	xfer->xferInt(&m_currentMultipleLives);

}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void UndeadBody::loadPostProcess( void )
{

	// extend base class
	ActiveBody::loadPostProcess();

}

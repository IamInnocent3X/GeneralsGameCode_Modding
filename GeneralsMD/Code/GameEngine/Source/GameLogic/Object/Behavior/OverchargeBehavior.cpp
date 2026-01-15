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

// FILE: OverchargeBehavior.cpp ///////////////////////////////////////////////////////////////////
// Author: Colin Day, June 2002
// Desc:   Objects with this behavior module will get the ability to produce more power
//				 for a short amount of time, during this "overcharge" state object health is
//				 slowly reduced
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine
#include "Common/Player.h"
#include "Common/PlayerList.h"
#include "Common/Radar.h"
#include "Common/Xfer.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/Object.h"
#include "GameLogic/Module/BodyModule.h"
#include "GameLogic/Module/OverchargeBehavior.h"
#include "GameLogic/Module/PowerPlantUpdate.h"
#include "GameClient/Drawable.h"
#include "GameClient/InGameUI.h"
#include "GameClient/TintStatus.h"
#include "GameLogic/Weapon.h"
#include "GameLogic/Damage.h"



//-------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
OverchargeBehaviorModuleData::OverchargeBehaviorModuleData( void )
{

	m_healthPercentToDrainPerSecond = 0.0f;
	m_notAllowedWhenHealthBelowPercent = 0.0f;

	m_damageTypeFX = DAMAGE_PENALTY;
	m_bonusToSet.clear();
	m_statusToSet.clear();
	m_modelConditionToSet.clear();
	m_customBonusToSet.clear();
	m_customStatusToSet.clear();
	m_tintStatusToSet = TINT_STATUS_INVALID;
	m_customTintStatusToSet.clear();

	m_showDescriptionLabel = TRUE;
	m_overchargeOnLabel.format("TOOLTIP:TooltipNukeReactorOverChargeIsOn");
	m_overchargeOffLabel.format("TOOLTIP:TooltipNukeReactorOverChargeIsOff");

	m_showOverchargeExhausted = TRUE;
	m_overchargeExhaustedMessage.format("GUI:OverchargeExhausted");

}

//-------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
/*static*/ void OverchargeBehaviorModuleData::buildFieldParse( MultiIniFieldParse &p )
{

  UpdateModuleData::buildFieldParse( p );

	static const FieldParse dataFieldParse[] =
	{
		{ "HealthPercentToDrainPerSecond", INI::parsePercentToReal,	nullptr, offsetof( OverchargeBehaviorModuleData, m_healthPercentToDrainPerSecond ) },
		{ "NotAllowedWhenHealthBelowPercent", INI::parsePercentToReal, nullptr, offsetof( OverchargeBehaviorModuleData, m_notAllowedWhenHealthBelowPercent ) },

		// New Properties
		{ "OverchargeDamageTypeFX",		DamageTypeFlags::parseSingleBitFromINI,	nullptr, offsetof(OverchargeBehaviorModuleData, m_damageTypeFX) },
		{ "StatusToSet",			ObjectStatusMaskType::parseFromINI,	nullptr, offsetof( OverchargeBehaviorModuleData, m_statusToSet ) },
		{ "CustomStatusToSet",	INI::parseAsciiStringVector, nullptr, offsetof( OverchargeBehaviorModuleData, m_customStatusToSet ) },
		{ "WeaponBonusToSet",	INI::parseWeaponBonusVector, nullptr, offsetof( OverchargeBehaviorModuleData, m_bonusToSet ) },
		{ "CustomWeaponBonusToSet",			INI::parseAsciiStringVector, nullptr, offsetof( OverchargeBehaviorModuleData, m_customBonusToSet ) },
		{ "TintStatusToSet",			TintStatusFlags::parseSingleBitFromINI,		nullptr, offsetof( OverchargeBehaviorModuleData, m_tintStatusToSet ) },
		{ "CustomTintStatusToSet",	INI::parseAsciiString, 	nullptr, offsetof( OverchargeBehaviorModuleData, m_customTintStatusToSet ) },
		{ "ModelConditionToSet", ModelConditionFlags::parseFromINI, nullptr, offsetof( OverchargeBehaviorModuleData, m_modelConditionToSet ) },

		{ "ShowDescriptionLabel",	INI::parseBool, 	nullptr, offsetof( OverchargeBehaviorModuleData, m_showDescriptionLabel ) },
		{ "OverchargeOnDescriptionLabel",	INI::parseAsciiString, 	nullptr, offsetof( OverchargeBehaviorModuleData, m_overchargeOnLabel ) },
		{ "OverchargeOffDescriptionLabel",	INI::parseAsciiString, 	nullptr, offsetof( OverchargeBehaviorModuleData, m_overchargeOffLabel ) },

		{ "ShowOverchargeExhausted",	INI::parseBool, 	nullptr, offsetof( OverchargeBehaviorModuleData, m_showOverchargeExhausted ) },
		{ "OverchargeExhaustedMessage",	INI::parseAsciiString, 	nullptr, offsetof( OverchargeBehaviorModuleData, m_overchargeExhaustedMessage ) },
		{ nullptr, nullptr, nullptr, 0 }
	};

  p.add( dataFieldParse );

}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
OverchargeBehavior::OverchargeBehavior( Thing *thing, const ModuleData* moduleData )
									 : UpdateModule( thing, moduleData )
{

	m_overchargeActive = FALSE;

	// start off sleeping forever until we become active
	setWakeFrame( getObject(), UPDATE_SLEEP_FOREVER );

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
OverchargeBehavior::~OverchargeBehavior( void )
{

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
UpdateSleepTime OverchargeBehavior::update( void )
{

	// if the overcharge is active we need to take away some life
	if( m_overchargeActive )
	{
		Object *us = getObject();

		// get mod data
		const OverchargeBehaviorModuleData *modData = getOverchargeBehaviorModuleData();

		// do some damage
		/// IamInnocent - Adjusted Overcharge to be able to not deal damage
		BodyModuleInterface *body = us->getBodyModule();
		if(modData->m_healthPercentToDrainPerSecond > 0)
		{
			DamageInfo damageInfo;
			damageInfo.in.m_amount = (body->getMaxHealth() * modData->m_healthPercentToDrainPerSecond) / LOGICFRAMES_PER_SECOND;
			damageInfo.in.m_sourceID = us->getID();
			damageInfo.in.m_damageType = DAMAGE_PENALTY;
			damageInfo.in.m_deathType = DEATH_NORMAL;
			damageInfo.in.m_damageFXOverride = modData->m_damageTypeFX;
			us->attemptDamage( &damageInfo );
		}

		// see if our health is below the allowable threshold
		if( body->getHealth() < body->getMaxHealth() * modData->m_notAllowedWhenHealthBelowPercent )
		{

			// turn off the overcharge
			enable( FALSE );

			// do some UI info for the local user if this is theirs
			if( ThePlayerList->getLocalPlayer() == us->getControllingPlayer() && modData->m_showOverchargeExhausted )
			{

				// print msg
				TheInGameUI->message( modData->m_overchargeExhaustedMessage.str() );//( "GUI:OverchargeExhausted" );

				// do radar event
				TheRadar->createEvent( us->getPosition(), RADAR_EVENT_INFORMATION );

			}

			// do nothing else
			return UPDATE_SLEEP_NONE;

		}

	}

	return UPDATE_SLEEP_NONE;

}

// ------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void OverchargeBehavior::onDamage( DamageInfo *damageInfo )
{

}

// ------------------------------------------------------------------------------------------------
/** Flip the state of our 'overcharge-ness' */
// ------------------------------------------------------------------------------------------------
void OverchargeBehavior::toggle( void )
{

	// just toggle using enable()
	enable( !m_overchargeActive );

}

// ------------------------------------------------------------------------------------------------
/** Enable or disable an overcharge */
// ------------------------------------------------------------------------------------------------
void OverchargeBehavior::enable( Bool enable )
{
	Object *us = getObject();

	// get mod data
	const OverchargeBehaviorModuleData *modData = getOverchargeBehaviorModuleData();

	if( enable == FALSE )
	{

		// if we're turned on, turn off
		if( m_overchargeActive == TRUE )
		{

			// make sure to NOT extend rods for purpose of maintaining proper model condition
			PowerPlantUpdateInterface *ppui;
			for( BehaviorModule **umi = getObject()->getBehaviorModules(); *umi; ++umi)
			{
				ppui = (*umi)->getPowerPlantUpdateInterface();
				if( ppui )
					ppui->extendRods(FALSE);
			}

			Player *player = us->getControllingPlayer();
			if ( player )
				player->removePowerBonus(us);

			// Weapon Bonus to Clear
			for (Int i = 0; i < modData->m_bonusToSet.size(); i++) {
				us->clearWeaponBonusCondition(modData->m_bonusToSet[i]);
			}
			for(std::vector<AsciiString>::const_iterator it = modData->m_customBonusToSet.begin(); it != modData->m_customBonusToSet.end(); ++it)
			{
				us->clearCustomWeaponBonusCondition( *it );
			}

			// Status to Clear
			us->clearStatus(modData->m_statusToSet);
			for(std::vector<AsciiString>::const_iterator it = modData->m_customStatusToSet.begin(); it != modData->m_customStatusToSet.end(); ++it)
			{
				us->clearCustomStatus( *it );
			}

			Drawable *draw = us->getDrawable();
			if (draw)
			{
				// Tint Status to Clear
				if(!modData->m_customTintStatusToSet.isEmpty())
					draw->clearCustomTintStatus( modData->m_customTintStatusToSet );
				else if(modData->m_tintStatusToSet != TINT_STATUS_INVALID)
					draw->clearTintStatus( modData->m_tintStatusToSet );

				// Model Condition to Clear
				draw->clearModelConditionFlags(modData->m_modelConditionToSet);
			}

			// we are no longer active
			m_overchargeActive = FALSE;

			// sleep forever
			setWakeFrame( us, UPDATE_SLEEP_FOREVER );

		}

	}
	else
	{

		// if we're turned off, turn on
		if( m_overchargeActive == FALSE )
		{

			// make sure to extend rods for purpose of maintaining proper model condition
			PowerPlantUpdateInterface *ppui;
			for( BehaviorModule **umi = getObject()->getBehaviorModules(); *umi; ++umi)
			{
				ppui = (*umi)->getPowerPlantUpdateInterface();
				if( ppui )
					ppui->extendRods(TRUE);
			}

			// add the power bonus
			Player *player = us->getControllingPlayer();
			if ( player )
				player->addPowerBonus(us);

			// Weapon Bonus to Clear
			for (Int i = 0; i < modData->m_bonusToSet.size(); i++) {
				us->setWeaponBonusCondition(modData->m_bonusToSet[i]);
			}
			for(std::vector<AsciiString>::const_iterator it = modData->m_customBonusToSet.begin(); it != modData->m_customBonusToSet.end(); ++it)
			{
				us->setCustomWeaponBonusCondition( *it );
			}

			// Status to Set
			us->setStatus(modData->m_statusToSet);
			for(std::vector<AsciiString>::const_iterator it = modData->m_customStatusToSet.begin(); it != modData->m_customStatusToSet.end(); ++it)
			{
				us->setCustomStatus( *it );
			}

			Drawable *draw = us->getDrawable();
			if (draw)
			{
				// Tint Status to Set
				if(!modData->m_customTintStatusToSet.isEmpty())
					draw->setCustomTintStatus( modData->m_customTintStatusToSet );
				else if(modData->m_tintStatusToSet != TINT_STATUS_INVALID)
					draw->setTintStatus( modData->m_tintStatusToSet );

				// Model Condition to Set
				draw->setModelConditionFlags(modData->m_modelConditionToSet);
			}

			// we are now active
			m_overchargeActive = TRUE;

			// need to update every frame now
			setWakeFrame( us, UPDATE_SLEEP_NONE );

		}

	}

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void OverchargeBehavior::onDelete( void )
{

	// if we haven't been upgraded there is nothing to clean up
	if( m_overchargeActive == FALSE )
		return;

	// remove the power bonus from the player
	Player *player = getObject()->getControllingPlayer();
	if( player )
		player->removePowerBonus( getObject() );

	m_overchargeActive = FALSE;

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void OverchargeBehavior::onCapture( Player *oldOwner, Player *newOwner )
{

	// do nothing if we haven't upgraded yet
	if( m_overchargeActive == FALSE )
		return;

	if (getObject()->isDisabled())
		return;

	// remove power bonus from old owner
	if( oldOwner )
		oldOwner->removePowerBonus( getObject() );

	// add power bonus to the new owner
	if( newOwner )
		newOwner->addPowerBonus( getObject() );

}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void OverchargeBehavior::crc( Xfer *xfer )
{

	// extend base class
	UpdateModule::crc( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void OverchargeBehavior::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	UpdateModule::xfer( xfer );

	// overcharge active
	xfer->xferBool( &m_overchargeActive );

}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void OverchargeBehavior::loadPostProcess( void )
{

	// extend base class
	UpdateModule::loadPostProcess();

	// Our effect is a fire and forget effect, not an upgrade state that is itself saved, so need to re-fire.
	if( m_overchargeActive && getObject()->getControllingPlayer() )
		getObject()->getControllingPlayer()->addPowerBonus( getObject() );

}

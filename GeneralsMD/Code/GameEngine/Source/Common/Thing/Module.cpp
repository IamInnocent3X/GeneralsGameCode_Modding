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

// FILE: Module.cpp ///////////////////////////////////////////////////////////////////////////////
// Author: Colin Day, September 2001
// Desc:	 Object and drawable modules and actions.  These are simply just class
//				 instances that we can assign to objects, drawables, and things to contain
//				 data and code for specific events, or just to hold data
///////////////////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#define DEFINE_DIFFICULTY_NAMES					// for DifficultyNames[]

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "Common/Module.h"
#include "Common/Thing.h"
#include "Common/INI.h"
#include "Common/ThingTemplate.h"
#include "Common/Upgrade.h"
#include "Common/Xfer.h"
#include "Common/Player.h"
#include "GameLogic/Object.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/Module/BodyModule.h"
#include "GameLogic/Module/CollideModule.h"
#include "GameLogic/Module/ContainModule.h"
#include "GameLogic/Module/DamageModule.h"
#include "GameLogic/Module/DieModule.h"
#include "GameLogic/Module/UpdateModule.h"
#include "GameLogic/Module/UpgradeModule.h"
#include "GameLogic/Module/ProductionUpdate.h"


///////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS ///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
// this method should NEVER be overridden by user code, only via the MAKE_STANDARD_MODULE_xxx macros!
// it should also NEVER be called directly; it's only for use by ModuleFactory!
/*static*/ ModuleData* Module::friend_newModuleData(INI* ini)
{
	ModuleData* data = MSGNEW("Module::friend_newModuleData") ModuleData;	// no need to memorypool these since we never allocate more than one of each
	if (ini)
		ini->initFromINI(data, nullptr);	// this is just so that an "end" token is required
	return data;
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
Module::~Module()
{

}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void Module::crc( Xfer *xfer )
{

}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void Module::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

}

// ------------------------------------------------------------------------------------------------
/** load post process */
// ------------------------------------------------------------------------------------------------
void Module::loadPostProcess()
{

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
ObjectModule::ObjectModule( Thing *thing, const ModuleData* moduleData ) : Module(moduleData)
{
	if (!moduleData)
	{
		DEBUG_CRASH(("module data may not be null"));
		throw INI_INVALID_DATA;
	}

	DEBUG_ASSERTCRASH( thing, ("Thing passed to ObjectModule is null!") );
	m_object = AsObject(thing);
	DEBUG_ASSERTCRASH( m_object, ("Thing passed to ObjectModule is not an Object!") );

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
ObjectModule::~ObjectModule()
{

}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void ObjectModule::crc( Xfer *xfer )
{

	// extend base class
	Module::crc( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void ObjectModule::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	Module::xfer( xfer );

}

// ------------------------------------------------------------------------------------------------
/** load post process */
// ------------------------------------------------------------------------------------------------
void ObjectModule::loadPostProcess()
{

	// extend base class
	Module::loadPostProcess();

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
DrawableModule::DrawableModule( Thing *thing, const ModuleData* moduleData ) : Module(moduleData)
{
	if (!moduleData)
	{
		DEBUG_CRASH(("module data may not be null"));
		throw INI_INVALID_DATA;
	}

	DEBUG_ASSERTCRASH( thing, ("Thing passed to DrawableModule is null!") );
	m_drawable = AsDrawable(thing);
	DEBUG_ASSERTCRASH( m_drawable, ("Thing passed to DrawableModule is not a Drawable!") );

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
DrawableModule::~DrawableModule()
{

}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void DrawableModule::crc( Xfer *xfer )
{

	// extend base class
	Module::crc( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void DrawableModule::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	Module::xfer( xfer );

}

// ------------------------------------------------------------------------------------------------
/** load post process */
// ------------------------------------------------------------------------------------------------
void DrawableModule::loadPostProcess()
{

	// extend base class
	Module::loadPostProcess();

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------
UpgradeMuxData::UpgradeMuxData(void)
{
	m_triggerUpgradeNames.clear();
	m_activationUpgradeNames.clear();
	m_conflictingUpgradeNames.clear();
	m_removalUpgradeNames.clear();
	m_grantUpgradeNames.clear();
	m_initiallyActiveDifficulty.clear();
	m_startsActiveDifficultyForAI.clear();

	m_fxListUpgrade = nullptr;
	m_activationMask.clear();
	m_conflictingMask.clear();
	m_requiresAllTriggers = false;
	m_initiallyActive = false;
	m_startsActiveForAI = false;
	m_parsedStartsActiveForAI = false;
	m_startsActiveChecksForConflictsWith = false;
}

//-------------------------------------------------------------------------------------------------
/*static*/ const FieldParse* UpgradeMuxData::getFieldParse()
{
	static const FieldParse dataFieldParse[] =
	{
		{ "TriggeredBy",		INI::parseAsciiStringVector, nullptr, offsetof( UpgradeMuxData, m_activationUpgradeNames ) },
		{ "ConflictsWith",	INI::parseAsciiStringVector, nullptr, offsetof( UpgradeMuxData, m_conflictingUpgradeNames ) },
		{ "RemovesUpgrades",INI::parseAsciiStringVector, nullptr, offsetof( UpgradeMuxData, m_removalUpgradeNames ) },
		{ "GrantUpgrades",		INI::parseAsciiStringVector, nullptr, offsetof( UpgradeMuxData, m_grantUpgradeNames ) },
		{ "FXListUpgrade",	INI::parseFXList, nullptr, offsetof( UpgradeMuxData, m_fxListUpgrade ) },
		{ "RequiresAllTriggers", INI::parseBool, nullptr, offsetof( UpgradeMuxData, m_requiresAllTriggers ) },
		{ "StartsActive",	INI::parseBool, nullptr, offsetof( UpgradeMuxData, m_initiallyActive ) },
		{ "StartsActiveForAI", parseStartsActiveForAI, nullptr, offsetof( UpgradeMuxData, m_startsActiveForAI ) },
		{ "StartsActiveChecksForConflictsWith", INI::parseBool, nullptr, offsetof( UpgradeMuxData, m_startsActiveChecksForConflictsWith ) },
		{ "StartsActiveDifficultySettings",	parseDifficultyBoolVector, nullptr, offsetof( UpgradeMuxData, m_initiallyActiveDifficulty ) },
		{ "StartsActiveDifficultySettingsForAI", parseDifficultyBoolVector, nullptr, offsetof( UpgradeMuxData, m_startsActiveDifficultyForAI ) },
		{ 0, 0, 0, 0 }
	};
	return dataFieldParse;
}

//-------------------------------------------------------------------------------------------------
void UpgradeMuxData::parseStartsActiveForAI( INI* ini, void *instance, void *store, const void* /*userData*/ )
{
	UpgradeMuxData* self = (UpgradeMuxData*)instance;
	self->m_parsedStartsActiveForAI = TRUE;

	*(Bool*)store = INI::scanBool(ini->getNextToken());
}

//-------------------------------------------------------------------------------------------------
void UpgradeMuxData::parseDifficultyBoolVector( INI* ini, void * /*instance*/, void *store, const void* /*userData*/ )
{
	std::pair<Int, Bool> data;
	Bool ParseNext = FALSE;
	Int count = 0;

	DifficultyBoolVec* s = (DifficultyBoolVec*)store;
	s->clear();

	for (const char *token = ini->getNextTokenOrNull(); token != nullptr; token = ini->getNextTokenOrNull())
	{
		count++;
		if(count > DIFFICULTY_COUNT * 2)
		{
			DEBUG_CRASH(("Invalid configuration of Difficulty to Amount of DifficultyBoolVector"));
			throw INI_INVALID_DATA;
		}
		if(!ParseNext)
		{
			data.first = INI::scanIndexList(token, TheDifficultyNames);
			ParseNext = TRUE;
		}
		else 
		{
			data.second = INI::scanBool(token);
			s->push_back(data);
			ParseNext = FALSE;
		}
	}
	if(ParseNext)
	{
		DEBUG_CRASH(("Invalid configuration of Difficulty to Amount of DifficultyBoolVector"));
		throw INI_INVALID_DATA;
	}
}

//-------------------------------------------------------------------------------------------------
void UpgradeMuxData::performUpgradeFX(Object* obj) const
{
	if (m_fxListUpgrade)
	{
		FXList::doFXObj(m_fxListUpgrade, obj);
	}
}

//-------------------------------------------------------------------------------------------------
void UpgradeMuxData::muxDataProcessUpgradeRemoval(Object* obj) const
{
	if( !m_removalUpgradeNames.empty() )
	{
		std::vector<AsciiString>::const_iterator it;
		for( it = m_removalUpgradeNames.begin();
					it != m_removalUpgradeNames.end();
					it++)
		{
			const UpgradeTemplate* theTemplate = TheUpgradeCenter->findUpgrade( *it );
			if( !theTemplate )
			{
				DEBUG_CRASH(("An upgrade module references '%s', which is not an Upgrade", it->str()));
				throw INI_INVALID_DATA;
			}
			if( theTemplate->getUpgradeType() == UPGRADE_TYPE_PLAYER )
			{
				Player *player = obj->getControllingPlayer();
				player->removeUpgrade( theTemplate );
			}
			else
			{
				obj->removeUpgrade(theTemplate);
			}
		}
	}
}

void UpgradeMuxData::muxDataProcessUpgradeGrant(Object* obj) const
{
	if( !m_grantUpgradeNames.empty() )
	{
		std::vector<AsciiString>::const_iterator it;
		for( it = m_grantUpgradeNames.begin();
					it != m_grantUpgradeNames.end();
					it++)
		{
			const UpgradeTemplate* theTemplate = TheUpgradeCenter->findUpgrade( *it );
			if( !theTemplate )
			{
				DEBUG_CRASH(("An upgrade module references '%s', which is not an Upgrade", it->str()));
				throw INI_INVALID_DATA;
			}
			if( !theTemplate )
			{
				DEBUG_CRASH( ("ProcessUpgradeGrant for %s can't find upgrade template %s.", obj->getName(), it->str() ) );
				return;
			}

			Player *player = obj->getControllingPlayer();
			if( theTemplate->getUpgradeType() == UPGRADE_TYPE_PLAYER )
			{
				// find and cancel any existing upgrades in the player's queue
				player->findUpgradeInQueuesAndCancelThem( theTemplate );
				// get the player
				player->addUpgrade( theTemplate, UPGRADE_STATUS_COMPLETE );
			}
			else
			{
				// Fail safe if in any other condition, for example: Undead Body, or new Future Implementations such as UpgradeDie to Give Upgrades.
				ProductionUpdateInterface *pui = obj->getProductionUpdateInterface();
				if( pui )
				{
					pui->cancelUpgrade( theTemplate );
				}
				obj->giveUpgrade( theTemplate );
			}
			
			player->getAcademyStats()->recordUpgrade( theTemplate, TRUE );
		}
	}
}

//-------------------------------------------------------------------------------------------------
Bool UpgradeMuxData::isTriggeredBy(const std::string &upgrade) const
{
	std::vector<AsciiString>::const_iterator it;
	for( it = m_triggerUpgradeNames.begin(); it != m_triggerUpgradeNames.end();	++it)
	{
		AsciiString trigger = *it;
		if (stricmp(trigger.str(), upgrade.c_str()) == 0)
		{
			return TRUE;
		}
	}

	return FALSE;
}

//-------------------------------------------------------------------------------------------------
void UpgradeMuxData::getUpgradeActivationMasks(UpgradeMaskType& activation, UpgradeMaskType& conflicting) const
{
	// already computed.
	if (!m_activationUpgradeNames.empty() || !m_conflictingUpgradeNames.empty())
	{
		m_activationMask.clear();
		m_conflictingMask.clear();

		std::vector<AsciiString>::const_iterator it;
		for( it = m_activationUpgradeNames.begin();
					it != m_activationUpgradeNames.end();
					it++)
		{
			const UpgradeTemplate* theTemplate = TheUpgradeCenter->findUpgrade( *it );
			if( !theTemplate )
			{
				DEBUG_CRASH(("An upgrade module references '%s', which is not an Upgrade", it->str()));
				throw INI_INVALID_DATA;
			}

			m_activationMask.set( theTemplate->getUpgradeMask() );
		}

		for( it = m_conflictingUpgradeNames.begin();
					it != m_conflictingUpgradeNames.end();
					it++)
		{
			const UpgradeTemplate* theTemplate = TheUpgradeCenter->findUpgrade( *it );
			if( !theTemplate )
			{
				DEBUG_CRASH(("An upgrade module references '%s', which is not an Upgrade", it->str()));
				throw INI_INVALID_DATA;
			}

			m_conflictingMask.set( theTemplate->getUpgradeMask() );
		}

		// We set the trigger upgrade names with the activationUpgradeNames entries to be used later.
		// We have to do this because the activationUpgradeNames are toasted just below.
		m_triggerUpgradeNames = m_activationUpgradeNames;

		//Clear the names now that we've cached the values!
		m_activationUpgradeNames.clear();
		m_conflictingUpgradeNames.clear();
	}
	activation = m_activationMask;
	conflicting = m_conflictingMask;
}

//-------------------------------------------------------------------------------------------------
Bool UpgradeMuxData::muxDataCheckStartsActive(const Object* obj) const
{
	// Sanity
	if(!obj)
		return FALSE;

	if(m_startsActiveChecksForConflictsWith)
	{
		// Check for any conflicts with
		const UpgradeMaskType& objectMask = obj->getObjectCompletedUpgradeMask();
		UpgradeMaskType maskToCheck = objectMask;
		if(obj->getControllingPlayer())
		{
			const UpgradeMaskType& playerMask = obj->getControllingPlayer()->getCompletedUpgradeMask();
			maskToCheck.set( playerMask );
		}

		UpgradeMaskType activation, conflicting;
		getUpgradeActivationMasks(activation, conflicting);
		if(maskToCheck.testForAny( conflicting ))
			return FALSE;
	}

	// Initiate the variables based on StartsActive
	Bool startsActive = m_initiallyActive;
	DifficultyBoolVec checkVec = m_initiallyActiveDifficulty;
	GameDifficulty difficulty;

	if(obj->getControllingPlayer())
	{
		// Check different values for AI
		if(obj->getControllingPlayer()->getPlayerType() == PLAYER_COMPUTER)
		{
			startsActive = m_parsedStartsActiveForAI ? m_startsActiveForAI : startsActive;
			checkVec = !m_startsActiveDifficultyForAI.empty() ? m_startsActiveDifficultyForAI : checkVec;
		}

		// Check whether it applies to different difficulties
		difficulty = obj->getControllingPlayer()->getPlayerDifficulty();
		for( DifficultyBoolVec::const_iterator it = checkVec.begin(); it != checkVec.end();	++it)
		{
			if (it->first == difficulty)
			{
				startsActive = it->second;
				break;
			}
		}
	}

	return startsActive;
}

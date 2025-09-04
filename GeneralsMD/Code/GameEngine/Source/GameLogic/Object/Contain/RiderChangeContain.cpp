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

// FILE: RiderChangeContain.cpp //////////////////////////////////////////////////////////////////////
// Author: Kris Morness, May 2003
// Desc:   Contain module for the combat bike (transport that switches units).
///////////////////////////////////////////////////////////////////////////////////////////////////

// USER INCLUDES //////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file int the GameEngine

#define DEFINE_LOCOMOTORSET_NAMES //Gain access to TheLocomotorSetNames[]

#include "Common/Player.h"
#include "Common/PlayerList.h"
#include "Common/ThingTemplate.h"
#include "Common/ThingFactory.h"
#include "Common/Upgrade.h"
#include "Common/Xfer.h"

#include "GameClient/ControlBar.h"
#include "GameClient/Drawable.h"
#include "GameClient/InGameUI.h"

#include "GameLogic/AI.h"
#include "GameLogic/AIPathfind.h"
#include "GameLogic/ExperienceTracker.h"
#include "GameLogic/Object.h"
#include "GameLogic/Locomotor.h"

#include "GameLogic/Module/AIUpdate.h"
#include "GameLogic/Module/BodyModule.h"
#include "GameLogic/Module/PhysicsUpdate.h"
#include "GameLogic/Module/StealthUpdate.h"
#include "GameLogic/Module/RiderChangeContain.h"




// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
RiderChangeContainModuleData::RiderChangeContainModuleData()
{
	m_scuttleFrames = 0;
	m_scuttleState = MODELCONDITION_TOPPLED;
	m_ridersCustom.clear();
	m_riderNotRequired = FALSE;
	m_useUpgradeNames = FALSE;
	m_dontScuttle = FALSE;
	m_dontDestroyRiderOnKill = FALSE;
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void RiderChangeContainModuleData::parseRiderInfo( INI* ini, void *instance, void *store, const void* /*userData*/ )
{
	RiderInfo* rider = (RiderInfo*)store;
	const char* name = ini->getNextToken();

	//Template name
	rider->m_templateName.format( name );

	//Model condition state
	INI::parseIndexList( ini, instance, &(rider->m_modelConditionFlagType), ModelConditionFlags::getBitNames() );

	//Weaponset
	INI::parseIndexList( ini, instance, &(rider->m_weaponSetFlag), WeaponSetFlags::getBitNames() );

	//Object status
	//INI::parseIndexList( ini, instance, &(rider->m_objectStatusType), ObjectStatusMaskType::getBitNames() );
	name = ini->getNextToken();
	Int count = 0;
	Int bitindex = -1;
	for(ConstCharPtrArray findname = ObjectStatusMaskType::getBitNames(); *findname; findname++, count++ )
	{
		if( stricmp( *findname, name ) == 0 )
		{
			bitindex = count;
			break;
		}
	}
	if(bitindex>=0)
	{
		rider->m_objectStatusType = (ObjectStatusType)bitindex;
		rider->m_objectCustomStatusType = NULL;
	}
	else
	{
		rider->m_objectStatusType = (ObjectStatusType)0;
		rider->m_objectCustomStatusType.format(name);
	}

	//Command set override
	name = ini->getNextToken();
	rider->m_commandSet.format( name );

	//Locomotor set type
	rider->m_locomotorSetType = (LocomotorSetType)INI::scanIndexList( ini->getNextToken(), TheLocomotorSetNames );
}

void RiderChangeContainModuleData::parseRiderInfoCustom(INI* ini, void* instance, void* store, const void* /*userData*/)
{
	RiderInfo rider;
	const char* name = ini->getNextToken();

	//Template name
	rider.m_templateName.format(name);

	//Model condition state
	INI::parseIndexList(ini, instance, &(rider.m_modelConditionFlagType), ModelConditionFlags::getBitNames());

	//Weaponset
	INI::parseIndexList(ini, instance, &(rider.m_weaponSetFlag), WeaponSetFlags::getBitNames());

	//Object status
	name = ini->getNextToken();
	Int count = 0;
	Int bitindex = -1;
	for(ConstCharPtrArray findname = ObjectStatusMaskType::getBitNames(); *findname; findname++, count++ )
	{
		if( stricmp( *findname, name ) == 0 )
		{
			bitindex = count;
			break;
		}
	}
	if (bitindex >= 0)
	{
		rider.m_objectStatusType = (ObjectStatusType)bitindex;
		rider.m_objectCustomStatusType = NULL;
	}
	else
	{
		rider.m_objectStatusType = (ObjectStatusType)0;
		rider.m_objectCustomStatusType.format(name);
	}

	//Command set override
	name = ini->getNextToken();
	rider.m_commandSet.format(name);

	//Locomotor set type
	rider.m_locomotorSetType = (LocomotorSetType)INI::scanIndexList(ini->getNextToken(), TheLocomotorSetNames);

	std::vector<RiderInfo>* s = (std::vector<RiderInfo>*)store;
	s->push_back(rider);
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void RiderChangeContainModuleData::buildFieldParse(MultiIniFieldParse& p)
{
  TransportContainModuleData::buildFieldParse(p);

	static const FieldParse dataFieldParse[] =
	{
		{ "Rider1",					parseRiderInfo,					NULL, offsetof( RiderChangeContainModuleData, m_riders[0] ) },
		{ "Rider2",					parseRiderInfo,					NULL, offsetof( RiderChangeContainModuleData, m_riders[1] ) },
		{ "Rider3",					parseRiderInfo,					NULL, offsetof( RiderChangeContainModuleData, m_riders[2] ) },
		{ "Rider4",					parseRiderInfo,					NULL, offsetof( RiderChangeContainModuleData, m_riders[3] ) },
		{ "Rider5",					parseRiderInfo,					NULL, offsetof( RiderChangeContainModuleData, m_riders[4] ) },
		{ "Rider6",					parseRiderInfo,					NULL, offsetof( RiderChangeContainModuleData, m_riders[5] ) },
		{ "Rider7",					parseRiderInfo,					NULL, offsetof( RiderChangeContainModuleData, m_riders[6] ) },
		{ "Rider8",					parseRiderInfo,					NULL, offsetof( RiderChangeContainModuleData, m_riders[7] ) },
		{ "RiderCustom",			parseRiderInfoCustom,			NULL, offsetof(RiderChangeContainModuleData, m_ridersCustom) },
		{ "RiderChangeOnStatusTypes",	INI::parseBool,					NULL, offsetof(RiderChangeContainModuleData, m_riderNotRequired) },
		{ "RiderUseUpgradeNames",		INI::parseBool,					NULL, offsetof(RiderChangeContainModuleData, m_useUpgradeNames) },
    { "ScuttleDelay",   INI::parseDurationUnsignedInt,	NULL, offsetof( RiderChangeContainModuleData, m_scuttleFrames ) },
    { "ScuttleStatus",  INI::parseIndexList,		ModelConditionFlags::getBitNames(), offsetof( RiderChangeContainModuleData, m_scuttleState ) },
	{ "DontDestroyRiderOnKill",		INI::parseBool,				NULL, offsetof( RiderChangeContainModuleData, m_dontDestroyRiderOnKill ) },
	{ "DontScuttleOnExit",		INI::parseBool,					NULL, offsetof( RiderChangeContainModuleData, m_dontScuttle ) },
		{ 0, 0, 0, 0 }
	};
  p.add(dataFieldParse);
}



// PRIVATE ////////////////////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
Int RiderChangeContain::getContainMax( void ) const
{
	if (getRiderChangeContainModuleData())
		return getRiderChangeContainModuleData()->m_slotCapacity;

	return 0;
}

// PUBLIC /////////////////////////////////////////////////////////////////////////////////////////
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
RiderChangeContain::RiderChangeContain( Thing *thing, const ModuleData *moduleData ) :
								 TransportContain( thing, moduleData )
{
	m_extraSlotsInUse = 0;
	m_frameExitNotBusy = 0;
	m_containing = FALSE;
	m_scuttledOnFrame = 0;
	m_dontCompare = FALSE;
	//m_theRiderToChange = -1;
	//m_theRiderName = NULL;
	//m_theRiderLastValid = -1;
	//m_theRiderLastName = NULL;
	//m_theRiderLastUpgrade = -1;
	//m_theRiderLastUpgradeName = NULL;
	//m_theRiderLastValidBackup = -1;
	//m_theRiderLastNameBackup = NULL;
	//m_theRiderToChangeBackup = -1;
	//m_theRiderNameBackup = NULL;
	//m_checkedRemove = TRUE;
	m_loaded = FALSE;
	//m_firstChange = FALSE;
	//m_firstUpgrade = FALSE;
	//m_firstStatusRemoval = FALSE;
	//m_statusDelayCheck = FALSE;
	//m_doRemovalOnLoad = FALSE;
	//m_addDelay = 0;
	//m_statusLoadDelay = 0;
	//m_resetLaterFrame = 0;
	m_theRiderDataRecord.clear(); // This is a better version of the features above
	m_registeredUpgradeNames = FALSE;
	m_prevStatus.clear();
	m_prevCustomStatusTypes.clear();
	m_prevMaskToCheck.clear();
	m_upgradeTemplates.clear();
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
RiderChangeContain::~RiderChangeContain( void )
{

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
/**
	can this container contain this kind of object?
	and, if checkCapacity is TRUE, does this container have enough space left to hold the given unit?
*/
Bool RiderChangeContain::isValidContainerFor(const Object* rider, Bool checkCapacity) const
{
	//Don't check capacity because our rider will kick the other rider out!
	if( TransportContain::isValidContainerFor( rider, FALSE ) )
	{
		if( m_scuttledOnFrame != 0 )
		{
			//Scuttled... too late!
			return FALSE;
		}

		//We can enter this bike... but now we need to extend the base functionality by limiting
		//which infantry can enter.
		const RiderChangeContainModuleData *data = getRiderChangeContainModuleData();
		for( int i = 0; i < MAX_RIDERS; i++ )
		{
			const ThingTemplate *thing = TheThingFactory->findTemplate( data->m_riders[ i ].m_templateName );
			if( thing && thing->isEquivalentTo( rider->getTemplate() ) )
			{
				//We found a valid rider, so return success.
				return TRUE;
			}
		}
		for (std::vector<RiderInfo>::const_iterator it = data->m_ridersCustom.begin(); it != data->m_ridersCustom.end(); ++it)
		{
			const ThingTemplate* thing = TheThingFactory->findTemplate((*it).m_templateName);
			if ( thing->isEquivalentTo( rider->getTemplate() ) )
			{
				//We found a valid rider, so return success.
				return TRUE;
			}
		}
	}
	return FALSE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void RiderChangeContain::onContaining( Object *rider, Bool wasSelected )
{
	Object *obj = getObject();
	m_containing = TRUE;

	//Remove our existing rider
	if( m_payloadCreated )
	{
		obj->getAI()->aiEvacuateInstantly( TRUE, CMD_FROM_AI );
	}

	//If the rider is currently selected, transfer selection to the container and preserve other units
	//that may be already selected. Note that containing the rider will automatically cause it to be
	//deselected, so all we have to do is select the container (if not already selected)!
	Drawable *containDraw = getObject()->getDrawable();
	if( containDraw && wasSelected && !containDraw->isSelected() )
	{
		//Create the selection message
		GameMessage *teamMsg = TheMessageStream->appendMessage( GameMessage::MSG_CREATE_SELECTED_GROUP );
		teamMsg->appendBooleanArgument( FALSE );// not creating new team so pass false
		teamMsg->appendObjectIDArgument( getObject()->getID() );
		TheInGameUI->selectDrawable( containDraw );
		TheInGameUI->setDisplayedMaxWarning( FALSE );
	}

	if(!m_loaded && !m_theRiderDataRecord.empty())
	{
		TransportContain::onContaining( rider, wasSelected );

		m_containing = FALSE;
		return;
	}

	//Find the rider in the list and set the appropriate model condition
	const RiderChangeContainModuleData *data = getRiderChangeContainModuleData();
	Bool found;
	for( int i = 0; i < MAX_RIDERS; i++ )
	{
		found = riderChangeContainingCheck(rider, data->m_riders[ i ]);
		if (found == TRUE)
		{
			// Prevent the template from being removed if there is currently any Status Affecting the Unit
			//m_checkedRemove = TRUE;

			// Hackky way to fix the Upgrade Template on Loading issue.
			//if(m_loaded == TRUE)
			//{
				//m_theRiderLastUpgrade = -1;
				//m_theRiderLastUpgradeName = NULL;

				// Clear the Data Record since the New Rider is given by Containing
				// m_theRiderDataRecord.clear();
			//}

			// Register the new rider index for checking.
			//m_theRiderToChange = i;
			//m_theRiderName = NULL;

			// Clear the Rider Data Record and assign it as the Last Template, only if the game is loaded or its past its first frame,
			// or else it will register new values on loading.
			// Register it as the first Template becuase the Upgrade and Status Checks will always need a template to refer to.
			if(m_loaded || m_theRiderDataRecord.empty())
			{
				RiderData riderData;
				char charName[2];
				sprintf( charName, "%d", i );
				riderData.templateName = charName;
				riderData.timeFrame = 1; // For indicating it as a Containing Template, in case of usage
				riderData.statusType = (ObjectStatusType)0;
				riderData.customStatusType = NULL;

				m_theRiderDataRecord.clear();

				m_theRiderDataRecord.push_back(riderData);
			}

			break;
		}

		/*const ThingTemplate* thing = TheThingFactory->findTemplate(data->m_riders[i].m_templateName);
		if( thing && thing->isEquivalentTo( rider->getTemplate() ) )
		{

			//This is our rider, so set the correct model condition.
			obj->setModelConditionState( data->m_riders[ i ].m_modelConditionFlagType );

			//Also set the correct weaponset flag
			obj->setWeaponSetFlag( data->m_riders[ i ].m_weaponSetFlag );

			//Also set the object status
			obj->setStatus( MAKE_OBJECT_STATUS_MASK( data->m_riders[ i ].m_objectStatusType ) );

			//Set the new commandset override
			obj->setCommandSetStringOverride( data->m_riders[ i ].m_commandSet );
			TheControlBar->markUIDirty();	// Refresh the UI in case we are selected

			//Change the locomotor.
			AIUpdateInterface* ai = obj->getAI();
			if( ai )
			{
				ai->chooseLocomotorSet( data->m_riders[ i ].m_locomotorSetType );
			}

			if( obj->getStatusBits().test( OBJECT_STATUS_STEALTHED ) )
			{
				StealthUpdate* stealth = obj->getStealth();
				if( stealth )
				{
					stealth->markAsDetected();
				}
			}

			//Transfer experience from the rider to the bike.
			ExperienceTracker *riderTracker = rider->getExperienceTracker();
			ExperienceTracker *bikeTracker = obj->getExperienceTracker();
			bikeTracker->setVeterancyLevel( riderTracker->getVeterancyLevel(), FALSE );
			riderTracker->setExperienceAndLevel( 0, FALSE );

			break;
		}*/
	}
	if (!found && !data->m_ridersCustom.empty())
	{
		for (std::vector<RiderInfo>::const_iterator it = data->m_ridersCustom.begin(); it != data->m_ridersCustom.end(); ++it)
		{
			found = riderChangeContainingCheck( rider, (*it) );
			if (found == TRUE)
			{
				// Set this in case when the new Status Types fade out and the Object depends on the status type for weapon switching 
				//m_checkedRemove = TRUE;

				// Hackky way to fix the Upgrade Template and Stautus Change List on Loading issue.
				//if(m_loaded == TRUE)
				//{
					//m_theRiderLastUpgrade = -1;
					//m_theRiderLastUpgradeName = NULL;

					// Clear the Data Record since the New Rider is given by Containing
					// m_theRiderDataRecord.clear();
				//}

				// Register the new rider name for checking.
				//m_theRiderName = (*it).m_templateName;

				// Clear the Rider Data Record and assign it as the Last Template, only if the game is loaded or its past its first frame,
				// or else it will register new values on loading.
				// Register it as the first Template becuase the Upgrade and Status Checks will always need a template to refer to.
				if(m_loaded || m_theRiderDataRecord.empty())
				{
					RiderData riderData;
					riderData.templateName = (*it).m_templateName;
					riderData.timeFrame = 1; // For indicating it as a Containing Template, in case of usage
					riderData.statusType = (ObjectStatusType)0;
					riderData.customStatusType = NULL;

					m_theRiderDataRecord.clear();

					m_theRiderDataRecord.push_back(riderData);
				}

				break;
			}
		}
	}

	//Extend base class
	TransportContain::onContaining( rider, wasSelected );

	// Save Load Stuff, must do Everytime the game is loaded because OnContaining screws up the current status after loaded.
	//if(m_firstChange)
	//{
	//	riderRemoveAll();
	//	riderGiveTemplate(m_theRiderToChange, m_theRiderName);
	//	m_checkedRemove = FALSE;
	//}

	m_containing = FALSE;
}

Bool RiderChangeContain::riderChangeContainingCheck(Object* rider, const RiderInfo& riderInfo)
{
	Object* obj = getObject();
	const ThingTemplate* thing = TheThingFactory->findTemplate( riderInfo.m_templateName );
	if (thing->isEquivalentTo(rider->getTemplate() ) )
	{
		//if(m_firstStatusRemoval && m_statusLoadDelay <= 0 && m_statusDelayCheck == TRUE && m_theRiderToChangeBackup>= 0)
		//{
		//	riderRemoveTemplate(m_theRiderToChangeBackup, m_theRiderNameBackup);
		//	m_theRiderToChangeBackup = -1;
		//}

		// If the applied status is removed from the unit, remove the statuses respectively from the unit.
		//riderRemoveTemplate(m_theRiderToChange, m_theRiderName);

		// Skip an instance of Comparison after the New Rider has been set. Mandatory everytime a new Template is set.
		m_dontCompare = TRUE;

		//riderRemoveAll();

		//This is our rider, so set the correct model condition.
		obj->setModelConditionState( riderInfo.m_modelConditionFlagType );

		//Also set the correct weaponset flag
		obj->setWeaponSetFlag( riderInfo.m_weaponSetFlag );

		//Also set the object status
		if (riderInfo.m_objectCustomStatusType.isEmpty())
		{
			obj->setStatus( MAKE_OBJECT_STATUS_MASK( riderInfo.m_objectStatusType ) );
		}
		else
		{
			obj->setCustomStatus( riderInfo.m_objectCustomStatusType );
		}

		//Set the new commandset override
		obj->setCommandSetStringOverride( riderInfo.m_commandSet );
		TheControlBar->markUIDirty();	// Refresh the UI in case we are selected

		//Change the locomotor.
		AIUpdateInterface* ai = obj->getAI();
		if (ai)
		{
			ai->chooseLocomotorSet( riderInfo.m_locomotorSetType );
		}

		if (obj->getStatusBits().test( OBJECT_STATUS_STEALTHED ) )
		{
			StealthUpdate* stealth = obj->getStealth();
			if ( stealth )
			{
				stealth->markAsDetected();
			}
		}

		//Transfer experience from the rider to the bike.
		ExperienceTracker* riderTracker = rider->getExperienceTracker();
		ExperienceTracker* bikeTracker = obj->getExperienceTracker();
		bikeTracker->setVeterancyLevel( riderTracker->getVeterancyLevel(), FALSE );
		riderTracker->setExperienceAndLevel( 0, FALSE );

		return TRUE;
	}
	return FALSE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void RiderChangeContain::onRemoving( Object *rider )
{
	Object *bike = getObject();
	const RiderChangeContainModuleData *data = getRiderChangeContainModuleData();
	//Note if the bike dies, the rider dies too.
	if( !data->m_dontDestroyRiderOnKill && bike->isEffectivelyDead() )
	{
		TheGameLogic->destroyObject( rider );
		return;
	}

	if( m_payloadCreated )
	{
		//Extend base class
		TransportContain::onRemoving( rider );
	}

	//Find the rider in the list and clear various data.
	Bool found = FALSE;

	for( int i = 0; i < MAX_RIDERS; i++ )
	{
		found = riderChangeRemoveCheck( rider, data->m_riders[ i ] );
		if (found == TRUE)
		{
			char charName[2];
			sprintf( charName, "%d", i );
			removeRiderDataRecord(charName);
			break;
		}

		/*const ThingTemplate* thing = TheThingFactory->findTemplate(data->m_riders[i].m_templateName);
		if( thing && thing->isEquivalentTo( rider->getTemplate() ) )
		{
			//This is our rider, so clear the current model condition.
			bike->clearModelConditionFlags( MAKE_MODELCONDITION_MASK2( data->m_riders[ i ].m_modelConditionFlagType, MODELCONDITION_DOOR_1_CLOSING ) );

			//Also clear the current weaponset flag
			bike->clearWeaponSetFlag( data->m_riders[ i ].m_weaponSetFlag );

			//Also clear the object status
			bike->clearStatus( MAKE_OBJECT_STATUS_MASK( data->m_riders[ i ].m_objectStatusType ) );

			if( rider->getControllingPlayer() != NULL )
			{
				//Wow, completely unforseeable game teardown order crash.  SetVeterancyLevel results in a call to player
				//about upgrade masks.  So if we have a null player, it is game teardown, so don't worry about transfering exp.

				//Transfer experience from the bike to the rider.
				ExperienceTracker *riderTracker = rider->getExperienceTracker();
				ExperienceTracker *bikeTracker = bike->getExperienceTracker();
				riderTracker->setVeterancyLevel( bikeTracker->getVeterancyLevel(), FALSE );
				bikeTracker->setExperienceAndLevel( 0, FALSE );
			}

			break;
		}*/
	}
	if (!found && !data->m_ridersCustom.empty())
	{
		for (std::vector<RiderInfo>::const_iterator it = data->m_ridersCustom.begin(); it != data->m_ridersCustom.end(); ++it)
		{
			found = riderChangeRemoveCheck(rider, (*it));
			if (found == TRUE)
			{
				removeRiderDataRecord((*it).m_templateName);
				break;
			}
		}
	}

	//If we're not replacing the rider, then if the cycle is selected, transfer selection
	//to the rider getting off (because the bike is gonna blow).
	if( !m_containing && !data->m_dontScuttle)
	{
		Drawable *containDraw = bike->getDrawable();
		Drawable *riderDraw = rider->getDrawable();
		if( containDraw && riderDraw )
		{
			//Create the selection message for the rider if it's ours and SELECTED!
			if( bike->getControllingPlayer() == ThePlayerList->getLocalPlayer() && containDraw->isSelected() )
			{
				GameMessage *teamMsg = TheMessageStream->appendMessage( GameMessage::MSG_CREATE_SELECTED_GROUP );
				teamMsg->appendBooleanArgument( FALSE );// not creating new team so pass false
				teamMsg->appendObjectIDArgument( rider->getID() );
				TheInGameUI->selectDrawable( riderDraw );
				TheInGameUI->setDisplayedMaxWarning( FALSE );

				//Create the de-selection message for the container
				teamMsg = TheMessageStream->appendMessage( GameMessage::MSG_REMOVE_FROM_SELECTED_GROUP );
				teamMsg->appendObjectIDArgument( bike->getID() );
				TheInGameUI->deselectDrawable( containDraw );
			}

			//Finally, scuttle the bike so nobody else can use it! <Design Spec>
			m_scuttledOnFrame = TheGameLogic->getFrame();
			bike->setStatus( MAKE_OBJECT_STATUS_MASK( OBJECT_STATUS_UNSELECTABLE ) );
			bike->setModelConditionState( data->m_scuttleState );
			if( !bike->getAI()->isMoving() )
			{
				bike->setStatus( MAKE_OBJECT_STATUS_MASK( OBJECT_STATUS_IMMOBILE ) );
			}
		}
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
Bool RiderChangeContain::riderChangeRemoveCheck(Object* rider, const RiderInfo& riderInfo)
{
	Object* bike = getObject();
	const ThingTemplate* thing = TheThingFactory->findTemplate(riderInfo.m_templateName);
	if( thing && thing->isEquivalentTo( rider->getTemplate() ) )
	{
		// Skip an instance of Comparison after the An Old Rider has been removed, as the Status Type may change.
		m_dontCompare = TRUE;
		
		//This is our rider, so clear the current model condition.
		bike->clearModelConditionFlags( MAKE_MODELCONDITION_MASK2(riderInfo.m_modelConditionFlagType, MODELCONDITION_DOOR_1_CLOSING ) );

		//Also clear the current weaponset flag
		bike->clearWeaponSetFlag( riderInfo.m_weaponSetFlag );

		//Also clear the object status
		if ( riderInfo.m_objectCustomStatusType.isEmpty() )
		{
			bike->clearStatus( MAKE_OBJECT_STATUS_MASK( riderInfo.m_objectStatusType ) );
		}
		else
		{
			bike->clearCustomStatus( riderInfo.m_objectCustomStatusType );
		}
		
		if ( rider->getControllingPlayer() != NULL )
		{
			//Wow, completely unforseeable game teardown order crash.  SetVeterancyLevel results in a call to player
			//about upgrade masks.  So if we have a null player, it is game teardown, so don't worry about transfering exp.

			//Transfer experience from the bike to the rider.
			ExperienceTracker* riderTracker = rider->getExperienceTracker();
			ExperienceTracker* bikeTracker = bike->getExperienceTracker();
			riderTracker->setVeterancyLevel( bikeTracker->getVeterancyLevel(), FALSE );
			bikeTracker->setExperienceAndLevel( 0, FALSE );
		}

		return TRUE;
	}
	return FALSE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void RiderChangeContain::removeRiderDataRecord(const AsciiString& rider)
{
	if(m_theRiderDataRecord.empty())
		return;

	std::vector<RiderData>::iterator it;
	for (it = m_theRiderDataRecord.begin(); it != m_theRiderDataRecord.end();)
	{
		if ((*it).templateName == rider)
		{
			it = m_theRiderDataRecord.erase(it);
			break;
		}
		++it;
	}
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void RiderChangeContain::createPayload()
{
	// extend base class
	TransportContain::createPayload();
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
UpdateSleepTime RiderChangeContain::update()
{
	const RiderChangeContainModuleData* data = getRiderChangeContainModuleData();

	// we need to determine if the game has just been recently 'loaded' to prevent Bikes from desyncing with their templates
	if(!m_containing && !m_loaded)
	{
		loadPreviousState();
		m_loaded = TRUE;
	}

	// Don't compare the Status Types after the Template Switch, because OnContaining also sets and removes Statuses
	if(m_dontCompare)
	{
		Object* obj = getObject();
		
		UpgradeMaskType playerMask = obj->getControllingPlayer()->getCompletedUpgradeMask();
		UpgradeMaskType objectMask = obj->getObjectCompletedUpgradeMask();
		m_prevMaskToCheck = playerMask;
		m_prevMaskToCheck.set( objectMask );

		m_prevStatus = obj->getStatusBits();
		m_prevCustomStatusTypes = obj->getCustomStatus();

		m_dontCompare = FALSE;
	}

	if( m_scuttledOnFrame != 0 )
	{
		//Bike in the process of getting scuttled.
		//const RiderChangeContainModuleData *data = getRiderChangeContainModuleData();
		UnsignedInt now = TheGameLogic->getFrame();
		if( m_scuttledOnFrame + data->m_scuttleFrames <= now )
		{
			//We have scuttled the bike (at least as far as tipping it over via scuttle animation. Now
			//kill the bike in a way that will cause it to sink into the ground without any real destruction.
			getObject()->kill( DAMAGE_UNRESISTABLE, DEATH_TOPPLED ); //Sneaky, eh? Toppled heheh.
		}
	}
	// extend base class
	return TransportContain::update(); //extend
}

Bool RiderChangeContain::riderTemplateIsValidChange(const AsciiString& newCustomStatus)
{
	const RiderChangeContainModuleData* data = getRiderChangeContainModuleData();

	Bool isValidChange = FALSE;

	// Get if the new Status Type is within the Unit's Rider list.
	for( int i = 0; i < MAX_RIDERS; i++ )
	{
		if (!data->m_riders[ i ].m_objectCustomStatusType.isEmpty() && data->m_riders[ i ].m_objectCustomStatusType == newCustomStatus)
		{
			isValidChange = TRUE;
			
				// Save Load Stuff, clear everything once loaded because OnContaining screws up loading existing statuses on Riders
				//if(m_firstChange)
				//	riderRemoveAll();

				// Also Save Load Stuff, related to fix Real Time Statuses not loaded on saves.
				//if(m_firstStatusRemoval && m_statusLoadDelay <= 0 && m_statusDelayCheck == TRUE && m_theRiderToChangeBackup>= 0)
				//{
				//	m_theRiderToChangeBackup = -1;
				//}

				// Remove both the current and last Rider Templates because if a new status is applied on top of the other
				// It will only remove one (invalid) template that has been updated from last time a Status is set
				//riderRemoveTemplate(m_theRiderToChange, m_theRiderName);

				//riderRemoveTemplate(m_theRiderLastValid, m_theRiderLastName);

				// Statuses screwed up this system, so just reset everything
				//riderRemoveAll();

				//m_theRiderLastValid = m_theRiderToChange;
				//m_theRiderLastName = m_theRiderName;

				//Find the rider in the latest list and clear various data.
				removeRiderTemplate(m_theRiderDataRecord[m_theRiderDataRecord.size()-1].templateName, FALSE);

				// Register the current Frame for assigning the New Template
				RiderData riderData;
				char charName[2];
				sprintf( charName, "%d", i );
				riderData.templateName = charName;
				riderData.timeFrame = TheGameLogic->getFrame();
				riderData.statusType = (ObjectStatusType)0;
				riderData.customStatusType = newCustomStatus;

				// Give the new template
				riderGiveTemplate(riderData);

				//m_theRiderToChange = i;
				//m_theRiderName = NULL;

			break;
		}
	}
	// If not found within default Rider List, check Custom Rider List
	if(!isValidChange)
	{
		for (std::vector<RiderInfo>::const_iterator it = data->m_ridersCustom.begin(); it != data->m_ridersCustom.end(); ++it)
		{
			if ( !(*it).m_objectCustomStatusType.isEmpty() && (*it).m_objectCustomStatusType == newCustomStatus )
			{
				isValidChange = TRUE;
				
					// Save Load Stuff, clear everything once loaded because OnContaining screws up loading existing statuses on Riders
					//if(m_firstChange)
					//	riderRemoveAll();

					// Also Save Load Stuff, related to fix Real Time Statuses not loaded on saves.
					//if(m_firstStatusRemoval && m_statusLoadDelay <= 0 && m_statusDelayCheck == TRUE && m_theRiderToChangeBackup>= 0)
					//{
					//	m_theRiderToChangeBackup = -1;
					//}

					// Remove both the current and last Rider Templates because if a new status is applied on top of the other
					// It will only remove one (invalid) template that has been updated from last time a Status is set
					//riderRemoveTemplate(m_theRiderToChange, m_theRiderName);

					//riderRemoveTemplate(m_theRiderLastValid, m_theRiderLastName);

					// Statuses screwed up this system, so just reset everything
					//riderRemoveAll();
					
					//m_theRiderLastValid = m_theRiderToChange;
					//m_theRiderLastName = m_theRiderName;

					//Find the rider in the latest list and clear various data.
					removeRiderTemplate(m_theRiderDataRecord[m_theRiderDataRecord.size()-1].templateName, FALSE);

					// Register the current Frame for assigning the New Template
					RiderData riderData;
					riderData.templateName = (*it).m_templateName;
					riderData.timeFrame = TheGameLogic->getFrame();
					riderData.statusType = (ObjectStatusType)0;
					riderData.customStatusType = newCustomStatus;
					
					// Give the new template
					riderGiveTemplate(riderData);

					//m_theRiderName = (*it).m_templateName;

				break;
			}
		}
	}
	return isValidChange;
}

//Bool RiderChangeContain::riderTemplateIsValidChange(Int newStatus, Bool set)
Bool RiderChangeContain::riderTemplateIsValidChange(ObjectStatusMaskType newStatus)
{
	const RiderChangeContainModuleData* data = getRiderChangeContainModuleData();

	Bool isValidChange = FALSE;

	// Get if the new Status Type is within the Unit's Rider list.
	for( int i = 0; i < MAX_RIDERS; i++ )
	{
		if( newStatus.test((ObjectStatusTypes)data->m_riders[ i ].m_objectStatusType) )
		{
			isValidChange = TRUE;

				// Save Load Stuff, clear everything once loaded because OnContaining screws up loading existing statuses on Riders
				//if(m_firstChange)
				//	riderRemoveAll();

				// Also Save Load Stuff, related to fix Real Time Statuses not loaded on saves.
				//if(m_firstStatusRemoval && m_statusLoadDelay <= 0 && m_statusDelayCheck == TRUE && m_theRiderToChangeBackup>= 0)
				//{
				//	m_theRiderToChangeBackup = -1;
				//}

				// Remove both the current and last Rider Templates because if a new status is applied on top of the other
				// It will only remove one (invalid) template that has been updated from last time a Status is set
				//riderRemoveTemplate(m_theRiderToChange, m_theRiderName);

				//riderRemoveTemplate(m_theRiderLastValid, m_theRiderLastName);

				// Statuses screwed up this system, so just reset everything
				//riderRemoveAll();

				//m_theRiderLastValid = m_theRiderToChange;
				//m_theRiderLastName = m_theRiderName;

				//Find the rider in the latest list and clear various data.
				removeRiderTemplate(m_theRiderDataRecord[m_theRiderDataRecord.size()-1].templateName, FALSE);

				// Register the current Frame for assigning the New Template
				RiderData riderData;
				char charName[2];
				sprintf( charName, "%d", i );
				riderData.templateName = charName;
				riderData.timeFrame = TheGameLogic->getFrame();
				riderData.statusType = data->m_riders[ i ].m_objectStatusType;
				riderData.customStatusType = NULL;

				// Give the new template
				riderGiveTemplate(riderData);

				//m_theRiderToChange = i;
				//m_theRiderName = NULL;

			break;
		}
	}
	// If not found within default Rider List, check Custom Rider List
	if(!isValidChange)
	{
		for (std::vector<RiderInfo>::const_iterator it = data->m_ridersCustom.begin(); it != data->m_ridersCustom.end(); ++it)
		{
			if( newStatus.test((ObjectStatusTypes)((*it).m_objectStatusType)) )
			{
				isValidChange = TRUE;

					// Also Save Load Stuff, related to fix Real Time Statuses not loaded on saves.
					//if(m_firstStatusRemoval && m_statusLoadDelay <= 0 && m_statusDelayCheck == TRUE && m_theRiderToChangeBackup>= 0)
					//{
					//	m_theRiderToChangeBackup = -1;
					//}

					// Remove both the current and last Rider Templates because if a new status is applied on top of the other
					// It will only remove one (invalid) template that has been updated from last time a Status is set
					//riderRemoveTemplate(m_theRiderToChange, m_theRiderName);

					//riderRemoveTemplate(m_theRiderLastValid, m_theRiderLastName);

					// Statuses screwed up this system, so just reset everything
					//riderRemoveAll();

					//m_theRiderLastValid = m_theRiderToChange;
					//m_theRiderLastName = m_theRiderName;

					//Find the rider in the latest list and clear various data.
					removeRiderTemplate(m_theRiderDataRecord[m_theRiderDataRecord.size()-1].templateName, FALSE);

					// Register the current Frame for assigning the New Template
					RiderData riderData;
					riderData.templateName = (*it).m_templateName;
					riderData.timeFrame = TheGameLogic->getFrame();
					riderData.statusType = (ObjectStatusType)(*it).m_objectStatusType;
					riderData.customStatusType = NULL;
					
					// Give the new template
					riderGiveTemplate(riderData);

					//m_theRiderName = (*it).m_templateName;

				break;
			}
		}
	}
	return isValidChange;
}

Bool RiderChangeContain::riderTemplateIsValidRemoval(ObjectStatusMaskType oldStatus)
{
	UnsignedInt LongestFrame = 0;
	UnsignedInt StatusTimeFrame = 0;
	AsciiString TemplateName = NULL;

	//DEBUG_LOG(("Checking riderTemplateIsValidRemoval. Frame: %d.", TheGameLogic->getFrame()));

	for (std::vector<RiderData>::iterator it = m_theRiderDataRecord.begin(); it != m_theRiderDataRecord.end(); ++it)
	{
		if(!LongestFrame || (*it).timeFrame > LongestFrame)
			LongestFrame = (*it).timeFrame;

		if((*it).statusType != 0 && oldStatus.test((*it).statusType))
		{
			TemplateName = (*it).templateName;
			StatusTimeFrame = (*it).timeFrame;
		}
	}

	// Do nothing if there's no matching Template
	if(TemplateName.isEmpty())
		return FALSE;

	//DEBUG_LOG(("Template to remove: %s.", TemplateName.str()));

	removeRiderDataRecord(TemplateName); // Remove it from the Data Record

	removeRiderTemplate(TemplateName, FALSE); // Remove the Template from the List

	// It is the last Template, remove it and grant the last template.
	if(StatusTimeFrame == LongestFrame && !m_theRiderDataRecord.empty())
	{
		// Grant the last Template of the Data Record, but don't Register it
		RiderData riderData;
		riderData.templateName = m_theRiderDataRecord[m_theRiderDataRecord.size()-1].templateName;
		riderData.timeFrame = 0;
		riderData.statusType = (ObjectStatusType)0;
		riderData.customStatusType = NULL;

		//DEBUG_LOG(("Template to give: %s. Not assigning", riderData.templateName.str()));

		// Grant the Rider the last Template
		riderGiveTemplate(riderData);
	}

	return TRUE;
}

Bool RiderChangeContain::riderTemplateIsValidRemoval(const AsciiString& oldCustomStatus)
{
	UnsignedInt LongestFrame = 0;
	UnsignedInt StatusTimeFrame = 0;
	AsciiString TemplateName = NULL;
	for (std::vector<RiderData>::iterator it = m_theRiderDataRecord.begin(); it != m_theRiderDataRecord.end(); ++it)
	{
		if(!LongestFrame || (*it).timeFrame > LongestFrame)
			LongestFrame = (*it).timeFrame;

		if((*it).customStatusType == oldCustomStatus)
		{
			TemplateName = (*it).templateName;
			StatusTimeFrame = (*it).timeFrame;
		}

		//DEBUG_LOG(("Template Name: %s, Time Frame: %d", (*it).templateName.str(), (*it).timeFrame));
	}

	//DEBUG_LOG(("StatusTimeFrame: %d, Time Frame: %d", StatusTimeFrame, LongestFrame));

	// Do nothing if there's no matching Template
	if(TemplateName.isEmpty())
	{
		return FALSE;
	}

	//DEBUG_LOG(("Template to remove: %s.", TemplateName.str()));

	removeRiderDataRecord(TemplateName); // Remove it from the Data Record

	removeRiderTemplate(TemplateName, FALSE); // Remove the Template from the List
	
	// It is the last Template, remove it and grant the last template.
	if(StatusTimeFrame == LongestFrame && !m_theRiderDataRecord.empty())
	{
		// Grant the last Template of the Data Record, but don't Register it
		RiderData riderData;
		riderData.templateName = m_theRiderDataRecord[m_theRiderDataRecord.size()-1].templateName;
		riderData.timeFrame = 0;
		riderData.statusType = (ObjectStatusType)0;
		riderData.customStatusType = NULL;

		//DEBUG_LOG(("Template to give: %s. Not assigning", riderData.templateName.str()));

		// Grant the Rider the last Template
		riderGiveTemplate(riderData);
	}

	return TRUE;
}

// Function to Change the Object according to the Rider Template.
void RiderChangeContain::riderGiveTemplate(RiderData riderData)
{
	// Skip an instance of Comparison after the New Rider has been set. Mandatory everytime a new Template is set.
	m_dontCompare = TRUE;

	// Save Load fixes, we don't want to do anything with the Records if the game has just been loaded
	// Register the New Template if it isn't granted OnContaining or if it wasn't switching to the last Template
	if(m_loaded && riderData.timeFrame)
		m_theRiderDataRecord.push_back(riderData);
	
	const RiderChangeContainModuleData* data = getRiderChangeContainModuleData();
	const char* RiderChar = riderData.templateName.str();
	Object* obj = getObject();

	// After we clear the previous condition types, we register the new conditions.
	// We first check whether the Rider is within Index List 
	if(isdigit(*RiderChar))
	{
		Int RiderIndex;
		if (sscanf( RiderChar, "%d", &RiderIndex ) != 1)
		{
			DEBUG_ASSERTCRASH( 0, ("RiderIndex isn't a valid digit: %s.", RiderChar) );
			throw INI_INVALID_DATA;
		}
		if(RiderIndex < 0 || RiderIndex > 8)
		{
			DEBUG_ASSERTCRASH( 0, ("RiderIndex is invalid: %d.", RiderIndex) );
			throw INI_INVALID_DATA;
		}

		if (data->m_riders[RiderIndex].m_objectCustomStatusType.isEmpty())
		{
			obj->setStatus( MAKE_OBJECT_STATUS_MASK( data->m_riders[RiderIndex].m_objectStatusType ) );
		}
		else
		{
			obj->setCustomStatus( data->m_riders[RiderIndex].m_objectCustomStatusType );
		}
		
		//This is our rider, so set the correct model condition.
		obj->setModelConditionState(data->m_riders[RiderIndex].m_modelConditionFlagType);

		//Also set the correct weaponset flag
		obj->setWeaponSetFlag(data->m_riders[RiderIndex].m_weaponSetFlag);

		//Set the new commandset override
		obj->setCommandSetStringOverride(data->m_riders[RiderIndex].m_commandSet);

		//Change the locomotor.
		AIUpdateInterface* ai = obj->getAI();
		if (ai)
		{
			ai->chooseLocomotorSet(data->m_riders[RiderIndex].m_locomotorSetType);
		}
	}
	// If it is not within Index List, then it must be from the Custom List.
	else
	{
		for (std::vector<RiderInfo>::const_iterator it = data->m_ridersCustom.begin(); it != data->m_ridersCustom.end(); ++it)
		{
			if((*it).m_templateName == riderData.templateName)
			{
				if ( (*it).m_objectCustomStatusType.isEmpty() )
				{
					obj->setStatus( MAKE_OBJECT_STATUS_MASK( (*it).m_objectStatusType ) );
				}
				else
				{
					obj->setCustomStatus( (*it).m_objectCustomStatusType );
				}
				
				//This is our rider, so set the correct model condition.
				obj->setModelConditionState((*it).m_modelConditionFlagType);

				//Also set the correct weaponset flag
				obj->setWeaponSetFlag((*it).m_weaponSetFlag);

				//Set the new commandset override
				obj->setCommandSetStringOverride((*it).m_commandSet);

				//Change the locomotor.
				AIUpdateInterface* ai = obj->getAI();
				if (ai)
				{
					ai->chooseLocomotorSet((*it).m_locomotorSetType);
				}

				break;
			}
		}
	}

	TheControlBar->markUIDirty();	// Refresh the UI in case we are selected

	if (obj->getStatusBits().test(OBJECT_STATUS_STEALTHED))
	{
		StealthUpdate* stealth = obj->getStealth();
		if (stealth)
		{
			stealth->markAsDetected();
		}
	}

	// Set this in case when the new Status Types fade out and the Object depends on the status type for weapon switching 
	//m_checkedRemove = TRUE;

	// Hackky way to fix the Upgrade Template on Loading issue.
	//m_theRiderLastUpgrade = -1;
	//m_theRiderLastUpgradeName = NULL;
}


void RiderChangeContain::removeRiderTemplate(const AsciiString& rider, Bool clearStatus)
{
	//m_firstChange = FALSE;
	m_dontCompare = TRUE;

	const RiderChangeContainModuleData* data = getRiderChangeContainModuleData();
	const char* RiderChar = rider.str();
	Object* obj = getObject();

	// After we clear the previous condition types, we register the new conditions.
	// We first check whether the Rider is within Index List 
	if(isdigit(*RiderChar))
	{
		Int RiderIndex;
		if (sscanf( RiderChar, "%d", &RiderIndex ) != 1)
		{
			DEBUG_ASSERTCRASH( 0, ("RiderIndex isn't a valid digit: %s.", RiderChar) );
			throw INI_INVALID_DATA;
		}
		if(RiderIndex < 0 || RiderIndex > 8)
		{
			DEBUG_ASSERTCRASH( 0, ("RiderIndex is invalid: %d.", RiderIndex) );
			throw INI_INVALID_DATA;
		}
		
		//This is our rider, so clear the current model condition.
		obj->clearModelConditionFlags( MAKE_MODELCONDITION_MASK( data->m_riders[ RiderIndex ].m_modelConditionFlagType ) );

		//Also clear the current weaponset flag
		obj->clearWeaponSetFlag( data->m_riders[ RiderIndex ].m_weaponSetFlag );

		// Don't clear its Status for Status Checks!
		if(!clearStatus)
			return;

		//Also clear the object status
		if (data->m_riders[ RiderIndex ].m_objectCustomStatusType.isEmpty())
		{
			obj->clearStatus(MAKE_OBJECT_STATUS_MASK(data->m_riders[ RiderIndex ].m_objectStatusType));
		}
		else
		{
			obj->clearCustomStatus(data->m_riders[ RiderIndex ].m_objectCustomStatusType);
		}
	}
	// If it is not within Index List, then it must be from the Custom List.
	else
	{
		for (std::vector<RiderInfo>::const_iterator it = data->m_ridersCustom.begin(); it != data->m_ridersCustom.end(); ++it)
		{
			if((*it).m_templateName == rider)
			{
				//This is our rider, so clear the current model condition.
				obj->clearModelConditionFlags( MAKE_MODELCONDITION_MASK( (*it).m_modelConditionFlagType ) );

				//Also clear the current weaponset flag
				obj->clearWeaponSetFlag( (*it).m_weaponSetFlag );

				// Don't clear its Status for Status Checks!
				if(!clearStatus)
					break;

				//Also clear the object status
				if ((*it).m_objectCustomStatusType.isEmpty())
				{
					obj->clearStatus(MAKE_OBJECT_STATUS_MASK((*it).m_objectStatusType));
				}
				else
				{
					obj->clearCustomStatus((*it).m_objectCustomStatusType);
				}

				break;
			}
		}
	}
}

// Stripping function that basically removes all Rider Templates from the Unit. 
//... Desperate times calls for desperate measures. 
void RiderChangeContain::riderRemoveAll()
{
	//m_firstChange = FALSE;
	m_dontCompare = TRUE;
	
	const RiderChangeContainModuleData* data = getRiderChangeContainModuleData();

	Object* obj = getObject();

	for( int i = 0; i < MAX_RIDERS; i++ )
	{
		obj->clearModelConditionFlags( MAKE_MODELCONDITION_MASK( data->m_riders[ i ].m_modelConditionFlagType ) );

		obj->clearWeaponSetFlag( data->m_riders[ i ].m_weaponSetFlag );

		if (data->m_riders[ i ].m_objectCustomStatusType.isEmpty())
		{
			obj->clearStatus(MAKE_OBJECT_STATUS_MASK(data->m_riders[ i ].m_objectStatusType));
		}
		else
		{
			obj->clearCustomStatus(data->m_riders[ i ].m_objectCustomStatusType);
		}
	}
	for (std::vector<RiderInfo>::const_iterator it = data->m_ridersCustom.begin(); it != data->m_ridersCustom.end(); ++it)
	{
		obj->clearModelConditionFlags( MAKE_MODELCONDITION_MASK( (*it).m_modelConditionFlagType ) );

		obj->clearWeaponSetFlag( (*it).m_weaponSetFlag );

		if ((*it).m_objectCustomStatusType.isEmpty())
		{
			obj->clearStatus(MAKE_OBJECT_STATUS_MASK((*it).m_objectStatusType));
		}
		else
		{
			obj->clearCustomStatus((*it).m_objectCustomStatusType);
		}
	}
}

// Register the Upgrades declared within Rider Names 
// This process is carried out only once. As it get from the Riders List
void RiderChangeContain::doRegisterUpgradeNames()
{
	const RiderChangeContainModuleData* data = getRiderChangeContainModuleData();

	// Only do it if UseUpgrade Names is set to 'Yes'
	if (data->m_useUpgradeNames == FALSE)
		return;
	
	// Check for each riders in the default Rider's list
	for( int i = 0; i < MAX_RIDERS; i++ )
	{
		const UpgradeTemplate* upgradeTemplate = TheUpgradeCenter->findUpgrade( data->m_riders[ i ].m_templateName );
		if( upgradeTemplate )
		{
			RiderUpgrade rd;
			rd.templateName = data->m_riders[ i ].m_templateName;
			rd.templateRider = i;
			m_upgradeTemplates.push_back(rd);
		}
	}
	// Also check for the Custom Rider List
	for (std::vector<RiderInfo>::const_iterator it = data->m_ridersCustom.begin(); it != data->m_ridersCustom.end(); ++it)
	{
		const UpgradeTemplate* upgradeTemplate = TheUpgradeCenter->findUpgrade( (*it).m_templateName );
		if( upgradeTemplate )
		{
			RiderUpgrade rd;
			rd.templateName = (*it).m_templateName;
			m_upgradeTemplates.push_back(rd);
		}
	}

	// We only do this once. Don't do it everytime.
	m_registeredUpgradeNames = TRUE;
}

// Update the Module every frame, but has a lot of checks to prevent updates because of For Loops.
// Could also do in Object::updateUpgradeModule(). More efficient, but less robust.
// 3 weeks later - Done

void RiderChangeContain::loadPreviousState()
{
	// Already loaded
	//if(m_loaded)
	//	return;

	// Nothing to Load, but shouldn't happen. Sanity
	if(m_theRiderDataRecord.empty())
		return;
	
	Object *obj = getObject();

	// Prevent Switching Template from Statuses
	m_dontCompare = TRUE;

	// Update all the statuses and Upgrade Masks to the current one
	UpgradeMaskType playerMask = obj->getControllingPlayer()->getCompletedUpgradeMask();
	UpgradeMaskType objectMask = obj->getObjectCompletedUpgradeMask();
	m_prevMaskToCheck = playerMask;
	m_prevMaskToCheck.set( objectMask );

	m_prevStatus = obj->getStatusBits();
	m_prevCustomStatusTypes = obj->getCustomStatus();

	// Register the Upgrade Names
	if(getRiderChangeContainModuleData()->m_useUpgradeNames)
		doRegisterUpgradeNames();

	// Grant the last Template of the Data Record, but don't Register it
	RiderData riderData;
	riderData.templateName = m_theRiderDataRecord[m_theRiderDataRecord.size()-1].templateName;
	riderData.timeFrame = 0;
	riderData.statusType = (ObjectStatusType)0;
	riderData.customStatusType = NULL;

	// Grant the Rider the last Template
	riderGiveTemplate(riderData);

}
// The main Rider Switching function for StatusTypes
void RiderChangeContain::doStatusChecks()
{
	// Prevent Switching States
	if(m_dontCompare)
	{
		return;
	}

	// There's nothing to refer to remove template
	if(m_theRiderDataRecord.empty())
		return;
	
	// If the Object doesn't use Status to change its Template, do nothing
	if (getRiderChangeContainModuleData()->m_riderNotRequired == FALSE)
		return;

	// We do not want to recheck Status to set everything that has been compared
	m_dontCompare = TRUE;

	Object* obj = getObject();

	ObjectStatusMaskType NewStatus = obj->getStatusBits();
	ObjectCustomStatusType NewCustomStatusTypes = obj->getCustomStatus();
	AsciiString NewCustomStatus;

	Bool checkChange = FALSE;

	// Check for the differences in the change in statuses.
	// Also get the values changed to register the new condition to the unit.
	if(NewStatus != m_prevStatus)
	{
		//DEBUG_LOG(("Difference between New Status and Old Status, Frame: %d", TheGameLogic->getFrame()));
		ObjectStatusMaskType oldStatus = m_prevStatus;
		oldStatus.clear(NewStatus);
		NewStatus.clear(m_prevStatus);

		// Check if there are any New Status assigned, and if there is, is it a Valid Change?
		// The Template assigning function is within the Conditional Checks
		if(riderTemplateIsValidChange( NewStatus ) )
		{
			//DEBUG_LOG(("New Status granted, Frame: %d", TheGameLogic->getFrame()));
			checkChange = TRUE;
		}
		// If it's not a Template granted by a Status, maybe it's a Status Removal?
		if(!checkChange && oldStatus.any() && riderTemplateIsValidRemoval( oldStatus ))
		{
			//DEBUG_LOG(("New Status removed, Frame: %d", TheGameLogic->getFrame()));
			checkChange = TRUE;
		}
	}
	if(!checkChange && NewCustomStatusTypes != m_prevCustomStatusTypes)
	{
		//DEBUG_LOG(("Rider Template: Checking Custom Status. Frame: %d", TheGameLogic->getFrame()));
		for (ObjectCustomStatusType::const_iterator it = NewCustomStatusTypes.begin(); it != NewCustomStatusTypes.end(); ++it)
		{
			if((*it).second == 1)
			{
				// If the status is not present in the Previous Status, yet present Recently, check if it is a Valid Status to change a Rider
				ObjectCustomStatusType::const_iterator it2 = m_prevCustomStatusTypes.find((*it).first);
				if(m_prevCustomStatusTypes.empty() || it2 == m_prevCustomStatusTypes.end() || ( it2 != m_prevCustomStatusTypes.end() && (*it2).second == 0) )
				{
					if(riderTemplateIsValidChange( (*it).first ) )
					{
						break;
					}
				} 
			}
			else
			{
				// If the status is present in the Previous Status, yet not present Recently, check if it is a Valid Template Removal
				ObjectCustomStatusType::const_iterator it2 = m_prevCustomStatusTypes.find((*it).first);
				if(it2 != m_prevCustomStatusTypes.end() && (*it2).second == 1)
				{
					if(riderTemplateIsValidRemoval( (*it).first ) )
					{
						break;
					}
				}
			}
		}
	}

	// Register the Status Types for future checking
	//m_prevStatus = NewStatus;
	//m_prevCustomStatusTypes = NewCustomStatusTypes;
}

void RiderChangeContain::doUpgradeChecks()
{
	// Prevent Switching States
	if(m_dontCompare)
		return;
	
	const RiderChangeContainModuleData* data = getRiderChangeContainModuleData();

	// If we do not Use Upgrade Template to Switch Riders, do nothing
	if (data->m_useUpgradeNames == FALSE)
		return;
	
	// Register the UpgradeNames if it is first present
	if (!m_registeredUpgradeNames)
		doRegisterUpgradeNames();

	// If there is no Upgrade Names, do nothing
	if (m_upgradeTemplates.empty())
		return;

	// There's nothing to switch back to
	if(m_theRiderDataRecord.empty())
		return;

	Object *obj = getObject();	

	// Get the Upgrade Mask
	UpgradeMaskType playerMask = obj->getControllingPlayer()->getCompletedUpgradeMask();
	UpgradeMaskType objectMask = obj->getObjectCompletedUpgradeMask();
	UpgradeMaskType maskToCheck = playerMask;
	maskToCheck.set( objectMask );

	// Do nothing if the Upgrade states are the same as previous ones
	if(maskToCheck == m_prevMaskToCheck)
		return;

	// Now we are checking, we do not want to do any checking for the next instance
	m_dontCompare = TRUE;

	// If any new Upgrades are installed check for that instead.
	UpgradeMaskType newMask = maskToCheck;
	newMask.clear( m_prevMaskToCheck );

	// Else if any Upgrade is cleared, we want to remove its Template
	UpgradeMaskType oldMask = m_prevMaskToCheck;
	oldMask.clear(maskToCheck);

	// Register the new Upgrade Statuses
	m_prevMaskToCheck = maskToCheck;

	for (std::vector<RiderUpgrade>::const_iterator it = m_upgradeTemplates.begin(); it != m_upgradeTemplates.end(); ++it)
	{
		Int doChange = 0;
		
		const UpgradeTemplate* upgradeTemplate = TheUpgradeCenter->findUpgrade( (*it).templateName );
		UpgradeMaskType upgradeMask = upgradeTemplate->getUpgradeMask();

		// See if upgrade is found in the player completed upgrades or within the object completed upgrades
		if ( newMask.testForAny(upgradeMask) && ( playerMask.testForAny(upgradeMask) || objectMask.testForAny(upgradeMask) ) )
		{
			doChange = 1;
		}
		
		// If it was previously Upgraded, but removed recently
		if ( oldMask.testForAny(upgradeMask) && !newMask.testForAny(upgradeMask) )
		{
			doChange = 2;
		}
		
		//if ( obj->hasUpgrade(upgradeTemplate) || player->hasUpgradeComplete(upgradeTemplate) )
		if (doChange == 1)
		{
			// Remove the Old Template
			removeRiderTemplate(m_theRiderDataRecord[m_theRiderDataRecord.size()-1].templateName, FALSE);

			// Prepare the Data for Register
			RiderData riderData;
			riderData.timeFrame = TheGameLogic->getFrame();
			riderData.statusType = (ObjectStatusType)0;
			riderData.customStatusType = NULL;
			
			// Then we add the new Rider Template.
			if((*it).templateRider >= 0)
			{
				// If the Rider is not a Custom Rider, register its Index
				char charName[2];
				sprintf( charName, "%d", (*it).templateRider );
				riderData.templateName = charName;
			}
			else
			{
				// If it is a Custom Rider, register its Name
				riderData.templateName = (*it).templateName;
			}

			// Give the Rider the Upgraded Template
			riderGiveTemplate(riderData);

			break;
		}
		else if (doChange == 2)
		{
			// If an Upgrade is removed, indicate its Template
			UnsignedInt LongestFrame = 0;
			UnsignedInt UpgradeTimeFrame = 0;
			AsciiString TemplateName = NULL;

			if((*it).templateRider >= 0)
			{
				char charName[2];
				sprintf( charName, "%d", (*it).templateRider );
				TemplateName = charName;
			}
			else
			{
				TemplateName = (*it).templateName;
			}

			// Find the Rider Template within the Records
			for (std::vector<RiderData>::iterator it = m_theRiderDataRecord.begin(); it != m_theRiderDataRecord.end(); ++it)
			{
				if(!LongestFrame || (*it).timeFrame > LongestFrame)
					LongestFrame = (*it).timeFrame;

				if(TemplateName == (*it).templateName)
				{
					UpgradeTimeFrame = (*it).timeFrame;
				}
			}

			// Upgrade isn't granted or is not indicated, do nothing
			if(!UpgradeTimeFrame)
				continue;

			removeRiderDataRecord(TemplateName); // Remove it from the Data Record

			removeRiderTemplate(TemplateName, FALSE); // Remove the Template from the List

			// It is the last Template, remove it and grant the last template.
			if(UpgradeTimeFrame == LongestFrame)
			{
				// The Record should always be present, as the first element is always the Rider
				if(!m_theRiderDataRecord.empty())
				{
					// Get the last Template of the Data Record, but don't Register it
					RiderData riderData;
					riderData.templateName = m_theRiderDataRecord[m_theRiderDataRecord.size()-1].templateName;
					riderData.timeFrame = 0;
					riderData.statusType = (ObjectStatusType)0;
					riderData.customStatusType = NULL;

					// Grant the Rider the last Template
					riderGiveTemplate(riderData);
				}
			}

			break;
		}
	}
}

// Obselete Codes

/*
UpdateSleepTime RiderChangeContain::update()
{
	// HOORAY NO MORE FRAME BASED UPDATES
	Object* obj = getObject();

	//A hackyy way to fix register upgrades and statuses on loading.
	if(m_doRemovalOnLoad && !m_loaded)
	{
		m_firstChange = TRUE;
		m_firstUpgrade = TRUE;
		m_firstStatusRemoval = TRUE;
		m_statusLoadDelay = 5;
	}

	// Skip instances of template update if the the object is currently doing template switch from Riders.
	if(!m_containing)
	{
		// A hackky way of fixing loading and spawning templates
		if(!m_dontCompare && ( m_addDelay > 0) )
		{
			if(m_statusLoadDelay>0 && !m_loaded)
				m_statusDelayCheck = TRUE;
			m_doRemovalOnLoad = TRUE;
			m_loaded = TRUE;
		}

		// Only switch templates if the Rider is not required to. For this instance, a Status Type
		if (data->m_riderNotRequired == TRUE)
		{
			if(!m_dontCompare) {
				changeRiderTemplateOnStatusUpdate();
			} else {
				if(!m_loaded && m_theRiderLastValidBackup >=0 && m_theRiderToChangeBackup >= 0)
					riderGiveTemplateStatus(m_theRiderToChangeBackup, m_theRiderNameBackup);
				m_dontCompare = FALSE;
				// Only do the upgrade changes on the Next Frame after it is just being produced.
				// The value is 2, but we require only a one frame delay after the Status Update is executed.
				// Therefore the countdown starts from below.
				m_addDelay = 2;
			}

			// If both Rider Status Switch and 
			if(data->m_useUpgradeNames == FALSE)
			{
				m_prevStatus = obj->getStatusBits();
				m_prevCustomStatusTypes = obj->getCustomStatus();
			}
		}
		
		// Switch the template everytime a New Upgrade has been triggered.
		if(data->m_useUpgradeNames == TRUE)
		{
			if (!m_dontCompare)
			{ 
				if(!m_registeredUpgradeNames)
					doRegisterUpgradeNames();
				else if (m_addDelay <= 0)
					riderUpdateUpgradeModules();
			}
			else
			{
				// Only do the upgrade changes on the Next Frame after it is just being produced.
				// The value is 2, but we require only a one frame delay after the Status Update is executed.
				// Therefore the countdown starts from below.
				m_addDelay = 2;
				m_dontCompare = FALSE;
			}

			if(data->m_riderNotRequired == TRUE)
			{
				m_prevStatus = obj->getStatusBits();
				m_prevCustomStatusTypes = obj->getCustomStatus();
			}
		}

		if(m_addDelay > 0)
			m_addDelay--;

		if(m_statusLoadDelay > 0)
			m_statusLoadDelay--;

		if(m_loaded && m_resetLaterFrame > 0)
		{
			m_resetLaterFrame--;
			if(m_resetLaterFrame < 1)
			{
				riderReset();
			}
		}
	}
}

// The main Rider Switching function for StatusTypes
void RiderChangeContain::changeRiderTemplateOnStatusUpdate()
{
	if(m_firstStatusRemoval && m_statusLoadDelay > 0 && m_theRiderToChangeBackup >= 0)
	{
		riderGiveTemplateStatus(m_theRiderToChangeBackup, m_theRiderNameBackup);
		m_checkedRemove = FALSE;
	}

	Object* obj = getObject();

	ObjectStatusMaskType NewStatus = obj->getStatusBits();
	ObjectCustomStatusType NewCustomStatusTypes = obj->getCustomStatus();
	AsciiString NewCustomStatus;

	Bool checkChange = FALSE;

	// Check for the differences in the change in statuses.
	// Also get the values changed to register the new condition to the unit.
	if(NewStatus != m_prevStatus)
	{
		ObjectStatusMaskType oldStatus = m_prevStatus;
		oldStatus.clear(NewStatus);
		NewStatus.clear(m_prevStatus);
		if(riderTemplateIsValidChange( NewStatus, TRUE ) )
		{
			checkChange = TRUE;
			m_checkedRemove = FALSE;
		}
		else if(!m_checkedRemove)
		{
			if(riderTemplateIsValidChange( oldStatus, FALSE ) )
			{
				// Statuses screwed up this system, so just reset everything
				riderRemoveAll();

				// Also Save Load Stuff, related to fix Real Time Statuses not loaded on saves.
				if(m_firstStatusRemoval && m_statusLoadDelay <= 0 && m_statusDelayCheck == TRUE && m_theRiderToChangeBackup>= 0)
				{
					m_theRiderToChangeBackup = -1;
				}
				
				// Save Load Stuff, related to granting the Last Template before the Status was granted. Due to how Save Load system works,
				// it scrambles both existing Rider Info.
				if(m_firstStatusRemoval && m_statusLoadDelay <= 0 && m_statusDelayCheck == TRUE && m_theRiderLastValidBackup >= 0)
				{
					m_firstStatusRemoval = FALSE;
					m_theRiderToChange = m_theRiderLastValidBackup;
					m_theRiderName = m_theRiderLastNameBackup;

					riderGiveTemplate(m_theRiderLastValidBackup, m_theRiderLastNameBackup);
				}
				else
				{
					m_theRiderToChange = m_theRiderLastValid;
					m_theRiderName = m_theRiderLastName;

					riderGiveTemplate(m_theRiderLastValid, m_theRiderLastName);
				}

				checkChange = TRUE;
			}
		}
	}
	if(!checkChange && !NewCustomStatusTypes.empty() && NewCustomStatusTypes != m_prevCustomStatusTypes)
	{
		for (ObjectCustomStatusType::const_iterator it = NewCustomStatusTypes.begin(); it != NewCustomStatusTypes.end(); ++it)
		{
			if((*it).second == 1)
			{
				ObjectCustomStatusType::const_iterator it2 = m_prevCustomStatusTypes.find((*it).first);
				if(m_prevCustomStatusTypes.empty() || it2 == m_prevCustomStatusTypes.end() || ( it2 != m_prevCustomStatusTypes.end() && (*it2).second == 0) )
				{
					if(riderTemplateIsValidChange( (*it).first, TRUE ) )
					{
						m_checkedRemove = FALSE;
						break;
					}
				} 
			}
			else if(!m_checkedRemove)
			{
				ObjectCustomStatusType::const_iterator it2 = m_prevCustomStatusTypes.find((*it).first);
				if(it2 != m_prevCustomStatusTypes.end() && (*it2).second == 1)
				{
					if(riderTemplateIsValidChange( (*it).first, FALSE) )
					{
						// Statuses screwed up this system, so just reset everything
						riderRemoveAll();

						// Also Save Load Stuff, related to fix Real Time Statuses not loaded on saves.
						if(m_firstStatusRemoval && m_statusLoadDelay <= 0 && m_statusDelayCheck == TRUE && m_theRiderToChangeBackup>= 0)
						{
							m_theRiderToChangeBackup = -1;
						}
						
						// Save Load Stuff, related to granting the Last Template before the Status was granted. Due to how Save Load system works,
						// it scrambles both existing Rider Info.
						if(m_firstStatusRemoval && m_statusLoadDelay <= 0 && m_statusDelayCheck == TRUE && m_theRiderLastValidBackup >= 0)
						{
							m_firstStatusRemoval = FALSE;
							m_theRiderToChange = m_theRiderLastValidBackup;
							m_theRiderName = m_theRiderLastNameBackup;

							riderGiveTemplate(m_theRiderLastValidBackup, m_theRiderLastNameBackup);
						}
						else
						{
							m_theRiderToChange = m_theRiderLastValid;
							m_theRiderName = m_theRiderLastName;

							riderGiveTemplate(m_theRiderLastValid, m_theRiderLastName);
						}

						break;
					}
				}
			}
		}
	}
}

void RiderChangeContain::riderGiveTemplateStatus(Int RiderIndex, const AsciiString& RiderName)
{
	const RiderChangeContainModuleData* data = getRiderChangeContainModuleData();

	Object* obj = getObject();

	// After we clear the previous condition types, we register the new conditions.
	// We first check whether the Rider is within Custom List. 
	if(!RiderName.isEmpty())
	{
		for (std::vector<RiderInfo>::const_iterator it = data->m_ridersCustom.begin(); it != data->m_ridersCustom.end(); ++it)
		{
			if((*it).m_templateName == RiderName)
			{
				if ( (*it).m_objectCustomStatusType.isEmpty() )
				{
					obj->setStatus( MAKE_OBJECT_STATUS_MASK( (*it).m_objectStatusType ) );
				}
				else
				{
					obj->setCustomStatus( (*it).m_objectCustomStatusType );
				}

				break;
			}
		}
	}
	// If it is not within Custom List, then it must be from the Index List.
	else
	{
		if (data->m_riders[RiderIndex].m_objectCustomStatusType.isEmpty())
		{
			obj->setStatus( MAKE_OBJECT_STATUS_MASK( data->m_riders[RiderIndex].m_objectStatusType ) );
		}
		else
		{
			obj->setCustomStatus( data->m_riders[RiderIndex].m_objectCustomStatusType );
		}
	}
}

// Function to Remove a Rider Template from the Object.
void RiderChangeContain::riderRemoveTemplate(const AsciiString& RiderTemplate)
{
	const RiderChangeContainModuleData* data = getRiderChangeContainModuleData();

	Object* obj = getObject();

	const char* RiderChar = RiderTemplate.str();

	// For clearing a Rider Set. Although this is mainly use for clearing Old Conditions.
	// There cannot be any mistakes since if the Object will be random if there are two conditions co-existing.
	if(isdigit(*RiderChar))
	{
		for (std::vector<RiderInfo>::const_iterator it = data->m_ridersCustom.begin(); it != data->m_ridersCustom.end(); ++it)
		{
			if((*it).m_templateName == RiderName)
			{
				obj->clearModelConditionFlags( MAKE_MODELCONDITION_MASK( (*it).m_modelConditionFlagType ) );

				obj->clearWeaponSetFlag( (*it).m_weaponSetFlag );

				if ((*it).m_objectCustomStatusType.isEmpty())
				{
					obj->clearStatus(MAKE_OBJECT_STATUS_MASK((*it).m_objectStatusType));
				}
				else
				{
					obj->clearCustomStatus((*it).m_objectCustomStatusType);
				}

				break;
			}
		}
	}
	else
	{
		obj->clearModelConditionFlags( MAKE_MODELCONDITION_MASK( data->m_riders[ RiderIndex ].m_modelConditionFlagType ) );

		obj->clearWeaponSetFlag( data->m_riders[ RiderIndex ].m_weaponSetFlag );

		if (data->m_riders[ RiderIndex ].m_objectCustomStatusType.isEmpty())
		{
			obj->clearStatus(MAKE_OBJECT_STATUS_MASK(data->m_riders[ RiderIndex ].m_objectStatusType));
		}
		else
		{
			obj->clearCustomStatus(data->m_riders[ RiderIndex ].m_objectCustomStatusType);
		}
	}

	// Make sure the object does not check for removal again if the Template is removed.
	// m_checkedRemove = TRUE;

}

void RiderChangeContain::riderReset()
{
	// Do this, a bit costly but nonetheless
	riderRemoveAll();
	
	const RiderChangeContainModuleData* data = getRiderChangeContainModuleData();

	Object* obj = getObject();

	// After we clear the previous condition types, we register the new conditions.
	// We first check whether the Rider is within Custom List. 
	if(!m_theRiderName.isEmpty())
	{
		for (std::vector<RiderInfo>::const_iterator it = data->m_ridersCustom.begin(); it != data->m_ridersCustom.end(); ++it)
		{
			if((*it).m_templateName == m_theRiderName)
			{
				if ( (*it).m_objectCustomStatusType.isEmpty() )
				{
					obj->setStatus( MAKE_OBJECT_STATUS_MASK( (*it).m_objectStatusType ) );
				}
				else
				{
					obj->setCustomStatus( (*it).m_objectCustomStatusType );
				}
				
				//This is our rider, so set the correct model condition.
				obj->setModelConditionState((*it).m_modelConditionFlagType);

				//Also set the correct weaponset flag
				obj->setWeaponSetFlag((*it).m_weaponSetFlag);

				//Set the new commandset override
				obj->setCommandSetStringOverride((*it).m_commandSet);
				TheControlBar->markUIDirty();	// Refresh the UI in case we are selected

				//Change the locomotor.
				AIUpdateInterface* ai = obj->getAI();
				if (ai)
				{
					ai->chooseLocomotorSet((*it).m_locomotorSetType);
				}

				break;
			}
		}
	}
	// If it is not within Custom List, then it must be from the Index List.
	else
	{
		for( int i = 0; i < MAX_RIDERS; i++ )
		{
			if(i == m_theRiderToChange)
			{
				if (data->m_riders[ i ].m_objectCustomStatusType.isEmpty())
				{
					obj->setStatus( MAKE_OBJECT_STATUS_MASK( data->m_riders[ i ].m_objectStatusType ) );
				}
				else
				{
					obj->setCustomStatus( data->m_riders[ i ].m_objectCustomStatusType );
				}

				//This is our rider, so set the correct model condition.
				obj->setModelConditionState(data->m_riders[i].m_modelConditionFlagType); 

				//Also set the correct weaponset flag
				obj->setWeaponSetFlag(data->m_riders[ i ].m_weaponSetFlag);

				//Set the new commandset override
				obj->setCommandSetStringOverride(data->m_riders[ i ].m_commandSet);
				TheControlBar->markUIDirty();	// Refresh the UI in case we are selected

				//Change the locomotor.
				AIUpdateInterface* ai = obj->getAI();
				if (ai)
				{
					ai->chooseLocomotorSet(data->m_riders[ i ].m_locomotorSetType);
				}

				break;
			} 
		}
	}

	// Skip an instance of Comparison after the New Rider has been set. Mandatory everytime a new Template is set.
	m_dontCompare = TRUE;

	// Set this in case when the new Status Types fade out and the Object depends on the status type for weapon switching 
	m_checkedRemove = TRUE;
}

// Function not working. Leave it here for future improvements.
// I have been working on this for 3 days and finally its robust, functional with both save and load.
// This is the previous find function that determines the Template based on StatusTypes
Bool RiderChangeContain::riderTemplateIsValidChange(ObjectStatusMaskType newStatus, const AsciiString& newCustomStatus)
{
	const RiderChangeContainModuleData* data = getRiderChangeContainModuleData();

	Bool isValidChange = FALSE;

	// Get if the new Status Type is within the Unit's Rider list.
	for( int i = 0; i < MAX_RIDERS; i++ )
	{
		if (data->m_riders[ i ].m_objectCustomStatusType.isEmpty())
		{
			if( newStatus.test( data->m_riders[ i ].m_objectStatusType ) )
				isValidChange = TRUE;
		}
		else
		{
			if (!newCustomStatus.isEmpty() && data->m_riders[ i ].m_objectCustomStatusType == newCustomStatus)
				isValidChange = TRUE;
		}
		// If it is within the Unit's Rider list, clear the Old Rider Template and register the new Rider Template.
		if(isValidChange)
		{
			m_theRiderLastValid = m_theRiderToChange;
			m_theRiderLastName = m_theRiderName;
			m_theRiderToChange = i;
			m_theRiderName = NULL;
			break;
		}
	}
	// If not found within default Rider List, check Custom Rider List
	if(!isValidChange)
	{
		for (std::vector<RiderInfo>::const_iterator it = data->m_ridersCustom.begin(); it != data->m_ridersCustom.end(); ++it)
		{
			if ( (*it).m_objectCustomStatusType.isEmpty() )
			{
				if( newStatus.test( (*it).m_objectStatusType ) )
					isValidChange = TRUE;
			}
			else
			{
				if ( !newCustomStatus.isEmpty() && (*it).m_objectCustomStatusType == newCustomStatus )
					isValidChange = TRUE;
			}
			// If it is within the Unit's Custom Rider list, clear the Old Rider Template and register the new Rider Template.
			if(isValidChange)
			{
				m_theRiderLastValid = m_theRiderToChange;
				m_theRiderLastName = m_theRiderName;
				m_theRiderName = (*it).m_templateName;
				break;
			}
		}
	}
	return isValidChange;
}

// Find whether the new statuses exist within the Riders List
// This is only used for status checking, you can also modify the function for other values.
Bool RiderChangeContain::riderRemoveTemplateStatusTypes(ObjectStatusType oldStatus, const AsciiString& oldCustomStatus)
{
	const RiderChangeContainModuleData* data = getRiderChangeContainModuleData();

	Object* obj = getObject();

	Bool doRemove = FALSE;

	// Get if the new Status Type is within the Unit's Rider list.
	for( int i = 0; i < MAX_RIDERS; i++ )
	{
		if (data->m_riders[ i ].m_objectCustomStatusType.isEmpty())
		{
			if( data->m_riders[i].m_objectStatusType == oldStatus )
				doRemove = TRUE;
		}
		else
		{
			if (data->m_riders[ i ].m_objectCustomStatusType == oldCustomStatus)
				doRemove = TRUE;
		}
		if(doRemove)
		{
			obj->clearModelConditionFlags( MAKE_MODELCONDITION_MASK( data->m_riders[ i ].m_modelConditionFlagType ) );

			obj->clearWeaponSetFlag( data->m_riders[ i ].m_weaponSetFlag );

			if (data->m_riders[ i ].m_objectCustomStatusType.isEmpty())
			{
				obj->clearStatus(MAKE_OBJECT_STATUS_MASK(data->m_riders[ i ].m_objectStatusType));
			}
			else
			{
				obj->clearCustomStatus(data->m_riders[ i ].m_objectCustomStatusType);
			}

			break;
		}
	}
	// If not found within default Rider List, check Custom Rider List
	if(!doRemove)
	{
		for (std::vector<RiderInfo>::const_iterator it = data->m_ridersCustom.begin(); it != data->m_ridersCustom.end(); ++it)
		{
			if ( (*it).m_objectCustomStatusType.isEmpty() )
			{
				if( (*it).m_objectStatusType == oldStatus )
					doRemove = TRUE;
			}
			else
			{
				if ( (*it).m_objectCustomStatusType == oldCustomStatus )
					doRemove = TRUE;
			}
			if(doRemove)
			{
				obj->clearModelConditionFlags( MAKE_MODELCONDITION_MASK( (*it).m_modelConditionFlagType ) );

				obj->clearWeaponSetFlag( (*it).m_weaponSetFlag );

				if ((*it).m_objectCustomStatusType.isEmpty())
				{
					obj->clearStatus(MAKE_OBJECT_STATUS_MASK((*it).m_objectStatusType));
				}
				else
				{
					obj->clearCustomStatus((*it).m_objectCustomStatusType);
				}

				break;
			}
		}
	}
	// Make sure the object does not recheck on loop if the Template is removed.
	m_checkedRemove = TRUE;
	return doRemove;
}


void RiderChangeContain::riderUpdateUpgradeModules()
{
	// Prevent Switching States
	if(m_dontCompare)
		return;
	
	const RiderChangeContainModuleData* data = getRiderChangeContainModuleData();

	if (data->m_useUpgradeNames == FALSE)
		return;
	
	if (!m_registeredUpgradeNames)
		doRegisterUpgradeNames();

	if (m_upgradeTemplates.empty())
		return;

	Object *obj = getObject();	

	UpgradeMaskType playerMask = obj->getControllingPlayer()->getCompletedUpgradeMask();
	UpgradeMaskType objectMask = obj->getObjectCompletedUpgradeMask();
	UpgradeMaskType maskToCheck = playerMask;
	maskToCheck.set( objectMask );

	// Save Load fixes for Upgrades
	if(m_firstUpgrade == TRUE)
	{
		// We only do this Once!
		m_firstUpgrade = FALSE;

		// First things first, reset everything.
		riderRemoveAll();

		// If the Unit's last Template is granted by an Upgrade, set the current Template Info to that Upgrade, because the game reloads Everything when triggering OnContaining.
		if(m_theRiderLastUpgrade >= 0)
		{
			m_theRiderToChange = m_theRiderLastUpgrade;
			m_theRiderName = m_theRiderLastUpgradeName;
			m_theRiderToChangeBackup = m_theRiderToChange;
			m_theRiderNameBackup = m_theRiderName;
			m_theRiderLastValidBackup = m_theRiderToChange;
			m_theRiderLastNameBackup = m_theRiderName;

			// Also prevent other Template Changes from triggering
			m_firstStatusRemoval = FALSE;
			m_statusLoadDelay = 0;
			m_theRiderToChangeBackup = -1;
			m_resetLaterFrame = 4;
		}
		// Sets the Unit's Template to the Intended Template Info.
		riderGiveTemplate(m_theRiderToChange, m_theRiderName);

		// The Upgrade System checks for the current Upgrade State Existed is the same for the last frame. As we have just set the intended Template, configure it to return.
		m_prevMaskToCheck = maskToCheck;
		return;
	}

	if(maskToCheck == m_prevMaskToCheck)
		return;

	// If any new Upgrades are installed check for that instead.
	UpgradeMaskType newMask = maskToCheck;
	newMask.clear( m_prevMaskToCheck );

	for (std::vector<RiderUpgrade>::const_iterator it = m_upgradeTemplates.begin(); it != m_upgradeTemplates.end(); ++it)
	{
		Bool doChange = FALSE;
		
		const UpgradeTemplate* upgradeTemplate = TheUpgradeCenter->findUpgrade( (*it).templateName );
		UpgradeMaskType upgradeMask = upgradeTemplate->getUpgradeMask();

		// See if upgrade is found in the player completed upgrades or within the object completed upgrades
		if ( newMask.testForAny(upgradeMask) && ( playerMask.testForAny(upgradeMask) || objectMask.testForAny(upgradeMask) ) )
		{
			doChange = TRUE;
		}
		
		//if ( obj->hasUpgrade(upgradeTemplate) || player->hasUpgradeComplete(upgradeTemplate) )
		if (doChange)
		{
			// Save Load Stuff, clear everything once loaded because OnContaining screws up loading existing statuses on Riders
			if(m_firstChange)
				riderRemoveAll();

			// First remove the previous Rider Template.
			riderRemoveTemplate(m_theRiderToChange, m_theRiderName);
			
			// Then we add the new Rider Template.
			if((*it).templateRider >= 0)
			{
				riderGiveTemplate((*it).templateRider, NULL);
				m_theRiderToChange = (*it).templateRider;
				m_theRiderName = NULL;
			}
			else
			{
				riderGiveTemplate(0, (*it).templateName);
				m_theRiderName = (*it).templateName;
			}

			// Register the Template so for Status Change.
			m_theRiderLastValid = m_theRiderToChange;
			m_theRiderLastName = m_theRiderName;

			// Register the Template so that the game recognizes the last Template is granted by an Upgrade
			m_theRiderLastUpgrade = m_theRiderToChange;
			m_theRiderLastUpgradeName = m_theRiderName;
			break;
		}
	}

	m_prevMaskToCheck = maskToCheck;
}*/


// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void RiderChangeContain::unreserveDoorForExit( ExitDoorType exitDoor )
{
	/* nothing */
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void RiderChangeContain::killRidersWhoAreNotFreeToExit()
{
	// extend base class
	TransportContain::killRidersWhoAreNotFreeToExit();
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
Bool RiderChangeContain::isSpecificRiderFreeToExit(Object* specificObject)
{
	// extend base class
	return TransportContain::isSpecificRiderFreeToExit( specificObject );
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
ExitDoorType RiderChangeContain::reserveDoorForExit( const ThingTemplate* objType, Object *specificObject )
{
	// extend base class
	return TransportContain::reserveDoorForExit( objType, specificObject );
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
Bool RiderChangeContain::isExitBusy() const	///< Contain style exiters are getting the ability to space out exits, so ask this before reserveDoor as a kind of no-commitment check.
{
	// extend base class
	return FALSE; //return TransportContain::isExitBusy();
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void RiderChangeContain::onCapture( Player *oldOwner, Player *newOwner )
{
	// extend base class
	TransportContain::onCapture( oldOwner, newOwner );
}

//-------------------------------------------------------------------------------------------------
Bool RiderChangeContain::getContainerPipsToShow(Int& numTotal, Int& numFull)
{
	//Don't show any pips for motorcycles as they always have one rider unless dead!
	numTotal = 0;
	numFull = 0;
	return false;
}

//-------------------------------------------------------------------------------------------------
const Object *RiderChangeContain::friend_getRider() const
{
 	if( m_containListSize > 0 ) // Yes, this does assume that infantry never ride double on the bike
 		return m_containList.front();

	return NULL;
}




// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void RiderChangeContain::crc( Xfer *xfer )
{

	// extend base class
	TransportContain::crc( xfer );

}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void RiderChangeContain::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	TransportContain::xfer( xfer );

	// payload created
	xfer->xferBool( &m_payloadCreated );

	// extra slots in use
	xfer->xferInt( &m_extraSlotsInUse );

	// frame exit not busy
	xfer->xferUnsignedInt( &m_frameExitNotBusy );

	// the Rider Change Record 
	UnsignedShort vecCount = m_theRiderDataRecord.size();
	xfer->xferUnsignedShort( &vecCount );

	AsciiString riderName;
	UnsignedInt riderFrame;
	ObjectStatusType riderStatus;
	AsciiString riderCustomStatus;
	if( xfer->getXferMode() == XFER_SAVE )
	{
		std::vector<RiderData>::iterator it;
		for( it = m_theRiderDataRecord.begin(); it != m_theRiderDataRecord.end(); ++it )
		{
			riderName = (*it).templateName;
			xfer->xferAsciiString( &riderName );

			riderFrame = (*it).timeFrame;
			xfer->xferUnsignedInt( &riderFrame );

			riderStatus = (*it).statusType;
			xfer->xferUser( &riderStatus, sizeof( riderStatus ) );

			riderCustomStatus = (*it).customStatusType;
			xfer->xferAsciiString( &riderCustomStatus );

		}  // end


	}  // end if, save
	else
	{

		// the list must be empty now
		if( m_theRiderDataRecord.empty() == FALSE )
		{

			DEBUG_CRASH(( "RiderChangeContain::xfer - m_theRiderDataRecord should be empty but is not" ));
			//throw SC_INVALID_DATA;

		}  // end if

		// read each item
		for( UnsignedShort i = 0; i < vecCount; ++i )
		{
			xfer->xferAsciiString( &riderName );

			xfer->xferUnsignedInt( &riderFrame );

			xfer->xferUser( &riderStatus, sizeof( riderStatus ) );

			xfer->xferAsciiString( &riderCustomStatus );

			RiderData riderData;
			riderData.templateName = riderName; 
			riderData.timeFrame = riderFrame;
			riderData.statusType = riderStatus;
			riderData.customStatusType = riderCustomStatus;

			// put in vector
			m_theRiderDataRecord.push_back( riderData );

		}  // end for, i

	}  // end else, laod

	/*xfer->xferBool( &m_checkedRemove );

	xfer->xferBool ( &m_doRemovalOnLoad );

	xfer->xferInt( &m_addDelay );

	xfer->xferInt( &m_theRiderToChange );

	xfer->xferAsciiString( &m_theRiderName );

	xfer->xferInt( &m_theRiderLastValid );

	xfer->xferAsciiString( &m_theRiderLastName );

	xfer->xferInt( &m_theRiderLastUpgrade );

	xfer->xferAsciiString( &m_theRiderLastUpgradeName );

	if( xfer->getXferMode() == XFER_SAVE )
	{
		AsciiString templateName = m_theRiderLastName;
		Int templateIndex = m_theRiderLastValid;
		xfer->xferAsciiString(&templateName);
		xfer->xferInt(&templateIndex);

		AsciiString templateName2 = m_theRiderName;
		Int templateIndex2 = m_theRiderToChange;
		xfer->xferAsciiString(&templateName2);
		xfer->xferInt(&templateIndex2);
	}
	else if (xfer->getXferMode() == XFER_LOAD)
	{
		AsciiString templateName;
		xfer->xferAsciiString(&templateName);
		Int templateIndex;
		xfer->xferInt(&templateIndex);
		m_theRiderLastNameBackup = templateName;
		m_theRiderLastValidBackup = templateIndex;

		AsciiString templateName2;
		xfer->xferAsciiString(&templateName2);
		Int templateIndex2;
		xfer->xferInt(&templateIndex2);
		m_theRiderNameBackup = templateName2;
		m_theRiderToChangeBackup = templateIndex2;
	}

	// Xfer Source Code from GameLogic.cpp
	// Xfer-Hash_Map
	// Might need checking.
	if (version >= 7) 
	{
		if( xfer->getXferMode() == XFER_SAVE )
		{
			for (ObjectCustomStatusType::const_iterator it = m_prevCustomStatusTypes.begin(); it != m_prevCustomStatusTypes.end(); ++it )
			{
				AsciiString statusName = it->first;
				Int flag = it->second;
				xfer->xferAsciiString(&statusName);
				xfer->xferInt(&flag);
			}
			AsciiString empty;
			xfer->xferAsciiString(&empty);
		}
		else if (xfer->getXferMode() == XFER_LOAD)
		{
			if (m_prevCustomStatusTypes.empty() == false)
			{
				DEBUG_CRASH(( "GameLogic::xfer - m_prevCustomStatusTypes should be empty, but is not\n"));
				//throw SC_INVALID_DATA;
			}
			
			for (;;) 
			{
				AsciiString statusName;
				xfer->xferAsciiString(&statusName);
				if (statusName.isEmpty())
					break;
				Int flag;
				xfer->xferInt(&flag);
				m_prevCustomStatusTypes[statusName] = flag;
			}
		}

		if( xfer->getXferMode() == XFER_SAVE )
		{
			for (std::vector<RiderUpgrade>::const_iterator it = m_upgradeTemplates.begin(); it != m_upgradeTemplates.end(); ++it )
			{
				AsciiString riderName = it->templateName;
				Int rider = it->templateRider;
				xfer->xferAsciiString(&riderName);
				xfer->xferInt(&rider);
			}
			AsciiString empty;
			xfer->xferAsciiString(&empty);
		}
		else if (xfer->getXferMode() == XFER_LOAD)
		{
			if (m_upgradeTemplates.empty() == false)
			{
				DEBUG_CRASH(( "GameLogic::xfer - m_upgradeTemplates should be empty, but is not\n"));
				//throw SC_INVALID_DATA;
			}
			
			for (;;) 
			{
				AsciiString riderName;
				xfer->xferAsciiString(&riderName);
				if (riderName.isEmpty())
					break;
				Int rider;
				xfer->xferInt(&rider);
				
				RiderUpgrade rd;
				rd.templateName = riderName;
				rd.templateRider = rider;
				m_upgradeTemplates.push_back(rd);
			}
		}
	}

	// status
	if( version >= 8 )
	{
		m_prevStatus.xfer( xfer );
	}
	else
	{
		//We are loading an old version, so we must convert it from a 32-bit int to a bitflag
		UnsignedInt oldStatus;
		xfer->xferUnsignedInt( &oldStatus );

		//Clear our status
		m_prevStatus.clear();

		for( int i = 0; i < 32; i++ )
		{
			UnsignedInt bit = 1<<i;
			if( oldStatus & bit )
			{
				ObjectStatusTypes status = (ObjectStatusTypes)(i+1);
				m_prevStatus.set( MAKE_OBJECT_STATUS_MASK( status ) );
			}
		}
	}*/

}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void RiderChangeContain::loadPostProcess( void )
{

	// extend base class
	TransportContain::loadPostProcess();

}  // end loadPostProcess

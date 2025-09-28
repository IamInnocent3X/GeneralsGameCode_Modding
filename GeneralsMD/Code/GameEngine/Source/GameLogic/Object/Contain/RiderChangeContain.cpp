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
#define DEFINE_SCUTTLE_NAMES

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
	m_dontDestroyPassengersOnKill = FALSE;
	m_dontEvacuateOnEnter = FALSE;
	m_canContainNonRiders = FALSE;
	m_moreThanOneRiders = FALSE;
	m_scuttleType = SCUTTLE_ON_EXIT;
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
	{ "DontDestroyPassengersOnKill",	INI::parseBool,				NULL, offsetof( RiderChangeContainModuleData, m_dontDestroyPassengersOnKill ) },
	{ "ScuttleRequirements",		INI::parseIndexList,			TheScuttleNames, offsetof( RiderChangeContainModuleData, m_scuttleType ) },
	{ "DontEvacuateOnEnter",		INI::parseBool,					NULL, offsetof( RiderChangeContainModuleData, m_dontEvacuateOnEnter ) },
	{ "CanContainNonRiders",		INI::parseBool,					NULL, offsetof( RiderChangeContainModuleData, m_canContainNonRiders ) },
	{ "MoreThanOneRiders",		INI::parseBool,					NULL, offsetof( RiderChangeContainModuleData, m_moreThanOneRiders ) },
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
	m_loaded = FALSE;
	m_theRiderDataRecord.clear(); // A better non-sleepy update version of Status and Upgrade Checking
	m_registeredUpgradeNames = FALSE;
	m_riderDataStatusRegister = (ObjectStatusType)0;
	m_riderDataCustomStatusRegister = NULL;
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
		Bool isARider = checkHasRiderTemplate(rider);
		if( isARider || m_scuttledOnFrame != 0 )
			return isARider;
		
		// If we can contain Non Riders, do a different calculation
		if( getRiderChangeContainModuleData()->m_canContainNonRiders )
		{
			Int transportSlotCount = rider->getTransportSlotCount();

			// if 0, this object isn't transportable.
			if (transportSlotCount == 0)
				return false;

			Int containMax = getContainMax();
			Int containCount = getContainCount();

				return (m_extraSlotsInUse + containCount + transportSlotCount <= containMax);

		}

	}

	return FALSE;
}

// Separated the function above unto 
Bool RiderChangeContain::checkHasRiderTemplate(const Object* rider) const
{
	if( m_scuttledOnFrame != 0 )
	{
		//Scuttled... too late!
		return FALSE;
	}

	if(rider == NULL)
		return FALSE;

	AsciiString riderName = rider->getTemplate()->getName();

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
		if ( thing && thing->isEquivalentTo( rider->getTemplate() ) )
		{
			//We found a valid rider, so return success.
			return TRUE;
		}
	}
	return FALSE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void RiderChangeContain::onContaining( Object *rider, Bool wasSelected )
{
	const RiderChangeContainModuleData *data = getRiderChangeContainModuleData();
	Object *obj = getObject();
	m_containing = TRUE;

	Bool hasEvacuated = FALSE;
	Int containMax = getContainMax();
	Int containCount = getContainCount();
	Int transportSlotCount = rider->getTransportSlotCount();

	//Remove riders if conditions are met
	if( ( m_payloadCreated && !data->m_dontEvacuateOnEnter ) ||
		transportSlotCount >= containMax )
	{
		obj->getAI()->aiEvacuateInstantly( TRUE, CMD_FROM_AI );
		hasEvacuated = TRUE;
	}

	if(!checkHasRiderTemplate(rider))
	{
		TransportContain::onContaining( rider, wasSelected );

		m_containing = FALSE;
		return;
	}

	//DEBUG_LOG(("Object to Contain: %s. Object Transport Slot Count: %d. Current Slots Available: %d. ContainMax: %d. ExtraSlotsInUse: %d", rider->getTemplate()->getName().str(), rider->getTransportSlotCount(), containMax - m_extraSlotsInUse - containCount, containMax, m_extraSlotsInUse));
	//if(hasEvacuated)
	//{
	//	DEBUG_LOG(("Evacuated to leave Contain for: %s.  Object Transport Slot Count: %d. ContainMax: %d", rider->getTemplate()->getName().str(), rider->getTransportSlotCount(), containMax));
	//}
	if( !hasEvacuated && transportSlotCount > 0 && m_extraSlotsInUse + containCount + transportSlotCount > containMax )
	{
		// I get contained, but remove the last rider before me
		ContainedItemsList::iterator it = m_containList.end();
		while(getContainCount() > 1)
		{
			// containCount variable doesn't update as getContainCount()
			// + 1 because the current Rider is already considered 'Contained' so it would consider exiting an extra slot to make way to contain
			if(m_extraSlotsInUse + getContainCount() + transportSlotCount <= containMax + 1)
				break;

			it = m_containList.end();
			it--;
			it--;

			//const char* name = "Nothing";
			//Int slots = 0;
			//if((*it) && (*it)->getTemplate())
			//{
			//	name = (*it)->getTemplate()->getName().str();
			//	slots = (*it)->getTransportSlotCount();
			//}

			removeFromContainViaIterator( it, TRUE );

			//DEBUG_LOG(("Removed Object: %s. Object Transport Slot Count: %d. Current Slots Available: %d. ContainMax: %d. ExtraSlotsInUse: %d", name, slots, containMax - m_extraSlotsInUse - getContainCount(), containMax, m_extraSlotsInUse));
		}
	}

	// Add the extra slot count after the calculation
	m_extraSlotsInUse += transportSlotCount - 1;
	//DEBUG_LOG(("Registered Extra Slots: %d. Current Slots Available: %d. ExtraSlotsInUse: %d", transportSlotCount - 1, containMax - m_extraSlotsInUse - getContainCount(), m_extraSlotsInUse));

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

	// For registering RiderData, clear all the previous statuses first
	RiderData riderData;
	m_riderDataStatusRegister = (ObjectStatusType)0;
	m_riderDataCustomStatusRegister = NULL;
	
	//Find the rider in the list and set the appropriate model condition
	Bool found;
	for( int i = 0; i < MAX_RIDERS; i++ )
	{
		found = riderChangeContainingCheck(rider, data->m_riders[ i ]);
		if (found == TRUE)
		{
			char charName[2];
			sprintf( charName, "%d", i );
			
			riderData.templateName = charName;
			riderData.timeFrame = 1;
			riderData.statusType = m_riderDataStatusRegister;
			riderData.customStatusType = m_riderDataCustomStatusRegister;

			break;
		}
	}
	if (!found && !data->m_ridersCustom.empty())
	{
		for (std::vector<RiderInfo>::const_iterator it = data->m_ridersCustom.begin(); it != data->m_ridersCustom.end(); ++it)
		{
			found = riderChangeContainingCheck( rider, (*it) );
			if (found == TRUE)
			{
				riderData.templateName = (*it).m_templateName;
				riderData.timeFrame = 1;
				riderData.statusType = m_riderDataStatusRegister;
				riderData.customStatusType = m_riderDataCustomStatusRegister;

				break;
			}
		}
	}

	// Register the new Data Record
	if(found)
		registerNewRiderDataOnContain(riderData);

	//for(int i = 0; i < m_theRiderDataRecord.size(); i++)
	//	DEBUG_LOG(("Template Slot: %d, Template Name: %s, Time Frame: %d", i, m_theRiderDataRecord[i].templateName.str(), m_theRiderDataRecord[i].timeFrame));

	//Extend base class
	TransportContain::onContaining( rider, wasSelected );

	m_containing = FALSE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void RiderChangeContain::orderAllPassengersToExit( CommandSourceType commandSource, Bool instantly )
{
	Bool reverse = !m_containing || getRiderChangeContainModuleData()->m_dontEvacuateOnEnter ? TRUE : FALSE;
	Bool first = TRUE;

	ContainedItemsList::const_iterator it;
	ContainedItemsList::const_iterator it_last;

	if(reverse)
	{
		it = getContainedItemsList()->end();
		it_last = getContainedItemsList()->begin();
	}
	else
	{
		it = getContainedItemsList()->begin();
		it_last = getContainedItemsList()->end();
	}

	for(it; it != it_last;)
	{
		if(first && reverse)
		{
			first = FALSE;
			continue;
		}

		// save the rider...
		Object* rider = *it;

		// incr/derc the iterator BEFORE calling the func (if the func removes the rider,
		// the iterator becomes invalid)
		if(reverse)
			--it;
		else
			++it;

		// call it
		if( rider && rider->getAI() && rider->getContainedBy() )
		{
			if( instantly )
			{
				rider->getAI()->aiExitInstantly( getObject(), commandSource );
			}
			else
			{
				rider->getAI()->aiExit( getObject(), commandSource );
			}
		}
	}

	if(!reverse)
		return;

	Object* first_rider = *getContainedItemsList()->begin();
	if( first_rider->getAI() )
	{
		if( instantly )
		{
			first_rider->getAI()->aiExitInstantly( getObject(), commandSource );
		}
		else
		{
			first_rider->getAI()->aiExit( getObject(), commandSource );
		}
	}
}

//-------------------------------------------------------------------------------------------------
void RiderChangeContain::registerNewRiderDataOnContain(RiderData riderData)
{
	Bool cond_1 = m_loaded || m_theRiderDataRecord.empty() ? TRUE : FALSE;
	//Bool cond_2 = data->m_moreThanOneRiders || getContainCount() == 1 ? TRUE : FALSE;

	if(!cond_1)
		return;

	// If the record is empty, register an Empty Template for reference to Status Check and Upgrade Check, or even when Emptying Rider Templates
	if(m_theRiderDataRecord.empty())
	{
		RiderData firstTemplate;
		
		char charName[2];
		sprintf( charName, "%d", 0 );
		firstTemplate.templateName = charName;
		firstTemplate.timeFrame = 1;
		firstTemplate.statusType = (ObjectStatusType)0;
		firstTemplate.customStatusType = NULL;

		m_theRiderDataRecord.push_back(firstTemplate);
	}
	else
	{
		// If we register more than one rider, we need to clear all the previous Status and Upgrade Templates and assign the new Template 
		clearAllTimeFramedDataRecords();
	}

	// Register the new Data Record
	m_theRiderDataRecord.push_back(riderData);
}

Bool RiderChangeContain::riderChangeContainingCheck(Object* rider, const RiderInfo& riderInfo)
{
	Object* obj = getObject();
	const ThingTemplate* thing = TheThingFactory->findTemplate( riderInfo.m_templateName );
	if ( thing && thing->isEquivalentTo(rider->getTemplate() ) )
	{
		const RiderChangeContainModuleData *data = getRiderChangeContainModuleData();
		
		// Skip an instance of Comparison after the New Rider has been set. Mandatory everytime a new Template is set.
		m_dontCompare = TRUE;

		// Don't register the Rider if we do not use More Than One Riders
		Bool cond_1 = m_loaded || m_theRiderDataRecord.empty() ? TRUE : FALSE;
		Bool cond_2 = data->m_moreThanOneRiders || getContainCount() == 1 || ( !m_theRiderDataRecord.empty() && m_theRiderDataRecord.size()-1 == 0 && m_theRiderDataRecord[m_theRiderDataRecord.size()-1].templateName == "0" ) ? TRUE : FALSE;

		if(!cond_1 || !cond_2)
			return TRUE;

		// Clear the last template
		if(!m_theRiderDataRecord.empty())
		{
			removeRiderTemplate(m_theRiderDataRecord[m_theRiderDataRecord.size()-1].templateName, FALSE);
		}

		//This is our rider, so set the correct model condition.
		obj->setModelConditionState( riderInfo.m_modelConditionFlagType );

		//Also set the correct weaponset flag
		obj->setWeaponSetFlag( riderInfo.m_weaponSetFlag );

		//Also set the object status
		if (riderInfo.m_objectCustomStatusType.isEmpty())
		{
			obj->setStatus( MAKE_OBJECT_STATUS_MASK( riderInfo.m_objectStatusType ) );
			m_riderDataStatusRegister = riderInfo.m_objectStatusType;
		}
		else
		{
			obj->setCustomStatus( riderInfo.m_objectCustomStatusType );
			m_riderDataCustomStatusRegister = riderInfo.m_objectCustomStatusType;
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
	
	Int transportSlotCount = rider->getTransportSlotCount();
	Bool checkRecord = FALSE;
	Bool hasScuttle = FALSE;

	m_extraSlotsInUse -= transportSlotCount - 1;

	//Note if the bike dies, the rider dies too.
	if( !data->m_dontDestroyPassengersOnKill && bike->isEffectivelyDead() )
	{
		TheGameLogic->destroyObject( rider );
		return;
	}

	if( m_payloadCreated )
	{
		//Extend base class
		TransportContain::onRemoving( rider );
	}

	if(data->m_scuttleType == SCUTTLE_ON_EXIT || (getContainCount() == 0 && data->m_scuttleType == SCUTTLE_ON_NO_PASSENGERS) )
	{
		hasScuttle = TRUE;
	}


	// If we exit a Passenger from an Object that Contains more than one Passengers while we're Entering and we did not eject the First Passenger
	// While that passenger has the same Template as the Rider, we will remove the Rider Template and leave it with the First Template.
	// This happens specifically while MoreThanOneRiders is set to 'No'
	// Example, a Bike has 3 Slot Capacity, Containing 3 Rebels, and a different Rider enters the Bike, exiting a Rebel and Removed its Template
	if(m_containing && !data->m_moreThanOneRiders && getContainCount() > 1 && !hasScuttle)
	{
		checkRecord = TRUE;
	}

	//Find the rider in the list and clear various data.
	Bool found = FALSE;
	AsciiString removeTemplate = NULL;

	for( int i = 0; i < MAX_RIDERS; i++ )
	{
		found = riderChangeRemoveCheck( rider, data->m_riders[ i ], checkRecord );
		if (found == TRUE)
		{
			char charName[2];
			sprintf( charName, "%d", i );
			removeTemplate = charName;
			break;
		}
	}
	if (!found && !data->m_ridersCustom.empty())
	{
		for (std::vector<RiderInfo>::const_iterator it = data->m_ridersCustom.begin(); it != data->m_ridersCustom.end(); ++it)
		{
			found = riderChangeRemoveCheck(rider, (*it), checkRecord);
			if (found == TRUE)
			{
				removeTemplate = (*it).m_templateName;
				break;
			}
		}
	}

	if(!removeTemplate.isEmpty() && !hasScuttle)
	{
		Bool switchTemplate = FALSE;
		Int CheckIndex = -1;

		// If we are exiting or evacuating when not trying to Contain a Rider
		if(!m_containing && getContainCount() > 0)
		{
			Bool isCurRider = FALSE;

			if(data->m_moreThanOneRiders)
			{
				if(m_theRiderDataRecord[m_theRiderDataRecord.size()-1].timeFrame == 1)
				{
					CheckIndex = m_theRiderDataRecord.size()-1;
					isCurRider = TRUE;
				}
				else
				{
					isCurRider = FALSE;
				}
			}
			else
			{
				Bool first = TRUE;
				for (int i = 0; i < m_theRiderDataRecord.size()-1; i++)
				{
					if(first && m_theRiderDataRecord[i].templateName == "0")
						continue;

					isCurRider = TRUE;
					if(m_theRiderDataRecord[i].timeFrame == 1)
					{
						if(CheckIndex == -1)
						{
							CheckIndex = i;
						}
					}
					else
					{
						isCurRider = FALSE;
						break;
					}

					first = false;
				}

			}

			if(!isCurRider)
				CheckIndex = -1;
			else
				switchTemplate = compareRiderStatus(removeTemplate, CheckIndex);
		}

		/*for(int i = 0; i < m_theRiderDataRecord.size(); i++)
		{
			DEBUG_LOG(("Template Slot: %d, Template Name: %s, Time Frame: %d", i, m_theRiderDataRecord[i].templateName.str(), m_theRiderDataRecord[i].timeFrame));
		}

		if(CheckIndex == -1)
		{
			DEBUG_LOG(("Removing Rider, No Template Found"));
		}*/

		
		// Remove the Data from the Data Record
		removeRiderDataRecord(removeTemplate);

		// If we have multiple Riders and the Bike has its Template and it is being removed
		if(switchTemplate && CheckIndex >= 0)
		{
			Int GrantIndex = 0;

			if(data->m_moreThanOneRiders)
				GrantIndex = m_theRiderDataRecord.size()-1;
			else
				GrantIndex = CheckIndex;

			// Grant the last Template before the Removed Template of the Data Record, but don't Register it
			RiderData riderData;
			riderData.templateName = m_theRiderDataRecord[GrantIndex].templateName;
			riderData.timeFrame = 0;
			riderData.statusType = (ObjectStatusType)0;
			riderData.customStatusType = NULL;

			// Grant the Rider the last Template before the Removed Rider
			riderGiveTemplate(riderData);

			//DEBUG_LOG(("Template to Grant: %d, Template Name: %s", GrantIndex, m_theRiderDataRecord[GrantIndex].templateName.str()));
		}
	}

	//If we're not replacing the rider, then if the cycle is selected, transfer selection
	//to the rider getting off (because the bike is gonna blow).
	if( !m_containing && hasScuttle )
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
Bool RiderChangeContain::riderChangeRemoveCheck(Object* rider, const RiderInfo& riderInfo, Bool checkRecordOnly)
{
	Object* bike = getObject();
	const ThingTemplate* thing = TheThingFactory->findTemplate(riderInfo.m_templateName);
	if( thing && thing->isEquivalentTo( rider->getTemplate() ) )
	{
		if(checkRecordOnly == TRUE)
			return TRUE;
		
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
	// Sanity
	if(m_theRiderDataRecord.empty())
		return;

	std::vector<RiderData>::iterator it;
	for (it = m_theRiderDataRecord.begin(); it != m_theRiderDataRecord.end();)
	{
		if(it == m_theRiderDataRecord.begin() && (*it).templateName == "0")
		{
			++it;
			if(it == m_theRiderDataRecord.end())
				break;
		}
		if ((*it).templateName == rider)
		{
			it = m_theRiderDataRecord.erase(it);
			break;
		}
		++it;
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
Bool RiderChangeContain::compareRiderStatus(const AsciiString& rider, Int checkIndex) const
{
	// Checking the Status and returns TRUE if the check Index matches the current rider
	const RiderChangeContainModuleData *data = getRiderChangeContainModuleData();
	const char* RiderChar = rider.str();

	// Check whether rider to check is an Index
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
			if(m_theRiderDataRecord[checkIndex].statusType == data->m_riders[RiderIndex].m_objectStatusType)
				return TRUE;
		}
		else
		{
			if(m_theRiderDataRecord[checkIndex].customStatusType == data->m_riders[RiderIndex].m_objectCustomStatusType)
				return TRUE;
		}
	}
	// If it is not within Index List, then it must be from the Custom List.
	else
	{
		for (std::vector<RiderInfo>::const_iterator it = data->m_ridersCustom.begin(); it != data->m_ridersCustom.end(); ++it)
		{
			if((*it).m_templateName == rider)
			{
				if ( (*it).m_objectCustomStatusType.isEmpty() )
				{
					if(m_theRiderDataRecord[checkIndex].statusType == (*it).m_objectStatusType)
						return TRUE;
				}
				else
				{
					if(m_theRiderDataRecord[checkIndex].customStatusType == (*it).m_objectCustomStatusType)
						return TRUE;
				}
			}
		}
	}

	return FALSE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void RiderChangeContain::clearAllTimeFramedDataRecords()
{
	if(m_theRiderDataRecord.empty())
		return;

	Bool hasTimeFramedTemplates = TRUE;
	while (hasTimeFramedTemplates)
	{
		hasTimeFramedTemplates = FALSE;
		
		std::vector<RiderData>::iterator it;
		for (it = m_theRiderDataRecord.begin(); it != m_theRiderDataRecord.end();)
		{
			if ((*it).timeFrame != 1)
			{
				hasTimeFramedTemplates = TRUE;
				it = m_theRiderDataRecord.erase(it);
				break;
			}
			++it;
		}
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
	else if(m_loaded && m_theRiderDataRecord.empty())
	{
		// If there isn't a Rider template in the Record, we need to make one so it is compatible with the Status and Upgrade Checks.
		RiderData riderData;
		char charName[2];
		sprintf( charName, "%d", 0 );
		riderData.templateName = charName;
		riderData.timeFrame = 1; // For indicating it as a Containing Template, in case of usage
		riderData.statusType = (ObjectStatusType)0;
		riderData.customStatusType = NULL;

		m_theRiderDataRecord.push_back(riderData);

		//for(int i = 0; i < m_theRiderDataRecord.size(); i++)
		//	DEBUG_LOG(("Template Slot: %d, Template Name: %s, Time Frame: %d", i, m_theRiderDataRecord[i].templateName.str(), m_theRiderDataRecord[i].timeFrame));
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

		if((*it).statusType != 0 && oldStatus.test((*it).statusType) && (*it).timeFrame != 1)
		{
			TemplateName = (*it).templateName;
			StatusTimeFrame = (*it).timeFrame;
		}
	}

	// Do nothing if there's no matching Template, or coincidentially the rider has the same template as the one granted by the Status
	if(TemplateName.isEmpty() || LongestFrame == 1)
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

		if((*it).customStatusType == oldCustomStatus && (*it).timeFrame != 1)
		{
			TemplateName = (*it).templateName;
			StatusTimeFrame = (*it).timeFrame;
		}

		//DEBUG_LOG(("Template Name: %s, Time Frame: %d", (*it).templateName.str(), (*it).timeFrame));
	}

	//DEBUG_LOG(("StatusTimeFrame: %d, Time Frame: %d", StatusTimeFrame, LongestFrame));

	// Do nothing if there's no matching Template, or coincidentially the rider has the same template as the one granted by the Status
	if(TemplateName.isEmpty() || LongestFrame == 1)
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
/*void RiderChangeContain::riderRemoveAll()
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
}*/

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

	//for(int i = 0; i < m_theRiderDataRecord.size(); i++)
	//	DEBUG_LOG(("Template Slot: %d, Template Name: %s, Time Frame: %d", i, m_theRiderDataRecord[i].templateName.str(), m_theRiderDataRecord[i].timeFrame));
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

				if(TemplateName == (*it).templateName && (*it).timeFrame != 1)
				{
					UpgradeTimeFrame = (*it).timeFrame;
				}
			}

			// Upgrade isn't granted or is not indicated, or coincidentially the rider has the same name as the Upgrade do nothing
			if(!UpgradeTimeFrame || LongestFrame == 1)
				continue;

			removeRiderDataRecord(TemplateName); // Remove it from the Data Record

			removeRiderTemplate(TemplateName, FALSE); // Remove the Template from the List

			// It is the last Template, remove it and grant the last template.
			if(UpgradeTimeFrame == LongestFrame)
			{
				// The Record should always be present, as the first element is always the Rider, or '0' if the Rider Data Record is empty.
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

	//for(int i = 0; i < m_theRiderDataRecord.size(); i++)
	//	DEBUG_LOG(("Template Slot: %d, Template Name: %s, Time Frame: %d", i, m_theRiderDataRecord[i].templateName.str(), m_theRiderDataRecord[i].timeFrame));
}

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

}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void RiderChangeContain::loadPostProcess( void )
{

	// extend base class
	TransportContain::loadPostProcess();

}  // end loadPostProcess

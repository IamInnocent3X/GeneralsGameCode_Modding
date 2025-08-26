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

// FILE: CaveContain.cpp ////////////////////////////////////////////////////////////////////////////
// Author: Graham Smallwood, July 2002
// Desc:   A version of OpenContain that overrides where the passengers are stored: one of CaveSystem's
//					entries. Changing entry is a script or ini command.  All queries about capacity and
//					contents are also redirected.  They change sides like Garrison too.
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file int the GameEngine
#include "Common/GameState.h"
#include "Common/Player.h"
#include "Common/PlayerList.h"
#include "Common/Team.h"
#include "Common/ThingTemplate.h"
#include "Common/TunnelTracker.h"
#include "Common/Xfer.h"
#include "GameClient/Drawable.h"
#include "GameLogic/Module/AIUpdate.h"
#include "GameLogic/Module/CaveContain.h"
#include "GameLogic/CaveSystem.h"
#include "GameLogic/Object.h"
#include "GameLogic/PartitionManager.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS ///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
CaveContain::CaveContain( Thing *thing, const ModuleData* moduleData ) : OpenContain( thing, moduleData )
{
	m_needToRunOnBuildComplete = true;
	m_loaded = false;
	m_caveIndex = 0;
	m_originalTeam = NULL;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
CaveContain::~CaveContain()
{
}

void CaveContain::addToContainList( Object *obj )
{
	TunnelTracker *myTracker = TheCaveSystem->getTunnelTracker( getCaveContainModuleData()->m_caveUsesTeams, m_caveIndex, getObject()->getTeam() );
	if(myTracker)
		myTracker->addToContainList( obj );
}

//-------------------------------------------------------------------------------------------------
/** Remove 'obj' from the m_containList of objects in this module.
	* This will trigger an onRemoving event for the object that this module
	* is a part of and an onRemovedFrom event for the object being removed */
//-------------------------------------------------------------------------------------------------
void CaveContain::removeFromContain( Object *obj, Bool exposeStealthUnits )
{

	// sanity
	if( obj == NULL )
		return;

	//
	// we can only remove this object from the contains list of this module if
	// it is actually contained by this module
	//
	TunnelTracker *myTracker = TheCaveSystem->getTunnelTracker( getCaveContainModuleData()->m_caveUsesTeams, m_caveIndex, getObject()->getTeam() );

	if( !myTracker || ! myTracker->isInContainer( obj ) )
	{
		return;
	}

	// This must come before the onRemov*, because CaveContain's version has a edge-0 triggered event.
	// If that were to go first, the number would still be 1 at that time.  Noone else cares about
	// order.
	myTracker->removeFromContain( obj, exposeStealthUnits );

	// trigger an onRemoving event for 'm_object' no longer containing 'itemToRemove->m_object'
	if (getObject()->getContain())
		getObject()->getContain()->onRemoving( obj );

	// trigger an onRemovedFrom event for 'remove'
	obj->onRemovedFrom( getObject() );

}

//-------------------------------------------------------------------------------------------------
/** Remove all contained objects from the contained list */
//-------------------------------------------------------------------------------------------------
void CaveContain::removeAllContained( Bool exposeStealthUnits )
{
	TunnelTracker *myTracker = TheCaveSystem->getTunnelTracker( getCaveContainModuleData()->m_caveUsesTeams, m_caveIndex, getObject()->getTeam() );
	if(!myTracker)
		return;
	const ContainedItemsList *fullList = myTracker->getContainedItemsList();

	Object *obj;
	ContainedItemsList::const_iterator it;
	it = (*fullList).begin();
	while( it != (*fullList).end() )
	{
		obj = *it;
		it++;
		removeFromContain( obj, exposeStealthUnits );
	}
}

//--------------------------------------------------------------------------------------------------------
/** Force all contained objects in the contained list to exit, and kick them in the pants on the way out*/
//--------------------------------------------------------------------------------------------------------
void CaveContain::harmAndForceExitAllContained( DamageInfo *info )
{
	TunnelTracker *myTracker = TheCaveSystem->getTunnelTracker( getCaveContainModuleData()->m_caveUsesTeams, m_caveIndex, getObject()->getTeam() );
	if(!myTracker)
		return;

	const ContainedItemsList *fullList = myTracker->getContainedItemsList();

	Object *obj;
	ContainedItemsList::const_iterator it;

	//Kris: Patch 1.01 -- November 6, 2003
	//No longer advances the iterator and saves it. The iterator is fetched from the beginning after
	//each loop. This is to prevent a crash where dropping a bunker buster on a tunnel network containing
	//multiple units (if they have the suicide bomb upgrade - demo general). In this case, multiple bunker
	//busters would hit the tunnel network in close succession. Missile #1 would iterate the list, killing
	//infantry #1. Infantry #1 would explode and destroy Missile #2. Missile #2 would start iterating the
	//same list, killing the remaining units. When Missile #1 picked up and continued processing the list
	//it would crash because it's iterator was deleted from under it.
	it = (*fullList).begin();
	while( it != (*fullList).end() )
	{
		obj = *it;
		removeFromContain( obj, true );
    obj->attemptDamage( info );
		it = (*fullList).begin();
	}  // end while

}  // end removeAllContained


//-------------------------------------------------------------------------------------------------
/** Remove all contained objects from the contained list */
//-------------------------------------------------------------------------------------------------
void CaveContain::killAllContained( void )
{
	// TheSuperHackers @bugfix xezon 04/06/2025 Empty the TunnelSystem's Contained Items List
	// straight away to prevent a potential child call to catastrophically modify it as well.
	// This scenario can happen if the killed occupant(s) apply deadly damage on death
	// to the host container, which then attempts to remove all remaining occupants
	// on the death of the host container. This is reproducible by shooting with
	// Neutron Shells on a GLA Tunnel containing GLA Terrorists.

	TunnelTracker *myTracker = TheCaveSystem->getTunnelTracker( getCaveContainModuleData()->m_caveUsesTeams, m_caveIndex, getObject()->getTeam() );
	if(!myTracker)
		return;

	const ContainedItemsList *fullList = myTracker->getContainedItemsList();

	Object *obj;
	ContainedItemsList::const_iterator it;
	it = (*fullList).begin();
	while( it != (*fullList).end() )
	{
		obj = *it;
		it++;
		removeFromContain( obj, true );

		obj->kill();
	}
}

//-------------------------------------------------------------------------------------------------
/** Iterate the contained list and call the callback on each of the objects */
//-------------------------------------------------------------------------------------------------
void CaveContain::iterateContained( ContainIterateFunc func, void *userData, Bool reverse )
{
	TunnelTracker *myTracker = TheCaveSystem->getTunnelTracker( getCaveContainModuleData()->m_caveUsesTeams, m_caveIndex, getObject()->getTeam() );
	if(myTracker)
		myTracker->iterateContained( func, userData, reverse );
}

// ------------------------------------------------------------------------------------------------
/** Per frame update */
// ------------------------------------------------------------------------------------------------
UpdateSleepTime CaveContain::update( void )
{
	if (!m_originalTeam && getObject()->getControllingPlayer() && getObject()->getControllingPlayer()->getRelationship(getObject()->getTeam()) != NEUTRAL )
		m_originalTeam = getObject()->getTeam();
	
	// Loading fixes. Cave System does not register properly while the game is loaded.
	if(!m_needToRunOnBuildComplete && !m_loaded)
	{
		TunnelTracker *myTracker = TheCaveSystem->getTunnelTracker( getCaveContainModuleData()->m_caveUsesTeams, m_caveIndex, getObject()->getTeam() );
		if( myTracker == NULL )
		{
			m_loaded = true;
			
			registerNewCave();

			TunnelTracker *myTracker = TheCaveSystem->getTunnelTracker( getCaveContainModuleData()->m_caveUsesTeams, m_caveIndex, getObject()->getTeam() );
			if(myTracker)
				myTracker->onTunnelCreated( getObject() );
		}
	}
	OpenContain::update();
	return UPDATE_SLEEP_NONE;
}

//-------------------------------------------------------------------------------------------------
void CaveContain::onContaining( Object *obj, Bool wasSelected )
{
	OpenContain::onContaining( obj, wasSelected );
	// objects inside a building are held
	obj->setDisabled( DISABLED_HELD );

	//
	// the team of the building is now the same as those that have garrisoned it, be sure
	// to save our original team tho so that we can revert back to it when all the
	// occupants are gone
	//
	if(!getCaveContainModuleData()->m_caveUsesTeams)
		recalcApparentControllingPlayer();

}

//-------------------------------------------------------------------------------------------------
void CaveContain::onRemoving( Object *obj )
{
	OpenContain::onRemoving(obj);
	// object is no longer held inside a garrisoned building
	obj->clearDisabled( DISABLED_HELD );

	/// place the object in the world at position of the container m_object
	ThePartitionManager->registerObject( obj );
	obj->setPosition( getObject()->getPosition() );
	if( obj->getDrawable() )
	{
		obj->getDrawable()->setDrawableHidden( false );
	}

	doUnloadSound();

	if( getContainCount() == 0 )
	{

		// put us back on our original team
		// (hokey exception: if our team is null, don't bother -- this
		// usually means we are being called during game-teardown and
		// the teams are no longer valid...)
		if (getObject()->getTeam() != NULL && !getCaveContainModuleData()->m_caveHasOwner)
		{
			changeTeamOnAllConnectedCaves( m_originalTeam, FALSE );
			m_originalTeam = NULL;
		}

		// change the state back from garrisoned
		Drawable *draw = getObject()->getDrawable();
		if( draw )
		{
			draw->clearModelConditionState( MODELCONDITION_GARRISONED );
		}

	}  // end if
}

//-------------------------------------------------------------------------------------------------
void CaveContain::onSelling()
{
	// A TunnelContain tells everyone to leave if this is the last tunnel
	TunnelTracker *myTracker = TheCaveSystem->getTunnelTracker( getCaveContainModuleData()->m_caveUsesTeams, m_caveIndex, getObject()->getTeam() );
	if( myTracker == NULL )
		return;

	// We are the last tunnel, so kick everyone out.  This makes tunnels act like Palace and Bunker
	// rather than killing the occupants as if the last tunnel died.
	if( myTracker->friend_getTunnelCount() == 1 )
		removeAllContained(FALSE);// Can't be order to exit, as I have no time to organize their exits.
	// If they don't go right now, I will delete them in a moment

	// Unregister after the kick out, or else the unregistering will activate a cavein-kill.
	// We need to do this in case someone sells their last two tunnels at the same time.
	TheCaveSystem->unregisterCave( m_caveIndex );

	myTracker->onTunnelDestroyed( getObject() );
}

Bool CaveContain::isValidContainerFor(const Object* obj, Bool checkCapacity) const
{
	TunnelTracker *myTracker = TheCaveSystem->getTunnelTracker( getCaveContainModuleData()->m_caveUsesTeams, m_caveIndex, getObject()->getTeam() );
	if(!myTracker)
		return FALSE;
	return myTracker->isValidContainerFor( obj, checkCapacity );
}

UnsignedInt CaveContain::getContainCount() const
{
	TunnelTracker *myTracker = TheCaveSystem->getTunnelTracker( getCaveContainModuleData()->m_caveUsesTeams, m_caveIndex, getObject()->getTeam() );
	if(!myTracker)
		return 0;
	return myTracker->getContainCount();
}

Int CaveContain::getContainMax( void ) const
{
	TunnelTracker *myTracker = TheCaveSystem->getTunnelTracker( getCaveContainModuleData()->m_caveUsesTeams, m_caveIndex, getObject()->getTeam() );
	if(!myTracker)
		return 0;
	return myTracker->getContainMax();
}

const ContainedItemsList* CaveContain::getContainedItemsList() const
{
	TunnelTracker *myTracker = TheCaveSystem->getTunnelTracker( getCaveContainModuleData()->m_caveUsesTeams, m_caveIndex, getObject()->getTeam() );
	if(!myTracker)
		return NULL;
	return myTracker->getContainedItemsList();
}

//-------------------------------------------------------------------------------------------------
/** The die callback. */
//-------------------------------------------------------------------------------------------------
void CaveContain::onDie( const DamageInfo * damageInfo )
{
	// override the onDie we inherit from OpenContain. no super call.
	if (!getCaveContainModuleData()->m_dieMuxData.isDieApplicable(getObject(), damageInfo))
		return;

	if( getObject()->getStatusBits().test( OBJECT_STATUS_UNDER_CONSTRUCTION ) )
		return;//it never registered itself as a tunnel

	TunnelTracker *myTracker = TheCaveSystem->getTunnelTracker( getCaveContainModuleData()->m_caveUsesTeams, m_caveIndex, getObject()->getTeam() );

	TheCaveSystem->unregisterCave( m_caveIndex );

	if(myTracker)
		myTracker->onTunnelDestroyed( getObject() );
}

//-------------------------------------------------------------------------------------------------
void CaveContain::onDelete( void )
{
	// Being sold is a straight up delete.  no death

	if( getObject()->getStatusBits().test( OBJECT_STATUS_UNDER_CONSTRUCTION ) )
		return;//it never registered itself as a tunnel

	TunnelTracker *myTracker = TheCaveSystem->getTunnelTracker( getCaveContainModuleData()->m_caveUsesTeams, m_caveIndex, getObject()->getTeam() );
		
	TheCaveSystem->unregisterCave( m_caveIndex );

	if(myTracker)
		myTracker->onTunnelDestroyed( getObject() );
}

//-------------------------------------------------------------------------------------------------
void CaveContain::onCreate( void )
{
	m_caveIndex = getCaveContainModuleData()->m_caveIndexData;
	//TheCaveSystem->registerNewCave( m_caveIndex );
	m_originalTeam = getObject()->getTeam();
}

//-------------------------------------------------------------------------------------------------
void CaveContain::onObjectCreated()
{
	//Kris: July 29, 2003
	//Added this function to support the sneak attack (which doesn't call onBuildComplete).
	if( ! shouldDoOnBuildComplete() )
		return;

	m_needToRunOnBuildComplete = false;

	// Don't do it here. It will crash the game after loading from a game save. Do it after the game has been loaded, like updates.

	/*registerNewCave();

	TunnelTracker *myTracker = TheCaveSystem->getTunnelTracker( getCaveContainModuleData()->m_caveUsesTeams, m_caveIndex, getObject()->getTeam() );
	if(myTracker)
		myTracker->onTunnelCreated( getObject() );*/
}

//-------------------------------------------------------------------------------------------------
void CaveContain::onBuildComplete( void )
{
	//Kris: July 29, 2003
	//Obsolete -- onObjectCreated handles it before this function gets called.
	//Restored due to Loading save fixes. 
	
	if( ! shouldDoOnBuildComplete() )
		return;

	m_needToRunOnBuildComplete = false;

	m_loaded = true;

	registerNewCave();

	TunnelTracker *myTracker = TheCaveSystem->getTunnelTracker( getCaveContainModuleData()->m_caveUsesTeams, m_caveIndex, getObject()->getTeam() );

	myTracker->onTunnelCreated( getObject() );
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void CaveContain::onCapture( Player *oldOwner, Player *newOwner )
{
	// TO-DO use Hash_Maps to register TeamIDs to have their own Cave Systems.
	/*TunnelTracker *curTracker = TheCaveSystem->getTunnelTrackerForCaveIndex( m_caveIndex );
	if( curTracker )
	{
		TheCaveSystem->unregisterCave( m_caveIndex );
		curTracker->onTunnelDestroyed(getObject());
	}
	
	TheCaveSystem->registerNewCave( m_caveIndex );

	TunnelTracker *newTracker = TheCaveSystem->getTunnelTrackerForCaveIndex( m_caveIndex );
	
	if( newTracker )
		newTracker->onTunnelCreated(getObject());*/

	if(getCaveContainModuleData()->m_caveUsesTeams && !getCaveContainModuleData()->m_caveCaptureLinkCaves)
	{
		switchOwners();
		setOriginalTeam( getObject()->getTeam() );
	}
	else
	{
		recalcApparentControllingPlayerAndEvacuateUnits();
	}
	

	// extend base class
	OpenContain::onCapture( oldOwner, newOwner );
}

//-------------------------------------------------------------------------------------------------
void CaveContain::tryToSetCaveIndex( Int newIndex )
{
	Bool canSet;
	if(getCaveContainModuleData()->m_caveUsesTeams)
		canSet = TheCaveSystem->canSwitchIndexToIndexTeam( m_caveIndex, getObject()->getTeam(), newIndex, getObject()->getTeam() );
	else
		canSet = TheCaveSystem->canSwitchIndexToIndex( m_caveIndex, newIndex );

	if( canSet )
	{
		TunnelTracker *myOldTracker = TheCaveSystem->getTunnelTracker( getCaveContainModuleData()->m_caveUsesTeams, m_caveIndex, getObject()->getTeam() );
		TheCaveSystem->unregisterCave( m_caveIndex );
		if(myOldTracker)
			myOldTracker->onTunnelDestroyed( getObject() );

		m_caveIndex = newIndex;

		registerNewCave();
		TunnelTracker *myNewTracker = TheCaveSystem->getTunnelTracker( getCaveContainModuleData()->m_caveUsesTeams, m_caveIndex, getObject()->getTeam() );
		if(myNewTracker)
			myNewTracker->onTunnelCreated( getObject() );
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void CaveContain::recalcApparentControllingPlayer( void )
{
	//Record original team first time through.
	if( m_originalTeam == NULL )
	{
		m_originalTeam = getObject()->getTeam();
	}

	// (hokey trick: if our team is null, nuke originalTeam -- this
	// usually means we are being called during game-teardown and
	// the teams are no longer valid...)
	if (getObject()->getTeam() == NULL)
		m_originalTeam = NULL;

	// This is called from onContaining, so a one is the edge trigger to do capture stuff
	if( getContainCount() == 1 )
	{
		ContainedItemsList::const_iterator it = getContainedItemsList()->begin();
		Object *rider = *it;

		// This also gets called during reset from the PlayerList, so we might not actually have players.
		// Check like the hokey trick mentioned above
		if( rider->getControllingPlayer() )
			changeTeamOnAllConnectedCaves( rider->getControllingPlayer()->getDefaultTeam(), TRUE );

	}
	else if( getContainCount() == 0 )
	{
		// And a 0 is the edge trigger to do uncapture stuff
		changeTeamOnAllConnectedCaves( m_originalTeam, FALSE );
	}

	// Handle the team color that is rendered
	const Player* controller = getApparentControllingPlayer(ThePlayerList->getLocalPlayer());
	if (controller)
	{
		if (TheGlobalData->m_timeOfDay == TIME_OF_DAY_NIGHT)
			getObject()->getDrawable()->setIndicatorColor( controller->getPlayerNightColor() );
		else
			getObject()->getDrawable()->setIndicatorColor( controller->getPlayerColor() );
	}
}

/////////////////////////////////////////////////////////////////////////////////////
static CaveInterface* findCave(Object* obj)
{
	for (BehaviorModule** i = obj->getBehaviorModules(); *i; ++i)
	{
		CaveInterface* c = (*i)->getCaveInterface();
		if (c != NULL)
			return c;
	}
	return NULL;
}

/////////////////////////////////////////////////////////////////////////////////////
void CaveContain::changeTeamOnAllConnectedCaves( Team *newTeam, Bool setOriginalTeams )
{
	TunnelTracker *myTracker = TheCaveSystem->getTunnelTrackerForCaveIndex( m_caveIndex );
	const std::list<ObjectID> *allCaves = myTracker->getContainerList();
	for( std::list<ObjectID>::const_iterator iter = allCaves->begin(); iter != allCaves->end(); iter++ )
	{
		// For each ID, look it up and change its team.  We all get captured together.
		Object *currentCave = TheGameLogic->findObjectByID( *iter );
		if( currentCave )
		{
			// This is a distributed Garrison in terms of capturing, so when one node
			// triggers the change, he needs to tell everyone, so anyone can do the un-change.
			CaveInterface *caveModule = findCave(currentCave);
			if( caveModule == NULL )
				continue;
			if( setOriginalTeams )
				caveModule->setOriginalTeam( currentCave->getTeam() );
			else
				caveModule->setOriginalTeam( NULL );

			// Now do the actual switch for this one.

			currentCave->defect( newTeam, 0 );
//			currentCave->setTeam( newTeam );
		}
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void CaveContain::recalcApparentControllingPlayerAndEvacuateUnits( void )
{
	// (hokey trick: if our team is null, nuke originalTeam -- this
	// usually means we are being called during game-teardown and
	// the teams are no longer valid...)
	if (getObject()->getTeam() == NULL)
		return;

	TunnelTracker *oldTracker = TheCaveSystem->getTunnelTrackerForCaveIndexTeam( m_caveIndex, m_originalTeam );
	const std::list<ObjectID> *allCaves = oldTracker->getContainerList();
	for( std::list<ObjectID>::const_iterator iter = allCaves->begin(); iter != allCaves->end(); iter++ )
	{
		// For each ID, look it up and change its team.  We all get captured together.
		Object *currentCave = TheGameLogic->findObjectByID( *iter );
		if( currentCave )
		{
			// This is a distributed Garrison in terms of capturing, so when one node
			// triggers the change, he needs to tell everyone, so anyone can do the un-change.
			CaveInterface *caveModule = findCave(currentCave);
			if( caveModule == NULL )
				continue;

			caveModule->switchOwners();
			caveModule->setOriginalTeam( currentCave->getTeam() );

			// Now do the actual switch for this one.

			if(*iter != getObject()->getID())
				currentCave->defect( currentCave->getTeam(), 0 );
		}
	}

	// Handle the team color that is rendered
	const Player* controller = getApparentControllingPlayer(ThePlayerList->getLocalPlayer());
	if (controller)
	{
		if (TheGlobalData->m_timeOfDay == TIME_OF_DAY_NIGHT)
			getObject()->getDrawable()->setIndicatorColor( controller->getPlayerNightColor() );
		else
			getObject()->getDrawable()->setIndicatorColor( controller->getPlayerColor() );
	}
}

void CaveContain::registerNewCave()
{
	if(getCaveContainModuleData()->m_caveUsesTeams)
		TheCaveSystem->registerNewCaveTeam( m_caveIndex, getObject()->getTeam() );
	else
		TheCaveSystem->registerNewCave( m_caveIndex );
}

void CaveContain::switchOwners()
{
	TunnelTracker *curTracker = TheCaveSystem->getTunnelTrackerForCaveIndexTeam( m_caveIndex, m_originalTeam );
	if(curTracker)
	{
		// Switching Owners, check if there's still other tunnels.  If not, evacuate everyone.
		if( curTracker->friend_getTunnelCount() == 1 )
			removeAllContained(FALSE);
		TheCaveSystem->unregisterCave( m_caveIndex );
		curTracker->onTunnelDestroyed(getObject());
	}

	registerNewCave();
	TunnelTracker *newTracker = TheCaveSystem->getTunnelTracker( getCaveContainModuleData()->m_caveUsesTeams, m_caveIndex, getObject()->getTeam() );
	if( newTracker )
		newTracker->onTunnelCreated(getObject());
}

/////////////////////////////////////////////////////////////////////////////////////
void CaveContain::setOriginalTeam( Team *oldTeam )
{
	m_originalTeam = oldTeam;
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void CaveContain::crc( Xfer *xfer )
{

	// extend base class
	OpenContain::crc( xfer );

}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void CaveContain::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	OpenContain::xfer( xfer );

	// need to run on build complete
	xfer->xferBool( &m_needToRunOnBuildComplete );

	// cave index
	xfer->xferInt( &m_caveIndex );

	// original team
	TeamID teamID = m_originalTeam ? m_originalTeam->getID() : TEAM_ID_INVALID;
	xfer->xferUser( &teamID, sizeof( TeamID ) );
	if( xfer->getXferMode() == XFER_LOAD )
	{

		if( teamID != TEAM_ID_INVALID )
		{

			m_originalTeam = TheTeamFactory->findTeamByID( teamID );
			if( m_originalTeam == NULL )
			{

				DEBUG_CRASH(( "CaveContain::xfer - Unable to find original team by id" ));
				throw SC_INVALID_DATA;

			}  // end if

		}  // end if
		else
			m_originalTeam = NULL;

	}  // end if

}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void CaveContain::loadPostProcess( void )
{

	// extend base class
	OpenContain::loadPostProcess();

}  // end loadPostProcess

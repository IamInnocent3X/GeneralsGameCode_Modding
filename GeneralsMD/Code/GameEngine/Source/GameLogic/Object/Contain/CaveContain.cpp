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
#include "Common/ThingFactory.h"
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
	m_payloadCreated = FALSE;
	m_switchingOwners = FALSE;
	m_containingFrames = 0;
	m_isCaptured = FALSE;
	m_capturedTeam = NULL;
	m_oldTeam = NULL;
	m_newTeam = NULL;
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

//-------------------------------------------------------------------------------------------------
void CaveContain::orderAllPassengersToExit( CommandSourceType commandSource, Bool instantly )
{
	TunnelTracker *myTracker = TheCaveSystem->getTunnelTracker( getCaveContainModuleData()->m_caveUsesTeams, m_caveIndex, getObject()->getTeam() );
	if(!myTracker)
		return;

	for( ContainedItemsList::const_iterator it = getContainedItemsList()->begin(); it != getContainedItemsList()->end(); )
	{
		// save the rider...
		Object* rider = *it;

		// incr the iterator BEFORE calling the func (if the func removes the rider,
		// the iterator becomes invalid)
		++it;

		if(rider->getTeam() != getObject()->getTeam())
			continue;

		// call it
		if( rider->getAI() )
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
	// Loading fixes. Cave System does not register properly while the game is loaded.
	if( (!m_needToRunOnBuildComplete || !getObject()->testStatus( OBJECT_STATUS_UNDER_CONSTRUCTION )) && !m_loaded)
	{
		TunnelTracker *myTracker = TheCaveSystem->getTunnelTracker( getCaveContainModuleData()->m_caveUsesTeams, m_caveIndex, getObject()->getTeam() );
		if( myTracker == NULL )
		{
			registerNewCave();
		}
		Bool registerMyTunnel = TRUE;
		
		TunnelTracker *newTracker = TheCaveSystem->getTunnelTracker( getCaveContainModuleData()->m_caveUsesTeams, m_caveIndex, getObject()->getTeam() );
		const std::list<ObjectID> *allCaves = newTracker->getContainerList();
		for( std::list<ObjectID>::const_iterator iter = allCaves->begin(); iter != allCaves->end(); iter++ )
		{
			// For each ID, look it up and change its team.  We all get captured together.
			if( *iter == getObject()->getID())
			{
				registerMyTunnel = FALSE;
				break;
			}
		}

		if(registerMyTunnel)
			newTracker->onTunnelCreated( getObject() );

		// Register a substitude value into captured Team so that we can register the original team later in case if this is neutral.
		if(m_capturedTeam == NULL && m_originalTeam != NULL)
		{
			m_capturedTeam = m_originalTeam;
		}
		
		// Payload System
		if(!m_payloadCreated)
		{
			ContainModuleInterface *contain = getObject()->getContain();
			if(contain && newTracker)
			{
				contain->enableLoadSounds( FALSE );

				Int count = getCaveContainModuleData()->m_initialPayload.count;

				for( int i = 0; i < count; i++ )
				{
					//We are creating a transport that comes with a initial payload, so add it now!
					const ThingTemplate* payloadTemplate = TheThingFactory->findTemplate( getCaveContainModuleData()->m_initialPayload.name );
					Object* payload = TheThingFactory->newObject( payloadTemplate, m_originalTeam );
					if( newTracker->isValidContainerFor( payload, true ) )
					{
						contain->addToContain( payload );
					}
					else
					{
						Object *theContainer = getObject();

						// Scatter to nearby Position
						Real angle = GameLogicRandomValueReal( 0.0f, 2.0f * PI );
					//	angle = TheTacticalView->getAngle();
					//	angle -= GameLogicRandomValueReal( PI / 3.0f, 2.0f * (PI / 3.0F) );

						Real minRadius = theContainer->getGeometryInfo().getBoundingCircleRadius();
						Real maxRadius = minRadius + minRadius / 2.0f;
						const Coord3D *containerPos = theContainer->getPosition();
						Real dist = GameLogicRandomValueReal( minRadius, maxRadius );

						Coord3D pos;
						pos.x = dist * Cos( angle ) + containerPos->x;
						pos.y = dist * Sin( angle ) + containerPos->y;
						pos.z = TheTerrainLogic->getGroundHeight( pos.x, pos.y );

						// set orientation
						payload->setOrientation( angle );

						AIUpdateInterface *ai = payload->getAIUpdateInterface();
						if( ai )
						{
							// set position of the object at center of building and move them toward pos
							payload->setPosition( theContainer->getPosition() );
							ai->ignoreObstacle(theContainer);
							ai->aiMoveToPosition( &pos, CMD_FROM_AI );
						}
						else
						{
							payload->setPosition( &pos );
						}
					}
				}

				contain->enableLoadSounds( TRUE );
			}
			m_payloadCreated = TRUE;
		}

		m_loaded = true;
	}

	if(m_oldTeam != NULL && m_newTeam != NULL)
	{		
		if(getCaveContainModuleData()->m_caveUsesTeams)
		{
			TheCaveSystem->registerNewCaveTeam( m_caveIndex, m_newTeam );
		}
		TunnelTracker *curTracker = TheCaveSystem->getTunnelTracker( getCaveContainModuleData()->m_caveUsesTeams, m_caveIndex, m_newTeam );
		if(curTracker)
		{
			doCapture(m_oldTeam, m_newTeam);
			m_oldTeam = NULL;
			m_newTeam = NULL;
		}
	}

	if(m_containingFrames && TheGameLogic->getFrame() > m_containingFrames)
	{
		TunnelTracker *curTracker = TheCaveSystem->getTunnelTracker( getCaveContainModuleData()->m_caveUsesTeams, m_caveIndex, getObject()->getTeam() );
		if(curTracker)
		{
			if(curTracker->getIsContaining())
				curTracker->setIsContaining(FALSE);
			if(curTracker->getIsCapturingLinkedCaves())
				curTracker->setIsCapturingLinkedCaves(FALSE);
		}
		m_containingFrames = 0;
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

	if(!m_switchingOwners)
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

	if( getContainCount() == 0 && !m_switchingOwners)
	{

		// put us back on our original team
		// (hokey exception: if our team is null, don't bother -- this
		// usually means we are being called during game-teardown and
		// the teams are no longer valid...)
		if (getObject()->getTeam() != NULL )
		{
			// Set it so it doesn't get treated as "capturing"
			TunnelTracker *myTracker = TheCaveSystem->getTunnelTracker( getCaveContainModuleData()->m_caveUsesTeams, m_caveIndex, getOldTeam() );
			if(myTracker)
				myTracker->setIsContaining(TRUE);

			m_containingFrames = TheGameLogic->getFrame() + 2;

			changeTeamOnAllConnectedCaves( m_originalTeam, FALSE );
			if(!getHasPermanentOwner())
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
	const Object *us = getObject();
	const CaveContainModuleData *modData = getCaveContainModuleData();

	// if we have any kind of masks set then we must make that check
	if (obj->isAnyKindOf( modData->m_allowInsideKindOf ) == FALSE ||
			obj->isAnyKindOf( modData->m_forbidInsideKindOf ) == TRUE)
	{
		return false;
	}

 	//
 	// check relationship, note that this behavior is defined as the relation between
 	// 'obj' and the container 'us', and not the reverse
 	//
 	Bool relationshipRestricted = FALSE;
 	Relationship r = obj->getRelationship( us );
 	switch( r )
 	{
 		case ALLIES:
 			if( modData->m_allowAlliesInside == FALSE )
 				relationshipRestricted = TRUE;
 			break;

 		case ENEMIES:
 			if( modData->m_allowEnemiesInside == FALSE )
 				relationshipRestricted = TRUE;
 			break;

 		case NEUTRAL:
 			if( modData->m_allowNeutralInside == FALSE )
 				relationshipRestricted = TRUE;
 			break;

 		default:
 			DEBUG_CRASH(( "isValidContainerFor: Undefined relationship (%d) between '%s' and '%s'",
 										r, getObject()->getTemplate()->getName().str(),
 										obj->getTemplate()->getName().str() ));
 			return FALSE;

 	}  // end switch
 	if( relationshipRestricted == TRUE )
 		return FALSE;

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

	// do the tunnel tracker stuff later, since onContaining Registers after onCapture
	m_oldTeam = oldOwner->getDefaultTeam();
	m_newTeam = newOwner->getDefaultTeam();
	
	// extend base class
	OpenContain::onCapture( oldOwner, newOwner );
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void CaveContain::doCapture( Team *oldTeam, Team *newTeam )
{
	switchCaveOwners(oldTeam, newTeam);

	TunnelTracker *newTracker = TheCaveSystem->getTunnelTracker( getCaveContainModuleData()->m_caveUsesTeams, m_caveIndex, newTeam );

	// If we are captured, not defected, grant the ownership to the Player permanently.
	if(ThePlayerList->getLocalPlayer()->getRelationship(getObject()->getTeam()) != NEUTRAL && !newTracker->getIsContaining())
	{
		setOriginalTeam(m_capturedTeam);
		m_isCaptured = TRUE; // The actual factor to determine that it is captured.
		m_capturedTeam = newTeam;
	}

	if( m_isCaptured && newTracker && !newTracker->getIsContaining() && getCaveContainModuleData()->m_caveCaptureLinkCaves && !newTracker->getIsCapturingLinkedCaves() )
	{
		newTracker->setIsCapturingLinkedCaves(TRUE);
		m_containingFrames = TheGameLogic->getFrame() + 2;
		if(!getCaveContainModuleData()->m_caveUsesTeams)
		{
			changeTeamOnAllConnectedCaves( newTeam, TRUE );
		}
		else
		{
			changeTeamOnAllConnectedCavesByTeam( oldTeam, TRUE, newTeam );
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
		{
			if(getCaveContainModuleData()->m_caveUsesTeams)
			{
				TheCaveSystem->registerNewCaveTeam( m_caveIndex, rider->getControllingPlayer()->getDefaultTeam() );
			}

			TunnelTracker *newTracker = TheCaveSystem->getTunnelTracker( getCaveContainModuleData()->m_caveUsesTeams, m_caveIndex, rider->getControllingPlayer()->getDefaultTeam() );
			if(newTracker)
				newTracker->setIsContaining(TRUE);

			m_containingFrames = TheGameLogic->getFrame() + 2;

			changeTeamOnAllConnectedCaves( rider->getControllingPlayer()->getDefaultTeam(), TRUE );
		}

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
	if(getCaveContainModuleData()->m_caveUsesTeams)
	{
		changeTeamOnAllConnectedCavesByTeam( getObject()->getTeam(), setOriginalTeams, newTeam );
		return;
	}

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
			
			Team *teamToSet;
			if( setOriginalTeams )
			{
				caveModule->setOriginalTeam( currentCave->getTeam() );
				teamToSet = newTeam;
			}
			else
			{
				caveModule->setOriginalTeam( NULL );
				teamToSet = caveModule->getOldTeam();
			}

			// Now do the actual switch for this one.
			if( !caveModule->getHasPermanentOwner() || myTracker->getIsCapturingLinkedCaves() )
				currentCave->defect( teamToSet, 0 );
//			currentCave->setTeam( newTeam );
		}
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void CaveContain::changeTeamOnAllConnectedCavesByTeam( Team *checkTeam, Bool setOriginalTeams, Team *newTeam )
{
	// Since we are switching Teams, we will destroy the teams of the Old Tunnels and add to our own.
	TunnelTracker *currTracker = TheCaveSystem->getTunnelTrackerForCaveIndexTeam( m_caveIndex, checkTeam );
	const ContainedItemsList* items = currTracker->getContainedItemsList();
	std::vector<ObjectID> vecContainedIDs, vecCaveIDs;
	
	// Tell the Module to not do shenenigans with OnContaining and OnRemoving
	m_switchingOwners = TRUE;

	// Remove the Contained Objects first
	ContainedItemsList::const_iterator it_test = items->begin();
	while ( it_test != items->end() )
	{
		// On Removing
		// If the Cave is captured, but other Team's "occupants" are still in their Cave, do not clear them.
		if(getContainCount() == 0)
			break;
		
		Object *obj = *it_test++;
		vecContainedIDs.push_back(obj->getID());

		// Remove All Contained from the current Cave
		if( obj && currTracker->isInContainer( obj ) )
		{
			// This must come before the onRemov*, because CaveContain's version has a edge-0 triggered event.
			// If that were to go first, the number would still be 1 at that time.  Noone else cares about
			// order.
			currTracker->removeFromContain( obj, TRUE );

			// trigger an onRemoving event for 'm_object' no longer containing 'itemToRemove->m_object'
			if (getObject()->getContain())
				getObject()->getContain()->onRemoving( obj );

			// trigger an onRemovedFrom event for 'remove'
			obj->onRemovedFrom( getObject() );
		}
	}

	//removeAllContained(TRUE);

	// Get all the current Caves and switch their properties
	const std::list<ObjectID> *allCaves = currTracker->getContainerList();
	for( std::list<ObjectID>::const_iterator iter = allCaves->begin(); iter != allCaves->end(); iter++ )
	{
		// For each ID, look it up and change its team.  We all get captured together.
		Object *currentCave = TheGameLogic->findObjectByID( *iter );
		if( currentCave )
		{
			// ...while registering them to remove them from their Tunnels list and add it to our own later.
			vecCaveIDs.push_back(*iter);
			// This is a distributed Garrison in terms of capturing, so when one node
			// triggers the change, he needs to tell everyone, so anyone can do the un-change.
			//CaveInterface *caveModule = findCave(currentCave);
			//if( caveModule == NULL )
			//	continue;
			//if( setOriginalTeams )
			//	caveModule->setOriginalTeam( currentCave->getTeam() );
			//else
			//	caveModule->setOriginalTeam( NULL );

			// Now do the actual switch for this one.

//			currentCave->defect( newTeam, 0 );
//			currentCave->setTeam( newTeam );
					
			// Do it later, or we will screw up the list
			// News, functions are carried out through onCapture. No need to do anything here.
			//currTracker->onTunnelDestroyed( getObject() );
			//TheCaveSystem->unregisterCave( m_caveIndex );
			//newTracker->onTunnelCreated( getObject() );
		}
	}
	
	TheCaveSystem->registerNewCaveTeam( m_caveIndex, newTeam );

	TunnelTracker *newTracker = TheCaveSystem->getTunnelTrackerForCaveIndexTeam( m_caveIndex, newTeam );
	
	// Wasn't so hard now was it?
	if(newTracker)
	{
		for(int i = 0; i < vecCaveIDs.size(); i++)
		{
			Object *cave = TheGameLogic->findObjectByID( vecCaveIDs[i] );
			if(cave)
			{
				//currTracker->onTunnelDestroyed( cave );
				CaveInterface *caveModule = findCave(cave);

				if(caveModule == NULL)
					continue;

				Team *teamToSet;
				if( setOriginalTeams )
				{
					caveModule->setOriginalTeam( cave->getTeam() );
					teamToSet = newTeam;
				}
				else
				{
					caveModule->setOriginalTeam( NULL );
					teamToSet = caveModule->getOldTeam();
				}

				if(!caveModule->getHasPermanentOwner() || newTracker->getIsCapturingLinkedCaves())
					cave->defect( teamToSet, 0 );
				//newTracker->onTunnelCreated( cave );
			}
		}

		// I added this.. for what reason?
		ContainModuleInterface *contain = getObject()->getContain();

		if(contain)
		{
			for(int i_2 = 0; i_2 < vecContainedIDs.size(); i_2++)
			{
				Object *add = TheGameLogic->findObjectByID( vecContainedIDs[i_2] );
				if(add && add->getTeam() == getObject()->getTeam())
				{
					contain->addToContain(add);
				}
			}
		}
	}
	m_switchingOwners = FALSE;
}

//-------------------------------------------------------------------------------------------------
void CaveContain::registerNewCave()
{
	if(getCaveContainModuleData()->m_caveUsesTeams)
		TheCaveSystem->registerNewCaveTeam( m_caveIndex, getObject()->getTeam() );
	else
		TheCaveSystem->registerNewCave( m_caveIndex );
}

//-------------------------------------------------------------------------------------------------
void CaveContain::switchCaveOwners( Team *oldTeam, Team *newTeam )
{
	TunnelTracker *curTracker = TheCaveSystem->getTunnelTracker( getCaveContainModuleData()->m_caveUsesTeams, m_caveIndex, oldTeam );

	// Disable OnContaining or OnRemoving from triggering their shenenigans
	m_switchingOwners = TRUE;
	
	// Function differs if the person uses Teams system
	if(curTracker && !curTracker->getIsContaining() && !getCaveContainModuleData()->m_caveUsesTeams)
	{
		// Switching Owners, evacuate everyone if it is Captured
		removeAllNonOwnContained(newTeam, TRUE);

		// Reenable them
		m_switchingOwners = FALSE;

		// If we don't use teams, we stop here
		return;
	}

	TunnelTracker *newTracker = TheCaveSystem->getTunnelTrackerForCaveIndexTeam( m_caveIndex, newTeam );
	if( curTracker && newTracker && !newTracker->getIsContaining() )
	{
		//removeAllContained(TRUE);

		// Remove from Old Teams
		if(curTracker->friend_getTunnelCount() == 1)
		{
			const ContainedItemsList *fullList = curTracker->getContainedItemsList();

			// Basically RemoveAllContained but for the team it was captured from
			Object *obj;
			ContainedItemsList::const_iterator it;
			it = (*fullList).begin();
			while( it != (*fullList).end() )
			{
				obj = *it;
				it++;

				if( curTracker->isInContainer( obj ) )
				{
					// This must come before the onRemov*, because CaveContain's version has a edge-0 triggered event.
					// If that were to go first, the number would still be 1 at that time.  Noone else cares about
					// order.
					curTracker->removeFromContain( obj, TRUE );

					// trigger an onRemoving event for 'm_object' no longer containing 'itemToRemove->m_object'
					if (getObject()->getContain())
						getObject()->getContain()->onRemoving( obj );

					// trigger an onRemovedFrom event for 'remove'
					obj->onRemovedFrom( getObject() );
				}
			}
		}

		// Destroy the other team's tunnels if we are using Teams
		//TheCaveSystem->unregisterCave( m_caveIndex );
		curTracker->onTunnelDestroyed(getObject());
		//curTracker->setIsContaining(FALSE);

		newTracker->onTunnelCreated(getObject());
	}

	m_switchingOwners = FALSE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void CaveContain::removeAllNonOwnContained( Team *myTeam, Bool exposeStealthUnits )
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
		if(obj->getTeam() != myTeam)
			removeFromContain( obj, exposeStealthUnits );
	}
}

/////////////////////////////////////////////////////////////////////////////////////
void CaveContain::setOriginalTeam( Team *oldTeam )
{
	if(!m_isCaptured && !getCaveContainModuleData()->m_caveHasOwner)
		m_originalTeam = oldTeam;
}

/////////////////////////////////////////////////////////////////////////////////////
Bool CaveContain::getHasPermanentOwner() const
{
	return m_isCaptured || getCaveContainModuleData()->m_caveHasOwner;
}

/////////////////////////////////////////////////////////////////////////////////////
Team *CaveContain::getOldTeam() const
{
	return m_capturedTeam;
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

	xfer->xferBool( &m_isCaptured );

	xfer->xferBool( &m_payloadCreated );

	// original team
	TeamID teamID = m_originalTeam ? m_originalTeam->getID() : TEAM_ID_INVALID;
	xfer->xferUser( &teamID, sizeof( TeamID ) );
	TeamID capturedID = m_capturedTeam ? m_capturedTeam->getID() : TEAM_ID_INVALID;
	xfer->xferUser( &capturedID, sizeof( TeamID ) );
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

		if( capturedID != TEAM_ID_INVALID )
		{

			m_capturedTeam = TheTeamFactory->findTeamByID( capturedID );
			if( m_capturedTeam == NULL )
			{

				DEBUG_CRASH(( "CaveContain::xfer - Unable to find captured team by id" ));
				throw SC_INVALID_DATA;

			}  // end if

		}  // end if
		else
			m_capturedTeam = NULL;

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

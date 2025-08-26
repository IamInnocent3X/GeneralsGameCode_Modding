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

// FILE: TunnelContain.cpp ////////////////////////////////////////////////////////////////////////////
// Author: Graham Smallwood, March 2002
// Desc:   A version of OpenContain that overrides where the passengers are stored: the Owning Player's
//					TunnelTracker.  All queries about capacity and contents are also redirected.
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file int the GameEngine

#include "Common/Player.h"
#include "Common/RandomValue.h"
#include "Common/ThingTemplate.h"
#include "Common/TunnelTracker.h"
#include "Common/Xfer.h"
#include "GameClient/Drawable.h"
#include "GameLogic/Module/AIUpdate.h"
#include "GameLogic/Module/OpenContain.h"
#include "GameLogic/Module/TunnelContain.h"
#include "GameLogic/Object.h"
#include "GameLogic/PartitionManager.h"


///////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS ///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
TunnelContain::TunnelContain( Thing *thing, const ModuleData* moduleData ) : OpenContain( thing, moduleData )
{
	m_needToRunOnBuildComplete = true;
	m_isCurrentlyRegistered = FALSE;
	m_checkAttackFrames = 0;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
TunnelContain::~TunnelContain()
{
}

void TunnelContain::addToContainList( Object *obj )
{
	Player *owningPlayer = getObject()->getControllingPlayer();

	if(!owningPlayer->getTunnelSystem())
		return;

	owningPlayer->getTunnelSystem()->addToContainList( obj );
}

//-------------------------------------------------------------------------------------------------
/** Remove 'obj' from the m_containList of objects in this module.
	* This will trigger an onRemoving event for the object that this module
	* is a part of and an onRemovedFrom event for the object being removed */
//-------------------------------------------------------------------------------------------------
void TunnelContain::removeFromContain( Object *obj, Bool exposeStealthUnits )
{

	// sanity
	if( obj == NULL )
		return;

	// trigger an onRemoving event for 'm_object' no longer containing 'itemToRemove->m_object'
	if( getObject()->getContain() )
	{
		getObject()->getContain()->onRemoving( obj );
	}

	// trigger an onRemovedFrom event for 'remove'
	obj->onRemovedFrom( getObject() );

	//
	// we can only remove this object from the contains list of this module if
	// it is actually contained by this module
	//
	Player *owningPlayer = getObject()->getControllingPlayer();
	if( owningPlayer == NULL )
		return; //game tear down.  We do the onRemove* stuff first because this is allowed to fail but that still needs to be done

	if(!owningPlayer->getTunnelSystem())
		return;

	owningPlayer->getTunnelSystem()->removeFromContain( obj, exposeStealthUnits );

}



//--------------------------------------------------------------------------------------------------------
/** Force all contained objects in the contained list to exit, and kick them in the pants on the way out*/
//--------------------------------------------------------------------------------------------------------
void TunnelContain::harmAndForceExitAllContained( DamageInfo *info )
{
	Player *owningPlayer = getObject()->getControllingPlayer();

	if(!owningPlayer->getTunnelSystem())
		return;

	const ContainedItemsList *fullList = owningPlayer->getTunnelSystem()->getContainedItemsList();

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
void TunnelContain::killAllContained( void )
{
	// TheSuperHackers @bugfix xezon 04/06/2025 Empty the TunnelSystem's Contained Items List
	// straight away to prevent a potential child call to catastrophically modify it as well.
	// This scenario can happen if the killed occupant(s) apply deadly damage on death
	// to the host container, which then attempts to remove all remaining occupants
	// on the death of the host container. This is reproducible by shooting with
	// Neutron Shells on a GLA Tunnel containing GLA Terrorists.

	Player *owningPlayer = getObject()->getControllingPlayer();

	if(!owningPlayer->getTunnelSystem())
		return;

	ContainedItemsList list;
	owningPlayer->getTunnelSystem()->swapContainedItemsList(list);

	ContainedItemsList::iterator it = list.begin();

	while ( it != list.end() )
	{
		Object *obj = *it++;
		DEBUG_ASSERTCRASH( obj, ("Contain list must not contain NULL element"));

		removeFromContain( obj, true );

		obj->kill();
	}
}
//-------------------------------------------------------------------------------------------------
/** Remove all contained objects from the contained list */
//-------------------------------------------------------------------------------------------------
void TunnelContain::removeAllContained( Bool exposeStealthUnits )
{
	Player *owningPlayer = getObject()->getControllingPlayer();

	if(!owningPlayer->getTunnelSystem())
		return;

	ContainedItemsList list;
	owningPlayer->getTunnelSystem()->swapContainedItemsList(list);

	ContainedItemsList::iterator it = list.begin();

	while ( it != list.end() )
	{
		Object *obj = *it++;
		DEBUG_ASSERTCRASH( obj, ("Contain list must not contain NULL element"));

		removeFromContain( obj, exposeStealthUnits );
	}
}

//-------------------------------------------------------------------------------------------------
/** Iterate the contained list and call the callback on each of the objects */
//-------------------------------------------------------------------------------------------------
void TunnelContain::iterateContained( ContainIterateFunc func, void *userData, Bool reverse )
{
	Player *owningPlayer = getObject()->getControllingPlayer();

	if(!owningPlayer->getTunnelSystem())
		return;

	owningPlayer->getTunnelSystem()->iterateContained( func, userData, reverse );
}

//-------------------------------------------------------------------------------------------------
void TunnelContain::onContaining( Object *obj, Bool wasSelected )
{
	OpenContain::onContaining( obj, wasSelected );

	// objects inside a building are held
	obj->setDisabled( DISABLED_HELD );

	obj->getControllingPlayer()->getAcademyStats()->recordUnitEnteredTunnelNetwork();




  obj->handlePartitionCellMaintenance();


}

//-------------------------------------------------------------------------------------------------
void TunnelContain::onRemoving( Object *obj )
{
	OpenContain::onRemoving(obj);

	// object is no longer held inside a garrisoned building
	obj->clearDisabled( DISABLED_HELD );

	/// place the object in the world at position of the container m_object
	ThePartitionManager->registerObject( obj );
	obj->setPosition( getObject()->getPosition() );
	if( obj->getDrawable() )
	{
		obj->setSafeOcclusionFrame(TheGameLogic->getFrame()+obj->getTemplate()->getOcclusionDelay());
		obj->getDrawable()->setDrawableHidden( false );
	}

	doUnloadSound();


}

//-------------------------------------------------------------------------------------------------
void TunnelContain::onSelling()
{
	// A TunnelContain tells everyone to leave if this is the last tunnel
	Player *owningPlayer = getObject()->getControllingPlayer();
	if( owningPlayer == NULL )
		return;
	TunnelTracker *tunnelTracker = owningPlayer->getTunnelSystem();
	if( tunnelTracker == NULL )
		return;

	// We are the last tunnel, so kick everyone out.  This makes tunnels act like Palace and Bunker
	// rather than killing the occupants as if the last tunnel died.
	if( tunnelTracker->friend_getTunnelCount() == 1 )
		removeAllContained(FALSE);// Can't be order to exit, as I have no time to organize their exits.
	// If they don't go right now, I will delete them in a moment

	// Unregister after the kick out, or else the unregistering will activate a cavein-kill.
	// We need to do this in case someone sells their last two tunnels at the same time.
	if( m_isCurrentlyRegistered )
	{
		tunnelTracker->onTunnelDestroyed( getObject() );
		m_isCurrentlyRegistered = FALSE;
	}
}

//-------------------------------------------------------------------------------------------------
Bool TunnelContain::isPassengerAllowedToFire( ObjectID id ) const
{
	// Sanity Checks
	if ( ! getObject() )
    	return FALSE;
	
	Player *owningPlayer = getObject()->getControllingPlayer();
	if( owningPlayer == NULL )
		return FALSE;
	TunnelTracker *tunnelTracker = owningPlayer->getTunnelSystem();
	if( tunnelTracker == NULL )
		return FALSE;

	const TunnelContainModuleData *modData = getTunnelContainModuleData();
	const Object *me = getObject();

	if( ! modData->m_passengersAllowedToFire )
		return FALSE;// Just no, no matter what.

	// ECM Properties
	if ( me->isDisabledByType( DISABLED_SUBDUED ) || me->isDisabledByType( DISABLED_FROZEN ) )
		return FALSE;

	// Check for upgrades needed for the Contained to Open Fire
	if(!modData->m_activationUpgradeNames.empty())
	{
		std::vector<AsciiString>::const_iterator it;
		for( it = modData->m_activationUpgradeNames.begin();
					it != modData->m_activationUpgradeNames.end();
					it++)
		{
			const UpgradeTemplate* ut = TheUpgradeCenter->findUpgrade( *it );
			if( !ut )
			{
				DEBUG_CRASH(("An upgrade module references '%s', which is not an Upgrade", it->str()));
				throw INI_INVALID_DATA;
			}

			if( !me->hasUpgrade(ut) )
			{
				return FALSE;
			}
		}
	}

	Object *passenger = TheGameLogic->findObjectByID(id);

	if( passenger != NULL )
	{
		// if we have any kind of masks set then we must make that check
		if (passenger->isAnyKindOf( modData->m_allowInsideKindOf ) == FALSE ||
				passenger->isAnyKindOf( modData->m_forbidInsideKindOf ) == TRUE)
		{
			return false;
		}
	}

  // but wait! I may be riding on an Overlord
  // This code detects the case of whether the contained passenger is in a bunker riding on an overlord, inside a helix!
  // Oh  my  God.
  const Object *heWhoContainsMe = me->getContainedBy();
  if ( heWhoContainsMe)
  {
    ContainModuleInterface *hisContain = heWhoContainsMe->getContain();
    DEBUG_ASSERTCRASH( hisContain,("TransportContain::isPassengerAllowedToFire()... CONTAINER WITHOUT A CONTAIN! AARRGH!") );
    if ( hisContain && hisContain->isSpecialOverlordStyleContainer() )
      return hisContain->isPassengerAllowedToFire( id );
  }

	// If we are ourselves contained, our passengers need to check with them if they get past us
	if( me->getContainedBy() != NULL )
		return me->getContainedBy()->getContain()->isPassengerAllowedToFire();

	return TRUE;
	//return OpenContain::isPassengerAllowedToFire();
}

void TunnelContain::doUpgradeChecks()
{
	if ( !getObject() )
    	return;
	Player *owningPlayer = getObject()->getControllingPlayer();
	if( owningPlayer == NULL )
		return;
	TunnelTracker *tunnelTracker = owningPlayer->getTunnelSystem();
	if( tunnelTracker == NULL )
		return;

	const TunnelContainModuleData *modData = getTunnelContainModuleData();

	if(modData->m_activationUpgradeNames.empty())
		return;

	std::vector<ObjectID> BunkerIDs = tunnelTracker->getBunkerIDs();
	Bool isInBunkerList;

	Object *obj = getObject();
	
	// Check for upgrades needed for the Contained to Open Fire
	std::vector<AsciiString>::const_iterator it;
	for( it = modData->m_activationUpgradeNames.begin();
				it != modData->m_activationUpgradeNames.end();
				it++)
	{
		const UpgradeTemplate* ut = TheUpgradeCenter->findUpgrade( *it );
		if( !ut )
		{
			DEBUG_CRASH(("An upgrade module references '%s', which is not an Upgrade", it->str()));
			throw INI_INVALID_DATA;
		}
		else if(modData->m_removeOtherUpgrades && ut->getUpgradeType() == UPGRADE_TYPE_PLAYER)
		{
			DEBUG_CRASH(("Upgrade '%s', is not an Object Upgrade. Not compatible with RemoveOtherOpenContainOnUpgrade feature.", it->str()));
		}

		if( obj->hasUpgrade(ut) )
		{
			// Check if it's in the list, if not, assign it to the list.
			for(std::vector<ObjectID>::const_iterator it2 = BunkerIDs.begin(); it2 != BunkerIDs.end(); ++it2)
			{
				if((*it2) == obj->getID())
				{
					isInBunkerList = TRUE;
				}
				else if( modData->m_removeOtherUpgrades )
				{
					Object *other = NULL;
					other = TheGameLogic->findObjectByID( (*it2) );
					if(other && other->hasUpgrade(ut))
					{
						other->removeUpgrade(ut);
					}
				}
			}
		}
	}
	if(modData->m_removeOtherUpgrades && ( (!isInBunkerList && !BunkerIDs.empty()) || (isInBunkerList && BunkerIDs.size()>1) ) )
	{
		tunnelTracker->clearAllOtherBunkerIDs(obj->getID());
	}
	else if(!isInBunkerList)
	{
		tunnelTracker->registerBunkerID(obj->getID());
	}
	// For switching all the units onto its attacking position
	doOpenFire();
}

//-------------------------------------------------------------------------------------------------
Bool TunnelContain::isValidContainerFor(const Object* obj, Bool checkCapacity) const
{
	const Object *us = getObject();
	const TunnelContainModuleData *modData = getTunnelContainModuleData();

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
	
	Player *owningPlayer = getObject()->getControllingPlayer();
	if( owningPlayer && owningPlayer->getTunnelSystem() )
	{
		return owningPlayer->getTunnelSystem()->isValidContainerFor( obj, checkCapacity );
	}
	return false;
}

UnsignedInt TunnelContain::getContainCount() const
{
	Player *owningPlayer = getObject()->getControllingPlayer();
	if( owningPlayer && owningPlayer->getTunnelSystem() )
	{
		return owningPlayer->getTunnelSystem()->getContainCount();
	}
	return 0;
}

Int TunnelContain::getContainMax( void ) const
{
	Player *owningPlayer = getObject()->getControllingPlayer();
	if( owningPlayer && owningPlayer->getTunnelSystem() )
	{
		return owningPlayer->getTunnelSystem()->getContainMax();
	}
	return 0;
}

const ContainedItemsList* TunnelContain::getContainedItemsList() const
{
	Player *owningPlayer = getObject()->getControllingPlayer();
	if( owningPlayer && owningPlayer->getTunnelSystem() )
	{
		return owningPlayer->getTunnelSystem()->getContainedItemsList();
	}
	return NULL;
}



///////////////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS //////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////


//-------------------------------------------------------------------------------------------------
void TunnelContain::scatterToNearbyPosition(Object* obj)
{
	Object *theContainer = getObject();

	//
	// for now we will just set the position of the object that is being removed from us
	// at a random angle away from our center out some distance
	//

	//
	// pick an angle that is in the view of the current camera position so that
	// the thing will come out "toward" the player and they can see it
	// NOPE, can't do that ... all players screen angles will be different, unless
	// we maintain the angle of each players screen in the player structure or something
	//
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
	obj->setOrientation( angle );

	AIUpdateInterface *ai = obj->getAIUpdateInterface();
	if( ai )
	{
		// set position of the object at center of building and move them toward pos
		obj->setPosition( theContainer->getPosition() );
		ai->ignoreObstacle(theContainer);
 		ai->aiMoveToPosition( &pos, CMD_FROM_AI );

	}  // end if
	else
	{

		// no ai, just set position at the target pos
		obj->setPosition( &pos );

	}  // end else
}

//-------------------------------------------------------------------------------------------------
/** The die callback. */
//-------------------------------------------------------------------------------------------------
void TunnelContain::onDie( const DamageInfo * damageInfo )
{
	// override the onDie we inherit from OpenContain. no super call.
	if (!getTunnelContainModuleData()->m_dieMuxData.isDieApplicable(getObject(), damageInfo))
		return;

	if( !m_isCurrentlyRegistered )
		return;//it isn't registered as a tunnel

	Player *owningPlayer = getObject()->getControllingPlayer();
	if( owningPlayer == NULL )
		return;
	TunnelTracker *tunnelTracker = owningPlayer->getTunnelSystem();
	if( tunnelTracker == NULL )
		return;

	tunnelTracker->onTunnelDestroyed( getObject() );
	m_isCurrentlyRegistered = FALSE;
}

//-------------------------------------------------------------------------------------------------
void TunnelContain::onDelete( void )
{
	// Being sold is a straight up delete.  no death

	if( !m_isCurrentlyRegistered )
		return;//it isn't registered as a tunnel

	Player *owningPlayer = getObject()->getControllingPlayer();
	if( owningPlayer == NULL )
		return;
	TunnelTracker *tunnelTracker = owningPlayer->getTunnelSystem();
	if( tunnelTracker == NULL )
		return;

	tunnelTracker->onTunnelDestroyed( getObject() );
	m_isCurrentlyRegistered = FALSE;
}

//-------------------------------------------------------------------------------------------------
void TunnelContain::onCreate( void )
{
}

//-------------------------------------------------------------------------------------------------
void TunnelContain::onObjectCreated()
{
	//Kris: July 29, 2003
	//Added this function to support the sneak attack (which doesn't call onBuildComplete).
	if( ! shouldDoOnBuildComplete() )
		return;

	m_needToRunOnBuildComplete = false;

	Player *owningPlayer = getObject()->getControllingPlayer();
	if( owningPlayer == NULL )
		return;
	TunnelTracker *tunnelTracker = owningPlayer->getTunnelSystem();
	if( tunnelTracker == NULL )
		return;

	tunnelTracker->onTunnelCreated( getObject() );
	m_isCurrentlyRegistered = TRUE;
}

//-------------------------------------------------------------------------------------------------
void TunnelContain::onBuildComplete( void )
{
	//Kris: July 29, 2003
	//Obsolete -- onObjectCreated handles it before this function gets called.
	/*
	if( ! shouldDoOnBuildComplete() )
		return;

	m_needToRunOnBuildComplete = false;

	Player *owningPlayer = getObject()->getControllingPlayer();
	if( owningPlayer == NULL )
		return;
	TunnelTracker *tunnelTracker = owningPlayer->getTunnelSystem();
	if( tunnelTracker == NULL )
		return;

	tunnelTracker->onTunnelCreated( getObject() );
	m_isCurrentlyRegistered = TRUE;
	*/
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void TunnelContain::onCapture( Player *oldOwner, Player *newOwner )
{
	if( m_isCurrentlyRegistered )
	{
		TunnelTracker *oldTunnelTracker = oldOwner->getTunnelSystem();
		if( oldTunnelTracker )
		{
			DEBUG_ASSERTCRASH( oldTunnelTracker->getContainCount() == 0, ("You shouldn't force a capture of a Tunnel with people in it. Future ExitFromContainer scripts will fail."));
			oldTunnelTracker->onTunnelDestroyed(getObject());
		}

		TunnelTracker *newTunnelTracker = newOwner->getTunnelSystem();
		if( newTunnelTracker )
		{
			newTunnelTracker->onTunnelCreated(getObject());
		}
	}

	// extend base class
	OpenContain::onCapture( oldOwner, newOwner );
}

//-------------------------------------------------------------------------------------------------
void TunnelContain::orderAllPassengersToExit( CommandSourceType commandSource, Bool instantly )
{
	Player *owningPlayer = getObject()->getControllingPlayer();
	if( !owningPlayer || !owningPlayer->getTunnelSystem() )
		return;

	OpenContain::orderAllPassengersToExit( commandSource, instantly );
}

//-------------------------------------------------------------------------------------------------
void TunnelContain::orderAllPassengersToIdle( CommandSourceType commandSource )
{
	Player *owningPlayer = getObject()->getControllingPlayer();
	if( !owningPlayer || !owningPlayer->getTunnelSystem() )
		return;

	OpenContain::orderAllPassengersToIdle( commandSource );
}

// ------------------------------------------------------------------------------------------------
/** Per frame update */
// ------------------------------------------------------------------------------------------------
UpdateSleepTime TunnelContain::update( void )
{
	// extending functionality to heal the units within the tunnel system
	OpenContain::update();
	const TunnelContainModuleData *modData = getTunnelContainModuleData();

	Object *obj = getObject();
	Player *controllingPlayer = NULL;
	if (obj)
	{
		controllingPlayer = obj->getControllingPlayer();
	}
	if (controllingPlayer)
	{
		TunnelTracker *tunnelSystem = controllingPlayer->getTunnelSystem();
		if (tunnelSystem)
		{
			tunnelSystem->healObjects(modData->m_framesForFullHeal);
		}

		// Check if the Tunnel has OpenContained Upgrade enabled. If so, skip updateNemesis
		if(modData->m_passengersAllowedToFire)
		{
			if(!modData->m_activationUpgradeNames.empty())
			{
				std::vector<AsciiString>::const_iterator it;
				for( it = modData->m_activationUpgradeNames.begin();
							it != modData->m_activationUpgradeNames.end();
							it++)
				{
					const UpgradeTemplate* ut = TheUpgradeCenter->findUpgrade( *it );
					if( !ut )
					{
						DEBUG_CRASH(("An upgrade module references '%s', which is not an Upgrade", it->str()));
						throw INI_INVALID_DATA;
					}

					if( obj->hasUpgrade(ut) )
					{
						if(obj->testStatus( OBJECT_STATUS_IS_ATTACKING ))
							doOpenFire();
						return UPDATE_SLEEP_NONE;
					}
				}
			}
			else
			{
				if(obj->testStatus( OBJECT_STATUS_IS_ATTACKING ))
					doOpenFire();
				return UPDATE_SLEEP_NONE;
			}
		}
		// check for attacked.
		BodyModuleInterface *body = obj->getBodyModule();
		if (body) {
			const DamageInfo *info = body->getLastDamageInfo();
			if (info) {
				if (body->getLastDamageTimestamp() + LOGICFRAMES_PER_SECOND > TheGameLogic->getFrame()) {
					// winner.
					ObjectID attackerID = info->in.m_sourceID;
					Object *attacker = TheGameLogic->findObjectByID(attackerID);
					if( attacker )
					{
						if (obj->getRelationship(attacker) == ENEMIES) {
							tunnelSystem->updateNemesis(attacker);
						}
					}
				}
			}
		}
	}
	return UPDATE_SLEEP_NONE;

}

void TunnelContain::doOpenFire()
{
	UnsignedInt now = TheGameLogic->getFrame();
	
	if(now < m_checkAttackFrames)
		return;

	// Check once every 0.1s to prevent constant switching confusion.
	m_checkAttackFrames = now + 3*LOGICFRAMES_PER_SECOND;

	Object *me = getObject();
	Player *owningPlayer = me->getControllingPlayer();

	if(!owningPlayer->getTunnelSystem())
		return;

	// Redirect the units onto another Tunnel
	ContainedItemsList list;
	owningPlayer->getTunnelSystem()->swapContainedItemsList(list);

	ContainedItemsList::iterator it = list.begin();
	std::vector<ObjectID>vecID;

	while ( it != list.end() )
	{
		Object *obj = *it++;
		DEBUG_ASSERTCRASH( obj, ("Contain list must not contain NULL element"));

		//if(obj->getContainedBy() == me)
		//	return;

		removeFromContain( obj, false );

		vecID.push_back(obj->getID());
	}
	Object *nemesis = NULL;
	if(me->getAI() && me->getAI()->getGoalObject())
	{
		nemesis = me->getAI()->getGoalObject();
	}

	for(int i = 0; i < vecID.size(); i++)
	{
		Object *add = TheGameLogic->findObjectByID( vecID[i] );
		if(add)
		{
			ContainModuleInterface *contain = me->getContain();

			if( contain )
			{
				contain->addToContain(add);
			}

			if(add->isKindOf( KINDOF_CAN_ATTACK) && add->getAI())
			{
				if(nemesis)
				{
					add->getAI()->friend_setGoalObject( nemesis );
				}
				//else if(me->getAI() && me->getAI()->getGoalPosition())
				//{
				//	add->getAI()->friend_setGoalPath(me->getAI()->getGoalPosition());
				//}
			}

		}
	}
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void TunnelContain::crc( Xfer *xfer )
{

	// extend base class
	OpenContain::crc( xfer );

}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void TunnelContain::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	OpenContain::xfer( xfer );

	// need to run on build complete
	xfer->xferBool( &m_needToRunOnBuildComplete );

	// Currently registered with owning player
	xfer->xferBool( &m_isCurrentlyRegistered );

}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void TunnelContain::loadPostProcess( void )
{

	// extend base class
	OpenContain::loadPostProcess();

}  // end loadPostProcess

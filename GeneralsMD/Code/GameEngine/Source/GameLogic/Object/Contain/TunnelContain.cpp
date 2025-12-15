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
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "Common/Player.h"
#include "Common/RandomValue.h"
#include "Common/ThingFactory.h"
#include "Common/ThingTemplate.h"
#include "Common/TunnelTracker.h"
#include "Common/Xfer.h"
#include "GameClient/Drawable.h"
#include "GameLogic/Module/AIUpdate.h"
#include "GameLogic/Module/OpenContain.h"
#include "GameLogic/Module/TunnelContain.h"
#include "GameLogic/Module/PassengersFireUpgrade.h"
#include "GameLogic/Object.h"
#include "GameLogic/PartitionManager.h"

class PassengersFireUpgrade;

///////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS ///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
TunnelContain::TunnelContain( Thing *thing, const ModuleData* moduleData ) : OpenContain( thing, moduleData )
{
	m_needToRunOnBuildComplete = true;
	m_isCurrentlyRegistered = FALSE;
	m_lastFiringObjID = INVALID_ID;
	m_lastFiringPos.zero();
	m_rebuildChecked = TRUE;

	if(getObject() && 
	   getObject()->getControllingPlayer() && 
	   getObject()->getControllingPlayer()->getTunnelSystem() && 
	  !getObject()->getControllingPlayer()->getTunnelSystem()->getOtherTunnelsGuardDisabled())
		m_hasTunnelGuard = TRUE;
	else
		m_hasTunnelGuard = FALSE;

	//if(getTunnelContainModuleData()->m_activationUpgradeNames.empty())
		//setPassengerAllowedToFire( getTunnelContainModuleData()->m_passengersAllowedToFire );
	//else
	//	setPassengerAllowedToFire( FALSE );
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
	}

}


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
#if RETAIL_COMPATIBLE_CRC
	ThePartitionManager->registerObject( obj );
	obj->setPosition( getObject()->getPosition() );
	if( obj->getDrawable() )
	{
		obj->setSafeOcclusionFrame(TheGameLogic->getFrame()+obj->getTemplate()->getOcclusionDelay());
		obj->getDrawable()->setDrawableHidden( false );
	}
#else
	// TheSuperHackers @bugfix Now correctly adds the objects to the world without issues with shrouded portable structures.
	obj->setPosition(getObject()->getPosition());
	obj->setSafeOcclusionFrame(TheGameLogic->getFrame() + obj->getTemplate()->getOcclusionDelay());
	addOrRemoveObjFromWorld(obj, TRUE);
#endif

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

	if( ! hasPassengerAllowedToFire() )
		return FALSE;// Just no, no matter what.
	
	Player *owningPlayer = getObject()->getControllingPlayer();
	if( owningPlayer == NULL )
		return FALSE;
	TunnelTracker *tunnelTracker = owningPlayer->getTunnelSystem();
	if( tunnelTracker == NULL )
		return FALSE;

	const TunnelContainModuleData *modData = getTunnelContainModuleData();
	const Object *me = getObject();

	// ECM Properties
	if ( me->isDisabledByType( DISABLED_SUBDUED ) || me->isDisabledByType( DISABLED_FROZEN ) )
		return FALSE;

	// Don't make it applicable if I'm Under Construction (Or being a Hole)
	if( me->testStatus( OBJECT_STATUS_UNDER_CONSTRUCTION ) )
		return FALSE;

	// Check for upgrades needed for the Contained to Open Fire
	/*if(!modData->m_activationUpgradeNames.empty())
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
	}*/

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

	return OpenContain::isPassengerAllowedToFire();
}


/////////////////////////////////////////////////////////////////////////////////////
static TunnelInterface* findTunnel(Object* obj)
{
	for (BehaviorModule** i = obj->getBehaviorModules(); *i; ++i)
	{
		TunnelInterface* t = (*i)->getTunnelInterface();
		if (t != NULL)
			return t;
	}
	return NULL;
}

/////////////////////////////////////////////////////////////////////////////////////
void TunnelContain::doUpgradeChecks( void )
{
	if ( !getObject() )
    	return;

	// extend base class
	OpenContain::doUpgradeChecks();

	Player *owningPlayer = getObject()->getControllingPlayer();
	if( owningPlayer == NULL )
		return;
	TunnelTracker *tunnelTracker = owningPlayer->getTunnelSystem();
	if( tunnelTracker == NULL )
		return;

	//const TunnelContainModuleData *modData = getTunnelContainModuleData();

	// Check for Tunnel Guard Designation Upgrades
	checkRemoveOtherGuard();
	
	// Check for self Guard Disabling Upgrades
	checkRemoveOwnGuard();

	// Only enable Fire Ports if the Open Fire feature is configured.
	/*if(!modData->m_passengersAllowedToFire)
		return;

	// Check for Bunker enabling Upgrades.
	if(modData->m_activationUpgradeNames.empty())
		return;

	Object *obj = getObject();
	const std::list<ObjectID> *tunnels = tunnelTracker->getContainerList();

	// Reset existing properties to check whether it has the appropriate upgrades to apply.
	setPassengerAllowedToFire( FALSE );
	
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
			// Assign the bunker according to the list of upgrades.
			for( std::list<ObjectID>::const_iterator iter = tunnels->begin(); iter != tunnels->end(); iter++ )
			{
				if( *iter == obj->getID())
				{
					setPassengerAllowedToFire( TRUE );
				}
				// If we want to assign only 1 fireable tunnel, remove other upgrades
				else if( modData->m_removeOtherUpgrades )
				{
					Object *other = NULL;
					other = TheGameLogic->findObjectByID( (*iter) );
					if( other )
					{
						if(other->hasUpgrade(ut))
							other->removeUpgrade(ut);
						TunnelInterface *tunnelModule = findTunnel(other);
						if( tunnelModule == NULL )
							continue;
						tunnelModule->removeBunker();
					}
				}
			}
			break;
		}
	
	}*/

	// For switching all the units onto its attacking position
	if(hasPassengerAllowedToFire())
	{
		// If we want to assign only 1 fireable tunnel, remove other ability for passengers allowed to fire
		if(getTunnelContainModuleData()->m_removeOtherPassengersAllowToFire)
		{
			static const NameKeyType key_PassengersFireUpgrade = NAMEKEY("PassengersFireUpgrade");
			PassengersFireUpgrade* pfu = (PassengersFireUpgrade*)(getObject()->findUpdateModule( key_PassengersFireUpgrade ));
			if (pfu)
			{
				std::vector<AsciiString> UpgradeNames = pfu->getActivationUpgradeNames();

				if(!UpgradeNames.empty())
					doRemoveOtherPassengersAllowToFire();
			}
		}
		doOpenFire(FALSE);
	}
	else
	{
		removeBunker();
	}
}

//-------------------------------------------------------------------------------------------------
void TunnelContain::doRemoveOtherPassengersAllowToFire()
{
	Player *owningPlayer = getObject()->getControllingPlayer();
	if( owningPlayer == NULL )
		return;
	TunnelTracker *tunnelTracker = owningPlayer->getTunnelSystem();
	if( tunnelTracker == NULL )
		return;

	static const NameKeyType key_PassengersFireUpgrade = NAMEKEY("PassengersFireUpgrade");
	const std::list<ObjectID> *tunnels = tunnelTracker->getContainerList();
	for( std::list<ObjectID>::const_iterator iter = tunnels->begin(); iter != tunnels->end(); iter++ )
	{
		if( !(*iter == getObject()->getID()) )
		{
			Object *other = NULL;
			other = TheGameLogic->findObjectByID( (*iter) );
			if( other )
			{
				// Set other Tunnels to not allow Passengers to Fire
				if(other->getContain())
					other->getContain()->setPassengerAllowedToFire( FALSE );
				TunnelInterface *tunnelModule = findTunnel(other);
				if( tunnelModule == NULL )
					continue;
				tunnelModule->removeBunker();

				PassengersFireUpgrade* pfu = (PassengersFireUpgrade*)(other->findUpdateModule( key_PassengersFireUpgrade ));
				if (pfu)
				{
					std::vector<AsciiString> UpgradeNames = pfu->getActivationUpgradeNames();

					//Remove the Upgrade from Passengers Fire Upgrade
					std::vector<AsciiString>::const_iterator it;
					for( it = UpgradeNames.begin();
								it != UpgradeNames.end();
								it++)
					{
						const UpgradeTemplate* ut = TheUpgradeCenter->findUpgrade( *it );
						if( !ut )
						{
							DEBUG_CRASH(("An upgrade module references '%s', which is not an Upgrade", it->str()));
							throw INI_INVALID_DATA;
						}
						else if(ut->getUpgradeType() == UPGRADE_TYPE_PLAYER)
						{
							DEBUG_CRASH(("Upgrade '%s', is not an Object Upgrade. Not compatible with RemoveOtherOpenContainOnUpgrade feature.", it->str()));
							throw INI_INVALID_DATA;
						}

						if( other->hasUpgrade(ut) )
						{
							other->removeUpgrade( ut );
						}
					}
				}
			}
		}
	}
}

//-------------------------------------------------------------------------------------------------
/*void TunnelContain::doHoleRebuildChecks()
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
	{
		setPassengerAllowedToFire( TRUE );
		return;
	}
	
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

		if( getObject()->hasUpgrade(ut) )
		{
			setPassengerAllowedToFire( TRUE );
			break;
		}
	
	}
}*/

//-------------------------------------------------------------------------------------------------
Bool TunnelContain::isValidContainerFor(const Object* obj, Bool checkCapacity) const
{
	// extend functionality
	if( OpenContain::isValidContainerFor( obj, checkCapacity ) == false )
		return false;
	
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

// No implementation for Slot expansion... yet
Int TunnelContain::getRawContainMax( void ) const
{
	Player *owningPlayer = getObject()->getControllingPlayer();
	if( owningPlayer && owningPlayer->getTunnelSystem() )
	{
		return owningPlayer->getTunnelSystem()->getContainMax();
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

UnsignedInt TunnelContain::getFullTimeForHeal(void) const
{
	const TunnelContainModuleData* modData = getTunnelContainModuleData();
	return modData->m_framesForFullHeal;
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

	}
	else
	{

		// no ai, just set position at the target pos
		obj->setPosition( &pos );

	}
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

	// Triggers when you turn into a hole
	m_rebuildChecked = FALSE;
	removeBunker();
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

	m_rebuildChecked = FALSE;
	removeBunker();
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

	Object *obj = getObject();
	Player *controllingPlayer = NULL;
	if (obj)
	{
		controllingPlayer = obj->getControllingPlayer();
	}
	if (controllingPlayer)
	{
		TunnelTracker *tunnelSystem = controllingPlayer->getTunnelSystem();
		const TunnelContainModuleData* modData = getTunnelContainModuleData();

		if (tunnelSystem)
		{
#if PRESERVE_RETAIL_BEHAVIOR || RETAIL_COMPATIBLE_CRC
			tunnelSystem->healObjects(modData->m_framesForFullHeal);
#endif
			tunnelSystem->removeDontLoadSound(TheGameLogic->getFrame());
			if(!m_payloadCreated && !obj->isEffectivelyDead() && !obj->testStatus( OBJECT_STATUS_UNDER_CONSTRUCTION ) && !obj->testStatus( OBJECT_STATUS_RECONSTRUCTING ))
				createPayload();
		}

		Bool openFireCheck;

		// Check if the Tunnel has OpenContained Upgrade enabled. If so, skip updateNemesis
		// Also don't check for Dead Tunnels, or Tunnels that are Holes.
		if(hasPassengerAllowedToFire() && !obj->isEffectivelyDead() && !obj->testStatus( OBJECT_STATUS_UNDER_CONSTRUCTION ) && !obj->testStatus( OBJECT_STATUS_RECONSTRUCTING ))
		{
			//Bool openFireUpgrade = TRUE;

			if(obj->testStatus( OBJECT_STATUS_IS_ATTACKING ) || obj->testStatus( OBJECT_STATUS_IS_FIRING_WEAPON ) || obj->testStatus( OBJECT_STATUS_IS_AIMING_WEAPON ) || obj->testStatus( OBJECT_STATUS_IGNORING_STEALTH ))
				openFireCheck = TRUE;

			/*if(!modData->m_activationUpgradeNames.empty() && openFireCheck)
			{
				openFireCheck = FALSE;
				openFireUpgrade = FALSE;

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
						openFireCheck = TRUE;
						openFireUpgrade = TRUE;
						break;
					}
				}

				// If it doesn't have the upgrade, remove it's Fire Port ability
				if(!openFireCheck)
				{
					removeBunker();
				}
			}*/

			if(openFireCheck)
			{
				ObjectID currFiringObjID = INVALID_ID;
				Coord3D currFiringPos;
				
				if(obj->getAI() )
				{
					currFiringPos = *obj->getAI()->getGoalPosition();
					if(obj->getAI()->getGoalObject())
						currFiringObjID = obj->getAI()->getGoalObject()->getID();
				}
				if( ( currFiringObjID != INVALID_ID && m_lastFiringObjID != currFiringObjID ) || ( currFiringObjID == INVALID_ID && m_lastFiringPos != currFiringPos ) )
					doOpenFire();
			}

			//if(openFireUpgrade)
				return UPDATE_SLEEP_NONE;
		}
		if (!hasPassengerAllowedToFire() && modData->m_passengersAllowedToFire && !m_rebuildChecked && !obj->testStatus( OBJECT_STATUS_UNDER_CONSTRUCTION ) && !obj->testStatus( OBJECT_STATUS_RECONSTRUCTING ))
		{
			//doHoleRebuildChecks();
			m_rebuildChecked = TRUE;
		}
		// Conditional Statements tested.
		if(!m_hasTunnelGuard)
			return UPDATE_SLEEP_NONE;
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


struct NemesisHolder
{
	Object *Nemesis;
};

static void attackAtMyTarget( Object *obj, void *nemesisHolder )
{
	if (!obj || ((NemesisHolder*)nemesisHolder)->Nemesis == NULL)
	{
		return;
	}

	if(obj->isKindOf( KINDOF_CAN_ATTACK) && obj->getAI())
	{
		const Weapon* weapon = obj->getCurrentWeapon();
		if (weapon && weapon->isWithinAttackRange(obj, ((NemesisHolder*)nemesisHolder)->Nemesis ))
		{
			obj->getAI()->friend_setGoalObject( ((NemesisHolder*)nemesisHolder)->Nemesis );
		}
	}
}

void TunnelContain::doOpenFire(Bool isAttacking)
{
	Object *me = getObject();
	Player *owningPlayer = me->getControllingPlayer();

	if( owningPlayer == NULL )
		return;
	TunnelTracker *tunnelTracker = owningPlayer->getTunnelSystem();
	if( tunnelTracker == NULL )
		return;

	UnsignedInt now = TheGameLogic->getFrame();

	if(now < tunnelTracker->getCheckOpenFireFrames())
		return;

	// Set it after a while so units doesn't get confused every now and then if multiple tunnels are attacking
	tunnelTracker->setCheckOpenFireFrames(now + LOGICFRAMES_PER_SECOND + 1);

	Bool changeTunnels = FALSE;
	
	ContainModuleInterface *contain = me->getContain();

	// Check if the Occupants are within this Tunnel. If yes, only do the targeting and don't need to change tunnels.
	// ...This first statement is a sanity check.
	if(contain)
	{
		const ContainedItemsList* items = tunnelTracker->getContainedItemsList();
		
		// Do change only if there's nothing in the Tunnel System.
		if(!items->empty())
		{
			// Check if this is the Tunnel where the object is currently at, change if there is one that is not in the current Object
			ContainedItemsList::const_iterator it_test = items->begin();
			while ( !changeTunnels && it_test != items->end() )
			{
				Object *test_obj = *it_test++;
				if( test_obj->getContainedBy() != me )
					changeTunnels = TRUE;
			}
		}
	}

	// Get the Object that is is attacking and redirects all Units within the Tunnel Network onto it.
	Object *nemesis = NULL;
	AIUpdateInterface *ai = me->getAI();
	if(ai && isAttacking)
	{
		nemesis = ai->getGoalObject();

		m_lastFiringPos = *ai->getGoalPosition();
		if(nemesis)
			m_lastFiringObjID = nemesis->getID();
	}

	if(!changeTunnels)
	{
		// Direct the current units in the current tunnel to attack the target Object
		if(nemesis)
		{
			NemesisHolder nemesisHolder;
			nemesisHolder.Nemesis = nemesis;
			tunnelTracker->iterateContained( attackAtMyTarget, (void*)&nemesisHolder, FALSE );
		}

		return;
	}

	// Disable the Garrison Sound First
	tunnelTracker->setDontLoadSound(now + LOGICFRAMES_PER_SECOND);
	
	// Redirect the units onto another Tunnel by Re-Garrisoning them.
	ContainedItemsList list;
	tunnelTracker->swapContainedItemsList(list);
	
	std::vector<ObjectID>vecID;

	ContainedItemsList::iterator it = list.begin();
	while ( it != list.end() )
	{
		Object *obj = *it++;
		DEBUG_ASSERTCRASH( obj, ("Contain list must not contain NULL element"));

		removeFromContain( obj, false );

		vecID.push_back(obj->getID());
	}

	for(int i = 0; i < vecID.size(); i++)
	{
		Object *add = TheGameLogic->findObjectByID( vecID[i] );
		if(add)
		{
			if( contain )
			{
				contain->addToContain(add);
			}

			if(nemesis && add->isKindOf( KINDOF_CAN_ATTACK) && add->getAI())
			{
				const Weapon* weapon = add->getCurrentWeapon();
				if (weapon && weapon->isWithinAttackRange(add, nemesis))
				{
					add->getAI()->friend_setGoalObject( nemesis );
				}
			}

		}
	}
}

// Remove my Fire Ports
void TunnelContain::removeBunker()
{ 
	// Sanity checks
	Object *me = getObject();
	Player *owningPlayer = me->getControllingPlayer();

	if( owningPlayer == NULL )
		return;
	TunnelTracker *tunnelTracker = owningPlayer->getTunnelSystem();
	if( tunnelTracker == NULL )
		return;

	// Disable the main function
	setPassengerAllowedToFire( FALSE );

	// If I am occupied, tell everyone in my current Tunnel to stop Firing
	const ContainedItemsList* items = tunnelTracker->getContainedItemsList();
	
	if(!items->empty())
	{
		// Iterate the Units in the contain and tell them to idle
		ContainedItemsList::const_iterator it_test = items->begin();
		while ( it_test != items->end() )
		{
			Object *test_obj = *it_test++;
			if( test_obj->getAI() && ( test_obj->getContainedBy() == me || !m_rebuildChecked ))
				test_obj->getAI()->aiIdle(CMD_FROM_AI);
		}
	}
}

// Function to remove its own Tunnel Guard to prevent Units to exit from the Tunnel to protect the Tunnel
void TunnelContain::checkRemoveOwnGuard()
{
	const TunnelContainModuleData *modData = getTunnelContainModuleData();

	// There's no variable that enables this feature, end
	if(modData->m_upgradeDisableOwnNames.empty())
		return;

	Object *obj = getObject();

	// Sanity Checks
	Player *owningPlayer = obj->getControllingPlayer();
	if( owningPlayer == NULL )
		return;
	TunnelTracker *tunnelTracker = owningPlayer->getTunnelSystem();
	if( tunnelTracker == NULL )
		return;
	
	// If the whole Tunnel System is affected by UpgradesDisableOtherTunnelGuard, do not reset Tunnel Guard if turned off
	if(!tunnelTracker->getOtherTunnelsGuardDisabled())
		m_hasTunnelGuard = TRUE;

	// Already disabled Tunnel Guard
	if(!m_hasTunnelGuard)
		return;

	// Scan for any Upgrades that disables Tunnel Guard
	std::vector<AsciiString>::const_iterator it_o;
	for( it_o = modData->m_upgradeDisableOwnNames.begin();
				it_o != modData->m_upgradeDisableOwnNames.end();
				it_o++)
	{
		const UpgradeTemplate* ut = TheUpgradeCenter->findUpgrade( *it_o );
		if( !ut )
		{
			DEBUG_CRASH(("An upgrade module references '%s', which is not an Upgrade", it_o->str()));
			throw INI_INVALID_DATA;
		}

		// One Upgrade is enough
		if( getObject()->hasUpgrade(ut) )
		{
			removeGuard();
			break;
		}
	}
		
}

// Function to make this Tunnel the only focus for Tunnel Guard and Disables other Tunnels from performing Tunnel Guard
void TunnelContain::checkRemoveOtherGuard()
{
	const TunnelContainModuleData *modData = getTunnelContainModuleData();

	// There's no variable that enables this feature, end
	if(modData->m_upgradeDisableOtherNames.empty())
		return;

	Object *obj = getObject();

	// Sanity Checks
	Player *owningPlayer = obj->getControllingPlayer();
	if( owningPlayer == NULL )
		return;
	TunnelTracker *tunnelTracker = owningPlayer->getTunnelSystem();
	if( tunnelTracker == NULL )
		return;

	// Remove the limitation that disables other Tunnels from turning back on their Tunnel Guard
	tunnelTracker->setOtherTunnelsGuardDisabled(FALSE);

	if(!modData->m_upgradeDisableOtherNames.empty())
	{
		const std::list<ObjectID> *tunnels = tunnelTracker->getContainerList();

		std::vector<AsciiString>::const_iterator it_n;
		for( it_n = modData->m_upgradeDisableOtherNames.begin();
					it_n != modData->m_upgradeDisableOtherNames.end();
					it_n++)
		{
			const UpgradeTemplate* ut = TheUpgradeCenter->findUpgrade( *it_n );
			if( !ut )
			{
				DEBUG_CRASH(("An upgrade module references '%s', which is not an Upgrade", it_n->str()));
				throw INI_INVALID_DATA;
			}

			if( obj->hasUpgrade(ut) )
			{
				// Designate which tunnel Guard according to the list of upgrades.
				for( std::list<ObjectID>::const_iterator iter = tunnels->begin(); iter != tunnels->end(); iter++ )
				{
					if( *iter == obj->getID())
					{
						// Turn on the Tunnel Guard in case it is off
						m_hasTunnelGuard = TRUE;

						// Sets the limitation that disables other Tunnels from turning on their Tunnel Guard, since there can be only one Tunnel that is focused for this function
						tunnelTracker->setOtherTunnelsGuardDisabled(TRUE);
					}
					else
					{
						// Now to turn off other Tunnels' Tunnel Guard
						Object *other = NULL;
						other = TheGameLogic->findObjectByID( (*iter) );
						if( other )
						{
							// Remove Upgrades from other Tunnels, as similar types possesses the same feature that enables all tunnels to focus towards them.
							if(other->hasUpgrade(ut))
								other->removeUpgrade(ut);

							// Turn off the Tunnel Guard
							TunnelInterface *tunnelModule = findTunnel(other);
							if( tunnelModule == NULL )
								continue;
							tunnelModule->removeGuard();
						}
					}
				}
				// Only one upgrade is enough
				break;
			}
		}
	}
}

void TunnelContain::createPayload()
{
	// Payload System
	if(m_payloadCreated)
		return;

	// A TunnelContain tells everyone to leave if this is the last tunnel
	Player *owningPlayer = getObject()->getControllingPlayer();
	if( owningPlayer == NULL )
		return;
	TunnelTracker *tunnelTracker = owningPlayer->getTunnelSystem();
	if( tunnelTracker == NULL )
		return;

	ContainModuleInterface *contain = getObject()->getContain();
	if(contain && tunnelTracker)
	{
		contain->enableLoadSounds( FALSE );

		Int count = getTunnelContainModuleData()->m_initialPayload.count;

		for( int i = 0; i < count; i++ )
		{
			//We are creating a transport that comes with a initial payload, so add it now!
			const ThingTemplate* payloadTemplate = TheThingFactory->findTemplate( getTunnelContainModuleData()->m_initialPayload.name );
			Object* payload = TheThingFactory->newObject( payloadTemplate, getObject()->getTeam() );
			if( tunnelTracker->isValidContainerFor( payload, true ) )
			{
				payload->setPosition( getObject()->getPosition() );
				contain->addToContain( payload );
			}
			else
			{
				scatterToNearbyPosition( payload );
			}
		}

		contain->enableLoadSounds( TRUE );
	}
	
	m_payloadCreated = TRUE;
}
// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void TunnelContain::crc( Xfer *xfer )
{

	// extend base class
	OpenContain::crc( xfer );

}

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

	xfer->xferBool( &m_hasTunnelGuard );

	xfer->xferBool( &m_rebuildChecked );

	xfer->xferBool( &m_payloadCreated );

}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void TunnelContain::loadPostProcess( void )
{

	// extend base class
	OpenContain::loadPostProcess();

}

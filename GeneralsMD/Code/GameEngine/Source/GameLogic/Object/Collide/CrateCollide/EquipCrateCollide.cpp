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

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// FILE: EquipCrateCollide.cpp
// Author: IamInnocent, September 2025
// Desc:   A crate module that makes the makes the user to latch onto a target
//				 granting either positive and negative attributes
//				 such as granting bonuses to allies, or dealing DoT on enemies
//
///////////////////////////////////////////////////////////////////////////////////////////////////



// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file int the GameEngine

#include "Common/ActionManager.h"
#include "Common/Player.h"
#include "Common/PlayerList.h"
#include "Common/Radar.h"
#include "Common/ThingTemplate.h"
#include "Common/Xfer.h"

#include "GameClient/ControlBar.h"
#include "GameClient/Drawable.h"
#include "GameClient/Eva.h"
#include "GameClient/InGameUI.h"  // useful for printing quick debug strings when we need to

#include "GameLogic/ExperienceTracker.h"
#include "GameLogic/Module/AIUpdate.h"
#include "GameLogic/Module/EquipCrateCollide.h"
#include "GameLogic/Module/HijackerUpdate.h"
#include "GameLogic/Module/ContainModule.h"
#include "GameLogic/Object.h"
#include "GameLogic/PartitionManager.h"
#include "GameLogic/ScriptEngine.h"


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
EquipCrateCollide::EquipCrateCollide( Thing *thing, const ModuleData* moduleData ) : CrateCollide( thing, moduleData )
{
	m_equipToID = INVALID_ID;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
EquipCrateCollide::~EquipCrateCollide()
{
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
Bool EquipCrateCollide::isValidToExecute( const Object *other ) const
{
	// If I am already attached to someone else, don't do anything.
	if( getObject()->getEquipToID() != INVALID_ID )
	{
		return FALSE;
	}

	if( !CrateCollide::isValidToExecute(other) )
	{
		return FALSE;
	}

	if( other->isEffectivelyDead() )
	{
		return FALSE;// can't equip a dead object
	}

	const EquipCrateCollideModuleData* data = getEquipCrateCollideModuleData();
	const Object *obj = getObject();
	CommandSourceType lastCommandSource = obj->getAI() ? obj->getAI()->getLastCommandSource() : CMD_FROM_PLAYER;

	// If we can only consider one equipped object as a time and the object is already equipped, return false
	if(data->m_isUnique && !other->getEquipObjectIDs().empty())
	{
		return FALSE;
	}

	// If we are to contain, but there is no valid container, return false
	if(data->m_isContain)
	{
		if(!other->getContain())
			return FALSE;

		if(obj->isContained() || !TheActionManager->canEnterObject( obj, other, lastCommandSource, CHECK_CAPACITY, FALSE ))
			return FALSE;
	}

	// Don't use Parasite Volunteeringly if the Object is an Ally 
	// Or if the Unit is not attacking while it is able to use weapon against the target
	if( data->m_isParasite && 
		!obj->getParasiteCollideActive() &&
		!TheInGameUI->isInForceAttackMode() &&
		(!TheInGameUI->getGUICommand() || TheInGameUI->getGUICommand()->getCommandType() != GUICOMMANDMODE_EQUIP_OBJECT ))
	{
		if( !obj->testStatus( OBJECT_STATUS_IS_ATTACKING ) &&
			  !obj->testStatus( OBJECT_STATUS_IS_FIRING_WEAPON ) &&
			  !obj->testStatus( OBJECT_STATUS_IS_AIMING_WEAPON ) &&
			  !obj->testStatus( OBJECT_STATUS_IGNORING_STEALTH ))
		  {
			  CanAttackResult result = obj->getAbleToAttackSpecificObject( TheInGameUI->isInForceAttackMode() ? ATTACK_NEW_TARGET_FORCED : ATTACK_NEW_TARGET, other, CMD_FROM_PLAYER, (WeaponSlotType)-1, TRUE );
			  if(( result != ATTACKRESULT_NOT_POSSIBLE && result != ATTACKRESULT_INVALID_SHOT ) ||
			  	   obj->getRelationship(other) == ALLIES ||
				   ( obj->getRelationship(other) != ENEMIES && TheActionManager->canEnterObject( obj, other, lastCommandSource, CHECK_CAPACITY, FALSE ))
		  		)
			  {
				  return FALSE;
			  }
		  }
		else if(obj->getRelationship(other) != ENEMIES && TheActionManager->canEnterObject( obj, other, lastCommandSource, CHECK_CAPACITY, FALSE ))
		  {
			  return FALSE;
		  }
	}

	return TRUE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
Bool EquipCrateCollide::executeCrateBehavior( Object *other )
{
	//Check to make sure that the other object is also the goal object in the AIUpdateInterface
	//in order to prevent an unintentional conversion simply by having the terrorist walk too close
	//to it.
	//Assume ai is valid because CrateCollide::isValidToExecute(other) checks it.
	Object *obj = getObject();
	const EquipCrateCollideModuleData* data = getEquipCrateCollideModuleData();
	AIUpdateInterface* ai = obj->getAIUpdateInterface();
	if (ai && ai->getGoalObject() != other)
	{
		return false;
	}

	// Register the ID, bonuses and status here, since we are also doing contain checks
	setEquipStatus(other);

	// If we are only to contain, we stop here.
	if(data->m_isContain)
	{
		// Sanity
		if(other->getContain())
			other->getContain()->addToContain(obj);
		return FALSE;
	}

	Relationship r = getObject()->getRelationship( other );
	//If we are equipping or 'Parasiting' an enemy, alert them
	if( r == ENEMIES )
	{
		TheRadar->tryUnderAttackEvent( other );
	}

	// I we have made it this far, we are going to ride in this vehicle for a while
	// get the name of the hijackerupdate
	//static NameKeyType key_HijackerUpdate = NAMEKEY( "HijackerUpdate" );
	//HijackerUpdate *hijackerUpdate = (HijackerUpdate*)obj->findUpdateModule( key_HijackerUpdate );
	//if( hijackerUpdate )
	HijackerUpdateInterface *hijackerUpdate = obj->getHijackerUpdateInterface();
	if( hijackerUpdate )
	{
		hijackerUpdate->setTargetObject( other );
		hijackerUpdate->setHijackType( HIJACK_EQUIP );
		hijackerUpdate->setIsInVehicle( TRUE );
		hijackerUpdate->setUpdate( TRUE );
		hijackerUpdate->setNoLeechExp( !data->m_leechExpFromObject );
		hijackerUpdate->setIsParasite( data->m_isParasite );
		if(data->m_isParasite)
		{
			hijackerUpdate->setHijackType( HIJACK_PARASITE );
			hijackerUpdate->setParasiteKey( data->m_parasiteKey );
		}
		hijackerUpdate->setDestroyOnHeal( data->m_destroyOnHeal );
		hijackerUpdate->setRemoveOnHeal( data->m_removeOnHeal );
		hijackerUpdate->setDestroyOnClear( data->m_destroyOnClear );
		hijackerUpdate->setDestroyOnTargetDie( data->m_destroyOnTargetDie );
		hijackerUpdate->setPercentDamage( data->m_damagePercentageToUnit );
		hijackerUpdate->setStatusToRemove( data->m_statusToRemove );
		hijackerUpdate->setStatusToDestroy( data->m_statusToDestroy );
		hijackerUpdate->setCustomStatusToRemove( data->m_customStatusToRemove );
		hijackerUpdate->setCustomStatusToDestroy( data->m_customStatusToDestroy );
	}

	// Copied from HIJACKER
	// THIS BLOCK HIDES THE HIJACKER AND REMOVES HIM FROM PARTITION MANAGER
	// remove object from its group (if any)
	obj->leaveGroup();
	if( ai )
	{
		//By setting him to idle, we will prevent him from entering the target after this gets called.
		ai->aiIdle( CMD_FROM_AI );
	}

	//This is kinda special... we will endow our new ride with our vision and shroud range, since we are driving
	other->setVisionRange(getObject()->getVisionRange());
	other->setShroudClearingRange(getObject()->getShroudClearingRange());

	// remove rider from partition manager
	ThePartitionManager->unRegisterObject( obj );

	// hide the drawable associated with rider
	if( obj->getDrawable() )
		obj->getDrawable()->setDrawableHidden( true );

	// By returning FALSE, we will not remove the object (Equipper)
	return FALSE;
//	return TRUE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void EquipCrateCollide::setEquipStatus(Object *other)
{
	// If the Object doesn't exist, or destroyed do nothing.
	if(!other || other->isDestroyed() || other->isEffectivelyDead())
		return;

	Object *obj = getObject();
	const EquipCrateCollideModuleData* data = getEquipCrateCollideModuleData();

	// Clear the Equipper from the list of last Collided Object 
	if(m_equipToID != INVALID_ID)
	{
		Object *lastEquipObj = TheGameLogic->findObjectByID(m_equipToID);
		if(lastEquipObj)
			lastEquipObj->clearLastEquipObjectID(obj->getID());
	}

	m_equipToID = other->getID();
	obj->setEquipToID( m_equipToID );
	other->setEquipObjectID(obj->getID());

	if(data->m_equipCanPassiveAcquire)
	{
		other->setEquipAttackableObjectID(obj->getID());
	}
	else if(!data->m_isContain)
	{
		obj->setStatus( MAKE_OBJECT_STATUS_MASK( OBJECT_STATUS_NO_ATTACK ) );
	}

	// Set the Statuses for to prevent selection collision detection
	obj->setStatus( MAKE_OBJECT_STATUS_MASK3( OBJECT_STATUS_NO_COLLISIONS, OBJECT_STATUS_MASKED, OBJECT_STATUS_UNSELECTABLE ) );

	// Set the Status when Equipping
	// Mitigated to the main CrateCollideModule
	CrateCollide::executeCrateBehavior(other);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void EquipCrateCollide::clearEquipStatus()
{
	// If we are not equipped, or this is not the correct Equip Crate Collide module, return
	if(getObject()->getEquipToID() == INVALID_ID || getObject()->getEquipToID() != m_equipToID)
		return;
	
	Object *obj = getObject();
	const EquipCrateCollideModuleData* data = getEquipCrateCollideModuleData();

	// Restore its status types to make it selectable and able to trigger Collision & Contain Properties
	obj->clearStatus( MAKE_OBJECT_STATUS_MASK3( OBJECT_STATUS_NO_COLLISIONS, OBJECT_STATUS_MASKED, OBJECT_STATUS_UNSELECTABLE ) );

	if(!data->m_equipCanPassiveAcquire && !data->m_isContain)
	{
		obj->clearStatus( MAKE_OBJECT_STATUS_MASK( OBJECT_STATUS_NO_ATTACK ) );
	}


	// Clear the statuses of the Object while it is equipped
	// Moved to Main CrateCollide module
	//obj->clearStatus( data->m_statusToSet );
	//for(std::vector<AsciiString>::const_iterator it = data->m_customStatusToSet.begin(); it != data->m_customStatusToSet.end(); ++it)
	//{
	//	obj->clearCustomStatus( *it );
	//}

	// Find the equipped Object ID and clear its given statuses and bonuses
	Object *other = TheGameLogic->findObjectByID(obj->getEquipToID());

	// clear the ID of the current equipped object to reenable new equip
	obj->setEquipToID(INVALID_ID);

	// We don't clear this one to prevent the Object from Squishing the Collider
	// clear its own ID for verifying equip behavior
	//m_equipToID = INVALID_ID;

	// If the Object doesn't exist, or destroyed do nothing.
	if(!other || other->isDestroyed() || other->isEffectivelyDead())
		return;

	other->clearEquipObjectID(obj->getID());

	// If there are other that uses the same template of the Equip Object, don't remove the bonuses
	if(data->m_isContain && other->getContain())
	{
		const ContainedItemsList* contained = other->getContain()->getContainedItemsList();
		if (contained)
		{
			for (ContainedItemsList::const_iterator it = contained->begin(); it != contained->end(); ++it)
			{
				if( (*it)->getTemplate()->isEquivalentTo( obj->getTemplate() ) )
					return;
			}
		}
	}
	// Do the same, but check for non-contain
	else if(!other->getEquipObjectIDs().empty())
	{
		std::vector<ObjectID> IDs = other->getEquipObjectIDs();
		for( std::vector<ObjectID>::const_iterator it = IDs.begin(); it != IDs.end(); ++it )
		{
			Object* equipMember = TheGameLogic->findObjectByID(*it);
			if(equipMember && equipMember->getTemplate()->isEquivalentTo( obj->getTemplate() ))
				return;
		}
	}

	/*other->clearStatus( data->m_statusToGive );
	for(std::vector<AsciiString>::const_iterator it = data->m_customStatusToGive.begin(); it != data->m_customStatusToGive.end(); ++it)
	{
		other->clearCustomStatus( *it );
	}
	for (Int i = 0; i < data->m_bonusToGive.size(); i++) {
		other->clearWeaponBonusCondition(m_bonusToGive[i]);
	}
	for(std::vector<AsciiString>::const_iterator it = data->m_customBonusToGive.begin(); it != data->m_customBonusToGive.end(); ++it)
	{
		other->clearCustomWeaponBonusCondition( *it );
	}*/
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
Bool EquipCrateCollide::revertCollideBehavior(Object *other)
{
	// If we are not equipped, or this is not the correct Equip Crate Collide module, return
	if(getObject()->getEquipToID() == INVALID_ID || getObject()->getEquipToID() != m_equipToID)
		return FALSE;

	ObjectID currentEquippedID = getObject()->getEquipToID();

	CrateCollide::revertCollideBehavior(other);
	clearEquipStatus();

	getObject()->setLastExitedFrame(TheGameLogic->getFrame() + 10*LOGICFRAMES_PER_SECOND);

	// If the Object doesn't exist, or is destroyed we stop here.
	if(!other || other->isDestroyed() || other->isEffectivelyDead())
		return FALSE;

	// Not the right equipped Object
	if(other->getID() != currentEquippedID)
		return FALSE;

	// Indicate that we have reverted the Collied Behavior and no need to continue 
	return TRUE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void EquipCrateCollide::overwriteEquipToIDModule(ObjectID ID)
{
	// We only overwrite if we are equipped to the right object, this is compatible for objects with multiple EquipCrateCollide modules
	if( ID != INVALID_ID && getObject()->getEquipToID() == m_equipToID )
		m_equipToID = ID;
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void EquipCrateCollide::crc( Xfer *xfer )
{

	// extend base class
	CrateCollide::crc( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void EquipCrateCollide::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	CrateCollide::xfer( xfer );

	xfer->xferObjectID(&m_equipToID);

}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void EquipCrateCollide::loadPostProcess()
{

	// extend base class
	CrateCollide::loadPostProcess();

}

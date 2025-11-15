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

// FILE: ReplaceObjectUpgrade.cpp /////////////////////////////////////////////////////////////////////////////
// Author: Graham Smallwood, July 2003
// Desc:	 UpgradeModule that creates a new Object in our exact location and then deletes our object
///////////////////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file int the GameEngine

#define DEFINE_HEALTHTRANSFERTYPE_NAMES

#include "GameLogic/Module/ReplaceObjectUpgrade.h"

#include "Common/Player.h"
#include "Common/ThingFactory.h"
#include "Common/ThingTemplate.h"
#include "GameLogic/AI.h"
#include "GameLogic/AIPathfind.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/Module/BodyModule.h"
#include "GameLogic/Module/HijackerUpdate.h"
#include "GameLogic/Module/StatusDamageHelper.h"
#include "GameLogic/Module/CreateModule.h"
#include "GameLogic/Module/ContainModule.h"
#include "GameLogic/Object.h"

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void ReplaceObjectUpgradeModuleData::buildFieldParse(MultiIniFieldParse& p)
{
  UpgradeModuleData::buildFieldParse(p);

	static const FieldParse dataFieldParse[] =
	{
		{ "ReplaceObject",	INI::parseAsciiString,	NULL, offsetof( ReplaceObjectUpgradeModuleData, m_replaceObjectName ) },
		{ "TransferHealth",	INI::parseBool,	NULL, offsetof( ReplaceObjectUpgradeModuleData, m_transferHealth ) },
		{ "TransferAttackers",	INI::parseBool,	NULL, offsetof( ReplaceObjectUpgradeModuleData, m_transferAttack ) },
		{ "TransferStatuses",	INI::parseBool,	NULL, offsetof( ReplaceObjectUpgradeModuleData, m_transferStatus ) },
		{ "TransferWeaponBonuses",	INI::parseBool,	NULL, offsetof( ReplaceObjectUpgradeModuleData, m_transferWeaponBonus ) },
		{ "TransferBombs",	INI::parseBool,	NULL, offsetof( ReplaceObjectUpgradeModuleData, m_transferBombs ) },
		{ "TransferHijackers",	INI::parseBool,	NULL, offsetof( ReplaceObjectUpgradeModuleData, m_transferHijackers ) },
		{ "TransferParasites",	INI::parseBool,	NULL, offsetof( ReplaceObjectUpgradeModuleData, m_transferParasites ) },
		{ "TransferPassengers",	INI::parseBool,	NULL, offsetof( ReplaceObjectUpgradeModuleData, m_transferPassengers ) },
		{ "HealthTransferType",		INI::parseIndexList,		TheMaxHealthChangeTypeNames, offsetof( ReplaceObjectUpgradeModuleData, m_transferHealthChangeType ) },
		{ 0, 0, 0, 0 }
	};
  p.add(dataFieldParse);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
ReplaceObjectUpgrade::ReplaceObjectUpgrade( Thing *thing, const ModuleData* moduleData ) : UpgradeModule( thing, moduleData )
{
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
ReplaceObjectUpgrade::~ReplaceObjectUpgrade( void )
{
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void ReplaceObjectUpgrade::upgradeImplementation( )
{
	const ReplaceObjectUpgradeModuleData *data = getReplaceObjectUpgradeModuleData();

	Object *me = getObject();

	Matrix3D myMatrix = *me->getTransformMatrix();
	Team *myTeam = me->getTeam();// Team implies player.  It is a subset.

	const ThingTemplate *replacementTemplate = TheThingFactory->findTemplate(data->m_replaceObjectName);
	if( replacementTemplate == NULL )
	{
		DEBUG_ASSERTCRASH(replacementTemplate != NULL, ("No such object '%s' in ReplaceObjectUpgrade.", data->m_replaceObjectName.str() ) );
		return;
	}

	AIUpdateInterface *ai = me->getAI();
	ObjectStatusMaskType prevStatus = me->getStatusBits();

	// Remove us first since occupation of cells is apparently not a refcount, but a flag.  If I don't remove, then the new
	// thing will be placed, and then on deletion I will remove "his" marks.
	//TheAI->pathfinder()->removeObjectFromPathfindMap( me );
	//TheGameLogic->destroyObject(me);

	// Don't destroy us first, make us non-interactable
	{
		me->setStatus( MAKE_OBJECT_STATUS_MASK3( OBJECT_STATUS_NO_COLLISIONS, OBJECT_STATUS_MASKED, OBJECT_STATUS_UNSELECTABLE ) );
		me->leaveGroup();
		if( ai )
		{
			//By setting him to idle, we will prevent him from entering the target after this gets called.
			ai->aiIdle( CMD_FROM_AI );
		}
		// remove from partition manager
		ThePartitionManager->unRegisterObject( me );
	}

	Object *replacementObject = TheThingFactory->newObject(replacementTemplate, myTeam);
	replacementObject->setTransformMatrix(&myMatrix);
	TheAI->pathfinder()->addObjectToPathfindMap( replacementObject );

	// Now onCreates were called at the constructor.  This magically created
	// thing needs to be considered as Built for Game specific stuff.
	for (BehaviorModule** m = replacementObject->getBehaviorModules(); *m; ++m)
	{
		CreateModuleInterface* create = (*m)->getCreate();
		if (!create)
			continue;
		create->onBuildComplete();
	}

	// Transfer any bombs to the replacement Object
	std::vector<ObjectID> BombsMarkedForDestroy;
	Object *obj = TheGameLogic->getFirstObject();
	while( obj )
	{
		if( obj->isKindOf( KINDOF_MINE ) )
		{
			//static NameKeyType key_StickyBombUpdate = NAMEKEY( "StickyBombUpdate" );
			//StickyBombUpdate *update = (StickyBombUpdate*)obj->findUpdateModule( key_StickyBombUpdate );
			StickyBombUpdateInterface *update = obj->getStickyBombUpdateInterface();
			if( update && update->getTargetObject() == me )
			{
				if(data->m_transferBombs)
					update->setTargetObject( replacementObject );
				else
					BombsMarkedForDestroy.push_back(obj->getID());
			}
		}
		obj = obj->getNextObject();
	}

	// Or not, we just get rid of the bomb
	for(std::vector<ObjectID>::const_iterator it = BombsMarkedForDestroy.begin(); it != BombsMarkedForDestroy.end(); ++it)
	{
		Object *bomb = TheGameLogic->findObjectByID(*it);
		if(bomb)
			TheGameLogic->destroyObject(bomb);
	}

	if( data->m_transferAIStates )
	{
		AIUpdateInterface *new_ai = replacementObject->getAI();
		if( ai && new_ai )
		{
			//This flag determines if the object has started moving yet... if not
			//it's a good initial check.
			Bool isEffectivelyMoving = ai->isMoving() || ai->isWaitingForPath();

			//Are we trying to attack something. If so, we need to be in range before we can do so.
			Bool isTryingToAttack = ai->isAttacking();

			//Are we in guard mode? If so, are we idle... idle guarders deploy for fastest response against attackers.
			Bool isInGuardIdleState = ai->friend_isInGuardIdleState();

			// Transfer my Attack State
			if( isTryingToAttack )
			{
				if(ai->getGoalObject() != NULL)
				{
					if(ai->getAIStateType() == AI_FORCE_ATTACK_OBJECT)
						new_ai->aiForceAttackObject( ai->getGoalObject(), NO_MAX_SHOTS_LIMIT, ai->getLastCommandSource() );
					else
						new_ai->aiAttackObject( ai->getGoalObject(), NO_MAX_SHOTS_LIMIT, ai->getLastCommandSource() );
				}
				else if(ai->getGoalPosition()->length() > 1.0f )
				{
					new_ai->aiAttackPosition( ai->getGoalPosition(), NO_MAX_SHOTS_LIMIT, ai->getLastCommandSource() );
				}
			}
			// Transfer my Guard State
			else if( isInGuardIdleState )
			{
				if(ai->getGuardObject() != INVALID_ID)
				{
					Object *guardObj = TheGameLogic->findObjectByID(ai->getGuardObject());
					if(guardObj)
						new_ai->aiGuardObject( guardObj, ai->getGuardMode(), ai->getLastCommandSource() );
				}
				else if(ai->getGuardLocation()->length() > 1.0f )
				{
					new_ai->aiGuardPosition( ai->getGuardLocation(), ai->getGuardMode(), ai->getLastCommandSource() );
				}
			}
			// Transfer my Moving State
			else if( isEffectivelyMoving )
			{
				if( ai->getAIStateType() == AI_ATTACK_MOVE_TO )
				{
					//Continue to move towards the attackmove area.
					aiAttackMoveToPosition( ai->getGoalPosition(), NO_MAX_SHOTS_LIMIT, ai->getLastCommandSource() );
				}
				else
				{
					aiMoveToPosition( ai->getGoalPosition(), ai->getLastCommandSource() );
				}
			}

			// Transfer my Supply State
			SupplyTruckAIInterface* supplyTruckAI = ai->getSupplyTruckAIInterface();
			SupplyTruckAIInterface* supplyTruckNewAI = ai->getSupplyTruckAIInterface();
			if( supplyTruckAI && supplyTruckNewAI ) {
				// If it is gathering supplies, tell its replacer to do the same
				if (supplyTruckAI->isCurrentlyFerryingSupplies() || supplyTruckAI->isForcedIntoWantingState())
				{
					supplyTruckNewAI->setForceWantingState(true);
				}
			}

			// Transfer my Dozer State
			DozerAIInterface* DozerAI = ai->getDozerAIInterface();
			DozerAIInterface* DozerNewAI = ai->getDozerAIInterface();
			if( DozerAI && DozerNewAI)
			{
				
				// If it is gathering supplies, tell its replacer to do the same
				if(DozerAI->getCurrentTask() != DOZER_TASK_INVALID)
				{
					Object taskTarget = TheGameLogic->findObjectByID( DozerAI->getTaskTarget(DozerAI->getCurrentTask()) );
					if(taskTarget)
					{
						switch(DozerAI->getCurrentTask())
						{
							case DOZER_TASK_BUILDING:
								new_ai->aiResumeConstruction(taskTarget, ai->getLastCommandSource());
								break;
							case DOZER_TASK_REPAIR:
								new_ai->aiRepair(taskTarget, ai->getLastCommandSource());
								break;
						}
					}
				}
				else if (DozerAI->isAnyTaskPending())
				{
					for( Int i = 0; i < DOZER_NUM_TASKS; i++ )
					{
						if( DozerAI->isTaskPending( (DozerTask)i ) )
						{
							Object taskTarget = TheGameLogic->findObjectByID( DozerAI->getTaskTarget((DozerTask)i) );
							if(taskTarget)
							{
								switch((DozerTask)i)
								{
									case DOZER_TASK_BUILDING:
										new_ai->aiResumeConstruction(taskTarget, ai->getLastCommandSource());
										break;
									case DOZER_TASK_REPAIR:
										new_ai->aiRepair(taskTarget, ai->getLastCommandSource());
										break;
								}
							}

							break;
						}
					}
				}

			}

		}
	}

	if(data->m_transferPassengers)
	{
		// Get the unit's contain
		ContainModuleInterface *contain = me->getContain();

		std::vector<ObjectID>vecID;

		// Disable Enter/Exit Sounds
		contain->enableLoadSounds(FALSE);

		// Get Contain List
		ContainedItemsList list = contain->getContainList();
		ContainedItemsList::iterator it = list.begin();
		while ( it != list.end() )
		{
			Object *obj = *it++;
			DEBUG_ASSERTCRASH( obj, ("Contain list must not contain NULL element"));

			// Remove Passenger from current contain
			removeFromContain( obj, false );

			// Add the Passenger to the list to put into the new container later
			vecID.push_back(obj->getID());
		}

		ContainModuleInterface *newContain = replacementObject->getContain();

		if(newContain)
		{
			// Disable Enter/Exit Sounds for Replacement Contain
			newContain->enableLoadSounds(FALSE);

			for(int i = 0; i < vecID.size(); i++)
			{
				Object *add = TheGameLogic->findObjectByID( vecID[i] );
				if(add)
				{
					// Add Passenger to current contain if valid
					if( newContain && newContain->isValidContainerFor(add, TRUE) )
					{
						newContain->addToContain(add);
					}
				}
			}

			// Enable Enter/Exit Sounds for Replacement Contain
			newContain->enableLoadSounds(TRUE);
		}

	}
	else
	{
		ContainModuleInterface *contain = me->getContain();

		if(contain)
		{
			contain->removeAllContained();
		}
	}

	
	// Assault Transport Matters, switching Transports
	if(me->getAssaultTransportObjectID() != INVALID_ID)
	{
		me->removeMeFromAssaultTransport(replacementObject->getID());
	}

	// Shielded Objects
	replacementObject->setShieldByTargetID(me->getShieldByTargetID(), me->getShieldByTargetType());

	if (data->m_transferAttackers)
	{
		for ( Object *obj = TheGameLogic->getFirstObject(); obj; obj = obj->getNextObject() )
		{
			AIUpdateInterface* aiInterface = obj->getAI();
			if (!aiInterface)
				continue;

			aiInterface->transferAttack(me->getID(), replacementObject->getID());
		}
	}

	if( data->m_transferHealth )
	{
		//Convert old health to new health.
		BodyModuleInterface *oldBody = me->getBodyModule();
		BodyModuleInterface *newBody = replacementObject->getBodyModule();
		if( oldBody && newBody )
		{
			//First transfer subdual damage
			DamageInfo damInfo;
			Real subdualDamageAmount = oldBody->getCurrentSubdualDamageAmount();
			if( subdualDamageAmount > 0.0f )
			{
				damInfo.in.m_amount = subdualDamageAmount;
				damInfo.in.m_damageType = DAMAGE_SUBDUAL_UNRESISTABLE;
				damInfo.in.m_sourceID = INVALID_ID;
				newBody->attemptDamage( &damInfo );
			}

			newBody->setCurrentSubdualDamageAmountCustom(oldBody->getCurrentSubdualDamageAmountCustom());
			replacementObject->transferSubdualHelperData(me->getSubdualHelperData());
			replacementObject->refreshSubdualHelper();

			//Now transfer the previous health from the old object to the new.
			/*damInfo.in.m_amount = oldBody->getMaxHealth() - oldBody->getPreviousHealth();
			damInfo.in.m_damageType = DAMAGE_UNRESISTABLE;
			damInfo.in.m_sourceID = oldBody->getLastDamageInfo()->in.m_sourceID;
			if( damInfo.in.m_amount > 0.0f )
			{
				newBody->attemptDamage( &damInfo );
			}*/

			Real chronoDamageAmount = oldBody->getCurrentChronoDamageAmount();
			if( chronoDamageAmount > 0.0f )
			{
				damInfo.in.m_amount = chronoDamageAmount;
				damInfo.in.m_damageType = DAMAGE_CHRONO_UNRESISTABLE;
				damInfo.in.m_sourceID = INVALID_ID;
				newBody->attemptDamage( &damInfo );
			}

			Real oldHealth = oldBody->getHealth();
			Real oldMaxHealth = oldBody->getMaxHealth();
			Real newMaxHealth = newBody->getMaxHealth();

			switch( data->m_transferHealthChangeType )
			{
				case PRESERVE_RATIO:
				{
					//400/500 (80%) + 100 becomes 480/600 (80%)
					//200/500 (40%) - 100 becomes 160/400 (40%)
					Real ratio = oldHealth / oldMaxHealth;
					Real newHealth = newMaxHealth * ratio;
					internalChangeHealth( newHealth - newMaxHealth );
					break;
				}
				// In this case, it becomes ADD_CURRENT_DAMAGE, there's no ADD_CURRENT_HEALTH_TOO
				case ADD_CURRENT_DAMAGE_NON_LETHAL:
				case ADD_CURRENT_DAMAGE:
				{
					//Add the same amount that we are adding to the max health.
					//This could kill you if max health is reduced (if we ever have that ability to add buffer health like in D&D)
					//400/500 (80%) + 100 becomes 500/600 (83%)
					//200/500 (40%) - 100 becomes 100/400 (25%)
					if(data->m_transferHealthChangeType == ADD_CURRENT_DAMAGE && fabs(oldHealth - oldMaxHealth) > newMaxHealth)
						replacementObject->kill();
					else
						internalChangeHealth( max(1.0f - newMaxHealth, oldHealth - oldMaxHealth) );
					break;
				}
				case SAME_CURRENTHEALTH:
					//preserve past health amount
					internalChangeHealth( oldHealth - newMaxHealth );
					break;
			}
		}
	}

	if( data->m_transferStatus )
	{
		replacementObject->setStatus( prevStatus );
		ObjectCustomStatusType prevCustomStatus = me->getCustomStatus();

		for(ObjectCustomStatusType::const_iterator it = prevCustomStatus.begin(); it != prevCustomStatus.end(); ++it)
			replacementObject->setCustomStatus( it->first, it->second );

		replacementObject->transferStatusHelperData(me->getStatusHelperData());
		replacementObject->refreshStatusHelper();
	}

	if (data->m_transferAttackers)
	{
		for ( Object *obj = TheGameLogic->getFirstObject(); obj; obj = obj->getNextObject() )
		{
			AIUpdateInterface* aiInterface = obj->getAI();
			if (!aiInterface)
				continue;

			aiInterface->transferAttack(me->getID(), replacementObject->getID());
		}
	}

	// Now we destroy the Object
	TheAI->pathfinder()->removeObjectFromPathfindMap( me );
	TheGameLogic->destroyObject(me);

	if( replacementObject->getControllingPlayer() )
	{
		replacementObject->getControllingPlayer()->onStructureConstructionComplete(me, replacementObject, FALSE);
	}
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void ReplaceObjectUpgrade::crc( Xfer *xfer )
{

	// extend base class
	UpgradeModule::crc( xfer );

}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void ReplaceObjectUpgrade::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	UpgradeModule::xfer( xfer );

}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void ReplaceObjectUpgrade::loadPostProcess( void )
{

	// extend base class
	UpgradeModule::loadPostProcess();

}  // end loadPostProcess

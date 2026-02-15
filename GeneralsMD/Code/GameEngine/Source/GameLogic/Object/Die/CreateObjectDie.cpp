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

// FILE: CreateObjectDie.cpp ///////////////////////////////////////////////////////////////////////////
// Author: Michael S. Booth, January 2002
// Desc:   Create an object upon this object's death
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#define DEFINE_OBJECT_STATUS_NAMES
#define DEFINE_MAXHEALTHCHANGETYPE_NAMES						// for TheMaxHealthChangeTypeNames[]
#define DEFINE_DISPOSITION_NAMES
#include "GameLogic/Module/AIUpdate.h"
#include "Common/Player.h"
#include "Common/ThingFactory.h"
#include "Common/Xfer.h"
#include "GameLogic/AIPathfind.h"
#include "GameLogic/ExperienceTracker.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/PartitionManager.h"
#include "GameLogic/ScriptEngine.h"
#include "GameLogic/TerrainLogic.h"
#include "GameLogic/Module/CreateObjectDie.h"
#include "GameLogic/Module/AssaultTransportAIUpdate.h"
#include "GameLogic/Module/DozerAIUpdate.h"
#include "GameLogic/Module/FloatUpdate.h"
#include "GameLogic/Module/HijackerUpdate.h"
#include "GameLogic/Module/PhysicsUpdate.h"
#include "GameLogic/Module/StickyBombUpdate.h"
#include "GameLogic/Module/SupplyTruckAIUpdate.h"
#include "GameLogic/Object.h"
#include "GameLogic/Weapon.h" // NoMaxShotsLimit
#include "GameLogic/ObjectCreationList.h"
#include "GameLogic/Module/ContainModule.h"
#include "GameLogic/Module/BodyModule.h"
#include "GameClient/Drawable.h"
#include "GameClient/InGameUI.h"


// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
CreateObjectDieModuleData::CreateObjectDieModuleData()
{

	m_ocl = nullptr;
	m_transferPreviousHealth = FALSE;
	m_transferSelection = FALSE;
	m_transferExperience = FALSE;
	m_transferAttackers = FALSE;
	m_transferPreviousHealthDontTransferAttackers = FALSE;
	m_transferAIStates = FALSE;
	m_transferStatus = FALSE;
	m_transferWeaponBonus = FALSE;
	m_transferDisabledType = FALSE;
	m_transferBombs = FALSE;
	m_transferHijackers = TRUE;
	m_transferEquippers = TRUE;
	m_transferParasites = TRUE;
	m_transferPassengers = FALSE;
	m_transferToAssaultTransport = FALSE;
	m_transferShieldedTargets = FALSE;
	m_transferShieldingTargets = FALSE;
	m_transferSelectionDontClearGroup = FALSE;
	m_transferObjectName = FALSE;
	m_previousHealthChangeType = ADD_CURRENT_DAMAGE;

	m_extraBounciness = 0.0f;
	m_extraFriction = 0.0f;
	m_disposition = ON_GROUND_ALIGNED;
	m_dispositionIntensity = 0.0f;
	m_spinRate = -1.0f;
	m_yawRate = -1.0f;
	m_rollRate = -1.0f;
	m_pitchRate = -1.0f;
	m_minMag = 0.0f;
	m_maxMag = 0.0f;
	m_minPitch = 0.0f;
	m_maxPitch = 0.0f;
	m_orientInForceDirection = FALSE;
	m_diesOnBadLand = FALSE;

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void parseFrictionPerSec( INI* ini, void * /*instance*/, void *store, const void* /*userData*/ )
{
	Real fricPerSec = INI::scanReal(ini->getNextToken());
	Real fricPerFrame = fricPerSec * SECONDS_PER_LOGICFRAME_REAL;
	*(Real *)store = fricPerFrame;
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
/*static*/ void CreateObjectDieModuleData::buildFieldParse(MultiIniFieldParse& p)
{
	DieModuleData::buildFieldParse(p);

	static const FieldParse dataFieldParse[] =
	{
		{ "CreationList",	INI::parseObjectCreationList,		nullptr,											offsetof( CreateObjectDieModuleData, m_ocl ) },
		{ "TransferPreviousHealth", INI::parseBool, nullptr	,offsetof( CreateObjectDieModuleData, m_transferPreviousHealth ) },
		{ "TransferSelection", INI::parseBool, nullptr, offsetof( CreateObjectDieModuleData, m_transferSelection ) },

		{ "TransferAIStates",	INI::parseBool,	nullptr, offsetof( CreateObjectDieModuleData, m_transferAIStates ) },
		{ "TransferExperience",	INI::parseBool,	nullptr, offsetof( CreateObjectDieModuleData, m_transferExperience ) },
		{ "TransferAttackers",	INI::parseBool,	nullptr, offsetof( CreateObjectDieModuleData, m_transferAttackers ) },
		{ "TransferPreviousHealthDontTransferAttackers", INI::parseBool, nullptr	,offsetof( CreateObjectDieModuleData, m_transferPreviousHealthDontTransferAttackers ) },
		{ "TransferStatuses",	INI::parseBool,	nullptr, offsetof( CreateObjectDieModuleData, m_transferStatus ) },
		{ "TransferWeaponBonuses",	INI::parseBool,	nullptr, offsetof( CreateObjectDieModuleData, m_transferWeaponBonus ) },
		{ "TransferDisabledType",	INI::parseBool,	nullptr, offsetof( CreateObjectDieModuleData, m_transferDisabledType ) },
		{ "TransferBombs",	INI::parseBool,	nullptr, offsetof( CreateObjectDieModuleData, m_transferBombs ) },
		{ "TransferHijackers",	INI::parseBool,	nullptr, offsetof( CreateObjectDieModuleData, m_transferHijackers ) },
		{ "TransferEquippers",	INI::parseBool,	nullptr, offsetof( CreateObjectDieModuleData, m_transferEquippers ) },
		{ "TransferParasites",	INI::parseBool,	nullptr, offsetof( CreateObjectDieModuleData, m_transferParasites ) },
		{ "TransferPassengers",	INI::parseBool,	nullptr, offsetof( CreateObjectDieModuleData, m_transferPassengers ) },
		{ "TransferToAssaultTransport",	INI::parseBool,	nullptr, offsetof( CreateObjectDieModuleData, m_transferToAssaultTransport ) },
		{ "TransferShieldedTargets",	INI::parseBool,	nullptr, offsetof( CreateObjectDieModuleData, m_transferShieldedTargets ) },
		{ "TransferShieldingTargets",	INI::parseBool,	nullptr, offsetof( CreateObjectDieModuleData, m_transferShieldingTargets ) },
		{ "TransferSelectionDontClearGroup",	INI::parseBool,	nullptr, offsetof( CreateObjectDieModuleData, m_transferSelectionDontClearGroup ) },
		{ "TransferObjectName",	INI::parseBool,	nullptr, offsetof( CreateObjectDieModuleData, m_transferObjectName ) },
		{ "HealthTransferType",		INI::parseIndexList,		TheMaxHealthChangeTypeNames, offsetof( CreateObjectDieModuleData, m_previousHealthChangeType ) },

		{ "OrientInForceDirection", INI::parseBool, nullptr, offsetof(CreateObjectDieModuleData, m_orientInForceDirection) },
		{ "ExtraBounciness",				INI::parseReal,						nullptr, offsetof( CreateObjectDieModuleData, m_extraBounciness ) },
		{ "ExtraFriction",				parseFrictionPerSec,						nullptr, offsetof( CreateObjectDieModuleData, m_extraFriction ) },
		{ "Offset",						INI::parseCoord3D,				nullptr, offsetof( CreateObjectDieModuleData, m_offset ) },
		{ "Disposition",			INI::parseBitString32,			DispositionNames, offsetof( CreateObjectDieModuleData, m_disposition ) },
		{ "DispositionIntensity",	INI::parseReal,						nullptr,	offsetof( CreateObjectDieModuleData, m_dispositionIntensity ) },
		{ "SpinRate",					INI::parseAngularVelocityReal,	nullptr, offsetof(CreateObjectDieModuleData, m_spinRate) },
		{ "YawRate",					INI::parseAngularVelocityReal,	nullptr, offsetof(CreateObjectDieModuleData, m_yawRate) },
		{ "RollRate",					INI::parseAngularVelocityReal,	nullptr, offsetof(CreateObjectDieModuleData, m_rollRate) },
		{ "PitchRate",				INI::parseAngularVelocityReal,	nullptr, offsetof(CreateObjectDieModuleData, m_pitchRate) },
		{ "MinForceMagnitude",	INI::parseReal,	nullptr, offsetof(CreateObjectDieModuleData, m_minMag) },
		{ "MaxForceMagnitude",	INI::parseReal,	nullptr, offsetof(CreateObjectDieModuleData, m_maxMag) },
		{ "MinForcePitch",	INI::parseAngleReal,	nullptr, offsetof(CreateObjectDieModuleData, m_minPitch) },
		{ "MaxForcePitch",	INI::parseAngleReal,	nullptr, offsetof(CreateObjectDieModuleData, m_maxPitch) },
		{ "DiesOnBadLand",	INI::parseBool, nullptr, offsetof(CreateObjectDieModuleData, m_diesOnBadLand) },

		{ nullptr, nullptr, nullptr, 0 }
	};
	p.add(dataFieldParse);

}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
CreateObjectDie::CreateObjectDie( Thing *thing, const ModuleData* moduleData ) : DieModule( thing, moduleData )
{
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
CreateObjectDie::~CreateObjectDie( void )
{

}

//-------------------------------------------------------------------------------------------------
/** The die callback. */
//-------------------------------------------------------------------------------------------------
void CreateObjectDie::onDie( const DamageInfo * damageInfo )
{
	const CreateObjectDieModuleData *data = getCreateObjectDieModuleData();
	if (!isDieApplicable(damageInfo))
		return;

	Object *damageDealer = TheGameLogic->findObjectByID( damageInfo->in.m_sourceID );

	Object *newObject = ObjectCreationList::create( data->m_ocl, getObject(), damageDealer );
	if(!newObject)
	{
		ContainModuleInterface *contain = getObject()->getContain();

		if(contain)
		{
			contain->removeAllContained();
		}
		return;
	}

	Object *me = getObject();

	doDisposition(me, newObject);

	// Transfer any bombs onto the replacement Object
	std::vector<ObjectID> BombsMarkedForDestroy;
	Object *obj = TheGameLogic->getFirstObject();
	while( obj )
	{
		// Transfer bombs to the replacement Object or destroy them
		//if( obj->isKindOf( KINDOF_MINE ) )
		//{
			//static NameKeyType key_StickyBombUpdate = NAMEKEY( "StickyBombUpdate" );
			//StickyBombUpdate *update = (StickyBombUpdate*)obj->findUpdateModule( key_StickyBombUpdate );
			StickyBombUpdateInterface *update = obj->getStickyBombUpdateInterface();
			if( update && update->getTargetObject() == me )
			{
				if(data->m_transferBombs)
					update->setTargetObject( newObject );
				else
					BombsMarkedForDestroy.push_back(obj->getID());
			}
		//}

		// Transfer attackers
		/// IamInnocent - Transfer Previous Health also transfer the Attackers for this module.
		if ((data->m_transferPreviousHealth && !data->m_transferPreviousHealthDontTransferAttackers) || data->m_transferAttackers)
		{
			AIUpdateInterface* aiInterface = obj->getAI();
			if (aiInterface)
				aiInterface->transferAttack(me->getID(), newObject->getID());

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
		AIUpdateInterface* ai = me->getAI();
		AIUpdateInterface *new_ai = newObject->getAI();
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
				if(ai->getGoalObject() != nullptr)
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
					new_ai->aiAttackMoveToPosition( ai->getGoalPosition(), NO_MAX_SHOTS_LIMIT, ai->getLastCommandSource() );
				}
				else
				{
					new_ai->aiMoveToPosition( ai->getGoalPosition(), ai->getLastCommandSource() );
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
					DozerTask curTask = DozerAI->getCurrentTask();
					Object *taskTarget = TheGameLogic->findObjectByID( DozerAI->getTaskTarget(curTask) );
					if(taskTarget)
					{
						switch(curTask)
						{
							case DOZER_TASK_BUILD:
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
							Object *taskTarget = TheGameLogic->findObjectByID( DozerAI->getTaskTarget((DozerTask)i) );
							if(taskTarget)
							{
								switch((DozerTask)i)
								{
									case DOZER_TASK_BUILD:
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

	if(data->m_transferPassengers && me->getContain())
	{
		// Get the unit's contain
		ContainModuleInterface *contain = me->getContain();

		std::vector<ObjectID>vecID;

		// Disable Enter/Exit Sounds
		contain->enableLoadSounds(FALSE);

		// Get Contain List
		ContainedItemsList list;
		contain->swapContainedItemsList(list);

		ContainedItemsList::iterator it = list.begin();
		while ( it != list.end() )
		{
			Object *obj = *it++;
			DEBUG_ASSERTCRASH( obj, ("Contain list must not contain null element"));

			// Remove Passenger from current contain
			contain->removeFromContain( obj, false );

			// Add the Passenger to the list to put into the new container later
			vecID.push_back(obj->getID());
		}

		ContainModuleInterface *newContain = newObject->getContain();

		if(newContain)
		{
			// Disable Enter/Exit Sounds for New Contain
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

			// Enable Enter/Exit Sounds for New Contain
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

	// Transfer my experience
	if (data->m_transferExperience && newObject->getExperienceTracker())
	{
		VeterancyLevel v = me->getVeterancyLevel();
		newObject->getExperienceTracker()->setHighestExpOrLevel(me->getExperienceTracker()->getCurrentExperience(), v, FALSE);
	}

	// Assault Transport Matters, switching Transports
	if(data->m_transferToAssaultTransport && me->getAssaultTransportObjectID() != INVALID_ID)
	{
		me->removeMeFromAssaultTransport(newObject->getID());
	}

	// Transfer Statuses
	if( data->m_transferStatus )
	{
		ObjectStatusMaskType prevStatus = me->getStatusBits();
		//prevStatus.clearStatus( MAKE_OBJECT_STATUS_MASK( OBJECT_STATUS_DESTROYED ));
		newObject->setStatus( prevStatus );
		newObject->setCustomStatusFlags( me->getCustomStatus() );

		newObject->doObjectStatusChecks();

		newObject->transferStatusHelperData(me->getStatusHelperData());
		newObject->refreshStatusHelper();
	}

	// Transfer Weapon Bonuses
	if( data->m_transferWeaponBonus )
	{
		newObject->setWeaponBonusConditionFlags(me->getWeaponBonusCondition());
		newObject->setWeaponBonusConditionIgnoreClear(me->getWeaponBonusConditionIgnoreClear());
		newObject->setCustomWeaponBonusConditionFlags(me->getCustomWeaponBonusCondition());
		newObject->setCustomWeaponBonusConditionIgnoreClear(me->getCustomWeaponBonusConditionIgnoreClear());
		newObject->doWeaponBonusChange();

		newObject->transferTempWeaponBonusHelperData(me->getTempWeaponBonusHelperData());
		newObject->refreshTempWeaponBonusHelper();
	}

	// Transfer Disabled Type
	if(data->m_transferDisabledType)
	{
		newObject->setDisabledTint(me->getDisabledTint());
		newObject->setDisabledCustomTint(me->getDisabledCustomTint());
		newObject->transferDisabledTillFrame(me->getDisabledTillFrame());
	}

	// Shielded Objects
	if(data->m_transferShieldedTargets && me->testCustomStatus("SHIELDED_TARGET"))
	{
		me->clearCustomStatus("SHIELDED_TARGET");
		newObject->setCustomStatus("SHIELDED_TARGET");
		newObject->setShieldByTargetID(me->getShieldByTargetID(), me->getShieldByTargetType());
	}

	if(data->m_transferShieldingTargets)
		newObject->setShielding(me->getShieldingTargetID(), me->getShieldByTargetType());

	// Transfer Objects with HijackerUpdate module (Checks within the Object Function for approval)
	me->doTransferHijacker(newObject->getID(), data->m_transferHijackers, data->m_transferEquippers, data->m_transferParasites);

	// Transfer the Selection Status
	/// IamInnocent - Integrated with the selection module from TheSuperHackers below
	/*if(data->m_transferSelection && newObject->isSelectable() && me->getDrawable() && newObject->getDrawable())
	{
		if(me->getDrawable()->isSelected())
			TheGameLogic->selectObject(newObject, FALSE, me->getControllingPlayer()->getPlayerMask(), me->isLocallyControlled());
	}*/

	// TheSuperHackers @bugfix Stubbjax 02/10/2025 If the old object was selected, select the new one.
	// This is important for the Sneak Attack, which is spawned via a CreateObjectDie module.
	// IamInnocent 02/12/2025 - Edited for Multi-Select of modules
	if (data->m_transferSelection && me->getDrawable() && me->getDrawable()->isSelected())
	{
		//Object* oldObject = getObject();
		//Drawable* selectedDrawable = TheInGameUI->getFirstSelectedDrawable();
		//Bool oldObjectSelected = selectedDrawable && selectedDrawable->getID() == oldObject->getDrawable()->getID();

		//if (oldObjectSelected)
		GameMessage* msg = TheMessageStream->appendMessage(GameMessage::MSG_CREATE_SELECTED_GROUP_NO_SOUND);
		if(data->m_transferSelectionDontClearGroup)
		{
			msg->appendBooleanArgument(FALSE);
		}
		else
		{
			msg->appendBooleanArgument(TRUE);
		}

		msg->appendObjectIDArgument(newObject->getID());
		TheInGameUI->selectDrawable(newObject->getDrawable());
	}

	// Transfer Object Name for Script Engine
	if (data->m_transferObjectName)
	{
		TheScriptEngine->transferObjectName( me->getName(), newObject );
	}

	// Originally an aspect of Disposition, but carry forward unto end of the function because the object may be killed from damage dealt
    if ( data->m_diesOnBadLand )
    {
	    // if we land in the water, we die. alas.
	    const Coord3D* riderPos = newObject->getPosition();
	    Real waterZ, terrainZ;
	    if (TheTerrainLogic->isUnderwater(riderPos->x, riderPos->y, &waterZ, &terrainZ)
			    && riderPos->z <= waterZ + 10.0f
			    && newObject->getLayer() == LAYER_GROUND)
	    {
		    // don't call kill(); do it manually, so we can specify DEATH_FLOODED
		    DamageInfo damageInfo;
		    damageInfo.in.m_damageType = DAMAGE_WATER;	// use this instead of UNRESISTABLE so we don't get a dusty damage effect
		    damageInfo.in.m_deathType = DEATH_FLOODED;
		    damageInfo.in.m_sourceID = INVALID_ID;
		    damageInfo.in.m_amount = HUGE_DAMAGE_AMOUNT;
		    newObject->attemptDamage( &damageInfo );
	    }

	    // Kill if materialized on impassable ground
	    Int cellX = REAL_TO_INT( newObject->getPosition()->x / PATHFIND_CELL_SIZE );
	    Int cellY = REAL_TO_INT( newObject->getPosition()->y / PATHFIND_CELL_SIZE );

	    PathfindCell* cell = TheAI->pathfinder()->getCell( newObject->getLayer(), cellX, cellY );
	    PathfindCell::CellType cellType = cell ? cell->getType() : PathfindCell::CELL_IMPASSABLE;

	    // If we land outside the map, we die too.
	    // Otherwise we exist outside the PartitionManger like a cheater.
	  if( newObject->isOffMap()
      || (cellType == PathfindCell::CELL_CLIFF)
      || (cellType == PathfindCell::CELL_WATER)
      || (cellType == PathfindCell::CELL_IMPASSABLE) )
	    {
		    // We are sorry, for reasons beyond our control, we are experiencing technical difficulties. Please die.
		    newObject->kill();
	    }

  // Note: for future enhancement of this feature, we should test the object against the cell type he is on,
  // using obj->getAI()->hasLocomotorForSurface( __ ). We cshould not assume here that the object can not
  // find happiness on cliffs or water or whatever.


    }
	
	//If we're transferring previous health, we're transferring the last known
	//health before we died. In the case of the sneak attack tunnel network, it
	//is killed after the lifetime update expires.
	if( data->m_transferPreviousHealth && newObject && !newObject->isEffectivelyDead() )
	{
		//Convert old health to new health.
		Object *oldObject = getObject();
		BodyModuleInterface *oldBody = oldObject->getBodyModule();
		BodyModuleInterface *newBody = newObject->getBodyModule();
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

			Real oldHealth = oldBody->getHealth();
			Real oldMaxHealth = oldBody->getMaxHealth();
			Real newMaxHealth = newBody->getMaxHealth();

			Real damageAmount;

			switch( data->m_previousHealthChangeType )
			{
				case PRESERVE_RATIO:
				{
					//400/500 (80%) + 100 becomes 480/600 (80%)
					//200/500 (40%) - 100 becomes 160/400 (40%)
					Real ratio = oldHealth / oldMaxHealth;
					Real newHealth = newMaxHealth * ratio;
					damageAmount = newMaxHealth - newHealth;
					break;
				}
				// In this case, it becomes ADD_CURRENT_DAMAGE, there's no ADD_CURRENT_HEALTH_TOO
				case ADD_CURRENT_DAMAGE_NON_LETHAL:
					damageAmount = min( oldMaxHealth - 1.0f, oldMaxHealth - oldHealth);
					break;
				case ADD_CURRENT_DAMAGE:
					//Add the same amount that we are adding to the max health.
					//This could kill you if max health is reduced (if we ever have that ability to add buffer health like in D&D)
					//400/500 (80%) + 100 becomes 500/600 (83%)
					//200/500 (40%) - 100 becomes 100/400 (25%)
					damageAmount = oldMaxHealth - oldHealth;
					break;
				case SAME_CURRENTHEALTH:
					//preserve past health amount
					damageAmount = newMaxHealth - oldHealth;
					break;
			}

			//Now transfer the previous health from the old object to the new.
			damInfo.in.m_amount = damageAmount;
			damInfo.in.m_damageType = DAMAGE_UNRESISTABLE;
			damInfo.in.m_sourceID = oldBody->getLastDamageInfo()->in.m_sourceID;
			if( damInfo.in.m_amount > 0.0f )
			{
				newBody->attemptDamage( &damInfo );
			}

			newBody->setCurrentSubdualDamageAmountCustom(oldBody->getCurrentSubdualDamageAmountCustom());
			newObject->transferSubdualHelperData(me->getSubdualHelperData());
			newObject->refreshSubdualHelper();

			Real chronoDamageAmount = oldBody->getCurrentChronoDamageAmount();
			if( chronoDamageAmount > 0.0f )
			{
				damInfo.in.m_amount = chronoDamageAmount;
				damInfo.in.m_damageType = DAMAGE_CHRONO_UNRESISTABLE;
				damInfo.in.m_sourceID = INVALID_ID;
				newBody->attemptDamage( &damInfo );
			}

		}

		//Transfer attackers.
		/*for( Object *obj = TheGameLogic->getFirstObject(); obj; obj = obj->getNextObject() )
		{
			AIUpdateInterface* ai = obj->getAI();
			if (!ai)
				continue;

			ai->transferAttack( oldObject->getID(), newObject->getID() );
		}*/
	}

}

//-------------------------------------------------------------------------------------------------
static void calcRandomForce(Real minMag, Real maxMag, Real minPitch, Real maxPitch, Coord3D* force)
{
	Real angle = GameLogicRandomValueReal(0, 2*PI);
	Real pitch = GameLogicRandomValueReal(minPitch, maxPitch);
	Real mag = GameLogicRandomValueReal(minMag, maxMag);

	Matrix3D mtx(1);
	mtx.Scale(mag);
	mtx.Rotate_Z(angle);
	mtx.Rotate_Y(-pitch);

	Vector3 v = mtx.Get_X_Vector();

	force->x = v.X;
	force->y = v.Y;
	force->z = v.Z;
}

//-------------------------------------------------------------------------------------------------
static void adjustVector(Coord3D *vec, const Matrix3D* mtx)
{
	if (mtx)
	{
		Vector3 vectmp;
		vectmp.X = vec->x;
		vectmp.Y = vec->y;
		vectmp.Z = vec->z;
		vectmp = mtx->Rotate_Vector(vectmp);
		vec->x = vectmp.X;
		vec->y = vectmp.Y;
		vec->z = vectmp.Z;
	}
}

//-------------------------------------------------------------------------------------------------
void CreateObjectDie::doDisposition(Object *sourceObj, Object* obj)
{
	// Sanity
	if( obj == nullptr )
		return;

	const CreateObjectDieModuleData *data = getCreateObjectDieModuleData();

	const Matrix3D *mtx = sourceObj->getTransformMatrix();
	Coord3D offset = data->m_offset;

	const Coord3D *pos = sourceObj->getPosition();
	Coord3D chunkPos = *pos;

	Real orientation = sourceObj->getOrientation();
	// Do nothing if vector is 0 or close to 0.
	if (fabs(offset.x) > WWMATH_EPSILON ||
		fabs(offset.y) > WWMATH_EPSILON ||
		fabs(offset.z) > WWMATH_EPSILON)
	{
		if (mtx)
		{
			adjustVector(&offset, mtx);

			chunkPos.x += offset.x;
			chunkPos.y += offset.y;
			chunkPos.z += offset.z;
		}
	}

	if( BitIsSet( data->m_disposition, INHERIT_VELOCITY ) && sourceObj )
	{
		const PhysicsBehavior *sourcePhysics = sourceObj->getPhysics();
		PhysicsBehavior *objectPhysics = obj->getPhysics();
		if( sourcePhysics && objectPhysics )
		{
			objectPhysics->applyForce( sourcePhysics->getVelocity() );
		}
	}

	if( BitIsSet( data->m_disposition, LIKE_EXISTING ) )
	{
		if (mtx && !BitIsSet(data->m_disposition, ALIGN_Z_UP))
			obj->setTransformMatrix(mtx);
		else
			obj->setOrientation(orientation);
		obj->setPosition(&chunkPos);
		if (sourceObj && sourceObj->isAboveTerrain())
		{
			PhysicsBehavior* physics = obj->getPhysics();
			if (physics)
				physics->setAllowToFall(true);
		}

	//Lorenzen sez:
	//Since the sneak attack is a structure created with an ocl, it bypasses a lot of the
	//goodness that it would have gotten from dozerAI::build( the normal way to make structures )
	// but, since it is a building... lets stamp it down in the pathfind map, here.
	if ( obj->isKindOf( KINDOF_STRUCTURE ) )
	{
		// Flatten the terrain underneath the object, then adjust to the flattened height. jba.
		TheTerrainLogic->flattenTerrain(obj);
		Coord3D adjustedPos = *obj->getPosition();
		adjustedPos.z = TheTerrainLogic->getGroundHeight(pos->x, pos->y);
		obj->setPosition(&adjustedPos);
		// Note - very important that we add to map AFTER we flatten terrain. jba.
		TheAI->pathfinder()->addObjectToPathfindMap( obj );

	}







	}

	if( BitIsSet( data->m_disposition, ON_GROUND_ALIGNED ) )
	{
		chunkPos.z = 99999.0f;
		PathfindLayerEnum layer = TheTerrainLogic->getHighestLayerForDestination(&chunkPos);
		obj->setOrientation(GameLogicRandomValueReal(0.0f, 2 * PI));
		chunkPos.z = TheTerrainLogic->getLayerHeight( chunkPos.x, chunkPos.y, layer );
		// ensure we are slightly above the bridge, to account for fudge & sloppy art
		if (layer != LAYER_GROUND)
			chunkPos.z += 1.0f;
		obj->setLayer(layer);
		obj->setPosition(&chunkPos);
	}

	if( BitIsSet( data->m_disposition, SEND_IT_OUT ) )
	{
		obj->setOrientation(GameLogicRandomValueReal(0.0f, 2 * PI));
		chunkPos.z = TheTerrainLogic->getGroundHeight( chunkPos.x, chunkPos.y );
		obj->setPosition(&chunkPos);
		PhysicsBehavior* objUp = obj->getPhysics();
		if (objUp)
		{

			objUp->setExtraFriction(data->m_extraFriction);

			Coord3D force;
			Real horizForce = 4.0f * data->m_dispositionIntensity;		// 2
			force.x = GameLogicRandomValueReal( -horizForce, horizForce );
			force.y = GameLogicRandomValueReal( -horizForce, horizForce );
			force.z = 0;

			objUp->applyForce(&force);
			if (data->m_orientInForceDirection)
				orientation = atan2(force.y, force.x);

		}
	}

	if( BitIsSet( data->m_disposition, SEND_IT_FLYING | SEND_IT_UP | RANDOM_FORCE ) )
	{
		if (mtx)
		{
			//DUMPMATRIX3D(mtx);
			obj->setTransformMatrix(mtx);
		}
		obj->setPosition(&chunkPos);
		//DUMPCOORD3D(&chunkPos);
		PhysicsBehavior* objUp = obj->getPhysics();
		if (objUp)
		{

			DEBUG_ASSERTCRASH(objUp->getMass() > 0.0f, ("Zero masses are not allowed for obj!"));

			objUp->setExtraBounciness(data->m_extraBounciness);
			objUp->setExtraFriction(data->m_extraFriction);
			objUp->setAllowBouncing(true);
			objUp->setBounceSound(&data->m_bounceSound);
			//DUMPREAL(data->m_extraBounciness);
			//DUMPREAL(data->m_extraFriction);

			// if omitted from INI, calc it based on intensity.
			Real spinRate		= data->m_spinRate >= 0.0f ? data->m_spinRate : (PI/32.0f) * data->m_dispositionIntensity;

			// Treat these as overrides.
			Real yawRate		= data->m_yawRate		>= 0.0f ? data->m_yawRate		: spinRate;
			Real rollRate		= data->m_rollRate	>= 0.0f ? data->m_rollRate	: spinRate;
			Real pitchRate	= data->m_pitchRate >= 0.0f ? data->m_pitchRate : spinRate;

			//DUMPREAL(spinRate);
			//DUMPREAL(yawRate);
			//DUMPREAL(rollRate);
			//DUMPREAL(pitchRate);

			Real yaw = GameLogicRandomValueReal( -yawRate, yawRate );
			Real roll = GameLogicRandomValueReal( -rollRate, rollRate );
			Real pitch = GameLogicRandomValueReal( -pitchRate, pitchRate );
			//DUMPREAL(yaw);
			//DUMPREAL(roll);
			//DUMPREAL(pitch);

			Coord3D force;
			if( BitIsSet( data->m_disposition, SEND_IT_FLYING ) )
			{
				Real horizForce = 4.0f * data->m_dispositionIntensity;		// 2
				Real vertForce = 3.0f * data->m_dispositionIntensity;		// 3
				force.x = GameLogicRandomValueReal( -horizForce, horizForce );
				force.y = GameLogicRandomValueReal( -horizForce, horizForce );
				force.z = GameLogicRandomValueReal( vertForce * 0.33f, vertForce );
				//DUMPREAL(horizForce);
				//DUMPREAL(vertForce);
				//DUMPCOORD3D(&force);
			}
			else if (BitIsSet(data->m_disposition, SEND_IT_UP) )
			{
				Real horizForce = 2.0f * data->m_dispositionIntensity;
				Real vertForce = 4.0f * data->m_dispositionIntensity;

				force.x = GameLogicRandomValueReal( -horizForce, horizForce );
				force.y = GameLogicRandomValueReal( -horizForce, horizForce );
				force.z = GameLogicRandomValueReal( vertForce * 0.75f, vertForce );
				//DUMPREAL(horizForce);
				//DUMPREAL(vertForce);
				//DUMPCOORD3D(&force);
			}
			else
			{
				calcRandomForce(data->m_minMag, data->m_maxMag, data->m_minPitch, data->m_maxPitch, &force);
				//DUMPREAL(data->m_minMag);
				//DUMPREAL(data->m_maxMag);
				//DUMPREAL(data->m_minPitch);
				//DUMPREAL(data->m_maxPitch);
				//DUMPCOORD3D(&force);
			}
			objUp->applyForce(&force);
			if (data->m_orientInForceDirection)
			{
				orientation = atan2(force.y, force.x);
			}
			//DUMPREAL(orientation);
			objUp->setAngles(orientation, 0, 0);
			objUp->setYawRate(yaw);
			objUp->setRollRate(roll);
			objUp->setPitchRate(pitch);
			//DUMPCOORD3D(objUp->getAcceleration());
			//DUMPCOORD3D(objUp->getVelocity());
			//DUMPMATRIX3D(obj->getTransformMatrix());

		}
	}
	if( BitIsSet( data->m_disposition, WHIRLING ) )
	{
		PhysicsBehavior* objUp = obj->getPhysics();
		if (objUp)
		{
			Real yaw = GameLogicRandomValueReal( -data->m_dispositionIntensity, data->m_dispositionIntensity );
			Real roll = GameLogicRandomValueReal( -data->m_dispositionIntensity, data->m_dispositionIntensity );
			Real pitch = GameLogicRandomValueReal( -data->m_dispositionIntensity, data->m_dispositionIntensity );

			objUp->setYawRate(yaw);
			objUp->setRollRate(roll);
			objUp->setPitchRate(pitch);
		}
	}

	if( BitIsSet( data->m_disposition, FLOATING ) )
	{
		static NameKeyType key = NAMEKEY( "FloatUpdate" );
		FloatUpdate *floatUpdate = (FloatUpdate *)obj->findUpdateModule( key );

		if( floatUpdate )
			floatUpdate->setEnabled( TRUE );

	}
}



// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void CreateObjectDie::crc( Xfer *xfer )
{

	// extend base class
	DieModule::crc( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void CreateObjectDie::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	DieModule::xfer( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void CreateObjectDie::loadPostProcess( void )
{

	// extend base class
	DieModule::loadPostProcess();

}

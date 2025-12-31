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
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#define DEFINE_MAXHEALTHCHANGETYPE_NAMES						// for TheMaxHealthChangeTypeNames[]
#define DEFINE_DISPOSITION_NAMES								// For DispositionNames[]

#include "GameLogic/Module/ReplaceObjectUpgrade.h"

#include "Common/Player.h"
#include "Common/ThingFactory.h"
#include "Common/ThingTemplate.h"
#include "GameLogic/AI.h"
#include "GameLogic/AIPathfind.h"
#include "GameLogic/ExperienceTracker.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/PartitionManager.h"
#include "GameLogic/ScriptEngine.h"
#include "GameLogic/TerrainLogic.h"
#include "GameLogic/Module/AIUpdate.h"
#include "GameLogic/Module/AssaultTransportAIUpdate.h"
#include "GameLogic/Module/BodyModule.h"
#include "GameLogic/Module/DozerAIUpdate.h"
#include "GameLogic/Module/FloatUpdate.h"
#include "GameLogic/Module/HijackerUpdate.h"
#include "GameLogic/Module/PhysicsUpdate.h"
#include "GameLogic/Module/StickyBombUpdate.h"
#include "GameLogic/Module/SupplyTruckAIUpdate.h"
#include "GameLogic/Module/CreateModule.h"
#include "GameLogic/Module/ContainModule.h"
#include "GameLogic/Module/CreateObjectDie.h"					// For DispositionNames
#include "GameLogic/Object.h"
#include "GameLogic/Weapon.h" // NoMaxShotsLimit
#include "GameClient/Drawable.h"
#include "GameClient/InGameUI.h"

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
void ReplaceObjectUpgradeModuleData::buildFieldParse(MultiIniFieldParse& p)
{
  UpgradeModuleData::buildFieldParse(p);

	static const FieldParse dataFieldParse[] =
	{
		{ "ReplaceObject",	INI::parseAsciiString,	NULL, offsetof( ReplaceObjectUpgradeModuleData, m_replaceObjectName ) },

		{ "TransferHealth",	INI::parseBool,	NULL, offsetof( ReplaceObjectUpgradeModuleData, m_transferHealth ) },
		{ "TransferAIStates",	INI::parseBool,	NULL, offsetof( ReplaceObjectUpgradeModuleData, m_transferAIStates ) },
		{ "TransferExperience",	INI::parseBool,	NULL, offsetof( ReplaceObjectUpgradeModuleData, m_transferExperience ) },
		{ "TransferAttackers",	INI::parseBool,	NULL, offsetof( ReplaceObjectUpgradeModuleData, m_transferAttack ) },
		{ "TransferStatuses",	INI::parseBool,	NULL, offsetof( ReplaceObjectUpgradeModuleData, m_transferStatus ) },
		{ "TransferWeaponBonuses",	INI::parseBool,	NULL, offsetof( ReplaceObjectUpgradeModuleData, m_transferWeaponBonus ) },
		{ "TransferDisabledType",	INI::parseBool,	NULL, offsetof( ReplaceObjectUpgradeModuleData, m_transferDisabledType ) },
		{ "TransferBombs",	INI::parseBool,	NULL, offsetof( ReplaceObjectUpgradeModuleData, m_transferBombs ) },
		{ "TransferHijackers",	INI::parseBool,	NULL, offsetof( ReplaceObjectUpgradeModuleData, m_transferHijackers ) },
		{ "TransferEquippers",	INI::parseBool,	NULL, offsetof( ReplaceObjectUpgradeModuleData, m_transferEquippers ) },
		{ "TransferParasites",	INI::parseBool,	NULL, offsetof( ReplaceObjectUpgradeModuleData, m_transferParasites ) },
		{ "TransferPassengers",	INI::parseBool,	NULL, offsetof( ReplaceObjectUpgradeModuleData, m_transferPassengers ) },
		{ "TransferToAssaultTransport",	INI::parseBool,	NULL, offsetof( ReplaceObjectUpgradeModuleData, m_transferToAssaultTransport ) },
		{ "TransferShieldedTargets",	INI::parseBool,	NULL, offsetof( ReplaceObjectUpgradeModuleData, m_transferShieldedTargets ) },
		{ "TransferShieldingTargets",	INI::parseBool,	NULL, offsetof( ReplaceObjectUpgradeModuleData, m_transferShieldingTargets ) },
		{ "TransferSelection",	INI::parseBool,	NULL, offsetof( ReplaceObjectUpgradeModuleData, m_transferSelection ) },
		{ "TransferSelectionDontClearGroup",	INI::parseBool,	NULL, offsetof( ReplaceObjectUpgradeModuleData, m_transferSelectionDontClearGroup ) },
		{ "TransferObjectName",	INI::parseBool,	NULL, offsetof( ReplaceObjectUpgradeModuleData, m_transferObjectName ) },
		{ "HealthTransferType",		INI::parseIndexList,		TheMaxHealthChangeTypeNames, offsetof( ReplaceObjectUpgradeModuleData, m_transferHealthChangeType ) },

		{ "OrientInForceDirection", INI::parseBool, NULL, offsetof(CreateObjectDieModuleData, m_orientInForceDirection) },
		{ "ExtraBounciness",				INI::parseReal,						NULL, offsetof( ReplaceObjectUpgradeModuleData, m_extraBounciness ) },
		{ "ExtraFriction",				parseFrictionPerSec,						NULL, offsetof( ReplaceObjectUpgradeModuleData, m_extraFriction ) },
		{ "Offset",						INI::parseCoord3D,				NULL, offsetof( ReplaceObjectUpgradeModuleData, m_offset ) },
		{ "Disposition",			INI::parseBitString32,			DispositionNames, offsetof( ReplaceObjectUpgradeModuleData, m_disposition ) },
		{ "DispositionIntensity",	INI::parseReal,						NULL,	offsetof( ReplaceObjectUpgradeModuleData, m_dispositionIntensity ) },
		{ "SpinRate",					INI::parseAngularVelocityReal,	NULL, offsetof(ReplaceObjectUpgradeModuleData, m_spinRate) },
		{ "YawRate",					INI::parseAngularVelocityReal,	NULL, offsetof(ReplaceObjectUpgradeModuleData, m_yawRate) },
		{ "RollRate",					INI::parseAngularVelocityReal,	NULL, offsetof(ReplaceObjectUpgradeModuleData, m_rollRate) },
		{ "PitchRate",				INI::parseAngularVelocityReal,	NULL, offsetof(ReplaceObjectUpgradeModuleData, m_pitchRate) },
		{ "MinForceMagnitude",	INI::parseReal,	NULL, offsetof(ReplaceObjectUpgradeModuleData, m_minMag) },
		{ "MaxForceMagnitude",	INI::parseReal,	NULL, offsetof(ReplaceObjectUpgradeModuleData, m_maxMag) },
		{ "MinForcePitch",	INI::parseAngleReal,	NULL, offsetof(ReplaceObjectUpgradeModuleData, m_minPitch) },
		{ "MaxForcePitch",	INI::parseAngleReal,	NULL, offsetof(ReplaceObjectUpgradeModuleData, m_maxPitch) },
		{ "DiesOnBadLand",	INI::parseBool, NULL, offsetof(ReplaceObjectUpgradeModuleData, m_diesOnBadLand) },

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
//-------------------------------------------------------------------------------------------------
void ReplaceObjectUpgrade::upgradeImplementation( )
{
	const ReplaceObjectUpgradeModuleData *data = getReplaceObjectUpgradeModuleData();
	const ThingTemplate* replacementTemplate = TheThingFactory->findTemplate(data->m_replaceObjectName);

	Object *me;

	Bool oldObjectSelected;
	Int oldObjectSquadNumber;
	Matrix3D myMatrix;
	Team* myTeam;

	AIUpdateInterface *ai;
	ObjectStatusMaskType prevStatus;

	{
		me = getObject();

		myMatrix = *me->getTransformMatrix();
		myTeam = me->getTeam();// Team implies player.  It is a subset.

		if (replacementTemplate == NULL)
		{
			DEBUG_ASSERTCRASH(replacementTemplate != NULL, ("No such object '%s' in ReplaceObjectUpgrade.", data->m_replaceObjectName.str()));
			return;
		}

		// IamInnocent - Edited for Selection To Not Clear Group
		Drawable* selectedDrawable = TheInGameUI->getFirstSelectedDrawable();
		oldObjectSelected = data->m_transferSelectionDontClearGroup ? me->getDrawable() && me->getDrawable()->isSelected() : selectedDrawable && selectedDrawable->getID() == me->getDrawable()->getID();
		oldObjectSquadNumber = me->getControllingPlayer()->getSquadNumberForObject(me);

		ai = me->getAI();
		prevStatus = me->getStatusBits();

		// Remove us first since occupation of cells is apparently not a refcount, but a flag.  If I don't remove, then the new
		// thing will be placed, and then on deletion I will remove "his" marks.
		//TheAI->pathfinder()->removeObjectFromPathfindMap(me);
		//TheGameLogic->destroyObject(me);
	}

	// Don't destroy us first, make us non-interactable
	{
		me->setStatus( MAKE_OBJECT_STATUS_MASK3( OBJECT_STATUS_NO_COLLISIONS, OBJECT_STATUS_MASKED, OBJECT_STATUS_UNSELECTABLE ) );
		me->leaveGroup();
		//if( ai )
		//{
			//By setting him to idle, we will prevent him from entering the target after this gets called.
		//	ai->aiIdle( CMD_FROM_AI );
		//}
		// remove from partition manager
		ThePartitionManager->unRegisterObject( me );
	}

	Object *replacementObject = TheThingFactory->newObject(replacementTemplate, myTeam);
	replacementObject->setTransformMatrix(&myMatrix);
	TheAI->pathfinder()->addObjectToPathfindMap( replacementObject );

	doDisposition(me, replacementObject);

	// Now onCreates were called at the constructor.  This magically created
	// thing needs to be considered as Built for Game specific stuff.
	for (BehaviorModule** m = replacementObject->getBehaviorModules(); *m; ++m)
	{
		CreateModuleInterface* create = (*m)->getCreate();
		if (!create)
			continue;
		create->onBuildComplete();
	}

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
					update->setTargetObject( replacementObject );
				else
					BombsMarkedForDestroy.push_back(obj->getID());
			}
		//}

		// Transfer attackers
		if (data->m_transferAttack)
		{
			AIUpdateInterface* aiInterface = obj->getAI();
			if (aiInterface)
				aiInterface->transferAttack(me->getID(), replacementObject->getID());

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
			DEBUG_ASSERTCRASH( obj, ("Contain list must not contain NULL element"));

			// Remove Passenger from current contain
			contain->removeFromContain( obj, false );

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

	// Transfer my experience
	if (data->m_transferExperience && replacementObject->getExperienceTracker())
	{
		VeterancyLevel v = me->getVeterancyLevel();
		replacementObject->getExperienceTracker()->setHighestExpOrLevel(me->getExperienceTracker()->getCurrentExperience(), v, FALSE);
	}

	// Assault Transport Matters, switching Transports
	if(data->m_transferToAssaultTransport && me->getAssaultTransportObjectID() != INVALID_ID)
	{
		me->removeMeFromAssaultTransport(replacementObject->getID());
	}

	// Transfer Statuses
	if( data->m_transferStatus )
	{
		replacementObject->setStatus( prevStatus );
		replacementObject->setCustomStatusFlags( me->getCustomStatus() );

		replacementObject->doObjectStatusChecks();

		replacementObject->transferStatusHelperData(me->getStatusHelperData());
		replacementObject->refreshStatusHelper();
	}

	// Transfer Weapon Bonuses
	if( data->m_transferWeaponBonus )
	{
		replacementObject->setWeaponBonusConditionFlags(me->getWeaponBonusCondition());
		replacementObject->setWeaponBonusConditionIgnoreClear(me->getWeaponBonusConditionIgnoreClear());
		replacementObject->setCustomWeaponBonusConditionFlags(me->getCustomWeaponBonusCondition());
		replacementObject->setCustomWeaponBonusConditionIgnoreClear(me->getCustomWeaponBonusConditionIgnoreClear());
		replacementObject->doWeaponBonusChange();

		replacementObject->transferTempWeaponBonusHelperData(me->getTempWeaponBonusHelperData());
		replacementObject->refreshTempWeaponBonusHelper();
	}

	// Transfer Disabled Type
	if(data->m_transferDisabledType)
	{
		replacementObject->setDisabledTint(me->getDisabledTint());
		replacementObject->setDisabledCustomTint(me->getDisabledCustomTint());
		replacementObject->transferDisabledTillFrame(me->getDisabledTillFrame());
	}

	// Shielded Objects
	if(data->m_transferShieldedTargets && me->testCustomStatus("SHIELDED_TARGET"))
	{
		me->clearCustomStatus("SHIELDED_TARGET");
		replacementObject->setCustomStatus("SHIELDED_TARGET");
		replacementObject->setShieldByTargetID(me->getShieldByTargetID(), me->getShieldByTargetType());
	}

	if(data->m_transferShieldingTargets)
		replacementObject->setShielding(me->getShieldingTargetID(), me->getShieldByTargetType());

	// Transfer Objects with HijackerUpdate module (Checks within the Object Function for approval)
	me->doTransferHijacker(replacementObject->getID(), data->m_transferHijackers, data->m_transferEquippers, data->m_transferParasites);

	// Transfer Object Name for Script Engine
	if (data->m_transferObjectName)
	{
		TheScriptEngine->transferObjectName( me->getName(), replacementObject );
	}

	// Originally an aspect of Disposition, but carry forward unto end of the function because the object may be killed from damage dealt
    if ( data->m_diesOnBadLand && replacementObject )
    {
	    // if we land in the water, we die. alas.
	    const Coord3D* riderPos = replacementObject->getPosition();
	    Real waterZ, terrainZ;
	    if (TheTerrainLogic->isUnderwater(riderPos->x, riderPos->y, &waterZ, &terrainZ)
			    && riderPos->z <= waterZ + 10.0f
			    && replacementObject->getLayer() == LAYER_GROUND)
	    {
		    // don't call kill(); do it manually, so we can specify DEATH_FLOODED
		    DamageInfo damageInfo;
		    damageInfo.in.m_damageType = DAMAGE_WATER;	// use this instead of UNRESISTABLE so we don't get a dusty damage effect
		    damageInfo.in.m_deathType = DEATH_FLOODED;
		    damageInfo.in.m_sourceID = INVALID_ID;
		    damageInfo.in.m_amount = HUGE_DAMAGE_AMOUNT;
		    replacementObject->attemptDamage( &damageInfo );
	    }

	    // Kill if materialized on impassable ground
	    Int cellX = REAL_TO_INT( replacementObject->getPosition()->x / PATHFIND_CELL_SIZE );
	    Int cellY = REAL_TO_INT( replacementObject->getPosition()->y / PATHFIND_CELL_SIZE );

	    PathfindCell* cell = TheAI->pathfinder()->getCell( replacementObject->getLayer(), cellX, cellY );
	    PathfindCell::CellType cellType = cell ? cell->getType() : PathfindCell::CELL_IMPASSABLE;

	    // If we land outside the map, we die too.
	    // Otherwise we exist outside the PartitionManger like a cheater.
	  if( replacementObject->isOffMap()
      || (cellType == PathfindCell::CELL_CLIFF)
      || (cellType == PathfindCell::CELL_WATER)
      || (cellType == PathfindCell::CELL_IMPASSABLE) )
	    {
		    // We are sorry, for reasons beyond our control, we are experiencing technical difficulties. Please die.
		    replacementObject->kill();
	    }

  // Note: for future enhancement of this feature, we should test the object against the cell type he is on,
  // using obj->getAI()->hasLocomotorForSurface( __ ). We cshould not assume here that the object can not
  // find happiness on cliffs or water or whatever.


    }

	if( data->m_transferHealth && replacementObject && !replacementObject->isEffectivelyDead() )
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

			// Then the Custom Subdual Damage
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

			if(oldHealth <= 0 && (data->m_transferHealthChangeType == PRESERVE_RATIO || data->m_transferHealthChangeType == SAME_CURRENTHEALTH ))
				replacementObject->kill(); // my that was easy
			else
			{
				switch( data->m_transferHealthChangeType )
				{
					case PRESERVE_RATIO:
					{
						//400/500 (80%) + 100 becomes 480/600 (80%)
						//200/500 (40%) - 100 becomes 160/400 (40%)
						Real ratio = oldHealth / oldMaxHealth;
						Real newHealth = newMaxHealth * ratio;
						newBody->internalChangeHealth( newHealth - newMaxHealth, TRUE );
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
						if(data->m_transferHealthChangeType == ADD_CURRENT_DAMAGE && fabs(oldHealth - oldMaxHealth) >= newMaxHealth)
							replacementObject->kill();
						else
							newBody->internalChangeHealth( max(1.0f - newMaxHealth, oldHealth - oldMaxHealth), TRUE );
						break;
					}
					case SAME_CURRENTHEALTH:
						//preserve past health amount
						newBody->internalChangeHealth( oldHealth - newMaxHealth, TRUE );
						break;
				}
			}
		}
	}


	// Now we destroy the Object
	TheAI->pathfinder()->removeObjectFromPathfindMap( me );
	TheGameLogic->destroyObject(me);

	if( replacementObject->getControllingPlayer() )
	{
		replacementObject->getControllingPlayer()->onStructureConstructionComplete(NULL, replacementObject, FALSE);

		// TheSuperHackers @bugfix Stubbjax 26/05/2025 If the old object was selected, select the new one.
		// IamInnocent 02/12/2025 - Integrated with Transfer Selection Feature added Below
		/*if (oldObjectSelected)
		{
			GameMessage* msg = TheMessageStream->appendMessage(GameMessage::MSG_CREATE_SELECTED_GROUP_NO_SOUND);
			msg->appendBooleanArgument(TRUE);
			msg->appendObjectIDArgument(replacementObject->getID());
			TheInGameUI->selectDrawable(replacementObject->getDrawable());
		}

		// TheSuperHackers @bugfix Stubbjax 26/05/2025 If the old object was grouped, group the new one.
		if (oldObjectSquadNumber != NO_HOTKEY_SQUAD)
		{
			if (replacementObject->isLocallyControlled())
			{
				GameMessage* msg = TheMessageStream->appendMessage((GameMessage::Type)(GameMessage::MSG_CREATE_TEAM0 + oldObjectSquadNumber));
				msg->appendObjectIDArgument(replacementObject->getID());
			}
		}*/

		// Transfer the Selection Status
		/// IamInnocent 02/12/2025 - Integrated with the selection module from TheSuperHackers @bugfix Stubbjax 26/05/2025
		if(data->m_transferSelection && oldObjectSelected)
		{
			GameMessage* msg = TheMessageStream->appendMessage(GameMessage::MSG_CREATE_SELECTED_GROUP_NO_SOUND);
			if(data->m_transferSelectionDontClearGroup)
			{
				msg->appendBooleanArgument(FALSE);
			}
			else
			{
				msg->appendBooleanArgument(TRUE);
			}

			msg->appendObjectIDArgument(replacementObject->getID());
			TheInGameUI->selectDrawable(replacementObject->getDrawable());

			// TheSuperHackers @bugfix Stubbjax 26/05/2025 If the old object was grouped, group the new one.
			if (oldObjectSquadNumber != NO_HOTKEY_SQUAD)
			{
				if (replacementObject->isLocallyControlled())
				{
					GameMessage* msg = TheMessageStream->appendMessage((GameMessage::Type)(GameMessage::MSG_CREATE_TEAM0 + oldObjectSquadNumber));
					msg->appendObjectIDArgument(replacementObject->getID());
				}
			}
		}

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
void ReplaceObjectUpgrade::doDisposition(Object *sourceObj, Object* obj)
{
	// Sanity
	if( obj == NULL )
		return;

	const ReplaceObjectUpgradeModuleData *data = getReplaceObjectUpgradeModuleData();

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
void ReplaceObjectUpgrade::crc( Xfer *xfer )
{

	// extend base class
	UpgradeModule::crc( xfer );

}

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

}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void ReplaceObjectUpgrade::loadPostProcess( void )
{

	// extend base class
	UpgradeModule::loadPostProcess();

}

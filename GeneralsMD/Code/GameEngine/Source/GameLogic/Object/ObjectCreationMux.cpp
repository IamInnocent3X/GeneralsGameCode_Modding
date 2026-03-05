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

// FILE: ObjectCreationMux.cpp ////////////////////////////////////////////////////////////////////////////
// Author: IamInnocent, March 2026
// Desc:	 A Mux that enables modules to Inherit or Receive attributes from its Source along with ObjectCreationList's disposition
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#define DEFINE_MAXHEALTHCHANGETYPE_NAMES						// for TheMaxHealthChangeTypeNames[]

#define NO_DEBUG_CRC

#include "Common/AudioEventRTS.h"
#include "Common/Player.h"
#include "Common/PlayerList.h"
#include "Common/ThingTemplate.h"
#include "Common/ThingFactory.h"

#include "GameClient/Drawable.h"
#include "GameClient/FXList.h"
#include "GameClient/ParticleSys.h"
#include "GameClient/InGameUI.h"

#include "GameLogic/ExperienceTracker.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/Module/BodyModule.h"
#include "GameLogic/Module/ContainModule.h"
#include "GameLogic/Module/FloatUpdate.h"
#include "GameLogic/Module/PhysicsUpdate.h"
#include "GameLogic/Object.h"
#include "GameLogic/ObjectCreationMux.h"
#include "GameLogic/ScriptEngine.h"
#include "GameLogic/Weapon.h"
#include "GameLogic/WeaponSet.h"

#include "GameLogic/Module/AIUpdate.h"
#include "GameLogic/Module/AssaultTransportAIUpdate.h"
#include "GameLogic/Module/DozerAIUpdate.h"
#include "GameLogic/Module/FloatUpdate.h"
#include "GameLogic/Module/HijackerUpdate.h"
#include "GameLogic/Module/PhysicsUpdate.h"
#include "GameLogic/Module/StickyBombUpdate.h"
#include "GameLogic/Module/SupplyTruckAIUpdate.h"

#include "GameLogic/AIPathfind.h"

#include "Common/CRCDebug.h"


//-------------------------------------------------------------------------------------------------
Bool ObjectCreationMux::checkIfDontHaveLivePlayer( const Object* sourceObj ) const
{
	return getCreationMuxData() && getCreationMuxData()->m_requiresLivePlayer && sourceObj && (!sourceObj->getControllingPlayer() || !sourceObj->getControllingPlayer()->isPlayerActive());
}

//-------------------------------------------------------------------------------------------------
Bool ObjectCreationMux::checkIfSkipWhileSignificantlyAirborne( const Object* sourceObj ) const
{
	return getCreationMuxData() && getCreationMuxData()->m_skipIfSignificantlyAirborne && sourceObj && sourceObj->isSignificantlyAboveTerrain();
}

//-------------------------------------------------------------------------------------------------
void ObjectCreationMux::setProducer( const Object *sourceObj, Object *obj, Object *container ) const
{
	// Sanity
	if(!sourceObj || !obj)
		return;

	// No creation mux data, set as default
	if(!getCreationMuxData())
	{
		obj->setProducer(sourceObj);
		return;
	}

	const ObjectCreationMuxData *data = getCreationMuxData();

	switch(data->m_setProducerType)
	{
		case SET_SOURCE_PRODUCER:
		{
			if(sourceObj->getProducerID() != INVALID_ID)
			{
				Object *producer = TheGameLogic->findObjectByID(sourceObj->getProducerID());
				if(producer)
				{
					obj->setProducer(producer);
					break;
				}
			}
			// allow fallthrough
		}
		case SET_CONTAINER:
		{
			if(container)
			{
				obj->setProducer(container);
				break;
			}
			// allow fallthrough
		}
		case SET_SOURCE:
		default:
			obj->setProducer(sourceObj);
			break;
	}
}

//-------------------------------------------------------------------------------------------------
void ObjectCreationMux::doPreserveLayer( const Object *sourceObj, Object *obj ) const
{
	// Sanity
	if(!sourceObj || !obj)
		return;

	// No creation mux data, do nothing
	if(!getCreationMuxData())
		return;

	const ObjectCreationMuxData *data = getCreationMuxData();

	if (data->m_preserveLayer)
	{
		PathfindLayerEnum layer = sourceObj->getLayer();
		if (layer != LAYER_GROUND)
			obj->setLayer(layer);
	}
}

//-------------------------------------------------------------------------------------------------
Object *ObjectCreationMux::getPutInContainer(Bool &requiresContainer, Team *owner) const
{
	// No creation mux data, do nothing
	if(!getCreationMuxData())
		return nullptr;

	const ObjectCreationMuxData *data = getCreationMuxData();

	if (!data->m_putInContainer.isEmpty())
	{
		requiresContainer = TRUE;
		const ThingTemplate* containerTmpl = TheThingFactory->findTemplate(data->m_putInContainer);
		if (containerTmpl)
		{
			Object *container = TheThingFactory->newObject( containerTmpl, owner );
			if( !container )
			{
				DEBUG_CRASH( ("ObjectCreationMux::getPutInContainer() failed to create container %s.", data->m_putInContainer.str() ) );
				return nullptr;
			}
			return container;
	//		//container->setProducer(sourceObj);
		}
	}
	return nullptr;
}

//-------------------------------------------------------------------------------------------------
void ObjectCreationMux::doObjectCreation( const Object *sourceObj, Object *obj ) const
{
	// Sanity
	if(!sourceObj || !obj)
		return;

	// No creation mux data, do nothing
	if(!getCreationMuxData())
		return;

	const ObjectCreationMuxData *data = getCreationMuxData();

	if (!data->m_particleSysName.isEmpty())
	{
		const ParticleSystemTemplate *tmp = TheParticleSystemManager->findTemplate(data->m_particleSysName);
		if (tmp)
		{
			ParticleSystem *sys = TheParticleSystemManager->createParticleSystem(tmp);
			sys->attachToObject(obj);
		}
	}

	if (data->m_ignorePrimaryObstacle)
	{
		PhysicsBehavior* p = obj->getPhysics();
		if (p)
			p->setIgnoreCollisionsWith(sourceObj);
	}

	BodyModuleInterface *body = obj->getBodyModule();
	Real healthPercent = GameLogicRandomValueReal( data->m_minHealth, data->m_maxHealth );
	if (body)
		body->setInitialHealth(healthPercent * 100.0f);

	if (data->m_experienceSink && sourceObj) {
		obj->getExperienceTracker()->setExperienceSink(sourceObj->getID());
	}


	if ( data->m_invulnerableTime > 0 )
	{
		obj->goInvulnerable( data->m_invulnerableTime );
	}
}

//-------------------------------------------------------------------------------------------------
void ObjectCreationMux::doInherit( const Object *sourceObj, Object *obj, ObjectStatusMaskType prevStatus ) const
{
	// Sanity
	if(!sourceObj || !obj)
		return;

	// No creation mux data, do nothing
	if(!getCreationMuxData())
		return;

	const ObjectCreationMuxData *data = getCreationMuxData();

	if (data->m_inheritsVeterancy && obj->getExperienceTracker()->isTrainable())
	{
		DEBUG_LOG(("Object %s inherits veterancy level %d from %s",
			obj->getTemplate()->getName().str(), sourceObj->getVeterancyLevel(), sourceObj->getTemplate()->getName().str()));
		VeterancyLevel v = sourceObj->getVeterancyLevel();
		obj->getExperienceTracker()->setVeterancyLevel(v);

		//In order to make things easier for the designers, we are going to transfer the unit name
		//to the ejected thing... so the designer can control the pilot with the scripts.
		if (!data->m_dontTransferObjectNameAfterInheritingVeterancy)
			TheScriptEngine->transferObjectName( sourceObj->getName(), obj );
	}

	if (data->m_inheritsWeaponBonus && sourceObj) {
		obj->setWeaponBonusConditionFlags(sourceObj->getWeaponBonusCondition());
		obj->setCustomWeaponBonusConditionFlags(sourceObj->getCustomWeaponBonusCondition());
		obj->setWeaponBonusConditionIgnoreClear(sourceObj->getWeaponBonusConditionIgnoreClear());
		obj->setCustomWeaponBonusConditionIgnoreClear(sourceObj->getCustomWeaponBonusConditionIgnoreClear());
		obj->doWeaponBonusChange();

		obj->transferTempWeaponBonusHelperData(sourceObj->getTempWeaponBonusHelperData());
		obj->refreshTempWeaponBonusHelper();
	}

	if( data->m_inheritsAIStates )
	{
		// Unconst_cast the source. Dirty hack
		Object *source = TheGameLogic->findObjectByID(sourceObj->getID()) ? TheGameLogic->findObjectByID(sourceObj->getID()) : nullptr;
		if(source)
		{
			AIUpdateInterface *ai = source->getAI();
			AIUpdateInterface *new_ai = obj->getAI();
			if( ai && new_ai )
			{
				//This flag determines if the object has started moving yet... if not
				//it's a good initial check.
				Bool isEffectivelyMoving = ai->isMoving() || ai->isWaitingForPath();

				//Are we trying to attack something. If so, we need to be in range before we can do so.
				Bool isTryingToAttack = ai->isAttacking();

				//Are we in guard mode? If so, are we idle... idle guarders deploy for fastest response against attackers.
				Bool isInGuardIdleState = ai->friend_isInGuardIdleState();

				// Inherits my Attack State
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
				// Inherits my Guard State
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
				// Inherits my Moving State
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

				// Inherits my Supply State
				SupplyTruckAIInterface* supplyTruckAI = ai->getSupplyTruckAIInterface();
				SupplyTruckAIInterface* supplyTruckNewAI = ai->getSupplyTruckAIInterface();
				if( supplyTruckAI && supplyTruckNewAI ) {
					// If it is gathering supplies, tell its replacer to do the same
					if (supplyTruckAI->isCurrentlyFerryingSupplies() || supplyTruckAI->isForcedIntoWantingState())
					{
						supplyTruckNewAI->setForceWantingState(true);
					}
				}

				// Inherits my Dozer State
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
	}

	// Inherits Statuses
	if( data->m_inheritsStatus )
	{
		obj->setStatus( sourceObj->getStatusBits() );
		obj->setCustomStatusFlags( sourceObj->getCustomStatus() );

		obj->doObjectStatusChecks();

		obj->transferStatusHelperData(sourceObj->getStatusHelperData());
		obj->refreshStatusHelper();
	}

	// Inherits Disabled Type
	if(data->m_inheritsDisabledType)
	{
		obj->setDisabledTint(sourceObj->getDisabledTint());
		obj->setDisabledCustomTint(sourceObj->getDisabledCustomTint());
		obj->transferDisabledTillFrame(sourceObj->getDisabledTillFrame());
	}
}

//-------------------------------------------------------------------------------------------------
void ObjectCreationMux::doTransfer( const Object *sourceObj, Object *obj, Bool isDestroyLater ) const
{
	// Sanity
	if(!sourceObj || !obj)
		return;

	// No creation mux data, do nothing
	if(!getCreationMuxData())
		return;

	const ObjectCreationMuxData *data = getCreationMuxData();
	std::vector<ObjectID> BombsMarkedForDestroy;

	// Transfer any bombs onto the created Object
	if(data->m_transferAttackers || data->m_transferBombs || data->m_destroyBombs)
	{
		Object *iterObj = TheGameLogic->getFirstObject();
		while( iterObj )
		{
			// Transfer bombs to the created Object
			//if( iterObj->isKindOf( KINDOF_MINE ) )
			//{
				//static NameKeyType key_StickyBombUpdate = NAMEKEY( "StickyBombUpdate" );
				//StickyBombUpdate *update = (StickyBombUpdate*)iterObj->findUpdateModule( key_StickyBombUpdate );
				StickyBombUpdateInterface *update = iterObj->getStickyBombUpdateInterface();
				if( update && update->getTargetObject() == sourceObj )
				{
					if(data->m_transferBombs)
						update->setTargetObject( obj );
					else if(data->m_destroyBombs)
						BombsMarkedForDestroy.push_back(iterObj->getID());
				}
			//}

			// Transfer attackers
			if ((data->m_inheritsHealth && !data->m_inheritsPreviousHealthDontTransferAttackers) || data->m_transferAttackers)
			{
				AIUpdateInterface* aiInterface = iterObj->getAI();
				if (aiInterface)
					aiInterface->transferAttack(sourceObj->getID(), obj->getID());

			}

			iterObj = iterObj->getNextObject();
		}
	}

	for(std::vector<ObjectID>::const_iterator it = BombsMarkedForDestroy.begin(); it != BombsMarkedForDestroy.end(); ++it)
	{
		Object *bomb = TheGameLogic->findObjectByID(*it);
		if(bomb)
			TheGameLogic->destroyObject(bomb);
	}

	if(data->m_transferPassengers && sourceObj->getContain())
	{
		// Get the unit's contain
		ContainModuleInterface *contain = sourceObj->getContain();

		std::vector<ObjectID>vecID;

		// Disable Enter/Exit Sounds
		contain->enableLoadSounds(FALSE);

		// Get Contain List
		ContainedItemsList list;
		contain->swapContainedItemsList(list);

		ContainedItemsList::iterator it = list.begin();
		while ( it != list.end() )
		{
			Object *contained = *it++;
			DEBUG_ASSERTCRASH( contained, ("Contain list must not contain null element"));

			// Remove Passenger from current contain
			contain->removeFromContain( contained, false );

			// Add the Passenger to the list to put into the new container later
			vecID.push_back(contained->getID());
		}

		ContainModuleInterface *newContain = obj->getContain();

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

		// Enable Enter/Exit Sounds for Previous Contain
		contain->enableLoadSounds(TRUE);

	}
	else if(isDestroyLater)
	{
		ContainModuleInterface *contain = sourceObj->getContain();

		if(contain)
		{
			contain->removeAllContained();
		}
	}

	// Unconst_cast the source. Dirty hack
	Object *source = TheGameLogic->findObjectByID(sourceObj->getID());
	if(!source)
		return;

	// Assault Transport Matters, switching Transports
	if(data->m_addToAssaultTransport && source && source->getAssaultTransportObjectID() != INVALID_ID)
	{
		source->addObjectIntoAssaultTransport(obj->getID(), isDestroyLater);
	}

	// Shielded Objects
	if(data->m_transferShieldedTargets && sourceObj->testCustomStatus("SHIELDED_TARGET"))
	{
		source->clearCustomStatus("SHIELDED_TARGET");
		obj->setCustomStatus("SHIELDED_TARGET");
		obj->setShieldByTargetID(sourceObj->getShieldByTargetID(), sourceObj->getShieldByTargetType());
	}

	if(data->m_transferShieldingTargets)
		obj->setShielding(sourceObj->getShieldingTargetID(), sourceObj->getShieldByTargetType());

	// Transfer Objects with HijackerUpdate module (Checks within the Object Function for approval)
	if(source)
		source->doTransferHijacker(obj->getID(), data->m_transferHijackers, data->m_transferEquippers, data->m_transferParasites, data->m_destroyHijackers, data->m_destroyParasites);

	// Transfer Object Name for Script Engine
	if (data->m_transferObjectName && (!data->m_inheritsVeterancy || data->m_dontTransferObjectNameAfterInheritingVeterancy || !obj->getExperienceTracker()->isTrainable()))
		TheScriptEngine->transferObjectName( source->getName(), obj );
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
static Real getGroundHeight(const Coord3D* pos, PathfindLayerEnum layer) {

	if (layer != LAYER_GROUND) {  // Bridge
		return TheTerrainLogic->getLayerHeight(pos->x, pos->y, layer) + 1.0;
	}
	else if (TheGlobalData->m_heightAboveTerrainIncludesWater) { // do water check
		Real waterZ = 0;
		Real terrainZ = 0;

		if (TheTerrainLogic->isUnderwater(pos->x, pos->y, &waterZ, &terrainZ))
			return waterZ;

		return terrainZ;
	}
	else {  // Ground height only
		return TheTerrainLogic->getLayerHeight(pos->x, pos->y, layer);
	}
}

//-------------------------------------------------------------------------------------------------
void ObjectCreationMux::doDisposition( const Object *sourceObj, Object *obj, const Coord3D *pos, const Matrix3D *mtx, Real orientation, Bool nameAreObjects ) const
{
	// Sanity
	if(!sourceObj || !obj)
		return;

	// No creation mux data, do nothing
	if(!getCreationMuxData())
		return;

	const ObjectCreationMuxData *data = getCreationMuxData();

	Coord3D offset = data->m_offset;
	if (mtx)
		adjustVector(&offset, mtx);

	Coord3D chunkPos;
	chunkPos.x = pos->x + offset.x;
	chunkPos.y = pos->y + offset.y;
	chunkPos.z = pos->z + offset.z;

	if( BitIsSet( data->m_disposition, INHERIT_VELOCITY ) )
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

		if (BitIsSet(data->m_disposition, ON_GROUND_ALIGNED)) {
			PathfindLayerEnum layer = TheTerrainLogic->getLayerForDestination(pos);
			chunkPos.z = getGroundHeight(pos, layer);
			obj->setLayer(layer);
		}
		obj->setPosition(&chunkPos);

		if (sourceObj->isAboveTerrain())
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

	else if( BitIsSet( data->m_disposition, ON_GROUND_ALIGNED ))
	{
		obj->setOrientation(GameLogicRandomValueReal(0.0f, 2 * PI));
		PathfindLayerEnum layer = TheTerrainLogic->getLayerForDestination(pos);
		chunkPos.z = getGroundHeight(pos, layer);
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

			if (!nameAreObjects)
				objUp->setMass( data->m_mass );

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
			DUMPMATRIX3D(mtx);
			obj->setTransformMatrix(mtx);
		}
		obj->setPosition(&chunkPos);
		DUMPCOORD3D(&chunkPos);
		PhysicsBehavior* objUp = obj->getPhysics();
		if (objUp)
		{

			if (!nameAreObjects)
			{
				DUMPREAL(data->m_mass);
				objUp->setMass( data->m_mass );
			}
			DEBUG_ASSERTCRASH(objUp->getMass() > 0.0f, ("Zero masses are not allowed for obj!"));

			objUp->setExtraBounciness(data->m_extraBounciness);
			objUp->setExtraFriction(data->m_extraFriction);
			objUp->setAllowBouncing(true);
			if (data->m_definedBounceSound)
				objUp->setBounceSound(&data->m_bounceSound);
			if (data->m_definedWaterImpactSound)
				objUp->setWaterImpactSound(&data->m_waterImpactSound);
			if (data->m_waterImpactFX)
				objUp->setWaterImpactFX(data->m_waterImpactFX);
			DUMPREAL(data->m_extraBounciness);
			DUMPREAL(data->m_extraFriction);

			// if omitted from INI, calc it based on intensity.
			Real spinRate		= data->m_spinRate >= 0.0f ? data->m_spinRate : (PI/32.0f) * data->m_dispositionIntensity;

			// Treat these as overrides.
			Real yawRate		= data->m_yawRate		>= 0.0f ? data->m_yawRate		: spinRate;
			Real rollRate		= data->m_rollRate	>= 0.0f ? data->m_rollRate	: spinRate;
			Real pitchRate	= data->m_pitchRate >= 0.0f ? data->m_pitchRate : spinRate;

			DUMPREAL(spinRate);
			DUMPREAL(yawRate);
			DUMPREAL(rollRate);
			DUMPREAL(pitchRate);

			Real yaw = GameLogicRandomValueReal( -yawRate, yawRate );
			Real roll = GameLogicRandomValueReal( -rollRate, rollRate );
			Real pitch = GameLogicRandomValueReal( -pitchRate, pitchRate );
			DUMPREAL(yaw);
			DUMPREAL(roll);
			DUMPREAL(pitch);

			Coord3D force;
			if( BitIsSet( data->m_disposition, SEND_IT_FLYING ) )
			{
				Real horizForce = 4.0f * data->m_dispositionIntensity;		// 2
				Real vertForce = 3.0f * data->m_dispositionIntensity;		// 3
				force.x = GameLogicRandomValueReal( -horizForce, horizForce );
				force.y = GameLogicRandomValueReal( -horizForce, horizForce );
				force.z = GameLogicRandomValueReal( vertForce * 0.33f, vertForce );
				DUMPREAL(horizForce);
				DUMPREAL(vertForce);
				DUMPCOORD3D(&force);
			}
			else if (BitIsSet(data->m_disposition, SEND_IT_UP) )
			{
				Real horizForce = 2.0f * data->m_dispositionIntensity;
				Real vertForce = 4.0f * data->m_dispositionIntensity;

				force.x = GameLogicRandomValueReal( -horizForce, horizForce );
				force.y = GameLogicRandomValueReal( -horizForce, horizForce );
				force.z = GameLogicRandomValueReal( vertForce * 0.75f, vertForce );
				DUMPREAL(horizForce);
				DUMPREAL(vertForce);
				DUMPCOORD3D(&force);
			}
			else
			{
				calcRandomForce(data->m_minMag, data->m_maxMag, data->m_minPitch, data->m_maxPitch, &force);
				DUMPREAL(data->m_minMag);
				DUMPREAL(data->m_maxMag);
				DUMPREAL(data->m_minPitch);
				DUMPREAL(data->m_maxPitch);
				DUMPCOORD3D(&force);
			}
			objUp->applyForce(&force);
			if (data->m_orientInForceDirection)
			{
				orientation = atan2(force.y, force.x);
			}
			DUMPREAL(orientation);
			objUp->setAngles(orientation, 0, 0);
			objUp->setYawRate(yaw);
			objUp->setRollRate(roll);
			objUp->setPitchRate(pitch);
			DUMPCOORD3D(objUp->getAcceleration());
			DUMPCOORD3D(objUp->getVelocity());
			DUMPMATRIX3D(obj->getTransformMatrix());

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

//-------------------------------------------------------------------------------------------------
void ObjectCreationMux::doPostDisposition( const Object *sourceObj, Object *obj ) const
{
	// Sanity
	if(!sourceObj || !obj)
		return;

	// No creation mux data, do nothing
	if(!getCreationMuxData())
		return;

	const ObjectCreationMuxData *data = getCreationMuxData();

	if( data->m_containInsideSourceObject )
	{
		// The Obj has been totally made, so stuff it inside ourselves if desired.
		if( sourceObj->getContain()  &&  sourceObj->getContain()->isValidContainerFor(obj, TRUE))
		{
			sourceObj->getContain()->addToContain( obj );

			// Need to hide if they are hidden.
			if( sourceObj->getDrawable() && obj->getDrawable() && sourceObj->getDrawable()->isDrawableEffectivelyHidden() )
				obj->getDrawable()->setDrawableHidden( TRUE );
		}
		else
		{
			DEBUG_CRASH(("A OCL with ContainInsideSourceObject failed the contain and is killing the new object."));
			// If we fail to contain it, we can't just leave it.  Stillborn it.
			TheGameLogic->destroyObject(obj);
		}
	}



    if ( data->m_diesOnBadLand && obj )
    {
	    // if we land in the water, we die. alas.
	    const Coord3D* riderPos = obj->getPosition();
	    Real waterZ, terrainZ;
	    if (TheTerrainLogic->isUnderwater(riderPos->x, riderPos->y, &waterZ, &terrainZ)
			    && riderPos->z <= waterZ + 10.0f
			    && obj->getLayer() == LAYER_GROUND)
	    {
		    // don't call kill(); do it manually, so we can specify DEATH_FLOODED
		    DamageInfo damageInfo;
		    damageInfo.in.m_damageType = DAMAGE_WATER;	// use this instead of UNRESISTABLE so we don't get a dusty damage effect
		    damageInfo.in.m_deathType = DEATH_FLOODED;
		    damageInfo.in.m_sourceID = INVALID_ID;
		    damageInfo.in.m_amount = HUGE_DAMAGE_AMOUNT;
		    obj->attemptDamage( &damageInfo );
	    }

	    // Kill if materialized on impassable ground
	    Int cellX = REAL_TO_INT( obj->getPosition()->x / PATHFIND_CELL_SIZE );
	    Int cellY = REAL_TO_INT( obj->getPosition()->y / PATHFIND_CELL_SIZE );

	    PathfindCell* cell = TheAI->pathfinder()->getCell( obj->getLayer(), cellX, cellY );
	    PathfindCell::CellType cellType = cell ? cell->getType() : PathfindCell::CELL_IMPASSABLE;

	    // If we land outside the map, we die too.
	    // Otherwise we exist outside the PartitionManger like a cheater.
	  if( obj->isOffMap()
      || (cellType == PathfindCell::CELL_CLIFF)
      || (cellType == PathfindCell::CELL_WATER)
      || (cellType == PathfindCell::CELL_IMPASSABLE) )
	    {
		    // We are sorry, for reasons beyond our control, we are experiencing technical difficulties. Please die.
		    obj->kill();
	    }

  // Note: for future enhancement of this feature, we should test the object against the cell type he is on,
  // using obj->getAI()->hasLocomotorForSurface( __ ). We cshould not assume here that the object can not
  // find happiness on cliffs or water or whatever.


    }

}

//-------------------------------------------------------------------------------------------------
void ObjectCreationMux::doInheritHealth( const Object *sourceObj, Object *obj ) const
{
	// Sanity
	if(!sourceObj || !obj)
		return;

	// Post disposition, object might be dead. Do nothing if object is dead
	if(obj->isEffectivelyDead())
		return;

	// No creation mux data, do nothing
	if(!getCreationMuxData())
		return;

	const ObjectCreationMuxData *data = getCreationMuxData();

	// Do this last because the object may be killed
	if( data->m_inheritsHealth )
	{
		//Convert old health to new health.
		BodyModuleInterface *oldBody = sourceObj->getBodyModule();
		BodyModuleInterface *newBody = obj->getBodyModule();
		if( oldBody && newBody )
		{
			//First inherits subdual damage
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
			obj->transferSubdualHelperData(sourceObj->getSubdualHelperData());
			obj->refreshSubdualHelper();

			//Now inherits the previous health from the old object to the new.
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

			if(oldHealth <= 0 && (data->m_inheritsHealthChangeType == PRESERVE_RATIO || data->m_inheritsHealthChangeType == SAME_CURRENTHEALTH ))
				obj->kill(); // my that was easy
			else
			{
				switch( data->m_inheritsHealthChangeType )
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
						if(data->m_inheritsHealthChangeType == ADD_CURRENT_DAMAGE && fabs(oldHealth - oldMaxHealth) >= newMaxHealth)
							obj->kill();
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

}

//-------------------------------------------------------------------------------------------------
void ObjectCreationMux::doInheritSelection( const Object *sourceObj, Object *obj, Bool bypassSourceCheck, Int oldSquadNumber ) const
{
	// Sanity
	if((!bypassSourceCheck && !sourceObj) || !obj)
		return;

	// Source needs to be selected
	if(!bypassSourceCheck && (!sourceObj->getDrawable() || !sourceObj->getDrawable()->isSelected()))
		return;

	// Object might be dead. Do nothing if object is dead
	if(obj->isEffectivelyDead())
		return;

	// Object needs to have a controlling player
	if(!obj->getControllingPlayer())
		return;

	// Object needs to be selectable
	if(!obj->getDrawable() || !obj->isSelectable())
		return;

	// No creation mux data, do nothing
	if(!getCreationMuxData())
		return;

	const ObjectCreationMuxData *data = getCreationMuxData();

	if (data->m_inheritsSelection)
	{
		//Object* oldObject = getObject();
		//Drawable* selectedDrawable = TheInGameUI->getFirstSelectedDrawable();
		//Bool oldObjectSelected = selectedDrawable && selectedDrawable->getID() == oldObject->getDrawable()->getID();

		//if (oldObjectSelected)
		GameMessage* msg = TheMessageStream->appendMessage(GameMessage::MSG_CREATE_SELECTED_GROUP_NO_SOUND);
		if(data->m_inheritsSelectionDontClearGroup)
		{
			msg->appendBooleanArgument(FALSE);
		}
		else
		{
			msg->appendBooleanArgument(TRUE);
		}

		msg->appendObjectIDArgument(obj->getID());
		TheInGameUI->selectDrawable(obj->getDrawable());

		Int squadNumber = oldSquadNumber;
		if(squadNumber == -1)
			squadNumber = sourceObj && sourceObj->getControllingPlayer() ? sourceObj->getControllingPlayer()->getSquadNumberForObject(sourceObj) : NO_HOTKEY_SQUAD;

		// TheSuperHackers @bugfix Stubbjax 26/05/2025 If the old object was grouped, group the new one.
		if (data->m_inheritsSquadNumber && squadNumber != NO_HOTKEY_SQUAD)
		{
			if (obj->isLocallyControlled())
			{
				GameMessage* msg = TheMessageStream->appendMessage((GameMessage::Type)(GameMessage::MSG_CREATE_TEAM0 + squadNumber));
				msg->appendObjectIDArgument(obj->getID());
			}
		}
	}
}


//-------------------------------------------------------------------------------------------------
void ObjectCreationMux::doFadeStuff( const Object *sourceObj ) const
{
	// Sanity
	if( !sourceObj )
		return;

	// No creation mux data, do nothing
	if(!getCreationMuxData())
		return;

	const ObjectCreationMuxData *data = getCreationMuxData();

	if (data->m_fadeIn)
	{
		AudioEventRTS fadeAudioEvent(data->m_fadeSoundName);
		fadeAudioEvent.setObjectID(sourceObj->getID());
		TheAudio->addAudioEvent(&fadeAudioEvent);
		sourceObj->getDrawable()->fadeIn(data->m_fadeFrames);
	}

	if (data->m_fadeOut)
	{
		AudioEventRTS fadeAudioEvent(data->m_fadeSoundName);
		fadeAudioEvent.setObjectID(sourceObj->getID());
		TheAudio->addAudioEvent(&fadeAudioEvent);
		sourceObj->getDrawable()->fadeOut(data->m_fadeFrames);
	}
}
		
//-------------------------------------------------------------------------------------------------
ObjectCreationMuxData::ObjectCreationMuxData() {
	m_destroyBombs = FALSE;
	m_destroyParasites = FALSE;
	m_destroyHijackers = FALSE;

	m_experienceSink = FALSE;

	m_inheritsHealth = FALSE;
	m_inheritsSelection = FALSE;
	m_inheritsExperience = FALSE;
	m_inheritsPreviousHealthDontTransferAttackers = TRUE;
	m_inheritsAIStates = FALSE;
	m_inheritsStatus = FALSE;
	m_inheritsWeaponBonus = FALSE;
	m_inheritsDisabledType = FALSE;
	m_inheritsSelectionDontClearGroup = TRUE;
	m_inheritsObjectName = FALSE;
	m_inheritsVeterancy = FALSE;

	m_inheritsHealthChangeType = SAME_CURRENTHEALTH;

	m_transferAttackers = FALSE;
	m_transferBombs = FALSE;
	m_transferHijackers = FALSE;
	m_transferEquippers = FALSE;
	m_transferParasites = FALSE;
	m_transferPassengers = FALSE;
	m_transferShieldedTargets = FALSE;
	m_transferShieldingTargets = FALSE;
	m_transferObjectName = FALSE;

	m_dontTransferObjectNameAfterInheritingVeterancy = TRUE;
	m_addToAssaultTransport = FALSE;

	m_setProducerType = SET_SOURCE;

	m_invulnerableTime = 0;
	m_fadeFrames = 0;

	m_putInContainer = AsciiString::TheEmptyString;
	m_particleSysName = AsciiString::TheEmptyString;
	m_fadeSoundName = AsciiString::TheEmptyString;

	m_mass = 0.0f;
	m_minHealth = 1.0f;
	m_maxHealth = 1.0f;
	m_extraBounciness = 0.0f;
	m_extraFriction = 0.0f;
	m_offset.zero();
	m_disposition = (DispositionType)0;
	m_dispositionIntensity = 0.0f;
	m_spinRate = -1.0f;
	m_yawRate = -1.0f;
	m_rollRate = -1.0f;
	m_pitchRate = -1.0f;
	m_minMag = 0.0f;
	m_maxMag = 0.0f;
	m_minPitch = 0.0f;
	m_maxPitch = 0.0f;
	m_waterImpactFX = nullptr;
	m_orientInForceDirection = FALSE;
	m_containInsideSourceObject = FALSE;
	m_fadeIn = FALSE;
	m_fadeOut = FALSE;
	m_ignorePrimaryObstacle = FALSE;
	m_requiresLivePlayer = FALSE;
	m_preserveLayer = TRUE;
	m_skipIfSignificantlyAirborne = FALSE;
	m_diesOnBadLand = FALSE;
	m_definedBounceSound = FALSE;
	m_definedWaterImpactSound = FALSE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void parseFrictionPerSec( INI* ini, void * /*instance*/, void *store, const void* /*userData*/ )
{
	Real fricPerSec = INI::scanReal(ini->getNextToken());
	Real fricPerFrame = fricPerSec * SECONDS_PER_LOGICFRAME_REAL;
	*(Real *)store = fricPerFrame;
}
//-------------------------------------------------------------------------------------------------
/*static*/ void ObjectCreationMuxData::parseBounceSound( INI* ini, void * instance, void *store, const void* /*userData*/ )
{
	ObjectCreationMuxData* self = (ObjectCreationMuxData*) instance;
	self->m_definedBounceSound = TRUE;
	INI::parseAudioEventRTS(ini, 0, store, 0);
}
//-------------------------------------------------------------------------------------------------
/*static*/ void ObjectCreationMuxData::parseWaterImpactSound( INI* ini, void * instance, void *store, const void* /*userData*/ )
{
	ObjectCreationMuxData* self = (ObjectCreationMuxData*) instance;
	self->m_definedWaterImpactSound = TRUE;
	INI::parseAudioEventRTS(ini, 0, store, 0);
}

//-------------------------------------------------------------------------------------------------
const FieldParse* ObjectCreationMuxData::getFieldParse()
{
	static const FieldParse dataFieldParse[] =
	{
		{ "ContainInsideSourceObject", INI::parseBool, nullptr, offsetof( ObjectCreationMuxData, m_containInsideSourceObject) },
		{ "InheritsVeterancy",	INI::parseBool, nullptr, offsetof(ObjectCreationMuxData, m_inheritsVeterancy) },
		{ "SkipIfSignificantlyAirborne", INI::parseBool, nullptr, offsetof(ObjectCreationMuxData, m_skipIfSignificantlyAirborne) },
		{ "InvulnerableTime",		INI::parseDurationUnsignedInt, nullptr, offsetof(ObjectCreationMuxData, m_invulnerableTime) },

		{ "PutInContainer", INI::parseAsciiString, nullptr, offsetof( ObjectCreationMuxData, m_putInContainer) },
		{ "ParticleSystem",		INI::parseAsciiString, nullptr, offsetof( ObjectCreationMuxData, m_particleSysName) },
		{ "IgnorePrimaryObstacle", INI::parseBool, nullptr, offsetof(ObjectCreationMuxData, m_ignorePrimaryObstacle) },
		{ "OrientInForceDirection", INI::parseBool, nullptr, offsetof(ObjectCreationMuxData, m_orientInForceDirection) },
		{ "Mass",										INI::parsePositiveNonZeroReal,			nullptr,					offsetof( ObjectCreationMuxData, m_mass ) },
		{ "ExtraBounciness",				INI::parseReal,						nullptr, offsetof( ObjectCreationMuxData, m_extraBounciness ) },
		{ "ExtraFriction",				parseFrictionPerSec,						nullptr, offsetof( ObjectCreationMuxData, m_extraFriction ) },
		{ "Offset",						INI::parseCoord3D,				nullptr, offsetof( ObjectCreationMuxData, m_offset ) },
		{ "Disposition",			INI::parseBitString32,			DispositionNames, offsetof( ObjectCreationMuxData, m_disposition ) },
		{ "DispositionIntensity",	INI::parseReal,						nullptr,	offsetof( ObjectCreationMuxData, m_dispositionIntensity ) },
		{ "SpinRate",					INI::parseAngularVelocityReal,	nullptr, offsetof(ObjectCreationMuxData, m_spinRate) },
		{ "YawRate",					INI::parseAngularVelocityReal,	nullptr, offsetof(ObjectCreationMuxData, m_yawRate) },
		{ "RollRate",					INI::parseAngularVelocityReal,	nullptr, offsetof(ObjectCreationMuxData, m_rollRate) },
		{ "PitchRate",				INI::parseAngularVelocityReal,	nullptr, offsetof(ObjectCreationMuxData, m_pitchRate) },
		{ "MinForceMagnitude",	INI::parseReal,	nullptr, offsetof(ObjectCreationMuxData, m_minMag) },
		{ "MaxForceMagnitude",	INI::parseReal,	nullptr, offsetof(ObjectCreationMuxData, m_maxMag) },
		{ "MinForcePitch",	INI::parseAngleReal,	nullptr, offsetof(ObjectCreationMuxData, m_minPitch) },
		{ "MaxForcePitch",	INI::parseAngleReal,	nullptr, offsetof(ObjectCreationMuxData, m_maxPitch) },
		{ "FadeIn",			INI::parseBool,	nullptr, offsetof(ObjectCreationMuxData, m_fadeIn) },
		{ "FadeOut",			INI::parseBool,	nullptr, offsetof(ObjectCreationMuxData, m_fadeOut) },
		{ "FadeTime",	INI::parseDurationUnsignedInt,	nullptr, offsetof(ObjectCreationMuxData, m_fadeFrames) },
		{ "FadeSound", INI::parseAsciiString, nullptr, offsetof( ObjectCreationMuxData, m_fadeSoundName) },
		{ "PreserveLayer", INI::parseBool, nullptr, offsetof( ObjectCreationMuxData, m_preserveLayer) },
		{ "DiesOnBadLand",	INI::parseBool, nullptr, offsetof(ObjectCreationMuxData, m_diesOnBadLand) },

		{ "BounceSound",						ObjectCreationMuxData::parseBounceSound,						nullptr,					offsetof( ObjectCreationMuxData, m_bounceSound) },
		{ "WaterImpactSound",				ObjectCreationMuxData::parseWaterImpactSound,						nullptr,					offsetof( ObjectCreationMuxData, m_waterImpactSound) },
		{ "WaterImpactFX",				  INI::parseFXList,										nullptr,					offsetof( ObjectCreationMuxData, m_waterImpactFX) },

		{ "MinHealth",					INI::parsePercentToReal, nullptr, offsetof(ObjectCreationMuxData, m_minHealth) },
		{ "MaxHealth",					INI::parsePercentToReal, nullptr, offsetof(ObjectCreationMuxData, m_maxHealth) },
		{ "RequiresLivePlayer",	INI::parseBool, nullptr, offsetof(ObjectCreationMuxData, m_requiresLivePlayer) },
		{ "InheritsWeaponBonus",	INI::parseBool, nullptr, offsetof(ObjectCreationMuxData, m_inheritsWeaponBonus) },
		{ "ExperienceSinkForCaller",	INI::parseBool, nullptr, offsetof(ObjectCreationMuxData, m_experienceSink) },

		{ "InheritsHealth",	INI::parseBool,	nullptr, offsetof( ObjectCreationMuxData, m_inheritsHealth ) },
		{ "InheritsAIStates",	INI::parseBool,	nullptr, offsetof( ObjectCreationMuxData, m_inheritsAIStates ) },
		{ "InheritsStatuses",	INI::parseBool,	nullptr, offsetof( ObjectCreationMuxData, m_inheritsStatus ) },
		{ "InheritsDisabledType",	INI::parseBool,	nullptr, offsetof( ObjectCreationMuxData, m_inheritsDisabledType ) },
		{ "InheritsSelection",	INI::parseBool,	nullptr, offsetof( ObjectCreationMuxData, m_inheritsSelection ) },
		{ "InheritsSelectionDontClearGroup", INI::parseBool, nullptr, offsetof( ObjectCreationMuxData, m_inheritsSelectionDontClearGroup ) },
		{ "InheritsSelectionSquadNumber", INI::parseBool, nullptr, offsetof( ObjectCreationMuxData, m_inheritsSquadNumber ) },
		{ "HealthInheritType",		INI::parseIndexList,		TheMaxHealthChangeTypeNames, offsetof( ObjectCreationMuxData, m_inheritsHealthChangeType ) },

		{ "DestroyBombs",	INI::parseBool,	nullptr, offsetof( ObjectCreationMuxData, m_destroyBombs ) },
		{ "DestroyHijackers",	INI::parseBool,	nullptr, offsetof( ObjectCreationMuxData, m_destroyHijackers ) },
		{ "DestroyParasites",	INI::parseBool,	nullptr, offsetof( ObjectCreationMuxData, m_destroyParasites ) },

		{ "TransferBombs",	INI::parseBool,	nullptr, offsetof( ObjectCreationMuxData, m_transferBombs ) },
		{ "TransferAttackers",	INI::parseBool,	nullptr, offsetof( ObjectCreationMuxData, m_transferAttackers ) },
		{ "TransferHijackers",	INI::parseBool,	nullptr, offsetof( ObjectCreationMuxData, m_transferHijackers ) },
		{ "TransferEquippers",	INI::parseBool,	nullptr, offsetof( ObjectCreationMuxData, m_transferEquippers ) },
		{ "TransferParasites",	INI::parseBool,	nullptr, offsetof( ObjectCreationMuxData, m_transferParasites ) },
		{ "TransferPassengers",	INI::parseBool,	nullptr, offsetof( ObjectCreationMuxData, m_transferPassengers ) },
		{ "TransferShieldedTargets",	INI::parseBool,	nullptr, offsetof( ObjectCreationMuxData, m_transferShieldedTargets ) },
		{ "TransferShieldingTargets",	INI::parseBool,	nullptr, offsetof( ObjectCreationMuxData, m_transferShieldingTargets ) },
		{ "TransferObjectName",	INI::parseBool,	nullptr, offsetof( ObjectCreationMuxData, m_transferObjectName ) },
		{ "DontTransferObjectNameAfterInheritingVeterancy",	INI::parseBool,	nullptr, offsetof( ObjectCreationMuxData, m_dontTransferObjectNameAfterInheritingVeterancy ) },

		{ "AddToAssaultTransport",	INI::parseBool,	nullptr, offsetof( ObjectCreationMuxData, m_addToAssaultTransport ) },
		{ nullptr, nullptr, nullptr, 0 } 
	};
  return dataFieldParse;
}

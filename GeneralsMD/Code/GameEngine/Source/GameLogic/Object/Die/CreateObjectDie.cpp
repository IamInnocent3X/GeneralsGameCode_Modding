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

#include "PreRTS.h"	// This must go first in EVERY cpp file int the GameEngine

#define DEFINE_OBJECT_STATUS_NAMES
#define DEFINE_MAXHEALTHCHANGETYPE_NAMES						// for TheMaxHealthChangeTypeNames[]
#define DEFINE_DISPOSITION_NAMES
#include "GameLogic/Module/AIUpdate.h"
#include "Common/ThingFactory.h"
#include "Common/Xfer.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/Module/CreateObjectDie.h"
#include "GameLogic/Object.h"
#include "GameLogic/ObjectCreationList.h"
#include "GameLogic/Module/BodyModule.h"


// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
CreateObjectDieModuleData::CreateObjectDieModuleData()
{

	m_ocl = NULL;
	m_transferPreviousHealth = FALSE;

	m_previousHealthChangeType = SAME_CURRENTHEALTH;

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

}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
/*static*/ void CreateObjectDieModuleData::buildFieldParse(MultiIniFieldParse& p)
{
	DieModuleData::buildFieldParse(p);

	static const FieldParse dataFieldParse[] =
	{
		{ "CreationList",	INI::parseObjectCreationList,		NULL,											offsetof( CreateObjectDieModuleData, m_ocl ) },
		{ "TransferPreviousHealth", INI::parseBool, NULL	,offsetof( CreateObjectDieModuleData, m_transferPreviousHealth ) },

		{ "TransferExperience",	INI::parseBool,	NULL, offsetof( ReplaceObjectUpgradeModuleData, m_transferExperience ) },
		{ "TransferAttackers",	INI::parseBool,	NULL, offsetof( ReplaceObjectUpgradeModuleData, m_transferAttack ) },
		{ "TransferStatuses",	INI::parseBool,	NULL, offsetof( ReplaceObjectUpgradeModuleData, m_transferStatus ) },
		{ "TransferWeaponBonuses",	INI::parseBool,	NULL, offsetof( ReplaceObjectUpgradeModuleData, m_transferWeaponBonus ) },
		{ "TransferBombs",	INI::parseBool,	NULL, offsetof( ReplaceObjectUpgradeModuleData, m_transferBombs ) },
		{ "TransferHijackers",	INI::parseBool,	NULL, offsetof( ReplaceObjectUpgradeModuleData, m_transferHijackers ) },
		{ "TransferParasites",	INI::parseBool,	NULL, offsetof( ReplaceObjectUpgradeModuleData, m_transferParasites ) },
		{ "TransferPassengers",	INI::parseBool,	NULL, offsetof( ReplaceObjectUpgradeModuleData, m_transferPassengers ) },
		{ "HealthTransferType",		INI::parseIndexList,		TheMaxHealthChangeTypeNames, offsetof( ReplaceObjectUpgradeModuleData, m_previousHealthChangeType ) },

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

		{ 0, 0, 0, 0 }
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

	//If we're transferring previous health, we're transfering the last known
	//health before we died. In the case of the sneak attack tunnel network, it
	//is killed after the lifetime update expires.
	if( newObject && data->m_transferPreviousHealth )
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

			//Now transfer the previous health from the old object to the new.
			//damInfo.in.m_amount = oldBody->getMaxHealth() - oldBody->getPreviousHealth();
			//damInfo.in.m_damageType = DAMAGE_UNRESISTABLE;
			//damInfo.in.m_sourceID = oldBody->getLastDamageInfo()->in.m_sourceID;
			//if( damInfo.in.m_amount > 0.0f )
			//{
			//	newBody->attemptDamage( &damInfo );
			//}

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

			switch( data->m_previousHealthChangeType )
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
					if(data->m_previousHealthChangeType == ADD_CURRENT_DAMAGE && fabs(oldHealth - oldMaxHealth) > newMaxHealth)
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

		//Transfer attackers.
		for( Object *obj = TheGameLogic->getFirstObject(); obj; obj = obj->getNextObject() )
		{
			AIUpdateInterface* ai = obj->getAI();
			if (!ai)
				continue;

			ai->transferAttack( oldObject->getID(), newObject->getID() );
		}
	}


}  // end onDie

void CreateObjectDie:doDisposition(Object *sourceObj, Object* obj)
{
	// Sanity
	if( obj == NULL )
		return;
	
	Matrix3D mtx = *sourceObj->getTransformMatrix();
	Coord3D offset = data->m_offset;
	Coord3D chunkPos = *sourceObj->getPosition();
	Real orientation = sourceObj->getOrientation();
	// Do nothing if vector is 0 or close to 0.
	if (fabs(offset.x) < WWMATH_EPSILON &&
		fabs(offset.y) < WWMATH_EPSILON &&
		fabs(offset.z) < WWMATH_EPSILON)
	else
	{
		if (mtx)
		{
			adjustVector(&offset, mtx);

			chunkPos.x += offset.x;
			chunkPos.y += offset.y;
			chunkPos.z += offset.z;
		}
	}
	
	const ReplaceObjectUpgradeModuleData *data = getReplaceObjectUpgradeModuleData();
	
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
			DUMPMATRIX3D(mtx);
			obj->setTransformMatrix(mtx);
		}
		obj->setPosition(&chunkPos);
		DUMPCOORD3D(&chunkPos);
		PhysicsBehavior* objUp = obj->getPhysics();
		if (objUp)
		{

			DEBUG_ASSERTCRASH(objUp->getMass() > 0.0f, ("Zero masses are not allowed for obj!"));

			objUp->setExtraBounciness(data->m_extraBounciness);
			objUp->setExtraFriction(data->m_extraFriction);
			objUp->setAllowBouncing(true);
			objUp->setBounceSound(&data->m_bounceSound);
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


// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void CreateObjectDie::crc( Xfer *xfer )
{

	// extend base class
	DieModule::crc( xfer );

}  // end crc

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

}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void CreateObjectDie::loadPostProcess( void )
{

	// extend base class
	DieModule::loadPostProcess();

}  // end loadPostProcess

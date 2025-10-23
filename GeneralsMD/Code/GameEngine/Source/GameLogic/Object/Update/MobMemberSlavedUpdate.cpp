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
// MobMemberSlavedUpdate.cpp ///////////////////////////////////////////////////////////////////////////
// Will obey spawner... or die trying
// Author: Mark Lorenzen, August 2002
// Desc:  Slaved unit(s) remain close to their master. Used by angry Mob members (various)
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "PreRTS.h"	// This must go first in EVERY cpp file int the GameEngine

#include "GameClient/InGameUI.h"// selection logic
#include "GameClient/Drawable.h"
#include "Common/RandomValue.h"
#include "Common/Xfer.h"
#include "GameClient/Drawable.h"
#include "GameClient/ParticleSys.h"
#include "GameLogic/AIPathfind.h"
#include "GameLogic/Damage.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/Locomotor.h"
#include "GameLogic/Object.h"
#include "GameLogic/PartitionManager.h"
#include "GameLogic/TerrainLogic.h"
#include "GameLogic/Module/AIUpdate.h"
#include "GameLogic/Module/BodyModule.h"
#include "GameLogic/Module/MobMemberSlavedUpdate.h"
#include "GameLogic/Module/SpawnBehavior.h"
#include "GameClient/InGameUI.h"// selection logic
#include "GameClient/Drawable.h"
#include "Common/ThingFactory.h"
#include "Common/ThingTemplate.h"





#define STRAY_MULTIPLIER 2.0f // Multiplier from stating diestance from tunnel, to max distance from
#define MOB_SLEEPY

const Real CLOSE_ENOUGH = 15;				// Our moveTo commands and pathfinding can't handle people in the way, so quit trying to hump someone on your spot
const Real CLOSE_ENOUGH_SQR = (CLOSE_ENOUGH * CLOSE_ENOUGH);
const Int FRAME_THRESHOLD = 16;

//-------------------------------------------------------------------------------------------------
MobMemberSlavedUpdate::MobMemberSlavedUpdate( Thing *thing, const ModuleData* moduleData ) : UpdateModule( thing, moduleData )
{

	m_slaver = INVALID_ID;
	m_framesToWait = GameLogicRandomValue(0,20);

	// MDC: moving to GameLogicRandomValue.  This does not need to be synced, but having it so makes searches *so* much nicer.
	m_personalColor.red = GameLogicRandomValueReal( 0.2f, 0.4f );
	m_personalColor.green = GameLogicRandomValueReal( 0.2f, 0.4f );
	m_personalColor.blue = GameLogicRandomValueReal( 0.2f, 0.4f );

//	Drawable *myDraw = getObject()->getDrawable();
//	if ( myDraw )
//		myDraw->colorTint( &m_personalColor );

	m_mobState = MOB_STATE_NONE;
	m_primaryVictimID = INVALID_ID;
	m_squirrellinessRatio = 0;
	m_isSelfTasking = FALSE;
	m_catchUpCrisisTimer = 0;

	m_masterlastCoord.zero();
	m_noAggregateHealth = FALSE;

	// MDC: moving to GameLogicRandomValue.  This does not need to be synced, but having it so makes searches *so* much nicer.
	//getObject()->getDrawable()->setInstanceScale(GameLogicRandomValueReal( 5.0f, 1.5f ));

}

//-------------------------------------------------------------------------------------------------
MobMemberSlavedUpdate::~MobMemberSlavedUpdate( void )
{
}

//-------------------------------------------------------------------------------------------------
void MobMemberSlavedUpdate::onObjectCreated()
{

	const MobMemberSlavedUpdateModuleData* data = getMobMemberSlavedUpdateModuleData();
	m_squirrellinessRatio = MIN(MAX_SQUIRRELLINESS, MAX(0, data->m_squirrellinessRatio));

}

//-------------------------------------------------------------------------------------------------
void MobMemberSlavedUpdate::onEnslave( const Object *slaver )
{
	startSlavedEffects( slaver );
}

//-------------------------------------------------------------------------------------------------
void MobMemberSlavedUpdate::onSlaverDie( const DamageInfo *info )
{
	stopSlavedEffects();
}

//-------------------------------------------------------------------------------------------------
void MobMemberSlavedUpdate::onSlaverDamage( const DamageInfo *info )
{
	// Only slaves with a ProneUpdate will even care.
	AIUpdateInterface *ai = getObject()->getAIUpdateInterface();
	if( ai )
		ai->aiGoProne( info, CMD_FROM_AI );
}


//-------------------------------------------------------------------------------------------------
UpdateSleepTime MobMemberSlavedUpdate::update( void )
{
/// @todo srj use SLEEPY_UPDATE here
/// IamInnocent - Done

	const MobMemberSlavedUpdateModuleData* data = getMobMemberSlavedUpdateModuleData();
	Object *me = getObject();
	if( !me )
	{
		return calcSleepTime();
	}

	Object *master = TheGameLogic->findObjectByID( m_slaver );
	if( master == NULL )
	{
		stopSlavedEffects();

		//TheGameLogic->destroyObject( me );
		me->kill();
		return calcSleepTime();	// you cannot return SLEEP_FOREVER unless you make yourself sleepy...
	}

	AIUpdateInterface *myAI = me->getAIUpdateInterface();
	AIUpdateInterface *masterAI = master->getAIUpdateInterface();
	if( ! myAI || ! masterAI)
	{
		return calcSleepTime();
	}

	Drawable *myDraw = me->getDrawable();
	Drawable *masterDraw = master->getDrawable();
	if ( ! myDraw || ! masterDraw)
	{
		return calcSleepTime();
	}

//	myDraw->colorTint( &m_personalColor );


	const ModelConditionFlags flags = myDraw->getModelConditionFlags();
	if (flags.anyIntersectionWith(MAKE_MODELCONDITION_MASK(MODELCONDITION_WEAPONSET_PLAYER_UPGRADE)))
	{
		ModelConditionFlags clearFlags;
		clearFlags.clear();
		clearFlags.set( MODELCONDITION_RELOADING_A );
		clearFlags.set( MODELCONDITION_BETWEEN_FIRING_SHOTS_A );
		clearFlags.set( MODELCONDITION_PREATTACK_A );
		clearFlags.set( MODELCONDITION_FIRING_A );
		clearFlags.set( MODELCONDITION_USING_WEAPON_A );
		myDraw->clearModelConditionFlags( clearFlags );
	}

	// Update my Info to my Slaver so he doesn't need to aggregate through everything
	// Do it at the Slaver's Module instead.
	//informMySlaverSelfInfo();

	UnsignedInt now = TheGameLogic->getFrame();
	if( m_framesToWait > now )
		return UPDATE_SLEEP(m_framesToWait - now);
	//if ( ++m_framesToWait < FRAME_THRESHOLD)
	//	return UPDATE_SLEEP_NONE;

	// Just refreshed my update. No need to update again.
	if(m_framesToWait && !myAI->isMoving())
		me->setMobUpdateRefreshed(FALSE);

	//m_framesToWait = 0;
	m_framesToWait = now + FRAME_THRESHOLD;


	Locomotor *locomotor = myAI->getCurLocomotor();
	if( !locomotor )
	{
		return calcSleepTime();
	}

	// IamInnocent - If I am not moving, and my slaver is not moving, then we sleep until someone moves.
	const Coord3D *masterCoord = master->getPosition();

	if( !masterAI->isMoving() && 
		  !myAI->isMoving() &&
		  masterCoord->x == m_masterlastCoord.x &&
		  masterCoord->y == m_masterlastCoord.y &&
		  masterCoord->z == m_masterlastCoord.z )
	{
		m_framesToWait = 0;
		return calcSleepTime();
	}

	m_masterlastCoord.set( masterCoord );

	Object *victim = getObject()->getAIUpdateInterface()->getCurrentVictim();
	Object *masterVictim = master->getAIUpdateInterface()->getCurrentVictim();

	if (masterVictim)
	{
		m_primaryVictimID = masterVictim->getID();
	}

	Object *primaryVictim = TheGameLogic->findObjectByID(m_primaryVictimID);


	//now, we don't know if master is standing still or going somewhere, so
	Real masterPathDistToGoal = masterAI->getLocomotorDistanceToGoal();
	Real myPathDistToGoal = myAI->getLocomotorDistanceToGoal();


	Real catchUpRadiusSquared = ThePartitionManager->getDistanceSquared( me, master, FROM_CENTER_3D );
	// I'm too far from the nexus... I need to catch up now!
	if( catchUpRadiusSquared > sqr( data->m_mustCatchUpRadius ) )
	{

		if ( masterAI->isMoving() )// master is on the move
		{
			if ( masterPathDistToGoal > myPathDistToGoal ) // I'm getting ahead of master, need to slow down
				myAI->chooseLocomotorSet(LOCOMOTORSET_WANDER);
			else
				myAI->chooseLocomotorSet(LOCOMOTORSET_PANIC); // I'm lagging, so I need to snap to it!

			Coord3D nuPos = *masterAI->getGoalPosition();

			if ( nuPos.length() < 1.0f ) //if a nasty error has sent me to map origin
			{
				myAI->aiMoveToPosition( master->getPosition(), CMD_FROM_AI ); // NASTY BEEHIVE EFFECT
			}
			else
			{
				Coord3D goalDelta = *myAI->getGoalPosition();
				goalDelta.sub( &nuPos );

				if ( goalDelta.length() > 5.0f * PATHFIND_CELL_SIZE_F )// only if I am not headed there already
				{
					myAI->aiMoveToPosition( &nuPos, CMD_FROM_AI ); // Whither thou goest... THis causes the mob to reconverge
				}
			}

																															// on the fly, instead of doubling back to reconverge
		}
		else // master is still, so let's re group in a hurry
		{
			myAI->chooseLocomotorSet(LOCOMOTORSET_PANIC);
			myAI->aiMoveToPosition( master->getPosition(), CMD_FROM_AI );

		}



		if (catchUpRadiusSquared > sqr( data->m_mustCatchUpRadius * 3))// I am critically far, now!
		{
			++ m_catchUpCrisisTimer; // I'm way too far from the nexus this frame

			if ( m_catchUpCrisisTimer > data->m_catchUpCrisisBailTime)
			{
				me->kill();
				return calcSleepTime();

				// Here is the rethink:
				// If the nexus has outrun me to the target by so much, //
				// lets make the nexus come to me, and try to return to where I find it
//				Coord3D masterPosition = *master->getPosition();
//				master->setPosition( me->getPosition() );
//				if ( ! masterAI->isMoving() )
//				{
//					masterAI->aiMoveToPosition( &masterPosition, CMD_FROM_AI );
//				}
//				m_catchUpCrisisTimer = 0;
			}
			else if ( m_catchUpCrisisTimer > data->m_catchUpCrisisBailTime/3 )
			{
				myAI->aiMoveToPosition( master->getPosition(), CMD_FROM_AI ); // NASTY BEEHIVE EFFECT
			}
		}


	}
	else if ( myAI->isMoving() ) // we're all on a trip, together
	{

		m_catchUpCrisisTimer = 0; // I'm not too far from the nexus this frame

		Int seed = GameLogicRandomValue( 0, 10 );
		if ( seed == 1 )
			myAI->chooseLocomotorSet(LOCOMOTORSET_WANDER);
		else if ( seed == 2 )
			myAI->chooseLocomotorSet(LOCOMOTORSET_PANIC);
		else if ( seed == 3 )
			myAI->chooseLocomotorSet(LOCOMOTORSET_NORMAL);
	//	else if ( seed >= 5 ) // go towards mommy's goal
	//	{
	//		Coord3D destination = *me->getPosition();
	//		TheAI->pathfinder()->adjustToPossibleDestination(me, myAI->getLocomotorSet(), &destination);
	//		myAI->aiMoveToPosition( &destination, CMD_FROM_AI ); // reconverge
	//	}

	}
	else // give me something to do while I'm standing here...
	{
		m_catchUpCrisisTimer = 0; // I'm not too far from the nexus this frame

		SpawnBehaviorInterface *spawnerBehavior = master->getSpawnBehaviorInterface();

		if ( spawnerBehavior ) // if I have a mommy
		{

			if ( masterAI->isIdle() ) // if controlling player has pressed stop, we stop! That's it!
			{
				myAI->aiIdle(CMD_FROM_AI);
				primaryVictim = NULL;
				m_primaryVictimID = INVALID_ID;
				m_framesToWait = 0;
				return calcSleepTime();
			}

			if ( spawnerBehavior->maySpawnSelfTaskAI( m_squirrellinessRatio ) ) // if mommy says it is okay
			{
				if ( myAI->getLastCommandSource() != CMD_FROM_AI ) // I may have been told to attack directly more recently
				{
					Object *newTarget = myAI->getNextMoodTarget( FALSE, FALSE );
					if ( newTarget && ( newTarget != victim) ) // if there is someone else around to attack
					{
						victim = newTarget;
						myAI->aiAttackObject( newTarget, 999, CMD_FROM_AI ); // go ahead and do it
						//m_isSelfTasking = TRUE;
						informMySlaverSelfTasking(TRUE);
					}
				}

			}

			if ( ! victim ) // If I still don't have anyone to shoot at
			{
				if ( primaryVictim ) // I remember the last target
				{
					myAI->aiAttackObject( primaryVictim, 999, CMD_FROM_AI );
				}
				else if( ! masterAI->isAttacking())// there Is no previous target and master isn't attacking
				{
			///		myAI->aiIdle( CMD_FROM_AI );// auto acquire mode
				}
				//m_isSelfTasking = FALSE;
				informMySlaverSelfTasking(FALSE);
			}
		}
		else
		{
			DEBUG_ASSERTCRASH(( spawnerBehavior != NULL ),("Hey!, why for this mob member got no spawner? MLorenzen"));
		}
	}

	return m_framesToWait > now ? UPDATE_SLEEP(m_framesToWait - now) : calcSleepTime();
}

//-------------------------------------------------------------------------------------------------
UpdateSleepTime MobMemberSlavedUpdate::calcSleepTime()  const
{
#ifdef MOB_SLEEPY
	return UPDATE_SLEEP_FOREVER;
#else
	return UPDATE_SLEEP_NONE;
#endif
}


//-------------------------------------------------------------------------------------------------
// We are too far from nexus, so we need to catch-up
//-------------------------------------------------------------------------------------------------
void MobMemberSlavedUpdate::doCatchUpLogic( Coord3D *pos )
{
	Coord3D nuPos;
	const MobMemberSlavedUpdateModuleData* data = getMobMemberSlavedUpdateModuleData();
	// recalc where we want to be if we wander around
	Real randomDirection = GameLogicRandomValue( 0, 2*PI );
	Real randomRadius = GameLogicRandomValue( 0, data->m_noNeedToCatchUpRadius );
	nuPos.set(pos);
	nuPos.x += randomRadius * Cos( randomDirection );
	nuPos.y += randomRadius * Sin( randomDirection );
	nuPos.z = TheTerrainLogic->getGroundHeight( nuPos.x, nuPos.y );

	AIUpdateInterface *ai = getObject()->getAIUpdateInterface();
	if( ai )
	{
		ai->aiMoveToPosition( &nuPos, CMD_FROM_AI );
	}

	setMobState(MOB_STATE_CATCHING_UP);

}

//-------------------------------------------------------------------------------------------------
void MobMemberSlavedUpdate::informMySlaverSelfInfo()
{
	/*if( m_noAggregateHealth )
		return;

	Object *master = TheGameLogic->findObjectByID( m_slaver );
	if( master == NULL )
		return;
	
	SpawnBehaviorInterface *spawnerBehavior = master->getSpawnBehaviorInterface();

	if ( spawnerBehavior ) // if I have a mommy
	{
		Object *obj = getObject();
		BodyModuleInterface *body = obj->getBodyModule();
		m_noAggregateHealth = body ? !spawnerBehavior->informSlaveInfo(obj->getID(), body->getHealth(), body->getMaxHealth() ) : TRUE;
		if(!m_noAggregateHealth && !obj->getMobUpdateRefreshed())
			spawnerBehavior->friend_refreshUpdate();
	}
	else
	{
		m_noAggregateHealth = TRUE;
	}*/
}

//-------------------------------------------------------------------------------------------------
void MobMemberSlavedUpdate::informMySlaverSelfTasking(Bool set)
{
	if( m_noAggregateHealth )
		return;

	Bool isLastSelfTasking = m_isSelfTasking;
	m_isSelfTasking = set;

	// Same info, don't update. Updating is expensive.
	if(isLastSelfTasking == m_isSelfTasking)
		return;

	Object *master = TheGameLogic->findObjectByID( m_slaver );
	if( master == NULL )
		return;
	
	SpawnBehaviorInterface *spawnerBehavior = master->getSpawnBehaviorInterface();

	if ( spawnerBehavior ) // if I have a mommy
	{
		m_noAggregateHealth = !spawnerBehavior->informSelfTasking(getObject()->getID(), m_isSelfTasking);
	}
	else
	{
		m_noAggregateHealth = TRUE;
	}
}




//-------------------------------------------------------------------------------------------------
void MobMemberSlavedUpdate::startSlavedEffects( const Object *slaver )
{
	if( slaver == NULL )
		return;

	m_slaver = slaver->getID();
	getObject()->setIsMobMember(TRUE);

	// mark selves as not selectable
	//getObject()->setStatus( OBJECT_STATUS_UNSELECTABLE );

}

//-------------------------------------------------------------------------------------------------
void MobMemberSlavedUpdate::stopSlavedEffects()
{
	m_slaver = INVALID_ID;
	getObject()->setIsMobMember(FALSE);

	/// @todo Just a thought.  Our Status bits on objects really need to be reference counts so you don't clear someone else's flag
	getObject()->clearStatus( MAKE_OBJECT_STATUS_MASK( OBJECT_STATUS_UNSELECTABLE ) );
	getObject()->clearDisabled( DISABLED_HELD );
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void MobMemberSlavedUpdate::crc( Xfer *xfer )
{

	// extend base class
	UpdateModule::crc( xfer );

}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void MobMemberSlavedUpdate::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	UpdateModule::xfer( xfer );

	// slaves
	xfer->xferObjectID( &m_slaver );

	// frames to wait
	xfer->xferInt( &m_framesToWait );

	// mob state
	xfer->xferUser( &m_mobState, sizeof( MobStates ) );

	// personal color
	xfer->xferRGBColor( &m_personalColor );

	// primary victim
	xfer->xferObjectID( &m_primaryVictimID );

	// squirrelliness ration
	xfer->xferReal( &m_squirrellinessRatio );

	// is self tasking
	xfer->xferBool( &m_isSelfTasking );

	// catch up crisis timer
  xfer->xferUnsignedInt( &m_catchUpCrisisTimer );

	// master last coordinates
	xfer->xferCoord3D( &m_masterlastCoord );

}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void MobMemberSlavedUpdate::loadPostProcess( void )
{

	// extend base class
	UpdateModule::loadPostProcess();

}  // end loadPostProcess

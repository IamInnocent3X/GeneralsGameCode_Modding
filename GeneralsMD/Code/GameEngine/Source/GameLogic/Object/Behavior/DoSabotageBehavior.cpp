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

// FILE: DoSabotageBehavior.cpp ///////////////////////////////////////////////////////////////////////
// Author: Lorenzen
///////////////////////////////////////////////////////////////////////////////////////////////////


// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine
//#include "Common/Player.h"
#include "Common/Xfer.h"
#include "GameClient/ParticleSys.h"
#include "GameLogic/Module/CollideModule.h"
#include "GameLogic/Module/DoSabotageBehavior.h"
#include "GameLogic/Module/ProductionUpdate.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/Object.h"
#include "GameLogic/PartitionManager.h"



//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
/*struct DoSabotagePlayerScanHelper
{
	KindOfMaskType m_kindOfToTest;
	Object *m_theGrantor;
};

static void checkForDoSabotage( Object *testObj, void *userData )
{
	DoSabotagePlayerScanHelper *helper = (DoSabotagePlayerScanHelper*)userData;

	if( testObj->isEffectivelyDead() )
		return;

	if( testObj->getControllingPlayer() == helper->m_theGrantor->getControllingPlayer() )
		return;

	if( testObj->isOffMap() )
		return;

	if( !testObj->isAnyKindOf(helper->m_kindOfToTest) )
		return;
}*/

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
DoSabotageBehavior::DoSabotageBehavior( Thing *thing, const ModuleData* moduleData ) : UpdateModule( thing, moduleData )
{
	const DoSabotageBehaviorModuleData *d = getDoSabotageBehaviorModuleData();

	m_radiusParticleSystemID = INVALID_PARTICLE_SYSTEM_ID;

  m_currentScanRadius = d->m_startRadius;
  m_doneObjs.clear();


  Object *obj = getObject();

	{
		if( d->m_radiusParticleSystemTmpl )
		{
			ParticleSystem *particleSystem;

			particleSystem = TheParticleSystemManager->createParticleSystem( d->m_radiusParticleSystemTmpl );
			if( particleSystem )
			{
				particleSystem->setPosition( obj->getPosition() );
				m_radiusParticleSystemID = particleSystem->getSystemID();
			}
		}
	}

		setWakeFrame( getObject(), UPDATE_SLEEP_NONE );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
DoSabotageBehavior::~DoSabotageBehavior()
{

	if( m_radiusParticleSystemID != INVALID_PARTICLE_SYSTEM_ID )
		TheParticleSystemManager->destroyParticleSystemByID( m_radiusParticleSystemID );

}



//-------------------------------------------------------------------------------------------------
/** The update callback. */
//-------------------------------------------------------------------------------------------------
UpdateSleepTime DoSabotageBehavior::update()
{

	Object *self = getObject();

	if ( self->isEffectivelyDead())
		return UPDATE_SLEEP_FOREVER;

	const DoSabotageBehaviorModuleData *d = getDoSabotageBehaviorModuleData();
	// setup scan filters
	PartitionFilterRelationship relationship( self, PartitionFilterRelationship::ALLOW_ENEMIES |
																							PartitionFilterRelationship::ALLOW_NEUTRAL );
	PartitionFilterSameMapStatus filterMapStatus( self );
	PartitionFilterAlive filterAlive;
	PartitionFilter *filters[] = { &relationship, &filterAlive, &filterMapStatus, nullptr };


  m_currentScanRadius += d->m_radiusGrowRate;

  Bool thisIsFinalScan = FALSE;
  if ( m_currentScanRadius >=  d->m_finalRadius )
  {
    m_currentScanRadius = d->m_finalRadius;
    thisIsFinalScan = TRUE;
  }

	// scan objects in our region
	ObjectIterator *iter = ThePartitionManager->iterateObjectsInRange( self->getPosition(), m_currentScanRadius, FROM_CENTER_2D, filters );
	MemoryPoolObjectHolder hold( iter );
	// DO SABOTAGE TO ENEMIES IN RADIUS
	for( Object *obj = iter->first(); obj; obj = iter->next() )
    doSabotageToObject( obj );

  if ( thisIsFinalScan )
  {

    TheGameLogic->destroyObject( self );
    return UPDATE_SLEEP_FOREVER;
  }

  return UPDATE_SLEEP_NONE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void DoSabotageBehavior::doSabotageToObject( Object *obj )
{

  if ( obj == getObject() )
    return;


	const DoSabotageBehaviorModuleData *d = getDoSabotageBehaviorModuleData();
  if ( ! obj->isAnyKindOf( d->m_kindOf ) )
    return;

	for(std::vector<ObjectID>::const_iterator it = m_doneObjs.begin(); it != m_doneObjs.end(); ++it)
	{
		// Object has been sabotaged
		if(obj->getID() == (*it))
			return;
	}

	for (BehaviorModule** m = getObject()->getBehaviorModules(); *m; ++m)
	{
		CollideModuleInterface* collide = (*m)->getCollide();
		if (!collide)
			continue;

		if( collide->isSabotageBuildingCrateCollide() && collide->canDoSabotageSpecialCheck(obj) )
		{
			Drawable *draw = obj->getDrawable();
			if ( draw )
			{
				draw->flashAsSelected();
			}
			collide->doSabotage(obj, getObject());
		}
	}

	m_doneObjs.push_back(obj->getID());


}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void DoSabotageBehavior::crc( Xfer *xfer )
{

	// extend base class
	UpdateModule::crc( xfer );


}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void DoSabotageBehavior::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	UpdateModule::xfer( xfer );


	// particle system id
	xfer->xferUser( &m_radiusParticleSystemID, sizeof( ParticleSystemID ) );

	// Timer safety
	xfer->xferReal( &m_currentScanRadius );

	// Done objects
	UnsignedShort doneObjsCount = m_doneObjs.size();
	xfer->xferUnsignedShort( &doneObjsCount );
	ObjectID objID;
	if( xfer->getXferMode() == XFER_SAVE )
	{
		for(std::vector<ObjectID>::const_iterator it = m_doneObjs.begin(); it != m_doneObjs.end(); ++it)
		{
			objID = (*it);
			xfer->xferObjectID( &objID );
		}
	}
	else
	{
		// the vector must be empty now
		if( m_doneObjs.empty() == FALSE )
		{

			DEBUG_CRASH(( "DoSabotageBehavior::xfer - m_doneObjs should be empty but is not" ));
			//throw SC_INVALID_DATA;

		}

		for( UnsignedShort i = 0; i < doneObjsCount; ++i )
		{
			xfer->xferObjectID( &objID );

			// put in vector
			m_doneObjs.push_back( objID );

		}
	}

}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void DoSabotageBehavior::loadPostProcess()
{

	// extend base class
	UpdateModule::loadPostProcess();


}

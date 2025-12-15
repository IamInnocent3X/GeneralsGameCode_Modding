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

// FILE: ObjectLevitationHelper.cpp ///////////////////////////////////////////////////////////////
// Modified from: ObjectSMCHelper.cpp
// Author: Colin Day, Steven Johnson, September 2002
// Desc:   SMC helper
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"
#include "Common/Xfer.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/Object.h"
#include "GameLogic/TerrainLogic.h"
#include "GameLogic/Module/ObjectLevitationHelper.h"
#include "GameLogic/Module/PhysicsUpdate.h"

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
ObjectLevitationHelper::~ObjectLevitationHelper( void )
{
	m_magnetLevitateHeight = 0.0f;
	m_levitateFrame = 0;
	m_levitateFlutterFrame = 0;
	m_dontLevitate = FALSE;
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void ObjectLevitationHelper::doLevitation( Real height )
{
	m_magnetLevitateHeight = height;
	m_levitateFrame = TheGameLogic->getFrame() + LOGICFRAMES_PER_SECOND / 3;
	m_levitateFlutterFrame = 0;
	m_dontLevitate = FALSE;
	setWakeFrame(getObject(), UPDATE_SLEEP_NONE);
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
UpdateSleepTime ObjectLevitationHelper::update()
{
	Object *obj = getObject();
	UnsignedInt now = TheGameLogic->getFrame();

	if(m_magnetLevitateHeight)
	{
		// Apply levitate force
		if(m_levitateFrame >= now)
		{
			Coord3D newPos = *obj->getPosition();
			Real terrainZ = TheTerrainLogic->getGroundHeight( newPos.x, newPos.y );
			Bool doLevitate;
			if(newPos.z - terrainZ < m_magnetLevitateHeight)
			{
				doLevitate = TRUE;

				// Set the New Pos
				newPos.z = m_magnetLevitateHeight + terrainZ + 0.5f;
				obj->setPosition( &newPos );

				// Assign new value to apply the Upward Force
				newPos.zero();
				newPos.z = 12.5;
				obj->getPhysics()->applyForce( &newPos );
			}

			if(now == m_levitateFrame)
			{
				if(doLevitate)
				{
					obj->getPhysics()->resetDynamicPhysics();
					m_dontLevitate = TRUE;
				}
				//getPhysics()->scrubVelocity2D(0);
			}
		}
	}
	if(m_dontLevitate)
	{
		// Descending effect
		Coord3D flutterForce;
		flutterForce.zero();
		flutterForce.z = 0.35;
		obj->getPhysics()->applyForce( &flutterForce );

		if(!m_levitateFlutterFrame)
			m_levitateFlutterFrame = now + LOGICFRAMES_PER_SECOND;
		if(now > m_levitateFlutterFrame)
		{
			m_dontLevitate = FALSE;
			m_levitateFlutterFrame = 0;
			m_magnetLevitateHeight = 0;
		}
	}

	// Continue to update if levitation properties are present
	if(m_levitateFrame >= now || m_levitateFlutterFrame)
		return UPDATE_SLEEP_NONE;
	else
		return UPDATE_SLEEP_FOREVER;
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void ObjectLevitationHelper::crc( Xfer *xfer )
{

	// object helper crc
	ObjectHelper::crc( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info;
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void ObjectLevitationHelper::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// object helper base class
	ObjectHelper::xfer( xfer );
	
	xfer->xferReal( &m_magnetLevitateHeight );

	xfer->xferUnsignedInt( &m_levitateFrame );

	xfer->xferUnsignedInt( &m_levitateFlutterFrame );

	xfer->xferBool ( &m_dontLevitate );

}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void ObjectLevitationHelper::loadPostProcess( void )
{

	// object helper base class
	ObjectHelper::loadPostProcess();

}


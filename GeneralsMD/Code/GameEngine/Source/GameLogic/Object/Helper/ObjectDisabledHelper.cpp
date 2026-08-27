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

// FILE: ObjectDisabledHelper.cpp /////////////////////////////////////////////////////////////////
// Modified from: ObjectSMCHelper.cpp
// Author: Colin Day, Steven Johnson, September 2002
// Desc:   SMC helper
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"
#include "Common/Xfer.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/Object.h"
#include "GameLogic/Module/ObjectDisabledHelper.h"

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
ObjectDisabledHelper::~ObjectDisabledHelper()
{
	m_sleepUntil = 0;
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void ObjectDisabledHelper::setNextDisableUpdateFrame( UnsignedInt frame )
{

	UnsignedInt now = TheGameLogic->getFrame();
	if( frame > now && (now >= m_sleepUntil || m_sleepUntil > frame) )
		m_sleepUntil = frame;
		
	setWakeFrame(getObject(), UPDATE_SLEEP(m_sleepUntil > now ? m_sleepUntil - now : UPDATE_SLEEP_FOREVER));

}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
/*void ObjectDisabledHelper::checkDisableUpdateStatus()
{
	if ( m_sleepUntil <= TheGameLogic->getFrame() )
		getObject()->checkDisabledStatus();
}*/

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
UpdateSleepTime ObjectDisabledHelper::update()
{
	// if we ever get here, stop this.
	getObject()->checkDisabledStatus();

	// then go back to sleep until we are forcibly awakened.
	//return UPDATE_SLEEP_FOREVER;
	UnsignedInt now = TheGameLogic->getFrame();
	return UPDATE_SLEEP(m_sleepUntil > now ? m_sleepUntil - now : UPDATE_SLEEP_FOREVER);
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void ObjectDisabledHelper::crc( Xfer *xfer )
{

	// object helper crc
	ObjectHelper::crc( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info;
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void ObjectDisabledHelper::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// object helper base class
	ObjectHelper::xfer( xfer );

	// Sleep Until Time
	xfer->xferUnsignedInt( &m_sleepUntil );

}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void ObjectDisabledHelper::loadPostProcess()
{

	// object helper base class
	ObjectHelper::loadPostProcess();

}


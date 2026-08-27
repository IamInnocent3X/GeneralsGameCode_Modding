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

// FILE: ObjectSpecialPowerHelper.cpp ///////////////////////////////////////////////////////////////
// Modified from: ObjectSMCHelper.cpp
// Author: Colin Day, Steven Johnson, September 2002
// Desc:   SMC helper
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"
#include "Common/Xfer.h"
#include "Common/Player.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/Object.h"
#include "GameLogic/Module/ObjectSpecialPowerHelper.h"

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
ObjectSpecialPowerHelper::~ObjectSpecialPowerHelper()
{
	m_nextWakeUpTime = 0;
	m_countdownPausedUntilFrames.clear();
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void ObjectSpecialPowerHelper::setCountdownPausedUntil(UnsignedInt frame)
{
	UnsignedInt now = TheGameLogic->getFrame();

	// Clear an instance if its lower than the current frame
	if(now >= frame)
	{
		getObject()->pauseAllSpecialPowers(FALSE);
		return;
	}

	m_countdownPausedUntilFrames.push_back(frame);
	for(std::vector<UnsignedInt>::const_iterator it = m_countdownPausedUntilFrames.begin(); it != m_countdownPausedUntilFrames.end(); ++it)
	{
		UnsignedInt countFrame = (*it);
		if(!m_nextWakeUpTime || m_nextWakeUpTime > countFrame)
			m_nextWakeUpTime = countFrame;
	}

	setWakeFrame(getObject(), UPDATE_SLEEP(m_nextWakeUpTime - now));
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
UpdateSleepTime ObjectSpecialPowerHelper::update()
{
	UnsignedInt now = TheGameLogic->getFrame();
	if(m_nextWakeUpTime && now >= m_nextWakeUpTime)
	{
		m_nextWakeUpTime = 0;

		for(std::vector<UnsignedInt>::iterator it = m_countdownPausedUntilFrames.begin(); it != m_countdownPausedUntilFrames.end();)
		{
			UnsignedInt countFrame = (*it);
			if(now >= countFrame)
			{
				getObject()->pauseAllSpecialPowers(FALSE);
				it = m_countdownPausedUntilFrames.erase( it );
				continue;
			}
			if(!m_nextWakeUpTime || m_nextWakeUpTime > countFrame)
				m_nextWakeUpTime = countFrame;

			++it;
		}
	}

	return UPDATE_SLEEP(m_nextWakeUpTime > now ? (m_nextWakeUpTime - now) : UPDATE_SLEEP_FOREVER);
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void ObjectSpecialPowerHelper::crc( Xfer *xfer )
{

	// object helper crc
	ObjectHelper::crc( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info;
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void ObjectSpecialPowerHelper::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// object helper base class
	ObjectHelper::xfer( xfer );

	xfer->xferUnsignedInt( &m_nextWakeUpTime );

	UnsignedShort countdownPausedSize = m_countdownPausedUntilFrames.size();
	xfer->xferUnsignedShort( &countdownPausedSize );// HANDY LITTLE SHORT TO SIZE MY LIST
	UnsignedInt countdownFrame;
	if( xfer->getXferMode() == XFER_SAVE )
	{
		for( std::vector<UnsignedInt>::const_iterator it = m_countdownPausedUntilFrames.begin(); it != m_countdownPausedUntilFrames.end(); ++it )
		{
			countdownFrame = (*it);
			xfer->xferUnsignedInt( &countdownFrame );
		}
	}
	else
	{
		if( !m_countdownPausedUntilFrames.empty() ) // sanity, list must be empty right now
		{
			DEBUG_CRASH(( "ObjectSpecialPowerHelper::xfer - m_countdownPausedUntilFrames should be empty but is not" ));
			//throw SC_INVALID_DATA;
		}

		// read each entry
		for( UnsignedInt i = 0; i < countdownPausedSize; ++i )
		{
			xfer->xferUnsignedInt( &countdownFrame );
			m_countdownPausedUntilFrames.push_back(countdownFrame);
		}
	}

}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void ObjectSpecialPowerHelper::loadPostProcess()
{

	// object helper base class
	ObjectHelper::loadPostProcess();

}


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

// FILE: StatusDamageHelper.h ////////////////////////////////////////////////////////////////////////
// Author: Graham Smallwood, June 2003
// Desc:   Object helper - Clears Status conditions on a timer.
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"
#include "Common/Xfer.h"

#include "GameLogic/Module/StatusDamageHelper.h"

#include "GameClient/Drawable.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/Object.h"

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
StatusDamageHelper::StatusDamageHelper( Thing *thing, const ModuleData *modData ) : ObjectHelper( thing, modData ) 
{ 
	//m_statusToHeal = OBJECT_STATUS_NONE;
	m_frameToHeal = 0;
	//m_currentTint = TINT_STATUS_INVALID;
	//m_customStatusToHeal = NULL;

	m_earliestDurationAsInt = 0;
	m_statusToHeal.clear();
	m_currentTint.clear();
	m_customStatusToHeal.clear();
	m_customTintStatus.clear();

	setWakeFrame(getObject(), UPDATE_SLEEP_FOREVER);
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
StatusDamageHelper::~StatusDamageHelper( void )
{

}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
UpdateSleepTime StatusDamageHelper::update()
{
	UnsignedInt now = TheGameLogic->getFrame();
	UnsignedInt nextStatusDuration = 1e9; //Approximate, but lower than UPDATE_SLEEP_FOREVER
	for(StatusTypeMap::iterator it = m_statusToHeal.begin(); it != m_statusToHeal.end(); ++it)
	{
		if((*it).second == 0)
			continue;

		if((*it).second <= now)
		{
			getObject()->clearStatus( MAKE_OBJECT_STATUS_MASK((*it).first) );
			(*it).second = 0;
			//DEBUG_LOG(( "StatusDamageHelper Cleared Status: %d. Frame: %d", (*it).first, now ));

			//Fixes bugs for not checking after clearing Statuses
			getObject()->doObjectStatusChecks();
		}
		else if((*it).second < now + nextStatusDuration)
		{
			nextStatusDuration = (*it).second - now;
			//DEBUG_LOG(( "StatusDamageHelper Status To Clear: %d, Frame to Clear: %d", (*it).first, (*it).second ));
		}
	}
	for(CustomStatusTypeMap::iterator it2 = m_customStatusToHeal.begin(); it2 != m_customStatusToHeal.end(); ++it2)
	{
		if((*it2).second == 0)
			continue;
			
		if((*it2).second <= now)
		{
			getObject()->clearCustomStatus( (*it2).first );
			(*it2).second = 0;
			//DEBUG_LOG(( "StatusDamageHelper Cleared Custom Status: %s. Frame: %d", (*it2).first.str(), now ));

			//Fixes bugs for not checking after clearing Statuses
			getObject()->doObjectStatusChecks();
		}
		else if((*it2).second < now + nextStatusDuration)
		{
			nextStatusDuration = (*it2).second - now;
			//DEBUG_LOG(( "StatusDamageHelper Custom Status To Clear: %s, Frame to Clear: %d", (*it2).first.str(), (*it2).second ));
		}
	}

	if (getObject()->getDrawable())
	{
		for(TintStatusDurationVec::iterator it = m_currentTint.begin(); it != m_currentTint.end(); ++it)
		{
			if((*it).second == 0)
				continue;
			
			if((*it).second <= now)
			{
				getObject()->getDrawable()->clearTintStatus((*it).first);
				(*it).second = 0;
				//DEBUG_LOG(( "StatusDamageHelper Cleared Tint Status: %d. Frame: %d", (*it).first, now ));
			}
			else if((*it).second < now + nextStatusDuration)
			{
				nextStatusDuration = (*it).second - now;
				//DEBUG_LOG(( "StatusDamageHelper TintStatus To Clear: %d, Frame to Clear: %d", (*it).first, (*it).second ));
			}
		}
		for(CustomTintStatusDurationVec::iterator it2 = m_customTintStatus.begin(); it2 != m_customTintStatus.end(); ++it2)
		{
			if((*it2).second == 0)
				continue;
			
			if((*it2).second <= now)
			{
				getObject()->getDrawable()->clearCustomTintStatus((*it2).first);
				(*it2).second = 0;
				//DEBUG_LOG(( "StatusDamageHelper Cleared CustomTintStatus: %s. Frame: %d", (*it2).first.str(), now ));
			}
			else if((*it2).second < now + nextStatusDuration)
			{
				nextStatusDuration = (*it2).second - now;
				//DEBUG_LOG(( "StatusDamageHelper CustomTintStatus To Clear: %s, Frame to Clear: %d", (*it2).first.str(), (*it2).second ));
			}
		}
	}

	//DEBUG_LOG(( "StatusDamageHelper Update Frame: %d, Sleep Duration: %d, Next Frame to Clear Status: %d", now, nextStatusDuration, now + nextStatusDuration ));

	if(nextStatusDuration < 1e9)
	{
		m_earliestDurationAsInt = nextStatusDuration;
		return UPDATE_SLEEP(nextStatusDuration);
	}
	
	m_earliestDurationAsInt = 0;
	return UPDATE_SLEEP_FOREVER;

	//DEBUG_ASSERTCRASH(m_frameToHeal <= TheGameLogic->getFrame(), ("StatusDamageHelper woke up too soon.") );

	//clearStatusCondition(); // We are sleep driven, so seeing an update means our timer is ready implicitly
	//return UPDATE_SLEEP_FOREVER;
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void StatusDamageHelper::clearStatusCondition()
{
	//if( m_statusToHeal != OBJECT_STATUS_NONE )
	//{
	//	getObject()->clearStatus( MAKE_OBJECT_STATUS_MASK(m_statusToHeal) );
	//	m_statusToHeal = OBJECT_STATUS_NONE;
	//	m_frameToHeal = 0;
	//}
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void StatusDamageHelper::doStatusDamage( ObjectStatusTypes status, Real duration, const AsciiString& customStatus, const AsciiString& customTintStatus, TintStatus tintStatus )
{
	Int durationAsInt = REAL_TO_INT_FLOOR(duration);
	UnsignedInt frameToHeal = TheGameLogic->getFrame() + durationAsInt;
	
	// Clear any different status we may have.  Re-getting the same status will just reset the timer
	
	// Scratch that, we can now assign as many Statuses as we want!
	// Starting with custom status.
	if(!customStatus.isEmpty())
	{
		CustomStatusTypeMap::iterator it = m_customStatusToHeal.find(customStatus);
		if(it != m_customStatusToHeal.end())
		{
			(*it).second = frameToHeal;
		}
		else
		{
			m_customStatusToHeal[customStatus] = frameToHeal;
		}

		getObject()->setCustomStatus( customStatus );
	}
	// If custom status is not present, assign the status type instead!
	else
	{
		StatusTypeMap::iterator it = m_statusToHeal.find((UnsignedShort)status);
		if(it != m_statusToHeal.end())
		{
			(*it).second = frameToHeal;
		}
		else
		{
			m_statusToHeal[(UnsignedShort)status] = frameToHeal;
		}

		getObject()->setStatus( MAKE_OBJECT_STATUS_MASK(status) );
	}

	//m_frameToHeal = TheGameLogic->getFrame() + durationAsInt;

	// Now for the Tint Statuses
	if (getObject()->getDrawable())
	{
		Bool assignTintStatus = TRUE;

		if(!customTintStatus.isEmpty())
		{
			for(CustomTintStatusDurationVec::iterator it = m_customTintStatus.begin(); it != m_customTintStatus.end(); ++it)
			{
				if((*it).first == customTintStatus)
				{
					(*it).second = frameToHeal;
					assignTintStatus = FALSE;
					break;
				}
			}
			if(assignTintStatus == TRUE)
			{
				CustomTintStatusDurationPair statusPair;
				statusPair.first = customTintStatus;
				statusPair.second = frameToHeal;
				m_customTintStatus.push_back(statusPair);
			}
		
			getObject()->getDrawable()->setCustomTintStatus(customTintStatus);

		}
		else if (tintStatus > TINT_STATUS_INVALID && tintStatus < TINT_STATUS_COUNT) {
			for(TintStatusDurationVec::iterator it = m_currentTint.begin(); it != m_currentTint.end(); ++it)
			{
				if((*it).first == tintStatus)
				{
					(*it).second = frameToHeal;
					assignTintStatus = FALSE;
					break;
				}
			}
			if(assignTintStatus == TRUE)
			{
				TintStatusDurationPair statusPair;
				statusPair.first = tintStatus;
				statusPair.second = frameToHeal;
				m_currentTint.push_back(statusPair);
			}
			
			getObject()->getDrawable()->setTintStatus(tintStatus);

		}

		// getObject()->getDrawable()->setTintStatus(TINT_STATUS_FRENZY);

		//    if (getObject()->isKindOf(KINDOF_INFANTRY))
		//      getObject()->getDrawable()->setSecondMaterialPassOpacity( 1.0f );
	}

	//setWakeFrame(getObject(), UPDATE_SLEEP_NONE);

	// We set the wake frame for updating, because running for loops everyframe is expensive
	if(frameToHeal < m_frameToHeal || !m_earliestDurationAsInt)
	{
		m_earliestDurationAsInt = durationAsInt;
		m_frameToHeal = frameToHeal;
		setWakeFrame( getObject(), UPDATE_SLEEP(durationAsInt) );

		//DEBUG_LOG(( "StatusDamageHelper, Current Frame: %d, Sleep Duration: %d, Frame For Clearing Status: %d", TheGameLogic->getFrame(), durationAsInt, frameToHeal ));
	}
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void StatusDamageHelper::crc( Xfer *xfer )
{

	// object helper crc
	ObjectHelper::crc( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info;
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void StatusDamageHelper::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// object helper base class
	ObjectHelper::xfer( xfer );

	//xfer->xferUser( &m_statusToHeal, sizeof(ObjectStatusTypes) );// an enum
	//xfer->xferUser( &m_currentTint, sizeof(TintStatus) );// an enum
	//xfer->xferAsciiString( &m_customStatusToHeal );
	xfer->xferUnsignedInt( &m_frameToHeal );
	xfer->xferUnsignedInt( &m_earliestDurationAsInt );

	// Modified from W3DModelDraw.cpp
	TintStatusDurationPair currentTintInfo;
	CustomTintStatusDurationPair customTintStatusInfo;

	UnsignedShort currentTintSize = m_currentTint.size();
	UnsignedShort customTintStatusSize = m_customTintStatus.size();
	xfer->xferUnsignedShort( &currentTintSize );
	xfer->xferUnsignedShort( &customTintStatusSize );

	if( xfer->getXferMode() == XFER_SAVE )
	{

		TintStatusDurationVec::const_iterator it;
		for( it = m_currentTint.begin(); it != m_currentTint.end(); ++it )
		{

			currentTintInfo.first = (*it).first;
			xfer->xferUser( &currentTintInfo.first, sizeof(TintStatus) );// an enum

			currentTintInfo.second = (*it).second;
			xfer->xferUnsignedInt( &currentTintInfo.second );

		}  // end for, it

		CustomTintStatusDurationVec::const_iterator it2;
		for( it2 = m_customTintStatus.begin(); it2 != m_customTintStatus.end(); ++it2 )
		{

			customTintStatusInfo.first = (*it2).first;
			xfer->xferAsciiString( &customTintStatusInfo.first );

			customTintStatusInfo.second = (*it2).second;
			xfer->xferUnsignedInt( &customTintStatusInfo.second );

		}  // end for, it2

	}  // end if, save
	else
	{

		// the vector must be emtpy
		m_currentTint.clear();
		m_customTintStatus.clear();

		// read each data item
		for( UnsignedShort i = 0; i < currentTintSize; ++i )
		{

			xfer->xferUser( &currentTintInfo.first, sizeof(TintStatus) );// an enum

			xfer->xferUnsignedInt( &currentTintInfo.second );

			// stuff in vector
			m_currentTint.push_back( currentTintInfo );

		}  // end for, i

		for( UnsignedShort i_2 = 0; i_2 < customTintStatusSize; ++i_2 )
		{

			xfer->xferAsciiString( &customTintStatusInfo.first );

			xfer->xferUnsignedInt( &customTintStatusInfo.second );

			// stuff in vector
			m_customTintStatus.push_back( customTintStatusInfo );

		}  // end for, i

	}  // end else, load

	// Modified from Team.cpp
	StatusTypeMap::iterator statusIt;
	CustomStatusTypeMap::iterator customStatusIt;

	UnsignedShort statusCount = m_statusToHeal.size();
	UnsignedShort customStatusCount = m_customStatusToHeal.size();
	xfer->xferUnsignedShort( &statusCount );
	xfer->xferUnsignedShort( &customStatusCount );

	//ObjectStatusTypes statusName;
	UnsignedShort statusName; 
	UnsignedInt statusTime;
	AsciiString customStatusName;
	UnsignedInt customStatusTime;
	if( xfer->getXferMode() == XFER_SAVE )
	{

		for( statusIt = m_statusToHeal.begin(); statusIt != m_statusToHeal.end(); ++statusIt )
		{

			statusName = (*statusIt).first;
			//xfer->xferUser( &statusName, sizeof(ObjectStatusTypes) );// an enum
			xfer->xferUnsignedShort( &statusName );

			statusTime = (*statusIt).second;
			xfer->xferUnsignedInt( &statusTime );

		}  // end for

		for( customStatusIt = m_customStatusToHeal.begin(); customStatusIt != m_customStatusToHeal.end(); ++customStatusIt )
		{

			customStatusName = (*customStatusIt).first;
			xfer->xferAsciiString( &customStatusName );

			customStatusTime = (*customStatusIt).second;
			xfer->xferUnsignedInt( &customStatusTime );

		}  // end for

	}  // end if, save
	else
	{

		for( UnsignedShort i = 0; i < statusCount; ++i )
		{

			//xfer->xferUser( &statusName, sizeof(ObjectStatusTypes) );// an enum
			xfer->xferUnsignedShort( &statusName );

			xfer->xferUnsignedInt( &statusTime );

			m_statusToHeal[statusName] = statusTime;
			
		}  // end for, i

		for( UnsignedShort i_2 = 0; i_2 < customStatusCount; ++i_2 )
		{

			xfer->xferAsciiString( &customStatusName );

			xfer->xferUnsignedInt( &customStatusTime );

			m_customStatusToHeal[customStatusName] = customStatusTime;
			
		}  // end for, i

	}  // end else load

}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void StatusDamageHelper::loadPostProcess( void )
{

	// object helper base class
	ObjectHelper::loadPostProcess();

}


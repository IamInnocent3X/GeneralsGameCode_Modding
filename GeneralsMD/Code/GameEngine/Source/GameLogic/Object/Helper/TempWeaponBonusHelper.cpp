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

// FILE: TempWeaponBonusHelper.h ////////////////////////////////////////////////////////////////////////
// Author: Graham Smallwood, June 2003
// Desc:   Object helper - Clears Temporary weapon bonus effects
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"
#include "Common/Xfer.h"

#include "GameLogic/Module/TempWeaponBonusHelper.h"

#include "GameClient/Drawable.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/Object.h"
#include "GameLogic/Weapon.h"

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
TempWeaponBonusHelper::TempWeaponBonusHelper( Thing *thing, const ModuleData *modData ) : ObjectHelper( thing, modData )
{
	//m_currentBonus = WEAPONBONUSCONDITION_INVALID;
	//m_currentTint = TINT_STATUS_INVALID;
	m_frameToRemove = 0;
	//m_currentCustomBonus = NULL;

	m_earliestDurationAsInt = 0;
	m_bonusMap.clear();
	m_currentTint.clear();
	m_customBonusMap.clear();
	m_customTintStatus.clear();

	setWakeFrame(getObject(), UPDATE_SLEEP_FOREVER);
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
TempWeaponBonusHelper::~TempWeaponBonusHelper( void )
{

}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
UpdateSleepTime TempWeaponBonusHelper::update()
{
	UnsignedInt now = TheGameLogic->getFrame();
	UnsignedInt nextBonusDuration = 1e9; //Approximate, but lower than UPDATE_SLEEP_FOREVER
	for(StatusTypeMap::iterator it = m_bonusMap.begin(); it != m_bonusMap.end(); ++it)
	{
		if((*it).second == 0)
			continue;

		if((*it).second <= now)
		{
			getObject()->clearWeaponBonusCondition((WeaponBonusConditionType)(*it).first);
			(*it).second = 0;
			//DEBUG_LOG(( "TempWeaponBonusHelper Cleared Status: %d. Frame: %d", (*it).first, now ));
		}
		else if((*it).second < now + nextBonusDuration)
		{
			nextBonusDuration = (*it).second - now;
			//DEBUG_LOG(( "TempWeaponBonusHelper Status To Clear: %d, Frame to Clear: %d", (*it).first, (*it).second ));
		}
	}
	for(CustomStatusTypeMap::iterator it2 = m_customBonusMap.begin(); it2 != m_customBonusMap.end(); ++it2)
	{
		if((*it2).second == 0)
			continue;

		if((*it2).second <= now)
		{
			getObject()->clearCustomWeaponBonusCondition( (*it2).first );
			(*it2).second = 0;
			//DEBUG_LOG(( "TempWeaponBonusHelper Cleared Custom Status: %s. Frame: %d", (*it2).first.str(), now ));
		}
		else if((*it2).second < now + nextBonusDuration)
		{
			nextBonusDuration = (*it2).second - now;
			//DEBUG_LOG(( "TempWeaponBonusHelper Custom Status To Clear: %s, Frame to Clear: %d", (*it2).first.str(), (*it2).second ));
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
				//DEBUG_LOG(( "TempWeaponBonusHelper Cleared Tint Status: %d. Frame: %d", (*it).first, now ));
			}
			else if((*it).second < now + nextBonusDuration)
			{
				nextBonusDuration = (*it).second - now;
				//DEBUG_LOG(( "TempWeaponBonusHelper TintStatus To Clear: %d, Frame to Clear: %d", (*it).first, (*it).second ));
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
				//DEBUG_LOG(( "TempWeaponBonusHelper Cleared CustomTintStatus: %s. Frame: %d", (*it2).first.str(), now ));
			}
			else if((*it2).second < now + nextBonusDuration)
			{
				nextBonusDuration = (*it2).second - now;
				//DEBUG_LOG(( "TempWeaponBonusHelper CustomTintStatus To Clear: %s, Frame to Clear: %d", (*it2).first.str(), (*it2).second ));
			}
		}
	}

	//DEBUG_LOG(( "TempWeaponBonusHelper Update Frame: %d, Sleep Duration: %d, Next Frame to Clear Status: %d", now, nextBonusDuration, now + nextBonusDuration ));

	if(nextBonusDuration < 1e9)
	{
		m_earliestDurationAsInt = nextBonusDuration;
		return UPDATE_SLEEP(nextBonusDuration);
	}
	
	m_earliestDurationAsInt = 0;
	
	//DEBUG_ASSERTCRASH(m_frameToRemove <= TheGameLogic->getFrame(), ("TempWeaponBonusHelper woke up too soon.") );

	//clearTempWeaponBonus(); // We are sleep driven, so seeing an update means our timer is ready implicitly
	return UPDATE_SLEEP_FOREVER;
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void TempWeaponBonusHelper::clearTempWeaponBonus()
{
	/*if( m_currentBonus != WEAPONBONUSCONDITION_INVALID || !m_currentCustomBonus.isEmpty() )
	{
		if(m_currentBonus != WEAPONBONUSCONDITION_INVALID){
			getObject()->clearWeaponBonusCondition(m_currentBonus);
		}
		m_currentBonus = WEAPONBONUSCONDITION_INVALID;
		if(!m_currentCustomBonus.isEmpty()){
			getObject()->clearCustomWeaponBonusCondition(m_currentCustomBonus);
		}
		m_currentCustomBonus = NULL;
		m_frameToRemove = 0;

		if (getObject()->getDrawable())
		{
			if(!m_currentCustomTint.isEmpty())
			{
				getObject()->getDrawable()->clearCustomTintStatus(m_currentCustomTint);
				m_currentCustomTint = NULL;
			}
			if (m_currentTint > TINT_STATUS_INVALID && m_currentTint < TINT_STATUS_COUNT) {
				getObject()->getDrawable()->clearTintStatus(m_currentTint);
				m_currentTint = TINT_STATUS_INVALID;
			}

			// getObject()->getDrawable()->clearTintStatus(TINT_STATUS_FRENZY);
			//      if (getObject()->isKindOf(KINDOF_INFANTRY))
			//        getObject()->getDrawable()->setSecondMaterialPassOpacity( 0.0f );
		}
	}*/
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void TempWeaponBonusHelper::doTempWeaponBonus( WeaponBonusConditionType status, const AsciiString& customStatus, UnsignedInt duration, const AsciiString& customTintStatus, TintStatus tintStatus )
{
	Int durationAsInt = REAL_TO_INT_FLOOR(duration);
	UnsignedInt frameToRemove = TheGameLogic->getFrame() + durationAsInt;
	
	// Clear any different status we may have.  Re-getting the same status will just reset the timer
	
	// Scratch that, we can now assign as many Weapon Bonuses as we want!
	// Starting with custom status.
	if(!customStatus.isEmpty())
	{
		CustomStatusTypeMap::iterator it = m_customBonusMap.find(customStatus);
		if(it != m_customBonusMap.end())
		{
			(*it).second = frameToRemove;
		}
		else
		{
			m_customBonusMap[customStatus] = frameToRemove;
		}

		getObject()->setCustomWeaponBonusCondition(customStatus);
	}
	// If custom status is not present, assign the status type instead!
	else
	{
		StatusTypeMap::iterator it = m_bonusMap.find((UnsignedShort)status);
		if(it != m_bonusMap.end())
		{
			(*it).second = frameToRemove;
		}
		else
		{
			m_bonusMap[(UnsignedShort)status] = frameToRemove;
		}

		getObject()->setWeaponBonusCondition(status);
	}

	//m_frameToRemove = TheGameLogic->getFrame() + durationAsInt;

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
					(*it).second = frameToRemove;
					assignTintStatus = FALSE;
					break;
				}
			}
			if(assignTintStatus == TRUE)
			{
				CustomTintStatusDurationPair statusPair;
				statusPair.first = customTintStatus;
				statusPair.second = frameToRemove;
				m_customTintStatus.push_back(statusPair);
			}
		
			getObject()->getDrawable()->setCustomTintStatus(customTintStatus);

		}
		else if (tintStatus > TINT_STATUS_INVALID && tintStatus < TINT_STATUS_COUNT) {
			for(TintStatusDurationVec::iterator it = m_currentTint.begin(); it != m_currentTint.end(); ++it)
			{
				if((*it).first == tintStatus)
				{
					(*it).second = frameToRemove;
					assignTintStatus = FALSE;
					break;
				}
			}
			if(assignTintStatus == TRUE)
			{
				TintStatusDurationPair statusPair;
				statusPair.first = tintStatus;
				statusPair.second = frameToRemove;
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
	if(frameToRemove < m_frameToRemove || !m_earliestDurationAsInt)
	{
		m_earliestDurationAsInt = durationAsInt;
		m_frameToRemove = frameToRemove;
		setWakeFrame( getObject(), UPDATE_SLEEP(durationAsInt) );

		//DEBUG_LOG(( "TempWeaponBonusHelper, Current Frame: %d, Sleep Duration: %d, Frame For Clearing Status: %d", TheGameLogic->getFrame(), durationAsInt, frameToRemove ));
	}
	
	// Clear any different status we may have.  Re-getting the same status will just reset the timer
	/*if(!customStatus.isEmpty())
	{
		if( m_currentCustomBonus != customStatus )
			clearTempWeaponBonus();

		getObject()->setCustomWeaponBonusCondition(customStatus);
		m_currentCustomBonus = customStatus;
	}
	else
	{
		if( m_currentBonus != status )
			clearTempWeaponBonus();

		getObject()->setWeaponBonusCondition(status);
		m_currentBonus = status;
	}

	m_frameToRemove = TheGameLogic->getFrame() + duration;

	if (getObject()->getDrawable())
	{
		if(!customTintStatus.isEmpty())
		{
			getObject()->getDrawable()->setCustomTintStatus(customTintStatus);
			m_currentCustomTint = customTintStatus;
		}
		else if (tintStatus > TINT_STATUS_INVALID && tintStatus < TINT_STATUS_COUNT) {
			getObject()->getDrawable()->setTintStatus(tintStatus);
			m_currentTint = tintStatus;
		}

		// getObject()->getDrawable()->setTintStatus(TINT_STATUS_FRENZY);

		//    if (getObject()->isKindOf(KINDOF_INFANTRY))
		//      getObject()->getDrawable()->setSecondMaterialPassOpacity( 1.0f );
	}

	setWakeFrame( getObject(), UPDATE_SLEEP(duration) );*/
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void TempWeaponBonusHelper::crc( Xfer *xfer )
{

	// object helper crc
	ObjectHelper::crc( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info;
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void TempWeaponBonusHelper::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// object helper base class
	ObjectHelper::xfer( xfer );

	//xfer->xferUser( &m_currentBonus, sizeof(WeaponBonusConditionType) );// an enum
	//xfer->xferUser( &m_currentTint, sizeof(TintStatus) );// an enum
	//xfer->xferAsciiString( &m_currentCustomBonus );
	xfer->xferUnsignedInt( &m_frameToRemove );
	//xfer->xferAsciiString( &m_currentCustomTint );

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

		}

		CustomTintStatusDurationVec::const_iterator it2;
		for( it2 = m_customTintStatus.begin(); it2 != m_customTintStatus.end(); ++it2 )
		{

			customTintStatusInfo.first = (*it2).first;
			xfer->xferAsciiString( &customTintStatusInfo.first );

			customTintStatusInfo.second = (*it2).second;
			xfer->xferUnsignedInt( &customTintStatusInfo.second );

		}

	}
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

		}

		for( UnsignedShort i_2 = 0; i_2 < customTintStatusSize; ++i_2 )
		{

			xfer->xferAsciiString( &customTintStatusInfo.first );

			xfer->xferUnsignedInt( &customTintStatusInfo.second );

			// stuff in vector
			m_customTintStatus.push_back( customTintStatusInfo );

		}

	}

	StatusTypeMap::iterator bonusIt;
	CustomStatusTypeMap::iterator customBonusIt;

	UnsignedShort bonusCount = m_bonusMap.size();
	UnsignedShort customBonusCount = m_customBonusMap.size();
	xfer->xferUnsignedShort( &bonusCount );
	xfer->xferUnsignedShort( &customBonusCount );

	UnsignedShort bonusName; 
	UnsignedInt bonusTime;
	AsciiString customBonusName;
	UnsignedInt customBonusTime;
	if( xfer->getXferMode() == XFER_SAVE )
	{

		for( bonusIt = m_bonusMap.begin(); bonusIt != m_bonusMap.end(); ++bonusIt )
		{

			bonusName = (*bonusIt).first;
			xfer->xferUnsignedShort( &bonusName );

			bonusTime = (*bonusIt).second;
			xfer->xferUnsignedInt( &bonusTime );

		}

		for( customBonusIt = m_customBonusMap.begin(); customBonusIt != m_customBonusMap.end(); ++customBonusIt )
		{

			customBonusName = (*customBonusIt).first;
			xfer->xferAsciiString( &customBonusName );

			customBonusTime = (*customBonusIt).second;
			xfer->xferUnsignedInt( &customBonusTime );

		}

	}
	else
	{

		for( UnsignedShort i = 0; i < bonusCount; ++i )
		{

			xfer->xferUnsignedShort( &bonusName );

			xfer->xferUnsignedInt( &bonusTime );

			m_bonusMap[bonusName] = bonusTime;
			
		}

		for( UnsignedShort i_2 = 0; i_2 < customBonusCount; ++i_2 )
		{

			xfer->xferAsciiString( &customBonusName );

			xfer->xferUnsignedInt( &customBonusTime );

			m_customBonusMap[customBonusName] = customBonusTime;
			
		}

	}

}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void TempWeaponBonusHelper::loadPostProcess( void )
{

	// object helper base class
	ObjectHelper::loadPostProcess();

}


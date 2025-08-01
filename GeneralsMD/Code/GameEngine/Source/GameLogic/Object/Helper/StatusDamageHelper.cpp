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
	m_statusToHeal = OBJECT_STATUS_NONE;
	m_frameToHeal = 0;
	m_currentTint = TINT_STATUS_INVALID;
	m_customStatusToHeal = NULL;

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
	Bool clearStatus;

	if(!m_customStatusToHeal.isEmpty())
	{
		ObjectCustomStatusType myCustomStatus = getObject()->getCustomStatus();
		ObjectCustomStatusType::const_iterator it = myCustomStatus.find(m_customStatusToHeal);
		if(it != myCustomStatus.end() && it->second == 0)
		{
			clearStatus = TRUE;
		}
	}
	if( ( m_statusToHeal != OBJECT_STATUS_NONE && !getObject()->getStatusBits().test( m_statusToHeal ) ) ||
		clearStatus == TRUE ||
		m_frameToHeal <= TheGameLogic->getFrame() )
	{
		//DEBUG_ASSERTCRASH(m_frameToHeal <= TheGameLogic->getFrame(), ("StatusDamageHelper woke up too soon.") );

		clearStatusCondition(); // We are sleep driven, so seeing an update means our timer is ready implicitly
		return UPDATE_SLEEP_FOREVER;
	}
	return UPDATE_SLEEP_NONE;
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void StatusDamageHelper::clearStatusCondition()
{
	if( m_statusToHeal != OBJECT_STATUS_NONE || !m_customStatusToHeal.isEmpty() )
	{
		if(m_statusToHeal != OBJECT_STATUS_NONE)
			getObject()->clearStatus( MAKE_OBJECT_STATUS_MASK(m_statusToHeal) );
		m_statusToHeal = OBJECT_STATUS_NONE;
		m_frameToHeal = 0;
		if(!m_customStatusToHeal.isEmpty())
			getObject()->clearCustomStatus( m_customStatusToHeal );
		m_customStatusToHeal = NULL;

		if (getObject()->getDrawable())
		{
			getObject()->getDrawable()->clearCustomTintStatus();
			if (m_currentTint > TINT_STATUS_INVALID && m_currentTint < TINT_STATUS_COUNT) {
				getObject()->getDrawable()->clearTintStatus(m_currentTint);
				m_currentTint = TINT_STATUS_INVALID;
			}

			// getObject()->getDrawable()->clearTintStatus(TINT_STATUS_FRENZY);
			//      if (getObject()->isKindOf(KINDOF_INFANTRY))
			//        getObject()->getDrawable()->setSecondMaterialPassOpacity( 0.0f );
		}
	}
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void StatusDamageHelper::doStatusDamage( ObjectStatusTypes status, Real duration, const AsciiString& customStatus, const AsciiString& customTintStatus, TintStatus tintStatus )
{
	Int durationAsInt = REAL_TO_INT_FLOOR(duration);
	
	// Clear any different status we may have.  Re-getting the same status will just reset the timer
	if(!customStatus.isEmpty())
	{
		if( m_customStatusToHeal != customStatus )
			clearStatusCondition();

		getObject()->setCustomStatus( customStatus );
		m_customStatusToHeal = customStatus;
	}
	else
	{
		if( m_statusToHeal != status )
			clearStatusCondition();
			
		getObject()->setStatus( MAKE_OBJECT_STATUS_MASK(status) );
		m_statusToHeal = status;
	}

	m_frameToHeal = TheGameLogic->getFrame() + durationAsInt;

	if (getObject()->getDrawable())
	{
		if(!customTintStatus.isEmpty())
		{
			getObject()->getDrawable()->setCustomTintStatus(customTintStatus);
		}
		else if (tintStatus > TINT_STATUS_INVALID && tintStatus < TINT_STATUS_COUNT) {
			getObject()->getDrawable()->setTintStatus(tintStatus);
			m_currentTint = tintStatus;
		}

		// getObject()->getDrawable()->setTintStatus(TINT_STATUS_FRENZY);

		//    if (getObject()->isKindOf(KINDOF_INFANTRY))
		//      getObject()->getDrawable()->setSecondMaterialPassOpacity( 1.0f );
	}

	setWakeFrame(getObject(), UPDATE_SLEEP_NONE);
	//setWakeFrame( getObject(), UPDATE_SLEEP(durationAsInt) );
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void StatusDamageHelper::crc( Xfer *xfer )
{

	// object helper crc
	ObjectHelper::crc( xfer );

}  // end crc

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

	xfer->xferUser( &m_statusToHeal, sizeof(ObjectStatusTypes) );// an enum
	xfer->xferUser( &m_currentTint, sizeof(TintStatus) );// an enum
	xfer->xferAsciiString( &m_customStatusToHeal );
	xfer->xferUnsignedInt( &m_frameToHeal );

}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void StatusDamageHelper::loadPostProcess( void )
{

	// object helper base class
	ObjectHelper::loadPostProcess();

}  // end loadPostProcess


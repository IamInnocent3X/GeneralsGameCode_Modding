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
	m_currentBonus = WEAPONBONUSCONDITION_INVALID;
	m_currentTint = TINT_STATUS_INVALID;
	m_frameToRemove = 0;
	m_currentCustomBonus = NULL;

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
	DEBUG_ASSERTCRASH(m_frameToRemove <= TheGameLogic->getFrame(), ("TempWeaponBonusHelper woke up too soon.") );

	clearTempWeaponBonus(); // We are sleep driven, so seeing an update means our timer is ready implicitly
	return UPDATE_SLEEP_FOREVER;
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void TempWeaponBonusHelper::clearTempWeaponBonus()
{
	if( m_currentBonus != WEAPONBONUSCONDITION_INVALID || !m_currentCustomBonus.isEmpty() )
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
	}
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void TempWeaponBonusHelper::doTempWeaponBonus( WeaponBonusConditionType status, const AsciiString& customStatus, UnsignedInt duration, const AsciiString& customTintStatus, TintStatus tintStatus )
{
	// Clear any different status we may have.  Re-getting the same status will just reset the timer
	if(!customStatus.isEmpty())
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

	setWakeFrame( getObject(), UPDATE_SLEEP(duration) );
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void TempWeaponBonusHelper::crc( Xfer *xfer )
{

	// object helper crc
	ObjectHelper::crc( xfer );

}  // end crc

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

	xfer->xferUser( &m_currentBonus, sizeof(WeaponBonusConditionType) );// an enum
	xfer->xferUser( &m_currentTint, sizeof(TintStatus) );// an enum
	xfer->xferAsciiString( &m_currentCustomBonus );
	xfer->xferUnsignedInt( &m_frameToRemove );
	xfer->xferAsciiString( &m_currentCustomTint );

}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void TempWeaponBonusHelper::loadPostProcess( void )
{

	// object helper base class
	ObjectHelper::loadPostProcess();

}  // end loadPostProcess


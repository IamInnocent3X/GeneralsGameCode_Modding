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

// FILE: ObjectPowerOutrageHelper.cpp ///////////////////////////////////////////////////////////////
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
#include "GameLogic/Module/ObjectPowerOutrageHelper.h"

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
ObjectPowerOutrageHelper::~ObjectPowerOutrageHelper()
{
	m_powerOutrageUntilFrame = 0;
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void ObjectPowerOutrageHelper::setPowerOutrageUntil(UnsignedInt frame)
{
	UnsignedInt now = TheGameLogic->getFrame();
	if(now >= frame)
		return;

	getObject()->doDisablePower(FALSE);
	m_powerOutrageUntilFrame = frame;
	setWakeFrame(getObject(), UPDATE_SLEEP(m_powerOutrageUntilFrame - now));
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
UpdateSleepTime ObjectPowerOutrageHelper::update()
{
	UnsignedInt now = TheGameLogic->getFrame();
	if(m_powerOutrageUntilFrame && now >= m_powerOutrageUntilFrame)
	{
		Bool doClear = TRUE;
		Object *object = getObject();
		Player *player = object->getControllingPlayer();

		// don't clear us from Disabled if we don't have sufficient Power
		if(object->isKindOf(KINDOF_POWERED) && player && !player->getEnergy()->hasSufficientPower())
			doClear = FALSE;

		if(doClear)
			object->clearDisablePower(FALSE);

		m_powerOutrageUntilFrame = 0;
	}

	return UPDATE_SLEEP(m_powerOutrageUntilFrame > now ? (m_powerOutrageUntilFrame - now) : UPDATE_SLEEP_FOREVER);
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void ObjectPowerOutrageHelper::crc( Xfer *xfer )
{

	// object helper crc
	ObjectHelper::crc( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info;
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void ObjectPowerOutrageHelper::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// object helper base class
	ObjectHelper::xfer( xfer );

	xfer->xferUnsignedInt( &m_powerOutrageUntilFrame );

}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void ObjectPowerOutrageHelper::loadPostProcess()
{

	// object helper base class
	ObjectHelper::loadPostProcess();

}


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

// FILE: ObjectDelayedOrderHelper.cpp ///////////////////////////////////////////////////////////////
// Modified from: ObjectSMCHelper.cpp
// Author: Colin Day, Steven Johnson, September 2002
// Desc:   SMC helper
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"
#include "Common/Xfer.h"
#include "Common/MessageStream.h"
#include "Common/Team.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/Object.h"
#include "GameLogic/Module/AIUpdate.h"
#include "GameLogic/Module/ObjectDelayedOrderHelper.h"

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
ObjectDelayedOrderHelper::~ObjectDelayedOrderHelper( void )
{
	m_msgType = GameMessage::MSG_INVALID;
	m_msgArguments.clear();
	m_delayFrame = 0;
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void ObjectDelayedOrderHelper::appendCommand(GameMessage::Type type, const std::vector<GameMessageArgumentStruct>& arguments, UnsignedInt delay)
{
	// If we are already configured with an appended command, don't do anything
	if(m_delayFrame)
		return;
	
	m_msgType = type;
	m_msgArguments = arguments;
	m_delayFrame = TheGameLogic->getFrame() + delay;
	setWakeFrame(getObject(), UPDATE_SLEEP(delay));
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void ObjectDelayedOrderHelper::clearCommand()
{
	// easy
	m_delayFrame = 0;
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
UpdateSleepTime ObjectDelayedOrderHelper::update()
{
	Object *me = getObject();

	// Don't do command if there is no delay frame or I am invalidated
	if(!m_delayFrame || !me || me->isEffectivelyDead() || me->isDestroyed() )
		return UPDATE_SLEEP_FOREVER;

	UnsignedInt now = TheGameLogic->getFrame();
	if(m_delayFrame && now >= m_delayFrame)
	{
		GameMessage *newMsg = TheMessageStream->appendMessage( m_msgType );
		for(std::vector<GameMessageArgumentStruct>::const_iterator it = m_msgArguments.begin(); it != m_msgArguments.end(); ++it)
		{
			switch (it->type) {

			case ARGUMENTDATATYPE_INTEGER:
				newMsg->appendIntegerArgument(it->data.integer);
				break;
			case ARGUMENTDATATYPE_REAL:
				newMsg->appendRealArgument(it->data.real);
				break;
			case ARGUMENTDATATYPE_BOOLEAN:
				newMsg->appendBooleanArgument(it->data.boolean);
				break;
			case ARGUMENTDATATYPE_OBJECTID:
				newMsg->appendObjectIDArgument(it->data.objectID);
				break;
			case ARGUMENTDATATYPE_DRAWABLEID:
				newMsg->appendDrawableIDArgument(it->data.drawableID);
				break;
			case ARGUMENTDATATYPE_TEAMID:
				newMsg->appendTeamIDArgument(it->data.teamID);
				break;
			case ARGUMENTDATATYPE_LOCATION:
				newMsg->appendLocationArgument(it->data.location);
				break;
			case ARGUMENTDATATYPE_PIXEL:
				newMsg->appendPixelArgument(it->data.pixel);
				break;
			case ARGUMENTDATATYPE_PIXELREGION:
				newMsg->appendPixelRegionArgument(it->data.pixelRegion);
				break;
			case ARGUMENTDATATYPE_TIMESTAMP:
				newMsg->appendTimestampArgument(it->data.timestamp);
				break;
			//case ARGUMENTDATATYPE_WIDECHAR:
			//	newMsg->appendWideCharArgument(it->data.wChar);
			//	break;
			}

		}
		newMsg->friend_setDoSingleID(getObject()->getID());
		m_delayFrame = 0;
	}
	return UPDATE_SLEEP( m_delayFrame > now ? m_delayFrame - now : UPDATE_SLEEP_FOREVER );
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void ObjectDelayedOrderHelper::crc( Xfer *xfer )
{

	// object helper crc
	ObjectHelper::crc( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info;
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void ObjectDelayedOrderHelper::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// object helper base class
	ObjectHelper::xfer( xfer );

	// game message type
	xfer->xferUser( &m_msgType, sizeof(GameMessage::Type) );

	// delay frame
	xfer->xferUnsignedInt( &m_delayFrame );

	// -------------------------
	// Xfer Message Arguments
	// -------------------------
	{
		UnsignedShort argumentCount = m_msgArguments.size();
		xfer->xferUnsignedShort(&argumentCount);
		std::vector<GameMessageArgumentStruct>::iterator it;
		if (xfer->getXferMode() == XFER_SAVE)
		{
			// iterate each prototype and xfer if it needs to be in the save file
			for (it = m_msgArguments.begin(); it != m_msgArguments.end(); ++it)
			{
				xfer->xferUser( &((*it).type), sizeof(GameMessageArgumentDataType) );

				switch (it->type) {
				case ARGUMENTDATATYPE_INTEGER:
					xfer->xferInt(&((*it).data).integer);
					break;
				case ARGUMENTDATATYPE_REAL:
					xfer->xferReal(&((*it).data).real);
					break;
				case ARGUMENTDATATYPE_BOOLEAN:
					xfer->xferBool(&((*it).data).boolean);
					break;
				case ARGUMENTDATATYPE_OBJECTID:
					xfer->xferObjectID(&((*it).data).objectID);
					break;
				case ARGUMENTDATATYPE_DRAWABLEID:
					xfer->xferDrawableID(&((*it).data).drawableID);
					break;
				case ARGUMENTDATATYPE_TEAMID:
					xfer->xferUser(&((*it).data).teamID, sizeof( TeamID ) );
					break;
				case ARGUMENTDATATYPE_LOCATION:
					xfer->xferCoord3D(&((*it).data).location);
					break;
				case ARGUMENTDATATYPE_PIXEL:
					xfer->xferICoord2D(&((*it).data).pixel);
					break;
				case ARGUMENTDATATYPE_PIXELREGION:
					xfer->xferIRegion2D(&((*it).data).pixelRegion);
					break;
				case ARGUMENTDATATYPE_TIMESTAMP:
					xfer->xferUnsignedInt(&((*it).data).timestamp);
					break;
				//case ARGUMENTDATATYPE_WIDECHAR:
				//	xfer->xferWideChar(&((*it).data).wChar);
				//	break;
				}
			}

		}
		else
		{
			for (UnsignedShort i = 0; i < argumentCount; ++i)
			{
				GameMessageArgumentStruct argument;

				xfer->xferUser(&(argument.type), sizeof(GameMessageArgumentDataType) );

				switch (argument.type) {
				case ARGUMENTDATATYPE_INTEGER:
					xfer->xferInt(&(argument.data).integer);
					break;
				case ARGUMENTDATATYPE_REAL:
					xfer->xferReal(&(argument.data).real);
					break;
				case ARGUMENTDATATYPE_BOOLEAN:
					xfer->xferBool(&(argument.data).boolean);
					break;
				case ARGUMENTDATATYPE_OBJECTID:
					xfer->xferObjectID(&(argument.data).objectID);
					break;
				case ARGUMENTDATATYPE_DRAWABLEID:
					xfer->xferDrawableID(&(argument.data).drawableID);
					break;
				case ARGUMENTDATATYPE_TEAMID:
					xfer->xferUser(&(argument.data).teamID, sizeof( TeamID ) );
					break;
				case ARGUMENTDATATYPE_LOCATION:
					xfer->xferCoord3D(&(argument.data).location);
					break;
				case ARGUMENTDATATYPE_PIXEL:
					xfer->xferICoord2D(&(argument.data).pixel);
					break;
				case ARGUMENTDATATYPE_PIXELREGION:
					xfer->xferIRegion2D(&(argument.data).pixelRegion);
					break;
				case ARGUMENTDATATYPE_TIMESTAMP:
					xfer->xferUnsignedInt(&(argument.data).timestamp);
					break;
				//case ARGUMENTDATATYPE_WIDECHAR:
				//	xfer->xferWideChar(&(argument.data).wChar);
				//	break;
				}

				m_msgArguments.push_back(argument);

			}

		}
	}

}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void ObjectDelayedOrderHelper::loadPostProcess( void )
{

	// object helper base class
	ObjectHelper::loadPostProcess();

}

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

// FILE: ObjectDelayedOrderHelper.h /////////////////////////////////////////////////////////////////
// Modified from: ObjectSMCHelper.h
// Author: Steven Johnson, Colin Day - September 202
// Desc:   Object helpder - SMC
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef __OBJECT_DELAYEDORDER_HELPER_H_
#define __OBJECT_DELAYEDORDER_HELPER_H_

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "GameLogic/Module/ObjectHelper.h"
#include "Common/MessageStream.h"

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
class ObjectDelayedOrderHelperModuleData : public ModuleData
{

};

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
class ObjectDelayedOrderHelper : public ObjectHelper
{

	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( ObjectDelayedOrderHelper, ObjectDelayedOrderHelperModuleData )
	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE(ObjectDelayedOrderHelper, "ObjectDelayedOrderHelper" )

public:

	ObjectDelayedOrderHelper( Thing *thing, const ModuleData *modData ) : ObjectHelper( thing, modData ) { }
	// virtual destructor prototype provided by memory pool object

	virtual UpdateSleepTime update();
	virtual DisabledMaskType getDisabledTypesToProcess() const { return DISABLEDMASK_ALL; }

	void appendCommand(GameMessage::Type type, const std::vector<GameMessageArgumentStruct>& arguments, UnsignedInt delay);
	void clearCommand();

private:
	GameMessage::Type 								m_msgType;
	std::vector<GameMessageArgumentStruct>			m_msgArguments;
	UnsignedInt										m_delayFrame;
};


#endif  // end __OBJECT_LEVITATION_HELPER_H_

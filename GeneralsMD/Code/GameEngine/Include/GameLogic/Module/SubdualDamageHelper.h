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

// FILE: SubdualDamageHelper.h ////////////////////////////////////////////////////////////////////////
// Author: Graham Smallwood, June 2003
// Desc:   Object helper - Clears subdual status and heals subdual damage since Body modules can't have Updates
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "Common/DisabledTypes.h"
#include "GameLogic/Module/ObjectHelper.h"
#include "GameLogic/Module/BodyModule.h"

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
class SubdualDamageHelperModuleData : public ModuleData
{

};

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
class SubdualDamageHelper : public ObjectHelper
{

	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( SubdualDamageHelper, SubdualDamageHelperModuleData )
	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE(SubdualDamageHelper, "SubdualDamageHelper" )

public:

	SubdualDamageHelper( Thing *thing, const ModuleData *modData );
	// virtual destructor prototype provided by memory pool object

	virtual DisabledMaskType getDisabledTypesToProcess() const { return DISABLEDMASK_ALL; }
	virtual void refreshUpdate() { setWakeFrame(getObject(), UPDATE_SLEEP_NONE); }
	virtual UpdateSleepTime update();

	void notifySubdualDamage( Real amount );
	void notifySubdualDamageCustom( SubdualCustomData subdualData, const AsciiString& customStatus );
	void replaceSubdualDamageCustom( CustomSubdualCurrentHealMap data ) { m_healingStepCountdownCustomMap = data; }
	CustomSubdualCurrentHealMap getSubdualHelperData() const { return m_healingStepCountdownCustomMap; }

protected:
	UnsignedInt m_healingStepCountdown;
	UnsignedInt m_healingStepFrame;
	UnsignedInt m_nextHealingStepFrame;
	CustomSubdualCurrentHealMap m_healingStepCountdownCustomMap;
};

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

#pragma once

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "GameLogic/Module/ObjectHelper.h"
#include "GameClient/TintStatus.h"

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
class StatusDamageHelperModuleData : public ModuleData
{

};

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
class StatusDamageHelper : public ObjectHelper
{

	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( StatusDamageHelper, StatusDamageHelperModuleData )
	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE(StatusDamageHelper, "StatusDamageHelper" )

public:

	StatusDamageHelper( Thing *thing, const ModuleData *modData );
	// virtual destructor prototype provided by memory pool object

	virtual DisabledMaskType getDisabledTypesToProcess() const { return DISABLEDMASK_ALL; }
	virtual void refreshUpdate() { setWakeFrame(getObject(), UPDATE_SLEEP_NONE); }
	virtual UpdateSleepTime update();

	void doStatusDamage( ObjectStatusTypes status, Real duration , const AsciiString& customStatus, const AsciiString& customTintStatus, TintStatus tintStatus = TINT_STATUS_INVALID );
	HelperTransferData getHelperData() const
	{
		HelperTransferData transferData;
		transferData.frameEnd = m_frameToHeal;
		transferData.earliestDurationAsInt = m_earliestDurationAsInt;
		transferData.statusMap = m_statusToHeal;
		transferData.customStatusMap = m_customStatusToHeal;
		transferData.currentTint = m_currentTint;
		transferData.customTintStatus = m_customTintStatus;

		return transferData;
	}
	void transferData(HelperTransferData transferData)
	{
		m_frameToHeal = transferData.frameEnd;
		m_earliestDurationAsInt = transferData.earliestDurationAsInt;
		m_statusToHeal = transferData.statusMap;
		m_customStatusToHeal = transferData.customStatusMap;
		m_currentTint = transferData.currentTint;
		m_customTintStatus = transferData.customTintStatus;
	}

protected:
	StatusTypeMap m_statusToHeal;
	CustomStatusTypeMap m_customStatusToHeal;
	CustomTintStatusDurationVec m_customTintStatus;
	TintStatusDurationVec m_currentTint;
	UnsignedInt m_frameToHeal;
	UnsignedInt m_earliestDurationAsInt;
	void clearStatusCondition();
};

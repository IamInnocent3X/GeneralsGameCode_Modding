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

#pragma once

#ifndef __TempWeaponBonusHelper_H_
#define __TempWeaponBonusHelper_H_

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "GameLogic/Module/ObjectHelper.h"
#include "GameClient/TintStatus.h"

enum WeaponBonusConditionType CPP_11(: Int);
// enum TintStatus CPP_11(: Int);

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
class TempWeaponBonusHelperModuleData : public ModuleData
{

};

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
class TempWeaponBonusHelper : public ObjectHelper
{

	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( TempWeaponBonusHelper, TempWeaponBonusHelperModuleData )
	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE(TempWeaponBonusHelper, "TempWeaponBonusHelper" )

public:

	TempWeaponBonusHelper( Thing *thing, const ModuleData *modData );
	// virtual destructor prototype provided by memory pool object

	virtual DisabledMaskType getDisabledTypesToProcess() const { return DISABLEDMASK_ALL; }
	virtual void refreshUpdate() { setWakeFrame(getObject(), UPDATE_SLEEP_NONE); }
	virtual UpdateSleepTime update();

	void doTempWeaponBonus( WeaponBonusConditionType status, const AsciiString& customStatus, UnsignedInt duration, const AsciiString& customTintStatus, TintStatus tintStatus = TINT_STATUS_INVALID );

	HelperTransferData getHelperData() const
	{
		HelperTransferData transferData;
		transferData.frameEnd = m_frameToRemove;
		transferData.earliestDurationAsInt = m_earliestDurationAsInt;
		transferData.statusMap m_bonusMap;
		transferData.customStatusMap = m_customBonusMap;
		transferData.currentTint = m_currentTint;
		transferData.customTintStatus = m_customTintStatus;

		return transferData;
	}
	void transferData(HelperTransferData transferData)
	{
		m_frameToRemove = transferData.frameEnd;
		m_earliestDurationAsInt = transferData.earliestDurationAsInt;
		m_bonusMap = transferData.statusMap;
		m_customBonusMap = transferData.customStatusMap;
		m_currentTint = transferData.currentTint;
		m_customTintStatus = transferData.customTintStatus;
	}

protected:
	//WeaponBonusConditionType m_currentBonus;
	//AsciiString m_currentCustomBonus;
	//AsciiString m_currentCustomTint;
	//TintStatus m_currentTint;
	StatusTypeMap m_bonusMap;
	CustomStatusTypeMap m_customBonusMap;
	CustomTintStatusDurationVec m_customTintStatus;
	TintStatusDurationVec m_currentTint;
	UnsignedInt m_frameToRemove;
	UnsignedInt m_earliestDurationAsInt;
	void clearTempWeaponBonus();
};


#endif  // end __TempWeaponBonusHelper_H_

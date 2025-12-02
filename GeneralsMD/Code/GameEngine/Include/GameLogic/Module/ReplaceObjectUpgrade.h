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

// FILE: ReplaceObjectUpgrade.h /////////////////////////////////////////////////////////////////////////////
// Author: Graham Smallwood, July 2003
// Desc:	 UpgradeModule that creates a new Object in our exact location and then deletes our object
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef _REPLACE_OBJECT_UPGRADE_H
#define _REPLACE_OBJECT_UPGRADE_H

#include "GameLogic/Module/UpgradeModule.h"
#include "GameLogic/Module/BodyModule.h"
#include "GameLogic/Module/CreateObjectDie.h"

//-----------------------------------------------------------------------------
class ReplaceObjectUpgradeModuleData : public UpgradeModuleData
{
public:
	AsciiString m_replaceObjectName;

	Bool											m_transferHealth;
	Bool											m_transferAIStates;
	Bool											m_transferExperience;
	Bool											m_transferAttack;
	Bool											m_transferStatus;
	Bool											m_transferWeaponBonus;
	Bool											m_transferDisabledType;
	Bool											m_transferBombs;
	Bool											m_transferHijackers;
	Bool											m_transferEquippers;
	Bool											m_transferParasites;
	Bool											m_transferPassengers;
	Bool											m_transferToAssaultTransport;
	Bool											m_transferShieldedTargets;
	Bool											m_transferShieldingTargets;
	Bool											m_transferSelection;
	Bool											m_transferSelectionDontClearGroup;
	Bool											m_transferObjectName;
	MaxHealthChangeType 							m_transferHealthChangeType;
	
	Real											m_extraBounciness;
	Real											m_extraFriction;
	Coord3D											m_offset;
	DispositionType									m_disposition;
	Real											m_dispositionIntensity;
	Real											m_spinRate;
	Real											m_yawRate;
	Real											m_rollRate;
	Real											m_pitchRate;
	Real											m_minMag, m_maxMag;
	Real											m_minPitch, m_maxPitch;
	AudioEventRTS							m_bounceSound;
	Bool											m_orientInForceDirection;
	Bool											m_diesOnBadLand;

	ReplaceObjectUpgradeModuleData()
	{
		m_transferHealth = FALSE;
		m_transferAIStates = FALSE;
		m_transferExperience = FALSE;
		m_transferAttack = FALSE;
		m_transferStatus = FALSE;
		m_transferWeaponBonus = FALSE;
		m_transferDisabledType = FALSE;
		m_transferBombs = FALSE;
		m_transferHijackers = TRUE;
		m_transferEquippers = TRUE;
		m_transferParasites = TRUE;
		m_transferPassengers = FALSE;
		m_transferToAssaultTransport = FALSE;
		m_transferShieldedTargets = FALSE;
		m_transferShieldingTargets = FALSE;
		m_transferSelection = FALSE;
		m_transferSelectionDontClearGroup = TRUE;
		m_transferObjectName = FALSE;
		m_transferHealthChangeType = SAME_CURRENTHEALTH;
		
		m_extraBounciness = 0.0f;
		m_extraFriction = 0.0f;
		m_disposition = ON_GROUND_ALIGNED;
		m_dispositionIntensity = 0.0f;
		m_spinRate = -1.0f;
		m_yawRate = -1.0f;
		m_rollRate = -1.0f;
		m_pitchRate = -1.0f;
		m_minMag = 0.0f;
		m_maxMag = 0.0f;
		m_minPitch = 0.0f;
		m_maxPitch = 0.0f;
		m_orientInForceDirection = FALSE;
		m_diesOnBadLand = FALSE;
	}

	static void buildFieldParse(MultiIniFieldParse& p);
};

//-----------------------------------------------------------------------------
class ReplaceObjectUpgrade : public UpgradeModule
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( ReplaceObjectUpgrade, "ReplaceObjectUpgrade" )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( ReplaceObjectUpgrade, ReplaceObjectUpgradeModuleData );

public:

	ReplaceObjectUpgrade( Thing *thing, const ModuleData* moduleData );
	// virtual destructor prototype defined by MemoryPoolObject

protected:
	virtual void upgradeImplementation( ); ///< Here's the actual work of Upgrading
	virtual Bool isSubObjectsUpgrade() { return false; }
	virtual Bool hasUpgradeRefresh() { return false; }

	void doDisposition(Object *sourceObj, Object* obj);

};
#endif // _COMMAND_SET_UPGRADE_H



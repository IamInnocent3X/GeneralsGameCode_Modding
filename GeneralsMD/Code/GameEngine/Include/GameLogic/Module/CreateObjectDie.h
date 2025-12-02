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

// FILE: CreateObjectDie.h /////////////////////////////////////////////////////////////////////////////
// Author: Michael S. Booth, January 2002
// Desc:   Create object at current object's death
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef _CREATE_OBJECT_DIE_H_
#define _CREATE_OBJECT_DIE_H_

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "Common/INI.h"
#include "GameLogic/Module/DieModule.h"

// FORWARD REFERENCES /////////////////////////////////////////////////////////////////////////////
class Thing;
class ObjectCreationList;

//-------------------------------------------------------------------------------------------------
enum DispositionType CPP_11(: Int)
{
	LIKE_EXISTING						= 0x00000001,
	ON_GROUND_ALIGNED				= 0x00000002,
	SEND_IT_FLYING					= 0x00000004,
	SEND_IT_UP							= 0x00000008,
	SEND_IT_OUT							= 0x00000010,
	RANDOM_FORCE						= 0x00000020,
	FLOATING								= 0x00000040,
	INHERIT_VELOCITY				= 0x00000080,
	WHIRLING								= 0x00000100,
	ALIGN_Z_UP							= 0x00000200
};

#ifdef DEFINE_DISPOSITION_NAMES
static const char* DispositionNames[] =
{
	"LIKE_EXISTING",
	"ON_GROUND_ALIGNED",
	"SEND_IT_FLYING",
	"SEND_IT_UP",
	"SEND_IT_OUT",
	"RANDOM_FORCE",
	"FLOATING",
	"INHERIT_VELOCITY",
	"WHIRLING",
	"ALIGN_Z_UP"
};
#endif

//-------------------------------------------------------------------------------------------------
class CreateObjectDieModuleData : public DieModuleData
{

public:

	const ObjectCreationList* m_ocl;			///< object creaton list to make
	Bool m_transferPreviousHealth; ///< Transfers previous health before death to the new object created.

	MaxHealthChangeType 							m_previousHealthChangeType;

	Bool											m_transferExperience;
	Bool											m_transferAttackers;
	Bool											m_transferAIStates;
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

	CreateObjectDieModuleData();

	static void buildFieldParse(MultiIniFieldParse& p);

};

//-------------------------------------------------------------------------------------------------
/** When this object dies, create another object in its place */
//-------------------------------------------------------------------------------------------------
class CreateObjectDie : public DieModule
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( CreateObjectDie, "CreateObjectDie"  )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( CreateObjectDie, CreateObjectDieModuleData );

public:

	CreateObjectDie( Thing *thing, const ModuleData* moduleData );
	// virtual destructor prototype provided by memory pool declaration

	virtual void onDie( const DamageInfo *damageInfo );

protected:
	void doDisposition(Object *sourceObj, Object* obj);
};

#endif // _CREATE_OBJECT_DIE_H_


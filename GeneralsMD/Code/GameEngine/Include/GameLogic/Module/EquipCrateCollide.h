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

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// FILE: EquipCrateCollide.h
// Author: IamInnocent, September 2025
// Desc:   A crate module that makes the makes the user to latch onto a target
//				 granting either positive and negative attributes
//				 such as granting bonuses to allies, or dealing DoT on enemies
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef EQUIP_CRATE_COLLIDE_H_
#define EQUIP_CRATE_COLLIDE_H_

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "Common/Module.h"
#include "GameLogic/Object.h"
#include "GameLogic/Module/CrateCollide.h"

// FORWARD REFERENCES /////////////////////////////////////////////////////////////////////////////
class Thing;

//-------------------------------------------------------------------------------------------------
class EquipCrateCollideModuleData : public CrateCollideModuleData
{
public:
	UnsignedInt m_rangeOfEffect;
	Bool m_isContain;
	Bool m_isParasite;
	Bool m_isUnique;
	Bool m_equipCanPassiveAcquire;
	Bool m_destroyOnClear;

	EquipCrateCollideModuleData()
	{
		m_rangeOfEffect = 0;
		m_isContain = FALSE;
		m_isParasite = FALSE;
		m_isUnique = FALSE;
		m_equipCanPassiveAcquire = FALSE;
		m_destroyOnClear = FALSE;
	}

	static void buildFieldParse(MultiIniFieldParse& p)
	{
    CrateCollideModuleData::buildFieldParse(p);
		static const FieldParse dataFieldParse[] =
		{
			{ "IsContain",				INI::parseBool,		NULL, offsetof( EquipCrateCollideModuleData, m_isContain ) },
			{ "IsParasite",				INI::parseBool,		NULL, offsetof( EquipCrateCollideModuleData, m_isParasite ) },
			{ "IsUnique",				INI::parseBool,		NULL, offsetof( EquipCrateCollideModuleData, m_isUnique ) },
			{ "CanPassiveAcquire",		INI::parseBool,		NULL, offsetof( EquipCrateCollideModuleData, m_equipCanPassiveAcquire ) },
			{ "DestroyOnClear",			INI::parseBool,		NULL, offsetof( EquipCrateCollideModuleData, m_destroyOnClear ) },
			{ 0, 0, 0, 0 }
		};
    p.add(dataFieldParse);

	}

};

//-------------------------------------------------------------------------------------------------
class EquipCrateCollide : public CrateCollide
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( EquipCrateCollide, "EquipCrateCollide" )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( EquipCrateCollide, EquipCrateCollideModuleData );

public:

	EquipCrateCollide( Thing *thing, const ModuleData* moduleData );
	// virtual destructor prototype provided by memory pool declaration

	ObjectID getEquipObjectID() const { return m_equipToID; }
	void setEquipStatus(Object *other);
	void clearEquipStatus();

protected:

	/// This allows specific vetoes to certain types of crates and their data
	virtual Bool isValidToExecute( const Object *other ) const;

	/// This is the game logic execution function that all real CrateCollides will implement
	virtual Bool executeCrateBehavior( Object *other );

	virtual Bool revertCollideBehavior(Object *other);

	virtual Bool isEquipCrateCollide() const { return TRUE; }
	virtual Bool isParasiteEquipCrateCollide() const  { return getEquipCrateCollideModuleData()->m_isParasite; }

private:
	ObjectID m_equipToID;
};

#endif

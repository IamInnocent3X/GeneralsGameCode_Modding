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

// FILE: CostModifierUpgrade.h /////////////////////////////////////////////////
//-----------------------------------------------------------------------------
//
//                       Electronic Arts Pacific.
//
//                       Confidential Information
//                Copyright (C) 2002 - All Rights Reserved
//
//-----------------------------------------------------------------------------
//
//	created:	Aug 2002
//
//	Filename: 	CostModifierUpgrade.h
//
//	author:		Chris Huybregts
//
//	purpose:
//
//-----------------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef __COST_MODIFIER_UPGRADE_H_
#define __COST_MODIFIER_UPGRADE_H_

//-----------------------------------------------------------------------------
// SYSTEM INCLUDES ////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// USER INCLUDES //////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------
#include "GameLogic/Module/UpgradeModule.h"
#include "Common/KindOf.h"
//-----------------------------------------------------------------------------
// FORWARD REFERENCES /////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------
class Thing;
class Player;

//-----------------------------------------------------------------------------
// TYPE DEFINES ///////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// INLINING ///////////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// EXTERNALS //////////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------

// FORWARD REFERENCES /////////////////////////////////////////////////////////////////////////////

enum BonusStackingType CPP_11(: Int)
{
	NO_STACKING = 0,  // Default behaviour: Values of different percentage stack
	OTHER_TYPE = 1,  // Values from the different source object types stack.
	SAME_TYPE = 2   // Values from the same type of source object stack.
};
static const char* TheBonusStackingTypeNames[] =
{
	"DIFFERENT_VALUE",
	"OTHER_TYPE",
	"SAME_TYPE",
	NULL
};

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
class CostModifierUpgradeModuleData : public UpgradeModuleData
{

public:

	CostModifierUpgradeModuleData( void );

	static void buildFieldParse(MultiIniFieldParse& p);

	Real m_percentage;
	KindOfMaskType m_kindOf;
	Bool m_isOneShot;
	BonusStackingType m_stackingType;
};

//-------------------------------------------------------------------------------------------------
/** The OCL upgrade module */
//-------------------------------------------------------------------------------------------------
class CostModifierUpgrade : public UpgradeModule
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( CostModifierUpgrade, "CostModifierUpgrade" )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( CostModifierUpgrade, CostModifierUpgradeModuleData );

public:

	CostModifierUpgrade( Thing *thing, const ModuleData* moduleData );
	// virtual destructor prototype defined by MemoryPoolObject

	virtual void onDelete( void );																///< we have some work to do when this module goes away
	virtual void onCapture( Player *oldOwner, Player *newOwner );

	void doCostModifier(Bool isAdd);

protected:

	virtual void upgradeImplementation( void ); ///< Here's the actual work of Upgrading
	virtual Bool isSubObjectsUpgrade() { return false; }
	virtual Bool hasUpgradeRefresh() { return true; }

private:
	Bool m_hasExecuted;
};

#endif // __COST_MODIFIER_UPGRADE_H_

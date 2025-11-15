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

// FILE: RadarUpgrade.h ///////////////////////////////////////////////////////////////////////////
// Author: Colin Day, March 2002
// Desc:	 Adding a radar upgrade to an object
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef __RADARUPGRADE_H_
#define __RADARUPGRADE_H_

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "GameLogic/Module/UpgradeModule.h"

// FORWARD REFERENCES /////////////////////////////////////////////////////////////////////////////
class Thing;
class Player;

//-----------------------------------------------------------------------------
class RadarUpgradeModuleData : public UpgradeModuleData
{
public:
	Bool m_isDisableProof;// Super radar, ignores radarDisabled checks

	RadarUpgradeModuleData()
	{
		m_isDisableProof = FALSE;
	}

	static void buildFieldParse(MultiIniFieldParse& p);
};

// ------------------------------------------------------------------------------------------------
class RadarUpgradeInterface
{
public:
	virtual Bool getIsDisableProof(void) const = 0;
	virtual Bool isUpgraded(void) const = 0;
};

//-------------------------------------------------------------------------------------------------
/** The Radar upgrade module */
//-------------------------------------------------------------------------------------------------
class RadarUpgrade : public UpgradeModule, public RadarUpgradeInterface
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( RadarUpgrade, "RadarUpgrade" )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( RadarUpgrade, RadarUpgradeModuleData );

public:

	RadarUpgrade( Thing *thing, const ModuleData* moduleData );
	// virtual destructor prototype defined by MemoryPoolObject

	virtual RadarUpgradeInterface* getRadarUpgradeInterface() { return this; }
	
	virtual void onDelete( void );																///< we have some work to do when this module goes away
	virtual void onCapture( Player *oldOwner, Player *newOwner );	///< object containing upgrade has changed teams
	virtual Bool getIsDisableProof(void) const { return getRadarUpgradeModuleData()->m_isDisableProof; }
	virtual Bool isUpgraded(void) const { return isAlreadyUpgraded(); }

	void doRadarUpgrade(Bool isAdd);

protected:

	virtual void upgradeImplementation( void ); ///< Here's the actual work of Upgrading
	virtual Bool isSubObjectsUpgrade() { return false; }
	virtual Bool hasUpgradeRefresh() { return true; }

private:
	Bool m_hasExecuted;
};

#endif // __RADARUPGRADE_H_


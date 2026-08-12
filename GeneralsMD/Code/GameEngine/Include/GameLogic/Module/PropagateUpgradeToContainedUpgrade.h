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

// FILE: PropagateUpgradeToContainedUpgrade.h ///////////////////////////////////
//-----------------------------------------------------------------------------
//
//	purpose:	When this upgrade triggers on a transport/container, grant the
//						configured upgrade(s) to each currently-contained passenger, so
//						riders with a matching UpgradeModule react as if upgraded.
//
//-----------------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////

#pragma once

//-----------------------------------------------------------------------------
// USER INCLUDES //////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------
#include "Common/STLTypedefs.h"
#include "GameLogic/Module/UpgradeModule.h"

//-----------------------------------------------------------------------------
// FORWARD REFERENCES /////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------
class Thing;

//-----------------------------------------------------------------------------
class PropagateUpgradeToContainedUpgradeModuleData : public UpgradeModuleData
{
public:
	std::vector<AsciiString> m_upgradeNamesToPropagate;	///< names granted to each rider when this fires

	PropagateUpgradeToContainedUpgradeModuleData()
	{
	}

	static void buildFieldParse(MultiIniFieldParse& p);
};

//-----------------------------------------------------------------------------
class PropagateUpgradeToContainedUpgrade : public UpgradeModule
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( PropagateUpgradeToContainedUpgrade, "PropagateUpgradeToContainedUpgrade" )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( PropagateUpgradeToContainedUpgrade, PropagateUpgradeToContainedUpgradeModuleData );

public:

	PropagateUpgradeToContainedUpgrade( Thing *thing, const ModuleData* moduleData );
	// virtual destructor prototype defined by MemoryPoolObject

protected:
	virtual void upgradeImplementation( ); ///< Here's the actual work of Upgrading
	virtual Bool isSubObjectsUpgrade() { return false; }
	virtual Bool hasUpgradeRefresh() { return true; }

private:
	Bool m_hasExecuted;

};

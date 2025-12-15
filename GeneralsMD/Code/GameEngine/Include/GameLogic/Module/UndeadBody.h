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

// FILE: UndeadBody.h ////////////////////////////////////////////////////////////////////////
// Author: Graham Smallwood, June 2003
// Desc:	 First death is intercepted and sets flags and setMaxHealth.  Second death is handled normally.
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "GameLogic/Module/ActiveBody.h"

// FORWARD REFERENCES /////////////////////////////////////////////////////////////////////////////
class Object;

//-------------------------------------------------------------------------------------------------
class UndeadBodyModuleData : public ActiveBodyModuleData
{
public:
	Real m_secondLifeMaxHealth;
	Real m_secondLifeSubdualCap;
	UnsignedInt m_secondLifeSubdualHealRate;
	Real m_secondLifeSubdualHealAmount;
	Int m_secondLifeArmorSetSwitch;
	Int m_multipleLives;
	Real m_multipleLivesMaxHealthRatio;
	Bool m_multipleLivesMaxHealthRatioScale;
	std::vector<Int> m_multipleLivesOverrideTrigger;
	std::vector<Real> m_multipleLivesMaxHealthOverride;
	std::vector<Real> m_multipleLivesSubdualCapOverride;
	std::vector<UnsignedInt> m_multipleLivesSubdualHealRate;
	std::vector<Real> m_multipleLivesSubdualHealAmount;
	std::vector<AsciiString> m_secondLifeUpgradeNames;
	std::vector<AsciiString> m_secondLifeUpgradeRemoveNames;
	std::vector<AsciiString> m_multipleLivesUpgradeList;
	std::vector<AsciiString> m_multipleLivesUpgradeRemoveList;

	UndeadBodyModuleData();

	static void buildFieldParse(MultiIniFieldParse& p);
};

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
class UndeadBody : public ActiveBody
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( UndeadBody, "UndeadBody" )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( UndeadBody, UndeadBodyModuleData )

public:

	UndeadBody( Thing *thing, const ModuleData* moduleData );
	// virtual destructor prototype provided by memory pool declaration

	virtual void attemptDamage( DamageInfo *damageInfo );		///< try to damage this object

protected:

	Bool m_isSecondLife;	/** This is false until I detect death the first time, then I
														change my Max, Initial, and Current health and stop intercepting anything.
												*/
	Int m_currentMultipleLives;
	void startSecondLife(DamageInfo *damageInfo);

};

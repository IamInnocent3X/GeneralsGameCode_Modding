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

// FILE: GenerateMinefieldBehavior.h /////////////////////////////////////////////////////////////////////////
// Author: Colin Day, December 2001
// Desc:   Update that will count down a lifetime and destroy object when it reaches zero
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "GameLogic/Module/BehaviorModule.h"
#include "GameLogic/Module/DieModule.h"
#include "GameLogic/Module/UpgradeModule.h"
#include "GameLogic/Module/UpdateModule.h"
#include "GameLogic/Module/CreateModule.h"

//-------------------------------------------------------------------------------------------------
class GenerateMinefieldBehaviorModuleData : public BehaviorModuleData
{
public:
	UpgradeMuxData				m_upgradeMuxData;
	AsciiString						m_mineName;
	AsciiString						m_mineNameUpgraded;
	AsciiString						m_mineUpgradeTrigger;
	const FXList*					m_genFX;
	Real									m_distanceAroundObject;
	Real									m_minesPerSquareFoot;
	Real									m_randomJitter;
	Real									m_skipIfThisMuchUnderStructure;
	Bool									m_onDeath;
	Bool									m_borderOnly;
	Bool									m_alwaysCircular;
	Bool									m_upgradable;
	Bool									m_smartBorder;
	Bool									m_smartBorderSkipInterior;

	GenerateMinefieldBehaviorModuleData();

	static void buildFieldParse(MultiIniFieldParse& p);

private:

};

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
class GenerateMinefieldBehavior : public UpdateModule,
																	public CreateModuleInterface,
																	public DieModuleInterface,
																	public UpgradeMux
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( GenerateMinefieldBehavior, "GenerateMinefieldBehavior" )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( GenerateMinefieldBehavior, GenerateMinefieldBehaviorModuleData )

public:

	GenerateMinefieldBehavior( Thing *thing, const ModuleData* moduleData );
	// virtual destructor prototype provided by memory pool declaration

	// module methods
	static Int getInterfaceMask() { return UpdateModule::getInterfaceMask() | (MODULEINTERFACE_DIE) | (MODULEINTERFACE_UPGRADE); }

	// BehaviorModule
	virtual DieModuleInterface* getDie() { return this; }
	virtual UpgradeModuleInterface* getUpgrade() { return this; }
	virtual UpdateSleepTime update();

	// DamageModuleInterface
	virtual void onDie( const DamageInfo *damageInfo );

	// CreateModuleInterface
	virtual CreateModuleInterface* getCreate() { return this; }
	virtual void onBuildComplete();
	virtual void onCreate( void ) { onBuildComplete(); }
	virtual Bool shouldDoOnBuildComplete() const { return FALSE; }

	// UpdateModule
	virtual void refreshUpdate() { setWakeFrame(getObject(), UPDATE_SLEEP_NONE); }

	void setMinefieldTarget(const Coord3D* pos);

	Bool canUpgrade() const;

protected:

	virtual void upgradeImplementation();
	virtual Bool isSubObjectsUpgrade() { return false; }
	virtual Bool hasUpgradeRefresh() { return false; }

	virtual void getUpgradeActivationMasks(UpgradeMaskType& activation, UpgradeMaskType& conflicting) const
	{
		getGenerateMinefieldBehaviorModuleData()->m_upgradeMuxData.getUpgradeActivationMasks(activation, conflicting);
	}
	virtual void performUpgradeFX()
	{
		getGenerateMinefieldBehaviorModuleData()->m_upgradeMuxData.performUpgradeFX(getObject());
	}
	virtual void processUpgradeGrant()
	{
		// I can't take it any more.  Let the record show that I think the UpgradeMux multiple inheritence is CRAP.
		getGenerateMinefieldBehaviorModuleData()->m_upgradeMuxData.muxDataProcessUpgradeGrant(getObject());
	}
	virtual void processUpgradeRemoval()
	{
		// I can't take it any more.  Let the record show that I think the UpgradeMux multiple inheritence is CRAP.
		getGenerateMinefieldBehaviorModuleData()->m_upgradeMuxData.muxDataProcessUpgradeRemoval(getObject());
	}

	virtual Bool requiresAllActivationUpgrades() const
	{
		return getGenerateMinefieldBehaviorModuleData()->m_upgradeMuxData.m_requiresAllTriggers;
	}

	virtual Bool checkStartsActive() const
	{
		return getGenerateMinefieldBehaviorModuleData()->m_upgradeMuxData.muxDataCheckStartsActive(getObject());
	}

	Bool isUpgradeActive() const { return isAlreadyUpgraded(); }

private:
	Coord3D							m_target;
	Bool								m_hasTarget;
	Bool								m_generated;
	Bool								m_upgraded;
	Bool								m_giveSelfUpgrade;
	std::list<ObjectID> m_mineList;

	const Coord3D* getMinefieldTarget() const;
	void placeMines();
	void placeMinesInFootprint(const GeometryInfo& geom, const ThingTemplate* mineTemplate);
	void placeMinesAroundCircle(const Coord3D& pos, Real radius, const ThingTemplate* mineTemplate);
	void placeMinesAlongLine(const Coord3D& posStart, const Coord3D& posEnd, const ThingTemplate* mineTemplate, Bool skipOneAtStart);
	void placeMinesAroundRect(const Coord3D& pos, Real majorRadius, Real minorRadius, const ThingTemplate* mineTemplate);
	Object* placeMineAt(const Coord3D& pt, const ThingTemplate* mineTemplate, Team* team, const Object* producer);
};

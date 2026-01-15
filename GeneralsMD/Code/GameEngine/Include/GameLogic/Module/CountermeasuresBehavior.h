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
//  (c) 2003 Electronic Arts Inc.																				      //
//																																						//
////////////////////////////////////////////////////////////////////////////////

// FILE: CountermeasuresBehavior.h /////////////////////////////////////////////////////////////////////////
// Author: Kris Morness, April 2003
// Desc: Handles countermeasure firing when under missile threat, and responsible
//       for diverting missiles to the flares.
//------------------------------------------

#pragma once

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "GameClient/ParticleSys.h"
#include "GameLogic/Module/BehaviorModule.h"
#include "GameLogic/Module/UpgradeModule.h"
#include "GameLogic/Module/UpdateModule.h"
#include "GameLogic/Module/DamageModule.h"
#include "Common/BitFlagsIO.h"

class ParticleSystem;
class ParticleSystemTemplate;

//-------------------------------------------------------------------------------------------------
class CountermeasuresBehaviorModuleData : public UpdateModuleData
{
public:
	UpgradeMuxData				m_upgradeMuxData;
	AsciiString						m_flareTemplateName;
	AsciiString						m_flareBoneBaseName;
  Real									m_evasionRate;
  UnsignedInt						m_volleySize;
  Real									m_volleyArcAngle;
	Real									m_volleyVelocityFactor;
  UnsignedInt						m_framesBetweenVolleys;
	UnsignedInt						m_numberOfVolleys;
  UnsignedInt						m_reloadFrames;
	UnsignedInt						m_missileDecoyFrames;
	UnsignedInt						m_countermeasureReactionFrames;
	Bool									m_mustReloadAtAirfield;
	Bool									m_mustReloadNearDock;
	Bool									m_mustReloadAtBarracks;
	KindOfMaskType 					m_reactingKindofs;
	Bool							m_noAirborne;
	Bool							m_considerGround;
	Bool							m_initiallyActive;
	Bool							m_continuousVolleyInAir;
	std::vector<AsciiString>		m_reloadNearObjects;
	Real							m_dockDistance;
	Int								m_volleyLimit;
	UnsignedInt						m_detonateDistance;


	CountermeasuresBehaviorModuleData()
	{
    m_volleySize            = 0;
		m_volleyArcAngle				= 0.0f;
    m_framesBetweenVolleys  = 0;
		m_numberOfVolleys       = 0;
    m_reloadFrames          = 0;
	m_volleyLimit			= 0;
    m_evasionRate           = 0.0f;
		m_mustReloadAtAirfield	= FALSE;
		m_mustReloadNearDock	= FALSE;
		m_mustReloadAtBarracks	= FALSE;
		m_missileDecoyFrames		= 0;
		m_volleyVelocityFactor  = 1.0f;
		m_reactingKindofs = KINDOFMASK_NONE;
		m_reloadNearObjects.clear();
		m_continuousVolleyInAir = TRUE;
		m_noAirborne = FALSE;
		m_considerGround = FALSE;
		m_initiallyActive = FALSE;
		m_dockDistance = 100.0f;
		m_detonateDistance = 0;
	}

	static void buildFieldParse(MultiIniFieldParse& p)
	{
		static const FieldParse dataFieldParse[] =
		{
			{ "FlareTemplateName",			INI::parseAsciiString,					nullptr, offsetof( CountermeasuresBehaviorModuleData, m_flareTemplateName ) },
			{ "FlareBoneBaseName",			INI::parseAsciiString,					nullptr, offsetof( CountermeasuresBehaviorModuleData, m_flareBoneBaseName ) },
			{ "VolleySize",							INI::parseUnsignedInt,					nullptr, offsetof( CountermeasuresBehaviorModuleData, m_volleySize ) },
			{ "VolleyArcAngle",					INI::parseAngleReal,						nullptr, offsetof( CountermeasuresBehaviorModuleData, m_volleyArcAngle ) },
			{ "VolleyVelocityFactor",		INI::parseReal,						nullptr, offsetof( CountermeasuresBehaviorModuleData, m_volleyVelocityFactor ) },
			{ "DelayBetweenVolleys",		INI::parseDurationUnsignedInt,  nullptr, offsetof( CountermeasuresBehaviorModuleData, m_framesBetweenVolleys ) },
			{ "NumberOfVolleys",				INI::parseUnsignedInt,					nullptr, offsetof( CountermeasuresBehaviorModuleData, m_numberOfVolleys ) },
			{ "ReloadTime",							INI::parseDurationUnsignedInt,  nullptr, offsetof( CountermeasuresBehaviorModuleData, m_reloadFrames ) },
			{ "EvasionRate",						INI::parsePercentToReal,				nullptr, offsetof( CountermeasuresBehaviorModuleData, m_evasionRate ) },
			{ "MustReloadAtAirfield",		INI::parseBool,									nullptr, offsetof( CountermeasuresBehaviorModuleData, m_mustReloadAtAirfield ) },
			{ "MissileDecoyDelay",			INI::parseDurationUnsignedInt,	nullptr, offsetof( CountermeasuresBehaviorModuleData, m_missileDecoyFrames ) },
			{ "ReactionLaunchLatency",	INI::parseDurationUnsignedInt,	nullptr, offsetof( CountermeasuresBehaviorModuleData, m_countermeasureReactionFrames ) },
			
			{ "StartsActive",	INI::parseBool, 				nullptr, offsetof( CountermeasuresBehaviorModuleData, m_initiallyActive ) },
			{ "VolleyLimitPerMissile",	INI::parseInt,					nullptr, offsetof( CountermeasuresBehaviorModuleData, m_volleyLimit ) },
			{ "ContinuousVolleyInAir",	INI::parseBool,					nullptr, offsetof( CountermeasuresBehaviorModuleData, m_continuousVolleyInAir ) },
			{ "ReactingToKindOfs",	KindOfMaskType::parseFromINI,	nullptr, offsetof( CountermeasuresBehaviorModuleData, m_reactingKindofs ) },
			{ "NoAirboneCountermeasures",	INI::parseBool,					nullptr, offsetof( CountermeasuresBehaviorModuleData, m_noAirborne ) },
			{ "GroundCountermeasures",	INI::parseBool,					nullptr, offsetof( CountermeasuresBehaviorModuleData, m_considerGround ) },
			{ "NonTrackingDetonateDistance", INI::parseUnsignedInt,					nullptr, offsetof( CountermeasuresBehaviorModuleData, m_detonateDistance ) },
			{ "MustReloadAtBarracks",		INI::parseBool,									nullptr, offsetof( CountermeasuresBehaviorModuleData, m_mustReloadAtBarracks ) },
			{ "MustReloadNearRepairDocks",		INI::parseBool,								nullptr, offsetof( CountermeasuresBehaviorModuleData, m_mustReloadNearDock ) },
			{ "MustReloadObjectDistance",	INI::parseReal,							nullptr,		offsetof( CountermeasuresBehaviorModuleData, m_dockDistance ) },
			{ "MustReloadNearObjects",	INI::parseAsciiStringVector,				nullptr,		offsetof( CountermeasuresBehaviorModuleData, m_reloadNearObjects ) },
			{ 0, 0, 0, 0 }
		};

		UpdateModuleData::buildFieldParse(p);
		p.add(dataFieldParse);
		p.add(UpgradeMuxData::getFieldParse(), offsetof( CountermeasuresBehaviorModuleData, m_upgradeMuxData ));
	}

};


// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
class CountermeasuresBehaviorInterface
{
public:
	virtual void reportMissileForCountermeasures( Object *missile ) = 0;
	virtual ObjectID calculateCountermeasureToDivertTo( const Object& victim ) = 0;
	virtual void reloadCountermeasures() = 0;
	virtual void setCountermeasuresParked() = 0;
	virtual Bool isActive() const = 0;
	virtual Bool getCountermeasuresMustReloadAtAirfield() const = 0;
	virtual Bool getCountermeasuresMustReloadAtDocks() const = 0;
	virtual Bool getCountermeasuresMustReloadAtBarracks() const = 0;
	virtual Bool getCountermeasuresNoAirborne() const = 0;
	virtual Bool getCountermeasuresConsiderGround() const = 0;
	virtual KindOfMaskType getCountermeasuresKindOfs() const = 0;
};


typedef std::vector<ObjectID> CountermeasuresVec;

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
class CountermeasuresBehavior : public UpdateModule, public UpgradeMux, public CountermeasuresBehaviorInterface
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( CountermeasuresBehavior, "CountermeasuresBehavior" )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( CountermeasuresBehavior, CountermeasuresBehaviorModuleData )

public:

	CountermeasuresBehavior( Thing *thing, const ModuleData* moduleData );
	// virtual destructor prototype provided by memory pool declaration

	// module methods
	static Int getInterfaceMask() { return UpdateModule::getInterfaceMask() | MODULEINTERFACE_UPGRADE; }

	// BehaviorModule
	virtual UpgradeModuleInterface* getUpgrade() { return this; }
	virtual CountermeasuresBehaviorInterface* getCountermeasuresBehaviorInterface() { return this; }
	virtual const CountermeasuresBehaviorInterface* getCountermeasuresBehaviorInterface() const { return this; }

	// UpdateModuleInterface
	virtual UpdateSleepTime update();
	virtual DisabledMaskType getDisabledTypesToProcess() const { return MAKE_DISABLED_MASK( DISABLED_HELD ); }

	// CountermeasuresBehaviorInterface
	virtual void reportMissileForCountermeasures( Object *missile );
	virtual ObjectID calculateCountermeasureToDivertTo( const Object& victim );
	virtual void reloadCountermeasures();
	virtual void setCountermeasuresParked();
	virtual Bool isActive() const;
	virtual Bool getCountermeasuresMustReloadAtAirfield() const;
	virtual Bool getCountermeasuresMustReloadAtDocks() const;
	virtual Bool getCountermeasuresMustReloadAtBarracks() const;
	virtual Bool getCountermeasuresNoAirborne() const;
	virtual Bool getCountermeasuresConsiderGround() const;
	virtual KindOfMaskType getCountermeasuresKindOfs() const;

protected:

	virtual void upgradeImplementation();

	virtual void getUpgradeActivationMasks(UpgradeMaskType& activation, UpgradeMaskType& conflicting) const
	{
		getCountermeasuresBehaviorModuleData()->m_upgradeMuxData.getUpgradeActivationMasks(activation, conflicting);
	}

	virtual void performUpgradeFX()
	{
		getCountermeasuresBehaviorModuleData()->m_upgradeMuxData.performUpgradeFX(getObject());
	}

	virtual void processUpgradeGrant()
	{
		// I can't take it any more.  Let the record show that I think the UpgradeMux multiple inheritence is CRAP.
		getCountermeasuresBehaviorModuleData()->m_upgradeMuxData.muxDataProcessUpgradeGrant(getObject());
	}

	virtual void processUpgradeRemoval()
	{
		// I can't take it any more.  Let the record show that I think the UpgradeMux multiple inheritence is CRAP.
		getCountermeasuresBehaviorModuleData()->m_upgradeMuxData.muxDataProcessUpgradeRemoval(getObject());
	}

	virtual Bool requiresAllActivationUpgrades() const
	{
		return getCountermeasuresBehaviorModuleData()->m_upgradeMuxData.m_requiresAllTriggers;
	}

	Bool isUpgradeActive() const { return isAlreadyUpgraded(); }

	virtual Bool isSubObjectsUpgrade() { return false; }
	virtual Bool hasUpgradeRefresh() { return true; }

	void launchVolley();

private:
	CountermeasuresVec m_counterMeasures;		//vector of countermeasures in the world.
	UnsignedInt m_availableCountermeasures;	//number of countermeasures that can be launched to divert missiles.
	UnsignedInt m_activeCountermeasures;		//number of countermeasures currently able to divert missiles.
	UnsignedInt m_divertedMissiles;					//number of missiles that have been diverted to countermeasures.
	UnsignedInt m_incomingMissiles;					//grand total of all missiles that were ever fired at me.
	UnsignedInt m_reactionFrame;						//The frame countermeasures will be launched after initial hostile act.
	UnsignedInt m_nextVolleyFrame;					//Frame the next volley is fired.
	UnsignedInt m_reloadFrame;							//The frame countermeasures will be ready to use again.
	UnsignedInt m_currentVolley;
	UnsignedInt m_checkDelay;
	Bool m_parked;
	Bool m_hasExecuted;
	ObjectID m_dockObjectID;
};

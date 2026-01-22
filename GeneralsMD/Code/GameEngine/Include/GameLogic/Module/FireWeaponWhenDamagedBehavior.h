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

// FILE: FireWeaponWhenDamagedBehavior.h /////////////////////////////////////////////////////////////////////////
// Author: Steven Johnson, June 2002
// Desc:   Update that will count down a lifetime and destroy object when it reaches zero
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "GameLogic/Module/BehaviorModule.h"
#include "GameLogic/Module/UpgradeModule.h"
#include "GameLogic/Module/UpdateModule.h"
#include "GameLogic/Module/DamageModule.h"
#include "GameLogic/Weapon.h"

//-------------------------------------------------------------------------------------------------
class FireWeaponWhenDamagedBehaviorModuleData : public UpdateModuleData
{
public:
	UpgradeMuxData				m_upgradeMuxData;
	DamageTypeFlags				m_damageTypes;
	Real									m_damageAmount;
	const WeaponTemplate* m_reactionWeaponPristine;///< fire these weapons only when damage is received
	const WeaponTemplate* m_reactionWeaponDamaged;
	const WeaponTemplate* m_reactionWeaponReallyDamaged;
	const WeaponTemplate* m_reactionWeaponRubble;
	const WeaponTemplate*	m_continuousWeaponPristine;///< fire these weapons continuously, versus just onDamage
	const WeaponTemplate*	m_continuousWeaponDamaged;
	const WeaponTemplate*	m_continuousWeaponReallyDamaged;
	const WeaponTemplate*	m_continuousWeaponRubble;
	UnsignedInt				m_continuousDurationPristine;
	UnsignedInt				m_continuousDurationDamaged;
	UnsignedInt				m_continuousDurationReallyDamaged;
	UnsignedInt				m_continuousDurationRubble;
	UnsignedInt				m_continuousIntervalPristine;
	UnsignedInt				m_continuousIntervalDamaged;
	UnsignedInt				m_continuousIntervalReallyDamaged;
	UnsignedInt				m_continuousIntervalRubble;
	AsciiString				m_weaponSlotName;

	DamageFlagsCustom	m_damageTypesCustom;
	CustomFlags 	m_customDamageTypes;

	ObjectStatusMaskType m_requiredStatus;
	ObjectStatusMaskType m_forbiddenStatus;
	std::vector<AsciiString> m_requiredCustomStatus;
	std::vector<AsciiString> m_forbiddenCustomStatus;

	FireWeaponWhenDamagedBehaviorModuleData()
	{
		m_reactionWeaponPristine = nullptr;
		m_reactionWeaponDamaged = nullptr;
		m_reactionWeaponReallyDamaged = nullptr;
		m_reactionWeaponRubble = nullptr;
		m_continuousWeaponPristine = nullptr;
		m_continuousWeaponDamaged = nullptr;
		m_continuousWeaponReallyDamaged = nullptr;
		m_continuousWeaponRubble = nullptr;
		m_damageTypes = DAMAGE_TYPE_FLAGS_ALL;
		m_damageAmount = 0;

		m_weaponSlotName.format("PRIMARY");
		m_damageTypesCustom.first = DAMAGE_TYPE_FLAGS_ALL;
		m_damageTypesCustom.second.format("ALL");
		m_requiredCustomStatus.clear();
		m_forbiddenCustomStatus.clear();

		m_customDamageTypes.clear();

		m_continuousDurationPristine = 0;  
		m_continuousDurationDamaged = 0; 	
		m_continuousDurationReallyDamaged = 0; 	
		m_continuousDurationRubble = 0; 	
		m_continuousIntervalPristine = 0;  
		m_continuousIntervalDamaged = 0; 	
		m_continuousIntervalReallyDamaged = 0; 	
		m_continuousIntervalRubble = 0; 
	}


	static void buildFieldParse(MultiIniFieldParse& p)
	{
		static const FieldParse dataFieldParse[] =
		{
			{ "ReactionWeaponPristine", INI::parseWeaponTemplate, nullptr, offsetof(FireWeaponWhenDamagedBehaviorModuleData,				m_reactionWeaponPristine) },
			{ "ReactionWeaponDamaged", INI::parseWeaponTemplate, nullptr, offsetof(FireWeaponWhenDamagedBehaviorModuleData,				m_reactionWeaponDamaged) },
			{ "ReactionWeaponReallyDamaged", INI::parseWeaponTemplate, nullptr, offsetof(FireWeaponWhenDamagedBehaviorModuleData,	m_reactionWeaponReallyDamaged) },
			{ "ReactionWeaponRubble", INI::parseWeaponTemplate, nullptr, offsetof(FireWeaponWhenDamagedBehaviorModuleData,					m_reactionWeaponRubble) },
			{ "ContinuousWeaponPristine", INI::parseWeaponTemplate, nullptr, offsetof(FireWeaponWhenDamagedBehaviorModuleData,			m_continuousWeaponPristine) },
			{ "ContinuousWeaponDamaged", INI::parseWeaponTemplate, nullptr, offsetof(FireWeaponWhenDamagedBehaviorModuleData,			m_continuousWeaponDamaged) },
			{ "ContinuousWeaponReallyDamaged", INI::parseWeaponTemplate, nullptr, offsetof(FireWeaponWhenDamagedBehaviorModuleData,m_continuousWeaponReallyDamaged) },
			{ "ContinuousWeaponRubble", INI::parseWeaponTemplate, nullptr, offsetof(FireWeaponWhenDamagedBehaviorModuleData,				m_continuousWeaponRubble) },
			{ "DamageTypes", INI::parseDamageTypeFlagsCustom, nullptr, offsetof( FireWeaponWhenDamagedBehaviorModuleData, m_damageTypesCustom ) },
			{ "DamageAmount", INI::parseReal, nullptr, offsetof( FireWeaponWhenDamagedBehaviorModuleData, m_damageAmount ) },
			
			{ "CustomDamageTypes", INI::parseCustomTypes, nullptr, offsetof( FireWeaponWhenDamagedBehaviorModuleData, m_customDamageTypes ) },
			{ "RequiredStatus",		ObjectStatusMaskType::parseFromINI,	nullptr, offsetof( FireWeaponWhenDamagedBehaviorModuleData, m_requiredStatus ) },
			{ "ForbiddenStatus",	ObjectStatusMaskType::parseFromINI,	nullptr, offsetof( FireWeaponWhenDamagedBehaviorModuleData, m_forbiddenStatus ) },
			{ "RequiredCustomStatus",	INI::parseAsciiStringVector, nullptr, 	offsetof( FireWeaponWhenDamagedBehaviorModuleData, m_requiredCustomStatus ) },
			{ "ForbiddenCustomStatus",	INI::parseAsciiStringVector, nullptr, 	offsetof( FireWeaponWhenDamagedBehaviorModuleData, m_forbiddenCustomStatus ) },
			{ "WeaponSlot",		INI::parseQuotedAsciiString,	nullptr, offsetof( FireWeaponWhenDamagedBehaviorModuleData, m_weaponSlotName ) },

			{ "ContinuousDurationPristine", INI::parseDurationUnsignedInt, nullptr, offsetof(FireWeaponWhenDamagedBehaviorModuleData,			m_continuousDurationPristine) },
			{ "ContinuousDurationDamaged", INI::parseDurationUnsignedInt, nullptr, offsetof(FireWeaponWhenDamagedBehaviorModuleData,			m_continuousDurationDamaged) },
			{ "ContinuousDurationReallyDamaged", INI::parseDurationUnsignedInt, nullptr, offsetof(FireWeaponWhenDamagedBehaviorModuleData,m_continuousDurationReallyDamaged) },
			{ "ContinuousDurationRubble", INI::parseDurationUnsignedInt, nullptr, offsetof(FireWeaponWhenDamagedBehaviorModuleData,				m_continuousDurationRubble) },
			{ "ContinuousIntervalPristine", INI::parseDurationUnsignedInt, nullptr, offsetof(FireWeaponWhenDamagedBehaviorModuleData,			m_continuousIntervalPristine) },
			{ "ContinuousIntervalDamaged", INI::parseDurationUnsignedInt, nullptr, offsetof(FireWeaponWhenDamagedBehaviorModuleData,			m_continuousIntervalDamaged) },
			{ "ContinuousIntervalReallyDamaged", INI::parseDurationUnsignedInt, nullptr, offsetof(FireWeaponWhenDamagedBehaviorModuleData,m_continuousIntervalReallyDamaged) },
			{ "ContinuousIntervalRubble", INI::parseDurationUnsignedInt, nullptr, offsetof(FireWeaponWhenDamagedBehaviorModuleData,				m_continuousIntervalRubble) },

			{ 0, 0, 0, 0 }
		};

		UpdateModuleData::buildFieldParse(p);
		p.add(dataFieldParse);
		p.add(UpgradeMuxData::getFieldParse(), offsetof( FireWeaponWhenDamagedBehaviorModuleData, m_upgradeMuxData ));
	}


private:

};

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
class FireWeaponWhenDamagedBehavior : public UpdateModule,
																			public UpgradeMux,
																			public DamageModuleInterface
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( FireWeaponWhenDamagedBehavior, "FireWeaponWhenDamagedBehavior" )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( FireWeaponWhenDamagedBehavior, FireWeaponWhenDamagedBehaviorModuleData )

public:

	FireWeaponWhenDamagedBehavior( Thing *thing, const ModuleData* moduleData );
	// virtual destructor prototype provided by memory pool declaration

	// module methods
	static Int getInterfaceMask() { return UpdateModule::getInterfaceMask() | (MODULEINTERFACE_UPGRADE) | (MODULEINTERFACE_DAMAGE); }

	// BehaviorModule
	virtual UpgradeModuleInterface* getUpgrade() { return this; }
	virtual DamageModuleInterface* getDamage() { return this; }

	// DamageModuleInterface
	virtual void onDamage( DamageInfo *damageInfo );
	virtual void onHealing( DamageInfo *damageInfo ) { }
	virtual void onBodyDamageStateChange(const DamageInfo* damageInfo, BodyDamageType oldState, BodyDamageType newState) { }

	// UpdateModuleInterface
	virtual UpdateSleepTime update();

protected:

	virtual void upgradeImplementation()
	{
		setWakeFrame(getObject(), UPDATE_SLEEP_NONE);
	}

	virtual void getUpgradeActivationMasks(UpgradeMaskType& activation, UpgradeMaskType& conflicting) const
	{
		getFireWeaponWhenDamagedBehaviorModuleData()->m_upgradeMuxData.getUpgradeActivationMasks(activation, conflicting);
	}

	virtual void performUpgradeFX()
	{
		getFireWeaponWhenDamagedBehaviorModuleData()->m_upgradeMuxData.performUpgradeFX(getObject());
	}

	virtual void processUpgradeGrant()
	{
		// I can't take it any more.  Let the record show that I think the UpgradeMux multiple inheritence is CRAP.
		getFireWeaponWhenDamagedBehaviorModuleData()->m_upgradeMuxData.muxDataProcessUpgradeGrant(getObject());
	}

	virtual void processUpgradeRemoval()
	{
		// I can't take it any more.  Let the record show that I think the UpgradeMux multiple inheritance is CRAP.
		getFireWeaponWhenDamagedBehaviorModuleData()->m_upgradeMuxData.muxDataProcessUpgradeRemoval(getObject());
	}

	virtual Bool requiresAllActivationUpgrades() const
	{
		return getFireWeaponWhenDamagedBehaviorModuleData()->m_upgradeMuxData.m_requiresAllTriggers;
	}

	virtual Bool checkStartsActive() const
	{
		return getFireWeaponWhenDamagedBehaviorModuleData()->m_upgradeMuxData.muxDataCheckStartsActive(getObject());
	}

	Bool isUpgradeActive() const { return isAlreadyUpgraded(); }

	virtual Bool isSubObjectsUpgrade() { return false; }
	virtual Bool hasUpgradeRefresh() { return false; }

private:
	Weapon *m_reactionWeaponPristine;
	Weapon *m_reactionWeaponDamaged;
	Weapon *m_reactionWeaponReallyDamaged;
	Weapon *m_reactionWeaponRubble;
	Weapon *m_continuousWeaponPristine;
	Weapon *m_continuousWeaponDamaged;
	Weapon *m_continuousWeaponReallyDamaged;
	Weapon *m_continuousWeaponRubble;
	UnsignedInt	m_duration;
	UnsignedInt	m_interval;
	Bool m_innate;

};

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

// FILE: EnergyShieldBehavior.h /////////////////////////////////////////////////////////////////////////
// Author: Colin Day, December 2001
// Desc:   Update that heals itself
//------------------------------------------
// Modified by Kris Morness, September 2002
// Kris: Added the ability to add effects, radius healing, and restricting the type of objects
//       subjected to the heal (or repair).
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "GameClient/ParticleSys.h"
#include "GameLogic/Module/BehaviorModule.h"
#include "GameLogic/Module/UpgradeModule.h"
#include "GameLogic/Module/UpdateModule.h"
#include "GameLogic/Module/DamageModule.h"
#include "Common/BitFlagsIO.h"


class ShieldBody;
class FXList;

//-------------------------------------------------------------------------------------------------
class EnergyShieldBehaviorModuleData : public UpdateModuleData
{
public:
	UpgradeMuxData				m_upgradeMuxData;
	//Bool									m_initiallyActive;
	//Real m_shieldMaxHealth;  ///< MaxHealth of the shield
	//Real m_shieldMaxHealthPercent;  ///< MaxHealth as percentage of activeBody MaxHealth (takes priority)

	UnsignedInt m_shieldRechargeDelay;  ///< frames of no damage taken until shield recharges
	UnsignedInt m_shieldRechargeRate;   ///< every this often, we heal the shield ...
	Real m_shieldRechargeAmount;					///< by this much.
	Real m_shieldRechargeAmountPercent;		///< Same as above but percentage of shieldMaxHealth (takes priority)

	RGBAColorInt m_barColor;
	RGBAColorInt m_barBGColor;
	Bool m_showBarWhenEmpty;
	Bool m_showBarWhenUnselected;

	ModelConditionFlagType m_shieldConditionFlag;
	ModelConditionFlagType m_shieldHitConditionFlag;
	AsciiString m_shieldSubObjName;
	AsciiString m_shieldHitSubObjName;
	UnsignedInt m_showShieldWhenHitDuration;
	//Bool m_alwaysShowShield;

	FXList* m_fxShieldUp;
	FXList* m_fxShieldDown;



	//DamageTypeFlags m_damageTypesToPassThrough;

	EnergyShieldBehaviorModuleData();

	static void buildFieldParse(MultiIniFieldParse& p);

	//static void parseShieldHealthPercent(INI* ini, void* instance, void* store, const void* userData);
	//static void parseShieldRechargeAmountPercent(INI* ini, void* instance, void* store, const void* userData);

};
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
class EnergyShieldBehaviorInterface
{
public:
	virtual void applyDamage(Real amount) = 0;
	virtual bool isActive() const = 0;
	virtual bool shouldShowHealthBar(bool selected) const = 0;
	virtual Real getShieldPercent() const = 0;
	virtual RGBAColorInt getHealthBarColor() const = 0;
	virtual RGBAColorInt getHealthBarBackgroundColor() const = 0;
};
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
class EnergyShieldBehavior : public UpdateModule,
												 public UpgradeMux,
												 public EnergyShieldBehaviorInterface
												 //public DamageModuleInterface
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( EnergyShieldBehavior, "EnergyShieldBehavior" )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( EnergyShieldBehavior, EnergyShieldBehaviorModuleData )

public:

	EnergyShieldBehavior( Thing *thing, const ModuleData* moduleData );
	// virtual destructor prototype provided by memory pool declaration

	// module methods
	static Int getInterfaceMask() { return UpdateModule::getInterfaceMask() | MODULEINTERFACE_UPGRADE; }  // | MODULEINTERFACE_DAMAGE; 

	// BehaviorModule
	virtual UpgradeModuleInterface* getUpgrade() override { return this; }
	virtual EnergyShieldBehaviorInterface* getEnergyShieldBehaviorInterface() override { return this; }
	//virtual DamageModuleInterface* getDamage() override { return this; }

	// DamageModuleInterface
	//virtual void onDamage( DamageInfo *damageInfo ) override;
	//virtual void onHealing( DamageInfo *damageInfo ) override { }
	//virtual void onBodyDamageStateChange(const DamageInfo* damageInfo, BodyDamageType oldState, BodyDamageType newState) override { }

	// UpdateModuleInterface
	virtual UpdateSleepTime update() override;
	//virtual DisabledMaskType getDisabledTypesToProcess() const override { return DISABLEDMASK_ALL; }

	//EnergyShieldBehaviorInterface
	virtual void applyDamage(Real amount) override;
	virtual bool isActive() const override { return isUpgradeActive(); }
	virtual bool shouldShowHealthBar(bool selected) const override;
	virtual Real getShieldPercent() const override;
	virtual RGBAColorInt getHealthBarColor() const override { return getEnergyShieldBehaviorModuleData()->m_barColor; }
	virtual RGBAColorInt getHealthBarBackgroundColor() const override { return getEnergyShieldBehaviorModuleData()->m_barBGColor; };

protected:

	virtual void upgradeImplementation() override;

	virtual void getUpgradeActivationMasks(UpgradeMaskType& activation, UpgradeMaskType& conflicting) const override
	{
		getEnergyShieldBehaviorModuleData()->m_upgradeMuxData.getUpgradeActivationMasks(activation, conflicting);
	}

	virtual void performUpgradeFX() override
	{
		getEnergyShieldBehaviorModuleData()->m_upgradeMuxData.performUpgradeFX(getObject());
	}

	virtual void processUpgradeGrant() override
	{
		// I can't take it any more.  Let the record show that I think the UpgradeMux multiple inheritence is CRAP.
		getEnergyShieldBehaviorModuleData()->m_upgradeMuxData.muxDataProcessUpgradeGrant(getObject());
	}

	virtual void processUpgradeRemoval() override
	{
		// I can't take it any more.  Let the record show that I think the UpgradeMux multiple inheritence is CRAP.
		getEnergyShieldBehaviorModuleData()->m_upgradeMuxData.muxDataProcessUpgradeRemoval(getObject());
	}

	virtual Bool requiresAllActivationUpgrades() const override
	{
		return getEnergyShieldBehaviorModuleData()->m_upgradeMuxData.m_requiresAllTriggers;
	}

	virtual Bool checkStartsActive() const override
	{
		return getEnergyShieldBehaviorModuleData()->m_upgradeMuxData.muxDataCheckStartsActive(getObject());
	}

	Bool isUpgradeActive() const { return isAlreadyUpgraded(); }

	virtual Bool isSubObjectsUpgrade() override { return false; }

	virtual Bool hasUpgradeRefresh() override { return false; }


private:

	ShieldBody*           m_body;
	//Real									m_currentShieldHealth;
	UnsignedInt           m_healingStepCountdown;
	UnsignedInt						m_shieldHitCountdown;

	void showShield(bool show, bool isHit = false);
	Bool alwaysShowShield() const;
};



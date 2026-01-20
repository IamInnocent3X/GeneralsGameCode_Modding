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

// FILE: SalvageCrateCollide.h /////////////////////////////////////////////////////////////////////////
// Author: Graham Smallwood, March 2002
// Desc:   The Savlage system can give a Weaponset bonus, a level, or money.  Salvagers create them
//					by killing marked units, and only WeaponSalvagers can get the WeaponSet bonus
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "Common/Module.h"
#include "Common/STLTypedefs.h"
#include "GameLogic/Module/CrateCollide.h"

//-------------------------------------------------------------------------------------------------
enum WeaponSetType CPP_11(: Int);
enum ArmorSetType CPP_11(: Int);
enum ModelConditionFlagType CPP_11(: Int);
enum WeaponBonusConditionType CPP_11(: Int);
enum ObjectStatusType CPP_11(: Int);
enum ScienceType CPP_11(: Int);

enum SalvagePickupType CPP_11(: Int)
{
	SALVAGE_ARMOR						= 0x001,
	SALVAGE_WEAPON					= 0x002,
	SALVAGE_LEVEL				= 0x004,
	SALVAGE_MONEY				= 0x008,
	SALVAGE_UPGRADES			= 0x010,
	SALVAGE_SCIENCE				= 0x020,
	SALVAGE_SCIENCE_POINTS		= 0x040,
	SALVAGE_SKILL_POINTS		= 0x080,
	SALVAGE_STATUS				= 0x100,
	SALVAGE_WEAPON_BONUS		= 0x200,
	SALVAGE_OCL					= 0x400,
};

#ifdef SALVAGE_TYPE_NAMES
static const char *TheSalvagePickupNames[] =
{
	"ARMOR",
	"WEAPON",
	"LEVEL",
	"MONEY",
	"UPGRADES",
	"SCIENCE",
	"SCIENCE_POINTS",
	"SKILL_POINTS",
	"STATUS",
	"WEAPON_BONUS",
	"OCL",
	nullptr
};
#endif


// FORWARD REFERENCES /////////////////////////////////////////////////////////////////////////////
class Thing;


struct BonusInfo
{
	Int Bonus;
	Int Amount;
	Int MinAmount;
	Int MaxAmount;
	Real Chance;
};
struct CompletionInfo
{
	Int Completed;
	std::vector<BonusInfo> Bonuses;
};
struct WeaponBonusTypes
{
	AsciiString customType;
	WeaponBonusConditionType bonusType;

	WeaponBonusTypes() : bonusType((WeaponBonusConditionType)(-1)), customType(AsciiString::TheEmptyString)
	{
	}
};
struct StatusTypes
{
	AsciiString customType;
	ObjectStatusType statusType;

	StatusTypes() : statusType((ObjectStatusType)(-1)), customType(AsciiString::TheEmptyString)
	{
	}
};

//-------------------------------------------------------------------------------------------------
class SalvageCrateCollideModuleData : public CrateCollideModuleData
{
public:
	Real m_weaponChance;	///< Chance to get a weapon upgrade, if possible
	Real m_levelChance;		///< Chance to get a level, if weaponChance fails
	Real m_moneyChance;		///< Chance to get money, if weaponChance fails
	Int m_minimumMoney;		///< How much, if we get money
	Int m_maximumMoney;		///< How much, if we get money

	Int m_minimumSciencePurchasePoints;
	Int m_maximumSciencePurchasePoints;
	Int m_minimumSkillPoints;
	Int m_maximumSkillPoints;

	Real m_scienceChance;
	Real m_sciencePurchasePointsChance;
	Real m_skillPointsChance;
	Real m_weaponBonusChance;
	Real m_statusChance;
	Real m_upgradeChance;
	Real m_oclChance;

	Bool m_removePreviousUpgrades;
	Bool m_removePreviousWeaponBonus;
	Bool m_removePreviousStatus;
	
	KindOfMaskType	m_kindofWeaponSalvager;				///< the kind(s) of units that can salvage Weapon
	KindOfMaskType	m_kindofnotWeaponSalvager;		///< the kind(s) of units that CANNOT salvage Weapon
	KindOfMaskType	m_kindofArmorSalvager;				///< the kind(s) of units that can salvage Armor
	KindOfMaskType	m_kindofnotArmorSalvager;		///< the kind(s) of units that CANNOT salvage Armor

	std::vector<SalvagePickupType> m_pickupOrder;
	std::vector<WeaponSetType> m_weaponSetFlags;
	std::vector<ArmorSetType> m_armorSetFlags;
	std::vector<ModelConditionFlagType> m_weaponModelConditionFlags;
	std::vector<ModelConditionFlagType> m_armorModelConditionFlags;
	std::vector<WeaponBonusTypes> m_weaponBonus;
	std::vector<StatusTypes> m_status;
	std::vector<AsciiString> m_grantUpgradeNames;
	std::vector<ScienceType> m_sciencesGranted;
	std::vector<const ObjectCreationList*> m_ocls;

	Bool m_crateGivesMultiBonus;
	Int m_multiPickupFlags;
	std::vector<CompletionInfo> m_multiFlagCompletionInfos;

	SalvageCrateCollideModuleData();
	/*{
		m_kindof = KINDOF_SALVAGER;
		m_kindofWeaponSalvager = KINDOF_WEAPON_SALVAGER;
		m_kindofArmorSalvager = KINDOF_ARMOR_SALVAGER;
		m_crateGivesMultiBonus = FALSE;
		m_multiPickupFlags = 0;
		m_weaponChance = 1.0f;
		m_levelChance = .25f;
		m_moneyChance = .75f;
		m_minimumMoney = 25;
		m_maximumMoney = 75;
	}*/

	static void buildFieldParse(MultiIniFieldParse& p);
	/*{
    CrateCollideModuleData::buildFieldParse(p);

		static const FieldParse dataFieldParse[] =
		{
			{ "WeaponChance",	INI::parsePercentToReal,	nullptr, offsetof( SalvageCrateCollideModuleData, m_weaponChance ) },
			{ "LevelChance",	INI::parsePercentToReal,	nullptr, offsetof( SalvageCrateCollideModuleData, m_levelChance ) },
			{ "MoneyChance",	INI::parsePercentToReal,	nullptr, offsetof( SalvageCrateCollideModuleData, m_moneyChance ) },
			{ "MinMoney",			INI::parseInt,						nullptr, offsetof( SalvageCrateCollideModuleData, m_minimumMoney ) },
			{ "MaxMoney",			INI::parseInt,						nullptr, offsetof( SalvageCrateCollideModuleData, m_maximumMoney ) },
			{ 0, 0, 0, 0 }
		};
    p.add(dataFieldParse);

	}*/

	static void parseAmountModifier( INI* ini, void *instance, void *store, const void* /*userData*/ );
	static void parseChanceModifier( INI* ini, void *instance, void *store, const void* /*userData*/ );
	static void parseWeaponBonus( INI* ini, void *instance, void *store, const void* /*userData*/ );
	static void parseStatus( INI* ini, void *instance, void *store, const void* /*userData*/ );
};

//-------------------------------------------------------------------------------------------------
class SalvageCrateCollide : public CrateCollide
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( SalvageCrateCollide, "SalvageCrateCollide" )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( SalvageCrateCollide, SalvageCrateCollideModuleData );

public:

	SalvageCrateCollide( Thing *thing, const ModuleData* moduleData );
	// virtual destructor prototype provided by memory pool declaration

	virtual Bool isSalvageCrateCollide() const { return true; }

protected:

	/// This allows specific vetoes to certain types of crates and their data
	virtual Bool isValidToExecute( const Object *other ) const;

	/// This is the game logic execution function that all real CrateCollides will implement
	virtual Bool executeCrateBehavior( Object *other );

private:
	Bool eligibleForWeaponSet( Object *other );
	Bool eligibleForArmorSet( Object *other );
	Bool eligibleForLevel( Object *other );
	Bool eligibleForUpgrades( Object *other );
	Bool eligibleForScience( Object *other );
	Bool eligibleForSciencePurchasePoints( Object *other );
	Bool eligibleForSkillPoints( Object *other );
	Bool eligibleForWeaponBonus( Object *other );
	Bool eligibleForStatus( Object *other );
	Bool testWeaponChance();
	Bool testLevelChance();
	Bool testUpgradeChance();
	Bool testScienceChance();
	Bool testSciencePurchasePointsChance();
	Bool testSkillPointsChance();
	Bool testWeaponBonusChance();
	Bool testStatusChance();
	Bool testOCLChance();

	Bool testArmorChanceMulti( Object *other );
	Bool testWeaponChanceMulti( Object *other );
	Bool testLevelChanceMulti( Object *other );
	Bool testUpgradeChanceMulti( Object *other );
	Bool testScienceChanceMulti( Object *other );
	Bool testSciencePurchasePointsChanceMulti( Object *other );
	Bool testSkillPointsChanceMulti( Object *other );
	Bool testWeaponBonusChanceMulti( Object *other );
	Bool testStatusChanceMulti( Object *other );
	Bool testOCLChanceMulti( Object *other );

	Bool testWeaponBonus( Object *other, WeaponBonusTypes bonus ) const;
	Bool testStatus( Object *other, StatusTypes status ) const;
	Bool testUpgrade( Object *other, Bool testMax ) const;
	Bool testSciencePurchasePoints( Object *other, Bool testMax ) const;
	Bool testSkillPoints( Object *other, Bool testMax ) const;

	Bool multiFlagEligibleConditions( Object *other, Int completed, SalvagePickupType exceptions ) const;

	void doWeaponSet( Object *other );
	void doArmorSet( Object *other );
	void doLevelGain( Object *other );
	//void doMoney( Object *other );
	Bool doMoney( Object *other );

	void grantUpgrades( Object *other );
	void grantSciences( Object *other );
	Bool grantSciencePurchasePoints( Object *other );
	Bool grantSkillPoints( Object *other );

	void doWeaponBonus( Object *other );
	void doStatus( Object *other );
	void doUpgrade( Object *other, const AsciiString& grantUpgradeName, AsciiString removeUpgradeName = AsciiString::TheEmptyString );
	Bool doOCL( Object *other );

};

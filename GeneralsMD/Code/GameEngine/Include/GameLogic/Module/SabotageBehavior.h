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
// FILE: SabotageBehavior.h
// Original Author: Kris Morness, June 2003
// Modified By: IamInnocent, February 2026
// Desc:   A crate (actually a saboteur - mobile crate) that makes the target powerplant lose power
//
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "Common/Module.h"
#include "Common/BitFlags.h"
#include "Common/DisabledTypes.h"
#include "Common/Science.h"
#include "GameClient/TintStatus.h"
#include "GameLogic/Module/CostModifierUpgrade.h"
#include "GameLogic/Module/CrateCollide.h"

// FORWARD REFERENCES /////////////////////////////////////////////////////////////////////////////
class Thing;

enum SabotageType CPP_11(: Int)
{
	SABOTAGE_SPECIAL_POWER					= 0x01,
	SABOTAGE_PRODUCTION				= 0x02,
	SABOTAGE_POWER				= 0x04,	
	SABOTAGE_CASH							= 0x08,
};

static const char *const TheSabotageNames[] =
{
	"SPECIAL_POWER",
	"PRODUCTION",
	"POWER",
	"CASH",
	nullptr
};

enum SabotageVictimType CPP_11(: Int)
{
	SAB_VICTIM_GENERIC = 0,
	SAB_VICTIM_COMMAND_CENTER,
	SAB_VICTIM_FAKE_BUILDING,
	SAB_VICTIM_INTERNET_CENTER,
	SAB_VICTIM_MILITARY_FACTORY,
	SAB_VICTIM_POWER_PLANT,
	SAB_VICTIM_SUPERWEAPON,
	SAB_VICTIM_SUPPLY_CENTER,
	SAB_VICTIM_DROP_ZONE,

	SAB_VICTIM_COUNT
};

static const char *const TheSabotageFeedbackNames[] =
{
	"GENERIC",
	"COMMAND_CENTER",
	"FAKE_BUILDING",
	"INTERNET_CENTER",
	"MILITARY_FACTORY",
	"POWER_PLANT",
	"SUPERWEAPON",
	"SUPPLY_CENTER",
	"DROP_ZONE",
	nullptr
};
static_assert(ARRAY_SIZE(TheSabotageFeedbackNames) == SAB_VICTIM_COUNT + 1, "Incorrect array size");

enum SabotageGrantType CPP_11(: Int)
{
	GRANT_ALL = 0,
	GRANT_FIRST_DONT_HAVE,
	GRANT_LAST_DONT_HAVE,
	GRANT_LIKE_LEVELING_UP,
	GRANT_RANDOM,

	GRANT_COUNT
};

static const char *const TheSabotageGrantNames[] =
{
	"ALL",
	"FIRST_DONT_HAVE",
	"LAST_DONT_HAVE",
	"LIKE_LEVELING_UP",
	"RANDOM",
	nullptr
};
static_assert(ARRAY_SIZE(TheSabotageGrantNames) == GRANT_COUNT + 1, "Incorrect array size");

typedef std::pair<AsciiString, std::vector<AsciiString>> ObjectUpgradeVecPair;
typedef std::vector<ObjectUpgradeVecPair> ObjectUpgradeVec;
typedef std::pair<AsciiString, ScienceVec> ObjectScienceVecPair;
typedef std::vector<ObjectScienceVecPair> ObjectScienceVec;

//-------------------------------------------------------------------------------------------------
class SabotageBehaviorModuleData : public CrateCollideModuleData
{
public:
	Int m_sabotageType;
	SabotageVictimType m_feedbackType;
	AsciiString m_feedbackSound;
	AsciiString m_specialPowerTemplateToTrigger;
	
	TintStatus m_tintStatus;
	AsciiString m_customTintStatus;

	UnsignedInt m_sabotageFrames;
	UnsignedInt m_stealCashAmount;
	UnsignedInt m_powerSabotageFrames;

	Bool m_sabotageIsCapture;
	Bool m_sabotageIsCollide;
	Bool m_sabotageDoAlert;
	Bool m_sabotageDisable;
	Bool m_sabotageDisableContained;
	Bool m_sabotageDisableCommands;
	DisabledType m_sabotageDisabledType;

	Bool m_sabotageResetSpecialPowers;
	Bool m_sabotagePauseSpecialPowers;
	Bool m_sabotageResetAllSameObjectsSpecialPowers;
	Bool m_sabotagePauseAllSameObjectsSpecialPowers;
	Bool m_sabotageResetSpyVision;
	Bool m_sabotageDisableSpyVision;
	Bool m_sabotageDisableSpyVisionDoesNotResetTimer;
	Bool m_sabotageResetOCLUpdate;
	Bool m_sabotageDisableOCLUpdate;
	Bool m_sabotageResetAllSpyVision;
	Bool m_sabotageDisableAllSpyVision;
	Bool m_sabotageResetAllOCLUpdate;
	Bool m_sabotageDisableAllOCLUpdate;
	Bool m_useCommandsUponSabotage;
	Bool m_useCommandsNeedsToBeReady;

	Bool m_stealsPower;
	Int m_powerAmount;

	Real m_powerPercentage;
	Real m_stealCashPercentage;
	Real m_sabotageDamage;
	Real m_sabotagePercentDamage;
	Real m_sabotageCaptureBelowHealth;
	Real m_sabotageCaptureBelowHealthPercent;

	ScienceVec m_sciencesGranted;
	std::vector<AsciiString> m_powerSabotageSpecificObjects;
	std::vector<AsciiString> m_commandsToDisable;
	std::vector<AsciiString> m_grantUpgradeNames;
	std::vector<NameKeyType> m_commandInstancesToGrant;
	NameKeyIntVec m_commandInstancesToGrantWithAmount;
	std::vector<AsciiString> m_grantUpgradeNamesIfCollidesWith;
	ObjectScienceVec m_sciencesGrantedIfCollidesWith;
	SabotageGrantType m_sciencesGrantType;
	SabotageGrantType m_upgradesGrantType;

	KindOfMaskType m_powerSabotageSpecificKindOf;
	KindOfMaskType m_sabotageDisableAllKindOf;
	KindOfMaskType m_sabotageDisableAllForbiddenKindOf;
	KindOfMaskType m_sabotageCostModifierKindOf;
	KindOfMaskType m_sabotageTimeModifierKindOf;
	UnsignedInt m_sabotageCostModifierFrames;
	UnsignedInt m_sabotageTimeModifierFrames;
	Int m_sabotageProductionViewFrames;

	Real m_sabotageCostModifierPercentage;
	Real m_sabotageTimeModifierPercentage;
	BonusStackingType m_sabotageCostModifierStackingType;
	BonusStackingType m_sabotageTimeModifierStackingType;

	SabotageBehaviorModuleData()
	{
		m_sabotageType = 0;
		m_stealCashAmount = 0;
		m_sabotageFrames = 0;
		m_sabotageProductionViewFrames = 0;
		m_powerSabotageFrames = 0;
		m_powerAmount = 0;
		m_powerPercentage = 0.0f;
		m_stealCashPercentage = 0.0f;
		m_sabotageDamage = 0.0f;
		m_sabotagePercentDamage = 0.0f;
		m_sabotageCaptureBelowHealth = 0.0f;
		m_sabotageCaptureBelowHealthPercent = 0.0f;
		m_sabotageIsCollide = FALSE;
		m_sabotageIsCapture = FALSE;
		m_sabotageDisable = FALSE;
		m_sabotageDisableContained = FALSE;
		m_sabotageDisableCommands = FALSE;
		m_sabotageResetSpecialPowers = FALSE;
		m_sabotagePauseSpecialPowers = FALSE;
		m_sabotageResetAllSameObjectsSpecialPowers = FALSE;
		m_sabotagePauseAllSameObjectsSpecialPowers = FALSE;
		m_sabotageResetSpyVision = FALSE;
		m_sabotageDisableSpyVision = FALSE;
		m_sabotageDisableSpyVisionDoesNotResetTimer = FALSE;
		m_sabotageResetOCLUpdate = FALSE;
		m_sabotageDisableOCLUpdate = FALSE;
		m_sabotageResetAllSpyVision = FALSE;
		m_sabotageDisableAllSpyVision = FALSE;
		m_sabotageResetAllOCLUpdate = FALSE;
		m_sabotageDisableAllOCLUpdate = FALSE;
		m_stealsPower = FALSE;
		m_sabotageDoAlert = TRUE;
		m_useCommandsUponSabotage = FALSE;
		m_useCommandsNeedsToBeReady = FALSE;

		m_commandsToDisable.clear();
		m_commandInstancesToGrant.clear();
		m_commandInstancesToGrantWithAmount.clear();
		m_grantUpgradeNames.clear();
		m_grantUpgradeNamesIfCollidesWith.clear();
		m_upgradesGrantType = GRANT_ALL;
		m_sciencesGranted.clear();
		m_sciencesGrantedIfCollidesWith.clear();
		m_sciencesGrantType = GRANT_ALL;

		m_powerSabotageSpecificObjects.clear();
		m_powerSabotageSpecificKindOf.clear();
		m_sabotageCostModifierKindOf.clear();
		m_sabotageTimeModifierKindOf.clear();
		m_sabotageDisableAllKindOf.clear();
		m_sabotageDisableAllForbiddenKindOf.clear();
		m_sabotageCostModifierFrames = 0;
		m_sabotageTimeModifierFrames = 0;
		m_feedbackType = SAB_VICTIM_GENERIC;
		m_sabotageDisabledType = DISABLED_HACKED;
		m_tintStatus = TINT_STATUS_INVALID;
		m_specialPowerTemplateToTrigger.clear();
		m_customTintStatus.clear();
		m_feedbackSound.clear();
	}

	static void parseDuration( INI *ini, void * /*instance*/, void *store, const void* /*userData*/ )
	{
		Int val = INI::scanInt(ini->getNextToken());
		*(Int *)store = val > 0 ? (Int)ceilf(ConvertDurationFromMsecsToFrames((Real)val)) : val;
	}

	static void parseNameKeyVectorWithAmount( INI* ini, void * /*instance*/, void *store, const void* /*userData*/ )
	{
		if (!TheNameKeyGenerator) {
			DEBUG_LOG(("name key generator not found"));
			DEBUG_CRASH(("name key generator not found"));
			throw INI_INVALID_DATA;
		}

		NameKeyIntVec* nkv = (NameKeyIntVec*)store;
		nkv->clear();
		NameKeyIntPair parseData;
		Bool parseKey = TRUE;
		for (const char *token = ini->getNextTokenOrNull(); token != nullptr; token = ini->getNextTokenOrNull())
		{
			if(parseKey)
			{
				parseData.first = TheNameKeyGenerator->nameToKey(token);
				parseKey = FALSE;
			}
			else
			{
				parseData.second = INI::scanInt(token);
				parseKey = TRUE;
				nkv->push_back(parseData);
			}
		}
		if(!parseKey)
		{
			parseData.second = 0;
			nkv->push_back(parseData);
		}
	}

	static void parseObjectScienceVec( INI *ini, void * /*instance*/, void *store, const void *userData )
	{
		ObjectScienceVec* vec = (ObjectScienceVec*)store;
		// Don't clear, it will be a long list
		//asv->clear();
		ScienceVec sciences;
		AsciiString objName;
		objName.clear();
		for (const char *token = ini->getNextTokenOrNull(); token != nullptr; token = ini->getNextTokenOrNull())
		{
			if (stricmp(token, "None") == 0)
			{
				vec->clear();
				return;
			}

			NameKeyType nkt = NAMEKEY(token);
			ScienceType st = (ScienceType)nkt;
			if(TheScienceStore->isValidScience(st))
			{
				if(objName.isEmpty())
				{
					DEBUG_LOG(("object name not found for parseObjectScienceVec"));
					DEBUG_CRASH(("object name not found for parseObjectScienceVec"));
					throw INI_INVALID_DATA;
				}
				sciences.push_back(INI::scanScience( token ));
			}
			else
			{
				if(!objName.isEmpty() && !sciences.empty())
				{
					ObjectScienceVecPair pair;
					pair.first = objName;
					pair.second = sciences;
					vec->push_back(pair);
				}
				sciences.clear();
				objName.format(token);
			}
		}
		if(!objName.isEmpty() && !sciences.empty())
		{
			ObjectScienceVecPair pair;
			pair.first = objName;
			pair.second = sciences;
			vec->push_back(pair);
		}
	}

	static void buildFieldParse(MultiIniFieldParse& p)
	{
    CrateCollideModuleData::buildFieldParse(p);

		static const FieldParse dataFieldParse[] =
		{
			// Sabotage Type
			{ "SabotageType",	INI::parseBitString32, TheSabotageNames, offsetof( SabotageBehaviorModuleData, m_sabotageType ) },
			{ "SabotageFeedbackType",	INI::parseIndexList, TheSabotageFeedbackNames, offsetof( SabotageBehaviorModuleData, m_feedbackType ) },
			{ "SabotageFeedbackSound",	INI::parseAsciiString, nullptr, offsetof( SabotageBehaviorModuleData, m_feedbackSound ) },
			{ "SabotageIsCollide",	INI::parseBool, TheSabotageNames, offsetof( SabotageBehaviorModuleData, m_sabotageIsCollide ) },
			{ "SabotageDoAlert",	INI::parseBool, nullptr, offsetof( SabotageBehaviorModuleData, m_sabotageDoAlert ) },
			{ "SabotageSpecialPowerToTrigger",	INI::parseAsciiString, nullptr, offsetof( SabotageBehaviorModuleData, m_specialPowerTemplateToTrigger ) },

			// Damage and Capture
			{ "SabotageDamage", INI::parseReal, nullptr, offsetof( SabotageBehaviorModuleData, m_sabotageDamage ) },
			{ "SabotageMaxHealth%AsDamage", INI::parsePercentToReal, nullptr, offsetof( SabotageBehaviorModuleData, m_sabotagePercentDamage ) },
			{ "SabotageIsCapture", INI::parseBool, nullptr, offsetof( SabotageBehaviorModuleData, m_sabotageIsCapture ) },
			{ "SabotageCaptureBelowHealth", INI::parseReal, nullptr, offsetof( SabotageBehaviorModuleData, m_sabotageCaptureBelowHealth ) },
			{ "SabotageCaptureBelowHealth%", INI::parseReal, nullptr, offsetof( SabotageBehaviorModuleData, m_sabotageCaptureBelowHealthPercent ) },

			// Steal Cash Amount
			{ "StealCashAmount",	INI::parseUnsignedInt, nullptr, offsetof( SabotageBehaviorModuleData, m_stealCashAmount ) },
			{ "StealCashPercentage",	INI::parsePercentToReal, nullptr, offsetof( SabotageBehaviorModuleData, m_stealCashPercentage ) },

			// Sabotage Power
			{ "SabotagePowerDuration", INI::parseDurationUnsignedInt, nullptr, offsetof( SabotageBehaviorModuleData, m_powerSabotageFrames ) },
			{ "SabotageStealsPower", INI::parseBool, nullptr, offsetof( SabotageBehaviorModuleData, m_stealsPower ) },
			{ "SabotagePowerAmount", INI::parseInt, nullptr, offsetof( SabotageBehaviorModuleData, m_powerAmount ) },
			{ "SabotagePowerPercentage", INI::parsePercentToReal, nullptr, offsetof( SabotageBehaviorModuleData, m_powerPercentage ) },
			{ "SabotageSpecificObjects", INI::parseAsciiStringVector, nullptr, offsetof( SabotageBehaviorModuleData, m_powerSabotageSpecificObjects ) },
			{ "SabotageSpecificKindOf", KindOfMaskType::parseFromINI, nullptr, offsetof( SabotageBehaviorModuleData, m_powerSabotageSpecificKindOf ) },

			// Sabotage General
			{ "SabotageDuration", INI::parseDurationUnsignedInt, nullptr, offsetof( SabotageBehaviorModuleData, m_sabotageFrames ) },
			{ "SabotageDisable", INI::parseBool, nullptr, offsetof( SabotageBehaviorModuleData, m_sabotageDisable ) },
			{ "SabotageDisabledType",	DisabledMaskType::parseSingleBitFromINI, nullptr, offsetof( SabotageBehaviorModuleData, m_sabotageDisabledType ) },
			{ "SabotageDisableContained", INI::parseBool, nullptr, offsetof( SabotageBehaviorModuleData, m_sabotageDisableContained ) },
			{ "DisableAllKindOf", KindOfMaskType::parseFromINI, nullptr, offsetof( SabotageBehaviorModuleData, m_sabotageDisableAllKindOf ) },
			{ "DisableAllForbiddenKindOf", KindOfMaskType::parseFromINI, nullptr, offsetof( SabotageBehaviorModuleData, m_sabotageDisableAllForbiddenKindOf ) },
			{ "TintStatusType",			TintStatusFlags::parseSingleBitFromINI,	nullptr, offsetof( SabotageBehaviorModuleData, m_tintStatus ) },
			{ "CustomTintStatusType",		INI::parseAsciiString,	nullptr, offsetof( SabotageBehaviorModuleData, m_customTintStatus ) },

			// Sabotage Grants
			{ "GrantUpgrades",		INI::parseAsciiStringVector, nullptr, offsetof( SabotageBehaviorModuleData, m_grantUpgradeNames ) },
			{ "SciencesGranted", INI::parseScienceVector, nullptr, offsetof( SabotageBehaviorModuleData, m_sciencesGranted ) },
			{ "GrantCommandInstances", INI::parseNameKeyVector, nullptr, offsetof( SabotageBehaviorModuleData, m_commandInstancesToGrant ) },
			{ "GrantCommandInstancesWithAmount", parseNameKeyVectorWithAmount, nullptr, offsetof( SabotageBehaviorModuleData, m_commandInstancesToGrantWithAmount ) },
			{ "GrantUpgradesIfCollidesWith",		INI::parseAsciiStringVectorAppend, nullptr, offsetof( SabotageBehaviorModuleData, m_grantUpgradeNamesIfCollidesWith ) },
			{ "SciencesGrantedIfCollidesWith", parseObjectScienceVec, nullptr, offsetof( SabotageBehaviorModuleData, m_sciencesGrantedIfCollidesWith ) },
			{ "UpgradesGrantType",		INI::parseIndexList, TheSabotageGrantNames, offsetof( SabotageBehaviorModuleData, m_upgradesGrantType ) },
			{ "SciencesGrantType", INI::parseIndexList, TheSabotageGrantNames, offsetof( SabotageBehaviorModuleData, m_sciencesGrantType ) },

			// Sabotage Production
			{ "CostModifierDuration", INI::parseDurationUnsignedInt, nullptr, offsetof( SabotageBehaviorModuleData, m_sabotageCostModifierFrames ) },
			{ "CostModifierKindOf",		KindOfMaskType::parseFromINI, nullptr, offsetof( SabotageBehaviorModuleData, m_sabotageCostModifierKindOf ) },
			{ "CostModifierPercentage",			INI::parsePercentToReal, nullptr, offsetof( SabotageBehaviorModuleData, m_sabotageCostModifierPercentage ) },
			{ "CostModifierBonusStacksWith",			INI::parseIndexList, TheBonusStackingTypeNames, offsetof( SabotageBehaviorModuleData, m_sabotageCostModifierStackingType) },
			{ "TimeModifierDuration", INI::parseDurationUnsignedInt, nullptr, offsetof( SabotageBehaviorModuleData, m_sabotageTimeModifierFrames ) },
			{ "TimeModifierKindOf",		KindOfMaskType::parseFromINI, nullptr, offsetof( SabotageBehaviorModuleData, m_sabotageTimeModifierKindOf ) },
			{ "TimeModifierPercentage",			INI::parsePercentToReal, nullptr, offsetof( SabotageBehaviorModuleData, m_sabotageTimeModifierPercentage ) },
			{ "TimeModifierBonusStacksWith",			INI::parseIndexList, TheBonusStackingTypeNames, offsetof( SabotageBehaviorModuleData, m_sabotageTimeModifierStackingType) },
			{ "SabotageProductionRevealDuration", parseDuration, nullptr, offsetof( SabotageBehaviorModuleData, m_sabotageProductionViewFrames ) },

			// Sabotage Special Power
			{ "SabotageResetSpecialPowers", INI::parseBool, nullptr, offsetof( SabotageBehaviorModuleData, m_sabotageResetSpecialPowers ) },
			{ "SabotagePauseSpecialPowers", INI::parseBool, nullptr, offsetof( SabotageBehaviorModuleData, m_sabotagePauseSpecialPowers ) },
			{ "SabotageResetAllSameObjectsSpecialPowers", INI::parseBool, nullptr, offsetof( SabotageBehaviorModuleData, m_sabotageResetAllSameObjectsSpecialPowers ) },
			{ "SabotagePauseAllSameObjectsSpecialPowers", INI::parseBool, nullptr, offsetof( SabotageBehaviorModuleData, m_sabotagePauseAllSameObjectsSpecialPowers ) },
			{ "SabotageDisableCommands", INI::parseBool, nullptr, offsetof( SabotageBehaviorModuleData, m_sabotageDisableCommands ) },
			{ "SabotageDisableSpecificCommands", INI::parseAsciiStringVectorAppend, nullptr, offsetof( SabotageBehaviorModuleData, m_commandsToDisable ) },
			{ "UseCommandsUponSabotage", INI::parseBool, nullptr, offsetof( SabotageBehaviorModuleData, m_useCommandsUponSabotage ) },
			{ "UseCommandsNeedsToBeReady", INI::parseBool, nullptr, offsetof( SabotageBehaviorModuleData, m_useCommandsNeedsToBeReady ) },

			// Spy Vision and OCL Update
			{ "SabotageResetSpyVision", INI::parseBool, nullptr, offsetof( SabotageBehaviorModuleData, m_sabotageResetSpyVision ) },
			{ "SabotageDisableSpyVision", INI::parseBool, nullptr, offsetof( SabotageBehaviorModuleData, m_sabotageDisableSpyVision ) },
			{ "SabotageDisableSpyVisionDoesNotResetTimer", INI::parseBool, nullptr, offsetof( SabotageBehaviorModuleData, m_sabotageDisableSpyVisionDoesNotResetTimer ) },
			{ "SabotageResetOCLUpdate", INI::parseBool, nullptr, offsetof( SabotageBehaviorModuleData, m_sabotageResetOCLUpdate ) },
			{ "SabotageDisableOCLUpdate", INI::parseBool, nullptr, offsetof( SabotageBehaviorModuleData, m_sabotageDisableOCLUpdate ) },
			{ "SabotageResetAllSpyVision", INI::parseBool, nullptr, offsetof( SabotageBehaviorModuleData, m_sabotageResetAllSpyVision ) },
			{ "SabotageDisableAllSpyVision", INI::parseBool, nullptr, offsetof( SabotageBehaviorModuleData, m_sabotageDisableAllSpyVision ) },
			{ "SabotageResetAllOCLUpdate", INI::parseBool, nullptr, offsetof( SabotageBehaviorModuleData, m_sabotageResetAllOCLUpdate ) },
			{ "SabotageDisableAllOCLUpdate", INI::parseBool, nullptr, offsetof( SabotageBehaviorModuleData, m_sabotageDisableAllOCLUpdate ) },


			{ nullptr, nullptr, nullptr, 0 }
		};
		p.add( dataFieldParse );
	}

};

//-------------------------------------------------------------------------------------------------
class SabotageBehavior : public CrateCollide
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( SabotageBehavior, "SabotageBehavior" )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( SabotageBehavior, SabotageBehaviorModuleData );

public:

	SabotageBehavior( Thing *thing, const ModuleData* moduleData );
	// virtual destructor prototype provided by memory pool declaration

  //void doSabotageFeedbackFX( const Object *other, SabotageVictimType type = SAB_VICTIM_GENERIC, AsciiString name = AsciiString::TheEmptyString );

	virtual void doSabotage( Object *other, Object *obj );

	virtual Bool canDoSabotageSpecialCheck(const Object *other) const;

	virtual const AsciiString& getSpecialPowerTemplateToTrigger() const { return getSabotageBehaviorModuleData()->m_specialPowerTemplateToTrigger; }

	virtual Bool wouldLikeToCollideWith(const Object* other) const { return isValidToExecute(other); }

protected:

	/// This allows specific vetoes to certain types of crates and their data
	virtual Bool isValidToExecute( const Object *other ) const;

	/// This is the game logic execution function that all real CrateCollides will implement
	virtual Bool executeCrateBehavior( Object *other );

	virtual Bool isSabotageBuildingCrateCollide() const { return TRUE; }

private:
	ObjectUpgradeVec m_upgradeIfCollidesWith;
};

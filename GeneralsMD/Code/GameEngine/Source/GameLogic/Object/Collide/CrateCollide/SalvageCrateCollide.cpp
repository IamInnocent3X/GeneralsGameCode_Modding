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

// FILE: SalvageCrateCollide.cpp ///////////////////////////////////////////////////////////////////////
// Author: Graham Smallwood, March 2002
// Desc:   The Savlage system can give a Weaponset bonus, a level, or money.  Salvagers create them
//					by killing marked units, and only WeaponSalvagers can get the WeaponSet bonus
// Modified : IamInnocent (11/10/2025) - De-hardcoded it from KINDOF_SALVAGER and extended it to
//									     fit desired flags along with other features such as 
//									     upgrades, sciences, weapon bonus, status and OCLs.
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#define SALVAGE_TYPE_NAMES
#define DEFINE_WEAPONBONUSCONDITION_NAMES

#include "GameLogic/Module/SalvageCrateCollide.h"

#include "Common/AudioEventRTS.h"
#include "Common/MiscAudio.h"
#include "Common/KindOf.h"
#include "Common/RandomValue.h"
#include "Common/Player.h"
#include "Common/ThingTemplate.h"
#include "Common/Xfer.h"
//#include "GameClient/ControlBar.h"
#include "GameClient/GameText.h"
#include "GameClient/InGameUI.h"
#include "GameLogic/ExperienceTracker.h"
#include "GameLogic/Object.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/Module/ProductionUpdate.h"


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
SalvageCrateCollideModuleData::SalvageCrateCollideModuleData()
{
	m_kindof = KINDOFMASK_NONE;
	m_kindof.set(KINDOF_SALVAGER);
	m_kindofWeaponSalvager = KINDOFMASK_NONE;
	m_kindofWeaponSalvager.set(KINDOF_WEAPON_SALVAGER);
	m_kindofArmorSalvager = KINDOFMASK_NONE;
	m_kindofArmorSalvager.set(KINDOF_ARMOR_SALVAGER);
	m_weaponSetFlags.clear();
	m_weaponSetFlags.push_back(WEAPONSET_CRATEUPGRADE_ONE);
	m_weaponSetFlags.push_back(WEAPONSET_CRATEUPGRADE_TWO);
	m_armorSetFlags.clear();
	m_armorSetFlags.push_back(ARMORSET_CRATE_UPGRADE_ONE);
	m_armorSetFlags.push_back(ARMORSET_CRATE_UPGRADE_TWO);
	m_weaponModelConditionFlags.clear();
	m_armorModelConditionFlags.clear();
	m_armorModelConditionFlags.push_back(MODELCONDITION_ARMORSET_CRATEUPGRADE_ONE);
	m_armorModelConditionFlags.push_back(MODELCONDITION_ARMORSET_CRATEUPGRADE_TWO);
	m_weaponBonus.clear();
	m_status.clear();
	m_grantUpgradeNames.clear();
	m_sciencesGranted.clear();
	m_ocls.clear();
	m_pickupOrder.clear();
	m_pickupOrder.push_back(SALVAGE_ARMOR);
	m_pickupOrder.push_back(SALVAGE_WEAPON);
	m_pickupOrder.push_back(SALVAGE_LEVEL);
	m_pickupOrder.push_back(SALVAGE_MONEY);
	m_crateGivesMultiBonus = FALSE;
	m_multiPickupFlags = 0;
	m_multiFlagCompletionInfos.clear();
	m_removePreviousWeaponBonus = FALSE;
	m_removePreviousStatus = FALSE;
	m_removePreviousUpgrades = FALSE;
	m_minimumSciencePurchasePoints = 0;
	m_maximumSciencePurchasePoints = 0;
	m_minimumSkillPoints = 0;
	m_maximumSkillPoints = 0;
	m_upgradeChance = 1.0f;
	m_scienceChance = 1.0f;
	m_sciencePurchasePointsChance = 1.0f;
	m_skillPointsChance = 1.0f;
	m_weaponBonusChance = 1.0f;
	m_statusChance = 1.0f;
	m_oclChance = 1.0f;
	m_weaponChance = 1.0f;
	m_levelChance = .25f;
	m_moneyChance = .75f;
	m_minimumMoney = 25;
	m_maximumMoney = 75;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void SalvageCrateCollideModuleData::buildFieldParse(MultiIniFieldParse& p)
{

	CrateCollideModuleData::buildFieldParse(p);

	static const FieldParse dataFieldParse[] =
	{
		{ "RequiredKindOfWeaponSalvager", KindOfMaskType::parseFromINI, NULL, offsetof( SalvageCrateCollideModuleData, m_kindofWeaponSalvager ) },
		{ "ForbiddenKindOfWeaponSalvager", KindOfMaskType::parseFromINI, NULL, offsetof( SalvageCrateCollideModuleData, m_kindofnotWeaponSalvager ) },
		{ "RequiredKindOfArmorSalvager", KindOfMaskType::parseFromINI, NULL, offsetof( SalvageCrateCollideModuleData, m_kindofArmorSalvager ) },
		{ "ForbiddenKindOfArmorSalvager", KindOfMaskType::parseFromINI, NULL, offsetof( SalvageCrateCollideModuleData, m_kindofnotArmorSalvager ) },

		{ "WeaponSetFlags", INI::parseIndexListVector, WeaponSetFlags::getBitNames(),offsetof( SalvageCrateCollideModuleData, m_weaponSetFlags) },
		{ "WeaponModelConditionFlags",	INI::parseIndexListVector,	ModelConditionFlags::getBitNames(),	offsetof( SalvageCrateCollideModuleData, m_weaponModelConditionFlags ) },
		{ "ArmorSetFlags", INI::parseIndexListVector,	ArmorSetFlags::getBitNames(),offsetof( SalvageCrateCollideModuleData, m_armorSetFlags) },
		{ "ArmorModelConditionFlags",	INI::parseIndexListVector,	ModelConditionFlags::getBitNames(),	offsetof( SalvageCrateCollideModuleData, m_armorModelConditionFlags ) },
		{ "GrantsMultipleBonusesOnPickup", 	INI::parseBool, 	NULL, offsetof( SalvageCrateCollideModuleData, m_crateGivesMultiBonus) },

		{ "PickupOrder", 	INI::parseIndexListVector, 	TheSalvagePickupNames, offsetof( SalvageCrateCollideModuleData, m_pickupOrder) },
		{ "MultiPickupFlags", 	INI::parseBitString32, 	TheSalvagePickupNames, offsetof( SalvageCrateCollideModuleData, m_multiPickupFlags) },
		{ "MultiPickupAmountModifierAfterCompletion", 	parseAmountModifier, 	NULL, offsetof( SalvageCrateCollideModuleData, m_multiFlagCompletionInfos) },
		{ "MultiPickupChanceModifierAfterCompletion", 	parseChanceModifier, 	NULL, offsetof( SalvageCrateCollideModuleData, m_multiFlagCompletionInfos) },

		{ "GrantWeaponBonus",		parseWeaponBonus, NULL, offsetof( SalvageCrateCollideModuleData, m_weaponBonus ) },
		{ "GrantStatus",			parseStatus, NULL, offsetof( SalvageCrateCollideModuleData, m_status ) },
		{ "GrantUpgrades",		INI::parseAsciiStringVector, NULL, offsetof( SalvageCrateCollideModuleData, m_grantUpgradeNames ) },
		{ "SciencesGranted", INI::parseScienceVector, NULL, offsetof( SalvageCrateCollideModuleData, m_sciencesGranted ) },
		{ "OCLs",	            INI::parseObjectCreationListVector,	NULL,	offsetof( SalvageCrateCollideModuleData, m_ocls) },

		{ "RemovePreviousWeaponBonus",	INI::parseBool,	NULL, offsetof( SalvageCrateCollideModuleData, m_removePreviousWeaponBonus ) },
		{ "RemovePreviousStatus",		INI::parseBool,	NULL, offsetof( SalvageCrateCollideModuleData, m_removePreviousStatus ) },
		{ "RemovePreviousUpgrade",		INI::parseBool,	NULL, offsetof( SalvageCrateCollideModuleData, m_removePreviousUpgrades ) },

		{ "MinimumSciencePurchasePointsGranted", INI::parseInt, NULL, offsetof( SalvageCrateCollideModuleData, m_minimumSciencePurchasePoints ) },
		{ "MaximumSciencePurchasePointsGranted", INI::parseInt, NULL, offsetof( SalvageCrateCollideModuleData, m_maximumSciencePurchasePoints ) },
		{ "MinimumSkillPointsGranted", INI::parseInt, NULL, offsetof( SalvageCrateCollideModuleData, m_minimumSkillPoints ) },
		{ "MaximumSkillPointsGranted", INI::parseInt, NULL, offsetof( SalvageCrateCollideModuleData, m_maximumSkillPoints ) },

		{ "WeaponBonusChance",	INI::parsePercentToReal,	NULL, offsetof( SalvageCrateCollideModuleData, m_weaponBonusChance ) },
		{ "StatusChance",	INI::parsePercentToReal,	NULL, offsetof( SalvageCrateCollideModuleData, m_statusChance ) },
		{ "UpgradeChance",	INI::parsePercentToReal,	NULL, offsetof( SalvageCrateCollideModuleData, m_upgradeChance ) },
		{ "ScienceChance",	INI::parsePercentToReal,	NULL, offsetof( SalvageCrateCollideModuleData, m_scienceChance ) },
		{ "SciencePurchasePointsChance",	INI::parsePercentToReal,	NULL, offsetof( SalvageCrateCollideModuleData, m_sciencePurchasePointsChance ) },
		{ "SkillPointsChance",	INI::parsePercentToReal,	NULL, offsetof( SalvageCrateCollideModuleData, m_skillPointsChance ) },
		{ "OCLChance",	INI::parsePercentToReal,	NULL, offsetof( SalvageCrateCollideModuleData, m_oclChance ) },

		{ "WeaponChance",	INI::parsePercentToReal,	NULL, offsetof( SalvageCrateCollideModuleData, m_weaponChance ) },
		{ "LevelChance",	INI::parsePercentToReal,	NULL, offsetof( SalvageCrateCollideModuleData, m_levelChance ) },
		{ "MoneyChance",	INI::parsePercentToReal,	NULL, offsetof( SalvageCrateCollideModuleData, m_moneyChance ) },
		{ "MinMoney",			INI::parseInt,						NULL, offsetof( SalvageCrateCollideModuleData, m_minimumMoney ) },
		{ "MaxMoney",			INI::parseInt,						NULL, offsetof( SalvageCrateCollideModuleData, m_maximumMoney ) },
		{ 0, 0, 0, 0 }
	};
    p.add(dataFieldParse);

}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void SalvageCrateCollideModuleData::parseAmountModifier( INI* ini, void *instance, void *store, const void* /*userData*/ )
{
	std::vector<CompletionInfo> *v = (std::vector<CompletionInfo>*)store;
	//v->clear();
	ConstCharPtrArray flagList = (ConstCharPtrArray)TheSalvagePickupNames;
		if( flagList == NULL || flagList[ 0 ] == NULL)
	{
		DEBUG_ASSERTCRASH( flagList, ("INTERNAL ERROR! parseBitString32: No flag list provided!") );
		throw INI_INVALID_NAME_LIST;
	}

	Int completion;
	BonusInfo bonus;
	bonus.Amount = 0;
	bonus.MinAmount = 0;
	bonus.MaxAmount = 0;

	Bool foundNormal = false;
	Bool foundCompletion = false;
	Bool foundAddOrSub = false;
	Bool foundMin = false;
	Bool foundMax = false;
	Bool foundPoints = false;

	//DEBUG_LOG(("Parse Amount Modifier"));

	// loop through all tokens
	for (const char *token = ini->getNextTokenOrNull(); token != NULL; token = ini->getNextTokenOrNull())
	{
		if (stricmp(token, "MIN") == 0)
		{
			//DEBUG_LOG(("Parse Min"));
			if (!foundPoints || foundMin || (foundMax && !foundAddOrSub))
			{
				//DEBUG_LOG(("Parse Min Failed"));
				DEBUG_CRASH(("min is required to declared by points"));
				throw INI_INVALID_NAME_LIST;
			}
			foundMin = true;
			foundMax = false;
			foundAddOrSub = false;
		}
		else if (stricmp(token, "MAX") == 0)
		{
			//DEBUG_LOG(("Parse Max"));
			if (!foundPoints || foundMax || (foundMin && !foundAddOrSub))
			{
				//DEBUG_LOG(("Parse Max Failed"));
				DEBUG_CRASH(("max is required to declared by points"));
				throw INI_INVALID_NAME_LIST;
			}
			foundMin = false;
			foundMax = true;
			foundAddOrSub = false;
		}
		else if(token[0] == '+')
		{
			//DEBUG_LOG(("Parse Add"));
			if (foundAddOrSub || !foundNormal || !foundCompletion)
			{
				//DEBUG_LOG(("Parse Add Failed"));
				DEBUG_CRASH(("you may not mix normal, completion, min, max, and +- ops in bitstring lists"));
				throw INI_INVALID_NAME_LIST;
			}
			Int amount = INI::scanInt(token+1);
			if(foundPoints)
			{
				//DEBUG_LOG(("Parse Add Points"));
				if (!foundMin && !foundMax)
				{
					//DEBUG_LOG(("Parse Add Points Failed"));
					DEBUG_CRASH(("you need either Min or Max for Points Amount"));
					throw INI_INVALID_NAME_LIST;
				}
				if(foundMin)
					bonus.MinAmount = amount;
				else if(foundMax)
					bonus.MaxAmount = amount;

			}
			else
				bonus.Amount = amount;

			//foundCompletion = false;
			if(!foundPoints || foundMax)
			{
				foundNormal = false;
				foundPoints = false;
				foundMax = false;
			}

			foundAddOrSub = true;
		}
		else if (token[0] == '-')
		{
			//DEBUG_LOG(("Parse Sub"));
			if (foundAddOrSub || !foundNormal || !foundCompletion)
			{
				//DEBUG_LOG(("Parse Sub Failed"));
				DEBUG_CRASH(("you may not mix normal, completion, min, max, and +- ops in bitstring lists"));
				throw INI_INVALID_NAME_LIST;
			}
			Int amount = INI::scanInt(token+1);
			if(foundPoints)
			{
				//DEBUG_LOG(("Parse Sub Points"));
				if (!foundMin && !foundMax)
				{
					//DEBUG_LOG(("Parse Sub Points Failed"));
					DEBUG_CRASH(("you need either Min or Max for Points Amount"));
					throw INI_INVALID_NAME_LIST;
				}
				if(foundMin)
					bonus.MinAmount = -amount;
				else if(foundMax)
					bonus.MaxAmount = -amount;
			}
			else
				bonus.Amount = -amount;

			//foundCompletion = false;
			if(!foundPoints || foundMax)
			{
				foundNormal = false;
				foundPoints = false;
				foundMax = false;
			}

			foundAddOrSub = true;
		}
		else if(stricmp(token, "COMPLETION") == 0)
		{
			//DEBUG_LOG(("Parse Completion"));
			if (foundCompletion)
			{
				//DEBUG_LOG(("Parse Completion Failed, Duplicate Completion Found"));
				DEBUG_CRASH(("you may not mix normal, completion, min, max, and +- ops in bitstring lists"));
				throw INI_INVALID_NAME_LIST;
			}

			Int bitIndex = INI::scanIndexList(ini->getNextToken(), flagList);	// this throws if the token is not found
			bitIndex = (1 << bitIndex);

			if(bitIndex == SALVAGE_MONEY)
			{
				//DEBUG_LOG(("Parse Completion Money Supposedly Failed from supposedly Parsing Amount"));
				DEBUG_CRASH(("you may not use Money for Completion"));
				throw INI_INVALID_NAME_LIST;
			}

			if(bitIndex == SALVAGE_OCL)
			{
				//DEBUG_LOG(("Parse Completion OCL Supposedly Failed from supposedly Parsing Amount"));
				DEBUG_CRASH(("you may not use OCLs for Completion"));
				throw INI_INVALID_NAME_LIST;
			}

			completion = bitIndex;
			foundCompletion = true;
		}
		else
		{
			//DEBUG_LOG(("Parse Normal"));
			if (foundNormal || !foundCompletion)
			{
				//DEBUG_LOG(("Parse Normal Failed"));
				DEBUG_CRASH(("you may not mix normal, completion, min, max, and +- ops in bitstring lists"));
				throw INI_INVALID_NAME_LIST;
			}

			//if (!foundNormal)
			//	*bits = 0;

			if(stricmp(token, "BONUS") == 0)
			{
				//DEBUG_LOG(("Parse Normal found Bonus"));
				token = ini->getNextToken();
			}

			Int bitIndex = (1 << INI::scanIndexList(token, flagList));	// this throws if the token is not found
			bonus.Bonus = bitIndex;
			foundNormal = true;

			if(bitIndex == SALVAGE_MONEY || bitIndex == SALVAGE_SCIENCE_POINTS || bitIndex == SALVAGE_SKILL_POINTS )
			{
				//DEBUG_LOG(("Parse Normal: Points"));
				foundPoints = true;
			}
			else
			{
				//DEBUG_LOG(("Parse Normal: %d", bitIndex));
			}
		}

		if(foundAddOrSub && (!foundPoints || foundMax))
		{
			Bool hasCompletion = false;
			for(std::vector<CompletionInfo>::iterator it = v->begin(); it != v->end(); ++it)
			{
				if(it->Completed == completion)
				{
					Bool hasBonus = false;
					for(int j = 0; j < it->Bonuses.size(); j++)
					{
						if(it->Bonuses[j].Bonus == bonus.Bonus)
						{
							it->Bonuses[j].Amount = bonus.Amount;
							it->Bonuses[j].MinAmount = bonus.MinAmount;
							it->Bonuses[j].MaxAmount = bonus.MaxAmount;
							hasBonus = true;
							//DEBUG_LOG(("Parse Amount Already Has Completion Bonus Modifier. Completion: %d, Bonus: %d", completion, bonus.Bonus));
							break;
						}
					}
					if(!hasBonus)
					{
						//DEBUG_LOG(("Parse Amount Already Has Completion Modifier. Completion: %d", completion));
						it->Bonuses.push_back(bonus);
					}

					hasCompletion = true;
				}
			}
			if(!hasCompletion)
			{
				//DEBUG_LOG(("Parse Amount New Completion Modifier. Completion: %d", completion));
				CompletionInfo info;
				info.Completed = completion;
				info.Bonuses.clear();
				info.Bonuses.push_back(bonus);
				v->push_back(info);
			}

			// remove this for next parsing
			foundAddOrSub = false;
		}

	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void SalvageCrateCollideModuleData::parseChanceModifier( INI* ini, void *instance, void *store, const void* /*userData*/ )
{
	std::vector<CompletionInfo> *v = (std::vector<CompletionInfo>*)store;
	//v->clear();
	ConstCharPtrArray flagList = (ConstCharPtrArray)TheSalvagePickupNames;
		if( flagList == NULL || flagList[ 0 ] == NULL)
	{
		DEBUG_ASSERTCRASH( flagList, ("INTERNAL ERROR! parseBitString32: No flag list provided!") );
		throw INI_INVALID_NAME_LIST;
	}

	Int completion;
	BonusInfo bonus;
	bonus.Chance = 0.0f;

	Bool foundNormal = false;
	Bool foundCompletion = false;
	Bool foundAddOrSub = false;

	//DEBUG_LOG(("Parse Chance Modifier"));

	// loop through all tokens
	for (const char *token = ini->getNextTokenOrNull(); token != NULL; token = ini->getNextTokenOrNull())
	{
		if(token[0] == '+')
		{
			//DEBUG_LOG(("Parse Add on Chance"));
			if (foundAddOrSub || !foundNormal || !foundCompletion)
			{
				//DEBUG_LOG(("Parse Add on Chance Failed"));
				DEBUG_CRASH(("you may not mix normal, completion, min, max, and +- ops in bitstring lists"));
				throw INI_INVALID_NAME_LIST;
			}
			Real chance = INI::scanPercentToReal(token+1);
			bonus.Chance = chance;
			//foundCompletion = false;
			foundNormal = false;
			foundAddOrSub = true;
		}
		else if (token[0] == '-')
		{
			//DEBUG_LOG(("Parse Sub on Chance"));
			if (foundAddOrSub || !foundNormal || !foundCompletion)
			{
				//DEBUG_LOG(("Parse Sub on Chance Failed"));
				DEBUG_CRASH(("you may not mix normal, completion, min, max, and +- ops in bitstring lists"));
				throw INI_INVALID_NAME_LIST;
			}
			Real chance = INI::scanPercentToReal(token+1);
			bonus.Chance = -chance;
			//foundCompletion = false;
			foundNormal = false;
			foundAddOrSub = true;
		}
		else if(stricmp(token, "COMPLETION") == 0)
		{
			//DEBUG_LOG(("Parse Completion"));
			if (foundCompletion)
			{
				//DEBUG_LOG(("Parse Completion Failed, Duplicate Completion Found"));
				DEBUG_CRASH(("you may not mix normal, completion, min, max, and +- ops in bitstring lists"));
				throw INI_INVALID_NAME_LIST;
			}

			Int bitIndex = INI::scanIndexList(ini->getNextToken(), flagList);	// this throws if the token is not found
			bitIndex = (1 << bitIndex);

			if(bitIndex == SALVAGE_MONEY)
			{
				//DEBUG_LOG(("Parse Completion Money Supposedly Failed from supposedly Parsing Chance"));
				DEBUG_CRASH(("you may not use Money for Completion"));
				throw INI_INVALID_NAME_LIST;
			}

			if(bitIndex == SALVAGE_OCL)
			{
				//DEBUG_LOG(("Parse Completion OCL Supposedly Failed from supposedly Parsing Chance"));
				DEBUG_CRASH(("you may not use OCLs for Completion"));
				throw INI_INVALID_NAME_LIST;
			}

			completion = bitIndex;
			foundCompletion = true;
			foundAddOrSub = false;
		}
		else
		{
			//DEBUG_LOG(("Parse Normal on Chance"));
			if (foundNormal || !foundCompletion)
			{
				//DEBUG_LOG(("Parse Normal on Chance Failed"));
				DEBUG_CRASH(("you may not mix normal, completion, and +- ops in bitstring lists"));
				throw INI_INVALID_NAME_LIST;
			}

			//if (!foundNormal)
			//	*bits = 0;

			if(stricmp(token, "BONUS") == 0)
				token = ini->getNextToken();

			Int bitIndex = (1 << INI::scanIndexList(token, flagList));	// this throws if the token is not found

			if(bitIndex == SALVAGE_ARMOR || bitIndex == SALVAGE_MONEY)
			{
				//DEBUG_LOG(("Parse Chance Supposedly Failed from Parsing Armor Or Money"));
				DEBUG_CRASH(("you may not parse salvage types without chances with this"));
				throw INI_INVALID_NAME_LIST;
			}

			bonus.Bonus = bitIndex;
			foundNormal = true;
		}

		if(foundAddOrSub)
		{
			Bool hasCompletion = false;
			for(std::vector<CompletionInfo>::iterator it = v->begin(); it != v->end(); ++it)
			{
				if(it->Completed == completion)
				{
					Bool hasBonus = false;
					for(int j = 0; j < it->Bonuses.size(); j++)
					{
						if(it->Bonuses[j].Bonus == bonus.Bonus)
						{
							it->Bonuses[j].Chance = bonus.Chance;
							hasBonus = true;
							//DEBUG_LOG(("Parse Chance Already Has Completion Bonus Modifier. Completion: %d, Bonus: %d", completion, bonus.Bonus));
							break;
						}
					}
					if(!hasBonus)
					{
						//DEBUG_LOG(("Parse Chance Already Has Completion Modifier. Completion: %d", completion));
						it->Bonuses.push_back(bonus);
					}

					hasCompletion = true;
				}
			}
			if(!hasCompletion)
			{
				//DEBUG_LOG(("Parse Chance New Completion Modifier. Completion: %d", completion));
				CompletionInfo info;
				info.Completed = completion;
				info.Bonuses.clear();
				info.Bonuses.push_back(bonus);
				v->push_back(info);
			}
			// remove this for next parsing
			foundAddOrSub = false;
		}

	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void SalvageCrateCollideModuleData::parseWeaponBonus( INI* ini, void *instance, void *store, const void* /*userData*/ )
{
	std::vector<WeaponBonusTypes> *v = (std::vector<WeaponBonusTypes>*)store;
	for (const char *token = ini->getNextTokenOrNull(); token != NULL; token = ini->getNextTokenOrNull())
	{
		WeaponBonusTypes bonus;

		Int count = 0;
		for(ConstCharPtrArray name = TheWeaponBonusNames; *name; name++, count++ )
		{
			if( stricmp( *name, token ) == 0 )
			{
				bonus.bonusType = (WeaponBonusConditionType)count;
				break;
			}
		}
		if (bonus.bonusType == -1)
		{
			bonus.customType.format(token);
		}
		v->push_back(bonus);
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void SalvageCrateCollideModuleData::parseStatus( INI* ini, void *instance, void *store, const void* /*userData*/ )
{
	std::vector<StatusTypes> *v = (std::vector<StatusTypes>*)store;
	for (const char *token = ini->getNextTokenOrNull(); token != NULL; token = ini->getNextTokenOrNull())
	{
		StatusTypes status;

		ObjectStatusType ost = (ObjectStatusType)ObjectStatusMaskType::getSingleBitFromName(token);
		if(ost >= 0)
		{
			status.statusType = ost;
		}
		else
		{
			status.customType.format(token);
		}
		v->push_back(status);
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
SalvageCrateCollide::SalvageCrateCollide( Thing *thing, const ModuleData* moduleData ) : CrateCollide( thing, moduleData )
{

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
SalvageCrateCollide::~SalvageCrateCollide( void )
{

}

//-------------------------------------------------------------------------------------------------
Bool SalvageCrateCollide::isValidToExecute( const Object *other ) const
{
	if( ! CrateCollide::isValidToExecute( other ) )
		return FALSE;

	// Only salvage units can pick up a Salvage crate
	/// IamInnocent 3/11/2025 - De-hardcoded
	//if( ! other->getTemplate()->isKindOf( KINDOF_SALVAGER ) )
	//	return FALSE;

	return TRUE;
}

//-------------------------------------------------------------------------------------------------
Bool SalvageCrateCollide::executeCrateBehavior( Object *other )
{
	const SalvageCrateCollideModuleData *md = getSalvageCrateCollideModuleData();
	Bool playCrateSound = false;
	Bool playMoneySound = false;
	if(md->m_crateGivesMultiBonus)
	{
		if( (md->m_multiPickupFlags & SALVAGE_ARMOR) && eligibleForArmorSet(other) )// No percent chance on this one, if you can get it, you get it.
		{
			doArmorSet(other);

			playCrateSound = true;
		}
		if( (md->m_multiPickupFlags & SALVAGE_WEAPON) && eligibleForWeaponSet( other ) && testWeaponChanceMulti(other) )
		{
			doWeaponSet( other );

			//Play the salvage installation crate pickup sound.
			playCrateSound = true;

			//Play the unit voice acknowledgement for upgrading weapons.
			//Already handled by the "move order"
			//const AudioEventRTS *soundToPlayPtr = other->getTemplate()->getPerUnitSound( "VoiceSalvage" );
			//soundToPlay = *soundToPlayPtr;
			//soundToPlay.setObjectID( other->getID() );
			//TheAudio->addAudioEvent( &soundToPlay );
		}
		if( (md->m_multiPickupFlags & SALVAGE_LEVEL) && eligibleForLevel( other ) && testLevelChanceMulti(other) )
		{
			doLevelGain( other );

			//Sound will play in
			//soundToPlay = TheAudio->getMiscAudio()->m_unitPromoted;
		}
		if( (md->m_multiPickupFlags & SALVAGE_MONEY) && doMoney( other ) )
		{
			playMoneySound = true;
		}
		if( (md->m_multiPickupFlags & SALVAGE_UPGRADES) && eligibleForUpgrades( other ) && testUpgradeChanceMulti(other) )
		{
			grantUpgrades( other );

			playCrateSound = true;
		}
		if( (md->m_multiPickupFlags & SALVAGE_SCIENCE) && eligibleForScience( other ) && testScienceChanceMulti(other) )
		{
			grantSciences( other );

			playCrateSound = true;
		}
		if( (md->m_multiPickupFlags & SALVAGE_SCIENCE_POINTS) && eligibleForSciencePurchasePoints( other ) && testSciencePurchasePointsChanceMulti(other) && grantSciencePurchasePoints( other ) )
		{
			playCrateSound = true;
		}
		if( (md->m_multiPickupFlags & SALVAGE_SKILL_POINTS) && eligibleForSkillPoints( other ) && testSkillPointsChanceMulti(other) && grantSkillPoints( other ) )
		{
			playCrateSound = true;
		}
		if( (md->m_multiPickupFlags & SALVAGE_WEAPON_BONUS) && eligibleForWeaponBonus( other ) && testWeaponBonusChanceMulti(other) )
		{
			doWeaponBonus( other );

			playCrateSound = true;
		}
		if( (md->m_multiPickupFlags & SALVAGE_STATUS) && eligibleForStatus( other ) && testStatusChanceMulti(other) )
		{
			doStatus( other );

			playCrateSound = true;
		}
		if( (md->m_multiPickupFlags & SALVAGE_OCL) && testOCLChanceMulti(other) && doOCL( other ) )
		{
			playCrateSound = true;
		}
	}
	else
	{
		Bool done = false;
		for(int i = 0; i < md->m_pickupOrder.size() ; i++)
		{
			switch(md->m_pickupOrder[i])
			{
				case SALVAGE_ARMOR:
				{
					if( eligibleForArmorSet(other) )// No percent chance on this one, if you can get it, you get it.
					{
						doArmorSet(other);

						//Play the salvage installation crate pickup sound.
						done = true;
						playCrateSound = true;
					}
					break;
				}
				case SALVAGE_WEAPON:
				{
					if( eligibleForWeaponSet( other ) && testWeaponChance() )
					{
						doWeaponSet(other);

						//Play the salvage installation crate pickup sound.
						done = true;
						playCrateSound = true;
					}
					break;
				}
				case SALVAGE_LEVEL:
				{
					if( eligibleForLevel( other ) && testLevelChance() )
					{
						doLevelGain( other );

						done = true;
					}
					break;
				}
				case SALVAGE_MONEY:
				{
					if( doMoney( other ) )
					{
						//Play the salvage installation crate pickup sound.
						done = true;
						playMoneySound = true;
					}
					break;
				}
				case SALVAGE_UPGRADES:
				{
					if( eligibleForUpgrades( other ) && testUpgradeChance() )
					{
						grantUpgrades( other );

						//Play the salvage installation crate pickup sound.
						done = true;
						playCrateSound = true;
					}
					break;
				}
				case SALVAGE_SCIENCE:
				{
					if( eligibleForScience( other ) && testScienceChance() )
					{
						grantSciences( other );

						//Play the salvage installation crate pickup sound.
						done = true;
						playCrateSound = true;
					}
					break;
				}
				case SALVAGE_SCIENCE_POINTS:
				{
					if( eligibleForSciencePurchasePoints( other ) && testSciencePurchasePointsChance() && grantSciencePurchasePoints( other ) )
					{
						//Play the salvage installation crate pickup sound.
						done = true;
						playCrateSound = true;
					}
					break;
				}
				case SALVAGE_SKILL_POINTS:
				{
					if( eligibleForSkillPoints( other ) && testSkillPointsChance() && grantSkillPoints( other ) )
					{
						//Play the salvage installation crate pickup sound.
						done = true;
						playCrateSound = true;
					}
					break;
				}
				case SALVAGE_STATUS:
				{
					if( eligibleForStatus( other ) && testStatusChance() )
					{
						doStatus( other );

						//Play the salvage installation crate pickup sound.
						done = true;
						playCrateSound = true;
					}
					break;
				}
				case SALVAGE_WEAPON_BONUS:
				{
					if( eligibleForWeaponBonus( other ) && testWeaponBonusChance() )
					{
						doWeaponBonus( other );

						//Play the salvage installation crate pickup sound.
						done = true;
						playCrateSound = true;
					}
					break;
				}
				case SALVAGE_OCL:
				{
					if( testOCLChance() && doOCL(other) )
					{
						//Play the salvage installation crate pickup sound.
						done = true;
						playCrateSound = true;
					}
					break;
				}
			}

			if(done)
				break;

		}
	}
	if(playCrateSound)
	{
		//Play the salvage installation crate pickup sound.
		AudioEventRTS soundToPlay = TheAudio->getMiscAudio()->m_crateSalvage;
		soundToPlay.setObjectID( other->getID() );
		TheAudio->addAudioEvent( &soundToPlay );
	}
	if(playMoneySound)
	{
		AudioEventRTS moneySoundToPlay = TheAudio->getMiscAudio()->m_crateMoney;
		moneySoundToPlay.setObjectID( other->getID() );
		TheAudio->addAudioEvent(&moneySoundToPlay);
	}
	/*if( eligibleForArmorSet(other) )// No percent chance on this one, if you can get it, you get it.
	{
		doArmorSet(other);

		//Play the salvage installation crate pickup sound.
		AudioEventRTS soundToPlay = TheAudio->getMiscAudio()->m_crateSalvage;
		soundToPlay.setObjectID( other->getID() );
		TheAudio->addAudioEvent( &soundToPlay );
	}
	else if( eligibleForWeaponSet( other ) && testWeaponChance() )
	{
		doWeaponSet( other );

		//Play the salvage installation crate pickup sound.
		AudioEventRTS soundToPlay = TheAudio->getMiscAudio()->m_crateSalvage;
		soundToPlay.setObjectID( other->getID() );
		TheAudio->addAudioEvent( &soundToPlay );

		//Play the unit voice acknowledgement for upgrading weapons.
		//Already handled by the "move order"
		//const AudioEventRTS *soundToPlayPtr = other->getTemplate()->getPerUnitSound( "VoiceSalvage" );
		//soundToPlay = *soundToPlayPtr;
		//soundToPlay.setObjectID( other->getID() );
		//TheAudio->addAudioEvent( &soundToPlay );
	}
	else if( eligibleForLevel( other ) && testLevelChance() )
	{
		doLevelGain( other );

		//Sound will play in
		//soundToPlay = TheAudio->getMiscAudio()->m_unitPromoted;
	}
	else if( doMoney( other ) )// just assume the testMoneyChance
	{
		//doMoney( other );
		AudioEventRTS soundToPlay = TheAudio->getMiscAudio()->m_crateMoney;
		soundToPlay.setObjectID( other->getID() );
		TheAudio->addAudioEvent(&soundToPlay);
	}
	*/

	other->getControllingPlayer()->getAcademyStats()->recordSalvageCollected();

	CrateCollide::executeCrateBehavior(other);

	return TRUE;
}

// ------------------------------------------------------------------------------------------------
Bool SalvageCrateCollide::eligibleForWeaponSet( Object *other )
{
	if( other == nullptr )
		return FALSE;

	const SalvageCrateCollideModuleData *md = getSalvageCrateCollideModuleData();

	// A kindof marks eligibility, and you must not be fully upgraded
	if ( !other->isKindOfMulti(md->m_kindofWeaponSalvager, md->m_kindofnotWeaponSalvager) )
		return FALSE;
	if( md->m_weaponSetFlags.size() == 0 || other->testWeaponSetFlag(md->m_weaponSetFlags[md->m_weaponSetFlags.size()-1]) )
		return FALSE;
	//if( !other->isKindOf(KINDOF_WEAPON_SALVAGER) )
	//	return FALSE;
	//if( other->testWeaponSetFlag(WEAPONSET_CRATEUPGRADE_TWO) )
	//	return FALSE;

	return TRUE;
}

// ------------------------------------------------------------------------------------------------
Bool SalvageCrateCollide::eligibleForArmorSet( Object *other )
{
	if( other == nullptr )
		return FALSE;

	const SalvageCrateCollideModuleData *md = getSalvageCrateCollideModuleData();

	// must match our kindof flags (if any)
	if ( !other->isKindOfMulti(md->m_kindofArmorSalvager, md->m_kindofnotArmorSalvager) )
		return FALSE;
	if( md->m_armorSetFlags.size() == 0 || other->testArmorSetFlag(md->m_armorSetFlags[md->m_armorSetFlags.size()-1]) )
		return FALSE;
	// A kindof marks eligibility, and you must not be fully upgraded
	//if( !other->isKindOf(KINDOF_ARMOR_SALVAGER) )
	//	return FALSE;
	//if( other->testArmorSetFlag(ARMORSET_CRATE_UPGRADE_TWO) )
	//	return FALSE;

	return TRUE;
}

// ------------------------------------------------------------------------------------------------
Bool SalvageCrateCollide::eligibleForLevel( Object *other )
{
	if( other == nullptr )
		return FALSE;

	// Sorry, you are max level
	if( other->getExperienceTracker()->getVeterancyLevel() == LEVEL_HEROIC )
		return FALSE;

	// Sorry, you can't gain levels
	if( !other->getExperienceTracker()->isTrainable() )
		return FALSE;

	const SalvageCrateCollideModuleData *md = getSalvageCrateCollideModuleData();
	Int amount = 1;
	if(md->m_multiPickupFlags)
	{
		for(int i = 0; i < md->m_multiFlagCompletionInfos.size(); i++)
		{
			if( multiFlagEligibleConditions( other, md->m_multiFlagCompletionInfos[i].Completed, SALVAGE_LEVEL) )
			{
				for(int j = 0; j < md->m_multiFlagCompletionInfos[i].Bonuses.size(); j++)
				{
					if(md->m_multiFlagCompletionInfos[i].Bonuses[j].Bonus == SALVAGE_LEVEL)
					{
						amount += md->m_multiFlagCompletionInfos[i].Bonuses[j].Amount;
						break;
					}
				}
			}
		}
	}

	if(amount <= 0)
		return FALSE;

	return TRUE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
Bool SalvageCrateCollide::eligibleForUpgrades( Object *other )
{
	// Sorry, no upgrades
	if( !testUpgrade(other, false) )
		return FALSE;

	return TRUE;

}

// ------------------------------------------------------------------------------------------------
Bool SalvageCrateCollide::testUpgrade( Object *other, Bool testMax ) const
{
	// Sanity
	if( other == NULL )
		return FALSE;

	const SalvageCrateCollideModuleData *md = getSalvageCrateCollideModuleData();
	if(md->m_grantUpgradeNames.empty())
		return FALSE;

	const UpgradeTemplate* theTemplate = TheUpgradeCenter->findUpgrade( md->m_grantUpgradeNames[md->m_grantUpgradeNames.size()-1] );
	if( !theTemplate )
	{
		DEBUG_CRASH(("An upgrade module references '%s', which is not an Upgrade", md->m_grantUpgradeNames[md->m_grantUpgradeNames.size()-1].str()));
		throw INI_INVALID_DATA;
	}
	if( !theTemplate )
	{
		DEBUG_ASSERTCRASH( 0, ("SalvageCrateCollide for %s can't find upgrade template %s.", obj->getTemplate()->getName(), md->m_grantUpgradeNames[md->m_grantUpgradeNames.size()-1].str() ) );
		return FALSE;
	}

	Bool hasUpgrade = FALSE;
	if( theTemplate->getUpgradeType() == UPGRADE_TYPE_PLAYER )
	{
		Player *player = other->getControllingPlayer();
		if( player && player->hasUpgradeComplete( theTemplate ) )
		{
			hasUpgrade = TRUE;
		}
	}
	else
	{
		if( other->hasUpgrade( theTemplate ) )
		{
			hasUpgrade = TRUE;
		}
	}

	if(hasUpgrade)
	{
		if(testMax)
			return TRUE;
		else
			return FALSE;
	}

	if(testMax)
		return FALSE;

	return TRUE;
}

// ------------------------------------------------------------------------------------------------
Bool SalvageCrateCollide::eligibleForScience( Object *other )
{
	if( other == NULL || other->getControllingPlayer() == NULL )
		return FALSE;

	// Sorry, you can't inherit sciences
	if( other->isNeutralControlled() )
		return FALSE;

	const SalvageCrateCollideModuleData *md = getSalvageCrateCollideModuleData();

	// Sorry, max level
	if( md->m_sciencesGranted.size() == 0 || other->getControllingPlayer()->hasScience(md->m_sciencesGranted[md->m_sciencesGranted.size()-1]) )
		return FALSE;

	return TRUE;
}

// ------------------------------------------------------------------------------------------------
Bool SalvageCrateCollide::eligibleForSciencePurchasePoints( Object *other )
{
	if( !testSciencePurchasePoints(other, false) )
		return FALSE;

	return TRUE;
}

// ------------------------------------------------------------------------------------------------
Bool SalvageCrateCollide::testSciencePurchasePoints( Object *other, Bool testMax ) const
{
	// Sanity
	if( other == NULL )
		return FALSE;

	// Sorry, you can't inherit sciences
	if( other->getControllingPlayer() == NULL )
		return FALSE;

	if( other->isNeutralControlled() )
		return FALSE;

	Player *player = other->getControllingPlayer();

	Int currPoints = player->getSciencePurchasePoints();
	Int totalPoints = 0;
	//Int sciencePointsAvailableToPurchase = TheControlBar->getRemainingSciencePointsAvailableToPurchase(player);

	ScienceVec purchasable, potential;
	TheScienceStore->getPurchasableSciences(player, purchasable, potential);

	for( int i = 0; i < potential.size(); i++ )
	{
		ScienceType	st = potential[ i ];

		if( !player->isScienceDisabled( st ) &&
			!player->isScienceHidden( st )
		  )
		{
			totalPoints += TheScienceStore->getSciencePurchaseCost(st);
		}

	}

	// currPoints can return 0, which can also be the same as the remaining Science Points required to max out Sciences
	if( currPoints >= totalPoints )
	{
		if(testMax)
			return TRUE;
		else
			return FALSE;
	}

	if(testMax)
		return FALSE;

	return TRUE;
}

// ------------------------------------------------------------------------------------------------
Bool SalvageCrateCollide::eligibleForSkillPoints( Object *other )
{
	if( other == NULL || !testSkillPoints(other, false) )
		return FALSE;

	return TRUE;
}

// ------------------------------------------------------------------------------------------------
Bool SalvageCrateCollide::testSkillPoints( Object *other, Bool testMax ) const
{
	// Sorry, you can't inherit sciences
	if( other->getControllingPlayer() == NULL )
		return FALSE;

	if( other->isNeutralControlled() )
		return FALSE;

	// Sorry, you are max level
	if( other->getControllingPlayer()->getRankLevel() == TheGameLogic->getRankLevelLimit() )
	{
		if(testMax)
			return TRUE;
		else
			return FALSE;
	}

	if(testMax)
		return FALSE;

	return TRUE;
}

// ------------------------------------------------------------------------------------------------
Bool SalvageCrateCollide::eligibleForWeaponBonus( Object *other )
{
	if( other == NULL )
		return FALSE;

	const SalvageCrateCollideModuleData *md = getSalvageCrateCollideModuleData();

	// You must not be fully upgraded
	if( md->m_weaponBonus.size() == 0 )
		return FALSE;

	return !testWeaponBonus(other, md->m_weaponBonus[md->m_weaponBonus.size()-1]);
}

// ------------------------------------------------------------------------------------------------
Bool SalvageCrateCollide::testWeaponBonus( Object *other, WeaponBonusTypes bonus ) const
{
	if( bonus.bonusType == -1 )
	{
		if(other->testCustomWeaponBonusCondition(bonus.customType))
			return TRUE;
	}
	else
	{
		if(other->testWeaponBonusCondition(bonus.bonusType))
			return TRUE;
	}

	return FALSE;
}

// ------------------------------------------------------------------------------------------------
Bool SalvageCrateCollide::eligibleForStatus( Object *other )
{
	if( other == NULL )
		return FALSE;

	const SalvageCrateCollideModuleData *md = getSalvageCrateCollideModuleData();

	// You must not be fully upgraded
	if( md->m_status.size() == 0 )
		return FALSE;

	return !testStatus(other, md->m_status[md->m_status.size()-1]);
}

// ------------------------------------------------------------------------------------------------
Bool SalvageCrateCollide::testStatus( Object *other, StatusTypes status ) const
{
	if( status.statusType == -1 )
	{
		if(other->testCustomStatus(status.customType))
			return TRUE;
	}
	else
	{
		if(other->testStatus((ObjectStatusTypes)status.statusType))
			return TRUE;
	}

	return FALSE;
}

// ------------------------------------------------------------------------------------------------
Bool SalvageCrateCollide::multiFlagEligibleConditions( Object *other, Int completed, SalvagePickupType exceptions ) const
{
	// Sanity
	if(other == NULL)
		return FALSE;

	// We don't count bonus towards ourselves for completion
	if ( completed == exceptions )
		return FALSE;

	if ( completed == SALVAGE_MONEY || completed == SALVAGE_OCL )
	{
		DEBUG_CRASH(("Completion on Money or Spawning OCL? Impossible!"));
		return FALSE;
	}

	const SalvageCrateCollideModuleData *md = getSalvageCrateCollideModuleData();
	if(( completed == SALVAGE_WEAPON && md->m_weaponSetFlags.size() > 0 && other->testWeaponSetFlag(md->m_weaponSetFlags[md->m_weaponSetFlags.size()-1]) ) ||
	   ( completed == SALVAGE_ARMOR && md->m_armorSetFlags.size() > 0 && other->testArmorSetFlag(md->m_armorSetFlags[md->m_armorSetFlags.size()-1]) ) ||
	   ( completed == SALVAGE_LEVEL && other->getExperienceTracker()->getVeterancyLevel() == LEVEL_HEROIC ) ||
	   ( completed == SALVAGE_WEAPON_BONUS && md->m_weaponBonus.size() > 0 && testWeaponBonus(other, md->m_weaponBonus[md->m_weaponBonus.size()-1]) ) ||
	   ( completed == SALVAGE_STATUS && md->m_status.size() > 0 && testStatus(other, md->m_status[md->m_status.size()-1]) ) ||
	   ( completed == SALVAGE_UPGRADES && testUpgrade(other, true) ) ||
	   ( completed == SALVAGE_SCIENCE && md->m_sciencesGranted.size() > 0 && other->getControllingPlayer()->hasScience(md->m_sciencesGranted[md->m_sciencesGranted.size()-1]) ) ||
	   ( completed == SALVAGE_SCIENCE_POINTS && testSciencePurchasePoints(other, true) ) ||
	   ( completed == SALVAGE_SKILL_POINTS && testSkillPoints(other, true) )
	  )
		return true;
	else
		return false;
}

// ------------------------------------------------------------------------------------------------
Bool SalvageCrateCollide::testWeaponChance()
{
	const SalvageCrateCollideModuleData *md = getSalvageCrateCollideModuleData();
	if( md->m_weaponChance == 1.0f )
		return TRUE; // don't waste a random number for a 100%

	Real randomNumber = GameLogicRandomValueReal( 0, 1 );
	if( randomNumber < md->m_weaponChance )
		return TRUE;

	return FALSE;
}

// ------------------------------------------------------------------------------------------------
Bool SalvageCrateCollide::testLevelChance()
{
	const SalvageCrateCollideModuleData *md = getSalvageCrateCollideModuleData();
	if( md->m_levelChance == 1.0f )
		return TRUE; // don't waste a random number for a 100%

	Real randomNumber = GameLogicRandomValueReal( 0, 1 );
	if( randomNumber < md->m_levelChance )
		return TRUE;

	return FALSE;
}

// ------------------------------------------------------------------------------------------------
Bool SalvageCrateCollide::testUpgradeChance()
{
	const SalvageCrateCollideModuleData *md = getSalvageCrateCollideModuleData();
	if( md->m_upgradeChance == 1.0f )
		return TRUE; // don't waste a random number for a 100%

	Real randomNumber = GameLogicRandomValueReal( 0, 1 );
	if( randomNumber < md->m_upgradeChance )
		return TRUE;

	return FALSE;
}

// ------------------------------------------------------------------------------------------------
Bool SalvageCrateCollide::testScienceChance()
{
	const SalvageCrateCollideModuleData *md = getSalvageCrateCollideModuleData();
	if( md->m_scienceChance == 1.0f )
		return TRUE; // don't waste a random number for a 100%

	Real randomNumber = GameLogicRandomValueReal( 0, 1 );
	if( randomNumber < md->m_scienceChance )
		return TRUE;

	return FALSE;
}

// ------------------------------------------------------------------------------------------------
Bool SalvageCrateCollide::testSciencePurchasePointsChance()
{
	const SalvageCrateCollideModuleData *md = getSalvageCrateCollideModuleData();
	if( md->m_sciencePurchasePointsChance == 1.0f )
		return TRUE; // don't waste a random number for a 100%

	Real randomNumber = GameLogicRandomValueReal( 0, 1 );
	if( randomNumber < md->m_sciencePurchasePointsChance )
		return TRUE;

	return FALSE;
}

// ------------------------------------------------------------------------------------------------
Bool SalvageCrateCollide::testSkillPointsChance()
{
	const SalvageCrateCollideModuleData *md = getSalvageCrateCollideModuleData();
	if( md->m_skillPointsChance == 1.0f )
		return TRUE; // don't waste a random number for a 100%

	Real randomNumber = GameLogicRandomValueReal( 0, 1 );
	if( randomNumber < md->m_skillPointsChance )
		return TRUE;

	return FALSE;
}

// ------------------------------------------------------------------------------------------------
Bool SalvageCrateCollide::testWeaponBonusChance()
{
	const SalvageCrateCollideModuleData *md = getSalvageCrateCollideModuleData();
	if( md->m_weaponBonusChance == 1.0f )
		return TRUE; // don't waste a random number for a 100%

	Real randomNumber = GameLogicRandomValueReal( 0, 1 );
	if( randomNumber < md->m_weaponBonusChance )
		return TRUE;

	return FALSE;
}

// ------------------------------------------------------------------------------------------------
Bool SalvageCrateCollide::testStatusChance()
{
	const SalvageCrateCollideModuleData *md = getSalvageCrateCollideModuleData();
	if( md->m_statusChance == 1.0f )
		return TRUE; // don't waste a random number for a 100%

	Real randomNumber = GameLogicRandomValueReal( 0, 1 );
	if( randomNumber < md->m_statusChance )
		return TRUE;

	return FALSE;
}

// ------------------------------------------------------------------------------------------------
Bool SalvageCrateCollide::testOCLChance()
{
	const SalvageCrateCollideModuleData *md = getSalvageCrateCollideModuleData();
	if( md->m_oclChance == 1.0f )
		return TRUE; // don't waste a random number for a 100%

	Real randomNumber = GameLogicRandomValueReal( 0, 1 );
	if( randomNumber < md->m_oclChance )
		return TRUE;

	return FALSE;
}

// ------------------------------------------------------------------------------------------------
Bool SalvageCrateCollide::testArmorChanceMulti( Object *other )
{
	const SalvageCrateCollideModuleData *md = getSalvageCrateCollideModuleData();
	Real Chance = md->m_weaponChance;

	for(int i = 0; i < md->m_multiFlagCompletionInfos.size(); i++)
	{
		if( multiFlagEligibleConditions( other, md->m_multiFlagCompletionInfos[i].Completed, SALVAGE_ARMOR) )
		{
			for(int j = 0; j < md->m_multiFlagCompletionInfos[i].Bonuses.size(); j++)
			{
				if(md->m_multiFlagCompletionInfos[i].Bonuses[j].Bonus == SALVAGE_ARMOR)
				{
					Chance += md->m_multiFlagCompletionInfos[i].Bonuses[j].Chance;
					break;
				}
			}
		}
	}

	if( Chance >= 1.0f )
		return TRUE; // don't waste a random number for a 100%

	Real randomNumber = GameLogicRandomValueReal( 0, 1 );
	if( randomNumber < Chance )
		return TRUE;

	return FALSE;
}

// ------------------------------------------------------------------------------------------------
Bool SalvageCrateCollide::testWeaponChanceMulti( Object *other )
{
	const SalvageCrateCollideModuleData *md = getSalvageCrateCollideModuleData();
	Real Chance = md->m_weaponChance;

	for(int i = 0; i < md->m_multiFlagCompletionInfos.size(); i++)
	{
		if( multiFlagEligibleConditions( other, md->m_multiFlagCompletionInfos[i].Completed, SALVAGE_WEAPON) )
		{
			for(int j = 0; j < md->m_multiFlagCompletionInfos[i].Bonuses.size(); j++)
			{
				if(md->m_multiFlagCompletionInfos[i].Bonuses[j].Bonus == SALVAGE_WEAPON)
				{
					Chance += md->m_multiFlagCompletionInfos[i].Bonuses[j].Chance;
					break;
				}
			}
		}
	}

	if( Chance >= 1.0f )
		return TRUE; // don't waste a random number for a 100%

	Real randomNumber = GameLogicRandomValueReal( 0, 1 );
	if( randomNumber < Chance )
		return TRUE;

	return FALSE;
}

// ------------------------------------------------------------------------------------------------
Bool SalvageCrateCollide::testLevelChanceMulti( Object *other )
{
	const SalvageCrateCollideModuleData *md = getSalvageCrateCollideModuleData();
	Real Chance = md->m_levelChance;

	for(int i = 0; i < md->m_multiFlagCompletionInfos.size(); i++)
	{
		if( multiFlagEligibleConditions( other, md->m_multiFlagCompletionInfos[i].Completed, SALVAGE_LEVEL) )
		{
			for(int j = 0; j < md->m_multiFlagCompletionInfos[i].Bonuses.size(); j++)
			{
				if(md->m_multiFlagCompletionInfos[i].Bonuses[j].Bonus == SALVAGE_LEVEL)
				{
					Chance += md->m_multiFlagCompletionInfos[i].Bonuses[j].Chance;
					break;
				}
			}
		}
	}

	if( Chance >= 1.0f )
		return TRUE; // don't waste a random number for a 100%
	
	Real randomNumber = GameLogicRandomValueReal( 0, 1 );
	if( randomNumber < Chance )
		return TRUE;

	return FALSE;
}

// ------------------------------------------------------------------------------------------------
Bool SalvageCrateCollide::testUpgradeChanceMulti( Object *other )
{
	const SalvageCrateCollideModuleData *md = getSalvageCrateCollideModuleData();
	Real Chance = md->m_upgradeChance;

	for(int i = 0; i < md->m_multiFlagCompletionInfos.size(); i++)
	{
		if( multiFlagEligibleConditions( other, md->m_multiFlagCompletionInfos[i].Completed, SALVAGE_UPGRADES) )
		{
			for(int j = 0; j < md->m_multiFlagCompletionInfos[i].Bonuses.size(); j++)
			{
				if(md->m_multiFlagCompletionInfos[i].Bonuses[j].Bonus == SALVAGE_UPGRADES)
				{
					Chance += md->m_multiFlagCompletionInfos[i].Bonuses[j].Chance;
					break;
				}
			}
		}
	}

	if( Chance >= 1.0f )
		return TRUE; // don't waste a random number for a 100%
	
	Real randomNumber = GameLogicRandomValueReal( 0, 1 );
	if( randomNumber < Chance )
		return TRUE;

	return FALSE;
}

// ------------------------------------------------------------------------------------------------
Bool SalvageCrateCollide::testScienceChanceMulti( Object *other )
{
	const SalvageCrateCollideModuleData *md = getSalvageCrateCollideModuleData();
	Real Chance = md->m_scienceChance;

	for(int i = 0; i < md->m_multiFlagCompletionInfos.size(); i++)
	{
		if( multiFlagEligibleConditions( other, md->m_multiFlagCompletionInfos[i].Completed, SALVAGE_SCIENCE) )
		{
			for(int j = 0; j < md->m_multiFlagCompletionInfos[i].Bonuses.size(); j++)
			{
				if(md->m_multiFlagCompletionInfos[i].Bonuses[j].Bonus == SALVAGE_SCIENCE)
				{
					Chance += md->m_multiFlagCompletionInfos[i].Bonuses[j].Chance;
					break;
				}
			}
		}
	}

	if( Chance >= 1.0f )
		return TRUE; // don't waste a random number for a 100%
	
	Real randomNumber = GameLogicRandomValueReal( 0, 1 );
	if( randomNumber < Chance )
		return TRUE;

	return FALSE;
}

// ------------------------------------------------------------------------------------------------
Bool SalvageCrateCollide::testSciencePurchasePointsChanceMulti( Object *other )
{
	const SalvageCrateCollideModuleData *md = getSalvageCrateCollideModuleData();
	Real Chance = md->m_sciencePurchasePointsChance;

	for(int i = 0; i < md->m_multiFlagCompletionInfos.size(); i++)
	{
		if( multiFlagEligibleConditions( other, md->m_multiFlagCompletionInfos[i].Completed, SALVAGE_SCIENCE_POINTS) )
		{
			for(int j = 0; j < md->m_multiFlagCompletionInfos[i].Bonuses.size(); j++)
			{
				if(md->m_multiFlagCompletionInfos[i].Bonuses[j].Bonus == SALVAGE_SCIENCE_POINTS)
				{
					Chance += md->m_multiFlagCompletionInfos[i].Bonuses[j].Chance;
					break;
				}
			}
		}
	}

	if( Chance >= 1.0f )
		return TRUE; // don't waste a random number for a 100%
	
	Real randomNumber = GameLogicRandomValueReal( 0, 1 );
	if( randomNumber < Chance )
		return TRUE;

	return FALSE;
}

// ------------------------------------------------------------------------------------------------
Bool SalvageCrateCollide::testSkillPointsChanceMulti( Object *other )
{
	const SalvageCrateCollideModuleData *md = getSalvageCrateCollideModuleData();
	Real Chance = md->m_skillPointsChance;

	for(int i = 0; i < md->m_multiFlagCompletionInfos.size(); i++)
	{
		if( multiFlagEligibleConditions( other, md->m_multiFlagCompletionInfos[i].Completed, SALVAGE_SKILL_POINTS) )
		{
			for(int j = 0; j < md->m_multiFlagCompletionInfos[i].Bonuses.size(); j++)
			{
				if(md->m_multiFlagCompletionInfos[i].Bonuses[j].Bonus == SALVAGE_SKILL_POINTS)
				{
					Chance += md->m_multiFlagCompletionInfos[i].Bonuses[j].Chance;
					break;
				}
			}
		}
	}

	if( Chance >= 1.0f )
		return TRUE; // don't waste a random number for a 100%
	
	Real randomNumber = GameLogicRandomValueReal( 0, 1 );
	if( randomNumber < Chance )
		return TRUE;

	return FALSE;
}

// ------------------------------------------------------------------------------------------------
Bool SalvageCrateCollide::testWeaponBonusChanceMulti( Object *other )
{
	const SalvageCrateCollideModuleData *md = getSalvageCrateCollideModuleData();
	Real Chance = md->m_weaponBonusChance;

	for(int i = 0; i < md->m_multiFlagCompletionInfos.size(); i++)
	{
		if( multiFlagEligibleConditions( other, md->m_multiFlagCompletionInfos[i].Completed, SALVAGE_WEAPON_BONUS) )
		{
			for(int j = 0; j < md->m_multiFlagCompletionInfos[i].Bonuses.size(); j++)
			{
				if(md->m_multiFlagCompletionInfos[i].Bonuses[j].Bonus == SALVAGE_WEAPON_BONUS)
				{
					Chance += md->m_multiFlagCompletionInfos[i].Bonuses[j].Chance;
					break;
				}
			}
		}
	}

	if( Chance >= 1.0f )
		return TRUE; // don't waste a random number for a 100%
	
	Real randomNumber = GameLogicRandomValueReal( 0, 1 );
	if( randomNumber < Chance )
		return TRUE;

	return FALSE;
}

// ------------------------------------------------------------------------------------------------
Bool SalvageCrateCollide::testStatusChanceMulti( Object *other )
{
	const SalvageCrateCollideModuleData *md = getSalvageCrateCollideModuleData();
	Real Chance = md->m_statusChance;

	for(int i = 0; i < md->m_multiFlagCompletionInfos.size(); i++)
	{
		if( multiFlagEligibleConditions( other, md->m_multiFlagCompletionInfos[i].Completed, SALVAGE_STATUS) )
		{
			for(int j = 0; j < md->m_multiFlagCompletionInfos[i].Bonuses.size(); j++)
			{
				if(md->m_multiFlagCompletionInfos[i].Bonuses[j].Bonus == SALVAGE_STATUS)
				{
					Chance += md->m_multiFlagCompletionInfos[i].Bonuses[j].Chance;
					break;
				}
			}
		}
	}

	if( Chance >= 1.0f )
		return TRUE; // don't waste a random number for a 100%
	
	Real randomNumber = GameLogicRandomValueReal( 0, 1 );
	if( randomNumber < Chance )
		return TRUE;

	return FALSE;
}

// ------------------------------------------------------------------------------------------------
Bool SalvageCrateCollide::testOCLChanceMulti( Object *other )
{
	const SalvageCrateCollideModuleData *md = getSalvageCrateCollideModuleData();
	Real Chance = md->m_oclChance;

	for(int i = 0; i < md->m_multiFlagCompletionInfos.size(); i++)
	{
		if( multiFlagEligibleConditions( other, md->m_multiFlagCompletionInfos[i].Completed, SALVAGE_OCL) )
		{
			for(int j = 0; j < md->m_multiFlagCompletionInfos[i].Bonuses.size(); j++)
			{
				if(md->m_multiFlagCompletionInfos[i].Bonuses[j].Bonus == SALVAGE_OCL)
				{
					Chance += md->m_multiFlagCompletionInfos[i].Bonuses[j].Chance;
					break;
				}
			}
		}
	}

	if( Chance >= 1.0f )
		return TRUE; // don't waste a random number for a 100%
	
	Real randomNumber = GameLogicRandomValueReal( 0, 1 );
	if( randomNumber < Chance )
		return TRUE;

	return FALSE;
}

// ------------------------------------------------------------------------------------------------
void SalvageCrateCollide::doWeaponSet( Object *other )
{
	const SalvageCrateCollideModuleData *md = getSalvageCrateCollideModuleData();
	Int extralevels = 0;
	if(md->m_crateGivesMultiBonus)
	{
		for(int i = 0; i < md->m_multiFlagCompletionInfos.size(); i++)
		{
			if( multiFlagEligibleConditions( other, md->m_multiFlagCompletionInfos[i].Completed, SALVAGE_WEAPON) )
			{
				for(int j = 0; j < md->m_multiFlagCompletionInfos[i].Bonuses.size(); j++)
				{
					if(md->m_multiFlagCompletionInfos[i].Bonuses[j].Bonus == SALVAGE_WEAPON)
					{
						extralevels += md->m_multiFlagCompletionInfos[i].Bonuses[j].Amount;
						break;
					}
				}
			}
		}
	}
	// We do from reverse as the last flag indicates the last level
	for(int i = md->m_weaponSetFlags.size() - 1; i >= 0; i--) 
	{
		if( i == 0 && !other->testWeaponSetFlag( md->m_weaponSetFlags[i] ) )
		{
			Int level = max(0, extralevels);
			if(level >= md->m_weaponSetFlags.size())
				level = md->m_weaponSetFlags.size() - 1;

			other->setWeaponSetFlag( md->m_weaponSetFlags[level] );
			if(md->m_weaponModelConditionFlags.size() > 0)
			{
				if(level >= md->m_weaponModelConditionFlags.size())
					level = md->m_weaponModelConditionFlags.size() - 1;

				other->setModelConditionState(md->m_weaponModelConditionFlags[level]);
			}
		}
		else if( other->testWeaponSetFlag( md->m_weaponSetFlags[i] ) )
		{
			Int level = i + 1 + extralevels;
			level = max(0, level);
			if(level >= md->m_weaponSetFlags.size())
				level = md->m_weaponSetFlags.size() - 1;

			// If the current level is the same as granted, we don't do anything.
			if(level == i)
				break;

			other->clearWeaponSetFlag( md->m_weaponSetFlags[i] );
			other->setWeaponSetFlag( md->m_weaponSetFlags[level] );

			if(md->m_weaponModelConditionFlags.size() > i + 1)
			{
				if(level >= md->m_weaponModelConditionFlags.size())
					level = md->m_weaponModelConditionFlags.size() - 1;

				other->clearAndSetModelConditionState(md->m_weaponModelConditionFlags[i], md->m_weaponModelConditionFlags[level]);
			}
		}
	}
	/*if( other->testWeaponSetFlag( WEAPONSET_CRATEUPGRADE_ONE ) )
	{
		other->clearWeaponSetFlag( WEAPONSET_CRATEUPGRADE_ONE );
		other->setWeaponSetFlag( WEAPONSET_CRATEUPGRADE_TWO );
	}
	else
	{
		other->setWeaponSetFlag( WEAPONSET_CRATEUPGRADE_ONE );
	}*/
}

// ------------------------------------------------------------------------------------------------
void SalvageCrateCollide::doArmorSet( Object *other )
{
	const SalvageCrateCollideModuleData *md = getSalvageCrateCollideModuleData();
	Int extralevels = 0;
	if(md->m_crateGivesMultiBonus)
	{
		for(int i = 0; i < md->m_multiFlagCompletionInfos.size(); i++)
		{
			if( multiFlagEligibleConditions( other, md->m_multiFlagCompletionInfos[i].Completed, SALVAGE_ARMOR) )
			{
				for(int j = 0; j < md->m_multiFlagCompletionInfos[i].Bonuses.size(); j++)
				{
					if(md->m_multiFlagCompletionInfos[i].Bonuses[j].Bonus == SALVAGE_ARMOR)
					{
						extralevels += md->m_multiFlagCompletionInfos[i].Bonuses[j].Amount;
						break;
					}
				}
			}
		}
	}
	// We do from reverse as the last flag indicates the last level
	for(int i = md->m_armorSetFlags.size() - 1; i >= 0; i--) 
	{
		if( i == 0 && !other->testArmorSetFlag( md->m_armorSetFlags[i] ) )
		{
			Int level = max(0, extralevels);
			if(level >= md->m_armorSetFlags.size())
				level = md->m_armorSetFlags.size() - 1;
			
			other->setArmorSetFlag( md->m_armorSetFlags[level] );
			if(md->m_armorModelConditionFlags.size() > 0)
			{
				if(level >= md->m_armorModelConditionFlags.size())
					level = md->m_armorModelConditionFlags.size() - 1;

				other->setModelConditionState(md->m_armorModelConditionFlags[level]);
			}
		}
		else if( other->testArmorSetFlag( md->m_armorSetFlags[i] ) )
		{
			Int level = i + 1 + extralevels;
			level = max(0, level);
			if(level >= md->m_armorSetFlags.size())
				level = md->m_armorSetFlags.size() - 1;

			// If the current level is the same as granted, we don't do anything.
			if(level == i)
				break;

			other->clearArmorSetFlag( md->m_armorSetFlags[i] );
			other->setArmorSetFlag( md->m_armorSetFlags[level] );

			if(md->m_armorModelConditionFlags.size() > i + 1)
			{
				if(level >= md->m_armorModelConditionFlags.size())
					level = md->m_armorModelConditionFlags.size() - 1;

				other->clearAndSetModelConditionState(md->m_armorModelConditionFlags[i], md->m_armorModelConditionFlags[level]);
			}
		}
	}
	/*if( other->testArmorSetFlag( ARMORSET_CRATE_UPGRADE_ONE ) )
	{
		other->clearArmorSetFlag( ARMORSET_CRATE_UPGRADE_ONE );
		other->setArmorSetFlag( ARMORSET_CRATE_UPGRADE_TWO );

		other->clearAndSetModelConditionState(MODELCONDITION_ARMORSET_CRATEUPGRADE_ONE, MODELCONDITION_ARMORSET_CRATEUPGRADE_TWO);
	}
	else
	{
		other->setArmorSetFlag( ARMORSET_CRATE_UPGRADE_ONE );

		other->setModelConditionState(MODELCONDITION_ARMORSET_CRATEUPGRADE_ONE);
	}*/
}

// ------------------------------------------------------------------------------------------------
void SalvageCrateCollide::doLevelGain( Object *other )
{
	const SalvageCrateCollideModuleData *md = getSalvageCrateCollideModuleData();

	Int levels = 1;

	if(md->m_crateGivesMultiBonus)
	{
		for(int i = 0; i < md->m_multiFlagCompletionInfos.size(); i++)
		{
			if( multiFlagEligibleConditions( other, md->m_multiFlagCompletionInfos[i].Completed, SALVAGE_LEVEL) )
			{
				for(int j = 0; j < md->m_multiFlagCompletionInfos[i].Bonuses.size(); j++)
				{
					if(md->m_multiFlagCompletionInfos[i].Bonuses[j].Bonus == SALVAGE_LEVEL)
					{
						levels += md->m_multiFlagCompletionInfos[i].Bonuses[j].Amount;
						break;
					}
				}
			}
		}
	}
	other->getExperienceTracker()->gainExpForLevel( levels );
}

// ------------------------------------------------------------------------------------------------
//void SalvageCrateCollide::doMoney( Object *other )
Bool SalvageCrateCollide::doMoney( Object *other )
{
	const SalvageCrateCollideModuleData *md = getSalvageCrateCollideModuleData();

	Int money;
	Int minMoney = md->m_minimumMoney;
	Int maxMoney = md->m_maximumMoney;

	if(md->m_crateGivesMultiBonus)
	{
		for(int i = 0; i < md->m_multiFlagCompletionInfos.size(); i++)
		{
			if( multiFlagEligibleConditions( other, md->m_multiFlagCompletionInfos[i].Completed, SALVAGE_MONEY) )
			{
				for(int j = 0; j < md->m_multiFlagCompletionInfos[i].Bonuses.size(); j++)
				{
					if(md->m_multiFlagCompletionInfos[i].Bonuses[j].Bonus == SALVAGE_MONEY)
					{
						minMoney += md->m_multiFlagCompletionInfos[i].Bonuses[j].MinAmount;
						maxMoney += md->m_multiFlagCompletionInfos[i].Bonuses[j].MaxAmount;
						break;
					}
				}
			}
		}
	}
	if( minMoney != maxMoney )// Random value doesn't like to get a constant range
		money = GameLogicRandomValue( minMoney, maxMoney );
	else
		money = minMoney;

	if( money > 0 )
	{
		other->getControllingPlayer()->getMoney()->deposit( money );
		other->getControllingPlayer()->getScoreKeeper()->addMoneyEarned( money );

		//Display cash income floating over the crate.  Position is me, everything else is them.
		UnicodeString moneyString;
		moneyString.format( TheGameText->fetch( "GUI:AddCash" ), money );
		Coord3D pos;
		pos.set( getObject()->getPosition() );
		pos.z += 10.0f; //add a little z to make it show up above the unit.
		Color color = other->getControllingPlayer()->getPlayerColor() | GameMakeColor( 0, 0, 0, 230 );
		if(other->showCashText())
			TheInGameUI->addFloatingText( moneyString, &pos, color );

		return true;
	}
	else
	{
		return false;
	}
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void SalvageCrateCollide::grantUpgrades( Object *other )
{
	const SalvageCrateCollideModuleData *md = getSalvageCrateCollideModuleData();

	// Sanity
	if(md->m_grantUpgradeNames.empty())
		return;

	Int extralevels = 0;
	if(md->m_crateGivesMultiBonus)
	{
		for(int i = 0; i < md->m_multiFlagCompletionInfos.size(); i++)
		{
			if( multiFlagEligibleConditions( other, md->m_multiFlagCompletionInfos[i].Completed, SALVAGE_UPGRADES) )
			{
				for(int j = 0; j < md->m_multiFlagCompletionInfos[i].Bonuses.size(); j++)
				{
					if(md->m_multiFlagCompletionInfos[i].Bonuses[j].Bonus == SALVAGE_UPGRADES)
					{
						extralevels += md->m_multiFlagCompletionInfos[i].Bonuses[j].Amount;
						break;
					}
				}
			}
		}
	}

	// We do from reverse as the last flag indicates the last level
	for(int i = md->m_grantUpgradeNames.size() - 1; i >= 0; i--) 
	{
		const UpgradeTemplate* theTemplate = TheUpgradeCenter->findUpgrade( md->m_grantUpgradeNames[i] );
		if( !theTemplate )
		{
			DEBUG_CRASH(("An upgrade module references '%s', which is not an Upgrade", md->m_grantUpgradeNames[i].str()));
			throw INI_INVALID_DATA;
		}
		if( !theTemplate )
		{
			DEBUG_ASSERTCRASH( 0, ("SalvageCrateCollide for %s can't find upgrade template %s.", obj->getTemplate()->getName(), md->m_grantUpgradeNames[i].str() ) );
			return;
		}

		if( theTemplate->getUpgradeType() == UPGRADE_TYPE_PLAYER )
		{
			Player *player = other->getControllingPlayer();
			if( player )
			{
				if( i == 0 && !player->hasUpgradeComplete( theTemplate ) )
				{
					Int level = max(0, extralevels);
					if(level >= md->m_grantUpgradeNames.size())
						level = md->m_grantUpgradeNames.size() - 1;
					
					doUpgrade(other, md->m_grantUpgradeNames[level]);
					break;
				}
				else if( player->hasUpgradeComplete( theTemplate ) )
				{
					Int level = i + 1 + extralevels;
					level = max(0, level);
					if(level >= md->m_grantUpgradeNames.size())
						level = md->m_grantUpgradeNames.size() - 1;

					// If the current level is the same as granted, we don't do anything.
					if(level != i)
						doUpgrade(other, md->m_grantUpgradeNames[level], md->m_grantUpgradeNames[i]);
					break;
				}
			}
		}
		else
		{
			if( i == 0 && !other->hasUpgrade( theTemplate ) )
			{
				Int level = max(0, extralevels);
				if(level >= md->m_grantUpgradeNames.size())
					level = md->m_grantUpgradeNames.size() - 1;
				
				doUpgrade(other, md->m_grantUpgradeNames[level]);
				break;
			}
			else if( other->hasUpgrade( theTemplate ) )
			{
				Int level = i + 1 + extralevels;
				level = max(0, level);
				if(level >= md->m_grantUpgradeNames.size())
					level = md->m_grantUpgradeNames.size() - 1;

				// If the current level is the same as granted, we don't do anything.
				if(level != i)
					doUpgrade(other, md->m_grantUpgradeNames[level], md->m_grantUpgradeNames[i]);
				break;
			}
		}
	}

}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void SalvageCrateCollide::doUpgrade( Object *other, const AsciiString& grantUpgradeName, const AsciiString& removeUpgradeName )
{
	const UpgradeTemplate* theTemplate;
	if(!removeUpgradeName.isEmpty() && getSalvageCrateCollideModuleData()->m_removePreviousUpgrades)
	{
		theTemplate = TheUpgradeCenter->findUpgrade( removeUpgradeName );
		if( !theTemplate )
		{
			DEBUG_CRASH(("An upgrade module references '%s', which is not an Upgrade", removeUpgradeName.str()));
			throw INI_INVALID_DATA;
		}
		if( !theTemplate )
		{
			DEBUG_ASSERTCRASH( 0, ("SalvageCrateCollide for %s can't find upgrade template %s.", obj->getTemplate()->getName(), removeUpgradeName.str() ) );
			return;
		}
		if( theTemplate )
		{
			//Check if it's a player Upgrade...
			if( theTemplate->getUpgradeType() == UPGRADE_TYPE_PLAYER )
			{
				Player *player = other->getControllingPlayer();
				player->removeUpgrade( theTemplate );
			}
			//We found the upgrade, now see if the parent object has it set...
			else if( other->hasUpgrade( theTemplate ) )
			{
				//Cool, now remove it.
				other->removeUpgrade( theTemplate );
			}
			else
			{
				DEBUG_ASSERTCRASH( 0, ("SalvageCrateCollide trying to removing Upgrade %s from Object %s",
					other->getName().str(),
					removeUpgradeName.str() ) );
			}
		}
	}

	theTemplate = TheUpgradeCenter->findUpgrade( grantUpgradeName );
	if( !theTemplate )
	{
		DEBUG_CRASH(("An upgrade module references '%s', which is not an Upgrade", grantUpgradeName.str()));
		throw INI_INVALID_DATA;
	}
	if( !theTemplate )
	{
		DEBUG_ASSERTCRASH( 0, ("SalvageCrateCollide for %s can't find upgrade template %s.", obj->getTemplate()->getName(), grantUpgradeName.str() ) );
		return;
	}

	Player *player = other->getControllingPlayer();
	if( theTemplate->getUpgradeType() == UPGRADE_TYPE_PLAYER )
	{
		if(player)
		{
			// find and cancel any existing upgrades in the player's queue
			player->findUpgradeInQueuesAndCancelThem( theTemplate );
			// get the player
			player->addUpgrade( theTemplate, UPGRADE_STATUS_COMPLETE );
		}
	}
	else
	{
		// Fail safe if in any other condition, for example: Undead Body, or new Future Implementations such as UpgradeDie to Give Upgrades.
		ProductionUpdateInterface *pui = other->getProductionUpdateInterface();
		if( pui )
		{
			pui->cancelUpgrade( theTemplate );
		}
		other->giveUpgrade( theTemplate );
	}
	
	if(player)
		player->getAcademyStats()->recordUpgrade( theTemplate, TRUE );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void SalvageCrateCollide::grantSciences( Object *other )
{
	const SalvageCrateCollideModuleData *md = getSalvageCrateCollideModuleData();
	if(md->m_sciencesGranted.empty())
		return;

	Int extralevels = 0;
	if(md->m_crateGivesMultiBonus)
	{
		for(int i = 0; i < md->m_multiFlagCompletionInfos.size(); i++)
		{
			if( multiFlagEligibleConditions( other, md->m_multiFlagCompletionInfos[i].Completed, SALVAGE_SCIENCE) )
			{
				for(int j = 0; j < md->m_multiFlagCompletionInfos[i].Bonuses.size(); j++)
				{
					if(md->m_multiFlagCompletionInfos[i].Bonuses[j].Bonus == SALVAGE_SCIENCE)
					{
						extralevels += md->m_multiFlagCompletionInfos[i].Bonuses[j].Amount;
						break;
					}
				}
			}
		}
	}
	// We do from reverse as the last flag indicates the last level
	Player *player = other->getControllingPlayer();
	for(int i = md->m_sciencesGranted.size() - 1; i >= 0; i--) 
	{
		if( i == 0 && !player->hasScience( md->m_sciencesGranted[i] ) )
		{
			Int level = max(0, extralevels);
			if(level >= md->m_sciencesGranted.size())
				level = md->m_sciencesGranted.size() - 1;
			
			player->grantScience( md->m_sciencesGranted[level] );
			break;
		}
		else if( player->hasScience( md->m_sciencesGranted[i] ) )
		{
			Int level = i + 1 + extralevels;
			level = max(0, level);
			if(level >= md->m_sciencesGranted.size())
				level = md->m_sciencesGranted.size() - 1;

			// There is currently no way to remove sciences.
			// If the current level is the same as granted, we don't do anything.
			if(level != i)
				player->grantScience( md->m_sciencesGranted[level] );
			break;
		}
	}

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void SalvageCrateCollide::doWeaponBonus( Object *other )
{
	const SalvageCrateCollideModuleData *md = getSalvageCrateCollideModuleData();
	if(md->m_weaponBonus.empty())
		return;

	Int extralevels = 0;
	if(md->m_crateGivesMultiBonus)
	{
		for(int i = 0; i < md->m_multiFlagCompletionInfos.size(); i++)
		{
			if( multiFlagEligibleConditions( other, md->m_multiFlagCompletionInfos[i].Completed, SALVAGE_WEAPON_BONUS) )
			{
				for(int j = 0; j < md->m_multiFlagCompletionInfos[i].Bonuses.size(); j++)
				{
					if(md->m_multiFlagCompletionInfos[i].Bonuses[j].Bonus == SALVAGE_WEAPON_BONUS)
					{
						extralevels += md->m_multiFlagCompletionInfos[i].Bonuses[j].Amount;
						break;
					}
				}
			}
		}
	}

	// We do from reverse as the last flag indicates the last level
	for(int i = md->m_weaponBonus.size() - 1; i >= 0; i--) 
	{
		if( i == 0 && !testWeaponBonus( other, md->m_weaponBonus[i] ) )
		{
			Int level = max(0, extralevels);
			if(level >= md->m_weaponBonus.size())
				level = md->m_weaponBonus.size() - 1;

			DEBUG_LOG(("SalvageCrateCollide: Granting Weapon Bonus First"));
			if(md->m_weaponBonus[level].bonusType == -1)
				other->setCustomWeaponBonusCondition( md->m_weaponBonus[level].customType );
			else
				other->setWeaponBonusCondition( md->m_weaponBonus[level].bonusType );

			break;
		}
		else if( testWeaponBonus( other, md->m_weaponBonus[i] ) )
		{
			Int level = i + 1 + extralevels;
			level = max(0, level);
			if(level >= md->m_weaponBonus.size())
				level = md->m_weaponBonus.size() - 1;

			DEBUG_LOG(("SalvageCrateCollide: Granting Weapon Bonus Other"));
			// If the current level is the same as granted, we don't do anything.
			if(level == i)
				break;

			if(md->m_removePreviousWeaponBonus)
			{
				if(md->m_weaponBonus[i].bonusType == -1)
					other->clearCustomWeaponBonusCondition( md->m_weaponBonus[i].customType );
				else
					other->clearWeaponBonusCondition( md->m_weaponBonus[i].bonusType );
			}

			if(md->m_weaponBonus[level].bonusType == -1)
				other->setCustomWeaponBonusCondition( md->m_weaponBonus[level].customType );
			else
				other->setWeaponBonusCondition( md->m_weaponBonus[level].bonusType );

			break;
		}
	}

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void SalvageCrateCollide::doStatus( Object *other )
{
	const SalvageCrateCollideModuleData *md = getSalvageCrateCollideModuleData();
	if(md->m_status.empty())
		return;

	Int extralevels = 0;
	if(md->m_crateGivesMultiBonus)
	{
		for(int i = 0; i < md->m_multiFlagCompletionInfos.size(); i++)
		{
			if( multiFlagEligibleConditions( other, md->m_multiFlagCompletionInfos[i].Completed, SALVAGE_STATUS) )
			{
				for(int j = 0; j < md->m_multiFlagCompletionInfos[i].Bonuses.size(); j++)
				{
					if(md->m_multiFlagCompletionInfos[i].Bonuses[j].Bonus == SALVAGE_STATUS)
					{
						extralevels += md->m_multiFlagCompletionInfos[i].Bonuses[j].Amount;
						break;
					}
				}
			}
		}
	}
	// We do from reverse as the last flag indicates the last level
	for(int i = md->m_status.size() - 1; i >= 0; i--) 
	{
		if( i == 0 && !testStatus( other, md->m_status[i] ) )
		{
			Int level = max(0, extralevels);
			if(level >= md->m_status.size())
				level = md->m_status.size() - 1;

			if(md->m_status[level].statusType == -1)
				other->setCustomStatus( md->m_status[level].customType );
			else
				other->setStatus( MAKE_OBJECT_STATUS_MASK(md->m_status[level].statusType) );

			break;
		}
		else if( testStatus( other, md->m_status[i] ) )
		{
			Int level = i + 1 + extralevels;
			level = max(0, level);
			if(level >= md->m_status.size())
				level = md->m_status.size() - 1;

			// If the current level is the same as granted, we don't do anything.
			if(level == i)
				break;

			if(md->m_removePreviousStatus)
			{
				if(md->m_status[level].statusType == -1)
					other->clearCustomStatus( md->m_status[i].customType );
				else
					other->setStatus( MAKE_OBJECT_STATUS_MASK(md->m_status[i].statusType), FALSE );
			}

			if(md->m_status[level].statusType == -1)
				other->setCustomStatus( md->m_status[level].customType );
			else
				other->setStatus( MAKE_OBJECT_STATUS_MASK(md->m_status[level].statusType) );

			break;
		}
	}

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
Bool SalvageCrateCollide::grantSciencePurchasePoints( Object *other )
{
	const SalvageCrateCollideModuleData *md = getSalvageCrateCollideModuleData();
	Int points;
	Int minPoints = md->m_minimumSciencePurchasePoints;
	Int maxPoints = md->m_maximumSciencePurchasePoints;

	if(md->m_crateGivesMultiBonus)
	{
		for(int i = 0; i < md->m_multiFlagCompletionInfos.size(); i++)
		{
			if( multiFlagEligibleConditions( other, md->m_multiFlagCompletionInfos[i].Completed, SALVAGE_SCIENCE_POINTS) )
			{
				for(int j = 0; j < md->m_multiFlagCompletionInfos[i].Bonuses.size(); j++)
				{
					if(md->m_multiFlagCompletionInfos[i].Bonuses[j].Bonus == SALVAGE_SCIENCE_POINTS)
					{
						minPoints += md->m_multiFlagCompletionInfos[i].Bonuses[j].MinAmount;
						maxPoints += md->m_multiFlagCompletionInfos[i].Bonuses[j].MaxAmount;
						break;
					}
				}
			}
		}
	}

	if( minPoints != maxPoints )// Random value doesn't like to get a constant range
		points = GameLogicRandomValue( minPoints, maxPoints );
	else
		points = minPoints;

	if( points > 0 )
	{
		other->getControllingPlayer()->addSciencePurchasePoints( points );

		return true;
	}
	else
	{
		return false;
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
Bool SalvageCrateCollide::grantSkillPoints( Object *other )
{
	const SalvageCrateCollideModuleData *md = getSalvageCrateCollideModuleData();
	Int points;
	Int minPoints = md->m_minimumSkillPoints;
	Int maxPoints = md->m_maximumSkillPoints;

	if(md->m_crateGivesMultiBonus)
	{
		for(int i = 0; i < md->m_multiFlagCompletionInfos.size(); i++)
		{
			if( multiFlagEligibleConditions( other, md->m_multiFlagCompletionInfos[i].Completed, SALVAGE_SKILL_POINTS) )
			{
				for(int j = 0; j < md->m_multiFlagCompletionInfos[i].Bonuses.size(); j++)
				{
					if(md->m_multiFlagCompletionInfos[i].Bonuses[j].Bonus == SALVAGE_SKILL_POINTS)
					{
						minPoints += md->m_multiFlagCompletionInfos[i].Bonuses[j].MinAmount;
						maxPoints += md->m_multiFlagCompletionInfos[i].Bonuses[j].MaxAmount;
						break;
					}
				}
			}
		}
	}

	if( minPoints != maxPoints )// Random value doesn't like to get a constant range
		points = GameLogicRandomValue( minPoints, maxPoints );
	else
		points = minPoints;

	if( points > 0 )
	{
		other->getControllingPlayer()->addSkillPoints( points );

		return true;
	}
	else
	{
		return false;
	}
}

// ------------------------------------------------------------------------------------------------
Bool SalvageCrateCollide::doOCL( Object *other )
{
	const SalvageCrateCollideModuleData *md = getSalvageCrateCollideModuleData();
	if(md->m_ocls.empty())
		return false;

	Int level = 1;
	if(md->m_crateGivesMultiBonus)
	{
		for(int i = 0; i < md->m_multiFlagCompletionInfos.size(); i++)
		{
			if( multiFlagEligibleConditions( other, md->m_multiFlagCompletionInfos[i].Completed, SALVAGE_OCL) )
			{
				for(int j = 0; j < md->m_multiFlagCompletionInfos[i].Bonuses.size(); j++)
				{
					if(md->m_multiFlagCompletionInfos[i].Bonuses[j].Bonus == SALVAGE_OCL)
					{
						level += md->m_multiFlagCompletionInfos[i].Bonuses[j].Amount;
						break;
					}
				}
			}
		}
	}

	if( level > 0 )
	{
		if(level >= md->m_ocls.size())
			level = md->m_ocls.size() - 1;

		ObjectCreationList::create(md->m_ocls[level], other, NULL );

		return true;
	}
	else
	{
		return false;
	}
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void SalvageCrateCollide::crc( Xfer *xfer )
{

	// extend base class
	CrateCollide::crc( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void SalvageCrateCollide::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	CrateCollide::xfer( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void SalvageCrateCollide::loadPostProcess( void )
{

	// extend base class
	CrateCollide::loadPostProcess();

}

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

// FILE: ArmorTemplate.cpp ///////////////////////////////////////////////////////////////////////////////
// Author: Steven Johnson, November 2001
// Desc:   ArmorTemplate descriptions
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file int the GameEngine


#define DEFINE_WEAPONBONUSCONDITION_NAMES
#include "Common/INI.h"
#include "Common/ThingFactory.h"
#include "GameLogic/Armor.h"
#include "GameLogic/Damage.h"
#include "GameLogic/Weapon.h"
#include "Common/ObjectStatusTypes.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC DATA ////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
ArmorStore* TheArmorStore = NULL;					///< the ArmorTemplate store definition

///////////////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE DATA ///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS ///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------------------------------
ArmorTemplate::ArmorTemplate()
{
	clear();
}

//-------------------------------------------------------------------------------------------------
void ArmorTemplate::clear()
{
	for (int i = 0; i < DAMAGE_NUM_TYPES; i++)
	{
		m_damageCoefficient[i] = 1.0f;
	}
	for (int i = 0; i < OBJECT_STATUS_COUNT; ++i)
	{
		m_statusCoefficient[i] = 1.0f;
	}
	for (int i = 0; i < WEAPONBONUSCONDITION_COUNT; ++i)
	{
		m_weaponBonusCoefficient[i] = 1.0f;
	}
	m_customCoefficients.clear();
	m_customStatusArmorBonus.clear();
	m_statusFlags.clear();
}

void ArmorTemplate::copyFrom(const ArmorTemplate* other) {
	for (int i = 0; i < DAMAGE_NUM_TYPES; i++)
	{
		m_damageCoefficient[i] = other->m_damageCoefficient[i];
	}
	for (int i = 0; i < OBJECT_STATUS_COUNT; ++i)
	{
		m_statusCoefficient[i] = other->m_statusCoefficient[i];
	}
	for (int i = 0; i < WEAPONBONUSCONDITION_COUNT; ++i)
	{
		m_weaponBonusCoefficient[i] = other->m_weaponBonusCoefficient[i];
	}
	for (CustomDamageTypeMap::const_iterator it = other->m_customCoefficients.begin(); it != other->m_customCoefficients.end(); ++it)
	{
		m_customCoefficients[it->first] = it->second;
	}
	for (CustomDamageTypeMap::const_iterator it = other->m_customStatusArmorBonus.begin(); it != other->m_customStatusArmorBonus.end(); ++it)
	{
		m_customStatusArmorBonus[it->first] = it->second;
	}
	for (int i = 0; i < other->m_statusFlags.size(); i++)
	{
		m_statusFlags.push_back(other->m_statusFlags[i]);
	}
}

// Apply damage amplification/reduction for armor bonuses.
Real ArmorTemplate::scaleArmorBonus(ObjectStatusMaskType statusType, WeaponBonusConditionFlags weaponBonusType, ObjectCustomStatusType customStatusType, ObjectCustomStatusType customBonusType) const
{
	Real damage = 1.0f;
	if(weaponBonusType != 0)
	{
		for (int i = 0; i < WEAPONBONUSCONDITION_COUNT; ++i)
		{
			if ((weaponBonusType & (1 << i)) == 0)
				continue;

			damage *= m_weaponBonusCoefficient[i];
		}
	}
	if(!m_statusFlags.empty())
	{
		for (int i = 0; i < m_statusFlags.size(); i++)
		{
			if (statusType.test(m_statusFlags[i]))
			{
				damage *= m_statusCoefficient[m_statusFlags[i]];
			}
		}
	}
	if(!customStatusType.empty() && !m_customStatusArmorBonus.empty())
	{
		for(CustomDamageTypeMap::const_iterator it = m_customStatusArmorBonus.begin(); it != m_customStatusArmorBonus.end(); ++it)
		{
			ObjectCustomStatusType::const_iterator it2 = customBonusType.find(it->first);
			if (it2 != customBonusType.end())
			{
				if(it2->second == 1) 
					damage *= it->second;
				continue;
			}
			ObjectCustomStatusType::const_iterator it3 = customStatusType.find(it->first);
			if (it3 != customStatusType.end() && it3->second == 1)
			{
				damage *= it->second;
			}
		}
	}
	return damage;
}

//-------------------------------------------------------------------------------------------------
Real ArmorTemplate::adjustDamage(DamageType t, Real damage, const AsciiString& ct) const
{
	if (ct.isNotEmpty())
	{
		// Find and return the Custom Coefficient assigned, easy.
		if(!m_customCoefficients.empty())
		{
			CustomDamageTypeMap::const_iterator it = m_customCoefficients.find(ct);

			// The found the CustomDamageType at the declared CustomDamageTypes data.
			if (it != m_customCoefficients.end())
			{
				damage *= it->second;
				return damage;
			}
		}

		//ArmorStore::CustomDamageTypesMap customDamageList = TheArmorStore->getCustomDamageTypes();

		// If not found find it in Global data
		if (TheArmorStore->getCustomDamageTypesSize()>0)
		{
			if(TheArmorStore->isNameInTypesList(ct))
			{
				// Find if any of CustomDamageType is assigned with any of the CustomArmor.
				// Ignore if the unit does not haven any CustomArmor Coefficient assigned or the Linked Custom Armor List is empty.
				std::vector<AsciiString> stringListLink;
				stringListLink = TheArmorStore->GetLinkInTypesList(ct);
				if (!m_customCoefficients.empty() && !stringListLink.empty())
				{
					std::vector<AsciiString> stringListParent;
					std::vector<AsciiString> stringListChild;
					//std::vector<AsciiString> stringListChecked;
					StringListCheckMap stringListChecked;
					stringListChild = stringListLink;

					// This is a basic concept of a DNA finder. It checks regarding the most compatible strain needed until all its LinkedCustomTypes are exhausted.

					while (!stringListChild.empty())
					{
						// Check if there's a String List that exists for the Child.
						// If we do, clear the parents DNA that has been searched and find relative Childs DNA.
						//stringListParent.clear();
						stringListParent = stringListChild;
						stringListChild.clear();

						// Begin finding the LinkedCustomDamageTypes and see if they are exactly the same as configured Custom Armor List.
						for (std::vector<AsciiString>::const_iterator it2 = stringListParent.begin();
							it2 != stringListParent.end();
							++it2)
						{
							// Check if it's already checked, skip it if it does.
							// This is to prevent infinite loop.
							if(stringListChecked[*it2] == 1)
								continue;

							CustomDamageTypeMap::const_iterator it3 = m_customCoefficients.find(*it2);

							// Found the CustomDamageType at the declared CustomDamageTypes data.
							if (it3 != m_customCoefficients.end())
							{
								damage *= it3->second;
								return damage;
							}

							if (TheArmorStore->isNameInTypesList(*it2))
							{
								stringListLink = TheArmorStore->GetLinkInTypesList(*it2);
								for (std::vector<AsciiString>::const_iterator link_it = stringListLink.begin();
									link_it != stringListLink.end();
									++link_it)
								{
									stringListChild.push_back(*link_it);
								}
							}

							stringListChecked[*it2] = 1;
						}
					}
				}

				// If it does not have any LinkedCustomDamageType, then we find the LinkedDamageType, if declared.
				DamageType declaredLinkedDamage = TheArmorStore->GetDeclaredLinkDamageType(ct);
				if (declaredLinkedDamage != DAMAGE_NUM_TYPES)
				{
					damage *= m_damageCoefficient[declaredLinkedDamage];
					return damage;
				}
				// If not, use the default value, if assigned.
				// If non-of these are configured. Use the Default DamageType.
				Real declaredCoefficient = TheArmorStore->GetDeclaredCoefficient(ct);
				if (declaredCoefficient >= 0)
				{
					damage *= declaredCoefficient;
					return damage;
				}
			}
		}
	}
	if (t == DAMAGE_UNRESISTABLE)
		return damage;
	if (t == DAMAGE_SUBDUAL_UNRESISTABLE)
		return damage;
	if (t == DAMAGE_CHRONO_UNRESISTABLE)
		return damage;

	damage *= m_damageCoefficient[t];

	if (damage < 0.0f)
		damage = 0.0f;
	
	return damage;
}

//-------------------------------------------------------------------------------------------Static
/*static*/ void ArmorTemplate::parseArmorCoefficients( INI* ini, void *instance, void* /* store */, const void* userData )
{
	ArmorTemplate* self = (ArmorTemplate*) instance;

	AsciiString damageNameStr = ini->getNextQuotedAsciiString();
	const char* damageName = damageNameStr.str();
	Real pct = INI::scanPercentToReal(ini->getNextToken());

	if (stricmp(damageName, "Default") == 0)
	{
		for (Int i = 0; i < DAMAGE_NUM_TYPES; i++)
		{
			self->m_damageCoefficient[i] = pct;
		}
		return;
	}

	DamageType dt = (DamageType)DamageTypeFlags::getSingleBitFromName(damageName);
	if (dt >= 0)
	{
		self->m_damageCoefficient[dt] = pct;
	}
	else
	{
		// Compatible with ArmorExtend
		CustomDamageTypeMap::iterator it = self->m_customCoefficients.find(damageNameStr);

		// The found the CustomDamageType at the declared CustomDamageTypes data.
		if (it != self->m_customCoefficients.end())
		{
			it->second = pct;
		}
		else
		{
			self->m_customCoefficients[damageNameStr] = pct;
		}
	}
}

void ArmorTemplate::parseArmorBonus(INI* ini, void* instance, void* /* store */, const void* userData)
{
	ArmorTemplate* self = (ArmorTemplate*)instance;

	AsciiString conditionNameStr = ini->getNextQuotedAsciiString();
	const char* conditionName = conditionNameStr.str(); //ini->getNextToken();
	Real pct = INI::scanPercentToReal(ini->getNextToken());

	Bool isObjectStatus;
	Bool isWeaponBonusCondition;

	Int count = 0;
	for(ConstCharPtrArray name = TheWeaponBonusNames; *name; name++, count++ )
	{
		if( stricmp( *name, conditionName ) == 0 )
		{
			isWeaponBonusCondition = TRUE;
			break;
		}
	}
	if (count >= 0 && isWeaponBonusCondition == TRUE)
	{
		self->m_weaponBonusCoefficient[count] = pct;
	}
	else
	{
		ObjectStatusTypes ost = (ObjectStatusTypes)ObjectStatusMaskType::getSingleBitFromName(conditionName);
		count = ost;
		if(count >= 0 && isWeaponBonusCondition == FALSE)
		{
			isObjectStatus = TRUE;
			self->m_statusCoefficient[count] = pct;
			self->m_statusFlags.push_back(ost);
		}
	}
	if (isObjectStatus == FALSE && isWeaponBonusCondition == FALSE)
	{
		// Compatible with ArmorExtend
		CustomDamageTypeMap::iterator it = self->m_customStatusArmorBonus.find(conditionNameStr);

		// The found the CustomDamageType at the declared CustomDamageTypes data.
		if (it != self->m_customStatusArmorBonus.end())
		{
			it->second = pct;
		}
		else
		{
			self->m_customStatusArmorBonus[conditionNameStr] = pct;
		}
	}
}

void ArmorTemplate::parseDefaultDamage(INI* ini, void*/*instance*/, void* /* store */, const void* /*userData*/)
{
	ArmorStore::CustomDamageType *customType = &TheArmorStore->m_customDamageTypeParse;

	if (customType->m_declaredCoefficient == TRUE)
		DEBUG_CRASH(("%s already has declared Default Coefficient.\n", customType->m_customDamageType.str()));

	INI::parsePercentToReal(ini, NULL, &customType->m_coefficient, NULL);
	customType->m_declaredCoefficient = TRUE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void ArmorTemplate::parseCustomDamageType(INI* ini, void*/*instance*/, void* /* store */, const void* /*userData*/)
{
	AsciiString *customDamagePtr = &TheArmorStore->m_customDamageTypeParseNext;
	
	if(customDamagePtr->isEmpty())
	{
		// Whatever works works, do not question the order of physics or why I decide to not put an '&'.
		INI::parseQuotedAsciiString(ini, NULL, customDamagePtr, NULL);
	}
	else
	{
		// Parse the values to the previous name
		ArmorStore::CustomDamageTypesMap::const_iterator it = TheArmorStore->m_customDamageTypes.find(*customDamagePtr);

		// The name is not registered in CustomDamageTypes, parse it into the Default DamageType
		if (it == TheArmorStore->m_customDamageTypes.end())
		{
			//m_customDamageTypes.push_back(m_customDamageTypeParse);
			TheArmorStore->m_customDamageTypes[*customDamagePtr] = TheArmorStore->m_customDamageTypeParse;

			//Reset the Parse Values
			TheArmorStore->m_customDamageTypeParse.m_coefficient = 1.0f;
			TheArmorStore->m_customDamageTypeParse.m_declaredLinkDamageType = FALSE;
			TheArmorStore->m_customDamageTypeParse.m_declaredCoefficient = FALSE;
			TheArmorStore->m_customDamageTypeParse.m_linkDamageType = DAMAGE_EXPLOSION;
			TheArmorStore->m_customDamageTypeParse.m_customDamageTypeLink.clear();

			//Designate the name of the next Custom Damage Type.
			*customDamagePtr = ini->getNextQuotedAsciiString();
		}
		else
		{
			DEBUG_CRASH(("Duplicate CustomDamageType Found (%s).\n", customDamageName.str()));
		}
	}
}

void ArmorTemplate::parseLinkDamageType(INI* ini, void*/*instance*/, void* /* store */, const void* /*userData*/)
{
	ArmorStore::CustomDamageType *customType = &TheArmorStore->m_customDamageTypeParse;

	if (customType->m_declaredLinkDamageType == TRUE)
		DEBUG_CRASH(("%s already has declared Default Coefficient.\n", customType->m_customDamageType.str()));

	DamageTypeFlags::parseSingleBitFromINI(ini, NULL, &customType->m_linkDamageType, NULL);
	customType->m_declaredLinkDamageType = TRUE;
}

void ArmorTemplate::parseLinkCustomDamageTypes(INI* ini, void*/*instance*/, void* /* store */, const void* /*userData*/)
{
	ArmorStore::CustomDamageType *customType = &TheArmorStore->m_customDamageTypeParse;

	INI::parseAsciiStringVectorAppend(ini, NULL, &customType->m_customDamageTypeLink, NULL);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
ArmorStore::ArmorStore()
{
	m_armorTemplates.clear();
	m_customDamageTypes.clear();
	m_customDamageTypeParseNext = NULL;

	m_customDamageTypeParse.m_coefficient = 1.0f;
	m_customDamageTypeParse.m_declaredLinkDamageType = FALSE;
	m_customDamageTypeParse.m_declaredCoefficient = FALSE;
	m_customDamageTypeParse.m_linkDamageType = DAMAGE_EXPLOSION;
	m_customDamageTypeParse.m_customDamageTypeLink.clear();
}

//-------------------------------------------------------------------------------------------------
ArmorStore::~ArmorStore()
{
	m_armorTemplates.clear();
	m_customDamageTypes.clear();
	m_customDamageTypeParseNext = NULL;

	m_customDamageTypeParse.m_coefficient = 1.0f;
	m_customDamageTypeParse.m_declaredLinkDamageType = FALSE;
	m_customDamageTypeParse.m_declaredCoefficient = FALSE;
	m_customDamageTypeParse.m_linkDamageType = DAMAGE_EXPLOSION;
	m_customDamageTypeParse.m_customDamageTypeLink.clear();
}

//-------------------------------------------------------------------------------------------------
const ArmorTemplate* ArmorStore::findArmorTemplate(AsciiString name) const
{
	NameKeyType namekey = TheNameKeyGenerator->nameToKey(name);
  ArmorTemplateMap::const_iterator it = m_armorTemplates.find(namekey);
  if (it == m_armorTemplates.end()) 
	{
		return NULL;
	}
	else
	{
		return &(*it).second;
	}
}

//-------------------------------------------------------------------------------------------------
/*static */ void ArmorStore::parseArmorDefinition(INI *ini)
{
	static const FieldParse myFieldParse[] = 
	{
		{ "Armor", ArmorTemplate::parseArmorCoefficients, NULL, 0 },
		{ "ArmorBonus", ArmorTemplate::parseArmorBonus, NULL, 0 },
		{ NULL, NULL, NULL, NULL }
	};

	const char *c = ini->getNextToken();
	NameKeyType key = TheNameKeyGenerator->nameToKey(c);
	ArmorTemplate& armorTmpl = TheArmorStore->m_armorTemplates[key];
	armorTmpl.clear();
	ini->initFromINI(&armorTmpl, myFieldParse);
}

//-------------------------------------------------------------------------------------------------
/*static */ void ArmorStore::parseArmorExtendDefinition(INI* ini)
{
	static const FieldParse myFieldParse[] =
	{
		{ "Armor", ArmorTemplate::parseArmorCoefficients, NULL, 0 },
		{ "ArmorBonus", ArmorTemplate::parseArmorBonus, NULL, 0 },
		{ NULL, NULL, NULL, NULL }
	};

	const char* new_armor_name = ini->getNextToken();

	const char* parent = ini->getNextToken();
	const ArmorTemplate* parentTemplate = TheArmorStore->findArmorTemplate(parent);
	if (parentTemplate == NULL) {
		DEBUG_CRASH(("ArmorExtend must extend a previously defined Armor (%s).\n", parent));
		throw INI_INVALID_DATA;
	}

	NameKeyType key = TheNameKeyGenerator->nameToKey(new_armor_name);
	ArmorTemplate& armorTmpl = TheArmorStore->m_armorTemplates[key];
	armorTmpl.clear();
	armorTmpl.copyFrom(parentTemplate);

	ini->initFromINI(&armorTmpl, myFieldParse);
}

//-------------------------------------------------------------------------------------------------
/*static*/ void INI::parseArmorDefinition(INI *ini)
{
	ArmorStore::parseArmorDefinition(ini);
}

//-------------------------------------------------------------------------------------------------
/*static*/ void INI::parseArmorExtendDefinition(INI* ini)
{
	ArmorStore::parseArmorExtendDefinition(ini);
}

//-------------------------------------------------------------------------------------------------
/*static*/ void INI::parseCustomDamageTypesDefinition(INI* ini)
{
	ArmorStore::parseCustomDamageTypesDefinition(ini);
}

void ArmorStore::parseCustomDamageTypesDefinition(INI* ini)
{

	static const FieldParse myFieldParse[] =
	{
		{ "CustomDamageType", ArmorTemplate::parseCustomDamageType, NULL, 0 },
		{ "Damage", ArmorTemplate::parseDefaultDamage, NULL, 0 },
		{ "LinkDamageType", ArmorTemplate::parseLinkDamageType, NULL, 0 },
		{ "LinkCustomDamageType", ArmorTemplate::parseLinkCustomDamageTypes, NULL, 0 },
		{ NULL, NULL, NULL, NULL }
	};

	ArmorStore::CustomDamageTypesMap& customDamageTypesMapInfo = TheArmorStore->m_customDamageTypes;
	customDamageTypesMapInfo.clear();

	ini->initFromINI( &customDamageTypesMapInfo, myFieldParse);

	if(TheArmorStore->m_customDamageTypeParseNext.isNotEmpty())
	{
		TheArmorStore->m_customDamageTypes[TheArmorStore->m_customDamageTypeParseNext] = TheArmorStore->m_customDamageTypeParse;
	}
}

Bool ArmorStore::isNameInTypesList(const AsciiString& ct) const
{
	CustomDamageTypesMap::const_iterator it = m_customDamageTypes.find(ct);

	// The found the CustomDamageType at the declared CustomDamageTypes data.
	if (it != m_customDamageTypes.end())
	{
		return TRUE;
	}
	return FALSE;
}

std::vector<AsciiString> ArmorStore::GetLinkInTypesList(const AsciiString& ct)
{
	std::vector<AsciiString> linkList;

	CustomDamageTypesMap::const_iterator it = m_customDamageTypes.find(ct);

	// The found the CustomDamageType at the declared CustomDamageTypes data.
	if (it != m_customDamageTypes.end())
	{
		return it->second.m_customDamageTypeLink;
	}
	return linkList;
}

DamageType ArmorStore::GetDeclaredLinkDamageType(const AsciiString& ct)
{
	CustomDamageTypesMap::const_iterator it = m_customDamageTypes.find(ct);

	// The found the CustomDamageType at the declared CustomDamageTypes data.
	if (it != m_customDamageTypes.end())
	{
		if (it->second.m_declaredLinkDamageType == FALSE)
			return DAMAGE_NUM_TYPES;
		return it->second.m_linkDamageType;
	}
	return DAMAGE_NUM_TYPES;
}

Real ArmorStore::GetDeclaredCoefficient(const AsciiString& ct)
{
	CustomDamageTypesMap::const_iterator it = m_customDamageTypes.find(ct);

	// The found the CustomDamageType at the declared CustomDamageTypes data.
	if (it != m_customDamageTypes.end())
	{
		if (it->second.m_declaredCoefficient == FALSE)
			return -1.0f;
		return it->second.m_coefficient;
	}
	return -1.0f;
}

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

// FILE: Armor.h /////////////////////////////////////////////////////////////////////////////////
// Author: Steven Johnson, March 2002
// Desc:   Damage Multiplier Descriptions
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "Common/NameKeyGenerator.h"
#include "Common/STLTypedefs.h"
#include "GameLogic/Damage.h"
#include "GameLogic/Weapon.h"
#include "Common/ObjectStatusTypes.h"

// FORWARD REFERENCES /////////////////////////////////////////////////////////////////////////////
class ArmorStore;
typedef std::pair<NameKeyType, Real> NameKeyTypeReal;
typedef std::vector<NameKeyTypeReal> CustomDamageTypeVec;
typedef std::pair<AsciiString, Real> AsciiStringReal;
typedef std::vector<AsciiStringReal> CustomDamageStrTypeVec;
//typedef std::hash_map<AsciiString, Real, rts::hash<AsciiString>, rts::equal_to<AsciiString> > CustomDamageTypeMap;
//typedef std::hash_map< AsciiString, Int, rts::hash<AsciiString>, rts::equal_to<AsciiString> > StringListCheckMap;

//-------------------------------------------------------------------------------------------------
/**
	An Armor encapsulates the a particular type of actual modifier to damage taken, in order
	to simulate different materials, and to help make game balance easier to adjust.
*/
//-------------------------------------------------------------------------------------------------
class ArmorTemplate
{
public:

	ArmorTemplate();

	void clear();

	void copyFrom(const ArmorTemplate* other);

	/**
		This is the real "meat" of the class: given a damage type and amount, adjust the damage
		and return the amount that should be dealt. 
	*/	
	Real adjustDamage(DamageType t, Real damage, const AsciiString& ct) const;
	Real scaleArmorBonus(ObjectStatusMaskType statusType, WeaponBonusConditionFlags weaponBonusType, const std::vector<AsciiString>& customStatusType, const std::vector<AsciiString>& customBonusType) const;

	static void parseArmorCoefficients( INI* ini, void *instance, void* /* store */, const void* userData );
	static void parseArmorMultiplier( INI* ini, void *instance, void* /* store */, const void* userData );
	static void parseArmorBonus(INI* ini, void* instance, void* /* store */, const void* userData);
	static void parseDefaultDamage(INI* ini, void*/*instance*/, void* /* store */, const void* /*userData*/);
	static void parseCustomDamageType(INI* ini, void*/*instance*/, void* /* store */, const void* /*userData*/);
	static void parseLinkDamageType(INI* ini, void*/*instance*/, void* /* store */, const void* /*userData*/);
	static void parseLinkCustomDamageTypes(INI* ini, void*/*instance*/, void* /* store */, const void* /*userData*/);

protected:

private:
	Real						m_damageCoefficient[DAMAGE_NUM_TYPES];	///< modifiers to damage
	Real						m_statusCoefficient[OBJECT_STATUS_COUNT];	///< modifiers to damage
	Real						m_weaponBonusCoefficient[WEAPONBONUSCONDITION_COUNT];	///< modifiers to damage
	std::vector<ObjectStatusTypes>	m_statusFlags;
	WeaponBonusConditionTypeVec		m_weaponBonusFlags;
	CustomDamageTypeVec			m_customCoefficients;
	CustomDamageStrTypeVec		m_customStatusArmorBonus;
	CustomDamageTypeVec			m_customMultCoefficients;
};  

//-------------------------------------------------------------------------------------------------
class Armor
{
public:

	inline Armor(const ArmorTemplate* tmpl = NULL) : m_template(tmpl)
	{
	}

	inline Real scaleArmorBonus(ObjectStatusMaskType statusType, WeaponBonusConditionFlags weaponBonusType, const std::vector<AsciiString>& customStatusType, const std::vector<AsciiString>& customBonusType) const
	{
		return m_template ? m_template->scaleArmorBonus(statusType, weaponBonusType, customStatusType, customBonusType) : 1.0f;
	}

	inline Real adjustDamage(DamageType t, Real damage, const AsciiString& ct) const
	{
		return m_template ? m_template->adjustDamage(t, damage, ct) : damage;
	}

	inline void clear()
	{
		m_template = NULL;
	}

private:
	const ArmorTemplate* m_template;		///< the kind of armor this is
};

//------------------------------------------------------------------------------------------------
/** Interface class for TheArmorStore, which is just a convenient place for us to
	* store each Armor we read from INI together in a list, with some access
	* methods for finding Armors */
//-------------------------------------------------------------------------------------------------
class ArmorStore : public SubsystemInterface
{

public:

	ArmorStore();
	~ArmorStore();

	void init() { }
	void reset() { }
	void update() { }

	const ArmorTemplate* findArmorTemplate(NameKeyType namekey) const;
	/**
		Find the Armor with the given name. If no such Armor exists, return null.
	*/
	const ArmorTemplate* findArmorTemplate(const AsciiString& name) const;
	const ArmorTemplate* findArmorTemplate(const char* name) const;

	inline Armor makeArmor(const ArmorTemplate *tmpl) const
	{
		return Armor(tmpl);	// my, that was easy
	}

	struct CustomDamageType
	{
		Real			m_coefficient;
		DamageType		m_linkDamageType;
		Bool			m_declaredLinkDamageType;
		Bool			m_declaredCoefficient;
		std::vector<NameKeyType> m_customDamageTypeLink;

		CustomDamageType() : m_coefficient(1.0f), m_declaredLinkDamageType(FALSE), m_declaredCoefficient(FALSE)
		{
			m_linkDamageType = DAMAGE_EXPLOSION;
			m_customDamageTypeLink.clear();
		}
	};

	//typedef std::hash_map< AsciiString, CustomDamageType, rts::hash<AsciiString>, rts::equal_to<AsciiString> > CustomDamageTypesMap;
	typedef std::hash_map< NameKeyType, CustomDamageType, rts::hash<NameKeyType>, rts::equal_to<NameKeyType> > CustomDamageTypesMap;
	CustomDamageTypesMap m_customDamageTypes;

	CustomDamageType m_customDamageTypeParse;
	AsciiString m_customDamageTypeParseNext;

	Bool findNameInTypesList(NameKeyType nameKey, Real &damage, const CustomDamageTypeVec& coefficients, DamageType &linkDamageType);
	//Bool isNameInTypesList(NameKeyType nameKey) const;
	std::vector<NameKeyType> GetLinkInTypesList(NameKeyType nameKey);
	//DamageType GetDeclaredLinkDamageType(NameKeyType nameKey);
	//Real GetDeclaredCoefficient(NameKeyType nameKey);

	static void parseArmorDefinition(INI* ini);
	static void parseArmorExtendDefinition(INI* ini);
	static void parseCustomDamageTypesDefinition(INI* ini);

private:

	typedef std::hash_map< NameKeyType, ArmorTemplate, rts::hash<NameKeyType>, rts::equal_to<NameKeyType> > ArmorTemplateMap;
	ArmorTemplateMap m_armorTemplates;

};

// EXTERNALS //////////////////////////////////////////////////////////////////////////////////////
extern ArmorStore *TheArmorStore;

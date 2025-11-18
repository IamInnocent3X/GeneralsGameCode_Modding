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
// FILE: HijackerUpdate.h /////////////////////////////////////////////////////////////////////////
// Author: Mark Lorenzen, July 2002
// Desc:   Allows hijacker to keep with his hijacked vehicle (though hidden) until it dies, then
// to become a hijacker once more
//
/////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef __HIJACKER_UPDATE_H
#define __HIJACKER_UPDATE_H

#include "Common/ObjectStatusTypes.h"
#include "GameLogic/Module/UpdateModule.h"

enum HijackType CPP_11(: Int)
{
	HIJACK_NONE,
	HIJACK_CARBOMB,
	HIJACK_HIJACKER,
	HIJACK_EQUIP,
	HIJACK_PARASITE
};

//-------------------------------------------------------------------------------------------------
class HijackerUpdateModuleData : public UpdateModuleData
{
public:
	AsciiString m_attachToBone;
	AsciiString m_parachuteName;

	//StickBombUpdateModuleData();

	static void buildFieldParse(MultiIniFieldParse& p)
	{
    UpdateModuleData::buildFieldParse(p);
		static const FieldParse dataFieldParse[] =
		{
			{ "AttachToTargetBone",	INI::parseAsciiString,		NULL, offsetof( HijackerUpdateModuleData, m_attachToBone ) },
			{ "ParachuteName",	INI::parseAsciiString,		NULL, offsetof( HijackerUpdateModuleData, m_parachuteName ) },
			{ 0, 0, 0, 0 }
		};
    p.add(dataFieldParse);
	}
};

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
class HijackerUpdateInterface
{
public:
	virtual void setTargetObject( const Object *object ) = 0;
	virtual void setRetargetObject( ObjectID ID ) = 0;
	virtual Object* getTargetObject() const = 0;
	virtual HijackType getHijackType() const = 0;
	virtual void setUpdate(Bool u ) = 0;
	virtual void setHijackType( HijackType i ) = 0;
	virtual void setIsInVehicle(Bool i ) = 0;
	virtual void setNoLeechExp(Bool i ) = 0;
	virtual void setIsParasite(Bool i ) = 0;
	virtual void setDestroyOnHeal(Bool i ) = 0;
	virtual void setRemoveOnHeal(Bool i ) = 0;
	virtual void setDestroyOnClear(Bool i ) = 0;
	virtual void setDestroyOnTargetDie(Bool i ) = 0;
	virtual void setPercentDamage(Real i ) = 0;
	virtual void setStatusToRemove(ObjectStatusMaskType i ) = 0;
	virtual void setStatusToDestroy(ObjectStatusMaskType i ) = 0;
	virtual void setCustomStatusToRemove(const std::vector<AsciiString>& i ) = 0;
	virtual void setCustomStatusToDestroy(const std::vector<AsciiString>& i ) = 0;
	virtual void setClear(Bool u ) = 0;
	virtual void setEject(Bool u ) = 0;
	virtual void setHealed(Bool u ) = 0;
	virtual void setNoSelfDamage(Bool u ) = 0; // This is to prevent Parasite from dealing damage to their allies
	virtual void setParasiteKey(const AsciiString& ParasiteKey ) = 0;
	virtual void setParasiteCheckKeys(const std::vector<AsciiString>& ParasiteKeys ) = 0;
};

//-------------------------------------------------------------------------------------------------
class HijackerUpdate : public UpdateModule, public HijackerUpdateInterface
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( HijackerUpdate, "HijackerUpdate" )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( HijackerUpdate, HijackerUpdateModuleData )

public:

	HijackerUpdate( Thing *thing, const ModuleData* moduleData );
	// virtual destructor prototype provided by memory pool declaration

	virtual HijackerUpdateInterface* getHijackerUpdateInterface() { return this; }
	virtual UpdateSleepTime update();							///< called once per frame

	virtual void setTargetObject( const Object *object );
	virtual void setRetargetObject( ObjectID ID );
	virtual Object* getTargetObject() const;
	virtual HijackType getHijackType() const { return m_hijackType; }
	virtual void setUpdate(Bool u ) {m_update = u; if(u) setWakeFrame(getObject(), UPDATE_SLEEP_NONE);}
	virtual void setIsInVehicle(Bool i ) {m_isInVehicle = i;}
	virtual void setHijackType( HijackType i ) { m_hijackType = i; }
	virtual void setNoLeechExp(Bool i ) {m_noLeechExp = i;}
	virtual void setIsParasite(Bool i ) {m_isParasite = i;}
	virtual void setDestroyOnHeal(Bool i ) {m_destroyOnHeal = i;}
	virtual void setRemoveOnHeal(Bool i ) {m_removeOnHeal = i;}
	virtual void setDestroyOnClear(Bool i ) {m_destroyOnClear = i;}
	virtual void setDestroyOnTargetDie(Bool i ) {m_destroyOnTargetDie = i;}
	virtual void setPercentDamage(Real i ) {m_percentDamage = i;}
	virtual void setStatusToRemove(ObjectStatusMaskType i ) {m_statusToRemove = i;}
	virtual void setStatusToDestroy(ObjectStatusMaskType i ) {m_statusToDestroy = i;}
	virtual void setCustomStatusToRemove(const std::vector<AsciiString>& i ) {m_customStatusToRemove = i;}
	virtual void setCustomStatusToDestroy(const std::vector<AsciiString>& i ) {m_customStatusToDestroy = i;}
	virtual void setClear(Bool u ) {m_clear = u;}
	virtual void setEject(Bool u ) {m_eject = u;}
	virtual void setHealed(Bool u ) {m_healed = u;}
	virtual void setNoSelfDamage(Bool u ) {m_noSelfDamage = u;} // This is to prevent Parasite from dealing damage to their allies
	virtual void setParasiteKey(const AsciiString& ParasiteKey ) {m_parasiteKey = ParasiteKey; }
	virtual void setParasiteCheckKeys(const std::vector<AsciiString>& ParasiteKeys ) { if(m_isParasite) m_recentParasiteKeys = ParasiteKeys; }

private:

	ObjectID m_targetID;
	HijackType m_hijackType;
	Coord3D	 m_ejectPos;
	Bool     m_update;
	Bool		 m_isInVehicle;
	Bool		 m_wasTargetAirborne;
	Bool		 m_isParasite;
	Bool		 m_noLeechExp;
	Bool		 m_destroyOnHeal;
	Bool		 m_removeOnHeal;
	Bool		 m_destroyOnClear;
	Bool		 m_destroyOnTargetDie;
	Bool     	 m_clear;
	Bool     	 m_eject;
	Bool     	 m_healed;
	Bool     	 m_noSelfDamage;
	Real		 m_percentDamage;
	Real		 m_targetObjHealth;
	AsciiString	 m_parasiteKey;
	ObjectStatusMaskType 	m_statusToRemove;
	ObjectStatusMaskType 	m_statusToDestroy;
	std::vector<AsciiString> 	m_customStatusToRemove;
	std::vector<AsciiString> 	m_customStatusToDestroy;
	std::vector<AsciiString> 	m_recentParasiteKeys;

//	DieModuleInterface *m_ejectPilotDMI; // point to ejectpilotdiemodule
																			 // of target vehicle if it has one

};

#endif // __HIJACKER_UPDATE_H


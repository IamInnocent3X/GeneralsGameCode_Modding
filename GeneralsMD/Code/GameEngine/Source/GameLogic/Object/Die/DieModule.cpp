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

// FILE: DieModule.cpp ////////////////////////////////////////////////////////////////////////////
// Author:
// Desc:
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "PreRTS.h"	// This must go first in EVERY cpp file int the GameEngine

#define DEFINE_OBJECT_STATUS_NAMES
#include "Common/Xfer.h"
#include "Common/GlobalData.h"
#include "GameClient/Drawable.h"
#include "GameLogic/ExperienceTracker.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/Module/DieModule.h"
#include "GameLogic/Object.h"





//-------------------------------------------------------------------------------------------------
DieMuxData::DieMuxData() {
	m_deathTypes = DEATH_TYPE_FLAGS_ALL;
	m_veterancyLevels = VETERANCY_LEVEL_FLAGS_ALL;
	m_deathTypesCustom.first = DEATH_TYPE_FLAGS_ALL;
	m_deathTypesCustom.second.format("ALL");
	m_requiredCustomStatus.clear();
	m_customDeathTypes.clear();

	if (TheGlobalData) {
		m_deathTypes &= ~TheGlobalData->m_defaultExcludedDeathTypes;
		m_deathTypesCustom.first &= ~TheGlobalData->m_defaultExcludedDeathTypes;
	}
}

//-------------------------------------------------------------------------------------------------
const FieldParse* DieMuxData::getFieldParse()
{
	static const FieldParse dataFieldParse[] =
	{
		//{ "DeathTypes",				INI::parseDeathTypeFlags,						NULL, offsetof( DieMuxData, m_deathTypes ) },
		{ "DeathTypes",			INI::parseDeathTypeFlagsCustom,				NULL, offsetof(DieMuxData, m_deathTypesCustom) },
		{ "VeterancyLevels",	INI::parseVeterancyLevelFlags,			NULL, offsetof( DieMuxData, m_veterancyLevels ) },
		{ "ExemptStatus",			ObjectStatusMaskType::parseFromINI,	NULL,	offsetof( DieMuxData, m_exemptStatus ) },
		{ "RequiredStatus",		ObjectStatusMaskType::parseFromINI, NULL,	offsetof( DieMuxData, m_requiredStatus ) },
		{ "RequiredCustomStatus",	INI::parseAsciiStringVector, NULL,	offsetof( DieMuxData, m_requiredCustomStatus ) },
		{ "CustomDeathTypes",		INI::parseCustomTypes,			NULL, offsetof( DieMuxData, m_customDeathTypes ) },
		{ 0, 0, 0, 0 }
	};
  return dataFieldParse;
}

//-------------------------------------------------------------------------------------------------
Bool DieMuxData::isDieApplicable(const Object* obj, const DamageInfo *damageInfo) const
{
	// wrong death type? punt
	if(damageInfo->in.m_customDeathType.isEmpty())
	{
		if (!getDeathTypeFlag(m_deathTypesCustom.first, damageInfo->in.m_deathType))
			return false;
	}
	else
	{
		if(!getCustomTypeFlag(m_deathTypesCustom.second, m_customDeathTypes, damageInfo->in.m_customDeathType))
			return false;
	}

	// wrong vet level? punt
	if (!getVeterancyLevelFlag(m_veterancyLevels, obj->getVeterancyLevel()))
		return false;

	// all 'exempt' bits must be clear for us to run.
	if( m_exemptStatus.any() && obj->getStatusBits().testForAny( m_exemptStatus ) )
		return false;

	// all 'required' bits must be set for us to run.
	// But only if we have a required status to check
	if( m_requiredStatus.any()  &&  !obj->getStatusBits().testForAll( m_requiredStatus ) )
		return false;

	for(std::vector<AsciiString>::const_iterator it = m_requiredCustomStatus.begin(); it != m_requiredCustomStatus.end(); ++it)
	{
		ObjectCustomStatusType::const_iterator it2 = obj->getCustomStatus().find(*it);
		if (it2 != obj->getCustomStatus().end()) 
		{
			if(it2->second == 0)
				return FALSE;
		}
		else
		{
			return FALSE;
		}
	}

	return true;
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void DieModule::crc( Xfer *xfer )
{

	// extend base class
	BehaviorModule::crc( xfer );

}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer Method */
// ------------------------------------------------------------------------------------------------
void DieModule::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// call base class
	BehaviorModule::xfer( xfer );

}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void DieModule::loadPostProcess( void )
{

	// call base class
	BehaviorModule::loadPostProcess();

}  // end loadPostProcess

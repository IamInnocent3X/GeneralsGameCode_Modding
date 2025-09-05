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

// FILE: MaxHealthUpgrade.cpp /////////////////////////////////////////////////////////////////////////////
// Author: Kris Morness, September 2002
// Desc:	 UpgradeModule that adds a scalar to the object's experience gain.
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file int the GameEngine

#define DEFINE_MAXHEALTHCHANGETYPE_NAMES
#include "Common/Player.h"
#include "Common/Xfer.h"
#include "GameLogic/Object.h"
#include "GameLogic/ExperienceTracker.h"
#include "GameLogic/Module/MaxHealthUpgrade.h"
#include "GameLogic/Module/BodyModule.h"
#include "GameLogic/Module/ContainModule.h"

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
MaxHealthUpgradeModuleData::MaxHealthUpgradeModuleData( void )
{
	m_addMaxHealth = 0.0f;
	m_multiplyMaxHealth = 1.0f;
	m_maxHealthChangeType = SAME_CURRENTHEALTH;
	m_addSubdualCap = 0.0f;
	m_multiplySubdualCap = 0.0f;
	m_addSubdualHealRate = 0;
	m_multiplySubdualHealRate = 0.0f;
	m_addSubdualHealAmount = 0.0f;
	m_multiplySubdualHealAmount = 0.0f;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void MaxHealthUpgradeModuleData::buildFieldParse(MultiIniFieldParse& p)
{

  UpgradeModuleData::buildFieldParse( p );

	static const FieldParse dataFieldParse[] =
	{
		{ "AddMaxHealth",					INI::parseReal,					NULL,										offsetof( MaxHealthUpgradeModuleData, m_addMaxHealth ) },
		{ "MultiplyMaxHealth",				INI::parseReal,					NULL,										offsetof( MaxHealthUpgradeModuleData, m_multiplyMaxHealth ) },
		{ "AddSubdualCap",					INI::parseReal,					NULL,										offsetof( MaxHealthUpgradeModuleData, m_addSubdualCap ) },
		{ "MultiplySubdualCap",				INI::parseReal,					NULL,										offsetof( MaxHealthUpgradeModuleData, m_multiplySubdualCap ) },
		{ "AddSubdualHealRate",				INI::parseReal,					NULL,										offsetof( MaxHealthUpgradeModuleData, m_addSubdualHealRate ) },
		{ "MultiplySubdualHealRate",		INI::parseReal,					NULL,										offsetof( MaxHealthUpgradeModuleData, m_multiplySubdualHealRate ) },
		{ "AddSubdualHealAmount",			INI::parseReal,					NULL,										offsetof( MaxHealthUpgradeModuleData, m_addSubdualHealAmount ) },
		{ "MultiplySubdualHealAmount",		INI::parseReal,					NULL,										offsetof( MaxHealthUpgradeModuleData, m_multiplySubdualHealAmount ) },
		{ "ChangeType",						INI::parseIndexList,		TheMaxHealthChangeTypeNames, offsetof( MaxHealthUpgradeModuleData, m_maxHealthChangeType ) },
		{ 0, 0, 0, 0 }
	};

  p.add(dataFieldParse);

}  // end buildFieldParse

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
MaxHealthUpgrade::MaxHealthUpgrade( Thing *thing, const ModuleData* moduleData ) : UpgradeModule( thing, moduleData )
{
	m_hasExecuted = FALSE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
MaxHealthUpgrade::~MaxHealthUpgrade( void )
{
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void MaxHealthUpgrade::upgradeImplementation( )
{
	//Simply add the xp scalar to the xp tracker!
	Object *obj = getObject();

	UpgradeMaskType objectMask = obj->getObjectCompletedUpgradeMask();
	UpgradeMaskType playerMask = obj->getControllingPlayer()->getCompletedUpgradeMask();
	UpgradeMaskType maskToCheck = playerMask;
	maskToCheck.set( objectMask );

	//First make sure we have the right combination of upgrades
	Int UpgradeStatus = wouldRefreshUpgrade(maskToCheck, m_hasExecuted);

	// Because this module does things differently, we need to take a different approach
	if( UpgradeStatus != 1 )
	{
		// If we do not have the Upgrade, yet we have not executed, do nothing
		if(!m_hasExecuted)
		{
			return;
		}
		else
		{
			// Remove the Upgrade Execution Status so it is treated as activation again
			m_hasExecuted = false;
			setUpgradeExecuted(false);
		}
	}

	Bool isApply = UpgradeStatus == 1 ? TRUE : FALSE;

	if(isApply)
	{
		// If we have yet to do the Upgrade, proceed to do the Upgrade, but if we already have the Upgrade, don't do anything.
		if(!m_hasExecuted)
			m_hasExecuted = isApply;
		else
			return;
	}

	doMaxHealthUpgrade(m_hasExecuted);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void MaxHealthUpgrade::doMaxHealthUpgrade( Bool isAdd )
{
	const MaxHealthUpgradeModuleData *data = getMaxHealthUpgradeModuleData();
	BodyModuleInterface *body = getObject()->getBodyModule();

	if(!body)
		return;

	if(data->m_addMaxHealth > 0 || data->m_multiplyMaxHealth != 1.0f)
	{
		Real newVal;
		if(isAdd)
		{
			newVal = (body->getMaxHealth() * data->m_multiplyMaxHealth) + data->m_addMaxHealth;
		}
		else
		{
			newVal = (body->getMaxHealth() - data->m_addMaxHealth ) / data->m_multiplyMaxHealth; // You do the subtraction first before the division, thats the order
		}
		// DEBUG_LOG(("MaxHealthUpgrade::upgradeImplementation - newVal: (%f * %f) + %f = %f.\n",
		//  	body->getMaxHealth(), data->m_multiplyMaxHealth, data->m_addMaxHealth, newVal));
		body->setMaxHealth( newVal, data->m_maxHealthChangeType );
	}

	if(body->canBeSubdued() && (data->m_addSubdualCap > 0 || data->m_multiplySubdualCap != 1.0f))
	{
		Real newVal;
		if(isAdd)
		{
			newVal = (body->getSubdualDamageCap() * data->m_multiplySubdualCap) + data->m_addSubdualCap;
		}
		else
		{
			newVal = (body->getSubdualDamageCap() - data->m_addSubdualCap) / data->m_multiplySubdualCap; // You do the subtraction first before the division, thats the order
		}
		// DEBUG_LOG(("MaxHealthUpgrade::upgradeImplementation - newVal: (%f * %f) + %f = %f.\n",
		//  	body->getSubdualCap(), data->m_multiplySubdualCap, data->m_addSubdualCap, newVal));
		body->setSubdualCap( newVal );
	}

	if(data->m_addSubdualHealRate > 0 || data->m_multiplySubdualHealRate != 1.0f)
	{
		Real realVal = INT_TO_REAL((int)(body->getSubdualDamageHealRate()));
		Real newVal = realVal;
		if(isAdd)
		{
			newVal = ( realVal * data->m_multiplySubdualHealRate) + data->m_addSubdualHealRate;
		}
		else
		{
			newVal = ( realVal - data->m_addSubdualHealRate ) / data->m_multiplySubdualHealRate; // You do the subtraction first before the division, thats the order
		}
		// DEBUG_LOG(("MaxHealthUpgrade::upgradeImplementation - newVal: (%f * %f) + %f = %f.\n",
		//  	realVal, data->m_multiplySubdualHealRate, data->m_addSubdualHealRate, newVal));
		body->setSubdualHealRate( (UnsignedInt)(REAL_TO_INT(newVal)) );
	}

	if(data->m_addSubdualHealAmount > 0 || data->m_multiplySubdualHealAmount != 1.0f)
	{
		Real newVal;
		if(isAdd)
		{
			newVal = (body->getSubdualDamageHealAmount() * data->m_multiplySubdualHealAmount) + data->m_addSubdualHealAmount;
		}
		else
		{
			newVal = (body->getSubdualDamageHealAmount() - data->m_addSubdualHealAmount ) / data->m_multiplySubdualHealAmount; // You do the subtraction first before the division, thats the order
		}
		// DEBUG_LOG(("MaxHealthUpgrade::upgradeImplementation - newVal: (%f * %f) + %f = %f.\n",
		//  	body->getSubdualHealAmount(), data->m_multiplySubdualHealAmount, data->m_addSubdualHealAmount, newVal));
		body->setSubdualHealAmount( newVal );
	}
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void MaxHealthUpgrade::crc( Xfer *xfer )
{

	// extend base class
	UpgradeModule::crc( xfer );

}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void MaxHealthUpgrade::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	UpgradeModule::xfer( xfer );

	xfer->xferBool(&m_hasExecuted);

}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void MaxHealthUpgrade::loadPostProcess( void )
{

	// extend base class
	UpgradeModule::loadPostProcess();

}  // end loadPostProcess

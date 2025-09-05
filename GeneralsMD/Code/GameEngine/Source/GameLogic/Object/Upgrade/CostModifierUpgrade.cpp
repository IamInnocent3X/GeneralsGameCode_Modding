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

// FILE: CostModifierUpgrade.cpp /////////////////////////////////////////////////
//-----------------------------------------------------------------------------
//
//                       Electronic Arts Pacific.
//
//                       Confidential Information
//                Copyright (C) 2002 - All Rights Reserved
//
//-----------------------------------------------------------------------------
//
//	created:	Aug 2002
//
//	Filename: 	CostModifierUpgrade.cpp
//
//	author:		Chris Huybregts
//
//	purpose:	Upgrade that modifies the cost by a certain percentage
//
//-----------------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// SYSTEM INCLUDES ////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// USER INCLUDES //////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------
#include "PreRTS.h"	// This must go first in EVERY cpp file int the GameEngine

#include "Common/Player.h"
#include "Common/ThingTemplate.h"
#include "Common/Xfer.h"
#include "GameLogic/Module/CostModifierUpgrade.h"
#include "GameLogic/Object.h"
#include "Common/BitFlagsIO.h"
//-----------------------------------------------------------------------------
// DEFINES ////////////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// PUBLIC FUNCTIONS ///////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// PRIVATE FUNCTIONS //////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
CostModifierUpgradeModuleData::CostModifierUpgradeModuleData( void )
{

	m_kindOf = KINDOFMASK_NONE;
	m_percentage = 0;
	m_isOneShot = FALSE;
	m_stackingType = NO_STACKING;

}  // end CostModifierUpgradeModuleData

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
/* static */ void CostModifierUpgradeModuleData::buildFieldParse(MultiIniFieldParse& p)
{
	UpgradeModuleData::buildFieldParse( p );

	static const FieldParse dataFieldParse[] =
	{
		{ "EffectKindOf",		KindOfMaskType::parseFromINI, NULL, offsetof( CostModifierUpgradeModuleData, m_kindOf ) },
		{ "Percentage",			INI::parsePercentToReal, NULL, offsetof( CostModifierUpgradeModuleData, m_percentage ) },
		{ "IsOneShotUpgrade",		INI::parseBool, NULL, offsetof( CostModifierUpgradeModuleData, m_isOneShot) },
		{ "BonusStacksWith",		INI::parseIndexList, TheBonusStackingTypeNames, offsetof( CostModifierUpgradeModuleData, m_stackingType) },
		
		{ 0, 0, 0, 0 } 
	};
	p.add(dataFieldParse);

}  // end buildFieldParse

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
CostModifierUpgrade::CostModifierUpgrade( Thing *thing, const ModuleData* moduleData ) :
							UpgradeModule( thing, moduleData )
{
	m_hasExecuted = FALSE;
}  // end CostModifierUpgrade

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
CostModifierUpgrade::~CostModifierUpgrade( void )
{

}  // end ~CostModifierUpgrade

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void CostModifierUpgrade::onDelete( void )
{
	const CostModifierUpgradeModuleData* d = getCostModifierUpgradeModuleData();
	
	// This is a global one time upgrade. Don't remove it.
	if (d->m_isOneShot)
		return;

	// if we haven't been upgraded there is nothing to clean up
	if( isAlreadyUpgraded() == FALSE )
		return;

	doCostModifier(FALSE);

}  // end onDelete

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void CostModifierUpgrade::onCapture( Player *oldOwner, Player *newOwner )
{
	const CostModifierUpgradeModuleData* d = getCostModifierUpgradeModuleData();

	// This is a global one time upgrade. Don't remove or transfer it.
	if (d->m_isOneShot)
		return;

	// do nothing if we haven't upgraded yet
	if( isAlreadyUpgraded() == FALSE )
		return;

	// remove bonus from old player and add to new player
	Bool stackUniqueType = d->m_stackingType == OTHER_TYPE;
	Bool stackWithAny = d->m_stackingType == SAME_TYPE;


	if( oldOwner )
	{
		oldOwner->removeKindOfProductionCostChange(d->m_kindOf, d->m_percentage,
			getObject()->getTemplate()->getTemplateID(), stackUniqueType, stackWithAny);

		setUpgradeExecuted(FALSE);

	}  // end if
	if( newOwner )
	{
		newOwner->addKindOfProductionCostChange(d->m_kindOf, d->m_percentage,
			getObject()->getTemplate()->getTemplateID(), stackUniqueType, stackWithAny);

		setUpgradeExecuted(TRUE);

	}  // end if

}  // end onCapture

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void CostModifierUpgrade::upgradeImplementation( void )
{
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
			m_hasExecuted = true;
		else
			return;
	}

	doCostModifier(m_hasExecuted);

}  // end upgradeImplementation


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void CostModifierUpgrade::doCostModifier(Bool isAdd)
{
	const CostModifierUpgradeModuleData* d = getCostModifierUpgradeModuleData();

	Bool stackWithAny = d->m_stackingType == SAME_TYPE;
	Bool stackUniqueType = d->m_stackingType == OTHER_TYPE;

	Player *player = getObject()->getControllingPlayer();
	if (player) {
		if(isAdd) {
			player->addKindOfProductionCostChange(d->m_kindOf, d->m_percentage,
				getObject()->getTemplate()->getTemplateID(), stackUniqueType, stackWithAny);
		} else {
			player->removeKindOfProductionCostChange(d->m_kindOf, d->m_percentage,
				getObject()->getTemplate()->getTemplateID(), stackUniqueType, stackWithAny);
		}
	}

	if(!isAdd)
	{
		m_hasExecuted = FALSE;
		// this upgrade module is now "not upgraded"
		setUpgradeExecuted(FALSE);
	}
}
// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void CostModifierUpgrade::crc( Xfer *xfer )
{

	// extend base class
	UpgradeModule::crc( xfer );

}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void CostModifierUpgrade::xfer( Xfer *xfer )
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
void CostModifierUpgrade::loadPostProcess( void )
{

	// extend base class
	UpgradeModule::loadPostProcess();

}  // end loadPostProcess

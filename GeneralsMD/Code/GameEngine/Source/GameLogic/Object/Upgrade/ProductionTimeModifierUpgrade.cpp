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

// FILE: ProductionTimeModifierUpgrade.cpp /////////////////////////////////////
//-----------------------------------------------------------------------------
//                                                                          
//                       Electronic Arts Pacific.                          
//                                                                          
//                       Confidential Information                           
//                Copyright (C) 2002 - All Rights Reserved                  
//                                                                          
//-----------------------------------------------------------------------------
//
//	created:	March 2025
//
//	Filename: 	ProductionTimeModifierUpgrade.cpp
//
//	author:		Andi W
//	
//	purpose:	Upgrade that modifies the production time by a certain percentage
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

#include "Common/player.h"
#include "Common/ThingTemplate.h"
#include "Common/Xfer.h"
#include "GameLogic/Module/ProductionTimeModifierUpgrade.h"
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
ProductionTimeModifierUpgradeModuleData::ProductionTimeModifierUpgradeModuleData( void )
{

	m_kindOf = KINDOFMASK_NONE;
	m_percentage = 0;
	m_isOneShot = FALSE;
	m_stackingType = NO_STACKING;

}  // end ProductionTimeModifierUpgradeModuleData

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
/* static */ void ProductionTimeModifierUpgradeModuleData::buildFieldParse(MultiIniFieldParse& p)
{
	UpgradeModuleData::buildFieldParse( p );

	static const FieldParse dataFieldParse[] = 
	{
		{ "EffectKindOf",		KindOfMaskType::parseFromINI, NULL, offsetof(ProductionTimeModifierUpgradeModuleData, m_kindOf ) },
		{ "Percentage",			INI::parsePercentToReal, NULL, offsetof(ProductionTimeModifierUpgradeModuleData, m_percentage ) },
		{ "IsOneShotUpgrade",		INI::parseBool, NULL, offsetof(ProductionTimeModifierUpgradeModuleData, m_isOneShot) },
		{ "BonusStacksWith",		INI::parseIndexList, TheBonusStackingTypeNames, offsetof(ProductionTimeModifierUpgradeModuleData, m_stackingType) },
		{ 0, 0, 0, 0 } 
	};
	p.add(dataFieldParse);

}  // end buildFieldParse

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
ProductionTimeModifierUpgrade::ProductionTimeModifierUpgrade( Thing *thing, const ModuleData* moduleData ) :
							UpgradeModule( thing, moduleData )
{
	m_hasExecuted = FALSE;
}  // end ProductionTimeModifierUpgrade

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
ProductionTimeModifierUpgrade::~ProductionTimeModifierUpgrade( void )
{

}  // end ~ProductionTimeModifierUpgrade

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void ProductionTimeModifierUpgrade::onDelete( void )
{
	const ProductionTimeModifierUpgradeModuleData* d = getProductionTimeModifierUpgradeModuleData();

	// This is a global one time upgrade. Don't remove it.
	if (d->m_isOneShot)
		return;

	// if we haven't been upgraded there is nothing to clean up
	if( isAlreadyUpgraded() == FALSE )
		return;

	doProductionModifierRemoval();

}  // end onDelete

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void ProductionTimeModifierUpgrade::onCapture( Player *oldOwner, Player *newOwner )
{

	// do nothing if we haven't upgraded yet
	if( isAlreadyUpgraded() == FALSE )
		return;

	// remove radar from old player and add to new player
	if( oldOwner )
	{

		oldOwner->removeKindOfProductionTimeChange(getProductionTimeModifierUpgradeModuleData()->m_kindOf, getProductionTimeModifierUpgradeModuleData()->m_percentage );
		setUpgradeExecuted(FALSE);

	}  // end if
	if( newOwner )
	{

		newOwner->addKindOfProductionTimeChange(getProductionTimeModifierUpgradeModuleData()->m_kindOf, getProductionTimeModifierUpgradeModuleData()->m_percentage );
		setUpgradeExecuted(TRUE);

	}  // end if

}  // end onCapture

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void ProductionTimeModifierUpgrade::upgradeImplementation( void )
{
	Object *obj = getObject();
	Player *player = getObject()->getControllingPlayer();

	UpgradeMaskType objectMask = obj->getObjectCompletedUpgradeMask();
	UpgradeMaskType playerMask = player->getCompletedUpgradeMask();
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

	// If there's no Upgrade Status, do Nothing;
	if( m_hasExecuted )
	{
		// update the player with another TypeOfProductionTimeChange
		player->addKindOfProductionTimeChange(getProductionTimeModifierUpgradeModuleData()->m_kindOf, getProductionTimeModifierUpgradeModuleData()->m_percentage );
	}
	else
	{
		doProductionModifierRemoval();
	}

}  // end upgradeImplementation

void ProductionTimeModifierUpgrade::doProductionModifierRemoval()
{
	const ProductionTimeModifierUpgradeModuleData* d = getProductionTimeModifierUpgradeModuleData();
	
	Bool stackWithAny = d->m_stackingType == SAME_TYPE;
	Bool stackUniqueType = d->m_stackingType == OTHER_TYPE;

	// remove the radar from the player
	Player* player = getObject()->getControllingPlayer();
	if (player) {
		player->removeKindOfProductionTimeChange(d->m_kindOf, d->m_percentage,
			getObject()->getTemplate()->getTemplateID(), stackUniqueType, stackWithAny);
	}

	// this upgrade module is now "not upgraded"
	m_hasExecuted = FALSE;
	setUpgradeExecuted(FALSE);
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void ProductionTimeModifierUpgrade::crc( Xfer *xfer )
{

	// extend base class
	UpgradeModule::crc( xfer );

}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void ProductionTimeModifierUpgrade::xfer( Xfer *xfer )
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
void ProductionTimeModifierUpgrade::loadPostProcess( void )
{

	// extend base class
	UpgradeModule::loadPostProcess();

}  // end loadPostProcess

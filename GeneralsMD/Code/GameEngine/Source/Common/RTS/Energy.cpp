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

// FILE: Energy.cpp /////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------
//
//                       Westwood Studios Pacific.
//
//                       Confidential Information
//                Copyright (C) 2001 - All Rights Reserved
//
//-----------------------------------------------------------------------------
//
// Project:   RTS3
//
// File name: Energy.cpp
//
// Created:   Steven Johnson, October 2001
//
// Desc:      @todo
//
//-----------------------------------------------------------------------------

#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "Common/Energy.h"
#include "Common/Player.h"
#include "Common/PlayerList.h"
#include "Common/ThingTemplate.h"
#include "Common/Xfer.h"

#include "GameLogic/GameLogic.h"
#include "GameLogic/Object.h"
#include "GameLogic/Module/BehaviorModule.h"
#include "GameLogic/Module/PowerPlantUpgrade.h"
#include "GameLogic/Module/OverchargeBehavior.h"

const Int DEFAULT_STOLEN_CONSTANT = UINT_MAX - 10;

typedef std::pair<Int, ObjectID> PowerLossObjectData;

//-----------------------------------------------------------------------------
Energy::Energy()
{
	m_energyProduction = 0;
	m_energyConsumption = 0;
	m_owner = nullptr;
	m_powerSabotagedTillFrame = 0;
	m_checkEnergyGivenTime = 0;
	m_energyBonus = 0;
	m_powerSabotageData.clear();
	m_energyTransferActiveReceivedPlayers.clear();
	m_energyGivenTo.clear();

}

//-----------------------------------------------------------------------------
Int Energy::getProduction() const
{
	if( TheGameLogic->getFrame() < m_powerSabotagedTillFrame )
	{
		//Power sabotaged, therefore no power.
		return 0;
	}
	return getTotalPower();
}

//-----------------------------------------------------------------------------
Real Energy::getEnergySupplyRatio() const
{
	DEBUG_ASSERTCRASH(m_energyProduction >= 0 && m_energyConsumption >= 0, ("neg Energy numbers"));

	if( TheGameLogic->getFrame() < m_powerSabotagedTillFrame )
	{
		//Power sabotaged, therefore no power, no ratio.
		return 0.0f;
	}

	if (m_energyConsumption == 0)
		return (Real)(getTotalPower());

	return (Real)(getTotalPower()) / (Real)m_energyConsumption;
}

//-------------------------------------------------------------------------------------------------
Bool Energy::hasSufficientPower(void) const
{
	if( TheGameLogic->getFrame() < m_powerSabotagedTillFrame )
	{
		//Power sabotaged, therefore no power.
		return FALSE;
	}
	return getTotalPower() >= m_energyConsumption;
}

//-------------------------------------------------------------------------------
static void getMaxProduction( Object *obj, void *userData )
{
	if( obj && !obj->isEffectivelyDead() && !obj->isDestroyed() && !obj->testStatus(OBJECT_STATUS_UNDER_CONSTRUCTION) )
	{
		Int maxEnergy = obj->getTemplate()->getEnergyProduction();

		static NameKeyType powerPlant = NAMEKEY("PowerPlantUpgrade");
		static NameKeyType overCharge = NAMEKEY("OverchargeBehavior");

		for (BehaviorModule** b = obj->getBehaviorModules(); *b; ++b)
		{
			if ((*b)->getModuleNameKey() == powerPlant)
			{
				PowerPlantUpgrade *powerPlantMod = (PowerPlantUpgrade*) *b;
				if (powerPlantMod->isAlreadyUpgraded()) {
					maxEnergy += obj->getTemplate()->getEnergyBonus();
				}
			}
			if ((*b)->getModuleNameKey() == overCharge)
			{
				OverchargeBehavior *overChargeMod = (OverchargeBehavior*) *b;
				if (overChargeMod->isOverchargeActive()) {
					maxEnergy += obj->getTemplate()->getEnergyBonus();
				}
			}
		}

		if(maxEnergy>0)
			*(Int*)userData += maxEnergy;
	}
}

//-------------------------------------------------------------------------------------------------
void Energy::adjustPower(Int powerDelta, Bool adding)
{
	if (powerDelta == 0) {
		return;
	}

	if (powerDelta > 0) {
		if (adding) {
			addProduction(powerDelta);
		} else {
			addProduction(-powerDelta);
		}
	} else {
		// Seems a little odd, however, consumption is reversed. Negative power is positive consumption.
		if (adding) {
			addConsumption(-powerDelta);
		} else {
			addConsumption(powerDelta);
		}
	}
}

//-------------------------------------------------------------------------------------------------
/** new 'obj' will now add/subtract from this energy construct */
//-------------------------------------------------------------------------------------------------
void Energy::objectEnteringInfluence( Object *obj )
{

	// sanity
	if( obj == nullptr )
		return;

	// get the amount of energy this object produces or consumes
	Int energy = obj->getTemplate()->getEnergyProduction();

	// adjust energy
	if( energy < 0 )
		addConsumption( -energy );
	else if( energy > 0 )
		addProduction( energy );

	// sanity
	DEBUG_ASSERTCRASH( m_energyProduction >= 0 && m_energyConsumption >= 0,
										 ("Energy - Negative Energy numbers, Produce=%d Consume=%d\n",
										 m_energyProduction, m_energyConsumption) );

}

//-------------------------------------------------------------------------------------------------
/** 'obj' will now no longer add/subtrack from this energy construct */
//-------------------------------------------------------------------------------------------------
void Energy::objectLeavingInfluence( Object *obj )
{

	// sanity
	if( obj == nullptr )
		return;

	// get the amount of energy this object produces or consumes
	Int energy = obj->getTemplate()->getEnergyProduction();

	// adjust energy
	if( energy < 0 )
		addConsumption( energy );
	else if( energy > 0 )
		addProduction( -energy );

	// sanity
	DEBUG_ASSERTCRASH( m_energyProduction >= 0 && m_energyConsumption >= 0,
										 ("Energy - Negative Energy numbers, Produce=%d Consume=%d\n",
										 m_energyProduction, m_energyConsumption) );

}

//-------------------------------------------------------------------------------------------------
/** Adds an energy bonus to the player's pool of energy when the "Control Rods" upgrade
		is made to the American Cold Fusion Plant */
//-------------------------------------------------------------------------------------------------
void Energy::addPowerBonus( Object *obj )
{

	// sanity
	if( obj == nullptr )
		return;

	addProduction(obj->getTemplate()->getEnergyBonus());

	// sanity
	DEBUG_ASSERTCRASH( m_energyProduction >= 0 && m_energyConsumption >= 0,
										 ("Energy - Negative Energy numbers, Produce=%d Consume=%d\n",
										 m_energyProduction, m_energyConsumption) );

}

// ------------------------------------------------------------------------------------------------
/** Removed an energy bonus */
// ------------------------------------------------------------------------------------------------
void Energy::removePowerBonus( Object *obj )
{

	// sanity
	if( obj == nullptr )
		return;

	// TheSuperHackers @bugfix Caball009 14/11/2025 Don't remove power bonus for disabled power plants.
#if !RETAIL_COMPATIBLE_CRC
	if ( obj->isDisabled() )
		return;
#endif

	addProduction( -obj->getTemplate()->getEnergyBonus() );

	// sanity
	DEBUG_ASSERTCRASH( m_energyProduction >= 0 && m_energyConsumption >= 0,
										 ("Energy - Negative Energy numbers, Produce=%d Consume=%d\n",
										 m_energyProduction, m_energyConsumption) );

}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
// Private functions
// ------------------------------------------------------------------------------------------------
void Energy::addProduction(Int amt)
{
	m_energyProduction += amt;

	if( m_owner == nullptr )
		return;

	Int totalEnergy = 0;
	m_owner->iterateObjects( getMaxProduction, &totalEnergy );

	m_energyProduced = totalEnergy;

	// A repeated Brownout signal does nothing bad, and we need to handle more than just edge cases.
	// Like low power, now even more low power, refresh disable.
	m_owner->onPowerBrownOutChange( !hasSufficientPower() );
}

// ------------------------------------------------------------------------------------------------
void Energy::addConsumption(Int amt)
{
	m_energyConsumption += amt;

	if( m_owner == nullptr )
		return;

	m_owner->onPowerBrownOutChange( !hasSufficientPower() );
}

// ------------------------------------------------------------------------------------------------
void Energy::setPowerSabotagedTillFrame( UnsignedInt frame, Int Amount, Real Percent )
{
	if(Amount != 0 || Percent != 0.0f)
	{
		SabotageData sabotage;
		sabotage.Frame = frame;
		sabotage.Amount = Amount;
		sabotage.Percent = Percent;
		//sabotage.SpecificID = SpecificID;
		//sabotage.MaxEnergy = MaxEnergy;
		m_powerSabotageData.push_back(sabotage);

		m_checkEnergyGivenTime = 1; // Reset the timer
		calculateCurrentBonusEnergy();

		m_owner->onPowerBrownOutChange( !hasSufficientPower() );
	}
	else
	{
		m_powerSabotagedTillFrame = frame;
	}
}

// ------------------------------------------------------------------------------------------------
void Energy::setEnergyGivenTo( UnsignedInt frame, Int amount, Int playerIndex, ObjectID specificID, Int maxEnergy )
{
	Bool found = FALSE;

	// If power is stolen with its default constant, check its max energy
	Bool doDefaultStolen = amount == DEFAULT_STOLEN_CONSTANT;

	UnsignedInt now = TheGameLogic->getFrame();
	if(doDefaultStolen)
	{
		for(EnergyStolenVec::iterator it = m_energyGivenTo.begin(); it != m_energyGivenTo.end();)
		{
			if(now >= it->Frame)
			{
				it = m_energyGivenTo.erase( it );
				continue;
			}
			if(it->PlayerIndex == playerIndex && it->Amount == DEFAULT_STOLEN_CONSTANT)
			{
				found = TRUE;
				it->Frame = frame;
				it->MaxEnergy = m_energyProduced;
				break;
			}
			++it;
		}
	}

	if(!found)
	{
		EnergyStolenData energyGiven;
		energyGiven.Frame = frame;
		energyGiven.Amount = amount;
		energyGiven.SpecificID = specificID;
		energyGiven.PlayerIndex = playerIndex;
		energyGiven.MaxEnergy = maxEnergy > 0 ? maxEnergy : m_energyProduced;
		m_energyGivenTo.push_back(energyGiven);
	}
}

// ------------------------------------------------------------------------------------------------
void Energy::setEnergyReceivedFrom( Int playerIndex )
{
	Bool registered = FALSE;
	for(std::vector<Int>::iterator it = m_energyTransferActiveReceivedPlayers.begin(); it != m_energyTransferActiveReceivedPlayers.end(); ++it)
	{
		if((*it) == playerIndex)
		{
			registered = TRUE;
			break;
		}
	}
	if(!registered)
		m_energyTransferActiveReceivedPlayers.push_back(playerIndex);

	m_checkEnergyGivenTime = 1; // Reset the timer
	calculateCurrentBonusEnergy();

	m_owner->onPowerBrownOutChange( !hasSufficientPower() );
}

// ------------------------------------------------------------------------------------------------
Int Energy::calculateMaxEnergyFromOtherPlayers()
{
	Int EnergyReceived = 0;
	Int OwningPlayer = m_owner->getPlayerIndex();

	for(std::vector<Int>::iterator it = m_energyTransferActiveReceivedPlayers.begin(); it != m_energyTransferActiveReceivedPlayers.end();)
	{
		Player *currentPlayer = ThePlayerList->getNthPlayer(*it);
		Int currentEnergy = currentPlayer->getEnergy()->calculateEnergyGivenToPlayer(OwningPlayer, m_checkEnergyGivenTime);
		if(currentEnergy == 0)
		{
			it = m_energyTransferActiveReceivedPlayers.erase(it);
			continue;
		}

		EnergyReceived += currentEnergy;
		++it;
	}

	return EnergyReceived;
}

// ------------------------------------------------------------------------------------------------
Int Energy::calculateEnergyGivenToPlayer(Int PlayerIndex, UnsignedInt &checkEnergyTime)
{
	UnsignedInt now = TheGameLogic->getFrame();
	Int MaxEnergy = m_energyProduced;
	Int totalEnergyGiven = 0;
	Int totalEnergyReceived = 0;
	std::vector<PowerLossObjectData> powerLossDataVec;
	std::vector<PowerLossObjectData>::iterator it_obj;
	Bool done = FALSE;

	for(EnergyStolenVec::iterator it = m_energyGivenTo.begin(); it != m_energyGivenTo.end();)
	{
		if(now >= it->Frame)
		{
			it = m_energyGivenTo.erase( it );
			continue;
		}
		else if(it->PlayerIndex == PlayerIndex && (!checkEnergyTime || checkEnergyTime > it->Frame))
		{
			checkEnergyTime = it->Frame;
		}

		// Check all the timers 
		if(done)
		{
			++it;
			continue;
		}

		Int grantEnergy = it->Amount;
		if(grantEnergy == DEFAULT_STOLEN_CONSTANT)
			grantEnergy = it->MaxEnergy;	// Get the stolen amount on that time

		Int currentEnergyGiven = 0;
		Int currentTotalEnergyGiven = totalEnergyGiven;
		Int currentMaxEnergy = MaxEnergy;
		if(it->SpecificID != INVALID_ID)
		{
			currentTotalEnergyGiven = 0;
			currentMaxEnergy = it->MaxEnergy;
			for(it_obj = powerLossDataVec.begin(); it_obj != powerLossDataVec.end(); ++it_obj)
			{
				if(it->SpecificID == it_obj->second)
				{
					currentTotalEnergyGiven = it_obj->first;
					break;
				}
			}
		}

		if( currentTotalEnergyGiven < currentMaxEnergy && totalEnergyGiven < MaxEnergy )
		{
			// For the current Obj or Instance
			if( currentTotalEnergyGiven + grantEnergy >= currentMaxEnergy )
			{
				currentEnergyGiven = currentMaxEnergy - currentTotalEnergyGiven;
				//currentTotalEnergyGiven = currentMaxEnergy;
			}
			else
			{
				currentEnergyGiven = grantEnergy;
				currentTotalEnergyGiven += grantEnergy;
			}

			// Calculate if the energy given exceeds the total player Energy
			if( totalEnergyGiven + currentEnergyGiven >= MaxEnergy )
			{
				currentEnergyGiven = MaxEnergy - totalEnergyGiven;
				totalEnergyGiven = MaxEnergy;
				done = TRUE;
			}
			else
			{
				totalEnergyGiven += currentEnergyGiven;
			}

			if(!done && it->SpecificID != INVALID_ID)
			{
				Bool foundObj = FALSE;
				for(it_obj = powerLossDataVec.begin(); it_obj != powerLossDataVec.end(); ++it_obj)
				{
					if(it->SpecificID == it_obj->second)
					{
						foundObj = TRUE;
						it_obj->first += currentEnergyGiven;
						break;
					}
				}
				if(!foundObj)
				{
					PowerLossObjectData loss;
					loss.first = currentEnergyGiven;
					loss.second = it->SpecificID;
					powerLossDataVec.push_back(loss);
				}
			}

			if(PlayerIndex == it->PlayerIndex)
			{
				totalEnergyReceived += currentEnergyGiven;
			}
		}
		++it;
	}

	return totalEnergyReceived;
}

//struct PowerLossPlayerData
//{
//	Int Amount;
//	Int MaxEnergy;
//	Int PlayerIndex;
//};

// ------------------------------------------------------------------------------------------------
void Energy::calculateCurrentBonusEnergy()
{
	UnsignedInt now = TheGameLogic->getFrame();
	if(!m_checkEnergyGivenTime || m_checkEnergyGivenTime > now)
		return;

	m_checkEnergyGivenTime = 0;
	m_energyBonus = calculateMaxEnergyFromOtherPlayers();
	if(m_powerSabotageData.empty())
		return;

	Int totalLossAmount = -m_energyBonus;
	Int totalLossPercent = 0.0f;

	std::vector<PowerLossObjectData> powerLossDataVec;
	std::vector<PowerLossObjectData>::iterator it_obj;
	for(SabotageVec::iterator it = m_powerSabotageData.begin(); it != m_powerSabotageData.end();)
	{
		if(now >= it->Frame)
		{
			it = m_powerSabotageData.erase( it );
			continue;
		}
		else if(!m_checkEnergyGivenTime || m_checkEnergyGivenTime > it->Frame)
		{
			m_checkEnergyGivenTime = it->Frame;
		}

		//Bool found  = FALSE;
		//Int lossAmount = 0;
		//Int MaxEnergy = it->MaxEnergy; // Get the Max Energy during the time it has sabotaged

		// Already sabotaged with Power Outrage
		/*if(it->SpecificID != INVALID_ID)
		{
			Int lossAmount = it->Amount;
			Int currentEnergyLoss = 0;
			Int currentTotalEnergyLoss = 0;
			Int currentMaxEnergy = it->MaxEnergy;
			for(it_obj = powerLossDataVec.begin(); it_obj != powerLossDataVec.end(); ++it_obj)
			{
				if(it->SpecificID == it_obj->second)
				{
					currentTotalEnergyLoss = it_obj->first;
					break;
				}
			}

			if( currentTotalEnergyLoss < currentMaxEnergy )
			{
				// For the current Obj or Instance
				if( currentTotalEnergyLoss + lossAmount >= currentMaxEnergy )
				{
					currentEnergyLoss = currentMaxEnergy - currentTotalEnergyLoss;
				}
				else
				{
					currentEnergyLoss = lossAmount;
					currentTotalEnergyLoss += lossAmount;
				}

				Bool foundObj = FALSE;
				for(it_obj = powerLossDataVec.begin(); it_obj != powerLossDataVec.end(); ++it_obj)
				{
					if(it->SpecificID == it_obj->second)
					{
						foundObj = TRUE;
						it_obj->first += currentEnergyLoss;
						break;
					}
				}
				if(!foundObj)
				{
					PowerLossObjectData loss;
					loss.first = currentEnergyLoss;
					loss.second = it->SpecificID;
					powerLossDataVec.push_back(loss);
				}

				totalLossAmount += currentEnergyLoss;
			}

			//for(it_oe = ObjPowerLossVec.begin(); it_oe != ObjPowerLossVec.end(); ++it_oe)
			//{
			//	if(it_oe->SpecificID == it->SpecificID)
			//	{
			//		found = TRUE;
			//		lossAmount = it->Amount;
			//		it_oe->Amount += lossAmount;
			//		it_oe->MaxEnergy = MaxEnergy;
			//	}
			//}
			//if(!found)
			//{
			//	lossAmount = it->Amount;

			//	PowerLossObjData data;
			//	data.Amount = lossAmount;
			//	data.MaxEnergy = it->MaxEnergy;
			//	data.SpecificID = it->SpecificID;
			//	ObjPowerLossVec.push_back(data);
			//}
		}*/
		/*else if(it->PlayerIndex >= 0)
		{
			lossAmount = it->Amount + REAL_TO_INT(it->Percent * MaxEnergy);

			for(it_pe = PlayerPowerLossVec.begin(); it_pe != PlayerPowerLossVec.end(); ++it_pe)
			{
				if(it_pe->PlayerIndex == it->PlayerIndex)
				{
					found = TRUE;
					it_pe->Amount += lossAmount;
					it_pe->MaxEnergy = MaxEnergy;
				}
			}
			if(!found)
			{
				PowerLossPlayerData data;
				data.Amount = lossAmount;
				data.MaxEnergy = it->MaxEnergy;
				data.PlayerIndex = it->PlayerIndex;
				PlayerPowerLossVec.push_back(data);
			}
		}*/
		//else
		//{
			totalLossAmount += it->Amount;
			totalLossPercent += it->Percent;
		//}
		++it;
	}

	//for(it_oe = ObjPowerLossVec.begin(); it_oe != ObjPowerLossVec.end(); ++it_oe)
	//{
		// Clamp it according to the max power able to steal
		//totalLossAmount += clamp(-it_oe->MaxEnergy, it_oe->Amount, it_oe->MaxEnergy);
	//}
	//for(it_pe = PlayerPowerLossVec.begin(); it_pe != PlayerPowerLossVec.end(); ++it_pe)
	//{
		// Clamp it according to the max power able to steal
		//totalLossAmount += clamp(-it_pe->MaxEnergy, it_pe->Amount, it_pe->MaxEnergy);
	//}

	totalLossAmount += REAL_TO_INT(m_energyProduction * totalLossPercent);
	totalLossAmount = min(totalLossAmount, m_energyProduction);
	m_energyBonus = -totalLossAmount;
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void Energy::crc( Xfer *xfer )
{

}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void Energy::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 3;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// It is actually incorrect to save these, as they are reconstructed when the buildings are loaded
	// I need to version though so old games will load wrong rather than crashing

	// production
	if( version < 2 )
		xfer->xferInt( &m_energyProduction );

	// produced
	if( version < 2 )
		xfer->xferInt( &m_energyProduced );

	// consumption
	if( version < 2 )
		xfer->xferInt( &m_energyConsumption );

	// bonus
	xfer->xferInt( &m_energyBonus );

	// owning player index
	Int owningPlayerIndex;
	if( xfer->getXferMode() == XFER_SAVE )
		owningPlayerIndex = m_owner->getPlayerIndex();
	xfer->xferInt( &owningPlayerIndex );
	m_owner = ThePlayerList->getNthPlayer( owningPlayerIndex );

	//Sabotage
	if( version >= 3 )
	{
		xfer->xferUnsignedInt( &m_powerSabotagedTillFrame );
		
		xfer->xferUnsignedInt( &m_checkEnergyGivenTime );

		UnsignedShort sabotageCount = m_powerSabotageData.size();
		xfer->xferUnsignedShort( &sabotageCount );
		UnsignedInt sabotageFrame;
		Int sabotageAmount;
		//Int sabotageMaxEnergy;
		//Int sabotagePlayerIndex;
		Real sabotagePercent;
		//ObjectID sabotageSpecificID;
		if( xfer->getXferMode() == XFER_SAVE )
		{

			// go through all IDs
			for (int i = 0; i < sabotageCount; i++)
			{
				sabotageFrame = m_powerSabotageData[i].Frame;
				xfer->xferUnsignedInt( &sabotageFrame );

				sabotageAmount = m_powerSabotageData[i].Amount;
				xfer->xferInt( &sabotageAmount );

				//sabotageMaxEnergy = m_powerSabotageData[i].MaxEnergy;
				//xfer->xferInt( &sabotageMaxEnergy );

				//sabotagePlayerIndex = m_powerSabotageData[i].PlayerIndex;
				//xfer->xferInt( &sabotagePlayerIndex );

				sabotagePercent = m_powerSabotageData[i].Percent;
				xfer->xferReal( &sabotagePercent );

				//sabotageSpecificID = m_powerSabotageData[i].SpecificID;
				//xfer->xferObjectID( &sabotageSpecificID );
			}

		}
		else
		{
			// this list should be empty on loading
			if( m_powerSabotageData.size() != 0 )
			{

				DEBUG_CRASH(( "Energy::xfer - m_powerSabotageData should be empty but is not" ));

			}

			// read all IDs
			for( UnsignedShort i = 0; i < sabotageCount; ++i )
			{
				// read and register ID
				xfer->xferUnsignedInt( &sabotageFrame );
				xfer->xferInt( &sabotageAmount );
				//xfer->xferInt( &sabotageMaxEnergy );
				//xfer->xferInt( &sabotagePlayerIndex );
				xfer->xferReal( &sabotagePercent );
				//xfer->xferObjectID( &sabotageSpecificID );

				SabotageData data;
				data.Frame = sabotageFrame;
				data.Amount = sabotageAmount;
				//data.MaxEnergy = sabotageMaxEnergy;
				//data.PlayerIndex = sabotagePlayerIndex;
				data.Percent = sabotagePercent;
				//data.SpecificID = sabotageSpecificID;
				m_powerSabotageData.push_back(data);
			}

		}

		UnsignedShort energyReceivedPlayersCount = m_energyTransferActiveReceivedPlayers.size();
		xfer->xferUnsignedShort( &energyReceivedPlayersCount );
		Int indexReceived;
		if( xfer->getXferMode() == XFER_SAVE )
		{
			for(std::vector<Int>::const_iterator it_receive = m_energyTransferActiveReceivedPlayers.begin(); it_receive != m_energyTransferActiveReceivedPlayers.end(); ++it_receive)
			{
				indexReceived = (*it_receive);
				xfer->xferInt( &indexReceived );
			}
		}
		else
		{
			// this list should be empty on loading
			if( m_energyTransferActiveReceivedPlayers.size() != 0 )
			{

				DEBUG_CRASH(( "Energy::xfer - m_energyTransferActiveReceivedPlayers should be empty but they're not" ));

			}

			// read all IDs
			for( UnsignedShort i = 0; i < energyReceivedPlayersCount; ++i )
			{
				// read and register ID
				xfer->xferInt( &indexReceived );
				m_energyTransferActiveReceivedPlayers.push_back(indexReceived);
			}
		}

		UnsignedShort energyGivenToCount = m_energyGivenTo.size();
		xfer->xferUnsignedShort( &energyGivenToCount );
		UnsignedInt energyFrame;
		Int energyAmount;
		Int energyMaxEnergy;
		Int energyPlayerIndex;
		ObjectID energySpecificID;
		if( xfer->getXferMode() == XFER_SAVE )
		{

			// go through all IDs
			for (int j = 0; j < energyGivenToCount; j++)
			{
				energyFrame = m_energyGivenTo[j].Frame;
				xfer->xferUnsignedInt( &energyFrame );

				energyAmount = m_energyGivenTo[j].Amount;
				xfer->xferInt( &energyAmount );

				energyMaxEnergy = m_energyGivenTo[j].MaxEnergy;
				xfer->xferInt( &energyMaxEnergy );

				energyPlayerIndex = m_energyGivenTo[j].PlayerIndex;
				xfer->xferInt( &energyPlayerIndex );

				energySpecificID = m_energyGivenTo[j].SpecificID;
				xfer->xferObjectID( &energySpecificID );
			}

		}
		else
		{
			// this list should be empty on loading
			if( m_energyGivenTo.size() != 0 )
			{

				DEBUG_CRASH(( "Energy::xfer - m_energyGivenTo should be empty but is not" ));

			}

			// read all IDs
			for( UnsignedShort j = 0; j < energyGivenToCount; ++j )
			{
				// read and register ID
				xfer->xferUnsignedInt( &energyFrame );
				xfer->xferInt( &energyAmount );
				xfer->xferInt( &energyMaxEnergy );
				xfer->xferInt( &energyPlayerIndex );
				xfer->xferObjectID( &energySpecificID );

				EnergyStolenData data;
				data.Frame = energyFrame;
				data.Amount = energyAmount;
				data.MaxEnergy = energyMaxEnergy;
				data.PlayerIndex = energyPlayerIndex;
				data.SpecificID = energySpecificID;
				m_energyGivenTo.push_back(data);
			}

		}
	}

}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void Energy::loadPostProcess( void )
{

}

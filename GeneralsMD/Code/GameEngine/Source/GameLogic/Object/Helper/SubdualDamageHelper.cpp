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

// FILE: SubdualDamageHelper.h ////////////////////////////////////////////////////////////////////////
// Author: Graham Smallwood, June 2003
// Desc:   Object helper - Clears subdual status and heals subdual damage since Body modules can't have Updates
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"
#include "Common/Xfer.h"

#include "Common/DisabledTypes.h"
#include "GameClient/Drawable.h"
#include "GameLogic/Object.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/Module/BodyModule.h"
#include "GameLogic/Module/SubdualDamageHelper.h"

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
SubdualDamageHelper::SubdualDamageHelper( Thing *thing, const ModuleData *modData ) : ObjectHelper( thing, modData )
{
	m_healingStepCountdown = 0;
	m_healingStepFrame = 0;
	m_healingStepCountdownCustomMap.clear();
	m_nextHealingStepFrame = 0;

	setWakeFrame(getObject(), UPDATE_SLEEP_FOREVER);
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
SubdualDamageHelper::~SubdualDamageHelper( void )
{

}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
UpdateSleepTime SubdualDamageHelper::update()
{
	BodyModuleInterface *body = getObject()->getBodyModule();
	UnsignedInt now = TheGameLogic->getFrame();
	UnsignedInt nextHealingStep = 0; //= 1e9; //Just a portion shy away from UPDATE_SLEEP_FOREVER

	std::vector<AsciiString> subdualDamageCustom;
	subdualDamageCustom = body->getAnySubdualDamageCustom();
	//if( !subdualDamageCustom.empty() )
	//{
		for(int i = 0; i < subdualDamageCustom.size(); i++)
		{
			UnsignedInt cooldown = body->getSubdualDamageHealRateCustom(subdualDamageCustom[i]);
			
			CustomSubdualCurrentHealMap::iterator it = m_healingStepCountdownCustomMap.find(subdualDamageCustom[i]);
			if(it != m_healingStepCountdownCustomMap.end())
			{
				if(it->second.healFrame <= now)
				{
					SubdualCustomData HealData;
					HealData.damage = body->getSubdualDamageHealAmountCustom(subdualDamageCustom[i]);
					HealData.tintStatus = it->second.tintStatus;
					HealData.customTintStatus = it->second.customTintStatus;
					HealData.disableType = it->second.disableType;
					HealData.isSubdued = FALSE;
					
					body->internalAddSubdualDamageCustom(HealData, subdualDamageCustom[i], TRUE);
					it->second.healFrame = now + cooldown;
					//DEBUG_LOG(( "SubdualDamageHelper Custom Subdual Healed: %s, Current Frame: %d, Next Frame to Heal: %d", it->first.str(), now, it->second.healFrame ));
				}
				if ( !nextHealingStep || nextHealingStep > it->second.healFrame - now )
				{
					nextHealingStep = it->second.healFrame - now;
					//DEBUG_LOG(( "SubdualDamageHelper No Custom Subdual Healed For: %s, Current Frame: %d, Next Frame to Heal: %d", it->first.str(), now, it->second.healFrame ));
				}
			}
		}
	//}

	//if(body->hasAnySubdualDamageCustom() && m_healingStepCountdown <= 0)
	//	return UPDATE_SLEEP_NONE;

	//m_healingStepCountdown--;
	//if( m_healingStepCountdown > 0 )
	//	return UPDATE_SLEEP_NONE;
	if( body->hasAnySubdualDamage() )
	{
		m_healingStepCountdown = body->getSubdualDamageHealRate();

		if(m_healingStepFrame <= now)
		{
			//DamageInfo removeSubdueDamage;
			//removeSubdueDamage.in.m_damageType = DAMAGE_SUBDUAL_UNRESISTABLE;
			//removeSubdueDamage.in.m_amount = -body->getSubdualDamageHealAmount();
			//body->attemptDamage(&removeSubdueDamage);

			body->internalAddSubdualDamage(body->getSubdualDamageHealAmount(), TRUE);
			
			m_healingStepFrame = now + m_healingStepCountdown;
			//DEBUG_LOG(( "SubdualDamageHelper Subdual Healed, Current Frame: %d, Next Frame to Heal: %d", now, m_healingStepFrame ));
		}
		if ( !nextHealingStep || nextHealingStep > m_healingStepFrame - now )
		{
			nextHealingStep = m_healingStepFrame - now;
			//DEBUG_LOG(( "SubdualDamageHelper No Subdual Healed, Current Frame: %d, Next Frame to Heal: %d", now, m_healingStepFrame ));
		}

	}

	if(	nextHealingStep > 0 )
	{
		//DEBUG_LOG(( "SubdualDamageHelper Next Frame to Wake Up: %d", now ));
		m_nextHealingStepFrame = now + nextHealingStep;
		return UPDATE_SLEEP(nextHealingStep);
	}
	else
	{
		//DEBUG_LOG(( "SubdualDamageHelper No More Healing. Frame: %d", now ));
		m_nextHealingStepFrame = 0;
		return UPDATE_SLEEP_FOREVER;
	}
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void SubdualDamageHelper::notifySubdualDamage( Real amount )
{
	if( amount > 0 )
	{
		m_healingStepCountdown = getObject()->getBodyModule()->getSubdualDamageHealRate();
		//setWakeFrame(getObject(), UPDATE_SLEEP_NONE);
		UnsignedInt now = TheGameLogic->getFrame();
		m_healingStepFrame = now + m_healingStepCountdown;
		if(m_nextHealingStepFrame > m_healingStepFrame || !m_nextHealingStepFrame)
		{
			m_nextHealingStepFrame = m_healingStepFrame;
			setWakeFrame(getObject(), UPDATE_SLEEP(m_healingStepCountdown));
		}
		//DEBUG_LOG(( "SubdualDamageHelper Object Subdual'ed, Current Frame: %d, Next Frame to Heal: %d", now, m_healingStepFrame ));
	}
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void SubdualDamageHelper::notifySubdualDamageCustom( SubdualCustomData subdualData, const AsciiString& customStatus )
{
	if( subdualData.damage > 0 )
	{
		UnsignedInt now = TheGameLogic->getFrame();
		UnsignedInt cooldown = getObject()->getBodyModule()->getSubdualDamageHealRateCustom(customStatus);

		CustomSubdualCurrentHealMap::iterator it = m_healingStepCountdownCustomMap.find(customStatus);
		if(it != m_healingStepCountdownCustomMap.end())
		{
			it->second.healFrame = now + cooldown;
			it->second.tintStatus = subdualData.tintStatus;
			it->second.customTintStatus = subdualData.customTintStatus;
			it->second.disableType = subdualData.disableType;
		}
		else
		{
			SubdualCustomHealData HealData;
			HealData.healFrame = now + cooldown;
			HealData.tintStatus = subdualData.tintStatus;
			HealData.customTintStatus = subdualData.customTintStatus;
			HealData.disableType = subdualData.disableType;

			m_healingStepCountdownCustomMap[customStatus] = HealData;
		}

		if(m_nextHealingStepFrame > now + cooldown || !m_nextHealingStepFrame)
		{
			m_nextHealingStepFrame = now + cooldown;
			setWakeFrame(getObject(), UPDATE_SLEEP(cooldown));
		}
	}
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void SubdualDamageHelper::crc( Xfer *xfer )
{

	// object helper crc
	ObjectHelper::crc( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info;
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void SubdualDamageHelper::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// object helper base class
	ObjectHelper::xfer( xfer );

	xfer->xferUnsignedInt( &m_healingStepCountdown );

	xfer->xferUnsignedInt( &m_nextHealingStepFrame );

	xfer->xferUnsignedInt( &m_healingStepFrame );

	// Modified from Team.cpp
	CustomSubdualCurrentHealMap::iterator customSubdualIt;
	UnsignedShort customSubdualCount = m_healingStepCountdownCustomMap.size();
	xfer->xferUnsignedShort( &customSubdualCount );

	AsciiString customSubdualName;
	UnsignedInt customSubdualAmount;
	UnsignedInt customSubdualTint;
	AsciiString customSubdualCustomTint;
	UnsignedInt customSubdualDisableType;
	if( xfer->getXferMode() == XFER_SAVE )
	{

		for( customSubdualIt = m_healingStepCountdownCustomMap.begin(); customSubdualIt != m_healingStepCountdownCustomMap.end(); ++customSubdualIt )
		{

			customSubdualName = (*customSubdualIt).first;
			xfer->xferAsciiString( &customSubdualName );

			customSubdualAmount = (*customSubdualIt).second.healFrame;
			xfer->xferUnsignedInt( &customSubdualAmount );

			customSubdualTint = (*customSubdualIt).second.tintStatus;
			xfer->xferUnsignedInt( &customSubdualTint );

			customSubdualCustomTint = (*customSubdualIt).second.customTintStatus;
			xfer->xferAsciiString( &customSubdualCustomTint );

			customSubdualDisableType = (*customSubdualIt).second.disableType;
			xfer->xferUnsignedInt( &customSubdualDisableType );

		}

	}
	else
	{

		for( UnsignedShort i = 0; i < customSubdualCount; ++i )
		{

			xfer->xferAsciiString( &customSubdualName );

			xfer->xferUnsignedInt( &customSubdualAmount );

			xfer->xferUnsignedInt( &customSubdualTint );

			xfer->xferAsciiString( &customSubdualCustomTint );

			xfer->xferUnsignedInt( &customSubdualDisableType );

			SubdualCustomHealData data;
			data.healFrame = customSubdualAmount;
			data.tintStatus = (TintStatus)customSubdualTint;
			data.customTintStatus = customSubdualCustomTint;
			data.disableType = (DisabledType)customSubdualDisableType;

			m_healingStepCountdownCustomMap[customSubdualName] = data;
			
		}

	}

}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void SubdualDamageHelper::loadPostProcess( void )
{

	// object helper base class
	ObjectHelper::loadPostProcess();

}


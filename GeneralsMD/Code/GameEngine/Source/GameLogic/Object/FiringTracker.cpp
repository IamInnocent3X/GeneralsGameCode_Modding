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

// FILE: FiringTracker.cpp //////////////////////////////////////////////////////////////////////
// Author: Graham Smallwood, February 2002
// Desc:   Keeps track of shots fired and people targeted for weapons that want a history of such a thing
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "Common/AudioHandleSpecialValues.h"
#include "Common/GameType.h"
#include "Common/GameAudio.h"
#include "Common/PerfTimer.h"
#include "Common/ThingTemplate.h"
#include "Common/Xfer.h"

#include "GameLogic/FiringTracker.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/Module/ObjectHelper.h"
#include "GameLogic/Object.h"
#include "GameLogic/Weapon.h"


//-------------------------------------------------------------------------------------------------
FiringTracker::FiringTracker(Thing* thing, const ModuleData *modData) : UpdateModule( thing, modData )
{
	m_consecutiveShots = 0;
	m_victimID = INVALID_ID;
	m_frameToStartCooldown = 0;
 	m_frameToForceReload = 0;
	m_frameToStopLoopingSound = 0;
	m_audioHandle = AHSV_NoSound;
	m_firingTrackerBonusCleared = FALSE;
	m_prevTargetCustomWeaponBonus.clear();
	m_prevTargetCustomStatus.clear();
	setWakeFrame(getObject(), UPDATE_SLEEP_FOREVER);
}

//-------------------------------------------------------------------------------------------------
FiringTracker::~FiringTracker()
{
	// no need to protect this.
	TheAudio->removeAudioEvent( m_audioHandle );
	m_audioHandle = AHSV_NoSound;
}

//-------------------------------------------------------------------------------------------------
Int FiringTracker::getNumConsecutiveShotsAtVictim( const Object *victim ) const
{
	if( victim == NULL )
		return 0;// safety, this function is for asking about shots at a victim

	if( victim->getID() != m_victimID )
		return 0;// nope, not shooting at him

	return m_consecutiveShots;// this is how any times I have shot at this hoser right now
}

//-------------------------------------------------------------------------------------------------
void FiringTracker::shotFired(const Weapon* weaponFired, ObjectID victimID)
{
	UnsignedInt now = TheGameLogic->getFrame();
	Object *me = getObject();
	const Object *victim = TheGameLogic->findObjectByID(victimID); // May be null for ground shot

	// Old Target Designator Logic
	if( victim && victim->testStatus(OBJECT_STATUS_FAERIE_FIRE) )
	{
		if( !me->testWeaponBonusCondition(WEAPONBONUSCONDITION_TARGET_FAERIE_FIRE) )
		{
			// We shoot faster at guys marked thusly
			me->setWeaponBonusCondition(WEAPONBONUSCONDITION_TARGET_FAERIE_FIRE);
		}
	}
	else
	{
		// A ground shot or the lack of the status on the target will clear this
		if( me->testWeaponBonusCondition(WEAPONBONUSCONDITION_TARGET_FAERIE_FIRE) )
		{
			me->clearWeaponBonusCondition(WEAPONBONUSCONDITION_TARGET_FAERIE_FIRE);
		}
	}

	// New Buff based 'WeaponBonusAgainst' Logic
	{
		WeaponBonusConditionFlags targetBonusFlags = 0;  // if we attack the ground, this stays empty
		std::vector<AsciiString> targetBonusCustomFlags;
		if (victim)
		{
			targetBonusFlags = victim->getWeaponBonusConditionAgainst();
			targetBonusCustomFlags = victim->getCustomWeaponBonusConditionAgainst();
		}

		// If new bonus is different from previous, remove it.
		if (targetBonusFlags != m_prevTargetWeaponBonus || targetBonusCustomFlags != m_prevTargetCustomWeaponBonus) {
			me->removeWeaponBonusConditionFlags(m_prevTargetWeaponBonus);
			me->removeCustomWeaponBonusConditionFlags(m_prevTargetCustomWeaponBonus);
		}

		// If we have a new bonus, apply it
		if (targetBonusFlags != 0) {
			me->applyWeaponBonusConditionFlags(targetBonusFlags);
		}

		if (!targetBonusCustomFlags.empty()) {
			me->applyCustomWeaponBonusConditionFlags(targetBonusCustomFlags);
		}

		m_prevTargetWeaponBonus = targetBonusFlags;
		m_prevTargetCustomWeaponBonus = targetBonusCustomFlags;

	}

	if( victimID == m_victimID )
	{
		// Shooting at the same guy
		++m_consecutiveShots;
	}
	else if( now < m_frameToStartCooldown )
	{
		// Switching targets within the coast time is valid, and we will not spin down
		++m_consecutiveShots;
		m_victimID = victimID;
	}
	else
	{
		// Start the count over for the new guy
		m_consecutiveShots = 1;
		m_victimID = victimID;
	}

 	// Push back the time that we should force a reload with each shot
 	UnsignedInt autoReloadDelay = weaponFired->getAutoReloadWhenIdleFrames();
 	if( autoReloadDelay > 0 )
 		m_frameToForceReload = now + autoReloadDelay;

	UnsignedInt coast = weaponFired->getContinuousFireCoastFrames();
	if (coast)
		m_frameToStartCooldown = weaponFired->getPossibleNextShotFrame() + coast;
	else
		m_frameToStartCooldown = 0;

	Int shotsNeededOne = weaponFired->getContinuousFireOneShotsNeeded();
	Int shotsNeededTwo = weaponFired->getContinuousFireTwoShotsNeeded();

	if( me->testWeaponBonusCondition( WEAPONBONUSCONDITION_CONTINUOUS_FIRE_MEAN ) )
	{
		// Can either go up or down from here.
		if( m_consecutiveShots < shotsNeededOne )
			coolDown();
		else if( m_consecutiveShots > shotsNeededTwo )
			speedUp();
	}
	else if( me->testWeaponBonusCondition( WEAPONBONUSCONDITION_CONTINUOUS_FIRE_FAST ) )
	{
		// Only place I can go here from here is all the way down
		if( m_consecutiveShots < shotsNeededTwo )
		{
			coolDown();
		}
	}
	else
	{
		// Check to go up
		if( m_consecutiveShots > shotsNeededOne )
			speedUp();
	}

	UnsignedInt fireSoundLoopTime = weaponFired->getFireSoundLoopTime();
	if (fireSoundLoopTime != 0)
	{
		// If the sound has stopped playing, then we need to re-add it.
		if (m_frameToStopLoopingSound == 0 || !TheAudio->isCurrentlyPlaying(m_audioHandle))
		{
			AudioEventRTS audio = weaponFired->getFireSound();
			audio.setObjectID(getObject()->getID());
			m_audioHandle = TheAudio->addAudioEvent( &audio );
		}
		m_frameToStopLoopingSound = now + fireSoundLoopTime;
	}
	else
	{
		AudioEventRTS fireAndForgetSound = weaponFired->getFireSound();
		fireAndForgetSound.setObjectID(getObject()->getID());
		TheAudio->addAudioEvent(&fireAndForgetSound);
		m_frameToStopLoopingSound = 0;
	}


	setWakeFrame(me, calcTimeToSleep());
}

//-------------------------------------------------------------------------------------------------
void FiringTracker::computeFiringTrackerBonus(const Weapon *weaponToFire, const Object *victim)
{
	if(!victim || !weaponToFire)
		return;

	// do nothing if it is the last victim we shot, and no new statuses have been added to it.
	if(m_victimID == victim->getID() && m_prevTargetStatus == victim->getStatusBits() && m_prevTargetCustomStatus == victim->getCustomStatus())
		return;

	Object *me = getObject();

	std::vector<ObjectStatusTypes> weaponStatusTypes = weaponToFire->getFiringTrackerStatusTypes();
	WeaponBonusConditionType weaponBonusType = weaponToFire->getFiringTrackerBonusCondition();
	std::vector<AsciiString> customWeaponStatusTypes = weaponToFire->getFiringTrackerCustomStatusTypes();
	AsciiString customWeaponBonusType = weaponToFire->getFiringTrackerCustomBonusCondition();

	std::vector<GlobalData::TrackerBonusCT> globalTrackerCT = TheGlobalData->m_statusWeaponBonus;
	std::vector<GlobalData::TrackerCustomBonusCT> globalTrackerCustomCT = TheGlobalData->m_statusCustomWeaponBonus;

	// Get bonuses granted outside of FiringTrackerBonus
	WeaponBonusConditionFlags weaponBonusTypeNoClear = me->getWeaponBonusConditionIgnoreClear();
	std::vector<AsciiString> customWeaponBonusTypeNoClear = me->getCustomWeaponBonusConditionIgnoreClear();

	Bool DoStatusBuff = FALSE;
	// Get the victim's custom status
	std::vector<AsciiString> victimCustomStatus = victim->getCustomStatus();

	// New Bonuses Set, enable the bonuses to be cleared
	m_firingTrackerBonusCleared = FALSE;
	m_prevTargetStatus = victim->getStatusBits();
	m_prevTargetCustomStatus = victimCustomStatus;

	// Test for Bonuses Provided by the weapon template.
	/// The bonuses are granted when any of the statuses are found on the target
	if(!customWeaponBonusType.isEmpty() && !victimCustomStatus.empty())
	{
		for (std::vector<AsciiString>::const_iterator it = customWeaponStatusTypes.begin(); it != customWeaponStatusTypes.end(); ++it )
		{
			DoStatusBuff = checkWithinStringVec((*it), victimCustomStatus);
			if(DoStatusBuff)
				break;
			/*std::vector<AsciiString>::const_iterator it2 = victimCustomStatus.find(*it);
			if(it2 != victimCustomStatus.end() && it2->second == 1)
			{
				DoStatusBuff = TRUE;
				break;
			}*/
		}
	}
	if(!DoStatusBuff && !weaponStatusTypes.empty())
	{
		for (std::vector<ObjectStatusTypes>::const_iterator it = weaponStatusTypes.begin(); it != weaponStatusTypes.end(); ++it)
		{
			DoStatusBuff = victim->testStatus(*it);
			if(DoStatusBuff)
				break;
		}
	}
	if( DoStatusBuff == TRUE )
	{
		if(!customWeaponBonusType.isEmpty())
		{
			me->setCustomWeaponBonusCondition(customWeaponBonusType, FALSE);
		}
		if(weaponBonusType != WEAPONBONUSCONDITION_INVALID)
		{
			if( !me->testWeaponBonusCondition(weaponBonusType) )
			{
				// We shoot faster at guys marked thusly
				me->setWeaponBonusCondition(weaponBonusType, FALSE);
			}
		}
	}
	else
	{
		// A ground shot or the lack of the status on the target will clear this
		if(!customWeaponBonusType.isEmpty())
		{
			// If the bonus is currently granted outside of FiringTrackerBonus, don't clear it.
			if(!checkWithinStringVec(customWeaponBonusType, customWeaponBonusTypeNoClear))
				me->clearCustomWeaponBonusCondition(customWeaponBonusType, FALSE);

			//if(it != customWeaponBonusTypeNoClear.end())
			//{
				// If the bonus has been cleared outside of FiringTrackerBonus, clear it.
			//	if(it->second == 0) 
			//		me->clearCustomWeaponBonusCondition(customWeaponBonusType, FALSE);
			//}
			//else
			//{
			//	me->clearCustomWeaponBonusCondition(customWeaponBonusType, FALSE);
			//}
		}
		if(weaponBonusType != WEAPONBONUSCONDITION_INVALID)
		{
			if( (weaponBonusTypeNoClear & (1 << weaponBonusType)) != 0 && me->testWeaponBonusCondition(weaponBonusType) )
			{
				me->clearWeaponBonusCondition(weaponBonusType, FALSE);
			}
		}
	}

	// The bonus provided by the Global Data

	// Test for each of the Custom Bonuses within the Global Data
	if(!globalTrackerCustomCT.empty())
	{

		for (std::vector<GlobalData::TrackerCustomBonusCT>::const_iterator it = globalTrackerCustomCT.begin(); it != globalTrackerCustomCT.end(); ++it )
		{

			Bool DoBuffGlobal = FALSE;
			// Test for the Custom Statuses first
			/*if(!victimCustomStatus.empty())
			{
				for (std::vector<AsciiString>::const_iterator it2 = it->c_status.begin(); it2 != it->c_status.end(); ++it2 )
				{
					std::vector<AsciiString>::const_iterator it3 = victimCustomStatus.find(*it2);
					if(it3 != victimCustomStatus.end() && it3->second == 1)
					{
						DoBuffGlobal = TRUE;
						break;
					}
				}
			}*/
			if(!victimCustomStatus.empty())
			{
				for (std::vector<AsciiString>::const_iterator it2 = it->c_status.begin(); it2 != it->c_status.end(); ++it2 )
				{
					DoBuffGlobal = checkWithinStringVec((*it2), victimCustomStatus);
					if(DoBuffGlobal)
						break;
				}
			}
			// If don't have the required status, Test for all the ObjectStatuses
			if(!DoBuffGlobal && !it->status.empty())
			{
				for (std::vector<ObjectStatusTypes>::const_iterator it3 = it->status.begin(); it3 != it->status.end(); ++it3)
				{
					DoBuffGlobal = victim->testStatus(*it3);
					if(DoBuffGlobal)
						break;
				}
			}

			// Provide the Custom Weapon Bonus to the Attacker
			if(!it->bonus.isEmpty())
			{
				if( DoBuffGlobal == TRUE )
				{
					me->setCustomWeaponBonusCondition(it->bonus, FALSE);
				}
				// Remove the Custom Weapon Bonus from the Attacker
				else
				{
					// If the bonus is currently granted outside of FiringTrackerBonus, don't clear it.
					if(!checkWithinStringVec(it->bonus, customWeaponBonusTypeNoClear))
						me->clearCustomWeaponBonusCondition(it->bonus, FALSE);
					//std::vector<AsciiString>::const_iterator it2 = customWeaponBonusTypeNoClear.find(it->bonus);

					//if(it2 != customWeaponBonusTypeNoClear->end())
					//{
						// If the bonus has been cleared outside of FiringTrackerBonus, clear it.
					//	if(it2->second == 0) 
					//		me->clearCustomWeaponBonusCondition(it->bonus, FALSE);
					//}
					//else
					//{
					//	me->clearCustomWeaponBonusCondition(it->bonus, FALSE);
					//}
				}
			}

		}

	}

	// Test for each of the Weapon Bonuses within the Global Data
	if(!globalTrackerCT.empty())
	{

		for (std::vector<GlobalData::TrackerBonusCT>::const_iterator it = globalTrackerCT.begin(); it != globalTrackerCT.end(); ++it )
		{

			Bool DoBuffGlobal = FALSE;
			// Test for the custom statuses first
			if(!victimCustomStatus.empty())
			{
				for (std::vector<AsciiString>::const_iterator it2 = it->c_status.begin(); it2 != it->c_status.end(); ++it2 )
				{
					DoBuffGlobal = checkWithinStringVec((*it2), victimCustomStatus);
					if(DoBuffGlobal)
						break;
				}
			}
			/*if(!victimCustomStatus.empty())
			{
				for (std::vector<AsciiString>::const_iterator it2 = it->c_status.begin(); it2 != it->c_status.end(); ++it2 )
				{
					std::vector<AsciiString>::const_iterator it3 = victimCustomStatus.find(*it2);
					if(it3 != victimCustomStatus.end() && it3->second == 1)
					{
						DoBuffGlobal = TRUE;
						break;
					}
				}
			}*/
			// If don't have the required status, Test for all the ObjectStatuses
			if(!DoBuffGlobal && !it->status.empty())
			{
				for (std::vector<ObjectStatusTypes>::const_iterator it3 = it->status.begin(); it3 != it->status.end(); ++it3)
				{
					DoBuffGlobal = victim->testStatus(*it3);
					if(DoBuffGlobal)
						break;
				}
			}

			// Provide the Weapon Bonus to the Attacker
			if(it->bonus != WEAPONBONUSCONDITION_INVALID)
			{
				if( DoBuffGlobal == TRUE )
				{
					if( !me->testWeaponBonusCondition(it->bonus) )
					{
						me->setWeaponBonusCondition(it->bonus, FALSE);
					}
				}
				// Remove the Weapon Bonus from the Attacker
				else
				{
					// If the bonus is currently granted outside of FiringTrackerBonus, and it exists within the Unit's WeaponBonus don't clear it.
					if( (weaponBonusTypeNoClear & (1 << it->bonus)) != 0 && me->testWeaponBonusCondition(it->bonus) )
					{
						me->clearWeaponBonusCondition(it->bonus, FALSE);
					}
				}
			}

		}

	}

}

//-------------------------------------------------------------------------------------------------
void FiringTracker::computeFiringTrackerBonusClear(const Weapon *weaponToFire)
{
	// Don't need to scan everytime
	if(!weaponToFire || m_firingTrackerBonusCleared)
		return;

	Object *me = getObject();

	WeaponBonusConditionType weaponBonusType = weaponToFire->getFiringTrackerBonusCondition();
	AsciiString customWeaponBonusType = weaponToFire->getFiringTrackerCustomBonusCondition();

	std::vector<GlobalData::TrackerBonusCT> globalTrackerCT = TheGlobalData->m_statusWeaponBonus;
	std::vector<GlobalData::TrackerCustomBonusCT> globalTrackerCustomCT = TheGlobalData->m_statusCustomWeaponBonus;

	// Get bonuses granted outside of FiringTrackerBonus
	WeaponBonusConditionFlags weaponBonusTypeNoClear = me->getWeaponBonusConditionIgnoreClear();
	std::vector<AsciiString> customWeaponBonusTypeNoClear = me->getCustomWeaponBonusConditionIgnoreClear();

	if(!customWeaponBonusType.isEmpty())
	{
		// If the bonus is currently granted outside of FiringTrackerBonus, don't clear it.
		Bool doClearBonus = TRUE;
		for (std::vector<AsciiString>::const_iterator it2 = customWeaponBonusTypeNoClear.begin(); it2 != customWeaponBonusTypeNoClear.end(); ++it2 )
		{
			if((*it2) == customWeaponBonusType)
			{
				doClearBonus = FALSE;
				break;
			}
		}
		if(doClearBonus)
			me->clearCustomWeaponBonusCondition(customWeaponBonusType, FALSE);
		//std::vector<AsciiString>::const_iterator it = customWeaponBonusTypeNoClear->find(customWeaponBonusType);

		//if(it != customWeaponBonusTypeNoClear->end())
		//{
			// If the bonus has been cleared outside of FiringTrackerBonus, clear it.
		//	if(it->second == 0) 
		//		me->clearCustomWeaponBonusCondition(customWeaponBonusType, FALSE);
		//}
		//else
		//{
		//	me->clearCustomWeaponBonusCondition(customWeaponBonusType, FALSE);
		//}
	}
	if(weaponBonusType != WEAPONBONUSCONDITION_INVALID)
	{
		// If the bonus is currently granted outside of FiringTrackerBonus, and it exists within the Unit's WeaponBonus don't clear it.
		if( (weaponBonusTypeNoClear & (1 << weaponBonusType)) != 0 && me->testWeaponBonusCondition(weaponBonusType) )
		{
			me->clearWeaponBonusCondition(weaponBonusType, FALSE);
		}
	}

	// The bonus provided by the Global Data

	// Clear for each of the Custom Bonuses within the Global Data
	if(!globalTrackerCustomCT.empty())
	{
		for (std::vector<GlobalData::TrackerCustomBonusCT>::const_iterator it = globalTrackerCustomCT.begin(); it != globalTrackerCustomCT.end(); ++it )
		{
			if(!it->bonus.isEmpty())
			{
				// If the bonus is currently granted outside of FiringTrackerBonus, don't clear it.
				if(!checkWithinStringVec(it->bonus, customWeaponBonusTypeNoClear))
					me->clearCustomWeaponBonusCondition(it->bonus, FALSE);
				//std::vector<AsciiString>::const_iterator it2 = customWeaponBonusTypeNoClear->find(it->bonus);

				//if(it2 != customWeaponBonusTypeNoClear->end())
				//{
					// If the bonus has been cleared outside of FiringTrackerBonus, clear it.
				//	if(it2->second == 0) 
				//		me->clearCustomWeaponBonusCondition(it->bonus, FALSE);
				//}
				//else
				//{
				//	me->clearCustomWeaponBonusCondition(it->bonus, FALSE);
				//}
			}
		}
	}

	// Clear for each of the Weapon Bonuses within the Global Data
	if(!globalTrackerCT.empty())
	{
		for (std::vector<GlobalData::TrackerBonusCT>::const_iterator it = globalTrackerCT.begin(); it != globalTrackerCT.end(); ++it )
		{
			if(it->bonus != WEAPONBONUSCONDITION_INVALID)
			{
				// If the bonus is currently granted outside of FiringTrackerBonus, and it exists within the Unit's WeaponBonus don't clear it.
				if( (weaponBonusTypeNoClear & (1 << it->bonus)) != 0 && me->testWeaponBonusCondition(it->bonus) )
					me->clearWeaponBonusCondition(it->bonus, FALSE);
			}
		}
	}

	m_firingTrackerBonusCleared = TRUE;
	m_prevTargetStatus.clear();
	m_prevTargetCustomStatus.clear();

}

//-------------------------------------------------------------------------------------------------
UpdateSleepTime FiringTracker::update()
{
	//DEBUG_ASSERTCRASH(m_frameToStartCooldown != 0 || m_frameToStopLoopingSound != 0, ("hmm, should be asleep"));

	UnsignedInt now = TheGameLogic->getFrame();

 	// I have been idle long enough that I should reload, so I do not hang around with a near empty clip forever.
 	if( m_frameToForceReload != 0  &&  now >= m_frameToForceReload )
 	{
 		getObject()->reloadAllAmmo(TRUE);
 		m_frameToForceReload = 0;
 	}

	// If it has been too long since I fired.  I have to start over.
	// (don't call if we don't need to cool down... it's expensive!)

	if (m_frameToStopLoopingSound != 0)
	{
		if (now >= m_frameToStopLoopingSound)
		{
			TheAudio->removeAudioEvent( m_audioHandle );
			m_audioHandle = AHSV_NoSound;
			m_frameToStopLoopingSound = 0;
		}
	}

	if( m_frameToStartCooldown != 0 && now > m_frameToStartCooldown )
	{
		m_frameToStartCooldown = now + LOGICFRAMES_PER_SECOND;
		coolDown();// if this is the coolest call to cooldown, it will set m_frameToStartCooldown to zero
		return UPDATE_SLEEP(LOGICFRAMES_PER_SECOND);
	}

	UpdateSleepTime sleepTime = calcTimeToSleep();

	return sleepTime;
}

//-------------------------------------------------------------------------------------------------
UpdateSleepTime FiringTracker::calcTimeToSleep()
{
 	// Figure out the longest amount of time we can sleep as unneeded

 	// If all the timers are off, then we aren't needed at all
 	if (m_frameToStopLoopingSound == 0 && m_frameToStartCooldown == 0 && m_frameToForceReload == 0)
   		return UPDATE_SLEEP_FOREVER;

 	// Otherwise, we need to wake up to service the shortest timer
   	UnsignedInt now = TheGameLogic->getFrame();
 	UnsignedInt sleepTime = UPDATE_SLEEP_FOREVER;
 	if( m_frameToStopLoopingSound != 0 )
 	{
 		if( m_frameToStopLoopingSound <= now )
 			sleepTime = UPDATE_SLEEP_NONE;
 		else if( (m_frameToStopLoopingSound - now) < sleepTime )
 			sleepTime = m_frameToStopLoopingSound - now;
 	}
 	if( m_frameToStartCooldown != 0 )
 	{
 		if( m_frameToStartCooldown <= now )
 			sleepTime = UPDATE_SLEEP_NONE;
 		else if( (m_frameToStartCooldown - now) < sleepTime )
 			sleepTime = m_frameToStartCooldown - now;
 	}
 	if( m_frameToForceReload != 0 )
 	{
 		if( m_frameToForceReload <= now )
 			sleepTime = UPDATE_SLEEP_NONE;
 		else if( (m_frameToForceReload - now) < sleepTime )
 			sleepTime = m_frameToForceReload - now;
 	}

 	return UPDATE_SLEEP(sleepTime);
}

//-------------------------------------------------------------------------------------------------
void FiringTracker::speedUp()
{
	ModelConditionFlags clr, set;
	Object *self = getObject();

	if( self->testWeaponBonusCondition( WEAPONBONUSCONDITION_CONTINUOUS_FIRE_FAST ) )
	{
		//self->clearWeaponBonusCondition( WEAPONBONUSCONDITION_CONTINUOUS_FIRE_MEAN );
		//self->clearModelConditionState( MODELCONDITION_CONTINUOUS_FIRE_MEAN );
		//self->clearModelConditionState( MODELCONDITION_CONTINUOUS_FIRE_SLOW );
	}
	else if(self->testWeaponBonusCondition( WEAPONBONUSCONDITION_CONTINUOUS_FIRE_MEAN ) )
	{
		const AudioEventRTS *soundToPlayPtr = self->getTemplate()->getPerUnitSound( "VoiceRapidFire" );
		AudioEventRTS soundToPlay = *soundToPlayPtr;
		soundToPlay.setObjectID( self->getID() );
		TheAudio->addAudioEvent( &soundToPlay );

		// These flags are exclusive, not cumulative
		self->setWeaponBonusCondition( WEAPONBONUSCONDITION_CONTINUOUS_FIRE_FAST );
		set.set(MODELCONDITION_CONTINUOUS_FIRE_FAST);

		// These flags are exclusive, not cumulative
		self->clearWeaponBonusCondition( WEAPONBONUSCONDITION_CONTINUOUS_FIRE_MEAN );
		clr.set(MODELCONDITION_CONTINUOUS_FIRE_MEAN);
		clr.set(MODELCONDITION_CONTINUOUS_FIRE_SLOW);


	}
	else
	{

		self->setWeaponBonusCondition( WEAPONBONUSCONDITION_CONTINUOUS_FIRE_MEAN );
		set.set(MODELCONDITION_CONTINUOUS_FIRE_MEAN);

		// These flags are exclusive, not cumulative
		self->clearWeaponBonusCondition( WEAPONBONUSCONDITION_CONTINUOUS_FIRE_FAST );
		clr.set(MODELCONDITION_CONTINUOUS_FIRE_FAST);
		clr.set(MODELCONDITION_CONTINUOUS_FIRE_SLOW);

	}

	self->clearAndSetModelConditionFlags(clr, set);

}

//-------------------------------------------------------------------------------------------------
void FiringTracker::coolDown()
{
	ModelConditionFlags clr, set;

	if( getObject()->testWeaponBonusCondition( WEAPONBONUSCONDITION_CONTINUOUS_FIRE_FAST )
	 || getObject()->testWeaponBonusCondition( WEAPONBONUSCONDITION_CONTINUOUS_FIRE_MEAN ))
	{

		// Straight to zero from wherever it is
		set.set(MODELCONDITION_CONTINUOUS_FIRE_SLOW);

		getObject()->clearWeaponBonusCondition( WEAPONBONUSCONDITION_CONTINUOUS_FIRE_FAST );
		getObject()->clearWeaponBonusCondition( WEAPONBONUSCONDITION_CONTINUOUS_FIRE_MEAN );
		clr.set(MODELCONDITION_CONTINUOUS_FIRE_FAST);
		clr.set(MODELCONDITION_CONTINUOUS_FIRE_MEAN);


	}
	else // we stop, now
	{

		getObject()->clearWeaponBonusCondition( WEAPONBONUSCONDITION_CONTINUOUS_FIRE_FAST );
		getObject()->clearWeaponBonusCondition( WEAPONBONUSCONDITION_CONTINUOUS_FIRE_MEAN );
		// Just chillin, nothing to change
		clr.set(MODELCONDITION_CONTINUOUS_FIRE_FAST);
		clr.set(MODELCONDITION_CONTINUOUS_FIRE_MEAN);
		clr.set(MODELCONDITION_CONTINUOUS_FIRE_SLOW);

		m_frameToStartCooldown = 0;
	}

	getObject()->clearAndSetModelConditionFlags(clr, set);

	// Start everything over
	m_consecutiveShots = 0;
	m_victimID = INVALID_ID;
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void FiringTracker::crc( Xfer *xfer )
{

	// object helper base class
	UpdateModule::crc( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void FiringTracker::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// object helper base class
	UpdateModule::xfer( xfer );

	// consecutive shots
	xfer->xferInt( &m_consecutiveShots );

	// victim id
	xfer->xferObjectID( &m_victimID );

	// frame to start cooldown
	xfer->xferUnsignedInt( &m_frameToStartCooldown );

	// currenly applied weaponBonus against the prev target
	xfer->xferUnsignedInt(&m_prevTargetWeaponBonus);

	// currenly applied custom weaponBonus against the prev target
	if( xfer->getXferMode() == XFER_SAVE )
	{
		for (std::vector<AsciiString>::const_iterator it = m_prevTargetCustomWeaponBonus.begin(); it != m_prevTargetCustomWeaponBonus.end(); ++it )
		{
			AsciiString bonusName = (*it);
			xfer->xferAsciiString(&bonusName);
		}
		AsciiString empty;
		xfer->xferAsciiString(&empty);
	}
	else if (xfer->getXferMode() == XFER_LOAD)
	{
		if (m_prevTargetCustomWeaponBonus.empty() == false)
		{
			DEBUG_CRASH(( "FiringTracker::xfer - m_prevTargetCustomWeaponBonus should be empty, but is not"));
			//throw SC_INVALID_DATA;
		}
		
		for (;;) 
		{
			AsciiString bonusName;
			xfer->xferAsciiString(&bonusName);
			if (bonusName.isEmpty())
				break;
			m_prevTargetCustomWeaponBonus.push_back(bonusName);
		}
	}

	// cleared firing tracker bonus
	xfer->xferBool( &m_firingTrackerBonusCleared );

	// previous target status
	m_prevTargetStatus.xfer( xfer );

	// previous target custom status
	if( xfer->getXferMode() == XFER_SAVE )
	{
		for (std::vector<AsciiString>::const_iterator it = m_prevTargetCustomStatus.begin(); it != m_prevTargetCustomStatus.end(); ++it )
		{
			AsciiString bonusName = (*it);
			xfer->xferAsciiString(&bonusName);
		}
		AsciiString empty;
		xfer->xferAsciiString(&empty);
	}
	else if (xfer->getXferMode() == XFER_LOAD)
	{
		if (m_prevTargetCustomStatus.empty() == false)
		{
			DEBUG_CRASH(( "FiringTracker::xfer - m_prevTargetCustomStatus should be empty, but is not"));
			//throw SC_INVALID_DATA;
		}
		
		for (;;) 
		{
			AsciiString bonusName;
			xfer->xferAsciiString(&bonusName);
			if (bonusName.isEmpty())
				break;
			m_prevTargetCustomStatus.push_back(bonusName);
		}
	}

}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void FiringTracker::loadPostProcess( void )
{

	// object helper back class
	UpdateModule::loadPostProcess();

}

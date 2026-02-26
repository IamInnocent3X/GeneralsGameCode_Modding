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
// FILE: SabotageBehavior.cpp
// Author: IamInnocent, February 2026
// Based On: SabotageSupplyDropzoneCrateCollide by Kris Morness, June 2003
// Desc:   A saboteur behavior with unified properties that can also be triggered by Hacker Special Powers 
//
///////////////////////////////////////////////////////////////////////////////////////////////////



// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "Common/GameAudio.h"
#include "Common/MiscAudio.h"
#include "Common/Player.h"
#include "Common/PlayerList.h"
#include "Common/Radar.h"
#include "Common/SpecialPower.h"
#include "Common/ThingFactory.h"
#include "Common/ThingTemplate.h"
#include "Common/Xfer.h"

#include "GameClient/Drawable.h"
#include "GameClient/Eva.h"
#include "GameClient/GameText.h"
#include "GameClient/InGameUI.h"  // useful for printing quick debug strings when we need to
#include "GameClient/ParticleSys.h"

#include "GameLogic/ExperienceTracker.h"
#include "GameLogic/Object.h"
#include "GameLogic/PartitionManager.h"
#include "GameLogic/ScriptEngine.h"

#include "GameLogic/Module/AIUpdate.h"
#include "GameLogic/Module/ContainModule.h"
#include "GameLogic/Module/DozerAIUpdate.h"
#include "GameLogic/Module/HijackerUpdate.h"
#include "GameLogic/Module/OCLUpdate.h"
#include "GameLogic/Module/ProductionUpdate.h"
#include "GameLogic/Module/SabotageBehavior.h"
#include "GameLogic/Module/SpecialPowerModule.h"
#include "GameLogic/Module/SpyVisionUpdate.h"



//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
SabotageBehavior::SabotageBehavior( Thing *thing, const ModuleData* moduleData ) : CrateCollide( thing, moduleData )
{
	m_upgradeIfCollidesWith.clear();
	const SabotageBehaviorModuleData *data = getSabotageBehaviorModuleData();
	std::vector<AsciiString>::const_iterator it;
	for(it = data->m_grantUpgradeNames.begin(); it != data->m_grantUpgradeNames.end(); it++)
	{
		const UpgradeTemplate *uu = TheUpgradeCenter->findUpgrade( *it );
		if(!uu)
		{
			DEBUG_LOG(("upgrade not found Sabotage Behavior"));
			DEBUG_CRASH(("upgrade not found Sabotage Behavior"));
			throw INI_INVALID_DATA;
		}

		if(uu->getUpgradeType() != UPGRADE_TYPE_PLAYER)
		{
			DEBUG_LOG(("only parse player upgrades for Sabotage Behavior"));
			DEBUG_CRASH(("only parse player upgrades for Sabotage Behavior"));
			throw INI_INVALID_DATA;
		}
	}
	std::vector<AsciiString> upgrades;
	AsciiString objName;
	objName.clear();
	for(it = data->m_grantUpgradeNamesIfCollidesWith.begin(); it != data->m_grantUpgradeNamesIfCollidesWith.end(); it++)
	{
		const UpgradeTemplate *uu = TheUpgradeCenter->findUpgrade( *it );
		if(uu)
		{
			if(objName.isEmpty())
			{
				DEBUG_LOG(("object name not found for parseObjectUpgradeVec"));
				DEBUG_CRASH(("object name not found for parseObjectUpgradeVec"));
				throw INI_INVALID_DATA;
			}
			if(uu->getUpgradeType() != UPGRADE_TYPE_PLAYER)
			{
				DEBUG_LOG(("only parse player upgrades for Sabotage Behavior"));
				DEBUG_CRASH(("only parse player upgrades for Sabotage Behavior"));
				throw INI_INVALID_DATA;
			}
			upgrades.push_back(*it);
		}
		else
		{
			if(!objName.isEmpty() && !upgrades.empty())
			{
				ObjectUpgradeVecPair pair;
				pair.first = objName;
				pair.second = upgrades;
				m_upgradeIfCollidesWith.push_back(pair);
			}
			upgrades.clear();
			objName.format(*it);
		}
	}
	if(!objName.isEmpty() && !upgrades.empty())
	{
		ObjectUpgradeVecPair pair;
		pair.first = objName;
		pair.second = upgrades;
		m_upgradeIfCollidesWith.push_back(pair);
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
SabotageBehavior::~SabotageBehavior( void )
{
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
Bool SabotageBehavior::isValidToExecute( const Object *other ) const
{
	const SabotageBehaviorModuleData *data = getSabotageBehaviorModuleData();
	if( !data->m_sabotageIsCollide )
	{
		return FALSE;
	}

	// Migrated to canSabotageSpecialCheck() function
	//if( !CrateCollide::isValidToExecute(other) )
	//{
		//Extend functionality.
		//return FALSE;
	//}

	if( !other || other->isEffectivelyDead() )
	{
		//Can't sabotage dead structures
		return FALSE;
	}

	if( !canDoSabotageSpecialCheck( other ) )
	{
		//Object does not have properties to be sabotaged
		return FALSE;
	}

#if !RETAIL_COMPATIBLE_CRC
	if (other->getStatusBits().testForAny(MAKE_OBJECT_STATUS_MASK2(OBJECT_STATUS_UNDER_CONSTRUCTION, OBJECT_STATUS_SOLD)))
	{
		// TheSuperHackers @bugfix Stubbjax 03/08/2025 Can't enter something being sold or under construction.
		return FALSE;
	}
#endif

	Relationship r = getObject()->getRelationship( other );
	if( r != ENEMIES )
	{
		//Can only sabotage enemy buildings.
		return FALSE;
	}

	return TRUE;
}

struct DisableData
{
	DisabledType disableType;
	KindOfMaskType kindOfs;
	KindOfMaskType forbiddenKindOfs;
	UnsignedInt frame;
	Bool disableOwner;
	Bool disableContained;
	TintStatus tintStatus;
	AsciiString customTintStatus;
};

struct DisableSpecialPowerData
{
	const ThingTemplate *tmpl;
	Bool doResetSpecialPower;
	Bool doPauseSpecialPowers;
	Bool doDisableSpyVision;
	Bool disableSpyVisionDoesNotResetTimer;
	Bool doDisableOCLUpdate;
	Bool doResetSpyVision;
	Bool doResetOCLUpdate;
	UnsignedInt frame;
};

//-------------------------------------------------------------------------------
static void disableHacker( Object *obj, void *userData )
{
	DisableData* info = (DisableData*)userData;
	if( obj )
	{
		if(info->disableOwner || obj->isContained())
			obj->setDisabledUntil( info->disableType, info->frame, info->tintStatus, info->customTintStatus );

		if(info->disableContained && obj->getContain())
			obj->getContain()->iterateContained( disableHacker, userData, FALSE );
	}
}

//-------------------------------------------------------------------------------
static void disableObjectsWithKindOfs( Object *obj, void *userData )
{
	DisableData* info = (DisableData*)userData;
	if(obj->isAnyKindOf(info->kindOfs) && !obj->isAnyKindOf(info->forbiddenKindOfs))
	{
		if(info->disableOwner || obj->isContained())
			obj->setDisabledUntil( info->disableType, info->frame, info->tintStatus, info->customTintStatus );

		if(info->disableContained && obj->getContain())
			obj->getContain()->iterateContained( disableHacker, userData, FALSE );
	}
}

//-------------------------------------------------------------------------------
//static void disableInternetCenterSpyVision( Object *obj, void *userData )
//{
//	if( obj && obj->isKindOf( KINDOF_FS_INTERNET_CENTER ) )
//	{
//		UnsignedInt frame = (UnsignedInt)userData;
//
//		//Loop through all it's SpyVisionUpdates() and wake them all up so they can be shut down. This is weird because
//		//it's one of the few update modules that is actually properly sleepified.
//		for( BehaviorModule** u = obj->getBehaviorModules(); *u; ++u )
//		{
//			SpyVisionUpdate *svUpdate = (*u)->getSpyVisionUpdate();
//			if( svUpdate )
//			{
//				//Turn off the vision temporarily. When it recovers from being disabled, the
//				//timer will need to start over from scratch so it won't come on right away unless
//				//it's a permanent spy vision.
//				svUpdate->setDisabledUntilFrame( frame );
//			}
//		}
//	}
//}

//-------------------------------------------------------------------------------
static void sabotageAllSpecificSpecialPowers( Object *obj, void *userData )
{
	DisableSpecialPowerData* info = (DisableSpecialPowerData*)userData;
	Bool canDoSpecialPower = FALSE;
	Bool hasPausedSpecialPowers = FALSE;

	if ( info->tmpl && info->tmpl->isEquivalentTo( obj->getTemplate() ) )
		canDoSpecialPower = TRUE;

	static NameKeyType key_ocl = NAMEKEY( "OCLUpdate" );

	//Loop through all it's SpyVisionUpdates() and wake them all up so they can be shut down. This is weird because
	//it's one of the few update modules that is actually properly sleepified.
	for( BehaviorModule** m = obj->getBehaviorModules(); *m; ++m )
	{
		if( (info->doResetOCLUpdate || info->doDisableOCLUpdate) && (*m)->getModuleNameKey() == key_ocl )
		{
			//Reset the timer on the dropzone. Um... only the dropzone has an OCLUpdate and one, so
			//we can "assume" it's going to be safe. Otherwise, we'll have to write code to search for
			//a specific OCLUpdate.
			OCLUpdate *update = (OCLUpdate*)(*m);
			if( update )
			{
				if( info->doResetOCLUpdate )
					update->resetTimer();

				if( info->doDisableOCLUpdate )
					update->setDisabledUntilFrame( info->frame );
			}
		}

		if( (info->doResetSpyVision || info->doDisableSpyVision) && (*m)->getSpyVisionUpdate() )
		{
			SpyVisionUpdate *svUpdate = (*m)->getSpyVisionUpdate();
			if( svUpdate )
			{
				//Turn off the vision temporarily. When it recovers from being disabled, the
				//timer will need to start over from scratch so it won't come on right away unless
				//it's a permanent spy vision.
				if(info->doDisableSpyVision)
					svUpdate->setDisabledUntilFrame( info->frame, info->disableSpyVisionDoesNotResetTimer );

				// Reset timer follows to enable this to overwrite does not reset timer feature
				if(info->doResetSpyVision)
					svUpdate->resetTimer();
			}
		}

		if( canDoSpecialPower && (info->doResetSpecialPower || info->doPauseSpecialPowers) )
		{
			SpecialPowerModuleInterface* sp = (*m)->getSpecialPower();
			if( sp )
			{
				if(info->doResetSpecialPower)
					sp->startPowerRecharge();

				if(info->doPauseSpecialPowers)
				{
					hasPausedSpecialPowers = TRUE;
					sp->pauseCountdown( TRUE );
				}
			}
		}
	}
	if(hasPausedSpecialPowers)
		obj->setPauseSpecialPowersUntil( info->frame );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void grantPlayerUpgrade( Player *player, const AsciiString upgradeName )
{
	const UpgradeTemplate *upgradeTemplate = TheUpgradeCenter->findUpgrade( upgradeName );
	if(!upgradeTemplate)
		return;

	player->findUpgradeInQueuesAndCancelThem( upgradeTemplate );
	player->addUpgrade( upgradeTemplate, UPGRADE_STATUS_COMPLETE );
	player->getAcademyStats()->recordUpgrade( upgradeTemplate, TRUE );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void grantPlayerSabotageUpgradeOrScience( Player *player, const ScienceVec& scienceVec, const std::vector<AsciiString>& upgradeVec, SabotageGrantType grantType )
{
	Bool isGrantingScience = upgradeVec.empty();
	Bool isGrantingUpgrade = scienceVec.empty();

	// Both are empty
	if(isGrantingScience && isGrantingUpgrade)
		return;

	Int vecSize;
	if(isGrantingScience) 
		vecSize = scienceVec.size();
	else if(isGrantingUpgrade)
		vecSize = upgradeVec.size();

	Int orderStart = 0;
	Bool reverse = FALSE;
	Bool random = FALSE;
	switch(grantType)
	{
		case GRANT_RANDOM:
			random = TRUE;
			break;
		case GRANT_LAST_DONT_HAVE:
		case GRANT_LIKE_LEVELING_UP:
			orderStart = vecSize - 1;
			reverse = TRUE;
			break;
		default:
			break;
	}

	Int i = random ? GameLogicRandomValue(0, vecSize - 1) : orderStart;
	std::vector<Int> doneRandomValue;
	for(;;)
	{
		Bool granted = FALSE;

		const UpgradeTemplate *upgradeTemplate;
		if(isGrantingUpgrade)
		{
			upgradeTemplate = TheUpgradeCenter->findUpgrade( upgradeVec[i] );
			DEBUG_ASSERTCRASH( upgradeTemplate, ("Upgrade %s not found!",upgradeVec[i].str()) );
		}

		if(grantType == GRANT_LIKE_LEVELING_UP)
		{
			if( isGrantingScience )
			{
				if( i == 0 && !player->hasScience( scienceVec[i] ) )
				{
					player->grantScience( scienceVec[i] );
					break;
				}
				else if( player->hasScience( scienceVec[i] ) )
				{
					// Increment the index to the next order
					i++;
					if(i == scienceVec.size())
						break;

					// grant the science as it is not found
					player->grantScience( scienceVec[i] );
					break;
				}
			}
			else if( isGrantingUpgrade )
			{
				if( i == 0 && !player->hasUpgradeComplete( upgradeTemplate ) )
				{
					grantPlayerUpgrade( player, upgradeVec[i] );
					break;
				}
				else if( player->hasUpgradeComplete( upgradeTemplate ) )
				{
					// Increment the index to the next order
					i++;
					if(i == upgradeVec.size())
						break;

					// grant the science as it is not found
					grantPlayerUpgrade( player, upgradeVec[i] );
					break;
				}
			}
		}
		else if( isGrantingUpgrade && !player->hasUpgradeComplete( upgradeTemplate ))
		{
			grantPlayerUpgrade( player, upgradeVec[i] );
			granted = TRUE;
		}
		else if( isGrantingScience && !player->hasScience( scienceVec[i] ))
		{
			player->grantScience( scienceVec[i] );
			granted = TRUE;
		}

		if(grantType != GRANT_ALL && granted)
			break;

		if(random)
		{
			doneRandomValue.push_back(i);
			if(doneRandomValue.size() == vecSize)
				break;

			i = GameLogicRandomValue(0, vecSize-1);
			for(std::vector<Int>::const_iterator it_int = doneRandomValue.begin(); it_int != doneRandomValue.end();)
			{
				// If the index has been checked before, try to get a new index
				if(i == (*it_int))
				{
					i = GameLogicRandomValue(0, vecSize-1);
					it_int = doneRandomValue.begin();
					continue;
				}
				++it_int;
			}
		}
		else if(reverse)
		{
			i--;
			if(i < 0)
				break;
		}
		else
		{
			i++;
			if(i == vecSize)
				break;
		}
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void doSabotageFeedbackFX( const Object *obj, const Object *other, SabotageVictimType type, AsciiString name )
{

  if ( ! obj )
    return;
  if ( ! other )
    return;
  if ( type == SAB_VICTIM_GENERIC && name.isEmpty() )
	return;

	AudioEventRTS soundToPlay;
	if(!name.isEmpty())
	{
		soundToPlay.setEventName( name );
	}
	else
	{
  switch ( type )
  {
    case  SAB_VICTIM_FAKE_BUILDING:
    {
      return; // THIS NEEDS NO ADD'L FEEDBACK
    }
    case 	SAB_VICTIM_COMMAND_CENTER:
    case 	SAB_VICTIM_SUPERWEAPON:
    {
      soundToPlay = TheAudio->getMiscAudio()->m_sabotageResetTimerBuilding;
      break;
    }
    case 	SAB_VICTIM_DROP_ZONE:
    case 	SAB_VICTIM_SUPPLY_CENTER:
    {
      soundToPlay = TheAudio->getMiscAudio()->m_moneyWithdrawSound;
      break;
    }
    case 	SAB_VICTIM_INTERNET_CENTER:
    case 	SAB_VICTIM_MILITARY_FACTORY:
    case 	SAB_VICTIM_POWER_PLANT:
    default:
    {
      soundToPlay = TheAudio->getMiscAudio()->m_sabotageShutDownBuilding;
      break;
    }
  }
	}

	soundToPlay.setPosition( other->getPosition() );
	TheAudio->addAudioEvent( &soundToPlay );

  Drawable *draw = other->getDrawable();
  if ( draw )
    draw->flashAsSelected();

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
Bool SabotageBehavior::executeCrateBehavior( Object *other )
{
	//Check to make sure that the other object is also the goal object in the AIUpdateInterface
	//in order to prevent an unintentional conversion simply by having the terrorist walk too close
	//to it.
	//Assume ai is valid because CrateCollide::isValidToExecute(other) checks it.
	Object *obj = getObject();
	AIUpdateInterface* ai = obj->getAIUpdateInterface();
	if (ai && ai->getGoalObject() != other)
	{
		return false;
	}

	doSabotage( other, getObject() );

	CrateCollide::executeCrateBehavior(other);


	return TRUE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void SabotageBehavior::doSabotage( Object *other, Object *obj )
{
	// sanity
	if(!other || !obj)
		return;

	const SabotageBehaviorModuleData *data = getSabotageBehaviorModuleData();

	if( data->m_sabotageDoAlert )
		TheRadar->tryInfiltrationEvent( other );

  doSabotageFeedbackFX( obj, other, data->m_feedbackType, data->m_feedbackSound );

	//Player *player = other->getControllingPlayer();
	//if( player )
	if( data->m_sabotageDamage || data->m_sabotagePercentDamage )
	{
		DamageInfo damageInfo;
		damageInfo.in.m_damageType = DAMAGE_UNRESISTABLE;
		damageInfo.in.m_deathType = DEATH_DETONATED;
		damageInfo.in.m_sourceID = obj->getID();
		damageInfo.in.m_amount = data->m_sabotageDamage + data->m_sabotagePercentDamage * other->getBodyModule()->getMaxHealth();
		other->attemptDamage( &damageInfo );
	}

	Player *player = obj->getControllingPlayer();
	Player *otherPlayer = other->getControllingPlayer();

	if( !obj->isNeutralControlled() )
	{
		std::vector<AsciiString> upgradeVec;
		ScienceVec scienceVec = data->m_sciencesGranted;
		for(ObjectScienceVec::const_iterator it_sv = data->m_sciencesGrantedIfCollidesWith.begin(); it_sv != data->m_sciencesGrantedIfCollidesWith.end(); ++it_sv)
		{
			const ThingTemplate *tt = TheThingFactory->findTemplate(it_sv->first);	// could be null!
			if ( tt && tt->isEquivalentTo( other->getTemplate() ) )
			{
				for(ScienceVec::const_iterator it_sci = it_sv->second.begin(); it_sci != it_sv->second.end(); ++it_sci)
					scienceVec.push_back(*it_sci);
			}
		}

		grantPlayerSabotageUpgradeOrScience(player, scienceVec, upgradeVec, data->m_sciencesGrantType);

		scienceVec.clear();
		upgradeVec = data->m_grantUpgradeNames;
		for(ObjectUpgradeVec::const_iterator it_uv = m_upgradeIfCollidesWith.begin(); it_uv != m_upgradeIfCollidesWith.end(); ++it_uv)
		{
			const ThingTemplate *tt = TheThingFactory->findTemplate(it_uv->first);	// could be null!
			if ( tt && tt->isEquivalentTo( other->getTemplate() ) )
			{
				for(std::vector<AsciiString>::const_iterator it_up = it_uv->second.begin(); it_up != it_uv->second.end(); ++it_up)
					upgradeVec.push_back(*it_up);
			}
		}

		grantPlayerSabotageUpgradeOrScience(player, scienceVec, upgradeVec, data->m_upgradesGrantType);

		// Grant Instances
		if( (!data->m_commandInstancesToGrant.empty() || !data->m_commandInstancesToGrantWithAmount.empty()) )
		{
			for(std::vector<NameKeyType>::const_iterator it_key = data->m_commandInstancesToGrant.begin(); it_key != data->m_commandInstancesToGrant.end(); ++it_key)
			{
				player->grantInstance( (*it_key) );
			}
			for(NameKeyIntVec::const_iterator it_ki = data->m_commandInstancesToGrantWithAmount.begin(); it_ki != data->m_commandInstancesToGrantWithAmount.end(); ++it_ki)
			{
				player->grantInstance( it_ki->first, it_ki->second );
			}
		}

	}

	//Steal cash!
	if((data->m_sabotageType & SABOTAGE_CASH) && (data->m_stealCashAmount || data->m_stealCashPercentage))
	{
		Money *targetMoney = otherPlayer->getMoney();
		Money *objectMoney = player->getMoney();
		if( targetMoney && objectMoney )
		{
			UnsignedInt cash = targetMoney->countMoney();
			UnsignedInt desiredAmount = data->m_stealCashAmount;
			desiredAmount += REAL_TO_INT(data->m_stealCashPercentage * cash);
			//Check to see if they have the cash, otherwise, take the remainder!
			cash = min( desiredAmount, cash );
			if( cash > 0 )
			{
				//Steal the cash
				targetMoney->withdraw( cash );
				objectMoney->deposit( cash );
				Player* controller = player;
				if (controller)
					controller->getScoreKeeper()->addMoneyEarned( cash );

				//Play the "cash stolen" EVA event if the local player is the victim!
				if( other->isLocallyViewed() && data->m_sabotageDoAlert )
				{
					TheEva->setShouldPlay( EVA_CashStolen );
				}

				//Display cash income floating over the about to be deleted saboteur.
				UnicodeString moneyString;
				moneyString.format( TheGameText->fetch( "GUI:AddCash" ), cash );
				Coord3D pos;
				pos.set( obj->getPosition() );
				pos.z += 20.0f; //add a little z to make it show up above the unit.
				TheInGameUI->addFloatingText( moneyString, &pos, GameMakeColor( 0, 255, 0, 255 ) );

				//Display cash lost floating over the target
				moneyString.format( TheGameText->fetch( "GUI:LoseCash" ), cash );
				pos.set( other->getPosition() );
				pos.z += 30.0f; //add a little z to make it show up above the unit.
				TheInGameUI->addFloatingText( moneyString, &pos, GameMakeColor( 255, 0, 0, 255 ) );
			}
			else
			{
				if( other->isLocallyViewed() && data->m_sabotageDoAlert )
				{
					TheEva->setShouldPlay( EVA_BuildingSabotaged );
				}
			}
		}
	}

	// Disable Power
	if((data->m_sabotageType & SABOTAGE_POWER) && data->m_powerSabotageFrames && otherPlayer)
	{
		UnsignedInt frame = TheGameLogic->getFrame() + data->m_powerSabotageFrames;
		Bool doSpecific = FALSE;

		if(other->isAnyKindOf(data->m_powerSabotageSpecificKindOf))
			doSpecific = TRUE;

		if(!doSpecific)
		{
			for(std::vector<AsciiString>::const_iterator it = data->m_powerSabotageSpecificObjects.begin(); it != data->m_powerSabotageSpecificObjects.end(); ++it)
			{
				const ThingTemplate *tt = TheThingFactory->findTemplate(*it);	// could be null!
				if ( tt && tt->isEquivalentTo( other->getTemplate() ) )
				{
					doSpecific = TRUE;
					break;
				}
			}
		}

		Int energy = other->getTemplate()->getEnergyProduction();
		Int bonus = other->getTemplate()->getEnergyBonus();

		if( doSpecific )
		{
			if( other->isKindOf(KINDOF_POWERED) || other->isKindOf(KINDOF_POWERED_TANK) || energy != 0 || bonus != 0 )
			{
				other->doPowerSabotage(frame, data->m_powerAmount, data->m_powerPercentage, player && data->m_stealsPower ? player->getPlayerIndex() : -1);
			}
		}
		else if( energy != 0 || bonus != 0 )
		{
			Int totalEnergy = otherPlayer->getEnergy()->getProduction();

			//Set the duration inside the player's energy class to record the length of the power outage.
			otherPlayer->getEnergy()->setPowerSabotagedTillFrame( frame, data->m_powerAmount, data->m_powerPercentage );

			//Trigger the callback function that will turn everything off.
			if(data->m_powerAmount == 0 && data->m_powerPercentage == 0.0f)
				otherPlayer->onPowerBrownOutChange( TRUE );

			//Note: Player::update() will check to turn it back on again once the timer expires.

			if(data->m_stealsPower && player)
			{
				Int currentEnergy = otherPlayer->getEnergy()->getProduction();
				Int stolenAmount = totalEnergy - currentEnergy;
				Bool stealDefault = data->m_powerAmount == 0 && data->m_powerPercentage == 0.0f;
				if(stealDefault || stolenAmount > 0)
				{
					if(stealDefault)
						stolenAmount = UINT_MAX - 10;	// Secret Constant Hack

					otherPlayer->getEnergy()->setEnergyGivenTo( frame, stolenAmount, player->getPlayerIndex() );
					player->getEnergy()->setEnergyReceivedFrom( otherPlayer->getPlayerIndex() );
				}
			}
		}

	}

	if(data->m_sabotageFrames > 0)
	{
		UnsignedInt frame = TheGameLogic->getFrame() + data->m_sabotageFrames;

		if(data->m_sabotageDisable)
		{
			other->setDisabledUntil( data->m_sabotageDisabledType, frame, data->m_tintStatus, data->m_customTintStatus );
		}
		else if(!data->m_customTintStatus.isEmpty() || data->m_tintStatus != TINT_STATUS_INVALID)
		{
			other->doStatusDamage( OBJECT_STATUS_NONE, data->m_sabotageFrames, AsciiString::TheEmptyString, data->m_customTintStatus, data->m_tintStatus );
		}

		DisableData disableData;
		disableData.disableType = data->m_sabotageDisabledType;
		disableData.frame = frame;
		disableData.disableOwner = data->m_sabotageDisable;
		disableData.disableContained = data->m_sabotageDisableContained;
		disableData.tintStatus = data->m_tintStatus;
		disableData.customTintStatus = data->m_customTintStatus;

		if(data->m_sabotageDisableContained && other->getContain())
			other->getContain()->iterateContained( disableHacker, &disableData, FALSE );

		if(data->m_sabotageDisableAllKindOf.any() && otherPlayer)
		{
			disableData.kindOfs = data->m_sabotageDisableAllKindOf;
			disableData.forbiddenKindOfs = data->m_sabotageDisableAllForbiddenKindOf;
			otherPlayer->iterateObjects( disableObjectsWithKindOfs, &disableData );
		}

		if( data->m_sabotageDisableCommands )
		{
			std::vector<AsciiString> emptyVec;
			other->setCommandsDisable(frame, emptyVec);
		}
		else if( !data->m_commandsToDisable.empty() )
		{
			other->setCommandsDisable(frame, data->m_commandsToDisable);
		}
	}

	if(data->m_sabotageType & SABOTAGE_PRODUCTION)
	{
		if(other->getProductionUpdateInterface() && player)
			other->getProductionUpdateInterface()->setProductionViewByEnemyFrame(player->getPlayerIndex(), data->m_sabotageProductionViewFrames);

		if(otherPlayer)
		{
			if(data->m_sabotageCostModifierKindOf.any() && data->m_sabotageCostModifierPercentage != 0.0f)
			{
				Bool stackWithAny = data->m_sabotageCostModifierStackingType == SAME_TYPE;
				Bool stackUniqueType = data->m_sabotageCostModifierStackingType == OTHER_TYPE;

				otherPlayer->addKindOfProductionCostChange(data->m_sabotageCostModifierKindOf, data->m_sabotageCostModifierPercentage,
																other->getTemplate()->getTemplateID(), stackUniqueType, stackWithAny, data->m_sabotageCostModifierFrames);
			}

			if(data->m_sabotageTimeModifierKindOf.any() && data->m_sabotageTimeModifierPercentage != 0.0f)
			{
				Bool stackWithAny = data->m_sabotageTimeModifierStackingType == SAME_TYPE;
				Bool stackUniqueType = data->m_sabotageTimeModifierStackingType == OTHER_TYPE;

				otherPlayer->addKindOfProductionTimeChange(data->m_sabotageTimeModifierKindOf, data->m_sabotageTimeModifierPercentage,
																other->getTemplate()->getTemplateID(), stackUniqueType, stackWithAny, data->m_sabotageTimeModifierFrames);
			}
		}
	}

	if(data->m_sabotageType & SABOTAGE_SPECIAL_POWER)
	{
		//Reset ALL special powers!
		UnsignedInt frame = 0;
		Bool doPauseSpecialPowers = FALSE;
		Bool doDisableSpyVision = FALSE;
		Bool doDisableOCLUpdate = FALSE;

		DisableSpecialPowerData disableSPData;
		disableSPData.frame = 0;
		disableSPData.tmpl = other->getTemplate();
		disableSPData.doResetSpecialPower = data->m_sabotageResetAllSameObjectsSpecialPowers;
		disableSPData.doResetSpyVision = data->m_sabotageResetAllSpyVision;
		disableSPData.doResetOCLUpdate = data->m_sabotageResetAllOCLUpdate;
		disableSPData.doDisableSpyVision = FALSE;
		disableSPData.doDisableOCLUpdate = FALSE;

		std::vector<SpecialPowerType> sptVec;

		if(data->m_sabotageFrames > 0)
		{
			frame = TheGameLogic->getFrame() + data->m_sabotageFrames;
			doPauseSpecialPowers = data->m_sabotagePauseSpecialPowers;
			doDisableSpyVision = data->m_sabotageDisableSpyVision;
			doDisableOCLUpdate = data->m_sabotageDisableOCLUpdate;

			disableSPData.frame = frame;
			disableSPData.doPauseSpecialPowers = data->m_sabotagePauseAllSameObjectsSpecialPowers;
			disableSPData.doDisableSpyVision = data->m_sabotageDisableAllSpyVision;
			disableSPData.doDisableOCLUpdate = data->m_sabotageDisableAllOCLUpdate;
			disableSPData.disableSpyVisionDoesNotResetTimer = data->m_sabotageDisableSpyVisionDoesNotResetTimer;
		}
	
		Bool useCommandsUponSabotage = data->m_useCommandsUponSabotage && !obj->isNeutralControlled();
		if( data->m_sabotageResetSpecialPowers ||
			data->m_sabotageResetOCLUpdate ||
			data->m_sabotageResetSpyVision ||
			doPauseSpecialPowers ||
			doDisableSpyVision ||
			doDisableOCLUpdate ||
			useCommandsUponSabotage )
		{
			Bool hasPausedSpecialPowers = FALSE;
			static NameKeyType key_ocl = NAMEKEY( "OCLUpdate" );
			for( BehaviorModule **m = other->getBehaviorModules(); *m; ++m )
			{
				if( (data->m_sabotageResetOCLUpdate || doDisableOCLUpdate) && (*m)->getModuleNameKey() == key_ocl )
				{
					//Reset the timer on the dropzone. Um... only the dropzone has an OCLUpdate and one, so
					//we can "assume" it's going to be safe. Otherwise, we'll have to write code to search for
					//a specific OCLUpdate.
					OCLUpdate *update = (OCLUpdate*)(*m);
					if( update )
					{
						if( data->m_sabotageResetOCLUpdate )
							update->resetTimer();

						if( doDisableOCLUpdate )
							update->setDisabledUntilFrame( frame );
					}
				}

				if( (data->m_sabotageResetSpyVision || doDisableSpyVision) && (*m)->getSpyVisionUpdate() )
				{
					SpyVisionUpdate *svUpdate = (*m)->getSpyVisionUpdate();
					if( svUpdate )
					{
						//Turn off the vision temporarily. When it recovers from being disabled, the
						//timer will need to start over from scratch so it won't come on right away unless
						//it's a permanent spy vision.
						if(doDisableSpyVision)
							svUpdate->setDisabledUntilFrame( frame, data->m_sabotageDisableSpyVisionDoesNotResetTimer );

						// Reset timer follows to enable this to overwrite does not reset timer feature
						if(data->m_sabotageResetSpyVision)
							svUpdate->resetTimer();
					}
				}

				if( data->m_sabotageResetSpecialPowers || doPauseSpecialPowers || useCommandsUponSabotage )
				{
					SpecialPowerModuleInterface* sp = (*m)->getSpecialPower();
					if( sp )
					{
						if(useCommandsUponSabotage)
						{
							Bool canUse = TRUE;
							if(data->m_useCommandsNeedsToBeReady)
							{
								if( sp->getPercentReady() < 1.0f )
								{
									//Not fully ready
									canUse = FALSE;
								}

								if( canUse )
								{
									const SpecialPowerTemplate *spTemplate = sp->getSpecialPowerTemplate();

									// Check for money cost
									if (spTemplate->getCost() > 0) {
										if (otherPlayer->getMoney()->countMoney() < spTemplate->getCost()) {
											canUse = FALSE;
										}
									}
								}
							}

							if(canUse)
							{
								SpecialPowerType spType = sp->getSpecialPowerTemplate()->getSpecialPowerType();
								Bool hasSpType = FALSE;

								for(std::vector<SpecialPowerType>::const_iterator it = sptVec.begin(); it != sptVec.end(); ++it)
								{
									if((*it) == spType)
									{
										hasSpType = TRUE;
										break;
									}
								}

								if(!hasSpType)
									sptVec.push_back(spType);
							}
						}

						if(data->m_sabotageResetSpecialPowers)
							sp->startPowerRecharge();

						if(doPauseSpecialPowers)
						{
							hasPausedSpecialPowers = TRUE;
							sp->pauseCountdown( TRUE );
						}
					}
				}
			}

			if(hasPausedSpecialPowers)
				other->setPauseSpecialPowersUntil( frame );

			if(!sptVec.empty())
			{
				Int randomValue = GameLogicRandomValue(0, sptVec.size()-1); 
				std::vector<Int> doneRandomValue;
				while(!player->forceDoCommandButtonSpecialPower(other, sptVec[randomValue]))
				{
					doneRandomValue.push_back(randomValue);
					if(doneRandomValue.size() >= sptVec.size())
						break;

					randomValue = GameLogicRandomValue(0, sptVec.size()-1);
					for(std::vector<Int>::const_iterator it_int = doneRandomValue.begin(); it_int != doneRandomValue.end();)
					{
						if(randomValue == (*it_int))
						{
							randomValue = GameLogicRandomValue(0, sptVec.size()-1);
							it_int = doneRandomValue.begin();
							continue;
						}
						++it_int;
					}
				}
			}
		}

		if(otherPlayer)
		{
			//Loop through every internet center to temporarily disable the spy vision upgrades.

			//Disable all internet center spy visions (they stack) without visually disabling the other centers.
			//Kris: Note -- Design has changed that we only can have one center at a time... logically, this code
			//doesn't need to change.
			// This loop goes before the Disabled_Hacked one since that will use the normal disabled code with its cool timers.
			// This loop is for the other centers, but it hits the main one too.

			if(disableSPData.doResetSpyVision || disableSPData.doResetOCLUpdate || disableSPData.doDisableSpyVision || disableSPData.doDisableOCLUpdate || disableSPData.doResetSpecialPower || disableSPData.doPauseSpecialPowers)
				otherPlayer->iterateObjects( sabotageAllSpecificSpecialPowers, &disableSPData );

			//Disable the internet center. Note this is purely fluff... because the spy vision update will still run even
			//though we are disabling it. We have to disable the spyvision updates manually because other centers need to
			//be turned off too but without visually disabling them. Yikes!
			//other->setDisabledUntil( DISABLED_HACKED, frame );

			//Disable all the hackers inside.
			//ContainModuleInterface *contain = other->getContain();
			//contain->iterateContained( disableHacker, (void*)frame, FALSE );
		}
	}

	
	if( data->m_sabotageIsCapture )
	{
		Bool canCapture = TRUE;

		// Whoops. Cancel if it or us is now dead.
		if( other->isEffectivelyDead() || getObject()->isEffectivelyDead() )
			canCapture = FALSE;

		if( canCapture && other->getTeam() == obj->getTeam() )
			canCapture = FALSE;

		if( canCapture && data->m_sabotageCaptureBelowHealth && (!other->getBodyModule() || other->getBodyModule()->getHealth() > data->m_sabotageCaptureBelowHealth) )
			canCapture = FALSE;

		if( canCapture && data->m_sabotageCaptureBelowHealthPercent && (!other->getBodyModule() || other->getBodyModule()->getHealth() > data->m_sabotageCaptureBelowHealthPercent * other->getBodyModule()->getMaxHealth()) )
			canCapture = FALSE;

		if( canCapture )
		{
			// Just in case we are capturing a building which is already garrisoned by other
			ContainModuleInterface * contain =  other->getContain();
			if ( contain && contain->isGarrisonable() )
			{
				contain->removeAllContained( TRUE );
			}

			//Play the "building stolen" EVA event if the local player is the victim!
			if( data->m_sabotageDoAlert && other && !other->isNeutralControlled() && other->isLocallyViewed() )
			{
				TheEva->setShouldPlay( EVA_BuildingStolen );
			}

			other->defect( obj->getControllingPlayer()->getDefaultTeam(), data->m_sabotageCaptureTime ); // one frame of flash!
		}
	}

	if ( !other->isEffectivelyDead() && data->m_sabotageFXParticleSystem )
	{
		const ParticleSystemTemplate *tmp = data->m_sabotageFXParticleSystem;
		if (tmp)
		{
			ParticleSystem *sys = TheParticleSystemManager->createParticleSystem(tmp);
			if (sys)
			{
				Coord3D offs = {0,0,0};
				other->getGeometryInfo().makeRandomOffsetWithinFootprint( offs );

				sys->attachToObject(other);
				sys->setPosition( &offs );	

				if( data->m_sabotageFXDuration > 0 )
				{
					UnsignedInt durationInterleaveFactor = 1;
					Real targetFootprintArea = other->getGeometryInfo().getFootprintArea();

					if ( ( targetFootprintArea < 300) && other->isKindOf( KINDOF_STRUCTURE ))
					{
						durationInterleaveFactor = 2;
					}

					sys->setSystemLifetime( data->m_sabotageFXDuration * durationInterleaveFactor ); //lifetime of the system, not the particles
				}
			}
		}
	}


}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
Bool SabotageBehavior::canDoSabotageSpecialCheck( const Object *other ) const
{
	if( !CrateCollide::isValidToExecute(other) )
	{
		//Extend functionality.
		return FALSE;
	}

	const SabotageBehaviorModuleData *data = getSabotageBehaviorModuleData();

	// Does Unresistable Damage
	if( data->m_sabotageDamage || data->m_sabotagePercentDamage )
		return TRUE;

	if( data->m_sabotageIsCapture )
	{
		Bool canCapture = TRUE;

		if( other->isEffectivelyDead() || getObject()->isEffectivelyDead() )
			canCapture = FALSE;

		if( canCapture && other->getTeam() == getObject()->getTeam() )
			canCapture = FALSE;

		if( canCapture && data->m_sabotageCaptureBelowHealth && (!other->getBodyModule() || other->getBodyModule()->getHealth() > data->m_sabotageCaptureBelowHealth) )
			canCapture = FALSE;

		if( canCapture && data->m_sabotageCaptureBelowHealthPercent && (!other->getBodyModule() || other->getBodyModule()->getHealth() > data->m_sabotageCaptureBelowHealthPercent * other->getBodyModule()->getMaxHealth()) )
			canCapture = FALSE;

		if( canCapture )
			return TRUE;
	}

	if( !getObject()->isNeutralControlled() )
	{
		if( (!data->m_commandInstancesToGrant.empty() || !data->m_commandInstancesToGrantWithAmount.empty()) )
			return TRUE;

		if(!data->m_grantUpgradeNames.empty())
			return TRUE;

		if(!data->m_sciencesGranted.empty())
			return TRUE;

		for(ObjectScienceVec::const_iterator it_sv = data->m_sciencesGrantedIfCollidesWith.begin(); it_sv != data->m_sciencesGrantedIfCollidesWith.end(); ++it_sv)
		{
			const ThingTemplate *tt = TheThingFactory->findTemplate(it_sv->first);	// could be null!
			if ( tt && tt->isEquivalentTo( other->getTemplate() ) && !it_sv->second.empty() )
				return TRUE;
		}

		for(ObjectUpgradeVec::const_iterator it_uv = m_upgradeIfCollidesWith.begin(); it_uv != m_upgradeIfCollidesWith.end(); ++it_uv)
		{
			const ThingTemplate *tt = TheThingFactory->findTemplate(it_uv->first);	// could be null!
			if ( tt && tt->isEquivalentTo( other->getTemplate() ) && !it_uv->second.empty() )
				return TRUE;
		}
	}

	// Can Steal cash!
	if((data->m_sabotageType & SABOTAGE_CASH) && (data->m_stealCashAmount || data->m_stealCashPercentage))
		return TRUE;

	// Check for Disable Power
	if (data->m_sabotageType & SABOTAGE_POWER)
	{
		// Has Energy, so can infiltrate
		if(other->getTemplate()->getEnergyProduction() > 0 || other->getTemplate()->getEnergyBonus() > 0)
			return TRUE;

		Bool doSpecific = FALSE;
		if(other->isAnyKindOf(data->m_powerSabotageSpecificKindOf))
			doSpecific = TRUE;

		if(!doSpecific)
		{
			for(std::vector<AsciiString>::const_iterator it = data->m_powerSabotageSpecificObjects.begin(); it != data->m_powerSabotageSpecificObjects.end(); ++it)
			{
				const ThingTemplate *tt = TheThingFactory->findTemplate(*it);	// could be null!
				if ( tt && tt->isEquivalentTo( other->getTemplate() ) )
				{
					doSpecific = TRUE;
					break;
				}
			}
		}

		if(doSpecific)
		{
			// Can infiltrate the powered object to disable it
			if(other->isKindOf(KINDOF_POWERED) || other->isKindOf(KINDOF_POWERED_TANK))
				return TRUE;
		}
	}

	if(data->m_sabotageFrames > 0)
	{
		// Can Disable the Target
		if(data->m_sabotageDisable)
			return TRUE;

		if(data->m_sabotageDisableContained && other->getContain())
			return TRUE;

		if( data->m_sabotageDisableCommands || !data->m_commandsToDisable.empty() )
			return TRUE;

		// Limit only to player objects if checks for all kind of
		if(data->m_sabotageDisableAllKindOf.any() && !other->isNeutralControlled())
			return TRUE;
	}

	if(data->m_sabotageType & SABOTAGE_PRODUCTION)
	{
		if(other->getProductionUpdateInterface() && data->m_sabotageProductionViewFrames != 0)
			return TRUE;

		if(!other->isNeutralControlled())
		{
			if(data->m_sabotageCostModifierKindOf.any() && data->m_sabotageCostModifierPercentage != 0.0f)
				return TRUE;

			if(data->m_sabotageTimeModifierKindOf.any() && data->m_sabotageTimeModifierPercentage != 0.0f)
				return TRUE;
		}
	}

	if(data->m_sabotageType & SABOTAGE_SPECIAL_POWER)
	{
		Bool doPauseSpecialPowers = FALSE;
		Bool doDisableSpyVision = FALSE;
		Bool doDisableOCLUpdate = FALSE;

		DisableSpecialPowerData disableSPData;
		disableSPData.doResetSpecialPower = data->m_sabotageResetAllSameObjectsSpecialPowers;
		disableSPData.doResetSpyVision = data->m_sabotageResetAllSpyVision;
		disableSPData.doResetOCLUpdate = data->m_sabotageResetAllOCLUpdate;
		disableSPData.doDisableSpyVision = FALSE;
		disableSPData.doDisableOCLUpdate = FALSE;

		if(data->m_sabotageFrames > 0)
		{
			doPauseSpecialPowers = data->m_sabotagePauseSpecialPowers;
			doDisableSpyVision = data->m_sabotageDisableSpyVision;
			doDisableOCLUpdate = data->m_sabotageDisableOCLUpdate;

			disableSPData.doPauseSpecialPowers = data->m_sabotagePauseAllSameObjectsSpecialPowers;
			disableSPData.doDisableSpyVision = data->m_sabotageDisableAllSpyVision;
			disableSPData.doDisableOCLUpdate = data->m_sabotageDisableAllOCLUpdate;
		}
	
		if((data->m_sabotageResetOCLUpdate || doDisableOCLUpdate) && other->getOCLUpdate())
			return TRUE;

		Bool useCommandsUponSabotage = data->m_useCommandsUponSabotage && !getObject()->isNeutralControlled();
		if( data->m_sabotageResetSpecialPowers ||
			data->m_sabotageResetSpyVision ||
			doPauseSpecialPowers ||
			doDisableSpyVision ||
			useCommandsUponSabotage )
		{
			for( BehaviorModule **m = other->getBehaviorModules(); *m; ++m )
			{
				if( (data->m_sabotageResetSpyVision || doDisableSpyVision) && (*m)->getSpyVisionUpdate() )
					return TRUE;

				if( data->m_sabotageResetSpecialPowers || doPauseSpecialPowers || useCommandsUponSabotage )
				{
					SpecialPowerModuleInterface* sp = (*m)->getSpecialPower();
					if( sp )
					{
						return TRUE;
					}
				}
			}
		}

		if(!other->isNeutralControlled())
		{
			if(disableSPData.doResetSpyVision || disableSPData.doResetOCLUpdate || disableSPData.doDisableSpyVision || disableSPData.doDisableOCLUpdate || disableSPData.doResetSpecialPower || disableSPData.doPauseSpecialPowers)
				return TRUE;
		}
	}

	return FALSE;

}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void SabotageBehavior::crc( Xfer *xfer )
{

	// extend base class
	CrateCollide::crc( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void SabotageBehavior::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	CrateCollide::xfer( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void SabotageBehavior::loadPostProcess( void )
{

	// extend base class
	CrateCollide::loadPostProcess();

}

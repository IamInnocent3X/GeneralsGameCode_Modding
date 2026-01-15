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

// FILE: StealthUpdate.cpp ////////////////////////////////////////////////////////////////////////
// Author: Kris Morness, May 2002
// Modified by: IamInnocent, November 2025
// Desc:	 An update that checks for a status bit to stealth the owning object
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#define DEFINE_STEALTHLEVEL_NAMES
#define DEFINE_OBJECT_STATUS_NAMES

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "Common/GameState.h"
#include "Common/GameUtility.h"
#include "Common/MapObject.h"
#include "Common/Player.h"
#include "Common/PlayerList.h"
#include "Common/Radar.h"
#include "Common/Team.h"
#include "Common/ThingTemplate.h"
#include "Common/ThingFactory.h"
#include "Common/Xfer.h"

#include "GameClient/ControlBar.h"
#include "GameClient/Drawable.h"
#include "GameClient/FXList.h"
#include "GameClient/GameClient.h"
#include "GameClient/Eva.h"

#include "GameLogic/Damage.h"
#include "GameLogic/Object.h"
#include "GameLogic/PartitionManager.h"
#include "GameLogic/Weapon.h"

#include "GameLogic/Module/AIUpdate.h"
#include "GameLogic/Module/StealthUpdate.h"
#include "GameLogic/Module/PhysicsUpdate.h"
#include "GameLogic/Module/ContainModule.h"
#include "GameLogic/Module/RailroadGuideAIUpdate.h"
#include "GameLogic/Module/SpawnBehavior.h"
#include "GameLogic/Module/SpecialAbilityUpdate.h"



StealthUpdateModuleData::StealthUpdateModuleData()
{
		m_disguiseFX = nullptr;
    m_disguiseRevealFX = nullptr;
    m_stealthDelay		= UINT_MAX;
    m_stealthLevel		= 0;
    m_stealthSpeed		= 0.0f;
    m_friendlyOpacityMin = 0.5f;
    m_friendlyOpacityMax = 1.0f;
    m_pulseFrames = 30;
    m_teamDisguised		= false;
    m_revealDistanceFromTarget = 0.0f;
    m_orderIdleEnemiesToAttackMeUponReveal = false;
    m_innateStealth   = true;
	m_innateDisguise   = false;
    m_disguiseTransitionFrames = 0;
    m_disguiseRevealTransitionFrames = 0;
    m_blackMarketCheckFrames = 0;
	m_disguiseFriendlyFlickerDelay = 0;
	m_disguiseFlickerTransitionTime = 0;
    m_enemyDetectionEvaEvent = EVA_Invalid;
    m_ownDetectionEvaEvent = EVA_Invalid;
    m_grantedBySpecialPower = false;
	m_autoDisguiseWhenAvailable = false;
	m_canStealthWhileDisguised = false;
	m_disguiseRetainAfterDetected = false;
	m_preservePendingCommandWhenDetected = false;
	m_dontFlashWhenFlickering = false;
	m_disguiseUseOriginalFiringOffset = false;
	m_isSimpleDisguise = false;
}


//-------------------------------------------------------------------------------------------------
void StealthUpdateModuleData::buildFieldParse(MultiIniFieldParse& p)
{
  UpdateModuleData::buildFieldParse(p);

	static const FieldParse dataFieldParse[] =
	{
		{ "StealthDelay",									INI::parseDurationUnsignedInt,	nullptr, offsetof( StealthUpdateModuleData, m_stealthDelay ) },
		{ "MoveThresholdSpeed",						INI::parseVelocityReal,					nullptr, offsetof( StealthUpdateModuleData, m_stealthSpeed ) },
		{ "StealthForbiddenConditions",		INI::parseBitString32,					TheStealthLevelNames, offsetof( StealthUpdateModuleData, m_stealthLevel) },
		{ "HintDetectableConditions",	  	ObjectStatusMaskType::parseFromINI,	nullptr, offsetof( StealthUpdateModuleData, m_hintDetectableStates) },
		{ "RequiredStatus",								ObjectStatusMaskType::parseFromINI,	nullptr, offsetof( StealthUpdateModuleData, m_requiredStatus ) },
		{ "ForbiddenStatus",							ObjectStatusMaskType::parseFromINI,	nullptr, offsetof( StealthUpdateModuleData, m_forbiddenStatus ) },
		{ "FriendlyOpacityMin",						INI::parsePercentToReal,				nullptr, offsetof( StealthUpdateModuleData, m_friendlyOpacityMin ) },
		{ "FriendlyOpacityMax",						INI::parsePercentToReal,				nullptr, offsetof( StealthUpdateModuleData, m_friendlyOpacityMax ) },
		{ "PulseFrequency",								INI::parseDurationUnsignedInt,	nullptr, offsetof( StealthUpdateModuleData, m_pulseFrames ) },
		{ "DisguisesAsTeam",							INI::parseBool,									nullptr, offsetof( StealthUpdateModuleData, m_teamDisguised ) },
		{ "RevealDistanceFromTarget",			INI::parseReal,									nullptr, offsetof( StealthUpdateModuleData, m_revealDistanceFromTarget ) },
		{ "OrderIdleEnemiesToAttackMeUponReveal", INI::parseBool,					nullptr, offsetof( StealthUpdateModuleData, m_orderIdleEnemiesToAttackMeUponReveal ) },
		{ "DisguiseFX",										INI::parseFXList,								nullptr, offsetof( StealthUpdateModuleData, m_disguiseFX ) },
		{ "DisguiseRevealFX",							INI::parseFXList,								nullptr, offsetof( StealthUpdateModuleData, m_disguiseRevealFX ) },
		{ "DisguiseTransitionTime",				INI::parseDurationUnsignedInt,  nullptr, offsetof( StealthUpdateModuleData, m_disguiseTransitionFrames ) },
		{ "DisguiseRevealTransitionTime",	INI::parseDurationUnsignedInt,  nullptr, offsetof( StealthUpdateModuleData, m_disguiseRevealTransitionFrames ) },
		{ "DisguiseFlickerTransitionTime",	INI::parseDurationUnsignedInt,  nullptr, offsetof( StealthUpdateModuleData, m_disguiseFlickerTransitionTime ) },
		{ "DisguiseFriendlyFlickerDelay",	INI::parseDurationUnsignedInt,	nullptr, offsetof( StealthUpdateModuleData, m_disguiseFriendlyFlickerDelay ) },
		{ "InnateStealth",								INI::parseBool,									nullptr, offsetof( StealthUpdateModuleData, m_innateStealth ) },
		{ "UseRiderStealth",							INI::parseBool,									nullptr, offsetof( StealthUpdateModuleData, m_useRiderStealth ) },
		{ "InnateDisguise",								INI::parseBool,									nullptr, offsetof( StealthUpdateModuleData, m_innateDisguise ) },
		{ "AutoDisguiseWhenAvailable",					INI::parseBool,	nullptr, offsetof( StealthUpdateModuleData, m_autoDisguiseWhenAvailable ) },
		{ "CanStealthWhileDisguised",					INI::parseBool,	nullptr, offsetof( StealthUpdateModuleData, m_canStealthWhileDisguised ) },
		{ "DisguiseRetainAfterDetected",				INI::parseBool,	nullptr, offsetof( StealthUpdateModuleData, m_disguiseRetainAfterDetected ) },
		{ "PreservePendingCommandWhenDetected",			INI::parseBool,	nullptr, offsetof( StealthUpdateModuleData, m_preservePendingCommandWhenDetected ) },
		{ "DontFlashWhenFlickering",			INI::parseBool,	nullptr, offsetof( StealthUpdateModuleData, m_dontFlashWhenFlickering ) },
		{ "UseOriginalFiringOffsetWhileDisguised",		INI::parseBool,	nullptr, offsetof( StealthUpdateModuleData, m_disguiseUseOriginalFiringOffset ) },
		{ "IsSimpleDisguiseForWeapons",		INI::parseBool,	nullptr, offsetof( StealthUpdateModuleData, m_isSimpleDisguise ) },
    { "EnemyDetectionEvaEvent",				Eva::parseEvaMessageFromIni,  	nullptr, offsetof( StealthUpdateModuleData, m_enemyDetectionEvaEvent ) },
    { "OwnDetectionEvaEvent",		  		Eva::parseEvaMessageFromIni,  	nullptr, offsetof( StealthUpdateModuleData, m_ownDetectionEvaEvent ) },
		{ "BlackMarketCheckDelay",				INI::parseDurationUnsignedInt,  nullptr, offsetof( StealthUpdateModuleData, m_blackMarketCheckFrames ) },
    { "GrantedBySpecialPower",        INI::parseBool,                 nullptr, offsetof( StealthUpdateModuleData, m_grantedBySpecialPower ) },
	{ "RequiredCustomStatus",	INI::parseAsciiStringVector, nullptr, 	offsetof( StealthUpdateModuleData, m_requiredCustomStatus ) },
	{ "ForbiddenCustomStatus",	INI::parseAsciiStringVector, nullptr, 	offsetof( StealthUpdateModuleData, m_forbiddenCustomStatus ) },

		{ nullptr, nullptr, nullptr, 0 }
	};
  p.add(dataFieldParse);
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
StealthUpdate::StealthUpdate( Thing *thing, const ModuleData* moduleData ) : UpdateModule( thing, moduleData )
{
	const StealthUpdateModuleData *data = getStealthUpdateModuleData();

	m_stealthAllowedFrame = TheGameLogic->getFrame() + data->m_stealthDelay;

	//Must be enabled manually if using disguise system (bomb truck uses)
	m_enabled = !data->m_teamDisguised || (data->m_innateDisguise && canDisguise());

	m_detectionExpiresFrame = 0;
	m_pulsePhaseRate		= 0.2f;
	m_pulsePhase				= GameClientRandomValueReal(0, PI);

	m_disguiseAsPlayerIndex			= -1;
	m_disguiseAsTemplate			  = nullptr;
	m_transitioningToDisguise		= false;
	m_disguised									= false;
	m_disguiseTransitionFrames	= 0;
	m_disguiseHalfpointReached  = false;
	m_nextBlackMarketCheckFrame = 0;
	m_framesGranted = 0;

	m_stealthLevelOverride = 0;

	m_lastDisguiseAsTemplate = nullptr;
	m_lastDisguiseAsPlayerIndex			= -1;

	m_flicked = false;
	m_flicking = false;
	m_flickerFrame = 0;
	m_disguiseTransitionIsFlicking = false;

	m_preserveLastGUI = false;

	m_markForClearStealthLater = false;
	m_isNotAutoDisguise = false;

	m_disguiseModelName.clear();

	m_updatePulse = false;
	m_updatePulseOnly = false;

	m_originalDrawableTemplate = nullptr;
	m_disguisedDrawableTemplate = nullptr;
	//m_originalDrawableFiringOffsets.clear();
	//m_disguisedDrawableFiringOffsets.clear();

	m_nextWakeUpFrame = 1;

	if( data->m_innateStealth )
	{
		//Giving innate stealth units this status bit allows other code to easily check the status bit.
		getObject()->setStatus( MAKE_OBJECT_STATUS_MASK( OBJECT_STATUS_CAN_STEALTH ) );
	}

	Bool foundDisguise = FALSE;
	if( data->m_innateDisguise && canDisguise() )
	{
		// Added disguise to be able to use as innate
		SpecialAbilityUpdate *spUpdate = getObject()->findSpecialAbilityUpdate( SPECIAL_DISGUISE_AS_VEHICLE );

		if(spUpdate)
		{
			KindOfMaskType disguiseMask = spUpdate->getKindOfs();
			KindOfMaskType forbiddenMask = spUpdate->getForbiddenKindOfs();
			if(disguiseMask == KINDOFMASK_NONE)
			{
				disguiseMask.set( KINDOF_VEHICLE );
				forbiddenMask.set( KINDOF_AIRCRAFT );
				forbiddenMask.set( KINDOF_BOAT );
				forbiddenMask.set( KINDOF_CLIFF_JUMPER );
			}

			for(Object *disguiseObj = TheGameLogic->getFirstObject(); disguiseObj; disguiseObj = disguiseObj->getNextObject() )
			{
				if( disguiseObj->isAnyKindOf(forbiddenMask) )
					continue;
				
				if( disguiseObj->isAnyKindOf(disguiseMask) )
					foundDisguise = TRUE;

				if(foundDisguise)
				{
					//Don't allow it to disguise as another bomb truck -- that's just plain dumb.
					//if( disguiseObj->getTemplate() != obj->getTemplate() )
					{
						//Don't allow it to disguise as a train -- they don't have KINDOF_TRAIN yet, but
						//if added, please change this code so it'll be faster!
						static const NameKeyType key = NAMEKEY( "RailroadBehavior" );
						RailroadBehavior *rBehavior = (RailroadBehavior*)disguiseObj->findUpdateModule( key );
						if( !rBehavior )
						{
							disguiseAsObject( disguiseObj );
							break;
						}
					}
				}
			}

			if(!foundDisguise)
			{
				const ThingTemplate *thingTemplate;
				MapObject *pMapObj;
				for (pMapObj = MapObject::getFirstMapObject(); pMapObj; pMapObj = pMapObj->getNext())
				{
					// get thing template based from map object name
					thingTemplate = pMapObj->getThingTemplate();
					if( thingTemplate == nullptr )
						continue;

					if( thingTemplate->isAnyKindOf(forbiddenMask) )
						continue;
				
					if( thingTemplate->isAnyKindOf(disguiseMask) )
						foundDisguise = TRUE;

					if(foundDisguise)
					{
						const ThingTemplate *givenTemplate = TheThingFactory->findTemplate( "GenericTree" );
						if(givenTemplate)
						{
							Drawable *newDraw = TheThingFactory->newDrawable( givenTemplate );
							if( newDraw )
							{
								Drawable *currDraw = TheThingFactory->newDrawable( thingTemplate );
								if(currDraw)
								{
									AsciiString modelName = currDraw->getModelName();
									TheGameClient->destroyDrawable( currDraw );
									disguiseAsObject( nullptr, newDraw );
									m_disguiseModelName = modelName;
									break;
								}
							}
						}
					}

				}
			}

		}
	}

	// start active, since some stealths start enabled from the get-go
  if ( data->m_grantedBySpecialPower )
	  setWakeFrame( getObject(), UPDATE_SLEEP_FOREVER );
  else
	  setWakeFrame( getObject(), UPDATE_SLEEP_NONE );

	// we do not need to restore a disguise
	// we do need to set disguise first if innate
	m_xferRestoreDisguise = foundDisguise; //FALSE;

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
StealthUpdate::~StealthUpdate( void )
{

}

//-------------------------------------------------------------------------------------------------
void isBlackMarket( Object *obj, void *userData )
{
	if( obj && obj->isKindOf( KINDOF_FS_BLACK_MARKET ) )
	{
		if( obj->isEffectivelyDead() )
		{
			return;
		}
		if( obj->testStatus(OBJECT_STATUS_UNDER_CONSTRUCTION) )
		{
			return;
		}
		if( obj->testStatus( OBJECT_STATUS_SOLD ) )
		{
			return;
		}
		*(Bool*)userData = TRUE;
	}
}

//---------------------------------------------------------------------------------------~-_-~-_-~-
void StealthUpdate::receiveGrant( Bool active, UnsignedInt frames )
{
  Object *obj = getObject();
  if ( obj == nullptr )
    return;

  if (this->canDisguise())
    return; //so bombtrucks and stuff do not get foiled by this.

	//Kris: Turn it off if we pass in FALSE for active.
	if( !active && m_enabled )
	{
		//markAsDetected();
	}

	m_enabled = active;

	if( m_enabled )
	{
		//On
		obj->setStatus( MAKE_OBJECT_STATUS_MASK2( OBJECT_STATUS_CAN_STEALTH, OBJECT_STATUS_STEALTHED ) );
		m_stealthAllowedFrame = TheGameLogic->getFrame();
	  setWakeFrame( obj, UPDATE_SLEEP_NONE );
		//m_framesGranted = frames;
		m_framesGranted = frames + m_stealthAllowedFrame;
	}
	else
	{
		//Off
		obj->clearStatus( MAKE_OBJECT_STATUS_MASK2( OBJECT_STATUS_CAN_STEALTH, OBJECT_STATUS_STEALTHED ) );
		m_stealthAllowedFrame = FOREVER;
		m_framesGranted = 0;
		Drawable *draw = obj->getDrawable();
		if( draw )
		{
			draw->setEffectiveOpacity( 1.0f );
		}
	}

  const ContainModuleInterface *contain = obj->getContain();
  if ( contain && contain->isRiderChangeContain() )
  {
    const Object *rider = contain->friend_getRider();
    if ( rider )
    {
      StealthUpdate *riderStealth = rider->getStealth();
      if ( riderStealth )
        riderStealth->receiveGrant( active, frames );
    }
  }



}


//-------------------------------------------------------------------------------------------------
Bool StealthUpdate::allowedToStealth( Object *stealthOwner ) const
{
	const Object *self = getObject();
	const StealthUpdateModuleData *data = getStealthUpdateModuleData();
	UnsignedInt now = TheGameLogic->getFrame();

	UnsignedInt flags = data->m_stealthLevel;
	if (m_stealthLevelOverride > 0)
		flags = m_stealthLevelOverride;

	if( self != stealthOwner )
	{
		//Extract the rules from the rider's stealthupdate module data instead
		//of our own, because the rider determines if the container can stealth or not.

    const StealthUpdate *stealthUpdate = stealthOwner->getStealth();
		if( stealthUpdate )
		{
			flags = stealthUpdate->getStealthLevel();
		}
	}

	//With regards to slaves that stealth with us, we need to all be stealthed or not at all. If
	//any of the slaves can't stealth, then reveal everyone!
	if( self->isKindOf( KINDOF_SPAWNS_ARE_THE_WEAPONS ) )
	{
		SpawnBehaviorInterface *sbInterface = self->getSpawnBehaviorInterface();
		if( sbInterface )
		{
			if( !sbInterface->areAllSlavesStealthed() )
			{
				sbInterface->revealSlaves();
				return FALSE;
			}
		}
	}

	if( flags & STEALTH_NOT_WHILE_ATTACKING && self->getStatusBits().test( OBJECT_STATUS_IS_FIRING_WEAPON ) )
	{
		//Doesn't stealth while aggressive (includes approaching).
		return FALSE;
	}

	if( flags & STEALTH_NOT_WHILE_USING_ABILITY && self->getStatusBits().test( OBJECT_STATUS_IS_USING_ABILITY ) )
	{
		//Doesn't stealth while using a special ability (starting with preparation, which takes place after unpacking).
		return FALSE;
	}

	if( flags & STEALTH_ONLY_WITH_BLACK_MARKET && m_nextBlackMarketCheckFrame < now )
	{
		//randomize timer a little incase we have a whole bunch on the same frame.
		m_nextBlackMarketCheckFrame += data->m_blackMarketCheckFrames + GameLogicRandomValue( 0, 10 );

		//If we can't find an active black market, then we can't stealth.
		Bool blackMarket = FALSE;
		self->getControllingPlayer()->iterateObjects( isBlackMarket, &blackMarket );
		if( !blackMarket )
		{
			return FALSE;
		}
	}

	// IamInnocent - Added ability to skip can_stealth check if object is disguised or able to disguise with auto Disguise
	if( !stealthOwner->getStatusBits().test( OBJECT_STATUS_CAN_STEALTH ) && !isDisguised() && !(data->m_autoDisguiseWhenAvailable && m_lastDisguiseAsTemplate))
	{
		return FALSE;
	}

	if( flags & STEALTH_NOT_WHILE_TAKING_DAMAGE && self->getBodyModule()->getLastDamageTimestamp() >= now - 1 )
	{
#if PRESERVE_RETAIL_BEHAVIOR
		//Only if it's not healing damage.
		if( self->getBodyModule()->getLastDamageInfo()->in.m_damageType != DAMAGE_HEALING )
#endif
		{
			//Can't stealth if we just took damage in the last frame or two.
			if( self->getBodyModule()->getLastDamageTimestamp() != 0xffffffff )
			{
				//But it's initialized to 0xffffffff so we don't think we took damage on the first frame.
				return FALSE;
			}
		}
	}

	//We need all required status or else we fail
	// If we have any requirements
	if( data->m_requiredStatus.any()  &&  !self->getStatusBits().testForAll( data->m_requiredStatus ) )
		return FALSE;

	//If we have any forbidden statii, then fail
	if( self->getStatusBits().testForAny( data->m_forbiddenStatus ) )
		return FALSE;

	if(!self->testCustomStatusForAll( data->m_requiredCustomStatus ))
		return FALSE;

	for(std::vector<AsciiString>::const_iterator it = data->m_forbiddenCustomStatus.begin(); it != data->m_forbiddenCustomStatus.end(); ++it)
	{
		if(self->testCustomStatus(*it))
			return FALSE;
	}

	//Do a quick preliminary test to see if we are restricted by firing particular weapons and we fired a shot last frame or this frame.
	if( flags & STEALTH_NOT_WHILE_FIRING_WEAPON && self->getStatusBits().test( OBJECT_STATUS_IS_FIRING_WEAPON ) )
	{
		if( (flags & STEALTH_NOT_WHILE_FIRING_WEAPON) == STEALTH_NOT_WHILE_FIRING_WEAPON )
		{
			//Not allowed to stealth while firing ANY weapon!
			return FALSE;
		}

		//Now do weapon specific checks.
		Weapon *weapon;
		UnsignedInt lastFrame = TheGameLogic->getFrame() - 1;

		if( flags & STEALTH_NOT_WHILE_FIRING_PRIMARY )
		{
			//Check primary weapon status
			weapon = self->getWeaponInWeaponSlot( PRIMARY_WEAPON );
			if( weapon && weapon->getLastShotFrame() >= lastFrame )
			{
				return FALSE;
			}
		}
		if( flags & STEALTH_NOT_WHILE_FIRING_SECONDARY )
		{
			//Check secondary weapon status
			weapon = self->getWeaponInWeaponSlot( SECONDARY_WEAPON );
			if( weapon && weapon->getLastShotFrame() >= lastFrame )
			{
				return FALSE;
			}
		}
		if( flags & STEALTH_NOT_WHILE_FIRING_TERTIARY )
		{
			//Check tertiary weapon status
			weapon = self->getWeaponInWeaponSlot( TERTIARY_WEAPON );
			if( weapon && weapon->getLastShotFrame() >= lastFrame )
			{
				return FALSE;
			}
		}
		if( flags & STEALTH_NOT_WHILE_FIRING_FOUR )
		{
			//Check tertiary weapon status
			weapon = self->getWeaponInWeaponSlot( WEAPON_FOUR );
			if( weapon && weapon->getLastShotFrame() >= lastFrame )
			{
				return FALSE;
			}
		}
		if( flags & STEALTH_NOT_WHILE_FIRING_FIVE )
		{
			//Check tertiary weapon status
			weapon = self->getWeaponInWeaponSlot( WEAPON_FIVE );
			if( weapon && weapon->getLastShotFrame() >= lastFrame )
			{
				return FALSE;
			}
		}
		if( flags & STEALTH_NOT_WHILE_FIRING_SIX )
		{
			//Check tertiary weapon status
			weapon = self->getWeaponInWeaponSlot( WEAPON_SIX );
			if( weapon && weapon->getLastShotFrame() >= lastFrame )
			{
				return FALSE;
			}
		}
		if( flags & STEALTH_NOT_WHILE_FIRING_SEVEN )
		{
			//Check tertiary weapon status
			weapon = self->getWeaponInWeaponSlot( WEAPON_SEVEN );
			if( weapon && weapon->getLastShotFrame() >= lastFrame )
			{
				return FALSE;
			}
		}
		if( flags & STEALTH_NOT_WHILE_FIRING_EIGHT )
		{
			//Check tertiary weapon status
			weapon = self->getWeaponInWeaponSlot( WEAPON_EIGHT );
			if( weapon && weapon->getLastShotFrame() >= lastFrame )
			{
				return FALSE;
			}
		}
	}

	const Object *containedBy = self->getContainedBy();
	if( containedBy )
	{
		ContainModuleInterface *contain = containedBy->getContain();
		if( contain && !contain->isGarrisonable() )
		{
			return FALSE;
		}
	}

  //new past-alpha feature, grr...
	if( flags & STEALTH_NOT_WHILE_RIDERS_ATTACKING )
	{
    ContainModuleInterface *myContain = self->getContain();
    if ( myContain && myContain->isPassengerAllowedToFire() )
    {
      if ( myContain->isAnyRiderAttacking() )
        return FALSE;

    }
  }



	const PhysicsBehavior *physics = self->getPhysics();
	if ((flags & STEALTH_NOT_WHILE_MOVING) && physics != nullptr &&
					physics->getVelocityMagnitude() > getStealthUpdateModuleData()->m_stealthSpeed)
		return FALSE;

	if( self->testScriptStatusBit(OBJECT_STATUS_SCRIPT_UNSTEALTHED))
	{
		//We can't stealth because a script disabled this ability for this object!
		return FALSE;
	}

	return TRUE;
}


//---------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
void StealthUpdate::hintDetectableWhileUnstealthed()
{
	Object *self = getObject();
	const StealthUpdateModuleData *md = getStealthUpdateModuleData();

	if( self && md->m_hintDetectableStates.testForAny( self->getStatusBits() ) )
	{
		if ( self->getControllingPlayer() == rts::getObservedOrLocalPlayer() )
		{
			Drawable *selfDraw = self->getDrawable();
			if ( selfDraw )
				selfDraw->setSecondMaterialPassOpacity( 1.0f );
		}
	}
}



//-------------------------------------------------------------------------------


Real StealthUpdate::getFriendlyOpacity() const
{
	return getStealthUpdateModuleData()->m_friendlyOpacityMin;
}


//=============================================================================
// indicate how the given unit is "stealthed" with respect to a given player.
StealthLookType StealthUpdate::calcStealthedStatusForPlayer(const Object* obj, const Player* player)
{
	/*
		for stealthy things, there are these distinct "logical" states:

		-- not stealthed at all (ie, totally visible)
		-- stealthed
		-- stealthed-but-detected

		and the following visual states:

		-- normal (n)
		-- invisible (i)
		-- stealthed-but-visible-to-friendly-folks (sv)
		-- stealthed-but-visible-to-everyone-due-to-being-detected (sd)

		Let's be ubergeeks and make a matrix of the possibilities:

								Ally		Nonally
		normal:			(n)			(n)
		stealthed:	(sv)		(i)
		detected:		(sd)		(sd)


		Or, to put it another way:

		If normal, you always appear normal.
		If stealthed (and not detected), you appear as (sv) to allies and (i) to others.
		If detected, you always appears as (sd).

		Sorry, there is one more condition, stealthed, but visible to friendly folks, YET detected
		In this state we render outselves visible and we ovlerlay the detection effect as a warning
		we'll call this STEALTHLOOK_VISIBLE_FRIENDLY_DETECTED


	*/


	if (obj->isEffectivelyDead())
		return STEALTHLOOK_NONE;			// making sure he turns visible when he dies

	if( obj->getStatusBits().test( OBJECT_STATUS_STEALTHED ) )
	{
		const Team* team = obj->getTeam();
		Relationship r = team ? team->getRelationship(player->getDefaultTeam()) : NEUTRAL;
		if( !player->isPlayerActive() )
		{
			//Observer players are friends to everyone!
			r = ALLIES;
		}

// srj sez: disguised stuff doesn't work well when combined with the normal "detected" stuff.
// so special case it here.
		if (canDisguise() && !getStealthUpdateModuleData()->m_canStealthWhileDisguised)
		{
			if (r != ALLIES && isDisguised())
				return STEALTHLOOK_DISGUISED_ENEMY;
			else
				return STEALTHLOOK_NONE;
		}

		if( obj->getStatusBits().test( OBJECT_STATUS_DETECTED ) )			// we're detected.
		{
			if (r == ALLIES)// if we're friendly to the given player, detection DOES matter though.
				return STEALTHLOOK_VISIBLE_FRIENDLY_DETECTED;
			else
				return STEALTHLOOK_VISIBLE_DETECTED;
		}
		else
		{
			if (r == ALLIES)
			{
				// if we're friendly to the given player, detection doesn't matter.
				return STEALTHLOOK_VISIBLE_FRIENDLY;
			}
			else
			{
// srj sez: disguised stuff doesn't work well when combined with the normal "detected" stuff.
// so special case it above.
//				if( getStealthUpdateModuleData()->m_teamDisguised )
//				{
//					return STEALTHLOOK_DISGUISED_ENEMY;
//				}
				// we're effectively hidden.
				return STEALTHLOOK_INVISIBLE;
			}
		}
	}
	else
	{
		return STEALTHLOOK_NONE;
	}
}

//-------------------------------------------------------------------------------------------------
Object* StealthUpdate::calcStealthOwner()
{
	const StealthUpdateModuleData *data = getStealthUpdateModuleData();
	//If we are going to use the rider for stealth rules, then we need to separate the
	//rider and the container. The rider will determine if the container is stealthed or
	//not.
	if( data->m_useRiderStealth )
	{
		//We're actually going to logically check the rider as the stealth owner, but the
		//stealth effects will go on the container.
		ContainModuleInterface *contain = getObject()->getContain();
		if( contain )
		{
			const ContainedItemsList *riderList = contain->getContainedItemsList();
			ContainedItemsList::const_iterator riderIterator;
			riderIterator = riderList->begin();
			if( riderIterator != riderList->end() )
			{
				//Return this rider!
				return *riderIterator;
			}
		}
	}
	//Not applicable, return ourself.
	return getObject();
}

void StealthUpdate::refreshUpdate()
{
	m_updatePulse = false;
	m_updatePulseOnly = false;
	m_nextWakeUpFrame = 1;
	setWakeFrame(getObject(), UPDATE_SLEEP_NONE);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
UpdateSleepTime StealthUpdate::calcSleepTime() const
{
	if(!m_enabled)
	{
		m_nextWakeUpFrame = 0;
		return UPDATE_SLEEP_FOREVER;
	}

	// Update for one instance
	if(m_nextWakeUpFrame == 1)
	{
		//DEBUG_LOG(("ObjectName: %s. ID: %d. nextWakeUpFrame 1, cleared to 0. Frame: %d.", getObject()->getTemplate()->getName().str(), getObject()->getID(), TheGameLogic->getFrame()));
		m_nextWakeUpFrame = 0;
		return UPDATE_SLEEP_NONE;
	}

	// Disguise transitioning, we need to update every frame
	//if(m_disguiseTransitionFrames || !m_transitioningToDisguise)
	//	return UPDATE_SLEEP_NONE;

	UnsignedInt now = TheGameLogic->getFrame();

	// Checking if there are any input commands for player from receiving Grant
	// Added a check to AIUpdateInterface::aiDoCommand()
	//if(m_framesGranted >= now)
	//	return UPDATE_SLEEP_NONE;

	if(now >= m_nextWakeUpFrame || m_nextWakeUpFrame > m_framesGranted)
	{
		if(m_framesGranted && m_framesGranted > now)
			m_nextWakeUpFrame = m_framesGranted;
	}

	if(now >= m_nextWakeUpFrame || m_nextWakeUpFrame > m_stealthAllowedFrame)
	{
		if(m_stealthAllowedFrame && m_stealthAllowedFrame > now)
			m_nextWakeUpFrame = m_stealthAllowedFrame;
	}

	if(now >= m_nextWakeUpFrame || m_nextWakeUpFrame > m_detectionExpiresFrame)
	{
		if(m_detectionExpiresFrame && m_detectionExpiresFrame > now)
			m_nextWakeUpFrame = m_detectionExpiresFrame;
	}

	if(now >= m_nextWakeUpFrame || m_nextWakeUpFrame > m_flickerFrame + 1)
	{
		if(m_flickerFrame && m_flickerFrame  + 1 > now)
			m_nextWakeUpFrame = m_flickerFrame + 1;
	}

	if(now >= m_nextWakeUpFrame || m_nextWakeUpFrame > m_nextBlackMarketCheckFrame + 1)
	{
		if(m_nextBlackMarketCheckFrame && m_nextBlackMarketCheckFrame  + 1 > now)
			m_nextWakeUpFrame = m_nextBlackMarketCheckFrame + 1;
	}

	UnsignedInt UpdateTime = m_nextWakeUpFrame > now ? m_nextWakeUpFrame - now : UPDATE_SLEEP_FOREVER;

	// If we are detected, but it has passed our detection duration, we need to update next frame and remove it.
	if(getObject()->getStatusBits().test( OBJECT_STATUS_DETECTED ) && now >= m_detectionExpiresFrame && UpdateTime == UPDATE_SLEEP_FOREVER)
		return UPDATE_SLEEP_NONE;

	// Need to update it for an extra frame after the supposed Update Time for StealthUpdate to work properly for sleepy updates
	// The wake up frame is set to 1 for the next update after this one.
	if(m_nextWakeUpFrame > 1)
	{
		//DEBUG_LOG(("ObjectName: %s. ID: %d. Set nextWakeUpFrame to 1. Frame: %d. NextWakeUpFrame: %d", getObject()->getTemplate()->getName().str(), getObject()->getID(), now, m_nextWakeUpFrame));
		m_updatePulseOnly = false;
		m_nextWakeUpFrame = 1;
	}

	// Always update for pulsePhase
	if(m_updatePulse)
		return UPDATE_SLEEP_NONE;
	else
		return UPDATE_SLEEP( UpdateTime ); //UPDATE_SLEEP_NONE : UPDATE_SLEEP_FOREVER;
}

//-------------------------------------------------------------------------------------------------
/** The update callback. */
//-------------------------------------------------------------------------------------------------
UpdateSleepTime StealthUpdate::update( void )
{

	// restore disguise if we need to from a game load
	if( m_xferRestoreDisguise == TRUE )
	{
		Drawable *draw = getObject()->getDrawable();
		Bool wasHidden = FALSE;

		// hack! if drawable was hidden (such as if we're inside a container) we must keep that state
		if( draw && draw->isDrawableEffectivelyHidden() )
			wasHidden = TRUE;

		// do the change (we get a new drawable from this)
		changeVisualDisguise();

		// restore hidden state in the new drawable
		draw = getObject()->getDrawable();
		if( wasHidden && draw )
			draw->setDrawableHidden( TRUE );

	}

	Object *self = getObject();
	Object *stealthOwner = calcStealthOwner();

	UnsignedInt stealthDelay;

	if( self == stealthOwner )
	{
		const StealthUpdateModuleData *data = getStealthUpdateModuleData();
		stealthDelay = data->m_stealthDelay;
	}
	else
	{
		//Extract the rules from the rider's stealthupdate module data instead
		//of our own, because the rider determines if the container can stealth or not.
    const StealthUpdate *stealthUpdate = stealthOwner->getStealth();
		if( stealthUpdate )
		{
			stealthDelay = stealthUpdate->getStealthDelay();
		}
	}

	UnsignedInt now = TheGameLogic->getFrame();

/// @todo srj -- improve sleeping behavior. we currently just sleep when not enabled,
// and demand every-frame attention when enabled. this could probably be smartened.
// IamInnocent - Done. Made Sleepy.

	if( !m_enabled )
	{
		return calcSleepTime();
	}

	const StealthUpdateModuleData *data = getStealthUpdateModuleData();
	Drawable* draw = self->getDrawable();

	Bool isDetected = self->getStatusBits().test( OBJECT_STATUS_DETECTED ) || m_detectionExpiresFrame > now;

	if( draw )
	{
		//Are we disguise transitioning (either gaining or losing disguise look?)
		/** @todo srj -- evil hack here... this whole heat-vision thing is fucked.
			don't want it on mines but no good way to do that. hack for now. */
		if (self->isKindOf(KINDOF_MINE))
		{
			// special case for mines
			draw->setEffectiveOpacity( 0.0f, 0.0f );
		}
		else if( m_disguiseTransitionFrames )
		{
			m_disguiseTransitionFrames--;
			Real factor;
			if( m_transitioningToDisguise )
			{
				factor = 1.0f - ( (Real)m_disguiseTransitionFrames / (Real)data->m_disguiseTransitionFrames );
			}
			else if(m_disguiseTransitionIsFlicking)
			{
				factor = 1.0f - ( (Real)m_disguiseTransitionFrames / (Real)data->m_disguiseFlickerTransitionTime );
			}
			else
			{
				factor = 1.0f - ( (Real)m_disguiseTransitionFrames / (Real)data->m_disguiseRevealTransitionFrames );
			}
			if( factor >= 0.5f && !m_disguiseHalfpointReached )
			{
			  if(m_flicking && isDisguised())
			  {
				m_flicking = false;
			  }
			  else
			  {
				//Switch models at the halfway point
				if(m_disguiseTransitionIsFlicking)
					changeVisualDisguiseFlicker(m_flicked);
				else
					changeVisualDisguise();
			  }
				// TheSuperHackers @fix Skyaero 06/05/2025 obtain the new drawable
				draw = getObject()->getDrawable();


				m_disguiseHalfpointReached = true;
			}
			//Opacity ranges from full to none at midpoint and full again at the end
			Real opacity = fabs( 1.0f - (factor * 2.0f) );
			Real overrideOpacity = opacity < 1.0f ? 0.0f : 1.0f;

			// IamInnocent - Don't change opacity if we are flickering for non-Allies
			Player *clientPlayer = ThePlayerList->getLocalPlayer();
			if(!m_disguiseTransitionIsFlicking || ( self->getControllingPlayer()->getRelationship( clientPlayer->getDefaultTeam() ) == ALLIES && clientPlayer->isPlayerActive() ) )
				draw->setEffectiveOpacity( opacity, overrideOpacity );

			if( !m_disguiseTransitionFrames && !m_disguiseTransitionIsFlicking && !m_transitioningToDisguise && !data->m_canStealthWhileDisguised )
			{
				Bool hasAutoDisguise = (data->m_autoDisguiseWhenAvailable && m_lastDisguiseAsTemplate) || data->m_disguiseFriendlyFlickerDelay;
				
				//We're finished removing disguise so turn off stealth update.
				if(!hasAutoDisguise)
					m_enabled = false;

				if(!hasAutoDisguise || !isDetected)
				{
					self->clearStatus( MAKE_OBJECT_STATUS_MASK( OBJECT_STATUS_STEALTHED ) );
					self->clearStatus( MAKE_OBJECT_STATUS_MASK( OBJECT_STATUS_DETECTED ) );
					m_markForClearStealthLater = FALSE;
				}

				if(m_isNotAutoDisguise)
				{
					self->clearStatus( MAKE_OBJECT_STATUS_MASK( OBJECT_STATUS_DETECTED ) );
				}

				return calcSleepTime();
			}

			// Always update for disguise
			m_nextWakeUpFrame = 1;
		}
		else if(!isDisguised() || data->m_canStealthWhileDisguised)
		{
			// Always update for pulsePhase
			m_updatePulse = true;

			draw->setEffectiveOpacity( 0.5f + ( Sin( m_pulsePhase ) * 0.5f ) );
			// between one half and full opacity
			m_pulsePhase += m_pulsePhaseRate;

			// We stop here if we only update Pulse Phase
			if(m_updatePulseOnly)
				return calcSleepTime();
		}
	}

/// @todo srj -- do we need to do this EVERY frame?
	Real revealDistance = getRevealDistanceFromTarget();
	if( revealDistance > 0.0f )
	{
		AIUpdateInterface *ai = self->getAI();
		if( ai )
		{
			Object *target = ai->getCurrentVictim();
			if( target )
			{
				Real distSqrd = ThePartitionManager->getDistanceSquared( self, target, FROM_CENTER_2D );
				if( distSqrd <= revealDistance * revealDistance )
				{
					//We're close enough to reveal ourselves
					markAsDetected();
					return calcSleepTime();
				}
			}
		}
	}

	//Deal with temporary stealth.
	//if( m_framesGranted > 0 )
	if( m_framesGranted >= now )
	{
		//m_framesGranted--;

		//If the last AI command given was by the player... then LOSE the stealth now!
		AIUpdateInterface *ai = self->getAI();
		if( ai )
		{
			if( ai->getLastCommandSource() == CMD_FROM_PLAYER )
			{
				//No exploits :)
				receiveGrant( FALSE );
			}
		}
		if( m_framesGranted == 0 || m_framesGranted == now)
		{
			//Disable it now that it has officially expired.
			receiveGrant( FALSE );
		}
	}

	if( allowedToStealth( stealthOwner ) )
	{
		// If I can stealth, don't attempt to Stealth until the timer is zero.
		if( m_stealthAllowedFrame > now )
		{
			if(m_markForClearStealthLater && !isDetected)
			{
				self->clearStatus( MAKE_OBJECT_STATUS_MASK( OBJECT_STATUS_STEALTHED ) );
				m_markForClearStealthLater = FALSE;
			}
			return calcSleepTime();
		}

		m_markForClearStealthLater = FALSE;

		// If we haven't stealthed yet( still destealthed ), play stealthOn here
		//if ( ( self->getStatusBits() && OBJECT_STATUS_STEALTHED ) == 0 )
		if( !self->getStatusBits().test( OBJECT_STATUS_STEALTHED ) )
		{
			AudioEventRTS soundEvent = *self->getTemplate()->getSoundStealthOn();
			soundEvent.setObjectID(self->getID());
			TheAudio->addAudioEvent( &soundEvent );
		}

		if(isDisguised() && !m_disguiseTransitionFrames && !isDetected && data->m_disguiseFriendlyFlickerDelay && now > m_flickerFrame && getObject()->getDrawable() )
		{
			m_flickerFrame = now + data->m_disguiseFriendlyFlickerDelay;
			if(data->m_disguiseFlickerTransitionTime > 0)
			{
				m_disguiseTransitionFrames	= data->m_disguiseFlickerTransitionTime;
				m_disguiseHalfpointReached  = false;
				m_disguiseTransitionIsFlicking = true;

				// Update next frame
				m_nextWakeUpFrame = 1;
			}
			else
			{
				changeVisualDisguiseFlicker(m_flicked);
			}
		}

		// IamInnocent - New feature, enable disguised objects to switch back to their disguise after stealth delay
		if(data->m_autoDisguiseWhenAvailable && !m_disguiseTransitionFrames && !isDetected )
		{
			m_isNotAutoDisguise = false;
			if(data->m_disguiseRetainAfterDetected)
			{
				if(isDisguised() && !self->testStatus( OBJECT_STATUS_DISGUISED ))
					self->setStatus( MAKE_OBJECT_STATUS_MASK( OBJECT_STATUS_DISGUISED ) );
			}
			else if(canDisguise() && !isDisguised() && m_lastDisguiseAsTemplate)
			{
				disguiseAsObject( nullptr, nullptr, TRUE );
			}
		}

		// The timer is zero, so if we aren't stealthed, do so now!
		self->setStatus( MAKE_OBJECT_STATUS_MASK( OBJECT_STATUS_STEALTHED ) );
	}
	else
	{
		m_stealthAllowedFrame = now + stealthDelay;

		// Add flicker frame so that it doesnt flicker instantly
		UnsignedInt delayFrame = m_stealthAllowedFrame + data->m_disguiseFriendlyFlickerDelay;
		if(delayFrame > m_flickerFrame)
			m_flickerFrame = delayFrame;

		// if you are destealthing on your own free will, play sound for all to hear
		if( !m_markForClearStealthLater && self->getStatusBits().test( OBJECT_STATUS_STEALTHED ) )
		{
			AudioEventRTS soundEvent = *self->getTemplate()->getSoundStealthOn();
			soundEvent.setObjectID(self->getID());
			TheAudio->addAudioEvent( &soundEvent );
		}

		//If we are disguised, remove the disguise permanently!
		if( isDisguised() )
		{
			disguiseAsObject( nullptr );

			// Also the last GUI to be preserved because selection overrides the current GUI
			m_preserveLastGUI = true;
		}

		// Fix Disguises auto Disguise back when detected
		if(canDisguise() && data->m_autoDisguiseWhenAvailable && m_lastDisguiseAsTemplate && isDetected)
		{
			m_markForClearStealthLater = TRUE;
		}
		else
		{
			self->clearStatus( MAKE_OBJECT_STATUS_MASK( OBJECT_STATUS_STEALTHED ) );
			m_markForClearStealthLater = FALSE;
		}

		hintDetectableWhileUnstealthed();
	}

	Bool detectedStatusChangedThisFrame = FALSE;
	if (m_detectionExpiresFrame > now)
	{
		// if this is the first time being detected, play stealth off sound
		if( !self->getStatusBits().test( OBJECT_STATUS_DETECTED ) )
		{
			detectedStatusChangedThisFrame = TRUE;
			AudioEventRTS soundEvent = *self->getTemplate()->getSoundStealthOff();
			soundEvent.setObjectID(self->getID());
			TheAudio->addAudioEvent( &soundEvent );
		}

		self->setStatus( MAKE_OBJECT_STATUS_MASK( OBJECT_STATUS_DETECTED ) );

		//If we are disguised, remove the disguise permanently!
		if( !m_isNotAutoDisguise && isDisguised() && data->m_autoDisguiseWhenAvailable )
		{
			// Retain the model even after deteccted
			if(data->m_disguiseRetainAfterDetected )
			{
				self->clearStatus( MAKE_OBJECT_STATUS_MASK( OBJECT_STATUS_DISGUISED ) );
			}
			else
			{
				disguiseAsObject( nullptr );

				// Also the last GUI to be preserved because selection overrides the current GUI
				// IamInnocent - This is a switch up improvisation, because when Bomb Truck is exposed,
				//				 The systems exposes the Bomb Truck and 'Selects' it, interrupting ANY Recent Pending Commands.
				//				 I set this as an adjustable feature because Mirage Tank does not have any interruptable GUI when exposed.
				if(data->m_preservePendingCommandWhenDetected)
					m_preserveLastGUI = true;
			}
		}

		if(TheGlobalData->m_useEfficientDrawableScheme && draw && ThePlayerList->getLocalPlayer()->getRelationship(self->getTeam()) == ENEMIES)
		{
			// Redraw everything as Stealth Detection bugs out how existing Drawables work
			TheGameClient->clearEfficientDrawablesList();
		}
	}
	else
	{
		// if this is the first time your clearing the detected status, play the stealth on sound
		if( self->getStatusBits().test( OBJECT_STATUS_DETECTED ) )
		{
			detectedStatusChangedThisFrame = TRUE;
			//Only play sound effect if the selected object is controllable.
			if( self->isLocallyControlled() )
			{
				AudioEventRTS soundEvent = *self->getTemplate()->getSoundStealthOn();
				soundEvent.setObjectID(self->getID());
				TheAudio->addAudioEvent( &soundEvent );
			}
		}

		self->clearStatus( MAKE_OBJECT_STATUS_MASK( OBJECT_STATUS_DETECTED ) );
	}

	if ( detectedStatusChangedThisFrame )
	{
		//do the trick where we tell our container to recals his apparent controlling player
		//since I may have just become either detected or undetected
		if ( self->isContained() )
		{
			Object *container = self->getContainedBy();
			if ( container )
			{
				ContainModuleInterface *contain = container->getContain();
				if( contain && contain->isGarrisonable() )
				{
					contain->recalcApparentControllingPlayer();
				}
			}
		}

	}



	if (draw)
	{
		StealthLookType stealthLook = calcStealthedStatusForPlayer( self, rts::getObservedOrLocalPlayer() );
		draw->setStealthLook( stealthLook );
	}

	if(!m_nextWakeUpFrame)
		m_updatePulseOnly = true;

	return calcSleepTime();
}

//-------------------------------------------------------------------------------------------------
void setWakeupIfInRange( Object *obj, void *userData)
{
	Object *victim = (Object *)userData;
	AIUpdateInterface *ai = obj->getAI();
	if (!ai) {
		return;
	}

	Real vision = obj->getVisionRange();

	Coord3D srcpos = *obj->getPosition();
	Coord3D dstpos = *victim->getPosition();

	srcpos.sub(&dstpos);
	if (srcpos.length() > vision)
		return;

	ai->wakeUpAndAttemptToTarget();

//	if( obj->isKindOf( KINDOF_SELECTABLE ) && ( obj->isAbleToAttack() || !obj->isKindOf( KINDOF_STRUCTURE ) )) {
//		Drawable *draw = obj->getDrawable();
//		if( draw ) {
//			draw->setEmoticon( "Emoticon_Alarm", 5000 );
//		}
//	}
}


//-------------------------------------------------------------------------------------------------
void StealthUpdate::markAsDetected(UnsignedInt numFrames)
{
	Object *self = getObject();
	Object *stealthOwner = calcStealthOwner();

	UnsignedInt stealthDelay, orderIdlesToAttack;
	const StealthUpdateModuleData *data = getStealthUpdateModuleData();
	if( self == stealthOwner )
	{
		//const StealthUpdateModuleData *data = getStealthUpdateModuleData();
		//Use the standard module data information (because we stealth ourself)
		stealthDelay = data->m_stealthDelay;
		orderIdlesToAttack = data->m_orderIdleEnemiesToAttackMeUponReveal;
	}
	else
	{
		//Extract the rules from the rider's stealthupdate module data instead
		//of our own, because the rider determines if the container can stealth or not.
    const StealthUpdate *stealthUpdate = stealthOwner->getStealth();
		if( stealthUpdate )
		{
			stealthDelay = stealthUpdate->getStealthDelay();
			orderIdlesToAttack = stealthUpdate->getOrderIdleEnemiesToAttackMeUponReveal();
		}
	}


	Player *thisPlayer = self->getControllingPlayer();
	UnsignedInt now = TheGameLogic->getFrame();

	//If we are disguised, remove the disguise permanently!
	if( isDisguised() )
	{
		// Retain the model even after deteccted
		if(data->m_disguiseRetainAfterDetected )
		{
			self->clearStatus( MAKE_OBJECT_STATUS_MASK( OBJECT_STATUS_DISGUISED ) );
		}
		else
		{
			disguiseAsObject( nullptr );

			// Also the last GUI to be preserved because selection overrides the current GUI
			// IamInnocent - This is a switch up improvisation, because when Bomb Truck is exposed,
			//				 The systems exposes the Bomb Truck and 'Selects' it, interrupting ANY Recent Pending Commands.
			//				 I set this as an adjustable feature because Mirage Tank does not have any interruptable GUI when exposed.
			if(data->m_preservePendingCommandWhenDetected)
				m_preserveLastGUI = true;
		}
	}

	m_isNotAutoDisguise = false;

	if( !numFrames )
	{
		//Kris:
		//If numFrames is zero (the default value), use the stealth delay specified in the ini file.
		m_detectionExpiresFrame = now + stealthDelay;
	}
	else if ( m_detectionExpiresFrame < now + numFrames )
	{
		m_detectionExpiresFrame = now + numFrames;
	}

	// Add flicker frame so that it doesnt flicker instantly
	UnsignedInt delayFrame = m_detectionExpiresFrame + data->m_disguiseFriendlyFlickerDelay;
	if(delayFrame > m_flickerFrame)
		m_flickerFrame = delayFrame;

	if( orderIdlesToAttack )
	{
		// This can't be a partitionmanager thing, because we need to know which objects can see
		// us. Therefore, walk the play list, and for each player that considers us an enemy,
		// check if any of their units can see us.

		Int numPlayers = ThePlayerList->getPlayerCount();
		for (Int n = 0; n < numPlayers; ++n)
		{

			Player *player = ThePlayerList->getNthPlayer(n);
			if (!player)
				continue;

			if (player->getRelationship(thisPlayer->getDefaultTeam()) != ENEMIES)
				continue;

			player->iterateObjects(setWakeupIfInRange, self);
		}
	}

	refreshUpdate();
	//setWakeFrame( getObject(), UPDATE_SLEEP_NONE );
}

//-------------------------------------------------------------------------------------------------
void StealthUpdate::disguiseAsObject( const Object *target, const Drawable *drawTemplate, Bool doLast )
{
	Object *self = getObject();
	const StealthUpdateModuleData *data = getStealthUpdateModuleData();
	if( target && target->getControllingPlayer() )
	{
    StealthUpdate* stealth = target->getStealth();
		m_disguiseModelName.clear();

		if( stealth && stealth->getDisguisedTemplate() )
		{
			m_disguiseAsTemplate				= stealth->getDisguisedTemplate();
			m_disguiseAsPlayerIndex			= stealth->getDisguisedPlayerIndex();
			m_disguiseModelName				= stealth->getDisguisedModelName();
		}
		else
		{
			m_disguiseAsTemplate				= target->getTemplate();
			m_disguiseAsPlayerIndex			= target->getControllingPlayer()->getPlayerIndex();
		}

		m_enabled										= true;
		m_transitioningToDisguise		= true; //Means we are gaining disguise over time.
		m_disguiseTransitionFrames	= data->m_disguiseTransitionFrames;
		m_disguiseHalfpointReached  = false;

		m_lastDisguiseAsTemplate = m_disguiseAsTemplate;
		m_lastDisguiseAsPlayerIndex = m_disguiseAsPlayerIndex;

		m_isNotAutoDisguise = true;

		//Wake up so I can process!
		refreshUpdate();
		//setWakeFrame( getObject(), UPDATE_SLEEP_NONE );

	}
	else if(drawTemplate)
	{
		m_lastDisguiseAsTemplate				= drawTemplate->getTemplate();
		m_disguiseAsPlayerIndex			= ThePlayerList->getNeutralPlayer()->getPlayerIndex();

		m_disguiseModelName.clear();

		if(m_lastDisguiseAsTemplate->isKindOf(KINDOF_SHRUBBERY))
		{
			m_disguiseModelName	= drawTemplate->getModelName();
			const ThingTemplate *givenTemplate = TheThingFactory->findTemplate( "GenericTree" );
			if(givenTemplate)
				m_disguiseAsTemplate = givenTemplate;
		}

		m_enabled										= true;
		m_transitioningToDisguise		= true; //Means we are gaining disguise over time.
		m_disguiseTransitionFrames	= data->m_disguiseTransitionFrames;
		m_disguiseHalfpointReached  = false;

		m_lastDisguiseAsTemplate = m_disguiseAsTemplate;
		m_lastDisguiseAsPlayerIndex = m_disguiseAsPlayerIndex;

		m_isNotAutoDisguise = true;

		//Wake up so I can process!
		refreshUpdate();
		//setWakeFrame( getObject(), UPDATE_SLEEP_NONE );
	}
	else if( doLast )
	{
		m_disguiseAsTemplate			= m_lastDisguiseAsTemplate;
		m_disguiseAsPlayerIndex			= m_lastDisguiseAsPlayerIndex;
		
		m_enabled										= true;
		m_transitioningToDisguise		= true; //Means we are gaining disguise over time.
		m_disguiseTransitionFrames	= data->m_disguiseTransitionFrames;
		m_disguiseHalfpointReached  = false;

		m_preserveLastGUI = true;

		//Wake up so I can process!
		refreshUpdate();
		//setWakeFrame( getObject(), UPDATE_SLEEP_NONE );
	}
	else if( m_disguised )
	{
		m_disguiseAsTemplate				= nullptr;
		m_disguiseAsPlayerIndex			= 0;
		m_disguiseTransitionFrames	= data->m_disguiseRevealTransitionFrames;
		m_transitioningToDisguise		= false; //Means we are losing the disguise over time.
		m_disguiseHalfpointReached  = false;
	}

	Drawable *draw = self->getDrawable();
	if( draw && draw->isSelected() )
	{
		TheControlBar->markUIDirty();
	}

	// Remove flicking status to change drawable
	m_flicking = false;
	m_disguiseTransitionIsFlicking = false;

}

//-------------------------------------------------------------------------------------------------
void StealthUpdate::changeVisualDisguise()
{
	Object *self = getObject();
	const StealthUpdateModuleData *data = getStealthUpdateModuleData();

	Drawable *draw = self->getDrawable();
	// We need to maintain our selection across the un/disguise, so pull selected out here.
	Bool selected = draw->isSelected();

	if( m_disguiseAsTemplate )
	{
		// Set the firing offsets for compatibility with railguns
		if(!m_originalDrawableTemplate)
			SetFiringOffsets(FALSE);

		Player *player = ThePlayerList->getNthPlayer( m_disguiseAsPlayerIndex );

		ModelConditionFlags flags = draw->getModelConditionFlags();

		//Get rid of the old instance!
		if(m_preserveLastGUI)
			TheGameClient->destroyDrawablePreserveGUI( draw );
		else
			TheGameClient->destroyDrawable( draw );

		draw = TheThingFactory->newDrawable( m_disguiseAsTemplate );
		if( draw )
		{
			// If we are disguised with Draw Template, we overwrite the model
			if(!m_disguiseModelName.isEmpty())
				draw->setModelName(m_disguiseModelName);

			TheGameLogic->bindObjectAndDrawable(self, draw);
			draw->setPosition( self->getPosition() );
			draw->setOrientation( self->getOrientation() );
			draw->setModelConditionFlags( flags );
			draw->updateDrawable();
			self->getPhysics()->resetDynamicPhysics();
			if( selected )
			{
				if(m_preserveLastGUI)
					TheInGameUI->selectDrawablePreserveGUI( draw, TRUE );
				else
					TheInGameUI->selectDrawable( draw );
			}
			Player *clientPlayer = ThePlayerList->getLocalPlayer();
			if( self->getControllingPlayer()->getRelationship( clientPlayer->getDefaultTeam() ) != ALLIES && clientPlayer->isPlayerActive() )
			{
				//Neutrals and enemies will see this disguised unit as the team it's disguised as.
				if (TheGlobalData->m_timeOfDay == TIME_OF_DAY_NIGHT)
					draw->setIndicatorColor( player->getPlayerNightColor() );
				else
					draw->setIndicatorColor( player->getPlayerColor() );
			}
			else
			{
				//If it's on our team or our ally's team, then show it's true colors.
				if (TheGlobalData->m_timeOfDay == TIME_OF_DAY_NIGHT)
					draw->setIndicatorColor( self->getNightIndicatorColor() );
				else
					draw->setIndicatorColor( self->getIndicatorColor() );
			}

			SetFiringOffsets(TRUE);
		}

		//Play a disguise sound!
		AudioEventRTS sound = *self->getTemplate()->getPerUnitSound( "DisguiseStarted" );
		sound.setObjectID( self->getID() );
		TheAudio->addAudioEvent( &sound );

		FXList::doFXPos( data->m_disguiseFX, self->getPosition() );

		m_disguised = true;
		self->setStatus( MAKE_OBJECT_STATUS_MASK( OBJECT_STATUS_DISGUISED ) );
		self->setModelConditionState( MODELCONDITION_DISGUISED );

		//33) Did the player ever build a "disguisable" unit and never used the disguise ability?
		self->getControllingPlayer()->getAcademyStats()->recordVehicleDisguised();
	}
	else if( m_disguiseAsPlayerIndex != -1 )
	{
		m_disguiseAsPlayerIndex = -1;
		ModelConditionFlags flags = draw->getModelConditionFlags();

		//Get rid of the old instance!
		if(m_preserveLastGUI)
			TheGameClient->destroyDrawablePreserveGUI( draw );
		else
			TheGameClient->destroyDrawable( draw );

		const ThingTemplate *tTemplate = self->getTemplate();

		// TheSuperHackers @fix helmutbuhler 13/04/2025 Fixes missing pointer assignment for the new drawable.
		// This originally caused no runtime crash because the new drawable is allocated at the same address as the previously deleted drawable via the MemoryPoolBlob.
		draw = TheThingFactory->newDrawable( tTemplate );
		if( draw )
		{
			TheGameLogic->bindObjectAndDrawable(self, draw);
			draw->setPosition( self->getPosition() );
			draw->setOrientation( self->getOrientation() );
			draw->setModelConditionFlags( flags );
			draw->updateDrawable();
			self->getPhysics()->resetDynamicPhysics();
			if (TheGlobalData->m_timeOfDay == TIME_OF_DAY_NIGHT)
				draw->setIndicatorColor( self->getNightIndicatorColor() );
			else
				draw->setIndicatorColor( self->getIndicatorColor() );
			if( selected )
			{
				if(m_preserveLastGUI)
					TheInGameUI->selectDrawablePreserveGUI( draw, TRUE );
				else
					TheInGameUI->selectDrawable( draw );
			}

			//UGH!
			//A concrete example is the bomb truck. Different payloads are displayed based on which upgrades have been
			//made. When the bomb truck disguises as something else, these subobjects are lost because the vector is
			//stored in W3DDrawModule. When we revert back to the original bomb truck, we call this function to
			//recalculate those upgraded subobjects.
			self->forceRefreshSubObjectUpgradeStatus();

			SetFiringOffsets(FALSE);
		}

		Bool successfulReveal = false;
		AIUpdateInterface *ai = self->getAI();
		if( ai )
		{
			Object *currTarget = ai->getCurrentVictim();
			if( currTarget )
			{
				successfulReveal = true;
			}
		}

		//Play a reveal sound!
		AudioEventRTS sound;
		if( successfulReveal )
		{
			sound = *self->getTemplate()->getPerUnitSound( "DisguiseRevealedSuccess" );
		}
		else
		{
			sound = *self->getTemplate()->getPerUnitSound( "DisguiseRevealedFailure" );
		}
		sound.setObjectID( self->getID() );
		TheAudio->addAudioEvent( &sound );

		FXList::doFXPos( data->m_disguiseRevealFX, self->getPosition() );
		m_disguised = false;
		self->clearStatus( MAKE_OBJECT_STATUS_MASK( OBJECT_STATUS_DISGUISED ) );
		self->clearModelConditionState( MODELCONDITION_DISGUISED );
	}

	// Also refresh the Efficient Drawable List
	if(TheGlobalData->m_useEfficientDrawableScheme && ThePlayerList->getLocalPlayer()->getRelationship(self->getTeam()) == ENEMIES)
	{
		// Redraw everything as Stealth Detection bugs out how existing Drawables work
		TheGameClient->clearEfficientDrawablesList();
	}

	//Reset the radar (determines color on add)
	TheRadar->removeObject( self );
	TheRadar->addObject( self );

	// Remove flicking status to change drawable
	m_flicking = false;
	m_disguiseTransitionIsFlicking = false;

	// Finished with current pending GUI Command
	m_preserveLastGUI = false;

	// couldn't possibly need to restore a disguise now :)
	m_xferRestoreDisguise = FALSE;

}

//-------------------------------------------------------------------------------------------------
void StealthUpdate::changeVisualDisguiseFlicker(Bool doFlick)
{
	Object *self = getObject();

	Drawable *draw = self->getDrawable();
	// We need to maintain our selection across the un/disguise, so pull selected out here.
	Bool selected = draw->isSelected();
	Bool refresh = FALSE;

	if( m_disguiseAsTemplate )
	{
		Player *player = ThePlayerList->getNthPlayer( m_disguiseAsPlayerIndex );

		ModelConditionFlags flags = draw->getModelConditionFlags();

		//Get rid of the old instance!
		TheGameClient->destroyDrawablePreserveGUI( draw );

		Player *clientPlayer = ThePlayerList->getLocalPlayer();

		//Get the drawable to show
		const ThingTemplate  *drawTempl = nullptr;
		if( doFlick && self->getControllingPlayer()->getRelationship( clientPlayer->getDefaultTeam() ) == ALLIES && clientPlayer->isPlayerActive() )
		{
			drawTempl = self->getTemplate();
			refresh = TRUE;
		}
		else
		{
			drawTempl = m_disguiseAsTemplate;
		}

		// Create the drawable and bind it to the Object
		draw = TheThingFactory->newDrawable( drawTempl );
		if( draw )
		{
			// If we are disguised with Draw Template, we overwrite the model
			if(!m_disguiseModelName.isEmpty())
				draw->setModelName(m_disguiseModelName);

			TheGameLogic->bindObjectAndDrawable(self, draw);
			draw->setPosition( self->getPosition() );
			draw->setOrientation( self->getOrientation() );
			draw->setModelConditionFlags( flags );
			draw->updateDrawable();
			//self->getPhysics()->resetDynamicPhysics();
			if( selected )
			{
				TheInGameUI->selectDrawablePreserveGUI( draw, !getStealthUpdateModuleData()->m_dontFlashWhenFlickering );
			}
		}
		if( self->getControllingPlayer()->getRelationship( clientPlayer->getDefaultTeam() ) != ALLIES && clientPlayer->isPlayerActive() )
		{
			//Neutrals and enemies will see this disguised unit as the team it's disguised as.
			if (TheGlobalData->m_timeOfDay == TIME_OF_DAY_NIGHT)
				draw->setIndicatorColor( player->getPlayerNightColor() );
			else
				draw->setIndicatorColor( player->getPlayerColor() );
		}
		else
		{
			//If it's on our team or our ally's team, then show it's true colors.
			if (TheGlobalData->m_timeOfDay == TIME_OF_DAY_NIGHT)
				draw->setIndicatorColor( self->getNightIndicatorColor() );
			else
				draw->setIndicatorColor( self->getIndicatorColor() );
		}

		if(refresh)
		{
			//UGH!
			//A concrete example is the bomb truck. Different payloads are displayed based on which upgrades have been
			//made. When the bomb truck disguises as something else, these subobjects are lost because the vector is
			//stored in W3DDrawModule. When we revert back to the original bomb truck, we call this function to
			//recalculate those upgraded subobjects.
			self->forceRefreshSubObjectUpgradeStatus();

			// Also refresh the Efficient Drawable List
			if(TheGlobalData->m_useEfficientDrawableScheme && draw && ThePlayerList->getLocalPlayer()->getRelationship(self->getTeam()) == ENEMIES)
			{
				// Redraw everything as Stealth Detection bugs out how existing Drawables work
				TheGameClient->clearEfficientDrawablesList();
			}
		}

		//Reset the radar (determines color on add)
		TheRadar->removeObject( self );
		TheRadar->addObject( self );

		// couldn't possibly need to restore a disguise now :)
		//m_xferRestoreDisguise = FALSE;

		if(!m_disguiseTransitionIsFlicking)
			m_flicking = true;

		m_flicked = !m_flicked;
	}

}

//-------------------------------------------------------------------------------------------------
Bool StealthUpdate::isDisguisedAndCheckIfNeedOffset() const
{
	if(isDisguised() || m_disguiseTransitionFrames)
	{
		// Don't need to get offset if the disguised template is the same as the object template
		if(m_originalDrawableTemplate && m_disguisedDrawableTemplate && m_originalDrawableTemplate->getTemplate() == m_disguisedDrawableTemplate->getTemplate())
			return FALSE;

		const StealthUpdateModuleData *data = getStealthUpdateModuleData();
		// only check whether to use Original or is Flicking
		return data->m_disguiseUseOriginalFiringOffset || data->m_disguiseFriendlyFlickerDelay > 0;
	}
	else
		return false;
}

//-------------------------------------------------------------------------------------------------
void StealthUpdate::SetFiringOffsets(Bool setDisguised)
{
	Drawable *draw = getObject()->getDrawable();
	// Sanity
	if(!draw)
		return;

	ModelConditionFlags flags = draw->getModelConditionFlags();

	if(setDisguised)
	{
		if(m_disguisedDrawableTemplate)
			TheGameClient->destroyDrawable(m_disguisedDrawableTemplate);

		m_disguisedDrawableTemplate = TheThingFactory->newDrawable( draw->getTemplate() );
		if(m_disguisedDrawableTemplate)
		{
			m_disguisedDrawableTemplate->setModelConditionFlags( flags );
			m_disguisedDrawableTemplate->setDrawableHidden(TRUE);
			m_disguisedDrawableTemplate->setCanDoFXWhileHidden(TRUE);
		}

		//m_disguisedDrawableFiringOffsets.clear();
	}
	else
	{
		if(m_originalDrawableTemplate)
			TheGameClient->destroyDrawable(m_originalDrawableTemplate);

		m_originalDrawableTemplate = TheThingFactory->newDrawable( getObject()->getTemplate() );
		if(m_originalDrawableTemplate)
		{
			m_originalDrawableTemplate->setModelConditionFlags( flags );
			m_originalDrawableTemplate->setDrawableHidden(TRUE);
			m_originalDrawableTemplate->setCanDoFXWhileHidden(TRUE);
		}

		//m_originalDrawableFiringOffsets.clear();
	}

	/*Coord3D objPos = *getObject()->getPosition();
	FiringPosStruct data;
	Coord3D checkPos;
	for(int i = 0; i < WEAPONSLOT_COUNT; i++)
	{
		data.weaponSlot = i;
		data.barrelCount = draw->getBarrelCount((WeaponSlotType)i);
		data.posVec.clear();
		Bool slotEmpty = true;

		for(int j = 0; j < data.barrelCount; j++)
		{
			//Matrix3D worldTransform(true);
			//Coord3D worldPos;
			//if(Weapon::calcWeaponFirePosition(getObject(), setDisguised ? m_disguisedDrawableTemplate : m_originalDrawableTemplate, (WeaponSlotType)i, j, worldTransform, checkPos))
			if(draw->getWeaponFireOffset((WeaponSlotType)i, j, &checkPos))
			{
				//checkPos = worldPos;
				checkPos.x -= objPos.x;
				checkPos.y -= objPos.y;
				checkPos.z -= objPos.z;
				
				BarrelCoordType dataPos;
				dataPos.first = j;
				dataPos.second = checkPos;
				data.posVec.push_back(dataPos);
				slotEmpty = false;
			}
		}

		if(!slotEmpty)
		{
			if(setDisguised)
				m_disguisedDrawableFiringOffsets.push_back(data);
			else
				m_originalDrawableFiringOffsets.push_back(data);
		}
	}*/
}

//-------------------------------------------------------------------------------------------------
// Obselete: Use getWeaponFireOffset from Drawable
/*Bool StealthUpdate::getFiringOffsetWhileDisguised(WeaponSlotType wslot, Int specificBarrelToUse, Coord3D *pos) const
{
	Bool doOriginal = getStealthUpdateModuleData()->m_disguiseUseOriginalFiringOffset;
	std::vector<FiringPosStruct> checkData = doOriginal ? m_originalDrawableFiringOffsets : m_disguisedDrawableFiringOffsets;
	for(int i = 0; i < checkData.size(); i++)
	{
		if(wslot != checkData[i].weaponSlot)
			continue;

		for(int j = 0; j < checkData[i].posVec.size(); j++)
		{
			if(checkData[i].posVec[j].first == specificBarrelToUse)
			{
				Coord3D offset = checkData[i].posVec[j].second;
				pos->x = offset.x;
				pos->y = offset.y;
				pos->z = offset.z;
				// We stop here we managed to get an offset
				return true;
			}
		}
	}
	return !doOriginal;
}*/

//-------------------------------------------------------------------------------------------------
Int StealthUpdate::getBarrelCountWhileDisguised(WeaponSlotType wslot) const
{
	//std::vector<FiringPosStruct> checkData = getStealthUpdateModuleData()->m_disguiseUseOriginalFiringOffset ? m_originalDrawableFiringOffsets : m_disguisedDrawableFiringOffsets;
	//for(int i = 0; i < checkData.size(); i++)
	//{
		//if(wslot != checkData[i].weaponSlot)
		//	continue;

		// IamInnocent - Prevent crashes if the current object has more barrels than the current disguise template
		//Int BarrelCount = m_disguiseAsTemplate && m_disguisedDrawableTemplate ? m_disguisedDrawableTemplate->getBarrelCount(wslot) : 999;
		//BarrelCount = min(BarrelCount, checkData[i].barrelCount);
		//return BarrelCount;

	//}
	//return 0;
	/// Update - Verified issues, moved to Drawable for verification
	if(!m_disguiseAsTemplate || !m_disguisedDrawableTemplate)
		return 0;

	Int BarrelCount = 0;
	if(getStealthUpdateModuleData()->m_disguiseUseOriginalFiringOffset)
		BarrelCount = m_originalDrawableTemplate->getBarrelCount(wslot);
	else
		BarrelCount = min(m_originalDrawableTemplate->getBarrelCount(wslot), m_disguisedDrawableTemplate->getBarrelCount(wslot));

	return BarrelCount;
}

//-------------------------------------------------------------------------------------------------
Int StealthUpdate::getBarrelCountDisguisedTemplate(WeaponSlotType wslot) const
{
	return m_disguisedDrawableTemplate ? m_disguisedDrawableTemplate->getBarrelCount(wslot) : 0;
}

//-------------------------------------------------------------------------------------------------
Drawable *StealthUpdate::getDrawableTemplateWhileDisguised() const
{
	return getStealthUpdateModuleData()->m_disguiseUseOriginalFiringOffset ? m_originalDrawableTemplate : m_disguisedDrawableTemplate;
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void StealthUpdate::crc( Xfer *xfer )
{

	// extend base class
	UpdateModule::crc( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void StealthUpdate::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 2;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	UpdateModule::xfer( xfer );

	// stealth allowed frame
	xfer->xferUnsignedInt( &m_stealthAllowedFrame );

	// detection expires frame
	xfer->xferUnsignedInt( &m_detectionExpiresFrame );

	// enabled
	xfer->xferBool( &m_enabled );

	// pulse phase rate
	xfer->xferReal( &m_pulsePhaseRate );

	// pulse phase
	xfer->xferReal( &m_pulsePhase );

	// disguise as player index
	xfer->xferInt( &m_disguiseAsPlayerIndex );

	xfer->xferInt( &m_lastDisguiseAsPlayerIndex );

	// disguise as template
	AsciiString name = m_disguiseAsTemplate ? m_disguiseAsTemplate->getName() : AsciiString::TheEmptyString;
	AsciiString name2 = m_lastDisguiseAsTemplate ? m_lastDisguiseAsTemplate->getName() : AsciiString::TheEmptyString;
	//AsciiString name3 = m_originalDrawableTemplate ? m_originalDrawableTemplate->getTemplate()->getName() : AsciiString::TheEmptyString;
	//AsciiString name4 = m_disguisedDrawableTemplate ? m_disguisedDrawableTemplate->getTemplate()->getName() : AsciiString::TheEmptyString;
	xfer->xferAsciiString( &name );
	xfer->xferAsciiString( &name2 );
	//xfer->xferAsciiString( &name3 );
	//xfer->xferAsciiString( &name4 );
	//const ThingTemplate *drawTempl = nullptr;
	if( xfer->getXferMode() == XFER_LOAD )
	{

		m_disguiseAsTemplate = nullptr;
		m_lastDisguiseAsTemplate = nullptr;
		//m_originalDrawableTemplate = nullptr;
		//m_disguisedDrawableTemplate = nullptr;
		if( name.isEmpty() == FALSE )
		{

			m_disguiseAsTemplate = TheThingFactory->findTemplate( name );
			if( m_disguiseAsTemplate == nullptr )
			{

				DEBUG_CRASH(( "StealthUpdate::xfer - Unknown template '%s'", name.str() ));
				throw SC_INVALID_DATA;

			}

		}

		if( name2.isEmpty() == FALSE )
		{

			m_lastDisguiseAsTemplate = TheThingFactory->findTemplate( name2 );
			if( m_lastDisguiseAsTemplate == nullptr )
			{

				DEBUG_CRASH(( "StealthUpdate::xfer - Unknown template '%s'", name2.str() ));
				throw SC_INVALID_DATA;

			}

		}

		/*if( name3.isEmpty() == FALSE )
		{

			drawTempl = TheThingFactory->findTemplate( name3 );
			if( drawTempl == nullptr )
			{

				DEBUG_CRASH(( "StealthUpdate::xfer - Unknown template '%s'", name3.str() ));
				throw SC_INVALID_DATA;

			}
			else
			{
				m_originalDrawableTemplate = TheThingFactory->newDrawable( drawTempl );
			}

		}

		if( name4.isEmpty() == FALSE )
		{

			drawTempl = TheThingFactory->findTemplate( name4 );
			if( drawTempl == nullptr )
			{

				DEBUG_CRASH(( "StealthUpdate::xfer - Unknown template '%s'", name4.str() ));
				throw SC_INVALID_DATA;

			}
			else
			{
				m_disguisedDrawableTemplate = TheThingFactory->newDrawable( drawTempl );
			}

		}*/
	}

	DrawableID drawableID = m_originalDrawableTemplate ? m_originalDrawableTemplate->getID() : INVALID_DRAWABLE_ID;
	DrawableID drawableID2 = m_disguisedDrawableTemplate ? m_disguisedDrawableTemplate->getID() : INVALID_DRAWABLE_ID;
	xfer->xferDrawableID( &drawableID );
	xfer->xferDrawableID( &drawableID2 );
	if( xfer->getXferMode() == XFER_LOAD )
	{
		// reconnect the drawable pointer
		m_originalDrawableTemplate = TheGameClient->findDrawableByID( drawableID );
		m_disguisedDrawableTemplate = TheGameClient->findDrawableByID( drawableID2 );

		// sanity
		if( drawableID != INVALID_DRAWABLE_ID && m_originalDrawableTemplate == nullptr )
		{
			DEBUG_CRASH(( "StealthUpdate::xfer - Unable to find drawable for m_originalDrawableTemplate" ));
		}
		if( drawableID2 != INVALID_DRAWABLE_ID && m_disguisedDrawableTemplate == nullptr )
		{
			DEBUG_CRASH(( "StealthUpdate::xfer - Unable to find drawable for m_disguisedDrawableTemplate" ));
		}
	}

	// disguise transition frames
	xfer->xferUnsignedInt( &m_disguiseTransitionFrames );

	// disguise halfpoint reached
	xfer->xferBool( &m_disguiseHalfpointReached );

	// transitioning to disguise
	xfer->xferBool( &m_transitioningToDisguise );

	// disguised
	xfer->xferBool( &m_disguised );

	if( version >= 2 )
	{
		xfer->xferUnsignedInt( &m_framesGranted );
	}

	xfer->xferUnsignedInt( &m_stealthLevelOverride );

	xfer->xferBool( &m_flicked );

	xfer->xferBool( &m_flicking );

	xfer->xferBool( &m_disguiseTransitionIsFlicking );

	xfer->xferUnsignedInt( &m_flickerFrame );

	xfer->xferBool( &m_preserveLastGUI );

	xfer->xferBool( &m_markForClearStealthLater );

	xfer->xferBool( &m_isNotAutoDisguise );

	xfer->xferAsciiString( &m_disguiseModelName );

	/*UnsignedShort OriginalFiringOffsetsSize = m_originalDrawableFiringOffsets.size();
	UnsignedShort DisguisedFiringOffsetsSize = m_disguisedDrawableFiringOffsets.size();
	xfer->xferUnsignedShort( &OriginalFiringOffsetsSize );
	xfer->xferUnsignedShort( &DisguisedFiringOffsetsSize );

	Int weaponSlot;
	Int weaponSlot_2;
	Int barrelCount;
	Int barrelCount_2;
	Int barrelSlot;
	Int barrelSlot_2;
	Coord3D barrelOffset;
	Coord3D barrelOffset_2;
	BarrelCoordType barrelPair;
	BarrelCoordVec barrelVec;
	if( xfer->getXferMode() == XFER_SAVE )
	{
		for(std::vector<FiringPosStruct>::iterator it = m_originalDrawableFiringOffsets.begin(); it != m_originalDrawableFiringOffsets.end(); ++it)
		{
			weaponSlot = it->weaponSlot;
			xfer->xferInt( &weaponSlot );

			barrelCount = it->barrelCount;
			xfer->xferInt( &barrelCount );

			barrelVec = it->posVec;
			for(BarrelCoordVec::iterator it_2 = barrelVec.begin(); it_2 != barrelVec.end(); ++it_2 )
			{
				barrelSlot = it_2->first;
				xfer->xferInt( &barrelSlot );

				barrelOffset = it_2->second;
				xfer->xferCoord3D( &barrelOffset );
			}
		}

		for(std::vector<FiringPosStruct>::iterator it2 = m_disguisedDrawableFiringOffsets.begin(); it2 != m_disguisedDrawableFiringOffsets.end(); ++it2)
		{
			weaponSlot_2 = it2->weaponSlot;
			xfer->xferInt( &weaponSlot_2 );

			barrelCount_2 = it2->barrelCount;
			xfer->xferInt( &barrelCount_2 );

			barrelVec = it2->posVec;
			for(BarrelCoordVec::iterator it2_2 = barrelVec.begin(); it2_2 != barrelVec.end(); ++it2_2 )
			{
				barrelSlot_2 = it2_2->first;
				xfer->xferInt( &barrelSlot_2 );

				barrelOffset_2 = it2_2->second;
				xfer->xferCoord3D( &barrelOffset_2 );
			}
		}
	}
	else if( xfer->getXferMode() == XFER_LOAD )
	{
		if (m_originalDrawableFiringOffsets.empty() == false)
		{
			DEBUG_CRASH(( "GameLogic::xfer - m_originalDrawableFiringOffsets should be empty, but is not"));
			//throw SC_INVALID_DATA;
		}

		if (m_disguisedDrawableFiringOffsets.empty() == false)
		{
			DEBUG_CRASH(( "GameLogic::xfer - m_disguisedDrawableFiringOffsets should be empty, but is not"));
			//throw SC_INVALID_DATA;
		}

		for(UnsignedShort i = 0; i < OriginalFiringOffsetsSize; ++i)
		{
			xfer->xferInt( &weaponSlot );

			xfer->xferInt( &barrelCount );

			FiringPosStruct data;
			data.weaponSlot = weaponSlot;
			data.barrelCount = barrelCount;

			for(UnsignedShort i_2 = 0; i_2 < barrelCount; ++i_2)
			{
				xfer->xferInt( &barrelSlot );

				xfer->xferCoord3D( &barrelOffset );

				barrelPair.first = barrelSlot;
				barrelPair.second = barrelOffset;
				data.posVec.push_back(barrelPair);
			}

			m_originalDrawableFiringOffsets.push_back(data);
			data.posVec.clear();
		}

		for(UnsignedShort i2 = 0; i2 < DisguisedFiringOffsetsSize; ++i2)
		{
			xfer->xferInt( &weaponSlot_2 );

			xfer->xferInt( &barrelCount_2 );

			FiringPosStruct data;
			data.weaponSlot = weaponSlot_2;
			data.barrelCount = barrelCount_2;

			for(UnsignedShort i2_2 = 0; i2_2 < barrelCount_2; ++i2_2)
			{
				xfer->xferInt( &barrelSlot_2 );

				xfer->xferCoord3D( &barrelOffset_2 );

				barrelPair.first = barrelSlot_2;
				barrelPair.second = barrelOffset_2;
				data.posVec.push_back(barrelPair);
			}

			m_disguisedDrawableFiringOffsets.push_back(data);
			data.posVec.clear();
		}
	}*/
	

}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void StealthUpdate::loadPostProcess( void )
{

	// extend base class
	UpdateModule::loadPostProcess();

	//
	// we will need to restore our disguise when the game is ready to run ... NOTE that we
	// cannot restore it here because if we called changeVisualDisguise() it would
	// destroy our drawable and create new stuff.  The destruction of a drawable during
	// a load is *very* bad ... it has a snapshot instance in the game state, other things
	// may be pointing at it etc.
	//
	if( isDisguised() )
		m_xferRestoreDisguise = TRUE;

}

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

// DeployStyleAIUpdate.cpp ////////////
// Author: Kris Morness, August 2002
// Desc:   A standard ai update that also handles units that must deploy to attack and pack before moving.

#include "PreRTS.h"	// This must go first in EVERY cpp file int the GameEngine

#include "Common/Player.h"
#include "Common/ThingFactory.h"
#include "Common/ThingTemplate.h"

#include "GameClient/Drawable.h"
#include "GameClient/InGameUI.h"

#include "GameLogic/ExperienceTracker.h"
#include "GameLogic/Locomotor.h"
#include "GameLogic/Object.h"
#include "GameLogic/PartitionManager.h"
#include "GameLogic/Weapon.h"

#include "GameLogic/Module/BodyModule.h"
#include "GameLogic/Module/ContainModule.h"
#include "GameLogic/Module/DeployStyleAIUpdate.h"
#include "GameLogic/Module/PhysicsUpdate.h"


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
DeployStyleAIUpdate::DeployStyleAIUpdate( Thing *thing, const ModuleData* moduleData ) : AIUpdateInterface( thing, moduleData )
{
	m_state = READY_TO_MOVE;
	m_frameToWaitForDeploy = 0;
	m_doDeploy = FALSE;
	m_doUndeploy = FALSE;
	m_deployTurretFunctionChangeUpgrade.clear();
	m_deployObjectFunctionChangeUpgrade.clear();
	m_turretDeployedFunctionChangeUpgrade.clear();

	const DeployStyleAIUpdateModuleData *data = getDeployStyleAIUpdateModuleData();
	if(!data->m_deployFunctionChangeUpgrade.empty())
	{
		Int parsing = 0;

		for(std::vector<AsciiString>::const_iterator it = data->m_deployFunctionChangeUpgrade.begin(); it != data->m_deployFunctionChangeUpgrade.end(); ++it)
		{
			if(parsing == 1 && stricmp((*it).str(), "TURRET") != 0 && stricmp((*it).str(), "OBJECT") != 0 && stricmp((*it).str(), "DEPLOYED") != 0 )
			{
				m_deployTurretFunctionChangeUpgrade.push_back(*it);
			}
			else if(parsing == 2 && stricmp((*it).str(), "TURRET") != 0 && stricmp((*it).str(), "OBJECT") != 0 && stricmp((*it).str(), "DEPLOYED") != 0 )
			{
				m_deployObjectFunctionChangeUpgrade.push_back(*it);
			}
			else if(parsing == 3 && stricmp((*it).str(), "TURRET") != 0 && stricmp((*it).str(), "OBJECT") != 0 && stricmp((*it).str(), "DEPLOYED") != 0 )
			{
				m_turretDeployedFunctionChangeUpgrade.push_back(*it);
			}
			else if(stricmp((*it).str(), "TURRET") == 0)
			{
				parsing = 1;
			}
			else if(stricmp((*it).str(), "OBJECT") == 0)
			{
				parsing = 2;
			}
			else if(stricmp((*it).str(), "DEPLOYED") == 0)
			{
				parsing = 3;
			}
		}
	}
}

//-------------------------------------------------------------------------------------------------
DeployStyleAIUpdate::~DeployStyleAIUpdate( void )
{
}

//-------------------------------------------------------------------------------------------------
Bool DeployStyleAIUpdate::isIdle() const
{
	return AIUpdateInterface::isIdle();
}

//-------------------------------------------------------------------------------------------------
void DeployStyleAIUpdate::aiDoCommand( const AICommandParms* parms )
{
	if (!isAllowedToRespondToAiCommands(parms))
		return;

	/*
	//Hack code to allow follow waypoint scripts to be converted to attack follow waypoint scripts
	//for simple-test-map-purposes.
	switch( parms->m_cmd )
	{
		case AICMD_FOLLOW_WAYPOINT_PATH:
			((AICommandParms*)parms)->m_cmd = AICMD_ATTACKFOLLOW_WAYPOINT_PATH;
			break;
		case AICMD_FOLLOW_WAYPOINT_PATH_AS_TEAM:
			((AICommandParms*)parms)->m_cmd = AICMD_ATTACKFOLLOW_WAYPOINT_PATH_AS_TEAM;
			break;
	}
	*/
	AIUpdateInterface::aiDoCommand( parms );
}

//-------------------------------------------------------------------------------------------------
UpdateSleepTime DeployStyleAIUpdate::update( void )
{
	// have to call our parent's isIdle, because we override it to never return true
	// when we have a pending command...
	Object *self = getObject();
	Weapon *weapon = self->getCurrentWeapon();
	UnsignedInt now = TheGameLogic->getFrame();
	const DeployStyleAIUpdateModuleData *data = getDeployStyleAIUpdateModuleData();

	//Are we attempting to move? If so we can't do it unless we are undeployed.
	Bool isTryingToMove = isWaitingForPath() || getPath();
	if(!m_doUndeploy)
	{
		m_doUndeploy = data->m_statusToUndeploy.any() && self->getStatusBits().testForAny( data->m_statusToUndeploy );
		if(!m_doUndeploy)
		{
			for(std::vector<AsciiString>::const_iterator it = data->m_customStatusToUndeploy.begin(); it != data->m_customStatusToUndeploy.end(); ++it)
			{
				if (self->testCustomStatus(*it))
				{
					m_doUndeploy = TRUE;
					break;
				}
			}
		}
	}

	if(m_doUndeploy && !data->m_removeStatusAfterTrigger )
		m_doDeploy = FALSE;

	//Are we trying to attack something. If so, we need to be in range before we can do so.
	Bool isTryingToAttack = getStateMachine()->isInAttackState();
	Bool isInRange = FALSE;

	//Are we in guard mode? If so, are we idle... idle guarders deploy for fastest response against attackers.
	Bool isInGuardIdleState = getStateMachine()->isInGuardIdleState();
	if(!m_doDeploy && !m_doUndeploy )
	{
		m_doDeploy = ( data->m_statusToDeploy.any() && self->getStatusBits().testForAny( data->m_statusToDeploy ) );
		if(!m_doDeploy)
		{
			for(std::vector<AsciiString>::const_iterator it = data->m_customStatusToDeploy.begin(); it != data->m_customStatusToDeploy.end(); ++it)
			{
				if (self->testCustomStatus(*it))
				{
					m_doDeploy = TRUE;
					break;
				}
			}
		}
	}

	if(m_doDeploy && !data->m_removeStatusAfterTrigger)
		m_doUndeploy = FALSE;		// There can only be One! ...Unless you are checking whether which status trigger rules first

	AIUpdateInterface *ai = self->getAI();

	// Check whether we are Deploying while in the middle of Moving
	if(m_doDeploy && isTryingToMove && ( m_state == READY_TO_MOVE || m_state == UNDEPLOY || m_state == ALIGNING_TURRETS ) )
	{
		ai->aiIdle(CMD_FROM_AI);
		isTryingToMove = FALSE;
		m_doUndeploy = FALSE;
	}

	// Check whether we are Undeploying while Deploying or Fully Deployed
	if(m_doUndeploy && ( m_state == READY_TO_ATTACK || m_state == DEPLOY ) )
	{
		ai->aiIdle(CMD_FROM_AI);
		isTryingToAttack = FALSE;
		isInGuardIdleState = FALSE;
		m_doDeploy = FALSE;
	}

	// New function that determines whether an Object requires to be Deployed to Fire its Weapon
	Bool needsDeployToFire = TRUE;

	if(data->m_deployInitiallyDisabled)
	{
		needsDeployToFire = FALSE;
	}

	if(!m_deployObjectFunctionChangeUpgrade.empty())
	{
		needsDeployToFire = checkForDeployUpgrades("OBJECT");
	}

	if(!needsDeployToFire)
	{
		isTryingToMove = FALSE;
		isInGuardIdleState = FALSE;
		isTryingToAttack = FALSE;
	}

	if( isTryingToAttack && weapon )
	{
		Object *victim = ai->getCurrentVictim();
		if( victim )
		{
			isInRange = weapon->isWithinAttackRange( self, victim );
		}
		else
		{
			const Coord3D *pos = ai->getCurrentVictimPos();
			if( pos )
			{
				isInRange = weapon->isWithinAttackRange( self, pos );
			}
		}
	}

	// New features
	Bool moveAfterDeploy = checkAfterDeploy("HAS_MOVEMENT");
	Bool deployNoRelocate = checkAfterDeploy("NO_RELOCATE");

	// Able to move after deploying
	if(moveAfterDeploy && ( m_state == DEPLOY || m_state == READY_TO_ATTACK || m_doDeploy ))
	{
		isTryingToMove = FALSE;
	}

	// New feature, disables relocating after Deploying
	if(deployNoRelocate && !moveAfterDeploy && !m_doUndeploy && ( m_state != READY_TO_MOVE || m_doDeploy ) )
	{
		// No moving, but you can still designate next destinations
		self->setStatus( MAKE_OBJECT_STATUS_MASK(OBJECT_STATUS_IMMOBILE) );

		isTryingToMove = FALSE;
	}
	// You can only clear this status by Undeploying
	else if((deployNoRelocate || moveAfterDeploy) && self->testStatus(OBJECT_STATUS_IMMOBILE) && m_doUndeploy )
	{
		self->clearStatus( MAKE_OBJECT_STATUS_MASK(OBJECT_STATUS_IMMOBILE) );
	}

	if( m_frameToWaitForDeploy != 0 && now >= m_frameToWaitForDeploy)
	{
		switch( m_state )
		{
			case DEPLOY:
				setMyState( READY_TO_ATTACK );
				break;
			case UNDEPLOY:
				setMyState( READY_TO_MOVE );
				break;
		}
	}

	// Fixed moving while deployed. 
	if( isTryingToMove || m_doUndeploy )
	{
		switch( m_state )
		{
			case READY_TO_MOVE:
				//We're ready... ai will handle moving.
				break;
			case READY_TO_ATTACK:
			{
				WhichTurretType tur = getWhichTurretForCurWeapon();
				if( tur != TURRET_INVALID )
				{
					if( doTurretsHaveToCenterBeforePacking() )
					{
						//Turrets need to center before we can undeploy.
						setMyState( ALIGNING_TURRETS );
						break;
					}
				}
				//Start undeploying.
				setMyState( UNDEPLOY );
				break;
			}
			case DEPLOY:
				if( m_frameToWaitForDeploy != 0 )
				{
					//Reverse the deploy at it's current frame!
					setMyState( UNDEPLOY, TRUE );
				}
				break;
			case UNDEPLOY:
				break;
			case ALIGNING_TURRETS:
			{
				WhichTurretType tur = getWhichTurretForCurWeapon();
				if( tur != TURRET_INVALID )
				{
					if( isTurretInNaturalPosition( tur ) )
					{
						//Turrets are centers, so now we can undeploy.
						setMyState( UNDEPLOY );
					}
				}
				break;
			}
		}

		// After sequence checks
		if( m_state == READY_TO_MOVE )
		{
			m_doUndeploy = FALSE;
		}

	}
	else if( isInRange || isInGuardIdleState || m_doDeploy )
	{
		switch( m_state )
		{
			case READY_TO_MOVE:
				//We're need to deploy before we can attack.
				setMyState( DEPLOY );
				break;
			case READY_TO_ATTACK:
				//Let the AI handle attacking.
				break;
			case DEPLOY:
				//We can't start attacking until we are fully deployed.
				break;
			case UNDEPLOY:
				if( m_frameToWaitForDeploy != 0 )
				{
					//Reverse the undeploy at it's current frame!
					setMyState( DEPLOY, TRUE );
				}
				break;
			case ALIGNING_TURRETS:
				//If turrets are aligning, we are able to attack right now.
				setMyState( READY_TO_ATTACK );
				break;
		}

		// After sequence checks
		if( m_doDeploy )
		{
			if( m_state == READY_TO_ATTACK )
			{
				m_doDeploy = FALSE;
			}
		}
		
		m_doUndeploy = FALSE;
	}

	switch( m_state )
	{
		case READY_TO_MOVE:
			if( isTryingToMove )
			{
				self->setModelConditionState( MODELCONDITION_MOVING );
			}
			break;

		case READY_TO_ATTACK:
			break;

		case DEPLOY:
			if( data->m_manualDeployAnimations )
			{
				UnsignedInt totalFrames = getPackTime();
				UnsignedInt framesLeft = m_frameToWaitForDeploy - now;
				Drawable *draw = self->getDrawable();
				if( draw )
				{
					draw->setAnimationFrame( totalFrames - framesLeft );
				}
			}
			getStateMachine()->setTemporaryState( AI_BUSY, UPDATE_SLEEP_NONE );
			setLocomotorGoalNone();
			break;
		case UNDEPLOY:
			if( data->m_manualDeployAnimations )
			{
				UnsignedInt framesLeft = m_frameToWaitForDeploy - now;
				Drawable *draw = self->getDrawable();
				if( draw )
				{
					draw->setAnimationFrame( framesLeft );
				}
			}
			getStateMachine()->setTemporaryState( AI_BUSY, UPDATE_SLEEP_NONE );
			setLocomotorGoalNone();
			break;

		case ALIGNING_TURRETS:
			getStateMachine()->setTemporaryState( AI_BUSY, UPDATE_SLEEP_NONE );
			setLocomotorGoalNone();
			break;
	}

	if(data->m_removeStatusAfterTrigger)
	{
		if(data->m_statusToDeploy.any()  && self->getStatusBits().testForAny( data->m_statusToDeploy ))
		{
			m_doUndeploy = FALSE;
			m_doDeploy = TRUE;
			self->clearStatus( data->m_statusToDeploy );
		}
		if(data->m_statusToUndeploy.any()  && self->getStatusBits().testForAny( data->m_statusToUndeploy ))
		{
			m_doDeploy = FALSE;
			m_doUndeploy = TRUE;
			self->clearStatus( data->m_statusToUndeploy );
		}
		if(!m_doDeploy)
		{
			for(std::vector<AsciiString>::const_iterator it = data->m_customStatusToDeploy.begin(); it != data->m_customStatusToDeploy.end(); ++it)
			{
				if (self->testCustomStatus(*it))
				{
					m_doUndeploy = FALSE;
					m_doDeploy = TRUE;
					self->setCustomStatus((*it), FALSE);
				}
			}
		}
		if(!m_doUndeploy)
		{
			for(std::vector<AsciiString>::const_iterator it = data->m_customStatusToUndeploy.begin(); it != data->m_customStatusToUndeploy.end(); ++it)
			{
				if (self->testCustomStatus(*it))
				{
					m_doDeploy = FALSE;
					m_doUndeploy = TRUE;
					self->setCustomStatus((*it), FALSE);
				}
			}
		}
	}

	AIUpdateInterface::update();
	//We can't sleep the deploy AI because any new commands need to be caught and sent
	//into busy state during the update.
	return UPDATE_SLEEP_NONE;

}

//-------------------------------------------------------------------------------------------------
void DeployStyleAIUpdate::setMyState( DeployStateTypes stateID, Bool reverseDeploy )
{
	m_state = stateID;
	Object *self = getObject();
	UnsignedInt now = TheGameLogic->getFrame();
	const DeployStyleAIUpdateModuleData *data = getDeployStyleAIUpdateModuleData();

	switch( stateID )
	{
		case DEPLOY:
		{
			//Tell our object to deploy (so it can continue the same attack later).
			self->clearAndSetModelConditionFlags( MAKE_MODELCONDITION_MASK( MODELCONDITION_PACKING ),
																						 MAKE_MODELCONDITION_MASK( MODELCONDITION_UNPACKING ) );

			if( reverseDeploy )
			{
				//This is a zero to max animation.
				UnsignedInt totalFrames = getUnpackTime();
				UnsignedInt framesLeft = m_frameToWaitForDeploy - now;
				m_frameToWaitForDeploy = now + totalFrames - framesLeft;
				if( data->m_manualDeployAnimations )
				{
					Drawable *draw = self->getDrawable();
					if( draw )
					{
						draw->setAnimationFrame( totalFrames - framesLeft );
					}
				}
			}
			else
			{
				m_frameToWaitForDeploy = getUnpackTime() + now;
			}

			//Play deploy sound
			const ThingTemplate *thing = self->getTemplate();
			const AudioEventRTS* soundToPlayPtr = thing->getPerUnitSound( "Deploy" );
			if( soundToPlayPtr )
			{
				AudioEventRTS soundToPlay = *soundToPlayPtr;
				soundToPlay.setObjectID( self->getID() );
				TheAudio->addAudioEvent( &soundToPlay );
			}

			break;
		}
		case UNDEPLOY:
		{
			//This status will tell the approach AI state to succeed automatically. This prevents
			//twitching on deploy. Make sure we clear it now so when it goes into that state, it'll
			//actually move.
			self->clearStatus( MAKE_OBJECT_STATUS_MASK( OBJECT_STATUS_DEPLOYED ) );

			//Tell our object to pack up (so it can continue the same move later).
			self->clearAndSetModelConditionFlags( MAKE_MODELCONDITION_MASK2( MODELCONDITION_UNPACKING, MODELCONDITION_DEPLOYED ),
																						 MAKE_MODELCONDITION_MASK( MODELCONDITION_PACKING ) );

			if( reverseDeploy )
			{
				//This is a max to zero animation.
				UnsignedInt totalFrames = getUnpackTime();
				UnsignedInt framesLeft = m_frameToWaitForDeploy - now;
				m_frameToWaitForDeploy = now + totalFrames - framesLeft;
				if( data->m_manualDeployAnimations )
				{
					Drawable *draw = self->getDrawable();
					if( draw )
					{
						draw->setAnimationFrame( framesLeft );
					}
				}
			}
			else
			{
				m_frameToWaitForDeploy = getPackTime() + now;
			}

			if( doTurretsFunctionOnlyWhenDeployed() )
			{
				WhichTurretType tur = getWhichTurretForCurWeapon();
				if( tur != TURRET_INVALID )
				{
					setTurretEnabled( tur, false );
				}
			}

			//Play undeploy sound
			const ThingTemplate *thing = self->getTemplate();
			const AudioEventRTS* soundToPlayPtr = thing->getPerUnitSound( "Undeploy" );
			if( soundToPlayPtr )
			{
				AudioEventRTS soundToPlay = *soundToPlayPtr;
				soundToPlay.setObjectID( self->getID() );
				TheAudio->addAudioEvent( &soundToPlay );
			}

			break;
		}
		case READY_TO_MOVE:
		{
			m_frameToWaitForDeploy = 0;

			self->clearModelConditionFlags( MAKE_MODELCONDITION_MASK( MODELCONDITION_PACKING ) );
			break;
		}
		case READY_TO_ATTACK:
		{
			//This status will tell the approach AI state to succeed automatically. This prevents
			//twitching on deploy.
			self->setStatus( MAKE_OBJECT_STATUS_MASK( OBJECT_STATUS_DEPLOYED ) );

			m_frameToWaitForDeploy = 0;

			self->clearAndSetModelConditionFlags( MAKE_MODELCONDITION_MASK( MODELCONDITION_UNPACKING ),
																						 MAKE_MODELCONDITION_MASK( MODELCONDITION_DEPLOYED) );

			if( doTurretsFunctionOnlyWhenDeployed() )
			{
				WhichTurretType tur = getWhichTurretForCurWeapon();
				if( tur != TURRET_INVALID )
				{
					setTurretEnabled( tur, true );
				}
			}

			break;
		}
		case ALIGNING_TURRETS:
		{
			m_frameToWaitForDeploy = 0;
			WhichTurretType tur = getWhichTurretForCurWeapon();
			if( tur != TURRET_INVALID )
			{
				recenterTurret( tur );
			}
			break;
		}
	}

	if(!m_deployTurretFunctionChangeUpgrade.empty())
	{
		if(stateID == READY_TO_MOVE || stateID == UNDEPLOY)
		{
			Bool needsDeployToFire = checkForDeployUpgrades("TURRET");
			WhichTurretType tur = getWhichTurretForCurWeapon();
			if( tur != TURRET_INVALID )
			{
				setTurretEnabled( tur, !needsDeployToFire );
			}
		}
	}

	if(!m_turretDeployedFunctionChangeUpgrade.empty() && stateID == READY_TO_ATTACK)
	{
		Bool needsDeployToFire = checkForDeployUpgrades("DEPLOYED");
		WhichTurretType tur = getWhichTurretForCurWeapon();
		if( tur != TURRET_INVALID )
		{
			setTurretEnabled( tur, !needsDeployToFire );
		}
	}

}

Bool DeployStyleAIUpdate::checkForDeployUpgrades(const AsciiString& type) const
{
	const Object *self = getObject();
	std::vector<AsciiString> checkVector;
	Bool checkUpgrade = TRUE;
	Bool dontCheckValue = FALSE;
	Bool needsDeployToFire = TRUE;

	if(type == "OBJECT")
	{
		checkVector = m_deployObjectFunctionChangeUpgrade;
	}
	else if(type == "TURRET")
	{
		checkVector = m_deployTurretFunctionChangeUpgrade;
	}
	else if(type == "DEPLOYED")
	{
		checkVector = m_turretDeployedFunctionChangeUpgrade;
		if(doTurretsFunctionOnlyWhenDeployed())
			needsDeployToFire = FALSE;
	}

	for(std::vector<AsciiString>::const_iterator it = checkVector.begin(); it != checkVector.end(); ++it)
	{
		if(checkUpgrade)
		{
			const UpgradeTemplate* ut = TheUpgradeCenter->findUpgrade( *it );
			if( !ut )
			{
				DEBUG_CRASH(("An upgrade module references '%s', which is not an Upgrade", it->str()));
				throw INI_INVALID_DATA;
			}
			if ( ut->getUpgradeType() == UPGRADE_TYPE_PLAYER )
			{
				if(!self->getControllingPlayer()->hasUpgradeComplete(ut))
				{
					dontCheckValue = TRUE;
				}
			}
			else if( !self->hasUpgrade(ut) )
			{
				dontCheckValue = TRUE;
			}
		}
		else if(dontCheckValue)
		{
			dontCheckValue = FALSE;
		}
		else
		{
			if(stricmp( it->str(), "yes" ) == 0)
			{
				needsDeployToFire = TRUE;
			}
			else if(stricmp( it->str(), "no" ) == 0)
			{
				needsDeployToFire = FALSE;
			}
		}

		if(checkUpgrade)
			checkUpgrade = FALSE;
		else
			checkUpgrade = TRUE;
	}

	return needsDeployToFire;
}

Bool DeployStyleAIUpdate::checkAfterDeploy(const AsciiString& type) const
{
	const Object *self = getObject();
	const DeployStyleAIUpdateModuleData *data = getDeployStyleAIUpdateModuleData();
	std::vector<AsciiString> checkVector;
	Bool checkUpgrade = TRUE;
	Bool checkValue = FALSE;

	if( type == "NO_RELOCATE")
		checkVector = data->m_deployNoRelocateUpgrades;
	else if( type == "HAS_MOVEMENT")
		checkVector = data->m_moveAfterDeployUpgrades;

	for(std::vector<AsciiString>::const_iterator it = checkVector.begin(); it != checkVector.end(); ++it)
	{
		if(checkUpgrade)
		{
			const UpgradeTemplate* ut = TheUpgradeCenter->findUpgrade( *it );
			if( !ut )
			{
				DEBUG_CRASH(("An upgrade module references '%s', which is not an Upgrade", it->str()));
				throw INI_INVALID_DATA;
			}
			if ( ut->getUpgradeType() == UPGRADE_TYPE_PLAYER )
			{
				if(self->getControllingPlayer()->hasUpgradeComplete(ut))
				{
					checkValue = TRUE;
				}
			}
			else if( self->hasUpgrade(ut) )
			{
				checkValue = TRUE;
			}
		}
		else if(checkValue)
		{
			return INI::scanBool((*it).str());
		}

		if(checkUpgrade)
			checkUpgrade = FALSE;
		else
			checkUpgrade = TRUE;
	}
	if( type == "NO_RELOCATE")
		return data->m_deployNoRelocate;
	else if( type == "HAS_MOVEMENT")
		return data->m_moveAfterDeploy;
	else
		return FALSE;
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void DeployStyleAIUpdate::crc( Xfer *xfer )
{
	// extend base class
	AIUpdateInterface::crc(xfer);
}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version
	* 2: Added support for attack move
	* 3: Added improved support for guard, and support for hunt AI
 **/
// ------------------------------------------------------------------------------------------------
void DeployStyleAIUpdate::xfer( Xfer *xfer )
{
  // version
  XferVersion currentVersion = 4;
  XferVersion version = currentVersion;
  xfer->xferVersion( &version, currentVersion );

 // extend base class
	AIUpdateInterface::xfer(xfer);

	if( version >= 4 )
	{
		xfer->xferUser(&m_state, sizeof(m_state));
		xfer->xferUnsignedInt(&m_frameToWaitForDeploy);
	}
	else if( xfer->getXferMode() == XFER_LOAD )
	{
		//Good riddance!!!
		Bool obsoleteBool;
		UnsignedInt obsoleteUnsignedInt;
		ObjectID obsoleteObjectID;
		Coord3D obsoleteCoord3D;
   	AICommandParmsStorage	obsoleteAICommandParmsStorage;

		xfer->xferBool( &obsoleteBool );
		xfer->xferUnsignedInt( &obsoleteUnsignedInt );
		xfer->xferUser(&m_state, sizeof(m_state));

		if( version >= 2 )
		{
			xfer->xferObjectID( &obsoleteObjectID );
			xfer->xferObjectID( &obsoleteObjectID );
			xfer->xferCoord3D( &obsoleteCoord3D );
			xfer->xferBool( &obsoleteBool );
			xfer->xferBool( &obsoleteBool );
			xfer->xferBool( &obsoleteBool );
		}
		if( version >= 3 )
		{
			xfer->xferBool( &obsoleteBool );
			xfer->xferBool( &obsoleteBool );
		}

		obsoleteAICommandParmsStorage.doXfer( xfer );

		if( version < 2 )
		{
			obsoleteAICommandParmsStorage.doXfer(xfer);
		}

		//Initialize unit to able to move.
		m_state = READY_TO_MOVE;
	}

}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void DeployStyleAIUpdate::loadPostProcess( void )
{
 // extend base class
	AIUpdateInterface::loadPostProcess();
}  // end loadPostProcess


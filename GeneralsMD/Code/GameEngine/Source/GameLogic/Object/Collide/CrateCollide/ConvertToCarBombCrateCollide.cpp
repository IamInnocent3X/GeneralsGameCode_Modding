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

// FILE: ConvertToCarBombCrateCollide.cpp ///////////////////////////////////////////////////////////////////////
// Author: Graham Smallwood, March 2002
// Desc:   A crate that gives a level of experience to all within n distance
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file int the GameEngine
#include "Common/Player.h"
#include "Common/Radar.h"
#include "Common/Team.h"
#include "Common/ThingTemplate.h"
#include "Common/Xfer.h"
#include "GameClient/Drawable.h"
#include "GameClient/FXList.h"
#include "GameLogic/ExperienceTracker.h"
#include "GameLogic/Object.h"
#include "GameLogic/Module/ConvertToCarBombCrateCollide.h"
#include "GameLogic/Module/AIUpdate.h"
#include "GameLogic/Module/HijackerUpdate.h"
#include "GameLogic/PartitionManager.h"
#include "GameLogic/ScriptEngine.h"
#include "GameLogic/ExperienceTracker.h"

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
ConvertToCarBombCrateCollide::ConvertToCarBombCrateCollide( Thing *thing, const ModuleData* moduleData ) : CrateCollide( thing, moduleData )
{
	m_originalName = NULL;
	m_originalTeam = NULL;
	m_originalVisionRange = 0.0f;
	m_originalShroudClearingRange = 0.0f;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
ConvertToCarBombCrateCollide::~ConvertToCarBombCrateCollide( void )
{

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
Bool ConvertToCarBombCrateCollide::isValidToExecute( const Object *other ) const
{
	if( !CrateCollide::isValidToExecute(other) )
	{
		return FALSE;
	}

	if( other->isEffectivelyDead() )
	{
		return FALSE;
	}

	if( other->isKindOf( KINDOF_AIRCRAFT ) || other->isKindOf( KINDOF_BOAT ) )
	{
		//Can't make carbombs out of planes and boats!
		return FALSE;
	}

	if( other->getStatusBits().test( OBJECT_STATUS_IS_CARBOMB ) )
	{
		return FALSE;// oops, sorry, I'll convert the next one.
	}

	// Check to see if this other object has a carbomb weapon set that isn't in use.
	WeaponSetFlags flags;
	flags.set( WEAPONSET_CARBOMB );
	const WeaponTemplateSet* set = other->getTemplate()->findWeaponTemplateSet( flags );
	if( !set )
	{
		//This unit has no weapon set!
		return FALSE;
	}
	if( !set->testWeaponSetFlag( WEAPONSET_CARBOMB ) )
	{
		//This unit has a weaponset, but the best match code above chose a different
		//weaponset.
		return FALSE;
	}

	// Also make sure that the car isn't already a carbomb!
	if( other->testWeaponSetFlag( WEAPONSET_CARBOMB ) )
	{
		return FALSE;
	}

	return TRUE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
Bool ConvertToCarBombCrateCollide::executeCrateBehavior( Object *other )
{
	//Check to make sure that the other object is also the goal object in the AIUpdateInterface
	//in order to prevent an unintentional conversion simply by having the terrorist walk too close
	//to it.
	//Assume ai is valid because CrateCollide::isValidToExecute(other) checks it.
	Object *obj = getObject();
	AIUpdateInterface* ai = obj->getAIUpdateInterface();
	if (ai && ai->getGoalObject() != other)
		return false;

	if( other && other->checkAndDetonateBoobyTrap(obj) )
	{
		// Whoops, it was mined.  Cancel if it or us is now dead.
		if( other->isEffectivelyDead() || getObject()->isEffectivelyDead() )
		{
			return false;
		}
	}

	const ConvertToCarBombCrateCollideModuleData *data = getConvertToCarBombCrateCollideModuleData();

	other->setWeaponSetFlag( WEAPONSET_CARBOMB );

	FXList::doFXObj( data->m_fxList, other );

	m_originalTeam = other->getTeam();
	other->defect( getObject()->getControllingPlayer()->getDefaultTeam(), 0);

	//In order to make things easier for the designers, we are going to transfer the terrorist name
	//to the car... so the designer can control the car with their scripts.
	m_originalName = other->getName();
	TheScriptEngine->transferObjectName( getObject()->getName(), other );

	//This is kinda special... we will endow our new ride with our vision and shroud range, since we are driving
	m_originalVisionRange = other->getVisionRange();
	m_originalShroudClearingRange = other->getShroudClearingRange();
	other->setVisionRange(getObject()->getVisionRange());
	other->setShroudClearingRange(getObject()->getShroudClearingRange());
	other->setStatus( MAKE_OBJECT_STATUS_MASK( OBJECT_STATUS_IS_CARBOMB ) );

	ExperienceTracker *exp = other->getExperienceTracker();
	if (exp)
	{
		exp->setVeterancyLevel(obj->getExperienceTracker()->getVeterancyLevel());
	}



	TheRadar->removeObject( other );
	TheRadar->addObject( other );

	CrateCollide::executeCrateBehavior(other);

	// New feature - Enable it to be compatible with hijackerupdate.
	// Enables new features such as remove or destroy, to revert the carbomb properties
	// get the name of the hijackerupdate
	//static NameKeyType key_HijackerUpdate = NAMEKEY( "HijackerUpdate" );
	//HijackerUpdate *hijackerUpdate = (HijackerUpdate*)obj->findUpdateModule( key_HijackerUpdate );
	//if( hijackerUpdate )
	HijackerUpdateInterface *hijackerUpdate = obj->getHijackerUpdateInterface();
	if( hijackerUpdate )
	{
		obj->setHijackingID( other->getID() );
		hijackerUpdate->setTargetObject( other );
		hijackerUpdate->setIsInVehicle( TRUE );
		hijackerUpdate->setUpdate( TRUE );

		hijackerUpdate->setNoLeechExp( !data->m_leechExpFromObject );
		hijackerUpdate->setDestroyOnHeal( data->m_destroyOnHeal );
		hijackerUpdate->setRemoveOnHeal( data->m_removeOnHeal );
		hijackerUpdate->setDestroyOnTargetDie( data->m_destroyOnTargetDie );
		hijackerUpdate->setPercentDamage( data->m_damagePercentageToUnit );
		hijackerUpdate->setStatusToRemove( data->m_statusToRemove );
		hijackerUpdate->setStatusToDestroy( data->m_statusToDestroy );
		hijackerUpdate->setCustomStatusToRemove( data->m_customStatusToRemove );
		hijackerUpdate->setCustomStatusToDestroy( data->m_customStatusToDestroy );

		// Set the ID for the CarBomb Converter
		other->setCarBombConverterID(getObject()->getID());
		
		// flag bits so hijacker won't be selectible or collideable
		//while within the vehicle
		obj->setStatus( MAKE_OBJECT_STATUS_MASK3( OBJECT_STATUS_NO_COLLISIONS, OBJECT_STATUS_MASKED, OBJECT_STATUS_UNSELECTABLE ) );

		// THIS BLOCK HIDES THE HIJACKER AND REMOVES HIM FROM PARTITION MANAGER
		// remove object from its group (if any)
		obj->leaveGroup();
		if( ai )
		{
			//By setting him to idle, we will prevent him from entering the target after this gets called.
			ai->aiIdle( CMD_FROM_AI );
		}

		// remove rider from partition manager
		ThePartitionManager->unRegisterObject( obj );

		// hide the drawable associated with rider
		if( obj->getDrawable() )
			obj->getDrawable()->setDrawableHidden( true );
		
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

Bool ConvertToCarBombCrateCollide::revertCollideBehavior(Object *other)
{
	CrateCollide::revertCollideBehavior(other);

	getObject()->setHijackingID(INVALID_ID);

	// If the Object doesn't exist, or is destroyed we stop here.
	if(!other || other->isDestroyed() || other->isEffectivelyDead())
		return FALSE;

	// Remove the WeaponSetFlag so that it can be converted to car bomb again
	other->clearWeaponSetFlag( WEAPONSET_CARBOMB );

	// Remove the Status so that it can be converted to car bomb again
	other->clearStatus( MAKE_OBJECT_STATUS_MASK( OBJECT_STATUS_IS_CARBOMB ) );

	// Set its name back to the original name
	TheScriptEngine->transferObjectName( m_originalName, other );

	// Set back its Shroud Ranges
	other->setVisionRange(m_originalVisionRange);
	other->setShroudClearingRange(m_originalShroudClearingRange);

	if ( other->getAI() )
	other->getAI()->aiIdle( CMD_FROM_AI );

	TheGameLogic->deselectObject(other, PLAYERMASK_ALL, TRUE);

	// Clear any terrain decals here
	Drawable* draw = other->getDrawable();
	if (draw)
		draw->setTerrainDecal(TERRAIN_DECAL_NONE);

	// Set it back to the original Team
	other->defect( m_originalTeam, 0 );

	// Indicate that we have reverted the Collied Behavior and no need to continue 
	return TRUE;
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void ConvertToCarBombCrateCollide::crc( Xfer *xfer )
{

	// extend base class
	CrateCollide::crc( xfer );

}  // end crc

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void ConvertToCarBombCrateCollide::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	CrateCollide::xfer( xfer );

	// original name
	xfer->xferAsciiString( &m_originalName );

	// vision range
	xfer->xferReal( &m_originalVisionRange );

	// shroud clearing range
	xfer->xferReal( &m_originalShroudClearingRange );

	// last team ID (to be enterable)
	TeamID teamID = m_originalTeam ? m_originalTeam->getID() : TEAM_ID_INVALID;
	xfer->xferUser( &teamID, sizeof( TeamID ) );
	if( xfer->getXferMode() == XFER_LOAD )
	{

		if( teamID != TEAM_ID_INVALID )
		{

			m_originalTeam = TheTeamFactory->findTeamByID( teamID );
			if( m_originalTeam == NULL )
			{

				DEBUG_CRASH(( "ConvertToCarBombCrateCollide::xfer - Unable to find original team by id" ));

			}  // end if

		}  // end if
		else
			m_originalTeam = NULL;
	}

}  // end xfer

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void ConvertToCarBombCrateCollide::loadPostProcess( void )
{

	// extend base class
	CrateCollide::loadPostProcess();

}  // end loadPostProcess

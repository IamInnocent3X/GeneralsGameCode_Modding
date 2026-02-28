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
// FILE: HijackerUpdate.cpp
// Author: Mark Lorenzen, July 2002
// Desc:   Allows hijacker to keep with his hijacked vehicle (though hidden) until it dies, then
// to become a hijacker once more
//
/////////////////////////////////////////////////////////////////////////////////////////////////

// USER INCLUDES //////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "Common/Player.h"
#include "Common/ThingFactory.h"
#include "Common/ThingTemplate.h"
#include "Common/Xfer.h"
#include "GameClient/Drawable.h"
#include "GameLogic/Object.h"
#include "GameLogic/Module/HijackerUpdate.h"
#include "GameLogic/Module/AIUpdate.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/Weapon.h"
#include "GameLogic/PartitionManager.h"
#include "GameLogic/Module/EjectPilotDie.h"
#include "GameLogic/Module/CollideModule.h"
#include "GameLogic/Module/EquipCrateCollide.h"
#include "GameLogic/Module/ContainModule.h"
#include "GameLogic/ExperienceTracker.h"

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
HijackerUpdate::HijackerUpdate( Thing *thing, const ModuleData *moduleData ) : UpdateModule( thing, moduleData )
{
	m_targetID = INVALID_ID;
	setUpdate( FALSE );
	setIsInVehicle( FALSE );
	setIsParasite( FALSE );
	setNoLeechExp( FALSE );
	setDestroyOnHeal( FALSE );
	setRemoveOnHeal( FALSE );
	setDestroyOnClear( FALSE );
	setDestroyOnTargetDie( FALSE );
	setEject( FALSE );
	setClear( FALSE );
	setHealed( FALSE );
	setNoSelfDamage( FALSE );
	setPercentDamage( 0.0f );
	setParasiteKey( AsciiString::TheEmptyString );
	m_statusToRemove.clear();
	m_statusToDestroy.clear();
	m_customStatusToRemove.clear();
	m_customStatusToDestroy.clear();
	m_recentParasiteKeys.clear();
	m_wasTargetAirborne = false;
	m_ejectPos.zero();
	m_hijackType = HIJACK_NONE;
//	m_ejectPilotDMI = nullptr;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
HijackerUpdate::~HijackerUpdate()
{
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
UpdateSleepTime HijackerUpdate::update()
{
/// @todo srj use SLEEPY_UPDATE here
/// IamInnocent - Done.

	//if ( ! m_update) // have not flagged for updating
	//{
	//	return UPDATE_SLEEP_FOREVER;
	//}

	if (m_isInVehicle)
	{
		Object *obj = getObject();

		// If hijacker has hijacked a vehicle, he needs to move along with it...
		//Continually reset position of hijacker to match the position of the target.
		Object *target = getTargetObject();
		Bool isDestroyed = FALSE;
		Bool isRemoved = FALSE;
		Bool isKilled = FALSE;
		Bool revertedCollide = FALSE;

		if( target )
		{
			if( target->getStatusBits().testForAny( m_statusToDestroy ) )
			{
				isDestroyed = TRUE;
			}
			for(std::vector<AsciiString>::const_iterator it = m_customStatusToDestroy.begin(); it != m_customStatusToDestroy.end(); ++it)
			{
				if(isDestroyed)
					break;

				if(target->testCustomStatus(*it))
				{
					isDestroyed = TRUE;
					break;
				}
			}
			if( !isDestroyed && target->getStatusBits().testForAny(m_statusToRemove) )
			{
				isRemoved = TRUE;
			}
			for(std::vector<AsciiString>::const_iterator it = m_customStatusToRemove.begin(); it != m_customStatusToRemove.end(); ++it)
			{
				if(isRemoved || isDestroyed)
					break;

				if(target->testCustomStatus(*it))
				{
					isRemoved = TRUE;
					break;
				}
			}

			Bool hasCorrectParasiteKey = m_parasiteKey.isEmpty() ? TRUE : FALSE;
			if(!m_recentParasiteKeys.empty())
			{
				if(!hasCorrectParasiteKey)
				{
					for(std::vector<AsciiString>::const_iterator it = m_recentParasiteKeys.begin(); it != m_recentParasiteKeys.end(); ++it)
					{
						if((*it) == m_parasiteKey)
						{
							hasCorrectParasiteKey = TRUE;
							break;
						}
					}
				}
				m_recentParasiteKeys.clear();
			}
			if(!isDestroyed && m_clear && m_isParasite && hasCorrectParasiteKey)
			{
				if(m_destroyOnClear)
					isDestroyed = TRUE;
				else
					isRemoved = TRUE;
			}

			if(!isDestroyed && m_eject)
			{
				if(m_destroyOnTargetDie)
					isDestroyed = TRUE;
				else
					isRemoved = TRUE;
			}

			Real curHealth = 0.0f;
			BodyModuleInterface *body = target->getBodyModule();
			if( body )
				curHealth = body->getHealth();

			if(!isDestroyed && m_targetObjHealth && curHealth != m_targetObjHealth)
			{
				if ( m_healed )
				{
					if( m_destroyOnHeal )
						isDestroyed = TRUE;
					else if( m_removeOnHeal )
						isRemoved = TRUE;
				}

				if(!m_noSelfDamage && !isDestroyed && m_targetObjHealth > curHealth)
				{
					//Calculate the damage to be inflicted on each unit.
					Real damage = (m_targetObjHealth - curHealth) * m_percentDamage;

					if(obj->getBodyModule() && damage > obj->getBodyModule()->getHealth())
					{
						isKilled = TRUE;
					}
					else if(damage > 0)
					{
						DamageInfo damageInfo;
						damageInfo.in.m_damageType = DAMAGE_UNRESISTABLE;
						damageInfo.in.m_deathType = DEATH_NORMAL;
						damageInfo.in.m_sourceID = obj->getID();
						damageInfo.in.m_amount = damage;
						obj->attemptDamage( &damageInfo );
					}
				}

				m_targetObjHealth = curHealth;
			}
			else if(!m_targetObjHealth)
			{
				m_targetObjHealth = curHealth;
			}

			// If the equipped Object or Hijacker is removed, revert the behavior
			if(isRemoved || isDestroyed || isKilled)
			{
				for (BehaviorModule** m = obj->getBehaviorModules(); *m; ++m)
				{
					CollideModuleInterface* collide = (*m)->getCollide();
					if (!collide)
						continue;

					if( collide->revertCollideBehavior(target) )
						break;
				}

				// Update the Position for ejection
				if(m_ejectPos.x < WWMATH_EPSILON && m_ejectPos.y < WWMATH_EPSILON && m_ejectPos.z < WWMATH_EPSILON)
					m_ejectPos = *target->getPosition();
				m_wasTargetAirborne = target->isSignificantlyAboveTerrain();
				obj->setPosition( &m_ejectPos );

				// Make the target null for reverting the behavior
				setTargetObject( nullptr );
				setHijackType( HIJACK_NONE );

				// Inform the module we have reverted
				revertedCollide = TRUE;
			}
		}

		setEject( FALSE );
		setClear( FALSE );
		setHealed( FALSE );
		setNoSelfDamage( FALSE );

		if( target && !target->isEffectivelyDead() && !target->isDestroyed() && !revertedCollide)
		{
			// @todo I think we should test for ! IsEffectivelyDead() as well, here
			if(m_ejectPos.x < WWMATH_EPSILON && m_ejectPos.y < WWMATH_EPSILON && m_ejectPos.z < WWMATH_EPSILON)
				m_ejectPos = *target->getPosition();
			m_wasTargetAirborne = target->isSignificantlyAboveTerrain();
			obj->setPosition( &m_ejectPos );

			// If I do not leech Exp, then I do not receive any EXP that should be given to the attached Object
			if(m_noLeechExp)
			{
				setUpdate( FALSE );
				return UPDATE_SLEEP_FOREVER;
			}

			// So, if while I am driving this American war vehicle, I gain skill points, I get to keep them when I wreck the vehicle
			ExperienceTracker *targetExp = target->getExperienceTracker();
			ExperienceTracker *jackerExp = obj->getExperienceTracker();
			if ( targetExp && jackerExp )
			{
				VeterancyLevel highestLevel = MAX(targetExp->getVeterancyLevel(),jackerExp->getVeterancyLevel());
				jackerExp->setVeterancyLevel( highestLevel );
				targetExp->setVeterancyLevel( highestLevel );
			}

		}
		else // the car we have been "driving" is dead now, and has safely ejected us
		{
			{


				//THIS BLOCK RESTORES HIJACKER TO PARTITION MANAGER AND UNHIDES HIM
				ThePartitionManager->registerObject( obj );

				if( obj->getDrawable() )
				{
					// so it is time to unhide ourselves and be a pedestrian hijacker again
					obj->getDrawable()->setDrawableHidden( false );
				}

				// We won't come back here until and unless we have hijacked another vehicle
				obj->clearStatus( MAKE_OBJECT_STATUS_MASK3( OBJECT_STATUS_NO_COLLISIONS, OBJECT_STATUS_MASKED, OBJECT_STATUS_UNSELECTABLE ) );

				AIUpdateInterface* ai = obj->getAIUpdateInterface();
				if ( ai )
				{
					ai->aiIdle( CMD_FROM_AI );
				}

				if (m_wasTargetAirborne && !isDestroyed && !isKilled)
				{
					const ThingTemplate* putInContainerTmpl = TheThingFactory->findTemplate(getHijackerUpdateModuleData()->m_parachuteName);
					DEBUG_ASSERTCRASH(putInContainerTmpl,("DeliverPayload: PutInContainer %s not found!",getHijackerUpdateModuleData()->m_parachuteName.str()));
					if (putInContainerTmpl)
					{
						Object* container = TheThingFactory->newObject( putInContainerTmpl, obj->getTeam() );
						container->setPosition(&m_ejectPos);
						if (container->getContain()->isValidContainerFor(obj, true))
						{
							container->getContain()->addToContain(obj);
						}
						else
						{
							DEBUG_CRASH(("DeliverPayload: PutInContainer %s is full, or not valid for the payload!",getHijackerUpdateModuleData()->m_parachuteName.str()));
						}
					}

				}


			}

			if(!revertedCollide)
			{
				for (BehaviorModule** m = obj->getBehaviorModules(); *m; ++m)
				{
					CollideModuleInterface* collide = (*m)->getCollide();
					if (!collide)
						continue;

					if( collide->revertCollideBehavior(target) )
						break;
				}
			}

			setTargetObject( nullptr );
			setHijackType( HIJACK_NONE );
			setIsInVehicle( FALSE );
			//setUpdate( FALSE );
			setNoLeechExp( FALSE );
			setDestroyOnHeal( FALSE );
			setRemoveOnHeal( FALSE );
			setDestroyOnClear( FALSE );
			setDestroyOnTargetDie( FALSE );
			setPercentDamage( 0.0f );
			setParasiteKey( AsciiString::TheEmptyString );
			m_targetObjHealth = 0.0f;
			m_statusToRemove.clear();
			m_statusToDestroy.clear();
			m_customStatusToRemove.clear();
			m_customStatusToDestroy.clear();
			m_wasTargetAirborne = false;

			if(isDestroyed)
			{
				TheGameLogic->destroyObject( obj );
			}
			else if(isKilled)
			{
				obj->kill();
			}

		}

	}
	else	// not in vehicle
	{
		m_wasTargetAirborne = false;
	}

	setUpdate( FALSE );

	return UPDATE_SLEEP_FOREVER;
}



void HijackerUpdate::setTargetObject( const Object *object )
{
  if( object )
  {
    m_targetID = object->getID();

		// here we also test the target to see whether it ejects pilots
		// when it dies... if so, stores a pointer to that diemoduleinterface
		// nullptr if not...

//		BehaviorModule **dmi = nullptr;
//		for( dmi = object->getBehaviorModules(); *dmi; ++dmi )
//		{
//			m_ejectPilotDMI = (*dmi)->getEjectPilotDieInterface();
//			if( m_ejectPilotDMI )
//				return;
//		}  // end for dmi
  }
	else
	{
		m_targetID = INVALID_ID;
//		m_ejectPilotDMI = nullptr;
	}

}

void HijackerUpdate::setRetargetObject( ObjectID ID )
{
	Object *object = TheGameLogic->findObjectByID( ID );
	if(object)
	{
		Object *self = getObject();
		switch(m_hijackType)
		{
			case HIJACK_CARBOMB:
			case HIJACK_HIJACKER:
			{
				// Set the new Object for Hijacking
				self->setHijackingID( object->getID() );

				// Set the vision range
				object->setVisionRange(self->getVisionRange());
				object->setShroudClearingRange(self->getShroudClearingRange());

				// Set extra properties according to the Hijack Type
				if(m_hijackType == HIJACK_CARBOMB)
				{
					object->setWeaponSetFlag( WEAPONSET_CARBOMB );
					object->setStatus( MAKE_OBJECT_STATUS_MASK( OBJECT_STATUS_IS_CARBOMB ) );
					object->setCarBombConverterID( self->getID() );
				}
				else
				{
					object->setHijackerID( self->getID() );
				}
				break;
			}
			case HIJACK_PARASITE:
			case HIJACK_EQUIP:
			{
				// We overwrite the module's EquipCrateCollide properties first
				// The module checks for the Object's and the Modules equipToID to match for being able to overwrite 
				for (BehaviorModule** m = self->getBehaviorModules(); *m; ++m)
				{
					CollideModuleInterface* collide = (*m)->getCollide();
					if (!collide)
						continue;

					if( collide->isEquipCrateCollide() )
					{
						// Check if it is the correct type of EquipCrateCollide module before proceeding
						if( (m_hijackType == HIJACK_PARASITE && !collide->isParasiteEquipCrateCollide()) ||
							(m_hijackType == HIJACK_EQUIP && collide->isParasiteEquipCrateCollide()) )
							continue;

						EquipCrateCollide *eqc = (EquipCrateCollide*) collide;
						if(eqc)
							eqc->overwriteEquipToIDModule( ID );
					}
				}

				// Change the current equipper properties to the retargeted object
				self->setEquipToID( object->getID() );

				object->setEquipObjectID( self->getID() );
				if(!self->testStatus(OBJECT_STATUS_NO_ATTACK))
					object->setEquipAttackableObjectID( self->getID() );

				break;
			}
		}
	}
}

Object* HijackerUpdate::getTargetObject() const
{
  if( m_targetID != INVALID_ID )
  {
    return TheGameLogic->findObjectByID( m_targetID );
  }
  return nullptr;
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void HijackerUpdate::crc( Xfer *xfer )
{

	// extend base class
	UpdateModule::crc( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void HijackerUpdate::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	UpdateModule::xfer( xfer );

	// target ID
	xfer->xferObjectID( &m_targetID );

	// eject pos
	xfer->xferCoord3D( &m_ejectPos );

	// update
	//xfer->xferBool( &m_update );

	// is in vehicle
	xfer->xferBool( &m_isInVehicle );

	// was target airborne
	xfer->xferBool( &m_wasTargetAirborne );

	// is Parasite
	xfer->xferBool( &m_isParasite );

	// will leech EXP
	xfer->xferBool( &m_noLeechExp );

	// destroy on repair
	xfer->xferBool( &m_destroyOnHeal );

	// remove on repair
	xfer->xferBool( &m_removeOnHeal );

	// destroy on clear, exclusive for parasites
	xfer->xferBool( &m_destroyOnClear );

	// destroy on target die
	xfer->xferBool( &m_destroyOnTargetDie );

	// Damage Percentage
	xfer->xferReal( &m_percentDamage );

	// Health of target
	xfer->xferReal( &m_targetObjHealth );

	// Parasite Key
	xfer->xferAsciiString( &m_parasiteKey );

	// status to remove
	m_statusToRemove.xfer( xfer );

	// status to destroy
	m_statusToDestroy.xfer( xfer );

	// hijack type
	xfer->xferUser( &m_hijackType, sizeof(HijackType) );

	UnsignedShort customStatusToRemoveCount = m_customStatusToRemove.size();
	UnsignedShort customStatusToDestroyCount = m_customStatusToDestroy.size();
	xfer->xferUnsignedShort( &customStatusToRemoveCount );
	xfer->xferUnsignedShort( &customStatusToDestroyCount );
	AsciiString customStatusToRemove;
	AsciiString customStatusToDestroy;
	if( xfer->getXferMode() == XFER_SAVE )
	{

		// go through all IDs
		for (int i = 0; i < customStatusToRemoveCount; i++)
		{
			customStatusToRemove = m_customStatusToRemove[i];
			xfer->xferAsciiString( &customStatusToRemove );
		}

		for (int i_2 = 0; i_2 < customStatusToDestroyCount; i_2++)
		{
			customStatusToDestroy = m_customStatusToDestroy[i_2];
			xfer->xferAsciiString( &customStatusToDestroy );
		}

	}
	else
	{
		// this list should be empty on loading
		if( m_customStatusToRemove.size() != 0 || m_customStatusToDestroy.size() != 0 )
		{

			DEBUG_CRASH(( "ScriptEngine::xfer - m_customStatusToRemove and m_customStatusToDestroy should be empty but is not" ));

		}

		// read all IDs
		for( UnsignedShort i = 0; i < customStatusToRemoveCount; ++i )
		{
			// read and register ID
			xfer->xferAsciiString( &customStatusToRemove );
			m_customStatusToRemove.push_back(customStatusToRemove);

		}

		for( UnsignedShort i_2 = 0; i_2 < customStatusToDestroyCount; ++i_2 )
		{
			// read and register ID
			xfer->xferAsciiString( &customStatusToDestroy );
			m_customStatusToDestroy.push_back(customStatusToDestroy);

		}

	}

}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void HijackerUpdate::loadPostProcess()
{

	// extend base class
	UpdateModule::loadPostProcess();

	// set the target object, this will also tie up the m_ejectPilotDMI pointer
	Object *obj = TheGameLogic->findObjectByID( m_targetID );
	setTargetObject( obj );

}

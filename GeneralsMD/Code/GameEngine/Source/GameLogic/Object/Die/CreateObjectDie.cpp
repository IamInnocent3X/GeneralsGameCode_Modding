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

// FILE: CreateObjectDie.cpp ///////////////////////////////////////////////////////////////////////////
// Author: Michael S. Booth, January 2002
// Desc:   Create an object upon this object's death
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#define DEFINE_OBJECT_STATUS_NAMES
#define DEFINE_MAXHEALTHCHANGETYPE_NAMES						// for TheMaxHealthChangeTypeNames[]
#define DEFINE_DISPOSITION_NAMES
#include "GameLogic/Module/AIUpdate.h"
#include "Common/Player.h"
#include "Common/ThingFactory.h"
#include "Common/Xfer.h"
#include "GameLogic/AIPathfind.h"
#include "GameLogic/ExperienceTracker.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/PartitionManager.h"
#include "GameLogic/ScriptEngine.h"
#include "GameLogic/TerrainLogic.h"
#include "GameLogic/Module/CreateObjectDie.h"
#include "GameLogic/Module/AssaultTransportAIUpdate.h"
#include "GameLogic/Module/DozerAIUpdate.h"
#include "GameLogic/Module/FloatUpdate.h"
#include "GameLogic/Module/HijackerUpdate.h"
#include "GameLogic/Module/PhysicsUpdate.h"
#include "GameLogic/Module/StickyBombUpdate.h"
#include "GameLogic/Module/SupplyTruckAIUpdate.h"
#include "GameLogic/Object.h"
#include "GameLogic/Weapon.h" // NoMaxShotsLimit
#include "GameLogic/ObjectCreationList.h"
#include "GameLogic/Module/ContainModule.h"
#include "GameLogic/Module/BodyModule.h"
#include "GameClient/Drawable.h"
#include "GameClient/InGameUI.h"


// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
CreateObjectDieModuleData::CreateObjectDieModuleData()
{

	m_ocl = nullptr;

	m_objectCreationData.m_inheritsPreviousHealthDontTransferAttackers = FALSE;
	m_objectCreationData.m_transferHijackers = TRUE;
	m_objectCreationData.m_transferEquippers = TRUE;
	m_objectCreationData.m_transferParasites = TRUE;
	m_objectCreationData.m_inheritsHealthChangeType = ADD_CURRENT_DAMAGE;
	m_objectCreationData.m_destroyBombs = TRUE;
	m_objectCreationData.m_ignorePrimaryObstacle = TRUE;
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
/*static*/ void CreateObjectDieModuleData::buildFieldParse(MultiIniFieldParse& p)
{
	DieModuleData::buildFieldParse(p);

	static const FieldParse dataFieldParse[] =
	{
		{ "CreationList",	INI::parseObjectCreationList,		nullptr,											offsetof( CreateObjectDieModuleData, m_ocl ) },
		{ "TransferPreviousHealth", INI::parseBool, nullptr	,offsetof( CreateObjectDieModuleData, m_objectCreationData.m_inheritsHealth ) },
		{ "TransferHealthChangeType",		INI::parseIndexList,		TheMaxHealthChangeTypeNames, offsetof( CreateObjectDieModuleData, m_objectCreationData.m_inheritsHealthChangeType ) },
		{ "TransferSelection", INI::parseBool, nullptr, offsetof( CreateObjectDieModuleData, m_objectCreationData.m_inheritsSelection ) },
		{ "TransferSelectionDontClearGroup", INI::parseBool, nullptr, offsetof( CreateObjectDieModuleData, m_objectCreationData.m_inheritsSelectionDontClearGroup ) },
		{ "TransferSelectionSquadNumber", INI::parseBool, nullptr, offsetof( CreateObjectDieModuleData, m_objectCreationData.m_inheritsSquadNumber ) },

		{ nullptr, nullptr, nullptr, 0 }
	};
	p.add(dataFieldParse);
	p.add(ObjectCreationMuxData::getFieldParse(), offsetof( CreateObjectDieModuleData, m_objectCreationData ));

}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
CreateObjectDie::CreateObjectDie( Thing *thing, const ModuleData* moduleData ) : DieModule( thing, moduleData )
{
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
CreateObjectDie::~CreateObjectDie( void )
{

}

//-------------------------------------------------------------------------------------------------
/** The die callback. */
//-------------------------------------------------------------------------------------------------
void CreateObjectDie::onDie( const DamageInfo * damageInfo )
{
	const CreateObjectDieModuleData *data = getCreateObjectDieModuleData();
	if (!isDieApplicable(damageInfo))
		return;

	Object *me = getObject();

	if(checkIfDontHaveLivePlayer(me) || checkIfSkipWhileSignificantlyAirborne(me))
		return;

	Bool requiresContainer = FALSE;
	Object* container = getPutInContainer(requiresContainer, me->getTeam());
	if(requiresContainer && !container)
	{
		//DEBUG_CRASH( ("CreateObjectDie::onDie() failed to create container %s.", m_objectCreationData.m_putInContainer.str() ) );
		return;
	}

	Object *damageDealer = TheGameLogic->findObjectByID( damageInfo->in.m_sourceID );

	Object *newObject = ObjectCreationList::create( data->m_ocl, getObject(), damageDealer );
	if(!newObject)
		return;

	if(container == nullptr)
		doPreserveLayer(me, newObject);

	setProducer(me, newObject);
	doObjectCreation(me, newObject);
	doDisposition(me, newObject, me->getPosition(), me->getTransformMatrix(), me->getOrientation(), TRUE);
	doInherit(me, newObject, me->getStatusBits());
	doTransfer(me, newObject, FALSE);
	doPostDisposition(me, newObject);
	doInheritHealth(me, newObject);
	doInheritSelection(me, newObject);
	doFadeStuff(me);

	if(container != nullptr)
	{
		doPreserveLayer(me, container);
		setProducer(me, container);
		doObjectCreation(me, container);
		doDisposition(me, container, me->getPosition(), me->getTransformMatrix(), me->getOrientation(), TRUE);
		doInherit(me, container, me->getStatusBits());
		doTransfer(me, container, FALSE);
		doPostDisposition(me, container);
		doInheritHealth(me, container);
		doInheritSelection(me, container);
		if(container->getContain() != nullptr && !container->isEffectivelyDead() && newObject && !newObject->isEffectivelyDead() && container->getContain()->isValidContainerFor(newObject, true))
			container->getContain()->addToContain(newObject);
	}

	// Transfer any bombs onto the replacement Object
	//std::vector<ObjectID> BombsMarkedForDestroy;
	//Object *obj = TheGameLogic->getFirstObject();
	//while( obj )
	//{
		// Transfer bombs to the replacement Object or destroy them
		//if( obj->isKindOf( KINDOF_MINE ) )
		//{
			//static NameKeyType key_StickyBombUpdate = NAMEKEY( "StickyBombUpdate" );
			//StickyBombUpdate *update = (StickyBombUpdate*)obj->findUpdateModule( key_StickyBombUpdate );
		//	StickyBombUpdateInterface *update = obj->getStickyBombUpdateInterface();
		//	if( update && update->getTargetObject() == me )
		//	{
		//		if(data->m_transferBombs)
		//			update->setTargetObject( newObject );
		//		else
		//			BombsMarkedForDestroy.push_back(obj->getID());
		//	}
		//}

		// Transfer attackers
		/// IamInnocent - Transfer Previous Health also transfer the Attackers for this module.
		//if ((data->m_transferPreviousHealth && !data->m_transferPreviousHealthDontTransferAttackers) || data->m_transferAttackers)
		//{
		//	AIUpdateInterface* aiInterface = obj->getAI();
		//	if (aiInterface)
		//		aiInterface->transferAttack(me->getID(), newObject->getID());

		//}

		//obj = obj->getNextObject();
	//}

	// Or not, we just get rid of the bomb
	//for(std::vector<ObjectID>::const_iterator it = BombsMarkedForDestroy.begin(); it != BombsMarkedForDestroy.end(); ++it)
	//{
	//	Object *bomb = TheGameLogic->findObjectByID(*it);
	//	if(bomb)
	//		TheGameLogic->destroyObject(bomb);
	//}

	// Transfer the Selection Status
	/// IamInnocent - Integrated with the selection module from TheSuperHackers below
	/*if(data->m_transferSelection && newObject->isSelectable() && me->getDrawable() && newObject->getDrawable())
	{
		if(me->getDrawable()->isSelected())
			TheGameLogic->selectObject(newObject, FALSE, me->getControllingPlayer()->getPlayerMask(), me->isLocallyControlled());
	}*/

	// TheSuperHackers @bugfix Stubbjax 02/10/2025 If the old object was selected, select the new one.
	// This is important for the Sneak Attack, which is spawned via a CreateObjectDie module.
	// IamInnocent 02/12/2025 - Edited for Multi-Select of modules
	//if (data->m_transferSelection && me->getDrawable() && me->getDrawable()->isSelected())
	//{
		//Object* oldObject = getObject();
		//Drawable* selectedDrawable = TheInGameUI->getFirstSelectedDrawable();
		//Bool oldObjectSelected = selectedDrawable && selectedDrawable->getID() == oldObject->getDrawable()->getID();

		//if (oldObjectSelected)
		//GameMessage* msg = TheMessageStream->appendMessage(GameMessage::MSG_CREATE_SELECTED_GROUP_NO_SOUND);
		//if(data->m_transferSelectionDontClearGroup)
		//{
		//	msg->appendBooleanArgument(FALSE);
		//}
		//else
		//{
		//	msg->appendBooleanArgument(TRUE);
		//}

		//msg->appendObjectIDArgument(newObject->getID());
		//TheInGameUI->selectDrawable(newObject->getDrawable());
	//}

}


// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void CreateObjectDie::crc( Xfer *xfer )
{

	// extend base class
	DieModule::crc( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void CreateObjectDie::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	DieModule::xfer( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void CreateObjectDie::loadPostProcess( void )
{

	// extend base class
	DieModule::loadPostProcess();

}

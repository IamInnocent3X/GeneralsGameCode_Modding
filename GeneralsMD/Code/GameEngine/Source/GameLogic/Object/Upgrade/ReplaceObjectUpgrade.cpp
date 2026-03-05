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

// FILE: ReplaceObjectUpgrade.cpp /////////////////////////////////////////////////////////////////////////////
// Author: Graham Smallwood, July 2003
// Desc:	 UpgradeModule that creates a new Object in our exact location and then deletes our object
///////////////////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#define DEFINE_MAXHEALTHCHANGETYPE_NAMES						// for TheMaxHealthChangeTypeNames[]
#define DEFINE_DISPOSITION_NAMES								// For DispositionNames[]

#include "GameLogic/Module/ReplaceObjectUpgrade.h"

#include "Common/Player.h"
#include "Common/ThingFactory.h"
#include "Common/ThingTemplate.h"
#include "GameLogic/AI.h"
#include "GameLogic/AIPathfind.h"
#include "GameLogic/ExperienceTracker.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/PartitionManager.h"
#include "GameLogic/ScriptEngine.h"
#include "GameLogic/TerrainLogic.h"
#include "GameLogic/Module/AIUpdate.h"
#include "GameLogic/Module/AssaultTransportAIUpdate.h"
#include "GameLogic/Module/BodyModule.h"
#include "GameLogic/Module/DozerAIUpdate.h"
#include "GameLogic/Module/FloatUpdate.h"
#include "GameLogic/Module/HijackerUpdate.h"
#include "GameLogic/Module/PhysicsUpdate.h"
#include "GameLogic/Module/StickyBombUpdate.h"
#include "GameLogic/Module/SupplyTruckAIUpdate.h"
#include "GameLogic/Module/CreateModule.h"
#include "GameLogic/Module/ContainModule.h"
#include "GameLogic/Module/CreateObjectDie.h"					// For DispositionNames
#include "GameLogic/Object.h"
#include "GameLogic/Weapon.h" // NoMaxShotsLimit
#include "GameClient/Drawable.h"
#include "GameClient/InGameUI.h"


// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
void ReplaceObjectUpgradeModuleData::buildFieldParse(MultiIniFieldParse& p)
{
  UpgradeModuleData::buildFieldParse(p);

	static const FieldParse dataFieldParse[] =
	{
		{ "ReplaceObject",	INI::parseAsciiString,	nullptr, offsetof( ReplaceObjectUpgradeModuleData, m_replaceObjectName ) },
		{ nullptr, nullptr, nullptr, 0 }
	};
  p.add(dataFieldParse);
  p.add(ObjectCreationMuxData::getFieldParse(), offsetof( ReplaceObjectUpgradeModuleData, m_objectCreationData ));
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
ReplaceObjectUpgrade::ReplaceObjectUpgrade( Thing *thing, const ModuleData* moduleData ) : UpgradeModule( thing, moduleData )
{
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
ReplaceObjectUpgrade::~ReplaceObjectUpgrade()
{
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void ReplaceObjectUpgrade::upgradeImplementation()
{
	const ReplaceObjectUpgradeModuleData *data = getReplaceObjectUpgradeModuleData();
	const ThingTemplate* replacementTemplate = TheThingFactory->findTemplate(data->m_replaceObjectName);

	Object *me;

	if(checkIfDontHaveLivePlayer(me) || checkIfSkipWhileSignificantlyAirborne(me))
		return;

	Bool oldObjectSelected;
	Int oldObjectSquadNumber;
	Matrix3D myMatrix;
	Team* myTeam;

	//AIUpdateInterface *ai;
	ObjectStatusMaskType prevStatus;

	{
		me = getObject();

		myMatrix = *me->getTransformMatrix();
		myTeam = me->getTeam();// Team implies player.  It is a subset.

		if (replacementTemplate == nullptr)
		{
			DEBUG_ASSERTCRASH(replacementTemplate != nullptr, ("No such object '%s' in ReplaceObjectUpgrade.", data->m_replaceObjectName.str()));
			return;
		}

		// IamInnocent - Edited for Selection To Not Clear Group
		Drawable* selectedDrawable = TheInGameUI->getFirstSelectedDrawable();
		oldObjectSelected = data->m_objectCreationData.m_inheritsSelectionDontClearGroup ? me->getDrawable() && me->getDrawable()->isSelected() : selectedDrawable && selectedDrawable->getID() == me->getDrawable()->getID();
		oldObjectSquadNumber = me->getControllingPlayer()->getSquadNumberForObject(me);

		//ai = me->getAI();
		prevStatus = me->getStatusBits();

		// Remove us first since occupation of cells is apparently not a refcount, but a flag.  If I don't remove, then the new
		// thing will be placed, and then on deletion I will remove "his" marks.
		//TheAI->pathfinder()->removeObjectFromPathfindMap(me);
		//TheGameLogic->destroyObject(me);
	}

	// Create the container before the replacement Object first
	Bool requiresContainer = FALSE;
	Object* container = getPutInContainer(requiresContainer, me->getTeam());
	if(requiresContainer && !container)
	{
		//DEBUG_CRASH( ("CreateObjectDie::onDie() failed to create container %s.", m_objectCreationData.m_putInContainer.str() ) );
		return;
	}

	// Don't destroy us first, make us non-interactable
	{
		me->setStatus( MAKE_OBJECT_STATUS_MASK3( OBJECT_STATUS_NO_COLLISIONS, OBJECT_STATUS_MASKED, OBJECT_STATUS_UNSELECTABLE ) );
		me->leaveGroup();
		//if( ai )
		//{
			//By setting him to idle, we will prevent him from entering the target after this gets called.
		//	ai->aiIdle( CMD_FROM_AI );
		//}
		// remove from partition manager
		ThePartitionManager->unRegisterObject( me );
	}

	Object *replacementObject = TheThingFactory->newObject(replacementTemplate, myTeam);
	replacementObject->setTransformMatrix(&myMatrix);
	TheAI->pathfinder()->addObjectToPathfindMap( replacementObject );

	if(container == nullptr)
		doPreserveLayer(me, replacementObject);
	else
	{
		container->setTransformMatrix(&myMatrix);
		TheAI->pathfinder()->addObjectToPathfindMap( container );

		// Now onCreates were called at the constructor.  This magically created
		// thing needs to be considered as Built for Game specific stuff.
		for (BehaviorModule** m = container->getBehaviorModules(); *m; ++m)
		{
			CreateModuleInterface* create = (*m)->getCreate();
			if (!create)
				continue;
			create->onBuildComplete();
		}
	}

	setProducer(me, replacementObject);
	doObjectCreation(me, replacementObject);
	doDisposition(me, replacementObject, me->getPosition(), me->getTransformMatrix(), me->getOrientation(), TRUE);

	// Now onCreates were called at the constructor.  This magically created
	// thing needs to be considered as Built for Game specific stuff.
	for (BehaviorModule** m = replacementObject->getBehaviorModules(); *m; ++m)
	{
		CreateModuleInterface* create = (*m)->getCreate();
		if (!create)
			continue;
		create->onBuildComplete();
	}

	doInherit(me, replacementObject, prevStatus);
	doTransfer(me, replacementObject, FALSE);
	doPostDisposition(me, replacementObject);
	doInheritHealth(me, replacementObject);
	doFadeStuff(me);

	if(container != nullptr)
	{
		doPreserveLayer(me, container);
		setProducer(me, container);
		doObjectCreation(me, container);
		doDisposition(me, container, me->getPosition(), me->getTransformMatrix(), me->getOrientation(), TRUE);
		doInherit(me, container, prevStatus);
		doTransfer(me, container, FALSE);
		doPostDisposition(me, container);
		doInheritHealth(me, container);
		if(container->getContain() != nullptr && !container->isEffectivelyDead() && replacementObject && !replacementObject->isEffectivelyDead() && container->getContain()->isValidContainerFor(replacementObject, true))
			container->getContain()->addToContain(replacementObject);
	}

	// Now we destroy the Object
	TheAI->pathfinder()->removeObjectFromPathfindMap( me );
	TheGameLogic->destroyObject(me);

	if( replacementObject->getControllingPlayer() )
	{
		replacementObject->getControllingPlayer()->onStructureConstructionComplete(nullptr, replacementObject, FALSE);

		doInheritSelection(nullptr, replacementObject, oldObjectSelected, oldObjectSquadNumber);

		// TheSuperHackers @bugfix Stubbjax 26/05/2025 If the old object was selected, select the new one.
		// IamInnocent 02/12/2025 - Integrated with Transfer Selection Feature added Below
		//if (oldObjectSelected)
		//{
		//	GameMessage* msg = TheMessageStream->appendMessage(GameMessage::MSG_CREATE_SELECTED_GROUP_NO_SOUND);
		//	msg->appendBooleanArgument(TRUE);
		//	msg->appendObjectIDArgument(replacementObject->getID());
		//	TheInGameUI->selectDrawable(replacementObject->getDrawable());
		//}

		// TheSuperHackers @bugfix Stubbjax 26/05/2025 If the old object was grouped, group the new one.
		//if (oldObjectSquadNumber != NO_HOTKEY_SQUAD)
		//{
		//	if (replacementObject->isLocallyControlled())
		//	{
		//		GameMessage* msg = TheMessageStream->appendMessage((GameMessage::Type)(GameMessage::MSG_CREATE_TEAM0 + oldObjectSquadNumber));
		//		msg->appendObjectIDArgument(replacementObject->getID());
		//	}
		//}

		// Transfer the Selection Status
		/// IamInnocent 02/12/2025 - Integrated with the selection module from TheSuperHackers @bugfix Stubbjax 26/05/2025
		//if(oldObjectSelected)
		//{
		//	GameMessage* msg = TheMessageStream->appendMessage(GameMessage::MSG_CREATE_SELECTED_GROUP_NO_SOUND);
		//	if(data->m_transferSelectionDontClearGroup)
		//	{
		//		msg->appendBooleanArgument(FALSE);
		//	}
		//	else
		//	{
		//		msg->appendBooleanArgument(TRUE);
		//	}

		//	msg->appendObjectIDArgument(replacementObject->getID());
		//	TheInGameUI->selectDrawable(replacementObject->getDrawable());

		//	// TheSuperHackers @bugfix Stubbjax 26/05/2025 If the old object was grouped, group the new one.
		//	if (oldObjectSquadNumber != NO_HOTKEY_SQUAD)
		//	{
		//		if (replacementObject->isLocallyControlled())
		//		{
		//			GameMessage* msg = TheMessageStream->appendMessage((GameMessage::Type)(GameMessage::MSG_CREATE_TEAM0 + oldObjectSquadNumber));
		//			msg->appendObjectIDArgument(replacementObject->getID());
		//		}
		//	}
		//}

	}

	if( container && container->getControllingPlayer() )
	{
		container->getControllingPlayer()->onStructureConstructionComplete(nullptr, container, FALSE);

		doInheritSelection(nullptr, container, oldObjectSelected, oldObjectSquadNumber);

	}
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void ReplaceObjectUpgrade::crc( Xfer *xfer )
{

	// extend base class
	UpgradeModule::crc( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void ReplaceObjectUpgrade::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	UpgradeModule::xfer( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void ReplaceObjectUpgrade::loadPostProcess()
{

	// extend base class
	UpgradeModule::loadPostProcess();

}

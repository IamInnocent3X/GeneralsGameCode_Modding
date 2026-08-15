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

// FILE: CaveContain.h ////////////////////////////////////////////////////////////////////////////
// Author: Graham Smallwood, July 2002
// Desc:   A version of OpenContain that overrides where the passengers are stored: one of CaveManager's
//					entries. Changing entry is a script or ini command.  All queries about capacity and
//					contents are also redirected.
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "GameLogic/Module/CreateModule.h"
#include "GameLogic/Module/OpenContain.h"

struct Sound;
class Team;

//-------------------------------------------------------------------------------------------------
class CaveContainModuleData : public OpenContainModuleData
{
public:
	Int m_caveIndexData;
	Bool m_caveHasOwner;
	Bool m_caveUsesTeams;
	Bool m_caveCaptureLinkCaves;

	CaveContainModuleData()
	{
		m_caveIndexData = 0;// By default, all Caves will be grouped together as number 0
		m_caveHasOwner = FALSE;
		m_caveUsesTeams = FALSE;
		m_caveCaptureLinkCaves = FALSE;
	}

	static void buildFieldParse(MultiIniFieldParse& p)
	{
    OpenContainModuleData::buildFieldParse(p);

		static const FieldParse dataFieldParse[] =
		{
			{ "CaveIndex", INI::parseInt, nullptr, offsetof( CaveContainModuleData, m_caveIndexData ) },
			{ "CaveHasOwner", INI::parseBool, nullptr, offsetof( CaveContainModuleData, m_caveHasOwner ) },
			{ "CaveUsesTeams", INI::parseBool, nullptr, offsetof( CaveContainModuleData, m_caveUsesTeams ) },
			{ "CaveCaptureLinkCaves", INI::parseBool, nullptr, offsetof( CaveContainModuleData, m_caveCaptureLinkCaves ) },
			{ 0, 0, 0, 0 }
		};
    p.add(dataFieldParse);
	}
};

//-------------------------------------------------------------------------------------------------
class CaveContain : public OpenContain, public CreateModuleInterface, public CaveInterface
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( CaveContain, "CaveContain" )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( CaveContain, CaveContainModuleData )

public:

	CaveContain( Thing *thing, const ModuleData* moduleData );
	// virtual destructor prototype provided by memory pool declaration

	virtual CreateModuleInterface* getCreate() override { return this; }
	virtual CaveInterface* getCaveInterface() override { return this; }
	static Int getInterfaceMask() { return OpenContain::getInterfaceMask() | (MODULEINTERFACE_CREATE); }

	virtual OpenContain *asOpenContain() override { return this; }  ///< treat as open container
	virtual Bool isGarrisonable() const override { return false; }	///< can this unit be Garrisoned? (ick)
	virtual Bool isBustable() const override { return TRUE; }	///< can this container get busted by a bunkerbuster
	virtual Bool isHealContain() const override { return false; } ///< true when container only contains units while healing (not a transport!)

	virtual void onContaining( Object *obj, Bool wasSelected ) override;		///< object now contains 'obj'
	virtual void onRemoving( Object *obj ) override;			///< object no longer contains 'obj'
	virtual void onSelling() override;///< Container is being sold.  Tunnel responds by kicking people out if this is the last tunnel.
	virtual void onCapture( Player *oldOwner, Player *newOwner ) override; // Need to change who we are registered with.

	virtual Bool isValidContainerFor(const Object* obj, Bool checkCapacity) const override;
	virtual void addToContainList( Object *obj ) override;		///< The part of AddToContain that inheritors can override (Can't do whole thing because of all the private stuff involved)
	virtual void removeFromContain( Object *obj, Bool exposeStealthUnits = FALSE ) override;	///< remove 'obj' from contain list
	virtual void removeAllContained( Bool exposeStealthUnits = FALSE ) override;				///< remove all objects on contain list

	virtual void orderAllPassengersToExit( CommandSourceType commandSource, Bool instantly ) override; ///< All of the smarts of exiting are in the passenger's AIExit. removeAllFrommContain is a last ditch system call, this is the game Evacuate

	// Bunker Buster Properties
	virtual void harmAndForceExitAllContained( DamageInfo *info ) override;
  virtual void killAllContained() override;				///< kill all objects on contain list

	/**
		return the player that *appears* to control this unit. if null, use getObject()->getControllingPlayer() instead.
	*/
	virtual void recalcApparentControllingPlayer() override;

	// contain list access
	virtual void iterateContained( ContainIterateFunc func, void *userData, Bool reverse ) override;
	virtual UnsignedInt getContainCount() const override;
	virtual Int getRawContainMax() const override;
	virtual Int getContainMax() const override;
	virtual const ContainedItemsList* getContainedItemsList() const override;
	virtual Bool isDisplayedOnControlBar() const override { return TRUE; } ///< Does this container display its contents on the ControlBar?
	virtual Bool isKickOutOnCapture() override { return FALSE; }///< Caves and Tunnels don't kick out on capture.

	// override the onDie we inherit from OpenContain
	virtual void onDie( const DamageInfo *damageInfo ) override;  ///< the die callback

	virtual void onDelete() override;
	virtual void onCreate() override;
	virtual void onBuildComplete() override;	///< This is called when you are a finished game object
	virtual void onObjectCreated() override;
	virtual Bool shouldDoOnBuildComplete() const override { return m_needToRunOnBuildComplete; }

	// Unique to Cave Contain
	virtual void tryToSetCaveIndex( Int newIndex ) override;	///< Called by script as an alternative to instancing separate objects.  'Try', because can fail.
	virtual void setOriginalTeam( Team *oldTeam ) override;	///< This is a distributed Garrison in terms of capturing, so when one node triggers the change, he needs to tell everyone, so anyone can do the un-change.
	virtual Bool getHasPermanentOwner() const override;
	virtual Team* getOldTeam() const override;

	virtual UpdateSleepTime update() override;												///< called once per frame

protected:

	void changeTeamOnAllConnectedCaves( Team *newTeam, Bool setOriginalTeams );	///< When one gets captured, all connected ones get captured.  DistributedGarrison.
	void changeTeamOnAllConnectedCavesByTeam( Team *checkTeam, Bool setOriginalTeams, Team *newTeam );	///< When one gets captured, all connected ones get captured.  DistributedGarrison.
	void registerNewCave();
	void switchCaveOwners( Team *oldTeam, Team *newTeam );
	void doCapture( Team *oldTeam, Team *newTeam );
	void removeAllNonOwnContained( Team *myTeam, Bool exposeStealthUnits = FALSE );				///< remove all objects on contain list
	void scatterToNearbyPosition(Object *obj);

	virtual void createPayload();

	Bool m_needToRunOnBuildComplete;
	Int m_caveIndex;

	Team *m_originalTeam;												///< our original team before we were garrisoned

private:

	Bool m_loaded;
	Bool m_switchingOwners;
	//Bool m_payloadCreated;
	Bool m_isCaptured;
	UnsignedInt m_containingFrames;
	Team *m_capturedTeam;
	Team *m_oldTeam;
	Team *m_newTeam;
};

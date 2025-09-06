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

// FILE: RiderChangeContain.cpp //////////////////////////////////////////////////////////////////////
// Author: Kris Morness, May 2003
// Desc:   Contain module for the combat bike (transport that switches units).
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef __RIDER_CHANGE_CONTAIN_H
#define __RIDER_CHANGE_CONTAIN_H

// USER INCLUDES //////////////////////////////////////////////////////////////////////////////////
#include "GameLogic/Module/TransportContain.h"

#define MAX_RIDERS 8 //***NOTE: If you change this, make sure you update the parsing section!

enum WeaponSetType CPP_11(: Int);
enum ObjectStatusType CPP_11(: Int);
enum LocomotorSetType CPP_11(: Int);

struct RiderInfo
{
	AsciiString m_templateName;
	WeaponSetType m_weaponSetFlag;
	ModelConditionFlagType m_modelConditionFlagType;
	ObjectStatusType m_objectStatusType;
	AsciiString m_commandSet;
	LocomotorSetType m_locomotorSetType;
	AsciiString m_objectCustomStatusType;
};

struct RiderData
{
	AsciiString templateName;
	UnsignedInt timeFrame;
	ObjectStatusType statusType;
	AsciiString customStatusType;
};

enum ScuttleType CPP_11(: Int)
{
	SCUTTLE_INVALID = 0,
	SCUTTLE_ON_EXIT = 1,
	SCUTTLE_ON_NO_PASSENGERS = 2,
	SCUTTLE_NEVER = 3,

	SCUTTLE_COUNT

};

#ifdef DEFINE_SCUTTLE_NAMES
static const char *TheScuttleNames[] = 
{
	"INVALID",
	"ON_EXIT",
	"ON_NO_PASSENGERS",
	"NEVER",

	NULL
};
#endif


//-------------------------------------------------------------------------------------------------
class RiderChangeContainModuleData : public TransportContainModuleData
{
public:

	RiderInfo m_riders[ MAX_RIDERS ];
	std::vector<RiderInfo> m_ridersCustom;
	UnsignedInt m_scuttleFrames;
	ModelConditionFlagType m_scuttleState;
	Bool m_riderNotRequired;
	Bool m_useUpgradeNames;
	Bool m_dontDestroyPassengersOnKill;
	Bool m_dontEvacuateOnEnter;
	Bool m_canContainNonRiders;
	Bool m_moreThanOneRiders;
	ScuttleType m_scuttleType;

	RiderChangeContainModuleData();

	static void buildFieldParse(MultiIniFieldParse& p);
	static void parseRiderInfo( INI* ini, void *instance, void *store, const void* /*userData*/ );
	static void parseRiderInfoCustom( INI* ini, void *instance, void *store, const void* /*userData*/ );

};

//-------------------------------------------------------------------------------------------------
class RiderChangeContain : public TransportContain
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( RiderChangeContain, "RiderChangeContain" )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( RiderChangeContain, RiderChangeContainModuleData )

public:

	RiderChangeContain( Thing *thing, const ModuleData* moduleData );
	// virtual destructor prototype provided by memory pool declaration

	virtual Bool isValidContainerFor( const Object* obj, Bool checkCapacity) const;

	virtual void onCapture( Player *oldOwner, Player *newOwner ); // have to kick everyone out on capture.
	virtual void onContaining( Object *obj, Bool wasSelected );		///< object now contains 'obj'
	virtual void onRemoving( Object *obj );			///< object no longer contains 'obj'
	virtual UpdateSleepTime update();							///< called once per frame

	virtual Bool isRiderChangeContain() const { return TRUE; }
	virtual const Object *friend_getRider() const;

	virtual Int getContainMax( void ) const;

	virtual Int getExtraSlotsInUse( void ) { return m_extraSlotsInUse; }///< Transports have the ability to carry guys how take up more than spot.

	virtual Bool isExitBusy() const;	///< Contain style exiters are getting the ability to space out exits, so ask this before reserveDoor as a kind of no-commitment check.
	virtual ExitDoorType reserveDoorForExit( const ThingTemplate* objType, Object *specificObject );
	virtual void unreserveDoorForExit( ExitDoorType exitDoor );
	virtual Bool isDisplayedOnControlBar() const {return TRUE;}///< Does this container display its contents on the ControlBar?

	virtual Bool getContainerPipsToShow( Int& numTotal, Int& numFull );

	virtual void orderAllPassengersToExit( CommandSourceType commandSource, Bool instantly ); ///< All of the smarts of exiting are in the passenger's AIExit. removeAllFrommContain is a last ditch system call, this is the game Evacuate

	virtual void doUpgradeChecks( void );
	virtual void doStatusChecks( void );

	void changeRiderTemplateOnStatusUpdate();
	Bool riderTemplateIsValidChange(ObjectStatusMaskType newStatus);
	Bool riderTemplateIsValidChange(const AsciiString& newCustomStatus);
	Bool riderTemplateIsValidRemoval(ObjectStatusMaskType newStatus);
	Bool riderTemplateIsValidRemoval(const AsciiString& newCustomStatus);
	void riderGiveTemplate(RiderData riderData);
	//void riderRemoveAll(); // The more expensive option. Execute once for loaded units to fix object states.

	void loadPreviousState(); // Loads the previous Rider Template of the existing unit if StatusCheck or UpgradeCheck is enabled
	void doRegisterUpgradeNames();
	void registerNewRiderDataOnContain(RiderData riderData);
	void removeRiderTemplate(const AsciiString& rider, Bool clearStatus); // Remove the template
	void removeRiderDataRecord(const AsciiString& rider); // Use in line with Remove Template, also clears it from the Rider Change Records
	void clearAllTimeFramedDataRecords();
	
	Bool checkHasRiderTemplate( const Object *rider ) const;
	Bool compareRiderStatus(const AsciiString& rider, Int checkIndex) const;
	Bool riderChangeContainingCheck( Object *rider, const RiderInfo& riderInfo );
	Bool riderChangeRemoveCheck( Object *rider, const RiderInfo& riderInfo, Bool checkRecordOnly);

protected:

	// exists primarily for RiderChangeContain to override
	virtual void killRidersWhoAreNotFreeToExit();
	virtual Bool isSpecificRiderFreeToExit(Object* obj);
	virtual void createPayload();

private:

	Int m_extraSlotsInUse;
	UnsignedInt m_frameExitNotBusy;
	UnsignedInt m_scuttledOnFrame;

	Bool m_containing; //doesn't require xfer.
	Bool m_loaded; //same
	Bool m_dontCompare; //me too
	Bool m_registeredUpgradeNames; // me three

	ObjectStatusMaskType m_prevStatus;
	ObjectCustomStatusType m_prevCustomStatusTypes;

	ObjectStatusType m_riderDataStatusRegister;
	AsciiString m_riderDataCustomStatusRegister;

	struct RiderUpgrade
	{
		AsciiString									templateName;
		Int											templateRider;

		RiderUpgrade() : templateName(NULL), templateRider(-1)
		{
		}
	};

	std::vector<RiderUpgrade>			m_upgradeTemplates;
	UpgradeMaskType						m_prevMaskToCheck;
	std::vector<RiderData>				m_theRiderDataRecord;

};

#endif // __RIDER_CHANGE_CONTAIN_H


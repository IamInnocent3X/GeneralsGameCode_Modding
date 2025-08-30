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
	Bool m_dontScuttle;
	Bool m_dontDestroyRiderOnKill;

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

	void changeRiderTemplateOnStatusUpdate();
	//Bool riderTemplateIsValidChange(ObjectStatusMaskType newCStatus, const AsciiString& newCustomStatus);
	Bool riderTemplateIsValidChange(ObjectStatusMaskType newStatus, Bool set);
	Bool riderTemplateIsValidChange(const AsciiString& newCustomStatus, Bool set);
	void riderRemoveTemplate(Int RiderIndex, const AsciiString& RiderName);
	//void riderGiveTemplate(Int RiderIndex, const AsciiString& RiderName);
	void riderGiveTemplate(Int RiderIndex, const AsciiString& RiderName);
	void riderGiveTemplateStatus(Int RiderIndex, const AsciiString& RiderName); //Status Giver Only. A hackky way for fixing saving and loading.
	void riderRemoveAll(); // The more expensive option. Execute once for loaded units to fix object states.
	void riderResetModel();
	void riderReset();

	void doRegisterUpgradeNames();
	void riderUpdateUpgradeModules();
	
	Bool doRiderChangeOnContaining( Object *rider, const RiderInfo& riderInfo );
	Bool doRiderChangeOnRemoving( Object *rider, const RiderInfo& riderInfo );

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
	Bool m_firstChange; //me three
	Bool m_firstUpgrade; //me four
	Bool m_firstStatusRemoval; //me five, because there's Xfers don't work.
	Bool m_statusDelayCheck; //me last. Six Bullets

	Bool m_doRemovalOnLoad; //not me

	// Default value for assigning Status and Upgrade Types as Rider Templates
	Int m_theRiderToChange;
	AsciiString m_theRiderName;

	// Applies when the Status is last removed to change back to the last Valid Rider. 
	// Overridens if new Rider or new Upgrade Template is assigned.
	Int m_theRiderLastValid;
	AsciiString m_theRiderLastName;

	// Preserves the values of the Last Valid Riders for Xfer hackyy fixes.
	// Does nothing if you don't use Save/Load functions
	Int m_theRiderLastValidBackup;
	AsciiString m_theRiderLastNameBackup;

	// Preserves the values of the Upgrade Template Riders for Xfer hackyy fixes.
	// Does nothing if you don't use Save/Load functions
	Int m_theRiderLastUpgrade;
	AsciiString m_theRiderLastUpgradeName;

	// The last tiny bit about the loading of Applied Status fix.
	// And, you guessed it, it's about Xfer.
	Int m_theRiderToChangeBackup;
	AsciiString m_theRiderNameBackup;

	Bool m_checkedRemove;
	ObjectStatusMaskType m_prevStatus;
	ObjectCustomStatusType m_prevCustomStatusTypes;

	Bool m_registeredUpgradeNames;
	Int m_addDelay;
	Int m_statusLoadDelay;
	Int m_resetLaterFrame;

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

};

#endif // __RIDER_CHANGE_CONTAIN_H


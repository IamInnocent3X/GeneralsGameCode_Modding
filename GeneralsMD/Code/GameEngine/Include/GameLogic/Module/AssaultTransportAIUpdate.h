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

// AssaultTransportAIUpdate.h ////////////
// Author: Kris Morness, December 2002
// Desc:   State machine that allows assault transports (troop crawler) to deploy
//         troops, order them to attack, then return. Can do extra things like ordering
//         injured troops to return to the transport for healing purposes.

#pragma once

#include "Common/StateMachine.h"
#include "GameLogic/Module/AIUpdate.h"

//-------------------------------------------------------------------------------------------------
enum AssaultStateTypes CPP_11(: Int)
{
	IDLE,							          ///< Not doing anything.
	ASSAULTING,						      ///< Transport is waiting while troops do fighting.
};

#define MAX_TRANSPORT_SLOTS 10

//-------------------------------------------------------------------------------------------------
class AssaultTransportAIUpdateModuleData : public AIUpdateModuleData
{
public:
	Real m_membersGetHealedAtLifeRatio;
	Real m_clearRangeRequiredToContinueAttackMove;
	Bool m_retreatWoundedMembersOnAssault;
	Bool m_canEnterOnMembersExit;

	AssaultTransportAIUpdateModuleData()
	{
		m_membersGetHealedAtLifeRatio = 0.0f;
		m_clearRangeRequiredToContinueAttackMove = 50.0f;
		m_retreatWoundedMembersOnAssault = TRUE;
		m_canEnterOnMembersExit = TRUE;
	}

	static void buildFieldParse(MultiIniFieldParse& p)
	{
    AIUpdateModuleData::buildFieldParse(p);

		static const FieldParse dataFieldParse[] =
		{
			{ "MembersGetHealedAtLifeRatio",						INI::parseReal,	NULL, offsetof( AssaultTransportAIUpdateModuleData, m_membersGetHealedAtLifeRatio ) },
			{ "MembersRetreatOnWounded",						INI::parseBool,	NULL, offsetof( AssaultTransportAIUpdateModuleData, m_retreatWoundedMembersOnAssault ) },
			{ "PassengersCanEnterOnMembersExit",						INI::parseBool,	NULL, offsetof( AssaultTransportAIUpdateModuleData, m_canEnterOnMembersExit ) },
			{ "ClearRangeRequiredToContinueAttackMove", INI::parseReal, NULL, offsetof( AssaultTransportAIUpdateModuleData, m_clearRangeRequiredToContinueAttackMove ) },
			{ 0, 0, 0, 0 }
		};
    p.add(dataFieldParse);
	}
};

class AssaultTransportAIInterface
{
public:
	virtual void beginAssault( const Object *designatedTarget ) const = 0;
	//virtual void checkMembersList() = 0;
	virtual void removeMember( ObjectID passengerID ) = 0;
	virtual void checkPassengerHealth( ObjectID passengerID ) = 0;
	virtual void addMember(ObjectID replacerID) = 0;
	virtual void doAddMembers() = 0;
	virtual void onAttack() = 0;
	virtual Int getCurrentAssaultingMembers() const = 0;
};


//-------------------------------------------------------------------------------------------------
class AssaultTransportAIUpdate : public AIUpdateInterface, public AssaultTransportAIInterface
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( AssaultTransportAIUpdate, "AssaultTransportAIUpdate" )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( AssaultTransportAIUpdate, AssaultTransportAIUpdateModuleData )

private:

public:

	AssaultTransportAIUpdate( Thing *thing, const ModuleData* moduleData );
	// virtual destructor prototype provided by memory pool declaration

 	virtual void aiDoCommand(const AICommandParms* parms);
	virtual Bool isIdle() const;
	virtual UpdateSleepTime update();
	virtual AssaultTransportAIInterface* getAssaultTransportAIInterface() { return this; }
	virtual const AssaultTransportAIInterface* getAssaultTransportAIInterface() const { return this; }
	virtual void beginAssault( const Object *designatedTarget ) const;
	//virtual void checkMembersList();
	virtual void addMember( ObjectID replacerID );
	virtual void removeMember( ObjectID passengerID );
	virtual void checkPassengerHealth( ObjectID passengerID );
	virtual void doAddMembers() { m_doAddMember = TRUE; wakeUpNow(); };
	virtual void onAttack();
	virtual Int getCurrentAssaultingMembers() const { return getAssaultTransportAIUpdateModuleData()->m_canEnterOnMembersExit ? 0 : m_maxNumInTransport; }

	void addMembers();

	//UpdateSleepTime calcSleepTime();

	void reset();
	Bool isMemberWounded( const Object *member ) const;	//Member requires medical attention?
	Bool isMemberHealthy( const Object *member ) const; //Member has full health?
	void retrieveMembers();
	void giveFinalOrders();
  Bool isAttackPointless() const;

protected:

	virtual void privateAttackObject( Object *victim, Int maxShotsToFire, CommandSourceType cmdSource );///< Extension.  Also tell occupants to attackObject
	virtual void privateAttackPosition( const Coord3D *pos, Int maxShotsToFire, CommandSourceType cmdSource );///< Extension.  Also tell occupants to attackPosition
	virtual void privateForceAttackObject( Object *victim, Int maxShotsToFire, CommandSourceType cmdSource );///< Extension.  Also tell occupants to forceAttackObject

	void removeAllMembers();
	void goalReset();

  ObjectID					m_memberIDs[ MAX_TRANSPORT_SLOTS ];
	Bool							m_memberHealing[ MAX_TRANSPORT_SLOTS ];
	Bool							m_newMember[ MAX_TRANSPORT_SLOTS ];
	Bool							m_countedSlotMember[ MAX_TRANSPORT_SLOTS ];
	Bool							m_countedAssaultingMember[ MAX_TRANSPORT_SLOTS ];
  Coord3D						m_attackMoveGoalPos;
  mutable ObjectID	m_designatedTarget;
	AssaultStateTypes	m_state;
	//UnsignedInt				m_framesRemaining;
	Int								m_currentMembers;
	Int								m_maxNumInTransport;
	Int								m_maxNumAttacking;
	Bool							m_isAttackMove;
	Bool							m_isAttackObject;
	Bool							m_newOccupantsAreNewMembers;
	Bool					m_doAddMember;
};

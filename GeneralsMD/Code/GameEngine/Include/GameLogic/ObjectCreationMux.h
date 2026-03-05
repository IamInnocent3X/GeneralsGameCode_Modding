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

// FILE: ObjectCreationMux.h /////////////////////////////////////////////////////////////////////////////////
// Author: IamInnocent, March 2026
// Desc:	 A Mux that enables modules to Inherit or Receive attributes from its Source along with ObjectCreationList's disposition
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Common/INI.h"

// FORWARD REFERENCES /////////////////////////////////////////////////////////////////////////////
class AudioEventRTS;
class Matrix3D;
class FXList;
class Object;
enum  MaxHealthChangeType CPP_11(: Int);

//-------------------------------------------------------------------------------------------------
/** OBJECT DIE MODULE base class */
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
enum DispositionType CPP_11(: Int)
{
	LIKE_EXISTING						= 0x00000001,
	ON_GROUND_ALIGNED				= 0x00000002,
	SEND_IT_FLYING					= 0x00000004,
	SEND_IT_UP							= 0x00000008,
	SEND_IT_OUT							= 0x00000010,
	RANDOM_FORCE						= 0x00000020,
	FLOATING								= 0x00000040,
	INHERIT_VELOCITY				= 0x00000080,
	WHIRLING								= 0x00000100,
	ALIGN_Z_UP							= 0x00000200
};

static const char* DispositionNames[] =
{
	"LIKE_EXISTING",
	"ON_GROUND_ALIGNED",
	"SEND_IT_FLYING",
	"SEND_IT_UP",
	"SEND_IT_OUT",
	"RANDOM_FORCE",
	"FLOATING",
	"INHERIT_VELOCITY",
	"WHIRLING",
	"ALIGN_Z_UP"
};

//-------------------------------------------------------------------------------------------------
enum SetProducerType CPP_11(: Int)
{
	SET_SOURCE,
	SET_SOURCE_PRODUCER,
	SET_CONTAINER,

	SET_COUNT
};

static const char *TheSetProducerNames[] =
{
	"SOURCE",
	"SOURCE_PRODUCER",
	"CONTAINER",
	nullptr
};
static_assert(ARRAY_SIZE(TheSetProducerNames) == SET_COUNT + 1, "Incorrect array size");

//-------------------------------------------------------------------------------------------------
class ObjectCreationMuxData	// does NOT inherit from ModuleData.
{
public:
	Bool											m_destroyBombs;
	Bool											m_destroyHijackers;
	Bool											m_destroyParasites;

	Bool 											m_inheritsHealth;
	Bool 											m_inheritsSelection;
	Bool											m_inheritsExperience;
	Bool											m_inheritsPreviousHealthDontTransferAttackers;
	Bool											m_inheritsAIStates;
	Bool											m_inheritsStatus;
	Bool											m_inheritsWeaponBonus;
	Bool											m_inheritsDisabledType;
	Bool											m_inheritsSelectionDontClearGroup;
	Bool											m_inheritsSquadNumber;
	Bool											m_inheritsObjectName;
	Bool											m_inheritsVeterancy;

	MaxHealthChangeType 							m_inheritsHealthChangeType;

	Bool											m_transferAttackers;
	Bool											m_transferBombs;
	Bool											m_transferHijackers;
	Bool											m_transferEquippers;
	Bool											m_transferParasites;
	Bool											m_transferPassengers;
	Bool											m_transferShieldedTargets;
	Bool											m_transferShieldingTargets;
	Bool											m_transferObjectName;

	Bool											m_dontTransferObjectNameAfterInheritingVeterancy;
	Bool											m_addToAssaultTransport;

	Bool											m_experienceSink;

	SetProducerType							m_setProducerType;

	UnsignedInt								m_invulnerableTime;
	UnsignedInt								m_fadeFrames;

	AsciiString								m_putInContainer;
	AsciiString								m_particleSysName;
	AsciiString								m_fadeSoundName;

	AudioEventRTS							m_bounceSound;
	AudioEventRTS							m_waterImpactSound;
	const FXList*							m_waterImpactFX;

	Real									m_mass;
	Real											m_minHealth;
	Real											m_maxHealth;

	Real											m_extraBounciness;
	Real											m_extraFriction;
	Coord3D											m_offset;
	DispositionType									m_disposition;
	Real											m_dispositionIntensity;
	Real											m_spinRate;
	Real											m_yawRate;
	Real											m_rollRate;
	Real											m_pitchRate;
	Real											m_minMag, m_maxMag;
	Real											m_minPitch, m_maxPitch;
	Bool											m_orientInForceDirection;
	Bool											m_containInsideSourceObject; ///< The created stuff will be added to the Conatin module of the SourceObject
	Bool											m_fadeIn;
	Bool											m_fadeOut;
	Bool											m_ignorePrimaryObstacle;
	Bool											m_requiresLivePlayer;
	Bool											m_preserveLayer;
	Bool											m_skipIfSignificantlyAirborne;
	Bool											m_diesOnBadLand;

	Bool											m_definedBounceSound;
	Bool											m_definedWaterImpactSound;

	ObjectCreationMuxData();
	static void parseBounceSound( INI* ini, void * instance, void *store, const void* /*userData*/ );
	static void parseWaterImpactSound( INI* ini, void * instance, void *store, const void* /*userData*/ );
	static const FieldParse* getFieldParse();

};

//-------------------------------------------------------------------------------------------------
class ObjectCreationMux
{
public:
	virtual Bool checkIfDontHaveLivePlayer( const Object* sourceObj ) const;
	virtual Bool checkIfSkipWhileSignificantlyAirborne( const Object* sourceObj ) const;
	virtual const ObjectCreationMuxData *getCreationMuxData() const { return nullptr; }
	virtual Object* getPutInContainer(Bool &requiresContainer, Team *owner) const;
	virtual void setProducer( const Object *sourceObj, Object *obj, Object *container = nullptr ) const;
	virtual void doPreserveLayer( const Object *sourceObj, Object *obj ) const;
	virtual void doObjectCreation( const Object *sourceObj, Object *obj ) const;
	virtual void doInherit( const Object *sourceObj, Object *obj, ObjectStatusMaskType prevStatus ) const;
	virtual void doInheritHealth( const Object *sourceObj, Object *obj ) const;
	virtual void doInheritSelection( const Object *sourceObj, Object *obj, Bool bypassSourceCheck = FALSE, Int oldSquadNumber = -1) const;
	virtual void doTransfer( const Object *sourceObj, Object *obj, Bool isDestroyLater ) const;
	virtual void doDisposition( const Object *sourceObj, Object *obj, const Coord3D *pos, const Matrix3D *mtx, Real orientation, Bool nameAreObjects ) const;
	virtual void doPostDisposition( const Object *sourceObj, Object *obj ) const;
	virtual void doFadeStuff( const Object *sourceObj ) const;
};

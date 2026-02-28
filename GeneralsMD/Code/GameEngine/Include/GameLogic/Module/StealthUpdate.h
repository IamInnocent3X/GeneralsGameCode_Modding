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

// FILE: StealthUpdate.h //////////////////////////////////////////////////////////////////////////
// Author: Kris Morness, May 2002
// Desc:	 An update that checks for a status bit to stealth the owning object
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "GameLogic/Module/UpdateModule.h"

// FORWARD REFERENCES /////////////////////////////////////////////////////////////////////////////
class Thing;
enum StealthLookType CPP_11(: Int);
enum EvaMessage CPP_11(: Int);
enum WeaponSetType CPP_11(: Int);
class FXList;

enum
{
	STEALTH_NOT_WHILE_ATTACKING					= 0x00000001,
	STEALTH_NOT_WHILE_MOVING						= 0x00000002,
	STEALTH_NOT_WHILE_USING_ABILITY			= 0x00000004,
	STEALTH_NOT_WHILE_FIRING_PRIMARY		= 0x00000008,
	STEALTH_NOT_WHILE_FIRING_SECONDARY	= 0x00000010,
	STEALTH_NOT_WHILE_FIRING_TERTIARY		= 0x00000020,
	STEALTH_ONLY_WITH_BLACK_MARKET			= 0x00000040,
	STEALTH_NOT_WHILE_TAKING_DAMAGE			= 0x00000080,
	STEALTH_NOT_WHILE_RIDERS_ATTACKING  = 0x00000100,
	STEALTH_NOT_WHILE_FIRING_FOUR		= 0x00000200,
	STEALTH_NOT_WHILE_FIRING_FIVE		= 0x00000400,
	STEALTH_NOT_WHILE_FIRING_SIX		= 0x00000800,
	STEALTH_NOT_WHILE_FIRING_SEVEN 		= 0x00001000,
	STEALTH_NOT_WHILE_FIRING_EIGHT		= 0x00002000,
	STEALTH_NOT_WHILE_FIRING_WEAPON			= (STEALTH_NOT_WHILE_FIRING_PRIMARY | STEALTH_NOT_WHILE_FIRING_SECONDARY | STEALTH_NOT_WHILE_FIRING_TERTIARY | STEALTH_NOT_WHILE_FIRING_FOUR | STEALTH_NOT_WHILE_FIRING_FIVE | STEALTH_NOT_WHILE_FIRING_SIX | STEALTH_NOT_WHILE_FIRING_SEVEN | STEALTH_NOT_WHILE_FIRING_EIGHT),
};

#ifdef DEFINE_STEALTHLEVEL_NAMES
static const char *const TheStealthLevelNames[] =
{
	"ATTACKING",
	"MOVING",
	"USING_ABILITY",
	"FIRING_PRIMARY",
	"FIRING_SECONDARY",
	"FIRING_TERTIARY",
	"NO_BLACK_MARKET",
	"TAKING_DAMAGE",
    "RIDERS_ATTACKING",
	"FIRING_WEAPON_FOUR",
	"FIRING_WEAPON_FIVE",
	"FIRING_WEAPON_SIX",
	"FIRING_WEAPON_SEVEN",
	"FIRING_WEAPON_EIGHT",
	nullptr
};
#endif
typedef std::pair<Int, Coord3D> BarrelCoordType;
typedef std::vector<BarrelCoordType> BarrelCoordVec;
struct FiringPosStruct
{
	Int weaponSlot;
	Int barrelCount;
	BarrelCoordVec posVec;
};

#define INVALID_OPACITY -1.0f

//-------------------------------------------------------------------------------------------------
class StealthUpdateModuleData : public UpdateModuleData
{
public:
	ObjectStatusMaskType m_hintDetectableStates;
	ObjectStatusMaskType m_requiredStatus;
	ObjectStatusMaskType m_forbiddenStatus;
	FXList				*m_disguiseRevealFX;
	FXList				*m_disguiseFX;
	Real					m_stealthSpeed;
	Real					m_friendlyOpacityMin;
	Real					m_friendlyOpacityMax;
	Real					m_revealDistanceFromTarget;
	UnsignedInt		m_disguiseTransitionFrames;
	UnsignedInt		m_disguiseRevealTransitionFrames;
	UnsignedInt		m_pulseFrames;
	UnsignedInt		m_stealthDelay;
	UnsignedInt		m_stealthLevel;
	UnsignedInt		m_blackMarketCheckFrames;
	UnsignedInt		m_disguiseFriendlyFlickerDelay;
	UnsignedInt		m_disguiseFlickerTransitionTime;
  EvaMessage    m_enemyDetectionEvaEvent;
  EvaMessage    m_ownDetectionEvaEvent;
  Bool					m_innateStealth;
	Bool					m_orderIdleEnemiesToAttackMeUponReveal;
	Bool					m_teamDisguised;
	Bool					m_useRiderStealth;
  Bool          m_grantedBySpecialPower;
    Bool					m_innateDisguise;
	Bool					m_autoDisguiseWhenAvailable;
	Bool					m_canStealthWhileDisguised;
	Bool					m_disguiseRetainAfterDetected;
	Bool					m_preservePendingCommandWhenDetected;
	Bool					m_dontFlashWhenFlickering;
	Bool					m_disguiseUseOriginalFiringOffset;
	Bool					m_isSimpleDisguise;
	Bool					m_dontDrawHealingWhenHealedByAllies;
  std::vector<AsciiString> m_requiredCustomStatus;
  std::vector<AsciiString> m_forbiddenCustomStatus;

  StealthUpdateModuleData();
	static void buildFieldParse(MultiIniFieldParse& p);

};

//-------------------------------------------------------------------------------------------------
class StealthUpdate : public UpdateModule
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( StealthUpdate, "StealthUpdate" )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( StealthUpdate, StealthUpdateModuleData );

public:

	StealthUpdate( Thing *thing, const ModuleData* moduleData );
	// virtual destructor prototype provided by memory pool declaration


  virtual StealthUpdate* getStealth() { return this; }


	virtual UpdateSleepTime update();

	//Still gets called, even if held -ML
	virtual DisabledMaskType getDisabledTypesToProcess() const { return MAKE_DISABLED_MASK( DISABLED_HELD ); }

	// ??? ugh
	Bool isDisguised() const { return m_disguiseAsTemplate != nullptr; }
	Bool hasLastDisguiseTemplate() const { return m_lastDisguiseAsTemplate != nullptr; }
	Bool isDisguiseTransitioning() const { return m_disguiseTransitionFrames > 0; }
	Int getDisguisedPlayerIndex() const { return m_disguiseAsPlayerIndex; }
	const AsciiString& getDisguisedModelName() const { return m_disguiseModelName; }
	const ThingTemplate *getDisguisedTemplate() { return m_disguiseAsTemplate; }
	void markAsDetected( UnsignedInt numFrames = 0 );
	void disguiseAsObject( const Object *target, const Drawable *drawTemplate = nullptr, Bool doLast = FALSE ); //wrapper function for ease.
	Real getFriendlyOpacity() const;
	UnsignedInt getStealthDelay() const { return getStealthUpdateModuleData()->m_stealthDelay; }
	UnsignedInt getStealthLevel() const { return getStealthUpdateModuleData()->m_stealthLevel; }
  EvaMessage getEnemyDetectionEvaEvent() const { return getStealthUpdateModuleData()->m_enemyDetectionEvaEvent; }
  EvaMessage getOwnDetectionEvaEvent() const { return getStealthUpdateModuleData()->m_ownDetectionEvaEvent; }
	Bool getOrderIdleEnemiesToAttackMeUponReveal() const { return getStealthUpdateModuleData()->m_orderIdleEnemiesToAttackMeUponReveal; }
	Bool isSimpleDisguise() const { return isDisguised() && getStealthUpdateModuleData()->m_isSimpleDisguise; }
	Bool dontDrawHealingWhenHealedByAllies(Team *team) const;
	Object* calcStealthOwner(); //Is it me that can stealth or is it my rider?
	Bool allowedToStealth( Object *stealthOwner ) const;
  void receiveGrant( Bool active = TRUE, UnsignedInt frames = 0 );

  Bool isGrantedBySpecialPower() { return getStealthUpdateModuleData()->m_grantedBySpecialPower; }
	Bool isTemporaryGrant() { return m_framesGranted > 0; }

	Bool canStealthWhileDisguised() const { return getStealthUpdateModuleData()->m_canStealthWhileDisguised; }

	void setStealthLevelOverride(UnsignedInt stealthLevel) { m_stealthLevelOverride = stealthLevel; }

	void refreshUpdate();

	Bool isDisguisedAndCheckIfNeedOffset() const;
	//Bool getFiringOffsetWhileDisguised(WeaponSlotType wslot, Int specificBarrelToUse, Coord3D *pos) const;
	Int getBarrelCountWhileDisguised(WeaponSlotType wslot) const;
	Int getBarrelCountDisguisedTemplate(WeaponSlotType wslot) const;

	Drawable *getDrawableTemplateWhileDisguised() const;

protected:

	StealthLookType calcStealthedStatusForPlayer(const Object* obj, const Player* player);
	Bool canDisguise() const { return getStealthUpdateModuleData()->m_teamDisguised; }
	Real getRevealDistanceFromTarget() const { return getStealthUpdateModuleData()->m_revealDistanceFromTarget; }
	void hintDetectableWhileUnstealthed() ;

	void changeVisualDisguise();
	void changeVisualDisguiseFlicker(Bool doFlick);
	void SetFiringOffsets(Bool setDisguised);

	UpdateSleepTime calcSleepTime() const;

	//const Object* calcStealthOwnerConst() const; //Is it me that can stealth or is it my rider?

private:
	UnsignedInt						m_stealthAllowedFrame;
	UnsignedInt						m_detectionExpiresFrame;
	mutable UnsignedInt		m_nextBlackMarketCheckFrame;
	Bool									m_enabled;

	Real                  m_pulsePhaseRate;
	Real                  m_pulsePhase;

	UnsignedInt						m_stealthLevelOverride;   //Override stealth conditions via upgrade

	//Disguise only members
	Int										m_disguiseAsPlayerIndex;		//The player team we are wanting to disguise as (might not actually be disguised yet).
	const ThingTemplate  *m_disguiseAsTemplate;				//The disguise template (might not actually be using it yet)
	UnsignedInt						m_disguiseTransitionFrames;	//How many frames are left before transition is complete.
	Bool									m_disguiseHalfpointReached;	//In the middle of the transition, we will switch drawables!
	Bool									m_transitioningToDisguise;	//Set when we are disguising -- clear when we're transitioning out of.
	Bool									m_disguised;								//We're disguised as far as other players are concerned.
	UnsignedInt						m_framesGranted;						//0 means forever... everything else is number of frames before stealth lost.

	// runtime xfer members (does not need saving)
	Bool									m_xferRestoreDisguise;			//Tells us we need to restore our disguise
	WeaponSetType					m_requiresWeaponSetType;

	const ThingTemplate  *m_lastDisguiseAsTemplate;				//The disguise template (might not actually be using it yet)
	Int					  m_lastDisguiseAsPlayerIndex;		//The player team we are wanting to disguise as (might not actually be disguised yet).

	Bool							m_flicked;							//Have I been flicked?
	Bool							m_flicking;							//I am flicking, don't remove my statuses
	UnsignedInt						m_flickerFrame;						//frames to flicker for disguised Objects

	Bool							m_disguiseTransitionIsFlicking;		//Current transition is flicking

	mutable UnsignedInt				m_nextWakeUpFrame;					//Next Wake Up Frame, only use for calcSleepTime, dont xfer

	Bool							m_preserveLastGUI;					//Select objects silently to not overwrite the Control Bar and GUI Commands

	Bool							m_markForClearStealthLater;			//Fix Disguises auto Disguise back when Detected
	Bool							m_isNotAutoDisguise;				//Fix Disguises auto Disguise back when Detected

	AsciiString						m_disguiseModelName;				//Disguise Model for overwriting the current template

	Bool							m_updatePulse;						//Saves update for going extra checks by indicating that it only checks for pulse phase
	mutable Bool					m_updatePulseOnly;					//See above

	//std::vector<FiringPosStruct>	m_originalDrawableFiringOffsets;	//For storing firing positions for disguised Objects
	//std::vector<FiringPosStruct>	m_disguisedDrawableFiringOffsets;	//See above
	Drawable						*m_disguisedDrawableTemplate;		//See above
	Drawable						*m_originalDrawableTemplate;		//See above
};

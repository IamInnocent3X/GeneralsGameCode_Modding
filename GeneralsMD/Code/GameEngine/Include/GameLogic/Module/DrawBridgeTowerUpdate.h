// FILE: DrawBridgeTowerUpdate.h //////////////////////////////////////////////////////////////////////////
// Desc:   Update module to handle draw bridge toggling
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "Common/KindOf.h"
#include "GameLogic/Module/SpecialPowerUpdateModule.h"
#include "GameLogic/Module/DrawBridgeUpdate.h"
#include "UpdateModule.h"

// FORWARD REFERENCES /////////////////////////////////////////////////////////////////////////////
class SpecialPowerModule;
class ParticleSystem;
class FXList;
class AudioEventRTS;
enum  CommandOption CPP_11(: Int);

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
class DrawBridgeTowerUpdateModuleData : public ModuleData
{
public:
	SpecialPowerTemplate *m_specialPowerTemplate;
	AsciiString 			m_cursorName;

	DrawBridgeTowerUpdateModuleData();
	static void buildFieldParse(MultiIniFieldParse& p);

private:

};

enum BridgeTransitionStatus CPP_11(: Int)
{
	  TRANSITIONSTATUS_CLOSED,
		TRANSITIONSTATUS_OPENING,
		TRANSITIONSTATUS_OPENED,
		TRANSITIONSTATUS_CLOSING,

		TRANSITIONSTATUS_COUNT
};

enum BridgeState CPP_11(: Int)
{
	  BRIDGESTATE_CLOSE,
		BRIDGESTATE_OPEN,
							
		BRIDGESTATE_COUNT
};

struct DrawBridgeTowerInfo {
	BridgeState currentState;
	BridgeState desiredState;
	BridgeTransitionStatus transitionStatus;

	UnsignedInt nextReadyFrame;
};

//-------------------------------------------------------------------------------------------------
/** The default	update module */
//-------------------------------------------------------------------------------------------------
class DrawBridgeTowerUpdate : public SpecialPowerUpdateModule
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( DrawBridgeTowerUpdate, "DrawBridgeTowerUpdate" )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( DrawBridgeTowerUpdate, DrawBridgeTowerUpdateModuleData );

public:

	DrawBridgeTowerUpdate( Thing *thing, const ModuleData* moduleData );
	// virtual destructor prototype provided by memory pool declaration

	// SpecialPowerUpdateInterface
	virtual Bool initiateIntentToDoSpecialPower(const SpecialPowerTemplate *specialPowerTemplate, const Object *targetObj, const Drawable *targetDraw, const Coord3D *targetPos, const Waypoint *way, UnsignedInt commandOptions ) override;
	virtual Bool isSpecialAbility() const override { return false; }
	virtual Bool isSpecialPower() const override { return true; }
	virtual Bool isActive() const override {return false;}
	virtual SpecialPowerUpdateInterface* getSpecialPowerUpdateInterface() override { return this; }
	virtual Bool doesSpecialPowerHaveOverridableDestinationActive() const override { return false; } //Is it active now?
	virtual Bool doesSpecialPowerHaveOverridableDestination() const override { return false; }	//Does it have it, even if it's not active?
	virtual void setSpecialPowerOverridableDestination( const Coord3D *loc ) override {}
	virtual Bool isPowerCurrentlyInUse( const CommandButton *command = nullptr ) override const;
	virtual const AsciiString& getCursorName() const override { return getDrawBridgeTowerUpdateModuleData()->m_cursorName; }
	virtual const AsciiString& getInvalidCursorName() const override { return AsciiString::TheEmptyString; }
	virtual void setDelay(UnsignedInt delayFrame) override { }


	virtual void onObjectCreated() override;
	virtual void onDelete() override;
	virtual UpdateSleepTime update() override;

	virtual CommandOption getCommandOption() const override;

protected:
	DrawBridgeUpdate* getDrawBridgeUpdate() const;
	Object* getBridge() const;

	SpecialPowerModuleInterface *m_specialPowerModule;
};

// FILE: DrawBridgeUpdate.h //////////////////////////////////////////////////////////////////////////
// Desc:   Update module to handle draw bridge toggling
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "Common/KindOf.h"
#include "Common/AudioEventRTS.h"
#include "GameLogic/Module/UpdateModule.h"

// FORWARD REFERENCES /////////////////////////////////////////////////////////////////////////////
class SpecialPowerModule;
class ParticleSystem;
class FXList;
enum  CommandOption CPP_11(: Int);

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
class DrawBridgeUpdateModuleData : public ModuleData
{
public:
	//SpecialPowerTemplate *m_specialPowerTemplate;
	UnsignedInt m_openingDuration;
	UnsignedInt m_closingDuration;
	Real m_openingPushForce;
	UnsignedInt m_closingDamageTime;

	const FXList* m_openingFX;			///< played when the bridge starts opening
	const FXList* m_openFX;				///< played when the bridge finishes opening
	const FXList* m_closingFX;			///< played when the bridge starts closing
	const FXList* m_closedFX;			///< played when the bridge finishes closing
	AudioEventRTS m_openingAudio;		///< looped while the bridge is opening
	AudioEventRTS m_closingAudio;		///< looped while the bridge is closing

	DrawBridgeUpdateModuleData();
	static void buildFieldParse(MultiIniFieldParse& p);

private:

};

//-------------------------------------------------------------------------------------------------
/** The default	update module */
//-------------------------------------------------------------------------------------------------
class DrawBridgeUpdate : public UpdateModule
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( DrawBridgeUpdate, "DrawBridgeUpdate" )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( DrawBridgeUpdate, DrawBridgeUpdateModuleData );

public:

	DrawBridgeUpdate( Thing *thing, const ModuleData* moduleData );
	// virtual destructor prototype provided by memory pool declaration

	virtual void onObjectCreated() override;
	virtual void onDelete() override;
	virtual UpdateSleepTime update() override;

	virtual CommandOption getCommandOption() const override;

	Bool setDrawBridgeState(bool opened, const Object* fromTower);

	void onBridgeDestroyed();
	void onBridgeRepaired();

protected:
	void pushObjectsOnOpeningDrawbridge();
	void destroyObjectsUnderClosingDrawbridge();

	void stopTransitionAudio();      ///< stop any looping opening/closing audio

	enum BridgeTransitionType
	{
		BRIDGE_TRANSITION_NONE = 0,
		BRIDGE_TRANSITION_OPENING,
		BRIDGE_TRANSITION_CLOSING,
	};

	bool m_bridgeOpened;

	UnsignedInt m_nextReadyFrame;

	UnsignedInt m_openingFrame;            ///< frame bridge started to open
	UnsignedInt m_closingDamageFrame;      ///< frame damage will be applied when closing

	BridgeTransitionType m_transitionState; ///< opening/closing/none, drives the finish FX and looping audio
	UnsignedInt m_transitionDoneFrame;      ///< frame the current opening/closing finishes

	AudioEventRTS m_openingAudio;          ///< runtime instance of the looping opening audio
	AudioEventRTS m_closingAudio;          ///< runtime instance of the looping closing audio
};

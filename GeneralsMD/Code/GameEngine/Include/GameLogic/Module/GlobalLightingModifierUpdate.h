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

// FILE: GlobalLightingModifierUpdate.h //////////////////////////////////////////////////////////
// Desc: While active, this module contributes a darken/colorize/brighten shift to the GLOBAL map
//       lighting (Phase 1: object lighting). Multiple instances aggregate. Fades in/out on
//       activation. Presentation-only: it registers with the client-side lighting manager and
//       never feeds back into the deterministic sim.
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "GameLogic/Module/UpdateModule.h"
#include "GameClient/GlobalLightingModifier.h"

//-------------------------------------------------------------------------------------------------
class GlobalLightingModifierUpdateModuleData : public UpdateModuleData
{
public:
	enum LightingBlendMode CPP_11(: Int)
	{
		LIGHTINGMOD_MULTIPLY = 0,	///< multiply lighting toward TargetColor (darken / tint)
		LIGHTINGMOD_ADDITIVE		///< add TargetColor to lighting (brighten / additive tint)
	};

	RGBColor		m_targetColor;		///< the color to multiply/add toward
	Int				m_blendMode;		///< LightingBlendMode
	Real			m_intensity;		///< 0..1 (or higher for ADDITIVE) strength at full fade
	UnsignedInt		m_initialDelayFrames;	///< after the gate is met, wait this long before activating (0 = none)
	UnsignedInt		m_durationFrames;	///< once activated, stay active this long then fade out (0 = infinite)
	UnsignedInt		m_fadeInFrames;		///< frames to ramp in when activated
	UnsignedInt		m_fadeOutFrames;	///< frames to ramp out when deactivated
	AsciiString		m_requiredUpgrade;	///< if set, only active while this upgrade is owned (else always active while alive)

	GlobalLightingModifierUpdateModuleData();
	static void buildFieldParse(MultiIniFieldParse& p);
};

//-------------------------------------------------------------------------------------------------
class GlobalLightingModifierUpdate : public UpdateModule, public LightingModifierContributor
{
	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( GlobalLightingModifierUpdate, "GlobalLightingModifierUpdate" )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( GlobalLightingModifierUpdate, GlobalLightingModifierUpdateModuleData )

public:

	GlobalLightingModifierUpdate( Thing *thing, const ModuleData* moduleData );
	// virtual destructor prototype provided by memory pool declaration

	virtual UpdateSleepTime update() override;

	// LightingModifierContributor
	virtual void getLightingContribution( RGBColor& outMul, RGBColor& outAdd ) const override;

protected:

	Bool computeConditionMet( void ) const;	///< is the activation gate satisfied right now (alive + upgrade)?

	Real m_weight;			///< current fade weight 0..1
	Bool m_triggered;		///< has the gate been met and the timed cycle started?
	UnsignedInt m_triggerFrame;	///< frame the gate was first met (start of InitialDelay)
};

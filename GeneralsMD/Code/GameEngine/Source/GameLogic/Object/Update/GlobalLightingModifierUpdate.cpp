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

// FILE: GlobalLightingModifierUpdate.cpp ////////////////////////////////////////////////////////

#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "Common/Player.h"
#include "Common/Upgrade.h"
#include "Common/Xfer.h"
#include "GameLogic/Object.h"
#include "GameLogic/Module/GlobalLightingModifierUpdate.h"

//-------------------------------------------------------------------------------------------------
static const char* const TheLightingBlendModeNames[] =
{
	"DARKEN",
	"COLORIZE",
	"BRIGHTEN",
	nullptr
};

//-------------------------------------------------------------------------------------------------
GlobalLightingModifierUpdateModuleData::GlobalLightingModifierUpdateModuleData()
{
	m_targetColor.red = m_targetColor.green = m_targetColor.blue = 0.0f;
	m_blendMode = LIGHTINGMOD_DARKEN;
	m_intensity = 1.0f;
	m_fadeInFrames = 1;
	m_fadeOutFrames = 1;
	// m_requiredUpgrade defaults empty -> always active while alive
}

//-------------------------------------------------------------------------------------------------
/*static*/ void GlobalLightingModifierUpdateModuleData::buildFieldParse(MultiIniFieldParse& p)
{
	UpdateModuleData::buildFieldParse( p );

	static const FieldParse dataFieldParse[] =
	{
		{ "TargetColor",		INI::parseRGBColor,				nullptr,						offsetof( GlobalLightingModifierUpdateModuleData, m_targetColor ) },
		{ "BlendMode",			INI::parseIndexList,			TheLightingBlendModeNames,		offsetof( GlobalLightingModifierUpdateModuleData, m_blendMode ) },
		{ "Intensity",			INI::parseReal,					nullptr,						offsetof( GlobalLightingModifierUpdateModuleData, m_intensity ) },
		{ "FadeInTime",			INI::parseDurationUnsignedInt,	nullptr,						offsetof( GlobalLightingModifierUpdateModuleData, m_fadeInFrames ) },
		{ "FadeOutTime",		INI::parseDurationUnsignedInt,	nullptr,						offsetof( GlobalLightingModifierUpdateModuleData, m_fadeOutFrames ) },
		{ "RequiredUpgrade",	INI::parseAsciiString,			nullptr,						offsetof( GlobalLightingModifierUpdateModuleData, m_requiredUpgrade ) },
		{ nullptr, nullptr, nullptr, 0 }
	};
	p.add(dataFieldParse);
}

//-------------------------------------------------------------------------------------------------
GlobalLightingModifierUpdate::GlobalLightingModifierUpdate( Thing *thing, const ModuleData* moduleData ) :
	UpdateModule( thing, moduleData ),
	m_weight( 0.0f )	// start faded out; ramps in
{
	GlobalLightingModifierManager::get().registerContributor( this );
}

//-------------------------------------------------------------------------------------------------
GlobalLightingModifierUpdate::~GlobalLightingModifierUpdate( void )
{
	GlobalLightingModifierManager::get().unregisterContributor( this );
}

//-------------------------------------------------------------------------------------------------
Bool GlobalLightingModifierUpdate::computeActive( void ) const
{
	const Object* obj = getObject();
	if( obj == nullptr || obj->isEffectivelyDead() )
		return FALSE;

	const GlobalLightingModifierUpdateModuleData* d = getGlobalLightingModifierUpdateModuleData();
	if( d->m_requiredUpgrade.isNotEmpty() )
	{
		const UpgradeTemplate* ut = TheUpgradeCenter->findUpgrade( d->m_requiredUpgrade );
		if( ut == nullptr )
			return FALSE;
		if( obj->hasUpgrade( ut ) )
			return TRUE;
		const Player* p = obj->getControllingPlayer();
		if( p != nullptr && p->getCompletedUpgradeMask().testForAll( ut->getUpgradeMask() ) )
			return TRUE;
		return FALSE;
	}

	return TRUE;
}

//-------------------------------------------------------------------------------------------------
UpdateSleepTime GlobalLightingModifierUpdate::update( void )
{
	const GlobalLightingModifierUpdateModuleData* d = getGlobalLightingModifierUpdateModuleData();

	Real target = computeActive() ? 1.0f : 0.0f;

	if( m_weight < target )
	{
		Real step = ( d->m_fadeInFrames > 0 ) ? ( 1.0f / (Real)d->m_fadeInFrames ) : 1.0f;
		m_weight += step;
		if( m_weight > target )
			m_weight = target;
	}
	else if( m_weight > target )
	{
		Real step = ( d->m_fadeOutFrames > 0 ) ? ( 1.0f / (Real)d->m_fadeOutFrames ) : 1.0f;
		m_weight -= step;
		if( m_weight < target )
			m_weight = target;
	}

	return UPDATE_SLEEP_NONE;
}

//-------------------------------------------------------------------------------------------------
void GlobalLightingModifierUpdate::getLightingContribution( RGBColor& outMul, RGBColor& outAdd ) const
{
	const GlobalLightingModifierUpdateModuleData* d = getGlobalLightingModifierUpdateModuleData();

	Real s = d->m_intensity * m_weight;
	if( s < 0.0f )
		s = 0.0f;

	outMul.red = outMul.green = outMul.blue = 1.0f;
	outAdd.red = outAdd.green = outAdd.blue = 0.0f;

	const RGBColor& c = d->m_targetColor;
	switch( d->m_blendMode )
	{
		case GlobalLightingModifierUpdateModuleData::LIGHTINGMOD_BRIGHTEN:
			// additive: add the target color scaled by strength
			outAdd.red   = c.red   * s;
			outAdd.green = c.green * s;
			outAdd.blue  = c.blue  * s;
			break;

		case GlobalLightingModifierUpdateModuleData::LIGHTINGMOD_DARKEN:
		case GlobalLightingModifierUpdateModuleData::LIGHTINGMOD_COLORIZE:
		default:
			// multiplicative: lerp each channel from 1 (no change) toward the target color
			outMul.red   = 1.0f + ( c.red   - 1.0f ) * s;
			outMul.green = 1.0f + ( c.green - 1.0f ) * s;
			outMul.blue  = 1.0f + ( c.blue  - 1.0f ) * s;
			break;
	}
}

//-------------------------------------------------------------------------------------------------
void GlobalLightingModifierUpdate::crc( Xfer *xfer )
{
	UpdateModule::crc( xfer );
}

//-------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
//-------------------------------------------------------------------------------------------------
void GlobalLightingModifierUpdate::xfer( Xfer *xfer )
{
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	UpdateModule::xfer( xfer );

	xfer->xferReal( &m_weight );
}

//-------------------------------------------------------------------------------------------------
void GlobalLightingModifierUpdate::loadPostProcess( void )
{
	UpdateModule::loadPostProcess();
}

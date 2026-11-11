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

// FILE: Water.cpp ////////////////////////////////////////////////////////////////////////////////
// Author: Colin Day, December 2001
// Desc:   Map water settings
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "GameClient/Water.h"
#include "Common/INI.h"

// GLOBALS ////////////////////////////////////////////////////////////////////////////////////////
OVERRIDE<WaterSetting> TheWaterSettings = nullptr;
OVERRIDE<WaterTransparencySetting> TheWaterTransparency = nullptr;

// PRIVATE DATA ///////////////////////////////////////////////////////////////////////////////////
const FieldParse WaterSettingData::m_waterSettingFieldParseTable[] =
{

	{ "SkyTexture",									INI::parseAsciiString,			nullptr, offsetof( WaterSettingData, m_skyTextureFile ) },
	{ "WaterTexture",								INI::parseAsciiString,			nullptr, offsetof( WaterSettingData, m_waterTextureFile ) },
  { "Vertex00Color",							INI::parseRGBAColorInt,			nullptr, offsetof( WaterSettingData, m_vertex00Diffuse ) },
	{ "Vertex10Color",							INI::parseRGBAColorInt,			nullptr, offsetof( WaterSettingData, m_vertex10Diffuse ) },
	{ "Vertex01Color",							INI::parseRGBAColorInt,			nullptr, offsetof( WaterSettingData, m_vertex01Diffuse ) },
  { "Vertex11Color",							INI::parseRGBAColorInt,			nullptr, offsetof( WaterSettingData, m_vertex11Diffuse ) },
	{ "DiffuseColor",								INI::parseRGBAColorInt,			nullptr, offsetof( WaterSettingData, m_waterDiffuseColor ) },
  { "TransparentDiffuseColor",		INI::parseRGBAColorInt,			nullptr, offsetof( WaterSettingData, m_transparentWaterDiffuse ) },
  { "UScrollPerMS",								INI::parseReal,							nullptr, offsetof( WaterSettingData, m_uScrollPerMs ) },
  { "VScrollPerMS",								INI::parseReal,							nullptr, offsetof( WaterSettingData, m_vScrollPerMs ) },
	{ "SkyTexelsPerUnit",						INI::parseReal,							nullptr, offsetof( WaterSettingData, m_skyTexelsPerUnit ) },
  { "WaterRepeatCount",						INI::parseInt,							nullptr, offsetof( WaterSettingData, m_waterRepeatCount ) },

	{ nullptr,													nullptr,												nullptr, 0 },

};

// PRIVATE DATA ///////////////////////////////////////////////////////////////////////////////////
const FieldParse WaterTransparencyData::m_waterTransparencySettingFieldParseTable[] =
{

	{ "TransparentWaterDepth",			INI::parseReal,				nullptr,			offsetof( WaterTransparencyData, m_transparentWaterDepth ) },
	{ "TransparentWaterMinOpacity",	INI::parseReal,				nullptr,			offsetof( WaterTransparencyData, m_minWaterOpacity ) },
	{ "StandingWaterColor",	INI::parseRGBColor,			nullptr,			offsetof( WaterTransparencyData, m_standingWaterColor ) },
	{ "StandingWaterTexture",INI::parseAsciiString,		nullptr,			offsetof( WaterTransparencyData, m_standingWaterTexture ) },
	{ "AdditiveBlending", INI::parseBool,				nullptr,			offsetof( WaterTransparencyData, m_additiveBlend) },
	{ "RadarWaterColor", INI::parseRGBColor,			nullptr,			offsetof( WaterTransparencyData, m_radarColor) },
	{ "SkyboxTextureN",							INI::parseAsciiString,nullptr,			offsetof( WaterTransparencyData, m_skyboxTextureN ) },
	{ "SkyboxTextureE",							INI::parseAsciiString,nullptr,			offsetof( WaterTransparencyData, m_skyboxTextureE ) },
	{ "SkyboxTextureS",							INI::parseAsciiString,nullptr,			offsetof( WaterTransparencyData, m_skyboxTextureS ) },
	{ "SkyboxTextureW",							INI::parseAsciiString,nullptr,			offsetof( WaterTransparencyData, m_skyboxTextureW ) },
	{ "SkyboxTextureT",							INI::parseAsciiString,nullptr,			offsetof( WaterTransparencyData, m_skyboxTextureT ) },


	{ nullptr, nullptr, nullptr, 0 },
};


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
WaterSetting::WaterSetting()
{

	for(int i = 0; i < TIME_OF_DAY_COUNT; i++)
	{
		m_waterSettingData[i].m_skyTextureFile.clear();
		m_waterSettingData[i].m_waterTextureFile.clear();
		m_waterSettingData[i].m_waterRepeatCount = 0;
		m_waterSettingData[i].m_skyTexelsPerUnit = 0.0f;

		m_waterSettingData[i].m_vertex00Diffuse.red = 0;
		m_waterSettingData[i].m_vertex00Diffuse.green = 0;
		m_waterSettingData[i].m_vertex00Diffuse.blue = 0;
		m_waterSettingData[i].m_vertex00Diffuse.alpha = 0;

		m_waterSettingData[i].m_vertex01Diffuse.red = 0;
		m_waterSettingData[i].m_vertex01Diffuse.green = 0;
		m_waterSettingData[i].m_vertex01Diffuse.blue = 0;
		m_waterSettingData[i].m_vertex01Diffuse.alpha = 0;

		m_waterSettingData[i].m_vertex10Diffuse.red = 0;
		m_waterSettingData[i].m_vertex10Diffuse.green = 0;
		m_waterSettingData[i].m_vertex10Diffuse.blue = 0;
		m_waterSettingData[i].m_vertex10Diffuse.alpha = 0;

		m_waterSettingData[i].m_vertex11Diffuse.red = 0;
		m_waterSettingData[i].m_vertex11Diffuse.green = 0;
		m_waterSettingData[i].m_vertex11Diffuse.blue = 0;
		m_waterSettingData[i].m_vertex11Diffuse.alpha = 0;

		m_waterSettingData[i].m_waterDiffuseColor.red = 0;
		m_waterSettingData[i].m_waterDiffuseColor.green = 0;
		m_waterSettingData[i].m_waterDiffuseColor.blue = 0;
		m_waterSettingData[i].m_waterDiffuseColor.alpha = 0;

		m_waterSettingData[i].m_transparentWaterDiffuse.red = 0;
		m_waterSettingData[i].m_transparentWaterDiffuse.green = 0;
		m_waterSettingData[i].m_transparentWaterDiffuse.blue = 0;
		m_waterSettingData[i].m_transparentWaterDiffuse.alpha = 0;

		m_waterSettingData[i].m_uScrollPerMs = 0.0f;
		m_waterSettingData[i].m_vScrollPerMs = 0.0f;
	}

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//WaterSetting::~WaterSetting()
//{
//
//}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
const WaterSettingData *WaterSetting::getWaterSettingData(Int timeofday) const
{
	return &m_waterSettingData[timeofday];
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
const WaterTransparencyData *WaterTransparencySetting::getWaterTransparencyData(Int timeofday) const
{
	return &m_waterTransparencyData[timeofday == -1 ? TheGlobalData->m_timeOfDay : timeofday];
}

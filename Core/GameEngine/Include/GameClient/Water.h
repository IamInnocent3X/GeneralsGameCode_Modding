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

// FILE: Water.h //////////////////////////////////////////////////////////////////////////////////
// Author: Colin Day, December 2001
// Desc:   Water settings
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

// INLCLUDES //////////////////////////////////////////////////////////////////////////////////////
#include "Common/GameType.h"
#include "Common/Overridable.h"
#include "Common/Override.h"

//-------------------------------------------------------------------------------------------------
struct FieldParse;
class WaterSettingData
{
public:
	AsciiString m_skyTextureFile;
	AsciiString m_waterTextureFile;
	Int m_waterRepeatCount;
	Real m_skyTexelsPerUnit;	//texel density of sky plane (higher value repeats texture more).
	RGBAColorInt m_vertex00Diffuse;
	RGBAColorInt m_vertex10Diffuse;
	RGBAColorInt m_vertex11Diffuse;
	RGBAColorInt m_vertex01Diffuse;
	RGBAColorInt m_waterDiffuseColor;
	RGBAColorInt m_transparentWaterDiffuse;
	Real m_uScrollPerMs;
	Real m_vScrollPerMs;

public:
	/// Get the INI parsing table for loading
	const FieldParse *getFieldParse() { return m_waterSettingFieldParseTable; }

	static const FieldParse m_waterSettingFieldParseTable[];		///< the parse table for INI definition

};

class WaterTransparencyData
{
public:
	Real m_transparentWaterDepth;
	Real m_minWaterOpacity;
	RGBColor m_standingWaterColor;
	RGBColor m_radarColor;
	Bool m_additiveBlend;
	AsciiString m_standingWaterTexture;

	AsciiString m_skyboxTextureN;
	AsciiString m_skyboxTextureE;
	AsciiString m_skyboxTextureS;
	AsciiString m_skyboxTextureW;
	AsciiString m_skyboxTextureT;

public:
	static const FieldParse m_waterTransparencySettingFieldParseTable[];		///< the parse table for INI definition

	/// Get the INI parsing table for loading
	const FieldParse *getFieldParse() const { return m_waterTransparencySettingFieldParseTable; }
};

//-------------------------------------------------------------------------------------------------
/** This structures keeps the settings for how our water will look */
//-------------------------------------------------------------------------------------------------
class WaterSetting : public Overridable
{
	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( WaterSetting, "WaterSetting"  )

private:
	WaterSettingData m_waterSettingData[ TIME_OF_DAY_COUNT ];

public:

	WaterSetting();
	//virtual ~WaterSetting();

	static void parseWaterSettingDefinition( INI* ini );

	const WaterSettingData *getWaterSettingData(Int timeofday) const;

};

EMPTY_DTOR(WaterSetting)

//-------------------------------------------------------------------------------------------------
/** This structure keeps the transparency and vertex settings, which are the same regardless of the
		time of day. They can be overridden on a per-map basis. */
//-------------------------------------------------------------------------------------------------
class WaterTransparencySetting : public Overridable
{
	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( WaterTransparencySetting, "WaterTransparencySetting"  )

	private:
		WaterTransparencyData m_waterTransparencyData[ TIME_OF_DAY_COUNT ];

	public:
		WaterTransparencySetting()
		{
			for(int i = 0; i < TIME_OF_DAY_COUNT; i++)
			{
				m_waterTransparencyData[i].m_transparentWaterDepth = 3.0f;
				m_waterTransparencyData[i].m_minWaterOpacity = 1.0f;
				m_waterTransparencyData[i].m_standingWaterColor.red = 1.0f;
				m_waterTransparencyData[i].m_standingWaterColor.green = 1.0f;
				m_waterTransparencyData[i].m_standingWaterColor.blue = 1.0f;
				m_waterTransparencyData[i].m_radarColor.red = 0.55f;
				m_waterTransparencyData[i].m_radarColor.green = 0.55f;
				m_waterTransparencyData[i].m_radarColor.blue = 1.0f;
				m_waterTransparencyData[i].m_standingWaterTexture = "TWWater01.tga";
				m_waterTransparencyData[i].m_additiveBlend = FALSE;

				m_waterTransparencyData[i].m_skyboxTextureN = "TSMorningN.tga";
				m_waterTransparencyData[i].m_skyboxTextureE = "TSMorningE.tga";
				m_waterTransparencyData[i].m_skyboxTextureS = "TSMorningS.tga";
				m_waterTransparencyData[i].m_skyboxTextureW = "TSMorningW.tga";
				m_waterTransparencyData[i].m_skyboxTextureT = "TSMorningT.tga";
			}
		}

		static void parseWaterTransparencyDefinition( INI* ini );
	
		const WaterTransparencyData *getWaterTransparencyData(Int timeofday = -1) const;
};

EMPTY_DTOR(WaterTransparencySetting)

// EXTERNAL ///////////////////////////////////////////////////////////////////////////////////////
extern OVERRIDE<WaterSetting> TheWaterSettings;

extern OVERRIDE<WaterTransparencySetting> TheWaterTransparency;

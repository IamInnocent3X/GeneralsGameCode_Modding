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

// FILE: INIWater.cpp /////////////////////////////////////////////////////////////////////////////
// Author: Colin Day, December 2001
// Desc:   Water settings
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#define DEFINE_TIME_OF_DAY_NAMES

#include "Common/INI.h"
#include "Common/GameType.h"

#include "GameClient/TerrainVisual.h"
#include "GameClient/Water.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS ///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------------------------------
/** Water setting, note that this does not support override situations.  As the water
	* system becomes more complex we may want to change this */
//-------------------------------------------------------------------------------------------------
void INI::parseWaterSettingDefinition( INI* ini )
{
	WaterSetting::parseWaterSettingDefinition(ini);
}

void WaterSetting::parseWaterSettingDefinition( INI* ini )
{
	AsciiString name;
	//WaterSetting *waterSetting = nullptr;

	// read the name
	const char* token = ini->getNextToken();

	name.set( token );

	// get the water setting we want to load based on name
	const char *const *timeOfDayName = TimeOfDayNames;
	Int timeOfDayIndex = 0;  // TIME_OF_DAY_INVALID
	while( timeOfDayName && *timeOfDayName )
	{

		if( stricmp( *timeOfDayName, name.str() ) == 0 )
		{

			//waterSetting = &WaterSettings[ timeOfDayIndex ];
			break;

		}

		// next name
		timeOfDayName++;
		timeOfDayIndex++;

	}

	// check for no time of day match
	//if( waterSetting == nullptr )
	if( timeOfDayIndex <= TIME_OF_DAY_INVALID || timeOfDayIndex >= TIME_OF_DAY_COUNT )
		throw INI_INVALID_DATA;

	// parse the data
	//ini->initFromINI( waterSetting, waterSetting->getFieldParse() );

	if (TheWaterSettings == nullptr) {
		TheWaterSettings = newInstance(WaterSetting);
	} else if (ini->getLoadType() == INI_LOAD_CREATE_OVERRIDES) {
		if(TheWaterSettings.getNonOverloadedPointer()->getNextOverride() == nullptr) {
			WaterSetting* ws = (WaterSetting*) (TheWaterSettings.getNonOverloadedPointer());
			WaterSetting* wsOverride = newInstance(WaterSetting);
			*wsOverride = *ws;

			// Mark that it is an override.
			wsOverride->markAsOverride();

			// Copy all respective data before overwriting
			for(int i = TIME_OF_DAY_FIRST; i < TIME_OF_DAY_COUNT; i++)
			{
				wsOverride->m_waterSettingData[i]=ws->m_waterSettingData[i];
			}

			ws->friend_getFinalOverride()->setNextOverride(wsOverride);
		}
	}

	WaterSetting* waterSet = (WaterSetting*) (TheWaterSettings.getNonOverloadedPointer());
	waterSet = (WaterSetting*) (waterSet->friend_getFinalOverride());

	// parse the data
	ini->initFromINI( &waterSet->m_waterSettingData[timeOfDayIndex], waterSet->m_waterSettingData[timeOfDayIndex].getFieldParse() );
}

//-------------------------------------------------------------------------------------------------
void INI::parseWaterTransparencyDefinition( INI *ini )
{
	WaterTransparencySetting::parseWaterTransparencyDefinition(ini);
}

//-------------------------------------------------------------------------------------------------
void WaterTransparencySetting::parseWaterTransparencyDefinition( INI *ini )
{
	AsciiString name;

	const char* token = ini->getNextTokenOrNull();
	Int timeOfDayIndex = 0;
	Bool firstDeclared = false;

	if(token != nullptr) {
		name.set( token );

		// get the water setting we want to load based on name
		const char *const *timeOfDayName = TimeOfDayNames;
		while( timeOfDayName && *timeOfDayName )
		{

			if( stricmp( *timeOfDayName, name.str() ) == 0 )
			{

				break;

			}

			// next name
			timeOfDayName++;
			timeOfDayIndex++;

		}
	}

	// check for no time of day match
	if( timeOfDayIndex >= TIME_OF_DAY_COUNT )
		throw INI_INVALID_DATA;

	if (TheWaterTransparency == nullptr) {
		TheWaterTransparency = newInstance(WaterTransparencySetting);
		if (ini->getLoadType() != INI_LOAD_CREATE_OVERRIDES) firstDeclared = true;
	} else if (ini->getLoadType() == INI_LOAD_CREATE_OVERRIDES) {
		if(TheWaterTransparency.getNonOverloadedPointer()->getNextOverride() == nullptr) {
			WaterTransparencySetting* wt = (WaterTransparencySetting*) (TheWaterTransparency.getNonOverloadedPointer());
			WaterTransparencySetting* wtOverride = newInstance(WaterTransparencySetting);
			*wtOverride = *wt;

			// Copy all respective data towards all day and night sources
			for(int i = TIME_OF_DAY_FIRST; i < TIME_OF_DAY_COUNT; i++)
			{
				wtOverride->m_waterTransparencyData[i]=wt->m_waterTransparencyData[i];
			}

			// Mark that it is an override.
			wtOverride->markAsOverride();

			wt->friend_getFinalOverride()->setNextOverride(wtOverride);
		}
	} else if(token == nullptr) {
		throw INI_INVALID_DATA;
	}

	WaterTransparencySetting* waterTrans = (WaterTransparencySetting*) (TheWaterTransparency.getNonOverloadedPointer());
	waterTrans = (WaterTransparencySetting*) (waterTrans->friend_getFinalOverride());
	// parse the data
	//ini->initFromINI( waterTrans, TheWaterTransparency->getFieldParse() );
	ini->initFromINI( &waterTrans->m_waterTransparencyData[timeOfDayIndex], waterTrans->m_waterTransparencyData[timeOfDayIndex].getFieldParse() );

	if(timeOfDayIndex == 0 || firstDeclared) {
		// Copy all respective data from the first source towards all day and night sources
		for(int i = TIME_OF_DAY_FIRST; i < TIME_OF_DAY_COUNT; i++)
		{
			waterTrans->m_waterTransparencyData[i]=waterTrans->m_waterTransparencyData[timeOfDayIndex];
		}
	}

	// If we overrode any skybox textures, then call the W3D Water stuff.
	if (ini->getLoadType() == INI_LOAD_CREATE_OVERRIDES) {
		// Check to see if we overrode any skybox textures.
		// If we did, then we need to replace them in the model.
		// Copy/Paste monkeys PLEASE TAKE NOTE. This technique only works for the skybox because we
		// know that there will never be more than one sky box. If you were to use this technique for
		// technicals, for instance, it would make all technicals in the level have the same new
		// texture.

		const WaterTransparencySetting* wtOriginal = TheWaterTransparency.getNonOverloadedPointer();
		OVERRIDE<WaterTransparencySetting> wtOverride = TheWaterTransparency;

		if (wtOriginal == wtOverride)
			return;

		const AsciiString *oldTextures[5],*newTextures[5];

		Int timeofday = TheGlobalData ? TheGlobalData->m_timeOfDay : 0;

		//Copy current texture names into arrays
		oldTextures[0]=&wtOriginal->m_waterTransparencyData[timeofday].m_skyboxTextureN;
		newTextures[0]=&wtOverride->m_waterTransparencyData[timeofday].m_skyboxTextureN;
		oldTextures[1]=&wtOriginal->m_waterTransparencyData[timeofday].m_skyboxTextureE;
		newTextures[1]=&wtOverride->m_waterTransparencyData[timeofday].m_skyboxTextureE;
		oldTextures[2]=&wtOriginal->m_waterTransparencyData[timeofday].m_skyboxTextureS;
		newTextures[2]=&wtOverride->m_waterTransparencyData[timeofday].m_skyboxTextureS;
		oldTextures[3]=&wtOriginal->m_waterTransparencyData[timeofday].m_skyboxTextureW;
		newTextures[3]=&wtOverride->m_waterTransparencyData[timeofday].m_skyboxTextureW;
		oldTextures[4]=&wtOriginal->m_waterTransparencyData[timeofday].m_skyboxTextureT;
		newTextures[4]=&wtOverride->m_waterTransparencyData[timeofday].m_skyboxTextureT;

		for(int i=0; i<5; i++)
		{
			if(oldTextures[i]->compareNoCase(*newTextures[i]) != 0)
			{
				TheTerrainVisual->replaceSkyboxTextures(oldTextures, newTextures);
				break;
			}
		}
	}
}



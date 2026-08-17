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

// FILE: GlobalLightingModifier.cpp //////////////////////////////////////////////////////////////

#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "GameClient/GlobalLightingModifier.h"

//-------------------------------------------------------------------------------------------------
/*static*/ GlobalLightingModifierManager& GlobalLightingModifierManager::get( void )
{
	static GlobalLightingModifierManager theInstance;
	return theInstance;
}

//-------------------------------------------------------------------------------------------------
void GlobalLightingModifierManager::registerContributor( const LightingModifierContributor* c )
{
	if( c == nullptr )
		return;
	// avoid duplicate registration
	for( std::vector<const LightingModifierContributor*>::const_iterator it = m_contributors.begin();
			it != m_contributors.end(); ++it )
	{
		if( *it == c )
			return;
	}
	m_contributors.push_back( c );
}

//-------------------------------------------------------------------------------------------------
void GlobalLightingModifierManager::unregisterContributor( const LightingModifierContributor* c )
{
	for( std::vector<const LightingModifierContributor*>::iterator it = m_contributors.begin();
			it != m_contributors.end(); ++it )
	{
		if( *it == c )
		{
			m_contributors.erase( it );
			return;
		}
	}
}

//-------------------------------------------------------------------------------------------------
void GlobalLightingModifierManager::computeCombined( RGBColor& outMul, RGBColor& outAdd ) const
{
	outMul.red = outMul.green = outMul.blue = 1.0f;	// identity multiply
	outAdd.red = outAdd.green = outAdd.blue = 0.0f;	// identity add

	for( std::vector<const LightingModifierContributor*>::const_iterator it = m_contributors.begin();
			it != m_contributors.end(); ++it )
	{
		RGBColor mul, add;
		mul.red = mul.green = mul.blue = 1.0f;
		add.red = add.green = add.blue = 0.0f;
		(*it)->getLightingContribution( mul, add );

		// multiplies compound (darker stacks darker), additives sum (brighten/colorize stack)
		outMul.red   *= mul.red;
		outMul.green *= mul.green;
		outMul.blue  *= mul.blue;
		outAdd.red   += add.red;
		outAdd.green += add.green;
		outAdd.blue  += add.blue;
	}
}

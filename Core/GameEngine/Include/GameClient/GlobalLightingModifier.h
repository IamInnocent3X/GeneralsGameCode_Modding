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

// FILE: GlobalLightingModifier.h ////////////////////////////////////////////////////////////////
// Client-side aggregator for "global lighting modifier" contributors (e.g. objects that darken /
// colorize / brighten the whole map's lighting while active). Contributors register themselves;
// the renderer asks for the combined multiply+add each frame and applies it to the scene lighting.
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Lib/BaseType.h"
#include "Common/STLTypedefs.h"

//-------------------------------------------------------------------------------------------------
/** Anything that contributes to the global lighting modifier implements this. The contribution is
	returned pre-scaled by the contributor's own fade weight / intensity: a multiply color (default
	white = no change) and an additive color (default black = no change). */
//-------------------------------------------------------------------------------------------------
class LightingModifierContributor
{
public:
	virtual ~LightingModifierContributor() {}
	virtual void getLightingContribution( RGBColor& outMul, RGBColor& outAdd ) const = 0;
};

//-------------------------------------------------------------------------------------------------
/** Global registry + combiner. Presentation-only; not part of the deterministic sim. */
//-------------------------------------------------------------------------------------------------
class GlobalLightingModifierManager
{
public:
	static GlobalLightingModifierManager& get( void );

	void registerContributor( const LightingModifierContributor* c );
	void unregisterContributor( const LightingModifierContributor* c );

	/** Combined multiply (component-wise product) and additive (sum) across all contributors.
		Returns identity (mul = 1,1,1  add = 0,0,0) when there are none. */
	void computeCombined( RGBColor& outMul, RGBColor& outAdd ) const;

	Bool hasActiveContributors( void ) const { return !m_contributors.empty(); }

private:
	GlobalLightingModifierManager() {}

	std::vector<const LightingModifierContributor*> m_contributors;
};

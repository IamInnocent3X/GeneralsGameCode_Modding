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

// WeaponBonusConditionFlags.h ////////////////////////////////////////////////////////////////////
// Part of header detangling
// JKMCD Aug 2002

#pragma once

typedef UnsignedInt WeaponBonusConditionFlags;
//typedef std::hash_map< AsciiString, Int, rts::hash<AsciiString>, rts::equal_to<AsciiString> > ObjectCustomStatusType;

//-------------------------------------------------------------------------------------------------
inline Bool checkWithinStringVec(const AsciiString& str, const std::vector<AsciiString>& vec)
{
	for(std::vector<AsciiString>::const_iterator it = vec.begin(); it != vec.end(); ++it)
	{
		if((*it) == str)
			return TRUE;
	}
	return FALSE;
}

//-------------------------------------------------------------------------------------------------
inline Bool checkWithinStringVecSize(const AsciiString& str, const std::vector<AsciiString>& vec, Int size)
{
	for(int i = 0; i < size; i++)
	{
		if(str == vec[i])
			return TRUE;
	}
	return FALSE;
}

//-------------------------------------------------------------------------------------------------
inline Bool removeWithinStringVec(const AsciiString& str, std::vector<AsciiString> &vec)
{
	std::vector<AsciiString>::iterator it = vec.begin();
	for(it; it != vec.end();)
	{
		if((*it) == str)
		{
			it = vec.erase( it );
			return TRUE;
		}
		++it;
	}
	return FALSE;
}

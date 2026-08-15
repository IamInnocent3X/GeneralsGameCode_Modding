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

// FILE: CaveSystem.cpp /////////////////////////////////////////////////////////////////////////////////
// Author: Graham Smallwood July 2002
// Desc:   System responsible for keeping track of all cave systems on the map
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "Common/GameState.h"
#include "Common/TunnelTracker.h"
#include "Common/Xfer.h"
#include "GameLogic/CaveSystem.h"

CaveSystem *TheCaveSystem = nullptr;

CaveSystem::CaveSystem()
{
}

CaveSystem::~CaveSystem()
{
}

void CaveSystem::init()
{
}

void CaveSystem::reset()
{
	for( std::vector<TunnelTracker*>::iterator iter = m_tunnelTrackerVector.begin(); iter != m_tunnelTrackerVector.end(); iter++ )
	{
		TunnelTracker *currentTracker = *iter; // could be null, since we don't slide back to fill deleted entries so offsets don't shift
		deleteInstance(currentTracker);
	}
	m_tunnelTrackerVector.clear();
	for( TeamTunnelTrackerMapType::iterator iter_2 = m_tunnelTrackerTeamMap.begin(); iter_2 != m_tunnelTrackerTeamMap.end(); ++iter_2 )
	{
		for( TunnelTrackerPtrVec::iterator iter_3 = (*iter_2).second.begin(); iter_3 != (*iter_2).second.end(); iter_3++ )
		{
			TunnelTracker *currentTracker_team = *iter_3;
			if( currentTracker_team )// could be null, since we don't slide back to fill deleted entries so offsets don't shift
			{
				deleteInstance(currentTracker_team);
			}
		}
	}
	m_tunnelTrackerTeamMap.clear();
}

void CaveSystem::update()
{
}

Bool CaveSystem::canSwitchIndexToIndex( Int oldIndex, Int newIndex )
{
	// When I grant permission, you need to do it.  ie call Unregister and then re-register with the new number
	TunnelTracker *oldTracker = nullptr;
	TunnelTracker *newTracker = nullptr;
	if( m_tunnelTrackerVector.size() > oldIndex )
	{
		oldTracker = m_tunnelTrackerVector[oldIndex];
		if( oldTracker && oldTracker->getContainCount() > 0 )
			return FALSE;// You can't switch a connection if one of the two is non empty
	}
	if( m_tunnelTrackerVector.size() > newIndex )
	{
		newTracker = m_tunnelTrackerVector[newIndex];
		if( newTracker && newTracker->getContainCount() > 0 )
			return FALSE;// You can't switch a connection if one of the two is non empty
	}

	// Both are either empty or non-existent, so go ahead.
	// (Remember non-exist is only a valid case because you are going to do the switch now.)

	return TRUE;
}

Bool CaveSystem::canSwitchIndexToIndexTeam( Int oldIndex, const Team *oldTeam, Int newIndex, const Team *newTeam )
{
	// When I grant permission, you need to do it.  ie call Unregister and then re-register with the new number
	TunnelTracker *oldTracker = nullptr;
	TunnelTracker *newTracker = nullptr;

	if(m_tunnelTrackerTeamMap.empty())
		return FALSE;

	TeamTunnelTrackerMapType::const_iterator it = m_tunnelTrackerTeamMap.find(oldTeam->getID());
	if (it != m_tunnelTrackerTeamMap.end())
	{
		if( (*it).second.size() > oldIndex )
		{
			oldTracker = (*it).second[oldIndex];
			if( oldTracker && oldTracker->getContainCount() > 0 )
				return FALSE;// You can't switch a connection if one of the two is non empty
		}
	}

	TeamTunnelTrackerMapType::const_iterator it_2 = m_tunnelTrackerTeamMap.find(newTeam->getID());
	if (it_2 != m_tunnelTrackerTeamMap.end())
	{
		if( (*it_2).second.size() > newIndex )
		{
			newTracker = (*it_2).second[newIndex];
			if( newTracker && newTracker->getContainCount() > 0 )
				return FALSE;// You can't switch a connection if one of the two is non empty
		}
	}
	// Both are either empty or non-existent, so go ahead.
	// (Remember non-exist is only a valid case because you are going to do the switch now.)

	return TRUE;
}

void CaveSystem::registerNewCave( Int theIndex )
{
	Bool needToCreate = FALSE;
	if( theIndex >= m_tunnelTrackerVector.size() )
	{
		// You are new and off the edge, so I will fill NULLs up to you and then make a newTracker at that spot
		while( theIndex >= m_tunnelTrackerVector.size() )
			m_tunnelTrackerVector.push_back( nullptr );

		needToCreate = TRUE;
	}
	else
	{
		// else you either exist or have existed, so I will either let things be or re-create that slot
		if( m_tunnelTrackerVector[theIndex] == nullptr )
			needToCreate = TRUE;
	}

	if( needToCreate )// if true, we new theIndex is the index of a nullptr to be filled
		m_tunnelTrackerVector[theIndex] = newInstance(TunnelTracker);
}

void CaveSystem::registerNewCaveTeam( Int theIndex, const Team *theTeam )
{
	Bool needToCreate = FALSE;

	TeamTunnelTrackerMapType::iterator it = m_tunnelTrackerTeamMap.find(theTeam->getID());
	if (it != m_tunnelTrackerTeamMap.end())
	{
		if( theIndex >= (*it).second.size() )
		{
			// You are new and off the edge, so I will fill NULLs up to you and then make a newTracker at that spot
			while( theIndex >= (*it).second.size() )
				(*it).second.push_back( nullptr );

			(*it).second[theIndex] = newInstance(TunnelTracker);
		}
		else
		{
			// else you either exist or have existed, so I will either let things be or re-create that slot
			if( (*it).second[theIndex] == nullptr )
				(*it).second[theIndex] = newInstance(TunnelTracker);
		}
	}
	else
	{
		needToCreate = TRUE;
	}

	if( needToCreate )// if true, we new theIndex is the index of a nullptr to be filled
	{
		while( theIndex >= m_tunnelTrackerTeamMap[theTeam->getID()].size() )
			m_tunnelTrackerTeamMap[theTeam->getID()].push_back( nullptr );
			
		m_tunnelTrackerTeamMap[theTeam->getID()][theIndex] = newInstance(TunnelTracker);
	}
}

void CaveSystem::unregisterCave( Int theIndex )
{
	// Doesn't need to do a thing.  ContainModule logic knows how to say goodbye, and a TunnelTracker
	// knows how to exist while having no entry points.
	theIndex;
}

TunnelTracker *CaveSystem::getTunnelTrackerForCaveIndex( Int theIndex )
{
	TunnelTracker *theTracker = nullptr;
	if( theIndex < m_tunnelTrackerVector.size() )
	{
		theTracker = m_tunnelTrackerVector[theIndex];
	}

	DEBUG_ASSERTCRASH( theTracker != nullptr, ("No one should be interested in a sub-cave that doesn't exist.") );

	return theTracker;
}

TunnelTracker *CaveSystem::getTunnelTrackerForCaveIndexTeam( Int theIndex, const Team *theTeam )
{
	if(m_tunnelTrackerTeamMap.empty())
		return nullptr;

	TunnelTracker *theTracker = nullptr;
	TeamTunnelTrackerMapType::const_iterator it = m_tunnelTrackerTeamMap.find(theTeam->getID());
	if (it != m_tunnelTrackerTeamMap.end())
	{
		if( theIndex < (*it).second.size() )
		{
			theTracker = (*it).second[theIndex];
		}
	}

	DEBUG_ASSERTCRASH( theTracker != nullptr, ("No one should be interested in a sub-cave that doesn't exist.") );

	return theTracker;
}

TunnelTracker *CaveSystem::getTunnelTracker( Bool useTeams, Int theIndex, const Team *theTeam )
{
	// That was quick
	if(useTeams)
		return getTunnelTrackerForCaveIndexTeam( theIndex, theTeam );
	else
		return getTunnelTrackerForCaveIndex( theIndex );
}

// ------------------------------------------------------------------------------------------------
/** Xfer Method
	* Version Info
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void CaveSystem::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// tunnel tracker size and data
	UnsignedShort count = m_tunnelTrackerVector.size();
	xfer->xferUnsignedShort( &count );
	TunnelTracker *tracker;
	if( xfer->getXferMode() == XFER_SAVE )
	{
		std::vector< TunnelTracker* >::iterator it;

		for( it = m_tunnelTrackerVector.begin(); it != m_tunnelTrackerVector.end(); ++it )
		{

			// xfer data
			if( *it )
				tracker = *it;
			else
				tracker = newInstance( TunnelTracker );

			xfer->xferSnapshot( tracker );

		}

	}
	else
	{

		// the list must be empty now
		if( m_tunnelTrackerVector.empty() == FALSE )
		{

			DEBUG_CRASH(( "CaveSystem::xfer - m_tunnelTrackerVector should be empty but is not" ));
			throw SC_INVALID_DATA;

		}

		// read each item
		for( UnsignedShort i = 0; i < count; ++i )
		{

			// allocate new tracker
			tracker = newInstance( TunnelTracker );

			// read data
			xfer->xferSnapshot( tracker );

			// put in vector
			m_tunnelTrackerVector.push_back( tracker );

		}

	}

	// tunnel tracker size and data
	UnsignedShort mapCount = m_tunnelTrackerTeamMap.size();
	xfer->xferUnsignedShort( &mapCount );
	
	TeamID teamID;
	UnsignedShort vecCount;
	TunnelTracker *tracker_t;
	if( xfer->getXferMode() == XFER_SAVE )
	{
		TeamTunnelTrackerMapType::iterator it;
		for( it = m_tunnelTrackerTeamMap.begin(); it != m_tunnelTrackerTeamMap.end(); ++it )
		{
			// write team ID
			teamID = (*it).first;
			xfer->xferUser( &teamID, sizeof( TeamID ) );
			
			// write vector size
			vecCount = (*it).second.size();
			xfer->xferUnsignedShort( &vecCount );
			
			// write Tunnel Tracker Pointer
			TunnelTrackerPtrVec::iterator it_2;

			for( it_2 = (*it).second.begin(); it_2 != (*it).second.end(); ++it_2 )
			{

				// xfer data
				if( *it_2 )
					tracker_t = *it_2;
				else
					tracker_t = newInstance( TunnelTracker );

				xfer->xferSnapshot( tracker_t );

			}
		}


	}
	else
	{

		// the list must be empty now
		if( m_tunnelTrackerTeamMap.empty() == FALSE )
		{

			DEBUG_CRASH(( "CaveSystem::xfer - m_tunnelTrackerTeamMap should be empty but is not" ));
			throw SC_INVALID_DATA;

		}

		// read each item
		for( UnsignedShort i = 0; i < mapCount; ++i )
		{
			// read team ID
			xfer->xferUser( &teamID, sizeof( TeamID ) );

			xfer->xferUnsignedShort( &vecCount );

			for( UnsignedShort i_2 = 0; i_2 < vecCount; ++i_2 )
			{
				// allocate new tracker
				tracker_t = newInstance( TunnelTracker );

				// read data
				xfer->xferSnapshot( tracker_t );

				// put in vector
				m_tunnelTrackerTeamMap[teamID].push_back( tracker_t );
			}

		}

	}

}



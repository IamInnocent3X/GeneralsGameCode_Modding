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

// DeployStyleAIUpdate.h ////////////
// Author: Kris Morness, August 2002
// Desc:   State machine that allows deploying/undeploying to control the AI.
//         When deployed, you can't move, when undeployed, you can't attack.

#pragma once

#include "Common/StateMachine.h"
#include "GameLogic/Module/AIUpdate.h"

//-------------------------------------------------------------------------------------------------
enum DeployStateTypes CPP_11(: Int)
{
	READY_TO_MOVE,							///< Mobile, can't attack.
	DEPLOY,											///< Not mobile, can't attack, currently unpacking to attack
	READY_TO_ATTACK,						///< Not mobile, can attack
	UNDEPLOY,										///< Not mobile, can't attack, currently packing to move
	ALIGNING_TURRETS,						///< While deployed, we must wait for the turret to go back to natural position prior to undeploying.
};

//-------------------------------------------------------------------------------------------------
class DeployStyleAIUpdateModuleData : public AIUpdateModuleData
{
public:
	UnsignedInt			m_unpackTime;
	UnsignedInt			m_packTime;
	Bool						m_resetTurretBeforePacking;
	Bool						m_turretsFunctionOnlyWhenDeployed;
	Bool						m_turretsMustCenterBeforePacking;
	Bool						m_manualDeployAnimations;
	Bool						m_removeStatusAfterTrigger;
	Bool						m_deployNoRelocate;
	Bool						m_moveAfterDeploy;
	Bool						m_deployInitiallyDisabled;
	ObjectStatusMaskType 		m_statusToDeploy;
	ObjectStatusMaskType 		m_statusToUndeploy;
	std::vector<AsciiString> 	m_customStatusToDeploy;
	std::vector<AsciiString> 	m_customStatusToUndeploy;
	std::vector<AsciiString> 	m_deployFunctionChangeUpgrade;
	std::vector<AsciiString> 	m_deployNoRelocateUpgrades;
	std::vector<AsciiString> 	m_moveAfterDeployUpgrades;

	DeployStyleAIUpdateModuleData()
	{
		m_unpackTime = 0;
		m_packTime = 0;
		m_resetTurretBeforePacking = false;
		m_turretsFunctionOnlyWhenDeployed = false;
		m_turretsMustCenterBeforePacking = FALSE;
		m_manualDeployAnimations = FALSE;
		m_removeStatusAfterTrigger = FALSE;
		m_deployInitiallyDisabled = FALSE;
		m_deployNoRelocate = FALSE;
		m_moveAfterDeploy = FALSE;
		m_deployNoRelocateUpgrades.clear();
		m_moveAfterDeployUpgrades.clear();
		m_customStatusToDeploy.clear();
		m_customStatusToUndeploy.clear();
		m_deployFunctionChangeUpgrade.clear();
	}

	static void buildFieldParse(MultiIniFieldParse& p)
	{
		AIUpdateModuleData::buildFieldParse(p);

		static const FieldParse dataFieldParse[] =
		{
			{ "UnpackTime",					INI::parseDurationUnsignedInt,	NULL, offsetof( DeployStyleAIUpdateModuleData, m_unpackTime ) },
			{ "PackTime",						INI::parseDurationUnsignedInt,	NULL, offsetof( DeployStyleAIUpdateModuleData, m_packTime ) },
			{ "ResetTurretBeforePacking", INI::parseBool,						NULL, offsetof( DeployStyleAIUpdateModuleData, m_resetTurretBeforePacking ) },
			{ "TurretsFunctionOnlyWhenDeployed", INI::parseBool,		NULL, offsetof( DeployStyleAIUpdateModuleData, m_turretsFunctionOnlyWhenDeployed ) },
			{ "TurretsMustCenterBeforePacking", INI::parseBool,			NULL, offsetof( DeployStyleAIUpdateModuleData, m_turretsMustCenterBeforePacking ) },
			{ "ManualDeployAnimations",	INI::parseBool,							NULL, offsetof( DeployStyleAIUpdateModuleData, m_manualDeployAnimations ) },
			{ "StatusToDeploy",		ObjectStatusMaskType::parseFromINI,	NULL, offsetof( DeployStyleAIUpdateModuleData, m_statusToDeploy ) },
			{ "StatusToUndeploy",		ObjectStatusMaskType::parseFromINI,	NULL, offsetof( DeployStyleAIUpdateModuleData, m_statusToUndeploy ) },
			{ "CustomStatusToDeploy",	INI::parseAsciiStringVector, NULL, 	offsetof( DeployStyleAIUpdateModuleData, m_customStatusToDeploy ) },
			{ "CustomStatusToUndeploy",	INI::parseAsciiStringVector, NULL, 	offsetof( DeployStyleAIUpdateModuleData, m_customStatusToUndeploy ) },
			{ "RemoveStatusAfterTrigger",	INI::parseBool, NULL, 	offsetof( DeployStyleAIUpdateModuleData, m_removeStatusAfterTrigger ) },
			{ "DeployNoRelocate",	INI::parseBool, NULL, 	offsetof( DeployStyleAIUpdateModuleData, m_deployNoRelocate ) },
			{ "MoveAfterDeploy",	INI::parseBool, NULL, 	offsetof( DeployStyleAIUpdateModuleData, m_moveAfterDeploy ) },
			{ "DeployNoRelocateUpgrades",	INI::parseAsciiStringVector, NULL, 	offsetof( DeployStyleAIUpdateModuleData, m_deployNoRelocateUpgrades ) },
			{ "MoveAfterDeployUpgrades",	INI::parseAsciiStringVector, NULL, 	offsetof( DeployStyleAIUpdateModuleData, m_moveAfterDeployUpgrades ) },
			{ "DeployInitiallyDisabled",	INI::parseBool, NULL, 	offsetof( DeployStyleAIUpdateModuleData, m_deployInitiallyDisabled ) },
			{ "ChangeInitiallyDisabledUpgrades", INI::parseDeployFunctionChangeUpgrade, NULL, offsetof( DeployStyleAIUpdateModuleData, m_deployFunctionChangeUpgrade ) },
			{ 0, 0, 0, 0 }
		};
		p.add(dataFieldParse);
	}
};

//-------------------------------------------------------------------------------------------------
class DeployStyleAIUpdate : public AIUpdateInterface
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE( DeployStyleAIUpdate, "DeployStyleAIUpdate"  )
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA( DeployStyleAIUpdate, DeployStyleAIUpdateModuleData )

private:

public:

	DeployStyleAIUpdate( Thing *thing, const ModuleData* moduleData );
	// virtual destructor prototype provided by memory pool declaration

 	virtual void aiDoCommand(const AICommandParms* parms);
	virtual Bool isIdle() const;
	//virtual void doIdleUpdate() { wakeUpNow(); }
	virtual void doStatusUpdate();
	virtual void doUpgradeUpdate();
	virtual UpdateSleepTime update();

	UnsignedInt getUnpackTime()					const { return getDeployStyleAIUpdateModuleData()->m_unpackTime; }
	UnsignedInt getPackTime()						const { return getDeployStyleAIUpdateModuleData()->m_packTime; }
	Bool doTurretsFunctionOnlyWhenDeployed() const { return getDeployStyleAIUpdateModuleData()->m_turretsFunctionOnlyWhenDeployed; }
	Bool doTurretsHaveToCenterBeforePacking() const { return getDeployStyleAIUpdateModuleData()->m_turretsMustCenterBeforePacking; }
	void setMyState( DeployStateTypes StateID, Bool reverseDeploy = FALSE );
protected:

	Bool checkForDeployUpgrades(const AsciiString& type) const;
	Bool checkAfterDeploy(const AsciiString& type) const;
	void doRemoveStatusTrigger();

	DeployStateTypes				m_state;
	UnsignedInt							m_frameToWaitForDeploy;
	Bool							m_isInRange;
	Bool							m_doDeploy;
	Bool							m_doUndeploy;
	Bool							m_hasDeploy;
	Bool							m_hasUndeploy;
	Bool							m_moveAfterDeploy;
	Bool							m_deployNoRelocate;
	Bool							m_needsDeployToFireObject;
	Bool							m_needsDeployToFireTurret;
	Bool							m_needsDeployToFireDeployed;
	Bool							m_doRemoveStatusAfterTrigger;
	std::vector<AsciiString> 		m_deployObjectFunctionChangeUpgrade;
	std::vector<AsciiString> 		m_deployTurretFunctionChangeUpgrade;
	std::vector<AsciiString> 		m_turretDeployedFunctionChangeUpgrade;
};

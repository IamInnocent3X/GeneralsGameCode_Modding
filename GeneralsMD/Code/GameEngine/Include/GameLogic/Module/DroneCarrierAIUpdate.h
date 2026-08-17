// FILE: DroneCarrierAIUpdate.h /////////////////////////////////////////////////////////////////////
// Desc: Mobile drone carrier.
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "GameLogic/Module/BehaviorModule.h"
#include "GameLogic/Module/DieModule.h"
#include "GameLogic/Module/AIUpdate.h"
#include "GameLogic/Module/SpawnBehavior.h"


//-------------------------------------------------------------------------------------------------
class DroneCarrierAIUpdateModuleData : public AIUpdateModuleData
{
public:
	Int										m_slots;
	UnsignedInt						m_respawn_time;
	Bool									m_drones_enter_main_door;
	std::vector<AsciiString> m_spawnTemplateNameData;

	DroneCarrierAIUpdateModuleData();

	static void buildFieldParse(MultiIniFieldParse& p);
};

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
class DroneCarrierAIUpdate : public AIUpdateInterface,
	public SpawnBehaviorInterface,
	public DieModuleInterface
	//public ExitInterface
{

	MEMORY_POOL_GLUE_WITH_USERLOOKUP_CREATE(DroneCarrierAIUpdate, "DroneCarrierAIUpdate")
	MAKE_STANDARD_MODULE_MACRO_WITH_MODULE_DATA(DroneCarrierAIUpdate, DroneCarrierAIUpdateModuleData)

public:

	DroneCarrierAIUpdate(Thing* thing, const ModuleData* moduleData);
	// virtual destructor prototype provided by memory pool declaration

	static Int getInterfaceMask() { return UpdateModule::getInterfaceMask() | (MODULEINTERFACE_DIE); }

	// BehaviorModule
	virtual DieModuleInterface* getDie() override { return this; }

	// UpdateModule
	virtual UpdateSleepTime update() override;

	// SpawnBehaviorInterface
	virtual SpawnBehaviorInterface* getSpawnBehaviorInterface() override { return this; }

	virtual Bool maySpawnSelfTaskAI(Real maxSelfTaskersRatio) override { return false; };
	virtual void onSpawnDeath(ObjectID deadSpawn, DamageInfo* damageInfo) override;
	virtual Object* getClosestSlave(const Coord3D* pos) override;
	virtual void orderSlavesToAttackTarget(Object* target, Int maxShotsToFire, CommandSourceType cmdSource) override;
	virtual void orderSlavesToAttackPosition(const Coord3D* pos, Int maxShotsToFire, CommandSourceType cmdSource) override;
	virtual CanAttackResult getCanAnySlavesAttackSpecificTarget(AbleToAttackType attackType, const Object* target, CommandSourceType cmdSource) override;
	virtual CanAttackResult getCanAnySlavesUseWeaponAgainstTarget(AbleToAttackType attackType, const Object* victim, const Coord3D* pos, CommandSourceType cmdSource) override;
	virtual Bool canAnySlavesAttack() override;
	virtual void orderSlavesToGoIdle(CommandSourceType cmdSource) override;
	virtual void orderSlavesDisabledUntil(DisabledType type, UnsignedInt frame) override;
	virtual void orderSlavesToClearDisabled(DisabledType type) override;
	virtual void giveSlavesStealthUpgrade(Bool grantStealth) override;
	virtual Bool areAllSlavesStealthed() const override;
	virtual void revealSlaves() override;
	virtual Bool doSlavesHaveFreedom() const override { return false; };

	// IamInnocent - declared for TheSuperHackers @bugfix Stubbjax 22/06/2026 Only allow targeting Stinger Sites when they contain Soldiers.
	virtual Int getSlaveCount() const override { return 0; }

	// IamInnocent - SpawnBehaviorInterface properties for MobMemberSlavedUpdate, required to be declared for SpawnBehaviorInterfaces
	virtual void friend_refreshUpdate(Bool isInstant) override {}
	virtual void updateMobMembers() override {}
	virtual Bool informSlaveInfo(ObjectID slaveID, Real currHealth, Real currMaxHealth) override { return FALSE; }
	virtual Bool informSelfTasking(ObjectID slaveID, Bool selfTasking) override { return FALSE; }

	// DieModule
	virtual void onDie(const DamageInfo* damageInfo) override;

	// AIUpdateInterface
	virtual void aiDoCommand(const AICommandParms* parms) override;

	static bool isDroneCombatReady(Object* drone);

private:

	Bool is_full(); ///< has this carrier an open spot?
	Bool createSpawn();										///< Actual work of creating a guy

	void deployDrones(); ///< let all drones exit the transport
	void retrieveDrones(); ///< order all drones to go back to the transport

	bool areDronesApproaching(); ///< check if there is a drone currently trying to land

	void propagateOrdersToDrones();
	void propagateOrderToSpecificDrone(Object* drone);

	//works for both Object and Coord3D
	template <typename T>
	bool targetInRange(const T* target);

	const ThingTemplate* m_spawnTemplate;	///< What it is I spawn
	std::vector<ObjectID> m_spawnIDs;				///< IDs of currently active drones
	std::vector<AsciiString>::const_iterator m_templateNameIterator;
	UnsignedInt m_rebuild_time; // which frame a drone will be rebuilt. 0 if not active
	Bool m_active;									///< Am I currently turned on
	Bool m_initial_spawns; ///< Have initial drones be spawned? (first update frame)
	ObjectID											m_designatedTarget;
	AICommandType									m_designatedCommand;
	Coord3D												m_designatedPosition;
	bool													m_doorOpen;
};

template bool DroneCarrierAIUpdate::targetInRange<Object>(const Object* target);
template bool DroneCarrierAIUpdate::targetInRange<Coord3D>(const Coord3D* target);


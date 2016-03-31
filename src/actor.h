#pragma once

#include "gameEffects.h"
#include "cellMgr.h"

#include <stdint.h>

struct actor_t;
struct mapChannel_t;
struct mapChannelClient_t;

struct actorAppearanceData_t
{
    uint32_t classId;
    uint32_t hue;
};

/*
 * Central structure used to store any kind of NPC/creature/player attribute
 * Note that even creatures have attributes like body, mind and spirit since they inherit from the actor class.
 */
struct actorStats_t
{
    int32_t level;
    // regen rate
    int32_t regenRateCurrentMax;
    int32_t regenRateNormalMax; // regen rate without bonus
    float regenHealthPerSecond; // health regen per second
    // health
    int32_t healthCurrent;
    int32_t healthCurrentMax;
    int32_t healthNormalMax;
    // body
    int32_t bodyCurrent;
    int32_t bodyCurrentMax;
    int32_t bodyNormalMax;
    // mind
    int32_t mindCurrent;
    int32_t mindCurrentMax;
    int32_t mindNormalMax;
    // spirit
    int32_t spiritCurrent;
    int32_t spiritCurrentMax;
    int32_t spiritNormalMax;
    // chi/power
    int32_t chiCurrent;
    int32_t chiCurrentMax;
    int32_t chiNormalMax;
    // armor
    int32_t armorCurrent;
    int32_t armorCurrentMax;
    int32_t armorNormalMax;
    // armor regen
    int32_t armorRegenCurrent;
    // todo: Regeneration rates
};

/*
    BODY = 1
    MIND = 2
    SPIRIT = 3
    HEALTH = 4
    CHI = 5
    POWER = 6
    AWARE = 7
    ARMOR = 8
    SPEED = 9
    REGEN = 10
*/



typedef struct
{
    int32_t actionId;
    int32_t actionArgId;
    int64_t targetEntityId;
    void(*actorActionUpdateCallback)(mapChannel_t* mapChannel, actor_t* actor, int32_t newActionState);
    // todo: For now, objects use a separate action handling code - eventually it should be merged into this one
}actorCurrentAction_t;

#define ACTOR_ACTION_STATE_COMPLETE			(1)
#define ACTOR_ACTION_STATE_INTERRUPTED		(2)


struct actor_t
{
    int32_t entityId;
    int32_t entityClassId;
    int8_t name[64];
    int8_t family[64];
    actorStats_t stats;
    // actorAppearanceData_t appearanceData[21]; // should move this to manifestation and npc structure? Is this used by creatures?
    float posX;
    float posY;
    float posZ;
    float rotation;
    int32_t contextId;
    //int32_t attackstyle;
    //int32_t actionid;
    bool isRunning;
    bool inCombatMode;
    mapCellLocation_t cellLocation;
    gameEffect_t *activeEffects;
    int8_t state;
    // action data
    actorCurrentAction_t currentAction;
    // sometimes we only have access to the actor, the owner variable allows us to access the client anyway (only if actor is a player manifestation)
    mapChannelClient_t* owner;
};

#define ACTOR_STATE_ALIVE	0
#define ACTOR_STATE_DEAD	1


// actor action handling
bool actor_hasActiveAction(actor_t* actor);
void actor_completeCurrentAction(mapChannel_t* mapChannel, actor_t* actor); // should not be called manually
void actor_startActionOnEntity(mapChannel_t* mapChannel, actor_t* actor, int64_t targetEntityId, int32_t actionId, int32_t actionArgId, int32_t windupTime, int32_t recoverTime, void(*actorActionUpdateCallback)(mapChannel_t* mapChannel, actor_t* actor, int32_t newActionState));

#define WINDUP_MANUAL_ACTION	(-1) // pass to actor_startActionOnEntity to disable the automatic action completion

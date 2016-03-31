#pragma once

#include <stdint.h>
#include <vector>

struct dtNavMesh;
struct di_characterData_t;

typedef struct _mapChannel_t mapChannel_t;
typedef struct _clientGamemain_t clientGamemain_t;
typedef struct _manifestation_t manifestation_t;

#define CHANNEL_LIMIT 14

struct mapChannelClient_objectUse_t
{
    uint64_t entityId;
    int32_t actionId;
    int32_t actionArg;
};

typedef struct _mapChannelClient_t
{
    unsigned long long clientEntityId;
    clientGamemain_t *cgm;
    manifestation_t *player;
    mapChannel_t *mapChannel;
    di_characterData_t *tempCharacterData; // used while loading
    bool logoutActive;
    unsigned long long logoutRequestedLast;
    bool removeFromMap; // should be removed from the map at end of processing
    bool disconnected; // client disconnected (do not pass to other)
    // chat
    uint32_t joinedChannels;
    uint32_t channelHashes[CHANNEL_LIMIT];
    // inventory data
    inventory_t inventory;
    // mission data
    hashTable_t mission;
    // object interaction
    mapChannelClient_objectUse_t usedObject;
    // gm flags
    bool gmFlagAlwaysFriendly;
}mapChannelClient_t;

#define MAPCHANNEL_PLAYERQUEUE 32

typedef struct  
{
    int32_t period;
    int32_t timeLeft;
    void *param;
    bool (*cb)(mapChannel_t *mapChannel, void *param, int32_t timePassed);
}mapChannelTimer_t;

struct mapChannelAutoFireTimer_t
{
    int32_t delay;
    int32_t timeLeft;
    //manifestation_t* origin;
    //item_t* weapon;
    mapChannelClient_t* client;
};

/*
 * For every map instance, there exists 1 map channel.
 * But there can exist multiple map instances for a single map.
 */
typedef struct _mapChannel_t
{
    gameData_mapInfo_t *mapInfo;
    mapChannelClient_t **playerList;
    int32_t loadState; // temporary variable used for multithreaded but synchronous operations
    int32_t playerCount;
    int32_t playerLimit;
    hashTable_t ht_socketToClient; // maps socket to client structure
    // ringbuffer for passed players
    clientGamemain_t *rb_playerQueue[MAPCHANNEL_PLAYERQUEUE];
    int32_t rb_playerQueueReadIndex;
    int32_t rb_playerQueueWriteIndex;
    TMutex criticalSection;
    // timers
    uint32_t timer_clientEffectUpdate;
    uint32_t timer_missileUpdate;
    uint32_t timer_dynObjUpdate;
    uint32_t timer_generalTimer;
    uint32_t timer_controller;
    uint32_t timer_playerUpdate;
    //mapChannelTimer_t cp_trigger; 
    // other timers
    std::vector<mapChannelAutoFireTimer_t> autoFire_timers;
    std::vector<mapChannelTimer_t*> timerList;
    // cell data
    mapCellInfo_t mapCellInfo;
    // missile data
    missileInfo_t missileInfo;
    // dynamic object info ( contains only objects that require regular updates )
    std::vector<dynObject_workEntry_t*> updateObjectList;
    // list of all waypoints on this map
    std::vector<dynObject_t*> waypoints;
    // list of all wormholes on this map // need to be moved elsewhere
    std::vector<dynObject_t*> wormholes;
    // navmesh
    dtNavMesh* navMesh;
    dtNavMeshQuery* navMeshQuery;
    // effect
    uint32_t currentEffectID; // increases with every spawned game effect
}mapChannel_t;

typedef struct 
{
    mapChannel_t *mapChannelArray;
    int32_t mapChannelCount;
}mapChannelList_t;

void mapChannel_init();
void mapChannel_start(int32_t *contextIdList, int32_t contextCount);
mapChannel_t *mapChannel_findByContextId(int32_t contextId);
bool mapChannel_pass(mapChannel_t *mapChannel, clientGamemain_t *cgm);

// timer
//void mapChannel_registerTimer(mapChannel_t *mapChannel, int32_t period, void *param, bool (*cb)(mapChannel_t *mapChannel, void *param, int32_t timePassed));
void mapChannel_registerTimer(mapChannel_t *mapChannel, int32_t period, void *param, bool (*cb)(mapChannel_t *mapChannel, void *param, int32_t timePassed));
void mapChannel_registerAutoFireTimer(mapChannelClient_t* cm);
void mapChannel_removeAutoFireTimer(mapChannelClient_t* cm);

#pragma once

#include <stdint.h>

struct mapChannelClient_t;
struct creature_t;

//// npc types
//#define NPC_TYPE_NONE	0
//#define NPC_TYPE_VENDOR	1

// conversation types
#define CONVO_STATUS_NONE 0
#define CONVO_STATUS_UNAVAILABLE 1
#define CONVO_STATUS_AVAILABLE 2
#define CONVO_STATUS_OBJ_COMP 3
#define CONVO_STATUS_MIS_COMP 4
#define CONVO_STATUS_REWARD 5
#define CONVO_STATUS_MIS_REMINDER 6
#define CONVO_STATUS_OBJ_CHOICE 7
#define CONVO_STATUS_OBJ_AMB 8
#define CONVO_STATUS_VENDING 9
#define CONVO_STATUS_TRAIN 10
#define CONVO_STATUS_GREETING 11
#define CONVO_STATUS_IMPORTANT_GREETING 12
#define CONVO_STATUS_CLAN 13
#define CONVO_STATUS_AUCTIONEER 14
#define CONVO_STATUS_ESCORT 15
#define CONVO_STATUS_CRIT_DEATH 16
#define CONVO_STATUS_WARGAME_HOSTILE 17
#define CONVO_STATUS_WARGAME_FRIENDLY 18
#define CONVO_STATUS_WARGAME_NEUTRAL 19
#define CONVO_STATUS_WARGAME_HOSTILE_SAFE 20
#define CONVO_STATUS_WARGAME_FRIENDLY_SAFE 21
#define	CONVO_STATUS_WARGAME_NEUTRAL_SAFE 22
#define	CONVO_STATUS_IDLE 23

typedef struct
{
    //uint8_t relationFlags; // is collector, dispenser, objective etc.
    uint16_t missionIndex;
}npcDataRelatedMission_t;

//#define NPC_MISSION_RFLAG_COLLECTOR	(1)
//#define NPC_MISSION_RFLAG_DISPENSOR	(2)
//#define NPC_MISSION_RFLAG_OBJECTIVE	(4)


typedef struct _npc_t 
{
	//missionList_t *missionList;		// available missions
	// sint8 npcType;						// the npc type (see above)
	// mission data
	npcDataRelatedMission_t* relatedMissions; // list of all missions that involve this NPC type
    int32_t relatedMissionCount;
    int32_t npcPackageId;
}npcData_t;

//void npc_test(mapChannelClient_t *client);
//
//npc_t* npc_createNPC(mapChannel_t *mapChannel, int32_t classId, sint8 *name);
//
//// set (must not be called after addToWorld, does not take effect on client instantly)
//void npc_setLocation(npc_t *npc, float x, float y, float z, float rX, float rY);
//void npc_setType(npc_t *npc, int32_t npcType);
//void npc_setMissionList(npc_t *npc, missionList_t *missionList);
//void npc_setAppearanceEntry(npc_t *npc, int32_t entryIndex, int32_t classId, uint32 hue);
//
//// update
//void npc_updateConversationStatus(mapChannelClient_t *client, npc_t *npc);
//void npc_updateAppearanceItem(mapChannel_t *mapChannel, npc_t *npc, uint32 itemClassId, uint32 hue);
//void npc_updateName(mapChannel_t *mapChannel, npc_t *npc, sint8 *newName);
//

void npc_creature_updateConversationStatus(mapChannelClient_t *client, creature_t *creature);

void npc_recv_RequestNPCConverse(mapChannelClient_t *cm, uint8_t *pyString, int32_t pyStringLen);
void npc_recv_RequestNPCVending(mapChannelClient_t *client, uint8_t *pyString, int32_t pyStringLen);
void npc_recv_RequestVendorSale(mapChannelClient_t *client, uint8_t *pyString, int32_t pyStringLen);
void npc_recv_RequestVendorPurchase(mapChannelClient_t *client, uint8_t *pyString, int32_t pyStringLen);
void npc_recv_AssignNPCMission(mapChannelClient_t *client, uint8_t *pyString, int32_t pyStringLen);
void npc_recv_CompleteNPCObjective(mapChannelClient_t *client, uint8_t *pyString, int32_t pyStringLen);
void npc_recv_CompleteNPCMission(mapChannelClient_t *client, uint8_t *pyString, int32_t pyStringLen);

//
//void npc_cellIntroduceNPCToClients(mapChannel_t *mapChannel, npc_t *npc, mapChannelClient_t **playerList, int32_t playerCount);
//void npc_cellIntroduceNPCsToClient(mapChannel_t *mapChannel, mapChannelClient_t *client, npc_t **npcList, int32_t npcCount);
//
//void npc_cellDiscardNPCToClients(mapChannel_t *mapChannel, npc_t *npc, mapChannelClient_t **playerList, int32_t playerCount);
//void npc_cellDiscardNPCsToClient(mapChannel_t *mapChannel, mapChannelClient_t *client, npc_t **npcList, int32_t npcCount);
//
void npc_init();

#include "Global.h"
#include <time.h>
/*
Behavior controller (AI)
responsible for:
    creature movement, decisions, combat or any other AI actions.
*/


float calcAngle(float a_x, float a_y)
{
    //float a_x = p2_x - p1_x;
    //float a_y = p2_y - p1_y;

    float b_x = 1.0;
    float b_y = 0.0;

    float dif = a_x*a_x+a_y*a_y;
    if( dif < 0.01f )
        return 0.0;

    return acos((a_x*b_x+a_y*b_y)/sqrt(dif));
}

// returns the distance moved
float updateEntityMovementW2(float difX, float difY, float difZ, creature_t *creature,mapChannel_t *mapChannel, float speeddiv, bool isMoved)
{
    float length = 1.0f / sqrt(difX*difX + difY*difY + difZ*difZ);
    float velocity = 0.0f;
    difX *= length;
    difY *= length;
    difZ *= length;
    float vX = atan2(-difX, -difZ);
    // multiplicate with speed
    if( isMoved == true)
        velocity = speeddiv;
    else
        velocity = 0.0f;
    velocity /= 4.0f;
    difX *= velocity;
    difY *= velocity;
    difZ *= velocity;
    // move unit
    if(isMoved == true)
    {
        creature->actor.posX += difX;
        creature->actor.posY += difY;
        creature->actor.posZ += difZ;
    }
    // send movement update
    netCompressedMovement_t movement = {0};
    movement.entityId = creature->actor.entityId;
    movement.posX24b = (uint32)(creature->actor.posX*256.0f);
    movement.posY24b = (uint32)(creature->actor.posY*256.0f);
    movement.posZ24b = (uint32)(creature->actor.posZ*256.0f);

    movement.flag = 0x08;
    movement.viewX = (uint16)(sint16)(vX*10240.0f);
    movement.viewY = 0;//(uint16)(sint16)(vX*10240.0f);//(sint32)(calcAngle(difX, difZ)*1024.0f);
    movement.velocity = (uint16)(velocity * 4.0f * 1024.0f);
    netMgr_cellDomain_sendEntityMovement(mapChannel, &creature->actor, &movement);

    return velocity;
}

#define FACTION_BANE    0
#define FACTION_AFS        1

#define WANDER_IDLE            0
#define WANDER_MOVING        1

void controller_setActionWander(creature_t *creature)
{
    creature->controller.currentAction = BEHAVIOR_ACTION_WANDER;
    creature->controller.actionWander.state = WANDER_IDLE;
    creature->controller.pathIndex = 0;
    creature->controller.pathLength = 0;
}

void controller_setActionFighting(creature_t *creature, uint64 targetEntityId)
{
    creature->controller.currentAction = BEHAVIOR_ACTION_FIGHTING;
    creature->controller.pathIndex = 0;
    creature->controller.pathLength = 0;
    creature->controller.actionFighting.targetEntityId = targetEntityId;
    creature->lastagression = GetTickCount();
}

void controller_setActionPathFollowing(creature_t *creature)
{
    creature->controller.currentAction = BEHAVIOR_ACTION_FOLLOWINGPATH;
    // random position bias added to every node (to make groups look like they do not run on the same path)
    creature->controller.aiPathFollowing.randomPathNodeBiasXZ[0] = (float)((rand()%1001)-500) / 500.0f * creature->controller.aiPathFollowing.generalPath->nodeOffsetRandomization;
    creature->controller.aiPathFollowing.randomPathNodeBiasXZ[1] = (float)((rand()%1001)-500) / 500.0f * creature->controller.aiPathFollowing.generalPath->nodeOffsetRandomization;
}

/*
 * Checks for enemy creatures and players within the given range
 */
bool controller_checkForAttackableEntityInRange(mapChannel_t *mapChannel, creature_t *creature, float range)
{
    // get area of affected cells
    sint32 minX = (sint32)(((creature->actor.posX-range) / CELL_SIZE) + CELL_BIAS);
    sint32 minZ = (sint32)(((creature->actor.posZ-range) / CELL_SIZE) + CELL_BIAS);
    sint32 maxX = (sint32)(((creature->actor.posX+range+(CELL_SIZE-0.0001f)) / CELL_SIZE) + CELL_BIAS);
    sint32 maxZ = (sint32)(((creature->actor.posZ+range+(CELL_SIZE-0.0001f)) / CELL_SIZE) + CELL_BIAS);
    float rangeSqr = range*range;
    float mPosX = creature->actor.posX;
    float mPosY = creature->actor.posY;
    float mPosZ = creature->actor.posZ;

    float foundEntity_distance = range*range+100.0f; // value that is guaranteed to be higher than the found creature
    uint64 foundEntity_entityId = 0;

    // check all cells for players and other creatures
    for(sint32 ix=minX; ix<=maxX; ix++)
    {
        for(sint32 iz=minZ; iz<=maxZ; iz++)
        {
            mapCell_t *nMapCell = cellMgr_getCell(mapChannel, ix, iz);
            if( nMapCell )
            {
                // check players
                if( !nMapCell->ht_playerList.empty() )
                {
                    std::vector<mapChannelClient_t*>::iterator itr = nMapCell->ht_playerList.begin();
                    while (itr != nMapCell->ht_playerList.end())
                    {
                        mapChannelClient_t* player = itr[0];
                        ++itr;
                        if( player->gmFlagAlwaysFriendly )
                            continue;
                        if( player->player->actor->stats.healthCurrent <= 0 )
                            continue;
                        // check distance
                        float difX = (sint32)(player->player->actor->posX) - mPosX;
                        float difY = (sint32)(player->player->actor->posY) - mPosY;
                        float difZ = (sint32)(player->player->actor->posZ) - mPosZ;
                        if( difY >= 1.0f )
                            difY *= difY; // creatures do not see good uphill
                        float dist = difX*difX + difY*difY + difZ*difZ;
                        //    dist = difX*difX + difZ*difZ;
                        if( dist <= rangeSqr )
                        {
                            // set target and change state
                            if (creature->type->faction != FACTION_AFS ) // all non-AFS creatures attack players
                            {
                                if( dist < foundEntity_distance )
                                {
                                    foundEntity_entityId = player->clientEntityId;
                                    foundEntity_distance = dist;
                                }
                            }
                        }
                    }    
                }
                // check creatures
                if( !nMapCell->ht_creatureList.empty() )
                {
                    std::vector<creature_t*>::iterator itrCreature = nMapCell->ht_creatureList.begin();
                    while (itrCreature != nMapCell->ht_creatureList.end())
                    {
                        creature_t* dCreature = itrCreature[0];
                        ++itrCreature;
                        if( dCreature->actor.stats.healthCurrent <= 0 )
                            continue;
                        if( dCreature == creature )
                            continue;
                        // check distance
                        float difX = (sint32)(dCreature->actor.posX) - mPosX;
                        float difY = (sint32)(dCreature->actor.posY) - mPosY;
                        float difZ = (sint32)(dCreature->actor.posZ) - mPosZ;
                        if( difY >= 1.0f )
                            difY *= difY; // creatures do not see good uphill
                        float dist = difX*difX + difY*difY + difZ*difZ;
                        //    dist = difX*difX + difZ*difZ;
                        if( dist <= rangeSqr )
                        {
                            // set target and change state
                            bool c1isBadFaction = (creature->type->faction != FACTION_AFS);
                            bool c2isBadFaction = (dCreature->type->faction != FACTION_AFS);
                            if(c1isBadFaction != c2isBadFaction)
                            {
                                if( dist < foundEntity_distance )
                                {
                                    foundEntity_entityId = dCreature->actor.entityId;
                                    foundEntity_distance = dist;
                                }
                            }
                        }
                    }    
                }
            }
        }
    }

    if( foundEntity_entityId != 0 )
    {
        controller_setActionFighting(creature, foundEntity_entityId);
        return true;
    }
    return false;
}

float controller_math_getDistanceSqr(float p1[3], float p2[3])
{
    float dx = p1[0] - p2[0];
    float dy = p1[1] - p2[1];
    float dz = p1[2] - p2[2];
    return dx*dx+dy*dy+dz*dz;
}

/*
 * Called every 250 milliseconds for every creature on the map
 * The tick parameter stores the amount of ms since the last call (should be 250 usually, but can go up on heavy load)
 */
void controller_creatureThink(mapChannel_t *mapChannel, creature_t *creature, sint32 tick, bool* needDeletion, bool* needCellUpdate)
{
    *needDeletion = false; // set to true if the creature should be removed (for whatever reason)
    *needCellUpdate = false; // set to true in case the creature moved across cells (doesnt need to be instant)
    if(creature->actor.stats.healthCurrent <= 0)     
    {
        creature->controller.deadTime += tick;
        if( creature->controller.deadTime >= 20000 )
        {
            // disappear after 20 seconds
            *needDeletion = true;
        }
        return; // creature dead
    }
    // creature keep apart (we use a stupid trick for this, but it works rather well)
    // pick a random creature from the same cell
    mapCell_t* tCell = cellMgr_getCell(mapChannel, creature->actor.cellLocation.x, creature->actor.cellLocation.z);
    if( tCell && tCell->ht_creatureList.size() > 1 )
    {
        sint32 randomCreatureIndex = rand() % tCell->ht_creatureList.size();
        // get the creature
        creature_t* tCreature = tCell->ht_creatureList[randomCreatureIndex];
        // is it a different alive creature?
        if( creature != tCreature && tCreature->actor.stats.healthCurrent > 0 )
        {
            float difX = creature->actor.posX - tCreature->actor.posX;
            float difY = creature->actor.posY - tCreature->actor.posY;
            float difZ = creature->actor.posZ - tCreature->actor.posZ;
            difY /= 4.0f; // y position has low importance
            float tDist = difX*difX + difY*difY + difZ*difZ;
            if( tDist < 1.0f )
            {
                // creatures are too close together
                // push them apart! (But only along x/z axis)
                // get direction vector
                float tLength = sqrt(difX*difX+difZ*difZ);
                if( tLength >= 0.05f )
                {
                    difX /= tLength;
                    difZ /= tLength;
                    // decrease strength of push vector
                    difX *= 0.3f;
                    difZ *= 0.3f;
                    // push
                    creature->actor.posX += difX;
                    creature->actor.posZ += difZ;
                    tCreature->actor.posX -= difX;
                    tCreature->actor.posZ -= difZ;
                }
            }
        }
    }
    // do we need to check for updated cell position?
    creature->updatePositionCounter -= tick;
    if( creature->updatePositionCounter <= 0 )
    {
        // check for changed cell
        uint32 newLocX = (uint32)((creature->actor.posX / CELL_SIZE) + CELL_BIAS);
        uint32 newLocZ = (uint32)((creature->actor.posZ / CELL_SIZE) + CELL_BIAS);
        // calculate initial cell
        if( creature->actor.cellLocation.x != newLocX || creature->actor.cellLocation.z != newLocZ )
        {
            *needCellUpdate = true;
        }
        creature->updatePositionCounter = CREATURE_LOCATION_UPDATE_TIME;
    }
    if(creature->controller.currentAction == BEHAVIOR_ACTION_WANDER )
    {
        // scan for enemy
        if( controller_checkForAttackableEntityInRange(mapChannel,creature,creature->type->aggroRange) )
        {
            // enemy found!
            return;
        }
        if(creature->controller.actionWander.state == WANDER_IDLE)
        {        
            //--- idle for int time before get new wander position
            uint32 resttime = GetTickCount();
            if( (resttime-creature->lastresttime) > 3500 )
            {
                creature->lastresttime = resttime;
                // does creature have a path?
                if( creature->controller.aiPathFollowing.generalPath )
                {
                    // has path -> don't wander aimlessly, go path walking
                    controller_setActionPathFollowing(creature);
                    return;
                }
                if( creature->type->wander_dist < 0.01f )
                    return; // creature doesn't wander
                // calc target
                sint32 srndx = (float)(rand() % 1000) * 0.001f * creature->type->wander_dist;
                sint32 srndz = (float)(rand() % 1000) * 0.001f * creature->type->wander_dist;
                creature->controller.actionWander.wanderDestination[0] = creature->homePos.x + (float)srndx;
                creature->controller.actionWander.wanderDestination[1] = creature->homePos.y+1.0f;
                creature->controller.actionWander.wanderDestination[2] = creature->homePos.z + (float)srndz;            
                // next step approaching
                creature->controller.actionWander.state = WANDER_MOVING;
            }
        }    
        if(creature->controller.actionWander.state == WANDER_MOVING)
        {   
            // following path (short path)
            if( creature->controller.pathLength == 0 )
            {
                // no path, generate new one
                float startPos[3];
                float endPos[3];
                startPos[0] = creature->actor.posX;
                startPos[1] = creature->actor.posY;
                startPos[2] = creature->actor.posZ;
                endPos[0] = creature->controller.actionWander.wanderDestination[0];
                endPos[1] = creature->controller.actionWander.wanderDestination[1];
                endPos[2] = creature->controller.actionWander.wanderDestination[2];    
                creature->controller.pathIndex = 0;
                creature->controller.pathLength = navmesh_getPath(mapChannel, startPos, endPos, creature->controller.path, false);
                if( creature->controller.pathLength == 0 )
                {
                    // path could not be generated or too short
                    // leave state and go idle mode
                    creature->controller.actionWander.state = WANDER_IDLE;
                    creature->lastresttime = GetTickCount() + (rand()%15)*100;
                    return;
                }
            }
            // get distance
            float* nextPathNodePos = creature->controller.path+(creature->controller.pathIndex*3);
            float difX = nextPathNodePos[0] - creature->actor.posX;
            float difY = nextPathNodePos[1] - creature->actor.posY;
            float difZ = nextPathNodePos[2] - creature->actor.posZ;
            float dist = difX*difX + difZ*difZ;
            // wander target location reached
            if( dist > 0.01f ) // to avoid division by zero
            {
                float distanceMoved = updateEntityMovementW2(difX,difY,difZ,creature,mapChannel,creature->type->walkspeed,true);
                // sometimes it is possible the creature walks past the pathnode a tiny bit,
                // which will force him to move back a step, it does look ugly so here is a tiny workaround
                if( distanceMoved > dist ) // distance moved greater than distance left?
                    dist = 0.0f; // mark pathnode reached
            }
            if(dist < 0.8f) 
            {
                creature->controller.pathIndex++; // goto next node
                if( creature->controller.pathIndex >= creature->controller.pathLength )
                {
                    creature->controller.pathLength = 0;
                    creature->controller.actionWander.state = 0;
                    return;
                }
            }

        }
    }
    else if(creature->controller.currentAction == BEHAVIOR_ACTION_FOLLOWINGPATH )
    {
        // following predefined path (long path)
        // scan for enemy
        if( controller_checkForAttackableEntityInRange(mapChannel,creature,creature->type->aggroRange) )
        {
            // enemy found!
            return;
        } 
        // following path (to next node)
        if( creature->controller.aiPathFollowing.generalPathCurrentNodeIndex >= creature->controller.aiPathFollowing.generalPath->numberOfPathNodes )
            return; // no more nodes in the path
        sint32 realCurrentNodeIndex = creature->controller.aiPathFollowing.generalPathCurrentNodeIndex;
        if( realCurrentNodeIndex < 0 )
            realCurrentNodeIndex = -realCurrentNodeIndex;
        float currentTargetNodePos[3];
        currentTargetNodePos[0] = creature->controller.aiPathFollowing.generalPath->pathNodeList[realCurrentNodeIndex].pos[0];
        currentTargetNodePos[1] = creature->controller.aiPathFollowing.generalPath->pathNodeList[realCurrentNodeIndex].pos[1];
        currentTargetNodePos[2] = creature->controller.aiPathFollowing.generalPath->pathNodeList[realCurrentNodeIndex].pos[2];
        currentTargetNodePos[0] += creature->controller.aiPathFollowing.randomPathNodeBiasXZ[0];
        currentTargetNodePos[2] += creature->controller.aiPathFollowing.randomPathNodeBiasXZ[1];
        // get distance
        float difX = currentTargetNodePos[0] - creature->actor.posX;
        float difY = currentTargetNodePos[1] - creature->actor.posY;
        float difZ = currentTargetNodePos[2] - creature->actor.posZ;
        float dist = difX*difX + difZ*difZ;
        // wander target location reached
        if( dist > 0.01f ) // to avoid division by zero
            updateEntityMovementW2(difX,difY,difZ,creature,mapChannel,creature->type->walkspeed,true);
        if(dist < 0.8f) 
        {
            creature->controller.aiPathFollowing.generalPathCurrentNodeIndex++; // goto next node
            if( creature->controller.aiPathFollowing.generalPathCurrentNodeIndex >= creature->controller.aiPathFollowing.generalPath->numberOfPathNodes )
            {
                // path end reached
                if( creature->controller.aiPathFollowing.generalPath->mode == PATH_MODE_CYCLE )
                    creature->controller.aiPathFollowing.generalPathCurrentNodeIndex = 0;
                else if( creature->controller.aiPathFollowing.generalPath->mode == PATH_MODE_RETURN )
                    creature->controller.aiPathFollowing.generalPathCurrentNodeIndex = -(creature->controller.aiPathFollowing.generalPath->numberOfPathNodes-1) + 1; // a negative number indicates reversed path walking
                else if( creature->controller.aiPathFollowing.generalPath->mode == PATH_MODE_ONESHOT )
                {
                    // no more path
                    // reset path and enter wander mode
                    creature->controller.aiPathFollowing.generalPath = 0;
                    creature->controller.aiPathFollowing.generalPathCurrentNodeIndex = 0;
                    controller_setActionWander(creature);
                }
                return;
            }
            else
            {
                // random position bias added to every node (to make groups look like they do not run on the same path)
                creature->controller.aiPathFollowing.randomPathNodeBiasXZ[0] = (float)((rand()%1001)-500) / 500.0f * creature->controller.aiPathFollowing.generalPath->nodeOffsetRandomization;
                creature->controller.aiPathFollowing.randomPathNodeBiasXZ[1] = (float)((rand()%1001)-500) / 500.0f * creature->controller.aiPathFollowing.generalPath->nodeOffsetRandomization;
                // update home position to be at the (old) current node
                creature->homePos.x = currentTargetNodePos[0];
                creature->homePos.y = currentTargetNodePos[1];
                creature->homePos.z = currentTargetNodePos[2];
            }
        }
    }
    else if(creature->controller.currentAction == BEHAVIOR_ACTION_FIGHTING )
    {
        // get target
        void *target = entityMgr_get(creature->controller.actionFighting.targetEntityId);
        if( target == NULL )
        {
            // target disappeared (player logout or deleted for some reason) - leave combat mode
            controller_setActionWander(creature);
            return;
        }
        // leave combat after 
        sint32 aggtime = GetTickCount();
        if(aggtime - creature->lastagression > creature->type->aggressionTime)
        {
            controller_setActionWander(creature);
            return;
        }            
        // get position of target
        float targetX = 0.0f;
        float targetY = 0.0f;
        float targetZ = 0.0f;
        if( entityMgr_getEntityType(creature->controller.actionFighting.targetEntityId) == ENTITYTYPE_CLIENT )
        {
            mapChannelClient_t *client = (mapChannelClient_t*)target;
            // if target dead, set wander state
            if( client->player->actor->stats.healthCurrent <= 0 || client->player->actor->state == ACTOR_STATE_DEAD )
            {
                controller_setActionWander(creature);
                return;
            }
            targetX = client->player->actor->posX;
            targetY = client->player->actor->posY;
            targetZ = client->player->actor->posZ;
        }
        else if( entityMgr_getEntityType(creature->controller.actionFighting.targetEntityId) == ENTITYTYPE_CREATURE )
        {  
            creature_t  *targetCreature = (creature_t*)target;
            if( targetCreature->actor.stats.healthCurrent <= 0 || targetCreature->actor.state == ACTOR_STATE_DEAD )
            {
                controller_setActionWander(creature);
                return;
            }
            targetX = targetCreature->actor.posX;
            targetY = targetCreature->actor.posY;
            targetZ = targetCreature->actor.posZ;
        }
        else
            __debugbreak(); // todo

        float targetDistX = (targetX - creature->actor.posX);
        float targetDistY = (targetY - creature->actor.posY);
        float targetDistZ = (targetZ - creature->actor.posZ);
        float targetDistSqr = (targetDistX*targetDistX+targetDistY*targetDistY+targetDistZ*targetDistZ);
        // stop tracking target after target exceeds a certain distance to home pos
        // Note: For patrolling creatures the homePos is the last arrived path node 
        float homeLocDistX = (creature->homePos.x - targetX);
        float homeLocDistZ = (creature->homePos.z - targetZ);
        float homeLocDist = homeLocDistX*homeLocDistX + homeLocDistZ*homeLocDistZ;
        if( homeLocDist >= 60.0f*60.0f )
        {
            creature->lastresttime = 0; // forces AI to immediately calculate new wander position
            controller_setActionWander(creature);
            return;
        }
        uint32 time = GetTickCount();
        creature->lastagression = time; // update aggression time if we found our target
        bool needToMove = true;
        for(sint32 i=0; i<8; i++)
        {
            creatureMissile_t* action = creature->type->actions[i];
            if( action == NULL )
                break; // no more action
            // check if we can execute action
            if( targetDistSqr < action->rangeMin*action->rangeMin || targetDistSqr >= action->rangeMax*action->rangeMax )
                continue;
            needToMove = false;
            if( time < creature->controller.actionLockTime[i] )
                continue;
            // rotate
            updateEntityMovementW2(targetDistX,targetDistY,targetDistZ,creature,mapChannel,0.0f,false);
            // execute action and quit
            sint32 dmg = action->minDamage + (rand()%(action->maxDamage-action->minDamage+1));
            missile_launch(mapChannel, &creature->actor, creature->controller.actionFighting.targetEntityId, dmg, action->actionId, action->actionArgId);
            uint32 globalLockTime = time + action->recoverTimeGlobal;
            creature->controller.actionLockTime[i] = time + action->recoverTime;
            for(sint32 f=0; f<8; f++)
                creature->controller.actionLockTime[i] = max(creature->controller.actionLockTime[i], globalLockTime);
            break;
        }
        if( needToMove == false )
            return;
        if( targetDistSqr <= 3.0f*3.0f )
            return; // near enough, dont move
        // after checking for melee and range attack without success, do pathing
        // invalidate path if the target moved away too far from the original path destination
        float tempPos[3];
        tempPos[0] = targetX;
        tempPos[1] = targetY;
        tempPos[2] = targetZ;
        // generate path if there is no current
        if( time >= creature->controller.timerPathUpdateLock )
        {
            if( creature->controller.pathLength > 0 && controller_math_getDistanceSqr(creature->controller.actionFighting.lockedTargetPosition, tempPos) > 1.0f )
            {
                creature->controller.pathLength = 0;
            }
            if( creature->controller.pathLength == 0 )
            {    
                // update path update lock timer
                creature->controller.timerPathUpdateLock = time + 800 + (sint32)(rand()%500);
                
                float pathTarget[3];
                if( targetDistSqr < 0.1f )
                {
                    // if too near, move out of enemy by running to random point somewhere x units around the creature
                    float angle = ((float)rand() / 32767.0f) * 6.28318f; // random angle
                    float distance = 2.5f; // keep 2.5 meter distance
                    pathTarget[0] = targetX + cos(angle) * distance;
                    pathTarget[1] = targetY;
                    pathTarget[2] = targetZ + sin(angle) * distance;
                }
                else
                {
                    // run to nearest point that maintains distance to creature
                    float vecV2A[2]; // vector2D victim->attacker
                    vecV2A[0] = -targetDistX;
                    vecV2A[1] = -targetDistZ;
                    // normalize
                    float vecV2ALen = sqrt(targetDistSqr);
                    vecV2A[0] /= vecV2ALen;
                    vecV2A[1] /= vecV2ALen;
                    // use vector to calculate nearest melee point from our current position
                    float distance = 2.5f; // keep 2.5 meter distance
                    pathTarget[0] = targetX + vecV2A[0] * distance;
                    pathTarget[1] = targetY;
                    pathTarget[2] = targetZ + vecV2A[1] * distance;
                }
                float startPos[3];
                float endPos[3];
                startPos[0] = creature->actor.posX;
                startPos[1] = creature->actor.posY;
                startPos[2] = creature->actor.posZ;
                endPos[0] = pathTarget[0];
                endPos[1] = pathTarget[1];
                endPos[2] = pathTarget[2];    
                creature->controller.pathIndex = 0;
                creature->controller.pathLength = navmesh_getPath(mapChannel, startPos, endPos, creature->controller.path, false);
                if( creature->controller.path == 0 )
                {
                    printf("Cannot find path");
                    return;
                }
                // also update path target variable (using creature position, not path target position)
                creature->controller.actionFighting.lockedTargetPosition[0] = targetX;
                creature->controller.actionFighting.lockedTargetPosition[1] = targetY;
                creature->controller.actionFighting.lockedTargetPosition[2] = targetZ;
            }
        }
        // follow path
        if( creature->controller.pathIndex < creature->controller.pathLength )
        {
            // get distance
            float* nextPathNodePos = creature->controller.path+(creature->controller.pathIndex*3);
            float difX = nextPathNodePos[0] - creature->actor.posX;
            float difY = nextPathNodePos[1] - creature->actor.posY;
            float difZ = nextPathNodePos[2] - creature->actor.posZ;
            float dist = difX*difX + difZ*difZ;
            bool skipDetected = false;
            if( dist > 0.01f ) // to avoid division by zero
            {
                updateEntityMovementW2(difX,difY,difZ,creature,mapChannel, creature->type->runspeed,true);
                // on high movement speeds the movement steps can be large, check if creature didn't run too far
                difX = nextPathNodePos[0] - creature->actor.posX;
                difY = nextPathNodePos[1] - creature->actor.posY;
                difZ = nextPathNodePos[2] - creature->actor.posZ;
                float dist2 = difX*difX + difZ*difZ;
                if( dist2 >= dist )
                {
                    skipDetected = true;
                }
            }            
            if(dist < 0.9f || skipDetected) 
            {
                creature->controller.pathIndex++; // goto next node
                if( creature->controller.pathIndex >= creature->controller.pathLength )
                {
                    creature->controller.pathLength = 0;
                    creature->controller.pathIndex = 0;
                }
            }
        }
    }//---fighting
}
/*
 * Called every 250 milliseconds
 */
void controller_mapChannelThink(mapChannel_t *mapChannel)
{
    // creature deletion and update queue
    std::vector<creature_t*> queue_creatureDeletion;
    std::vector<creature_t*> queue_creatureCellUpdate;
    // todo: When on heavy load, the server should increase the time between calls to
    //       this function. (check player updating as a reference)
    for(sint32 i=0; i<mapChannel->mapCellInfo.loadedCellCount; i++)
    {
        mapCell_t *mapCell = mapChannel->mapCellInfo.loadedCellList[i];
        if( mapCell == NULL ) // should never happen, but still do a check for safety
            continue;
        // creatures
        if( mapCell->ht_creatureList.empty() != true )
        {
            creature_t **creatureList = &mapCell->ht_creatureList[0];
            sint32 creatureCount = mapCell->ht_creatureList.size();
            for(sint32 f=0; f<creatureCount; f++)
            {
                bool needDeletion = false;
                bool needCellUpdate = false;
                controller_creatureThink(mapChannel, creatureList[f], 250, &needDeletion, &needCellUpdate); // update time hardcoded, see todo
                if( needDeletion )
                    queue_creatureDeletion.push_back(creatureList[f]);
                if( needCellUpdate ) // update cell (even when creature is also deleted)
                    queue_creatureCellUpdate.push_back(creatureList[f]);

                    // need to delete creature & we still have a free space in the deletion queue
                    // not so nice hack to remove creatures from the map cell when creature_cellUpdateLocation is called
                    /*std::swap(mapCell->ht_creatureList.at(f), mapCell->ht_creatureList.at(creatureCount-1));
                    mapCell->ht_creatureList.pop_back();
                    creatureCount = mapCell->ht_creatureList.size();
                    if( creatureCount == 0 )
                        break;
                    creatureList = &mapCell->ht_creatureList[0];
                    f--;*/
            }
        }
    }
    //update logic for creatures (same like for deletion below, moving creatures to different vectors is not good)
    if( queue_creatureCellUpdate.empty() != true )
    {
        creature_t **creatureList = &queue_creatureCellUpdate[0];
        sint32 creatureCount = queue_creatureCellUpdate.size();
        for(sint32 f=0; f<creatureCount; f++)
        {
            // calculate new cell position
            uint32 newLocX = (uint32)((creatureList[f]->actor.posX / CELL_SIZE) + CELL_BIAS);
            uint32 newLocZ = (uint32)((creatureList[f]->actor.posZ / CELL_SIZE) + CELL_BIAS);
            creature_cellUpdateLocation(mapChannel, creatureList[f], newLocX, newLocZ);
        }
    }
    // deletion logic for creatures (we have to do it here, since deleting creatures while iterating them is not so nice...)
    // do we need to delete some creatures?
    if( queue_creatureDeletion.empty() != true )
    {
        creature_t **creatureList = &queue_creatureDeletion[0];
        sint32 creatureCount = queue_creatureDeletion.size();
        for(sint32 f=0; f<creatureCount; f++)
        {
            // did the creature have an active loot dispenser?
            if( creatureList[f]->lootDispenserObjectEntityId != 0 )
            {
                dynObject_t* lootDispenserObject = (dynObject_t*)entityMgr_get(creatureList[f]->lootDispenserObjectEntityId);
                if( lootDispenserObject )
                    dynamicObject_destroy(mapChannel, lootDispenserObject);
                creatureList[f]->lootDispenserObjectEntityId = 0;
            }
            // remove creature from world
            cellMgr_removeCreatureFromWorld(mapChannel, creatureList[f]);
            // delete creature entity
            creature_destroy(creatureList[f]);
        }
    }
}

void controller_initForMapChannel(mapChannel_t *mapChannel)
{

}


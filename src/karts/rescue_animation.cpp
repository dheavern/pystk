//
//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2010-2015 Joerg Henrichs
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 3
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#include "karts/rescue_animation.hpp"

#include "config/user_config.hpp"
#include "graphics/referee.hpp"
#include "items/attachment.hpp"
#include "karts/abstract_kart.hpp"
#include "karts/kart_properties.hpp"
#include "modes/follow_the_leader.hpp"
#include "modes/three_strikes_battle.hpp"
#include "network/network_string.hpp"
#include "physics/physics.hpp"
#include "physics/triangle_mesh.hpp"
#include "race/race_manager.hpp"
#include "tracks/drive_graph.hpp"
#include "tracks/quad.hpp"
#include "tracks/track.hpp"
#include "tracks/track_sector.hpp"

#include "ISceneNode.h"

#include <algorithm>
#include <cmath>

RescueAnimation* RescueAnimation::create(AbstractKart* kart,
                                         bool is_auto_rescue)
{
    // When goal phase is happening karts is made stationary, so no animation
    // will be created
    if (World::getWorld()->isGoalPhase())
        return NULL;
    return new RescueAnimation(kart, is_auto_rescue);
}   // create

//-----------------------------------------------------------------------------
/** The constructor stores a pointer to the kart this object is animating,
 *  and initialised the timer.
 *  \param kart Pointer to the kart which is animated.
 */
RescueAnimation::RescueAnimation(AbstractKart* kart, bool is_auto_rescue)
               : AbstractKartAnimation(kart, "RescueAnimation")
{
    m_referee = NULL;
    btTransform prev_trans = kart->getTrans();
    // Get the required final physical transform for network, then reset back
    // to the original transform
    World::getWorld()->moveKartAfterRescue(kart);

    btTransform rescue_transform = kart->getTrans();
    kart->getBody()->setCenterOfMassTransform(prev_trans);
    kart->setTrans(prev_trans);

    // Determine maximum rescue height with up-raycast
    float max_height = m_kart->getKartProperties()->getRescueHeight();
    Vec3 up_vector = m_kart->getTrans().getBasis().getColumn(1);
    float hit_dest = getMaximumHeight(up_vector, Referee::getHeight());

    max_height = std::min(hit_dest, max_height);
    float timer = m_kart->getKartProperties()->getRescueDuration();
    float velocity = max_height / timer;

    init(rescue_transform, velocity);
    m_kart->getAttachment()->clear();

    // Add a hit unless it was auto-rescue
    if (race_manager->isBattleMode() &&
        !is_auto_rescue)
    {
        World::getWorld()->kartHit(m_kart->getWorldKartId());
    }

    // Allow FTL mode to apply special action when the leader is rescued
    if (race_manager->isFollowMode())
    {
        FollowTheLeaderRace *ftl_world =
            dynamic_cast<FollowTheLeaderRace*>(World::getWorld());
        if(ftl_world->isLeader(kart->getWorldKartId()))
            ftl_world->leaderRescued();
    }

    // Clear powerups when rescue in CTF
    if (race_manager->getMinorMode() ==
        RaceManager::MINOR_MODE_CAPTURE_THE_FLAG)
        resetPowerUp();
}   // RescueAnimation

//-----------------------------------------------------------------------------
/* When rescue transform and velocity is known, setting up the rest of data.
 * It is also used for each restoreState to make sure animation end in correct
 * time.
 */
void RescueAnimation::init(const btTransform& rescue_transform,
                           float velocity)
{
    m_rescue_transform = rescue_transform;
    float timer = m_kart->getKartProperties()->getRescueDuration();
    m_end_ticks = m_created_ticks + stk_config->time2Ticks(timer);
    m_rescue_moment = m_created_ticks + stk_config->time2Ticks(timer * 0.4f);
    m_velocity = velocity;
}   // init

//-----------------------------------------------------------------------------
/** This object is automatically destroyed when the timer expires.
 */
RescueAnimation::~RescueAnimation()
{
    m_kart->getBody()->setLinearVelocity(btVector3(0, 0, 0));
    m_kart->getBody()->setAngularVelocity(btVector3(0, 0, 0));
    if (m_referee)
    {
        m_kart->getNode()->removeChild(m_referee->getSceneNode());
        delete m_referee;
    }
}   // ~RescueAnimation

// ----------------------------------------------------------------------------
/** Updates the kart animation.
 *  \param ticks Number of time steps - should be 1.
 */
void RescueAnimation::update(int ticks)
{
    if (World::getWorld()->getTicksSinceStart() > m_rescue_moment)
    {
        float dur = stk_config->ticks2Time(m_end_ticks - m_rescue_moment -
            (World::getWorld()->getTicksSinceStart() - m_rescue_moment));
        Vec3 xyz = m_rescue_transform.getOrigin() +
            dur * m_velocity * m_rescue_transform.getBasis().getColumn(1);
        m_kart->setXYZ(xyz);
        m_kart->setRotation(m_rescue_transform.getRotation());
    }
    else
    {
        float dur = stk_config->ticks2Time(
            World::getWorld()->getTicksSinceStart() - m_created_ticks);
        Vec3 xyz = m_created_transform.getOrigin() +
            dur * m_velocity * m_created_transform.getBasis().getColumn(1);
        m_kart->setXYZ(xyz);
        m_kart->setRotation(m_created_transform.getRotation());
    }
    AbstractKartAnimation::update(ticks);
}   // update

// ----------------------------------------------------------------------------
void RescueAnimation::updateGraphics(float dt)
{
    if (m_referee == NULL)
    {
        m_referee = new Referee(*m_kart);
        m_kart->getNode()->addChild(m_referee->getSceneNode());
    }
    m_referee->setAnimationFrameWithCreatedTicks(m_created_ticks);
    AbstractKartAnimation::updateGraphics(dt);
}   // updateGraphics

// ----------------------------------------------------------------------------
void RescueAnimation::saveState(BareNetworkString* buffer)
{
    AbstractKartAnimation::saveState(buffer);
}   // saveState

// ----------------------------------------------------------------------------
void RescueAnimation::restoreState(BareNetworkString* buffer)
{
    AbstractKartAnimation::restoreState(buffer);
}   // restoreState

/*
 * Copyright (C) 2004 Ivo Danihelka (ivo@danihelka.net)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include "Room.h"

#include "Log.h"
#include "Picture.h"
#include "Field.h"
#include "Rules.h"
#include "LogicException.h"
#include "SoundAgent.h"
#include "SubTitleAgent.h"
#include "DialogAgent.h"
#include "Controls.h"
#include "LoadException.h"

//-----------------------------------------------------------------
Room::Room(int w, int h, const Path &picture)
{
    m_bg = new Picture(picture, 0, 0);
    m_field = new Field(w, h);
    m_controls = new Controls();
    m_impact = Cube::NONE;
    m_fresh = true;
}
//-----------------------------------------------------------------
/**
 * Delete field and models.
 */
Room::~Room()
{
    //NOTE: dialogs must be killed because pointer to the actors
    SubTitleAgent::agent()->removeAll();
    DialogAgent::agent()->removeAll();
    delete m_controls;

    Cube::t_models::iterator end = m_models.end();
    for (Cube::t_models::iterator i = m_models.begin(); i != end; ++i) {
        delete (*i);
    }

    delete m_field;
    delete m_bg;
}
//-----------------------------------------------------------------
/**
 * Add model at scene.
 * @return model index
 */
    int
Room::addModel(Cube *model)
{
    model->rules()->takeField(m_field);
    m_models.push_back(model);

    int model_index = m_models.size() - 1;
    model->setIndex(model_index);
    return model_index;
}
//-----------------------------------------------------------------
/**
 * Return model at index.
 * @throws LogicException when model_index is out of range
 */
Cube *
Room::getModel(int model_index)
{
    Cube *result = NULL;
    if (0 <= model_index && model_index < (int)m_models.size()) {
        result = m_models[model_index];
    }
    else {
        throw LogicException(ExInfo("bad model index")
                .addInfo("model_index", model_index));
    }

    return result;
}
//-----------------------------------------------------------------
/**
 * Return model at location.
 */
Cube *
Room::askField(const V2 &loc)
{
    return m_field->getModel(loc);
}
//-----------------------------------------------------------------
/**
 * Update all models.
 * Prepare new move, let models fall, let models drive, release old position.
 * @return true when room is finished
 */
    bool
Room::nextRound()
{
    bool falling = beginFall();
    if (false == falling) {
        m_controls->driving();
    }
    m_controls->lockPhases();

    return finishRound();
}
//-----------------------------------------------------------------
/**
 * Play sound when some object has fall.
 * NOTE: only one sound is played even more objects have fall
 */
void
Room::playImpact()
{
    switch (m_impact) {
        case Cube::NONE:
            break;
        case Cube::LIGHT:
            SoundAgent::agent()->playRandomSound("impact_light");
            break;
        case Cube::HEAVY:
            SoundAgent::agent()->playRandomSound("impact_heavy");
            break;
        default:
            assert("unknown impact weight" == NULL);
    }
    m_impact = Cube::NONE;
}
//-----------------------------------------------------------------
/**
 * Move all models to new position
 * and check dead fihes.
 */
void
Room::prepareRound()
{
    bool interrupt = false;

    //NOTE: we must call this functions sequential for all objects
    Cube::t_models::iterator end = m_models.end();
    for (Cube::t_models::iterator i = m_models.begin(); i != end; ++i) {
        (*i)->rules()->occupyNewPos();
    }
    for (Cube::t_models::iterator j = m_models.begin(); j != end; ++j) {
        interrupt |= (*j)->rules()->checkDead();
    }
    for (Cube::t_models::iterator k = m_models.begin(); k != end; ++k) {
        interrupt |= (*k)->rules()->checkOut();
    }
    for (Cube::t_models::iterator l = m_models.begin(); l != end; ++l) {
        (*l)->rules()->prepareRound();
    }

    if (interrupt) {
        DialogAgent::agent()->killPlan();
        m_controls->checkActive();
    }
}
//-----------------------------------------------------------------
/**
 * Let things fall.
 * @return true when something is falling.
 */
    bool
Room::falldown()
{
    bool result = false;
    m_impact = Cube::NONE;
    Cube::t_models::iterator end = m_models.end();
    for (Cube::t_models::iterator i = m_models.begin(); i != end; ++i) {
        Rules::eFall fall = (*i)->rules()->actionFall();
        if (Rules::FALL_NOW == fall) {
            result = true;
        }
        else if (Rules::FALL_LAST == fall) {
            if (m_impact < (*i)->getWeight()) {
                m_impact = (*i)->getWeight();
            }
        }
    }

    return result;
}
//-----------------------------------------------------------------
/**
 * Let models to release their old position.
 * Check complete room.
 * @return true when room is finished
 */
bool
Room::finishRound()
{
    bool room_complete = true;

    Cube::t_models::iterator end = m_models.end();
    for (Cube::t_models::iterator i = m_models.begin(); i != end; ++i) {
        (*i)->finishRound();
        room_complete &= (*i)->isSatisfy();
    }

    m_fresh = false;
    return room_complete;
}

//-----------------------------------------------------------------
void
Room::addUnit(Unit *unit)
{
    m_controls->addUnit(unit);
}
//-----------------------------------------------------------------
void
Room::switchFish()
{
    m_controls->switchActive();
}
//-----------------------------------------------------------------
std::string
Room::getMoves() const
{
    return m_controls->getMoves();
}
//-----------------------------------------------------------------
/**
 * Load this move, let object to fall fast.
 * Don't play sound.
 * @return true for finished level
 * @throws LoadException for bad moves
 */
bool
Room::loadMove(char move)
{
    bool complete = false;
    bool falling = true;
    while (falling) {
        falling = beginFall(false);
        makeMove(move, false);

        complete = finishRound();
        if (complete && falling) {
            throw LoadException(ExInfo("load error - early finished level")
                    .addInfo("move", std::string(1, move)));
        }

    }
    return complete;
}
//-----------------------------------------------------------------
/**
 * Begin round.
 * Let objects fall. Don't allow player to move.
 * @param sound whether play sound of impact
 * @return true when something was falling
 */
bool
Room::beginFall(bool sound)
{
    m_fresh = true;
    prepareRound();

    bool falling = falldown();
    m_fresh = (false == falling);
    if (sound) {
        playImpact();
    }
    return falling;
}
//-----------------------------------------------------------------
/**
 * Try make single move.
 * @param anim whether ensure phases for move animation
 * @return true for success or false when something has moved before
 * @throws LoadException for bad moves
 */
bool
Room::makeMove(char move, bool anim)
{
    bool result = false;
    if (m_fresh) {
        if (false == m_controls->makeMove(move)) {
            throw LoadException(ExInfo("load error - bad move")
                    .addInfo("move", std::string(1, move)));
        }
        if (anim) {
            m_controls->lockPhases();
        }
        m_fresh = false;
        result = true;
    }
    return result;
}



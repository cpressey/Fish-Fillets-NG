/*
 * Copyright (C) 2004 Ivo Danihelka (ivo@danihelka.net)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include "SDLSoundAgent.h"

#include "Log.h"
#include "ExInfo.h"
#include "SDLException.h"
#include "MixException.h"
#include "Random.h"
#include "BaseMsg.h"
#include "ResSoundAgent.h"

BaseMsg *SDLSoundAgent::ms_finished = NULL;
//-----------------------------------------------------------------
    void
SDLSoundAgent::own_init()
{
    //TODO: load volume option
    if(SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
        throw SDLException(ExInfo("SDL_InitSubSystem"));
    }

    //NOTE: music does not play on my ALSA with less frequency than 44100
    //int frequency = MIX_DEFAULT_FREQUENCY;
    int frequency = 44100;
    if(Mix_OpenAudio(frequency, MIX_DEFAULT_FORMAT, 2, 1024) < 0) {
        throw MixException(ExInfo("Mix_OpenAudio"));
    }

    m_music = NULL;
    m_soundVolume = MIX_MAX_VOLUME;
    Mix_AllocateChannels(8);
}
//-----------------------------------------------------------------
    void
SDLSoundAgent::own_shutdown()
{
    stopMusic();
    Mix_CloseAudio();
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}
//-----------------------------------------------------------------
/**
 * Play this sound.
 *
 * @return channel number where the sound is played,
 * return -1 on error or when sound is NULL
 */
    int
SDLSoundAgent::playSound(Mix_Chunk *sound)
{
    int channel = -1;
    if (sound) {
        channel = Mix_PlayChannel(-1, sound, 0);
        if (-1 == channel) {
            //NOTE: maybe there are too few open channels
            LOG_WARNING(ExInfo("cannot play sound")
                    .addInfo("Mix", Mix_GetError()));
        }
    }

    return channel;
}

//-----------------------------------------------------------------
/**
 * Play sound once.
 * Take random sound with this name.
 * @param name sound name
 */
    void
SDLSoundAgent::playRandomSound(const std::string &name)
{
    Mix_Chunk *chunk = ResSoundAgent::agent()->getRandomRes(name);
    playSound(chunk);
}
//-----------------------------------------------------------------
/**
 * @param volume percentage volume, e.g. 30%=30
 */
    void
SDLSoundAgent::setSoundVolume(int volume)
{
    m_soundVolume = static_cast<int>(MIX_MAX_VOLUME * volume / 100.0);
    if (m_soundVolume > MIX_MAX_VOLUME) {
        m_soundVolume = MIX_MAX_VOLUME;
    }
    else if (m_soundVolume < 0) {
        m_soundVolume = 0;
    }
    Mix_Volume(-1, m_soundVolume);
}
//-----------------------------------------------------------------
    int
SDLSoundAgent::getSoundVolume()
{
    return m_soundVolume * 100 / MIX_MAX_VOLUME;
}

//---------------------------------------------------------------------------
// Music part
//---------------------------------------------------------------------------

/**
 * Play music.
 * @param file path to music file (i.e. *.ogg)
 * @param finished send this message when music is finished.
 * If finished is NULL, play music forever.
 */
void
SDLSoundAgent::playMusic(const Path &file,
        BaseMsg *finished)
{
    if (m_music) {
        Mix_FreeMusic(m_music);
        m_music = NULL;
    }

    int loops = -1;
    //TODO: is musicFinished callback called from different thread?
    // this code is not thread safe
    if (ms_finished) {
        delete ms_finished;
        ms_finished = NULL;
    }
    if (finished) {
        ms_finished = finished;
        loops = 1;
    }

    m_music = Mix_LoadMUS(file.getNative().c_str());
    if (m_music && (0 == Mix_PlayMusic(m_music, loops))) {
        Mix_HookMusicFinished(musicFinished);
        LOG_DEBUG(ExInfo("playing music")
                .addInfo("music", file.getNative())
                .addInfo("loops", loops));
    }
    else {
        LOG_WARNING(ExInfo("cannot play music")
                .addInfo("music", file.getNative())
                .addInfo("Mix", Mix_GetError()));
    }
}
//-----------------------------------------------------------------
/**
 * @param volume percentage volume, e.g. 30%=30
 */
    void
SDLSoundAgent::setMusicVolume(int volume)
{
    Mix_VolumeMusic(static_cast<int>(MIX_MAX_VOLUME * volume / 100.0));
}
//-----------------------------------------------------------------
    int
SDLSoundAgent::getMusicVolume()
{
    return Mix_VolumeMusic(-1) * 100 / MIX_MAX_VOLUME;
}
//-----------------------------------------------------------------
    void
SDLSoundAgent::stopMusic()
{
    if(Mix_PlayingMusic()) {
        Mix_HookMusicFinished(NULL);
        //NOTE: nice fadeout is too slow
        //Mix_FadeOutMusic(1000);
        Mix_HaltMusic();
    }
    if (m_music) {
        Mix_FreeMusic(m_music);
    }
    if (ms_finished) {
        delete ms_finished;
    }
}
//-----------------------------------------------------------------
/**
 * Callback called when music is finished.
 * NOTE: no one exception can be passed to "C" SDL_mixer code
 */
    void
SDLSoundAgent::musicFinished()
{
    try {
        if (ms_finished) {
            ms_finished->sendClone();
        }
        else {
            LOG_WARNING(ExInfo("NULL == ms_finished"));
        }
    }
    catch (std::exception &e) {
        LOG_WARNING(ExInfo("musicFinished error")
                .addInfo("what", e.what()));
    }
    catch (...) {
        LOG_ERROR(ExInfo("musicFinished error - unknown exception"));
    }
}




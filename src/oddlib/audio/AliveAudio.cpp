#include "oddlib/audio/AliveAudio.h"
#include "imgui/imgui.h"

void AliveAudio::CleanVoices()
{
    std::vector<AliveAudioVoice *> deadVoices;
    std::unique_lock<std::recursive_mutex> voiceLock(mVoiceMutex);
    for (auto& voice : m_Voices)
    {
        if (voice->b_Dead)
        {
            deadVoices.push_back(voice);
        }
    }

    for (auto& obj : deadVoices)
    {
        delete obj;

        m_Voices.erase(std::remove(m_Voices.begin(), m_Voices.end(), obj), m_Voices.end());
    }
}

void AliveAudio::AliveRenderAudio(f32 * AudioStream, int StreamLength)
{
    // Reset buffers
    for (int i = 0; i < StreamLength; ++i)
    {
        m_DryChannelBuffer[i] = 0;
        m_ReverbChannelBuffer[i] = 0;
    }

    {
        std::unique_lock<std::recursive_mutex> voiceLock(mVoiceMutex);
        const size_t voiceCount = m_Voices.size();

        AliveAudioVoice ** rawPointer = m_Voices.data(); // Real nice speed boost here.

        for (int i = 0; i < StreamLength; i += 2)
        {
            for (size_t v = 0; v < voiceCount; v++)
            {
                AliveAudioVoice * voice = rawPointer[v]; // Raw pointer skips all that vector bottleneck crap

                voice->f_TrackDelay--;

                if (voice->m_UsesNoteOffDelay)
                {
                    voice->f_NoteOffDelay--;
                }

                if (voice->m_UsesNoteOffDelay && voice->f_NoteOffDelay <= 0 && voice->b_NoteOn == true)
                {
                    voice->b_NoteOn = false;
                    //printf("off");
                }

                if (voice->b_Dead || voice->f_TrackDelay > 0)
                {
                    continue;
                }

                f32 centerPan = voice->m_Tone->f_Pan;
                f32 leftPan = 1.0f;
                f32 rightPan = 1.0f;

                if (centerPan > 0)
                {
                    leftPan = 1.0f - std::abs(centerPan);
                }

                if (centerPan < 0)
                {
                    rightPan = 1.0f - std::abs(centerPan);
                }

                // TODO FIX ME
                f32  s = voice->GetSample(Interpolation, false);
                f32 leftSample = (s * leftPan);
                f32 rightSample = (s * rightPan);

                if (voice->m_Tone->Reverbate || ForceReverb)
                {
                    m_ReverbChannelBuffer[i] += leftSample;
                    m_ReverbChannelBuffer[i + 1] += rightSample;
                }
                else
                {
                    m_DryChannelBuffer[i] += leftSample;
                    m_DryChannelBuffer[i + 1] += rightSample;
                }
            }

            mCurrentSampleIndex++;
        }
    }

    m_Reverb.setEffectMix(ReverbMix);

    // TODO: Find a better way of feeding the data in
    for (int i = 0; i < StreamLength; i += 2)
    {
        const f32 left = static_cast<f32>(m_Reverb.tick(m_ReverbChannelBuffer[i], m_ReverbChannelBuffer[i + 1], 0));
        const f32 right = static_cast<f32>(m_Reverb.lastOut(1));
        m_ReverbChannelBuffer[i] = left;
        m_ReverbChannelBuffer[i + 1] = right;
    }
   
    for (int i = 0; i < StreamLength; i += 2)
    {
        const f32 left = m_DryChannelBuffer[i] + m_ReverbChannelBuffer[i];
        const f32 right = m_DryChannelBuffer[i + 1] + m_ReverbChannelBuffer[i + 1];
        SDL_MixAudioFormat((u8 *)(AudioStream + i), (const u8*)&left, AUDIO_F32, sizeof(f32), SDL_MIX_MAXVOLUME);
        SDL_MixAudioFormat((u8 *)(AudioStream + i + 1), (const u8*)&right, AUDIO_F32, sizeof(f32), SDL_MIX_MAXVOLUME);
    }

    CleanVoices();
}


void AliveAudio::Play(f32* stream, u32 len)
{
    if (m_DryChannelBuffer.size() != len)
    {
        // Maybe it's ok to have some crackles when the buffer size changes.
        // (This allocates memory, which you should never do in audio thread.)
        m_DryChannelBuffer.resize(len);
        m_ReverbChannelBuffer.resize(len);
    }


    AliveRenderAudio(stream, len);
}

void AliveAudio::VabBrowserUi()
{
    if (m_Soundbank)
    {
        ImGui::Begin("VAB content");

        int i = 0;
        for (std::unique_ptr<AliveAudioProgram>& prog : m_Soundbank->m_Programs)
        {
            if (!prog->m_Tones.empty())
            {
                ImGui::TextUnformatted(("Program number: " + std::to_string(i)).c_str());
                int j = 0;
                for (std::unique_ptr<AliveAudioTone>& tone : prog->m_Tones)
                {
                    if (ImGui::Button(("     Tone number: " +
                        std::to_string(i) + "_"
                        + std::to_string(j++)
                        + " min key: " + std::to_string(tone->Min)
                        + " max key: " + std::to_string(tone->Max)
                        ).c_str()))
                    {
                        ClearAllTrackVoices(true);
                        NoteOn(i, tone->Min, 127, 0.0, 0.0);
                    }
                }
            }
            i++;
        }

        ImGui::End();
    }
}

/*
void AliveAudio::PlayOneShot(int program, int note, f32 volume, f32 pitch)
{
    std::lock_guard<std::recursive_mutex> lock(voiceListMutex);
    for (auto& tone : m_CurrentSoundbank->m_Programs[program]->m_Tones)
    {
        if (note >= tone->Min && note <= tone->Max)
        {
            AliveAudioVoice * voice = new AliveAudioVoice();
            voice->i_Note = note;
            voice->f_Velocity = volume;
            voice->m_Tone = tone.get();
            voice->f_Pitch = pitch;
            voice->m_DebugDisableResampling = DebugDisableVoiceResampling;
            m_Voices.push_back(voice);
        }
    }
}
*/

/*
void AliveAudio::PlayOneShot(std::string soundID)
{
    jsonxx::Array soundList = m_Config.get<jsonxx::Array>("sounds");

    for (size_t i = 0; i < soundList.size(); i++)
    {
        jsonxx::Object sndObj = soundList.get<jsonxx::Object>(static_cast<unsigned int>(i));
        if (sndObj.get<jsonxx::String>("id") == soundID)
        {
            f32 randA = 0;
            f32 randB = 0;

            if (sndObj.has<jsonxx::Array>("pitchrand"))
            {
                randA = (f32)sndObj.get<jsonxx::Array>("pitchrand").get<jsonxx::Number>(0);
                randB = (f32)sndObj.get<jsonxx::Array>("pitchrand").get<jsonxx::Number>(1);
            }

            PlayOneShot((int)sndObj.get<jsonxx::Number>("prog"), (int)sndObj.get<jsonxx::Number>("note"), 1.0f, RandFloat(randA, randB));
        }
    }
}
*/

void AliveAudio::NoteOn(int program, int note, char velocity, f64 trackDelay, f64 pitch, bool ignoreLoops)
{
    for (auto& tone : m_Soundbank->m_Programs[program]->m_Tones)
    {
        if (note >= tone->Min && note <= tone->Max)
        {
            AliveAudioVoice * voice = new AliveAudioVoice();
            voice->i_Note = note;
            voice->m_Tone = tone.get();
            voice->f_Pitch = pitch;
            voice->i_Program = program;
            voice->f_Velocity = velocity / 127.0f;
            voice->f_TrackDelay = trackDelay;
            voice->m_DebugDisableResampling = DebugDisableVoiceResampling;
            voice->mbIgnoreLoops = ignoreLoops;
            std::unique_lock<std::recursive_mutex> voiceLock(mVoiceMutex);
            m_Voices.push_back(voice);
        }
    }
}

void AliveAudio::NoteOff(int program, int note)
{
    std::unique_lock<std::recursive_mutex> voiceLock(mVoiceMutex);
    for (auto& voice : m_Voices)
    {
        if (voice->i_Note == note && voice->i_Program == program)
        {
            voice->b_NoteOn = false;
        }
    }
}

void AliveAudio::NoteOffDelay(int program, int note, f32 trackDelay)
{
    std::unique_lock<std::recursive_mutex> voiceLock(mVoiceMutex);
    for (auto& voice : m_Voices)
    {
        if (voice->i_Note == note && voice->i_Program == program && voice->f_TrackDelay < trackDelay && voice->f_NoteOffDelay <= 0)
        {
            voice->m_UsesNoteOffDelay = true;
            voice->f_NoteOffDelay = trackDelay;
        }
    }
}


void AliveAudio::ClearAllVoices(bool forceKill)
{
    std::vector<AliveAudioVoice *> deadVoices;

    std::unique_lock<std::recursive_mutex> voiceLock(mVoiceMutex);
    for (auto& voice : m_Voices)
    {
        if (forceKill)
        {
            deadVoices.push_back(voice);
        }
        else
        {
            voice->b_NoteOn = false; // Send a note off to all of the notes though.
            if (voice->f_SampleOffset == 0) // Let the voices that are CURRENTLY playing play.
            {
                deadVoices.push_back(voice);
            }
        }
    }


    for (auto& obj : deadVoices)
    {
        delete obj;

        m_Voices.erase(std::remove(m_Voices.begin(), m_Voices.end(), obj), m_Voices.end());
    }
}

void AliveAudio::ClearAllTrackVoices(bool forceKill)
{
    std::vector<AliveAudioVoice *> deadVoices;

    std::unique_lock<std::recursive_mutex> voiceLock(mVoiceMutex);
    for (auto& voice : m_Voices)
    {
        if (forceKill)
        {
            // Kill the voices no matter what. Cuts of any sounds = Ugly sound
            deadVoices.push_back(voice);
        }
        else
        {
            voice->b_NoteOn = false; // Send a note off to all of the notes though.
            if (voice->f_SampleOffset == 0) // Let the voices that are CURRENTLY playing play.
            {
                // TODO: This will make us delete them all after this loop as with force kill
                // probably we shouldn't be adding to the "delete" list here?
                deadVoices.push_back(voice);
            }
        }
    }

    for (auto& obj : deadVoices)
    {
        delete obj;

        m_Voices.erase(std::remove(m_Voices.begin(), m_Voices.end(), obj), m_Voices.end());
    }

}

void AliveAudio::SetSoundbank(std::unique_ptr<AliveAudioSoundbank> soundbank)
{
    ClearAllVoices(true);
    m_Soundbank = std::move(soundbank);
}

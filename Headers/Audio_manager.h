#pragma once

#include "Globals.h"

#include "AL/al.h"
#include "AL/alc.h"

class Audio_manager
{
public:
    struct Config
    {
        float master_volume = 1.0f;  
        int   max_sources = 32;  
        bool  distance_model = true;  
    };

public:
    explicit Audio_manager(const Config& cfg = Config())
        : m_config(cfg)
    {
        m_device = alcOpenDevice(nullptr);
        if (!m_device)
        {
            LOG_FATAL("Audio_manager - alcOpenDevice() failed!");
            throw std::runtime_error("Audio_manager - no OpenAL device");
        }

        m_context = alcCreateContext(m_device, nullptr);
        if (!m_context || alcMakeContextCurrent(m_context) == ALC_FALSE)
        {
            LOG_FATAL("Audio_manager - alcCreateContext() failed!");
            alcCloseDevice(m_device);
            throw std::runtime_error("Audio_manager - no OpenAL context");
        }

        if (cfg.distance_model)
            alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);

        alListenerf(AL_GAIN, cfg.master_volume);

        check_al_error("Audio_manager::ctor");
        LOG_INFO("Audio_manager - initialized (max_sources=%d)", cfg.max_sources);
    }

    ~Audio_manager()
    {
        clear_all_sources();
        clear_all_buffers();

        alcMakeContextCurrent(nullptr);
        if (m_context) alcDestroyContext(m_context);
        if (m_device)  alcCloseDevice(m_device);

        LOG_INFO("Audio_manager - destroyed");
    }

    Audio_manager(const Audio_manager&) = delete;
    Audio_manager& operator=(const Audio_manager&) = delete;

    void set_listener_position(const glm::vec3& pos)
    {
        alListener3f(AL_POSITION, pos.x, pos.y, pos.z);
    }

    void set_listener_velocity(const glm::vec3& vel)
    {
        alListener3f(AL_VELOCITY, vel.x, vel.y, vel.z);
    }

    void set_listener_orientation(const glm::vec3& forward, const glm::vec3& up)
    {
        float orientation[6] = {
            forward.x, forward.y, forward.z,
            up.x,      up.y,      up.z
        };
        alListenerfv(AL_ORIENTATION, orientation);
    }

    void update_listener(const glm::vec3& pos,
        const glm::vec3& forward,
        const glm::vec3& up,
        const glm::vec3& velocity = glm::vec3(0.0f))
    {
        set_listener_position(pos);
        set_listener_velocity(velocity);
        set_listener_orientation(forward, up);
    }

    void set_master_volume(float vol)
    {
        m_config.master_volume = glm::clamp(vol, 0.0f, 1.0f);
        alListenerf(AL_GAIN, m_config.master_volume);
    }

    float get_master_volume() const { return m_config.master_volume; }

    ALuint load_wav(const std::string& path)
    {
        auto it = m_path_to_buffer.find(path);
        if (it != m_path_to_buffer.end())
            return it->second;

        std::vector<char> pcm_data;
        ALenum  format;
        ALsizei sample_rate;

        if (!parse_wav(path, pcm_data, format, sample_rate))
        {
            LOG_ERROR("Audio_manager - load_wav failed: %s", path.c_str());
            return 0;
        }

        ALuint buffer;
        alGenBuffers(1, &buffer);
        alBufferData(buffer, format,
            pcm_data.data(),
            static_cast<ALsizei>(pcm_data.size()),
            sample_rate);

        if (check_al_error("load_wav"))
        {
            alDeleteBuffers(1, &buffer);
            return 0;
        }

        m_buffers.push_back(buffer);
        m_path_to_buffer[path] = buffer;

        LOG_INFO("Audio_manager - loaded WAV: %s (buffer=%u)", path.c_str(), buffer);
        return buffer;
    }

    ALuint create_buffer_from_pcm(const void* data, ALsizei size,
        ALenum format, ALsizei sample_rate)
    {
        ALuint buffer;
        alGenBuffers(1, &buffer);
        alBufferData(buffer, format, data, size, sample_rate);

        if (check_al_error("create_buffer_from_pcm"))
        {
            alDeleteBuffers(1, &buffer);
            return 0;
        }

        m_buffers.push_back(buffer);
        return buffer;
    }

    void delete_buffer(ALuint buffer)
    {
        if (!buffer) return;

        for (auto it = m_path_to_buffer.begin(); it != m_path_to_buffer.end(); )
        {
            if (it->second == buffer)
                it = m_path_to_buffer.erase(it);
            else
                ++it;
        }

        m_buffers.erase(std::remove(m_buffers.begin(), m_buffers.end(), buffer), m_buffers.end());
        alDeleteBuffers(1, &buffer);
    }

    void clear_all_buffers()
    {
        if (!m_buffers.empty())
            alDeleteBuffers(static_cast<ALsizei>(m_buffers.size()), m_buffers.data());
        m_buffers.clear();
        m_path_to_buffer.clear();
    }

    ALuint create_source()
    {
        if (static_cast<int>(m_sources.size()) >= m_config.max_sources)
        {
            LOG_WARNING("Audio_manager - max_sources (%d) reached!", m_config.max_sources);
            return 0;
        }

        ALuint src;
        alGenSources(1, &src);

        if (check_al_error("create_source"))
            return 0;

        m_sources.push_back(src);
        return src;
    }

    void set_source_buffer(ALuint source, ALuint buffer)
    {
        alSourcei(source, AL_BUFFER, static_cast<ALint>(buffer));
    }

    void delete_source(ALuint source)
    {
        stop(source);
        m_sources.erase(std::remove(m_sources.begin(), m_sources.end(), source), m_sources.end());
        alDeleteSources(1, &source);
    }

    void clear_all_sources()
    {
        for (ALuint src : m_sources)
        {
            alSourceStop(src);
            alDeleteSources(1, &src);
        }
        m_sources.clear();
    }

    void play(ALuint source) { alSourcePlay(source); }
    void pause(ALuint source) { alSourcePause(source); }
    void stop(ALuint source) { alSourceStop(source); }
    void rewind(ALuint source) { alSourceRewind(source); }

    bool is_playing(ALuint source) const
    {
        ALint state;
        alGetSourcei(source, AL_SOURCE_STATE, &state);
        return state == AL_PLAYING;
    }

    bool is_paused(ALuint source) const
    {
        ALint state;
        alGetSourcei(source, AL_SOURCE_STATE, &state);
        return state == AL_PAUSED;
    }

    void set_source_position(ALuint source, const glm::vec3& pos)
    {
        alSource3f(source, AL_POSITION, pos.x, pos.y, pos.z);
    }

    void set_source_velocity(ALuint source, const glm::vec3& vel)
    {
        alSource3f(source, AL_VELOCITY, vel.x, vel.y, vel.z);
    }

    void set_source_looping(ALuint source, bool loop)
    {
        alSourcei(source, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
    }

    void set_source_gain(ALuint source, float gain)
    {
        alSourcef(source, AL_GAIN, gain);
    }

    void set_source_pitch(ALuint source, float pitch)
    {
        alSourcef(source, AL_PITCH, pitch);
    }

    void set_source_relative(ALuint source, bool relative)
    {
        alSourcei(source, AL_SOURCE_RELATIVE, relative ? AL_TRUE : AL_FALSE);
    }

    void set_source_distance(ALuint source,
        float reference_distance = 1.0f,
        float max_distance = 100.0f,
        float rolloff_factor = 1.0f)
    {
        alSourcef(source, AL_REFERENCE_DISTANCE, reference_distance);
        alSourcef(source, AL_MAX_DISTANCE, max_distance);
        alSourcef(source, AL_ROLLOFF_FACTOR, rolloff_factor);
    }

    ALuint play_oneshot(const std::string& wav_path,
        float gain = 1.0f,
        float pitch = 1.0f)
    {
        ALuint buf = load_wav(wav_path);
        if (!buf) return 0;

        ALuint src = create_source();
        if (!src) return 0;

        set_source_buffer(src, buf);
        set_source_gain(src, gain);
        set_source_pitch(src, pitch);
        set_source_looping(src, false);
        play(src);

        return src;
    }

    ALuint play_oneshot_3d(const std::string& wav_path,
        const glm::vec3& position,
        float gain = 1.0f,
        float pitch = 1.0f,
        float ref_dist = 1.0f,
        float max_dist = 50.0f)
    {
        ALuint buf = load_wav(wav_path);
        if (!buf) return 0;

        ALuint src = create_source();
        if (!src) return 0;

        set_source_buffer(src, buf);
        set_source_position(src, position);
        set_source_gain(src, gain);
        set_source_pitch(src, pitch);
        set_source_looping(src, false);
        set_source_distance(src, ref_dist, max_dist);
        play(src);

        return src;
    }

    void cleanup_finished_sources()
    {
        for (auto it = m_sources.begin(); it != m_sources.end(); )
        {
            ALint state;
            alGetSourcei(*it, AL_SOURCE_STATE, &state);

            if (state == AL_STOPPED)
            {
                alDeleteSources(1, &(*it));
                it = m_sources.erase(it);
            }
            else
                ++it;
        }
    }

    static bool check_al_error(const char* context = "")
    {
        ALenum err = alGetError();
        if (err != AL_NO_ERROR)
        {
            LOG_ERROR("OpenAL error [%s]: 0x%x - %s",
                context, err, alGetString(err));
            return true;
        }
        return false;
    }


    int active_source_count() const { return static_cast<int>(m_sources.size()); }
    int loaded_buffer_count() const { return static_cast<int>(m_buffers.size()); }

private:
    Config   m_config;
    ALCdevice* m_device = nullptr;
    ALCcontext* m_context = nullptr;

    std::vector<ALuint>              m_buffers;
    std::vector<ALuint>              m_sources;
    std::unordered_map<std::string, ALuint> m_path_to_buffer;

    static bool parse_wav(const std::string& path,
        std::vector<char>& out_pcm,
        ALenum& out_format,
        ALsizei& out_rate)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open())
        {
            LOG_ERROR("Audio_manager::parse_wav - cannot open: %s", path.c_str());
            return false;
        }

        char riff_id[4];
        file.read(riff_id, 4);
        if (std::strncmp(riff_id, "RIFF", 4) != 0)
        {
            LOG_ERROR("Audio_manager::parse_wav - not a RIFF file: %s", path.c_str());
            return false;
        }

        uint32_t chunk_size; file.read(reinterpret_cast<char*>(&chunk_size), 4);

        char wave_id[4]; file.read(wave_id, 4);
        if (std::strncmp(wave_id, "WAVE", 4) != 0)
        {
            LOG_ERROR("Audio_manager::parse_wav - not a WAVE file: %s", path.c_str());
            return false;
        }

        char fmt_id[4]; file.read(fmt_id, 4);
        if (std::strncmp(fmt_id, "fmt ", 4) != 0)
        {
            LOG_ERROR("Audio_manager::parse_wav - missing fmt chunk: %s", path.c_str());
            return false;
        }

        uint32_t fmt_size;   file.read(reinterpret_cast<char*>(&fmt_size), 4);
        uint16_t audio_fmt;  file.read(reinterpret_cast<char*>(&audio_fmt), 2);
        uint16_t channels;   file.read(reinterpret_cast<char*>(&channels), 2);
        uint32_t sample_rate;file.read(reinterpret_cast<char*>(&sample_rate), 4);
        uint32_t byte_rate;  file.read(reinterpret_cast<char*>(&byte_rate), 4);
        uint16_t block_align;file.read(reinterpret_cast<char*>(&block_align), 2);
        uint16_t bits;       file.read(reinterpret_cast<char*>(&bits), 2);

        if (audio_fmt != 1)
        {
            LOG_ERROR("Audio_manager::parse_wav - only PCM WAV supported (got fmt %u): %s",
                audio_fmt, path.c_str());
            return false;
        }

        if (fmt_size > 16)
            file.seekg(fmt_size - 16, std::ios::cur);

        if (channels == 1 && bits == 8) out_format = AL_FORMAT_MONO8;
        else if (channels == 1 && bits == 16) out_format = AL_FORMAT_MONO16;
        else if (channels == 2 && bits == 8) out_format = AL_FORMAT_STEREO8;
        else if (channels == 2 && bits == 16) out_format = AL_FORMAT_STEREO16;
        else
        {
            LOG_ERROR("Audio_manager::parse_wav - unsupported format (ch=%u bits=%u): %s",
                channels, bits, path.c_str());
            return false;
        }

        out_rate = static_cast<ALsizei>(sample_rate);

        char chunk_id[4];
        while (file.read(chunk_id, 4))
        {
            uint32_t sz;
            file.read(reinterpret_cast<char*>(&sz), 4);

            if (std::strncmp(chunk_id, "data", 4) == 0)
            {
                out_pcm.resize(sz);
                file.read(out_pcm.data(), sz);
                return true;
            }
            file.seekg(sz, std::ios::cur);
        }

        LOG_ERROR("Audio_manager::parse_wav - no data chunk found: %s", path.c_str());
        return false;
    }
};
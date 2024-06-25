#include "playlist.hpp"
#include <music_player.hpp>

void data_callback(ma_device *pDevice, void *pOutput, const void *pInput,
                   ma_uint32 frameCount) {
  ma_decoder *pDecoder = (ma_decoder *)pDevice->pUserData;
  if (pDecoder == NULL) {
    return;
  }

  /* Reading PCM frames will loop based on what we specified when called
   * ma_data_source_set_looping(). */
  ma_data_source_read_pcm_frames(pDecoder, pOutput, frameCount, NULL);

  (void)pInput;
}

MusicPlayer::MusicPlayer() {}

MusicPlayer::~MusicPlayer() { auto _ = this->stop(); }

bool MusicPlayer::play(MusicEntry const &music_entry) {
  if (!this->stop()) {
    return false;
  }

  if (ma_decoder_init_file(music_entry.music_file_path.string().c_str(), NULL,
                           &m_ma_decoder) != MA_SUCCESS) {
    return false;
  }

  ma_device_config config = ma_device_config_init(ma_device_type_playback);

  config.pUserData = &m_ma_decoder;
  config.sampleRate = m_ma_decoder.outputSampleRate;
  config.playback.channels = m_ma_decoder.outputChannels;
  config.playback.format = m_ma_decoder.outputFormat;
  config.dataCallback = data_callback;
  config.noPreSilencedOutputBuffer = MA_TRUE; // optimize
  if (ma_device_init(NULL, &config, &m_ma_device) != MA_SUCCESS) {
    spdlog::error("Failed to initialize miniaudio device.");
    return false;
  }

  // get length information before ma_device_start( cause glitchy sounds and
  // invalid memory location read on MSVC (Release) with mp3. not sure why but
  // avoid.)
  float music_length_in_sec{};
  auto result =
      ma_data_source_get_length_in_seconds(&m_ma_decoder, &music_length_in_sec);
  ma_uint64 music_length_in_pcm_frames{};
  auto result2 = ma_data_source_get_length_in_pcm_frames(
      &m_ma_decoder, &music_length_in_pcm_frames);
  if (result != MA_SUCCESS || result2 != MA_SUCCESS) {
    spdlog::error("Failed to get music length.");
    return false;
  }

  // set looping flag
  if (ma_data_source_set_looping(&m_ma_decoder, true) != MA_SUCCESS) {
    spdlog::error("Failed to set looping flag.");
    return false;
  }

  // set loop points if available

  if (music_entry.loop_start_end_offsets.has_value()) {
    auto const loop_points = *music_entry.loop_start_end_offsets;

    spdlog::info("Loop information: start={}, end={}.", loop_points.first,
                 loop_points.second);

    if (ma_data_source_set_loop_point_in_pcm_frames(
            &m_ma_decoder, loop_points.first, loop_points.second) !=
        MA_SUCCESS) {
      spdlog::error("Failed to set loop points to data source.");
      return false;
    }
    // ma_sound_set_stop_time_in_pcm_frames(&sound, loop_points.second);
  }

  // set music start / end offsets

  auto play_start_offset = music_entry.start_end_offsets.first;
  auto play_end_offset = music_entry.start_end_offsets.second;

  if (play_start_offset == -1) {
    play_start_offset = 0;
  }

  if (play_end_offset == -1) {
    play_end_offset = music_length_in_pcm_frames;
  }

  if (play_end_offset <= play_start_offset) {

    spdlog::error("Invalid music start & end offsets(end offset <= start "
                  "offset) for music entry id {:#x}.",
                  music_entry.unique_music_id);
    return false;
  }

  spdlog::info("PCM frame range: start={}, end={}.", play_start_offset,
               play_end_offset);

  if (ma_data_source_set_range_in_pcm_frames(&m_ma_decoder, play_start_offset,
                                             play_end_offset) != MA_SUCCESS) {
    spdlog::error("Failed to set pcm frame range to data source.");
    return false;
  }

  if (ma_device_start(&m_ma_device) != MA_SUCCESS) {
    spdlog::error("Failed to start device.");
    return false;
  }

#ifdef __linux__
  spdlog::info(
      "\033[31;1;4mPlaying music: {}, length: {} seconds({} pcm "
      "frames)\nsample "
      "rate: {}, channels: {}, looping: {}, unique music id: {}\033[0m",
      music_entry.music_file_path.string().c_str(), music_length_in_sec,
      music_length_in_pcm_frames, config.sampleRate, config.playback.channels,
      [&]() {
        if (ma_data_source_is_looping(&m_ma_decoder) == MA_TRUE) {
          return "yes";
        }
        return "no";
      }(),
      music_entry.unique_music_id);
#endif

  spdlog::info(
      "Playing music: {}, length: {} seconds({} pcm frames)\nsample "
      "rate: {}, channels: {}, looping: {}, unique music id: {}",
      music_entry.music_file_path.string().c_str(), music_length_in_sec,
      music_length_in_pcm_frames, config.sampleRate, config.playback.channels,
      [&]() {
        if (ma_data_source_is_looping(&m_ma_decoder) == MA_TRUE) {
          return "yes";
        }
        return "no";
      }(),
      music_entry.unique_music_id);

  m_is_playing = true;
  return true;
}

bool MusicPlayer::stop() {
  if (m_is_playing) {
    if (ma_device_stop(&m_ma_device) != MA_SUCCESS) {
      return false;
    }

    ma_decoder_uninit(&m_ma_decoder);
    ma_device_uninit(&m_ma_device);

    m_is_playing = false;
  }
  return true;
}
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

bool MusicPlayer::play(music_entry const &music_entry) {
  if (!this->stop()) {
    return false;
  }

  auto const &file_path = std::get<1>(music_entry);
  if (ma_decoder_init_file(file_path.string().c_str(), NULL, &m_ma_decoder) !=
      MA_SUCCESS) {
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
    std::osyncstream(std::cout)
        << "[!] Failed to initialize miniaudio device.\n";
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
    std::osyncstream(std::cout) << "[!] Failed to get music length.\n ";
    return false;
  }

  // set looping flag
  if (ma_data_source_set_looping(&m_ma_decoder, true) != MA_SUCCESS) {
    std::osyncstream(std::cout) << "[!] Failed to set looping flag.\n";
    return false;
  }

  // set loop points if available
  if (std::get<3>(music_entry).has_value()) {
    auto const &loop_points = std::get<3>(music_entry).value();

    std::osyncstream(std::cout)
        << fmt::format("[+] Loop information: start={}, end={}.\n",
                       loop_points.first, loop_points.second);

    if (ma_data_source_set_loop_point_in_pcm_frames(
            &m_ma_decoder, loop_points.first, loop_points.second) !=
        MA_SUCCESS) {
      std::osyncstream(std::cout)
          << "[!] Failed to set loop points to data source.\n ";
      return false;
    }
    // ma_sound_set_stop_time_in_pcm_frames(&sound, loop_points.second);
  }

  // set music start / end offsets
  auto const &play_offsets = std::get<2>(music_entry);

  auto play_start_offset = play_offsets.first;
  auto play_end_offset = play_offsets.second;

  if (play_start_offset == -1) {
    play_start_offset = 0;
  }

  if (play_end_offset == -1) {
    play_end_offset = music_length_in_pcm_frames;
  }

  if (play_end_offset <= play_start_offset) {

    std::osyncstream(std::cout) << fmt ::format(
        "[!] Invalid music start&end offsets for music entry id {}\n",
        std::get<0>(music_entry));
    return false;
  }

  std::osyncstream(std::cout)
      << fmt::format("[+] PCM frame range: start={}, end={}.\n",
                     play_start_offset, play_end_offset);
  if (ma_data_source_set_range_in_pcm_frames(&m_ma_decoder, play_start_offset,
                                             play_end_offset) != MA_SUCCESS) {
    std::osyncstream(std::cout)
        << "[!] Failed to set pcm frame range to data source.\n";
    return false;
  }

  if (ma_device_start(&m_ma_device) != MA_SUCCESS) {
    std::osyncstream(std::cout) << "[!] Failed to start device.\n";
    return false;
  }

  std::osyncstream(std::cout) << fmt::format(
      "[+] Playing music: {}, length: {} seconds({} pcm frames)\nsample "
      "rate: {}, channels: {}, looping: {}, unique music id: {}\n",
      file_path.string().c_str(), music_length_in_sec,
      music_length_in_pcm_frames, config.sampleRate, config.playback.channels,
      [&]() {
        if (ma_data_source_is_looping(&m_ma_decoder) == MA_TRUE) {
          return "yes";
        }
        return "no";
      }(),
      std::get<0>(music_entry));
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
#pragma once

#include <vector>
#include <string>

namespace piano_recorder {

std::vector<std::string> get_midi_device_names(void);
std::vector<std::string> get_audio_device_names(void);

} // namespace piano_recorder

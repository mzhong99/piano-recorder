#include "midi_recorder.h"
#include "device_utils.h"
#include <spdlog/spdlog.h>

namespace piano_recorder {

bool MidiRecorder::start_recording(const std::filesystem::path &file_path)
{
    if (_running.load(std::memory_order_acquire)) {
        return true;
    }

    _output_path = file_path;
    _output_path.replace_extension(".mid");

    _running = true;
    return true;
}

void MidiRecorder::stop_recording()
{
    _running.store(false, std::memory_order_release);
}

} // namespace piano_recorder


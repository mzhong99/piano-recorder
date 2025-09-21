#include "instrument-recorder.h"
#include <sstream>
#include <spdlog/spdlog.h>

namespace piano_recorder {

InstrumentRecorder::InstrumentRecorder(
    std::weak_ptr<juce::AudioDeviceManager> device_manager_weak, const std::filesystem::path &path) :
    _output_base_path(path), _device_manager(device_manager_weak)
{
    std::shared_ptr<juce::AudioDeviceManager> device_manager = _device_manager.lock();
    if (device_manager == nullptr) {
        spdlog::error("device_manager ptr is invalid");
        return;
    }

    // Attach callback hooks - for midi, listen to everything, then enable specific devices when selected
    device_manager->addAudioCallback(&_audio_recorder);
    device_manager->addMidiInputDeviceCallback("", &_midi_recorder);
}

bool InstrumentRecorder::start_recording(void)
{
    if (_running) {
        spdlog::error("start_recording() attempted to be called while already running");
        return false;
    }

    std::shared_ptr<juce::AudioDeviceManager> device_manager = _device_manager.lock();
    if (device_manager == nullptr) {
        spdlog::error("device_manager ptr is invalid");
        return false;
    }

    if (_audio_source_name.empty()) {
        spdlog::info("Audio: recording skipped");
    } else {
        // Swap input device to selected device
        juce::AudioDeviceManager::AudioDeviceSetup setup = device_manager->getAudioDeviceSetup();
        setup.inputDeviceName = juce::String(_audio_source_name);

        auto err = device_manager->setAudioDeviceSetup(setup, true);
        if (err.isNotEmpty()) {
            spdlog::error(err.toStdString());
            return false;
        } else {
            spdlog::info("Audio ({}): device selected", _audio_source_name);
        }
    }

    if (_midi_source_name.empty()) {
        spdlog::info("MIDI: recording skipped");
    } else {
        auto midi_inputs = juce::MidiInput::getAvailableDevices();
        for (auto &device : midi_inputs) {
            if (device.name.containsIgnoreCase(_midi_source_name)) {
                device_manager->setMidiInputDeviceEnabled(device.identifier, true);
                spdlog::info("MIDI ({}): Started record", _midi_source_name);
            }
        }
    }

    if (!_audio_recorder.start_recording(_output_base_path)) {
        return false;
    }

    if (!_midi_recorder.start_recording(_output_base_path)) {
        return false;
    }

    _running = true;
    return true;
}

void InstrumentRecorder::stop_recording(void)
{
    if (!_running) {
        return;
    }

    std::shared_ptr<juce::AudioDeviceManager> device_manager = _device_manager.lock();
    if (device_manager == nullptr) {
        spdlog::warn("device_manager ptr is invalid - midi can't be detached!");
    } else if (!_midi_source_name.empty()) {
        auto midi_inputs = juce::MidiInput::getAvailableDevices();
        for (auto &device : midi_inputs) {
            if (device.name.containsIgnoreCase(_midi_source_name)) {
                device_manager->setMidiInputDeviceEnabled(device.identifier, false);
                spdlog::info("MIDI ({}): Stopped record", _midi_source_name);
            }
        }
    }

    _running = false;
}

bool InstrumentRecorder::set_output_path(const std::filesystem::path &path)
{
    if (_running) {
        return false;
    }

    _output_base_path = path;
    return true;
}

bool InstrumentRecorder::set_audio_source(const std::string &name)
{
    if (_running) {
        return false;
    }

    _audio_source_name = name;
    return true;
}

bool InstrumentRecorder::set_midi_source(const std::string &name)
{
    if (_running) {
        return false;
    }

    _midi_source_name = name;
    return true;
}

}  // namespace piano_recorder

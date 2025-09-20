#include "instrument-recorder.h"
#include <sstream>
#include <spdlog/spdlog.h>

namespace piano_recorder {

InstrumentRecorder::InstrumentRecorder(const std::filesystem::path &path) : _output_base_path(path)
{
    _device_manager.initialise(1, 2, nullptr, true);

    // Attach callback hooks - for midi, listen to everything, then enable specific devices when selected
    _device_manager.addAudioCallback(&_audio_recorder);
    _device_manager.addMidiInputDeviceCallback("", &_midi_recorder);
}

bool InstrumentRecorder::start_recording(void)
{
    if (_running) {
        return false;
    }

    if (_audio_source_name.empty()) {
        spdlog::info("Audio: recording skipped");
    } else {
        juce::AudioDeviceManager::AudioDeviceSetup setup = _device_manager.getAudioDeviceSetup();
        setup.inputDeviceName = juce::String(_audio_source_name);
        setup.outputDeviceName = "";  // or keep default if you want monitoring
        setup.useDefaultInputChannels = false;
        setup.inputChannels.setRange(0, 2, true);

        auto err = _device_manager.setAudioDeviceSetup(setup, true);
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
                _device_manager.setMidiInputDeviceEnabled(device.identifier, true);
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

    if (!_midi_source_name.empty()) {
        auto midi_inputs = juce::MidiInput::getAvailableDevices();
        for (auto &device : midi_inputs) {
            if (device.name.containsIgnoreCase(_midi_source_name)) {
                _device_manager.setMidiInputDeviceEnabled(device.identifier, false);
                spdlog::info("MIDI ({}): Stopped record", _midi_source_name);
            }
        }
    }

    _running = false;
}

std::vector<std::string> InstrumentRecorder::get_audio_device_names(void) {
    juce::OwnedArray<juce::AudioIODeviceType> types;
    _device_manager.createAudioDeviceTypes(types);
    std::vector<std::string> names;

    for (juce::AudioIODeviceType *type : types) {
        type->scanForDevices();
        juce::StringArray devices = type->getDeviceNames(true);
        for (juce::String &name : devices) {
            names.push_back(name.toStdString());
        }
    }

    return names;
}

std::vector<std::string> InstrumentRecorder::get_midi_device_names(void) const {
    auto midi_inputs = juce::MidiInput::getAvailableDevices();
    std::vector<std::string> names;

    for (auto &device : midi_inputs) {
        names.push_back(device.name.toStdString());
    }

    return names;
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

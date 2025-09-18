#include "device-utils.h"
#include <juce_audio_devices/juce_audio_devices.h>

namespace piano_recorder {

std::vector<std::string> get_audio_device_names(void) {
    juce::AudioDeviceManager audio_device_manager;
    juce::OwnedArray<juce::AudioIODeviceType> types;

    audio_device_manager.createAudioDeviceTypes(types);
    std::vector<std::string> names;

    for (auto *type : types) {
        type->scanForDevices();
        auto devices = type->getDeviceNames(true);
        for (auto &name : devices) {
            names.push_back(name.toStdString());
        }
    }

    return names;
}

std::vector<std::string> get_midi_device_names(void) {
    auto midi_inputs = juce::MidiInput::getAvailableDevices();
    std::vector<std::string> names;

    for (auto &device : midi_inputs) {
        names.push_back(device.name.toStdString());
    }

    return names;
}

}

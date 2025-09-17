#include <iostream>

#include <juce_audio_devices/juce_audio_devices.h>
#include <CLI/CLI.hpp>

int main(int argc, char **argv)
{
    CLI::App app{"Piano Recorder Core Application"};

    bool list_devices = false;
    bool fake_mode = false;
    std::string audio_device_name;
    std::string midi_device_name;

    app.add_flag("--list", list_devices, "List available audio and MIDI devices");
    app.add_flag("--fake", fake_mode, "Run system in a fake/mock mode");
    app.add_option("--audio", audio_device_name, "Select audio input device");
    app.add_option("--midi", midi_device_name, "Select MIDI input device");

    CLI11_PARSE(app, argc, argv);
    juce::AudioDeviceManager audio_device_manager;

    if (list_devices) {
        // List audio inputs
        std::cout << "Audio inputs:\n";
        juce::OwnedArray<juce::AudioIODeviceType> types;
        audio_device_manager.createAudioDeviceTypes(types);
        for (auto *type : types) {
            type->scanForDevices();
            auto devices = type->getDeviceNames(true);
            for (auto &dev : devices) {
                std::cout << " - " << dev << std::endl;
            }
        }

        // List MIDI devices
        std::cout << "MIDI inputs:\n";
        auto midi_inputs = juce::MidiInput::getAvailableDevices();
        for (auto &d : midi_inputs) {
            std::cout << " - " << d.name << std::endl;
        }
        return 0;
    }

    return EXIT_SUCCESS;
}

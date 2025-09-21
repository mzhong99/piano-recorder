#include "juce-audio-context.h"
#include <spdlog/spdlog.h>

namespace piano_recorder {

JuceAudioContext::JuceAudioContext(int num_inputs, int num_outputs) : _num_inputs(num_inputs), _num_outputs(num_outputs)
{
    _worker = std::thread(&JuceAudioContext::thread_func, this);
    while (!_running) {
        continue;
    }
    spdlog::info("JuceAudioContext thread started");

    refresh_device_names();
}

JuceAudioContext::~JuceAudioContext(void)
{
    call_on_message_thread([this](void) {
        _device_manager.reset();
    });

    juce::MessageManager *message_manager = juce::MessageManager::getInstance();
    message_manager->stopDispatchLoop();

    if (_worker.joinable()) {
        _worker.join();
    }
}

void JuceAudioContext::thread_func(void)
{
    _juce_system = std::make_unique<juce::ScopedJuceInitialiser_GUI>();
    juce::MessageManager *message_manager = juce::MessageManager::getInstance();

    // AudioDeviceManager has to be spawned inside of the message thread
    call_on_message_thread([this](void) {
        _device_manager = std::make_unique<juce::AudioDeviceManager>();
        _device_manager->initialise(_num_inputs, _num_outputs, nullptr, true);
        // refresh_device_names_internal();
    });

    _running = true;
    message_manager->runDispatchLoop();
}

void JuceAudioContext::refresh_device_names_internal(void)
{
    _cached_audio_sources.clear();
    _cached_midi_sources.clear();

    juce::OwnedArray<juce::AudioIODeviceType> types;
    _device_manager->createAudioDeviceTypes(types);
    for (juce::AudioIODeviceType *type : types) {
        type->scanForDevices();
        juce::StringArray devices = type->getDeviceNames(true);
        for (juce::String &name : devices) {
            _cached_audio_sources.push_back(name.toStdString());
        }
    }
    spdlog::info("Audio device list cache refreshed");

    auto midi_inputs = juce::MidiInput::getAvailableDevices();
    for (auto &device : midi_inputs) {
        _cached_midi_sources.push_back(device.name.toStdString());
    }
    spdlog::info("MIDI device list cache refreshed");
}

void JuceAudioContext::refresh_device_names(void)
{
    call_on_message_thread([this](void) {
        this->refresh_device_names_internal();
    });
}

}  // namespace piano_recorder

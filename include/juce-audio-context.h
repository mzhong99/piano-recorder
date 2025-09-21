#pragma once
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_events/juce_events.h>

#include <vector>
#include <string>
#include <memory>

#include <functional>
#include <optional>

#include <mutex>
#include <thread>

namespace piano_recorder {

class JuceAudioContext {
public:
    JuceAudioContext(int num_inputs, int num_outputs);
    ~JuceAudioContext(void);

    JuceAudioContext(const JuceAudioContext &other) = delete;
    JuceAudioContext(JuceAudioContext &&other) = delete;
    JuceAudioContext &operator=(const JuceAudioContext &other) = delete;
    JuceAudioContext &operator=(JuceAudioContext &&other) = delete;

    std::weak_ptr<juce::AudioDeviceManager> get_device_manager(void) { return _device_manager; }

    void refresh_device_names(void);
    std::vector<std::string> get_audio_device_names(void) const { return _cached_audio_sources; }
    std::vector<std::string> get_midi_device_names(void) const { return _cached_midi_sources; }

    template <typename Func>
    auto call_on_message_thread(Func &&func) -> decltype(func()) {
        using ResultType = decltype(func());
        if (juce::MessageManager::getInstance()->isThisTheMessageThread()) {
            return func();
        }

        juce::WaitableEvent finished;
        if constexpr (std::is_void_v<ResultType>) {
            juce::MessageManager::callAsync([&] {
                func();
                finished.signal();
            });

            finished.wait();
        } else {
            std::optional<ResultType> result;
            juce::MessageManager::callAsync([&] {
                result = func();
                finished.signal();
            });

            finished.wait();
            return *result;
        }
    }

    inline void call_on_message_thread(std::function<void()> func);

private:
    void thread_func(void);
    void refresh_device_names_internal(void);
    const int _num_inputs;
    const int _num_outputs;

    std::unique_ptr<juce::ScopedJuceInitialiser_GUI> _juce_system;
    std::shared_ptr<juce::AudioDeviceManager> _device_manager;

    std::vector<std::string> _cached_audio_sources;
    std::vector<std::string> _cached_midi_sources;

    std::atomic<bool> _running{false};
    std::thread _worker;
};

}  // namespace piano_recorder

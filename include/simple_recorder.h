#pragma once
#include <filesystem>
#include <atomic>

namespace piano_recorder {

class SimpleRecorder
{
protected:
    std::atomic<bool> _running{false};
    std::filesystem::path _output_path;

public:
    virtual ~SimpleRecorder() = default;
    virtual bool start_recording(const std::filesystem::path &file) = 0;
    virtual void stop_recording(void) = 0;

    bool is_recording() const { return _running; }
    std::filesystem::path get_output_path(void) { return _output_path; }
};

} // namespace piano_recorder


#pragma once
#include <filesystem>

namespace piano_recorder {

class SimpleRecorder
{
public:
    virtual ~SimpleRecorder() = default;

    virtual bool start_recording(const std::filesystem::path &file) = 0;
    virtual void stop_recording() = 0;
    virtual bool is_recording() const = 0;
    virtual std::filesystem::path get_output_file() const = 0;
};

} // namespace piano_recorder


#pragma once

#include <vector>
#include <string>
#include <filesystem>
#include <juce_core/juce_core.h>

namespace piano_recorder {

inline juce::File to_juce_file(const std::filesystem::path& path)
{
    return juce::File(path.string());
}

inline std::filesystem::path to_std_path(const juce::File& file)
{
    return std::filesystem::path(file.getFullPathName().toStdString());
}

} // namespace piano_recorder

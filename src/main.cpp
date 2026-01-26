#include "midi_device.hpp"
#include "midi_recorder.hpp"

#include <chrono>
#include <cxxopts.hpp>
#include <stdlib.h>
#include <signal.h>
#include <httplib.h>
#include <iostream>

#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>
#include <spdlog/version.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <string>
#include <thread>
#include <memory>
#include <vector>

#include <sago/platform_folders.h>
#include <alsa/asoundlib.h>

static std::atomic<bool> g_stop_requested = false;
const std::string kAppName = "piano-recorder";

extern "C" void signal_handler(int) {
    static bool s_stop_attempted = false;

    if (s_stop_attempted) {
        exit(EXIT_FAILURE);
    }

    g_stop_requested.store(true, std::memory_order_relaxed);
    s_stop_attempted = true;
}

spdlog::level::level_enum parse_log_level(const std::string &s) {
    static const std::unordered_map<std::string, spdlog::level::level_enum> map{
        {"trace", spdlog::level::trace},
        {"debug", spdlog::level::debug},
        {"info", spdlog::level::info},
        {"warn", spdlog::level::warn},
        {"warning", spdlog::level::warn},
        {"error", spdlog::level::err},
        {"critical", spdlog::level::critical},
        {"off", spdlog::level::off},
    };

    auto it = map.find(s);
    if (it == map.end()) {
        throw std::runtime_error("invalid log level: " + s);
    }
    return it->second;
}

std::filesystem::path get_user_dir(void) {
    return std::filesystem::path(sago::getDataHome()) / kAppName;
}

std::filesystem::path get_user_recording_dir(void) {
    return get_user_dir() / "recordings";
}

void init_logging(const std::string &log_level_str) {
    const auto level = parse_log_level(log_level_str);

    std::error_code ec;
    std::filesystem::path user_dir = get_user_dir();
    std::filesystem::create_directories(user_dir, ec);
    if (ec) {
        spdlog::error("Could not create {} - exiting (rc={})", user_dir.string(), ec.message());
        return;
    }

    std::string log_path = (user_dir / "piano-recorder.log").string();
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_path, false);
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

    console_sink->set_level(level);
    file_sink->set_level(level);

    std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};
    auto logger = std::make_shared<spdlog::logger>("default", sinks.begin(), sinks.end());
    logger->set_level(level);
    logger->flush_on(level);

    spdlog::set_default_logger(logger);
}

void list_devices(void) {
    auto devices = pr::midi::enumerate_midi_sources();
    for (const auto &device : devices) {
        std::cout << fmt::format("Device: {}", device.to_expanded_str()) << std::endl;
    }
}

void print_version(const std::string &prog_name) {
    spdlog::info("{}, using the following libs:", prog_name);
    spdlog::info("    spdlog: {}.{}.{}", SPDLOG_VER_MAJOR, SPDLOG_VER_MINOR, SPDLOG_VER_PATCH);
    spdlog::info("    httplib: {}", CPPHTTPLIB_VERSION);
    spdlog::info("    alsa: {}", SND_LIB_VERSION_STR);
}

int run_application(const cxxopts::ParseResult &args) {
    pr::midi::MidiPortHandle handle;
    std::string output_path;

    if (args.count("output")) {
        output_path = args["output"].as<std::string>();
    } else {
        std::error_code ec;
        std::filesystem::path recording_dir = get_user_recording_dir();
        std::filesystem::create_directories(recording_dir, ec);
        if (ec) {
            spdlog::error("Could not create {} - exiting (rc={})", recording_dir.string(), ec.message());
            return EXIT_FAILURE;
        }

        output_path = (recording_dir / "default.mid").string();
    }
    spdlog::info("Use output path: {}", output_path);

    if (args.count("port")) {
        std::string chosen_port = args["port"].as<std::string>();

        auto devices = pr::midi::enumerate_midi_sources();
        for (const auto &device : devices) {
            std::string possible_port = fmt::format("{}:{}", device.client_id, device.port_id);
            if (chosen_port == possible_port) {
                spdlog::info("Selected {}", device.to_expanded_str());
                handle = device;
                break;
            }
        }
    }

    pr::midi::MidiRecorder recorder{handle, output_path};
    recorder.start();

    while (!g_stop_requested.load(std::memory_order_relaxed)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    recorder.stop();
    spdlog::info("Recording finished.");

    return EXIT_SUCCESS;
}

int main(int argc, char **argv) {
    // clang-format off
    cxxopts::Options options("piano-recorder", "MIDI recorder prototype");
    options.add_options()
        ("l,list", "List ALSA sequencer clients/ports")
        ("L,log-level", "trace|debug|info|warn|error|critical|off", cxxopts::value<std::string>()->default_value("info"))
        ("V,version", "Print library versions")
        ("p,port", "Select source port as client:port (e.g., 24:0)", cxxopts::value<std::string>())
        ("o,output", "Select path to output .mid file", cxxopts::value<std::string>())
        ("h,help", "Print help");
    // clang-format on

    cxxopts::ParseResult result = options.parse(argc, argv);
    if (result["help"].as<bool>()) {
        std::cout << options.help() << "\n";
        return 0;
    }

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    init_logging(result["log-level"].as<std::string>());

    if (result["list"].as<bool>()) {
        list_devices();
    } else if (result["version"].as<bool>()) {
        print_version(argv[0]);
    } else {
        return run_application(result);
    }

    spdlog::shutdown();
    return 0;
}

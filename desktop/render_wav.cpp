// Renders a single virtual CD40106 inverter to a WAV file plus a CSV trace of
// capacitor voltage and output state for inspection.

#include "SchmittInverter.h"
#include "WavWriter.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {

struct Options {
    lab::InverterParams params;
    float seconds = 10.0f;
    uint32_t sampleRate = 48000;
    float level = 0.5f;
    float csvSeconds = 0.05f;
    std::string outPrefix = "out/inverter";
};

void printUsage()
{
    std::printf(
        "Usage: render_wav [options]\n"
        "  --r OHMS          feedback resistance (default 100000)\n"
        "  --c FARADS        capacitance (default 10e-9)\n"
        "  --vlow V          lower threshold, 0-1 of supply (default 0.39)\n"
        "  --vhigh V         upper threshold, 0-1 of supply (default 0.59)\n"
        "  --seconds S       audio length (default 10)\n"
        "  --rate HZ         sample rate (default 48000)\n"
        "  --level L         output amplitude, 0-1 (default 0.5)\n"
        "  --csv-seconds S   length of the CSV trace (default 0.05)\n"
        "  --out PREFIX      writes PREFIX.wav and PREFIX.csv (default out/inverter)\n");
}

bool parseArgs(int argc, char** argv, Options& o)
{
    for (int i = 1; i < argc; ++i) {
        const char* flag = argv[i];
        if (std::strcmp(flag, "--help") == 0 || std::strcmp(flag, "-h") == 0) {
            printUsage();
            std::exit(0);
        }
        if (i + 1 >= argc) {
            std::fprintf(stderr, "missing value for %s\n", flag);
            return false;
        }
        const char* value = argv[++i];
        if (std::strcmp(flag, "--r") == 0) o.params.resistanceOhms = std::strtof(value, nullptr);
        else if (std::strcmp(flag, "--c") == 0) o.params.capacitanceFarads = std::strtof(value, nullptr);
        else if (std::strcmp(flag, "--vlow") == 0) o.params.thresholdLow = std::strtof(value, nullptr);
        else if (std::strcmp(flag, "--vhigh") == 0) o.params.thresholdHigh = std::strtof(value, nullptr);
        else if (std::strcmp(flag, "--seconds") == 0) o.seconds = std::strtof(value, nullptr);
        else if (std::strcmp(flag, "--rate") == 0) o.sampleRate = static_cast<uint32_t>(std::strtoul(value, nullptr, 10));
        else if (std::strcmp(flag, "--level") == 0) o.level = std::strtof(value, nullptr);
        else if (std::strcmp(flag, "--csv-seconds") == 0) o.csvSeconds = std::strtof(value, nullptr);
        else if (std::strcmp(flag, "--out") == 0) o.outPrefix = value;
        else {
            std::fprintf(stderr, "unknown option %s\n", flag);
            return false;
        }
    }
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    Options o;
    if (!parseArgs(argc, argv, o)) {
        printUsage();
        return 2;
    }
    if (o.sampleRate < 8000 || o.sampleRate > 384000 || !(o.seconds > 0.0f)
        || !(o.level >= 0.0f && o.level <= 1.0f)) {
        std::fprintf(stderr, "rate must be 8000-384000, seconds > 0, level 0-1\n");
        return 2;
    }

    lab::SchmittInverter inverter;
    const lab::ParamError error = inverter.setParams(o.params);
    if (error != lab::ParamError::None) {
        std::fprintf(stderr, "invalid parameters: %s\n", lab::describe(error));
        return 2;
    }

    const size_t frames = static_cast<size_t>(o.seconds * static_cast<float>(o.sampleRate));
    const size_t csvFrames = static_cast<size_t>(o.csvSeconds * static_cast<float>(o.sampleRate));
    const float dt = 1.0f / static_cast<float>(o.sampleRate);

    const std::string wavPath = o.outPrefix + ".wav";
    const std::string csvPath = o.outPrefix + ".csv";
    const std::filesystem::path parent = std::filesystem::path(csvPath).parent_path();
    if (!parent.empty()) {
        std::error_code ignored;
        std::filesystem::create_directories(parent, ignored);
    }
    std::ofstream csv(csvPath);
    if (!csv) {
        std::fprintf(stderr, "cannot write %s\n", csvPath.c_str());
        return 1;
    }
    csv << "time_s,capacitor_v,output_high\n";

    std::vector<float> audio(frames);
    size_t edges = 0;
    for (size_t n = 0; n < frames; ++n) {
        if (n < csvFrames) {
            csv << static_cast<double>(n) / o.sampleRate << ',' << inverter.capacitorVoltage()
                << ',' << (inverter.outputHigh() ? 1 : 0) << '\n';
        }
        // Naive point sampling of the output state: aliases (see README).
        audio[n] = inverter.outputHigh() ? o.level : -o.level;
        edges += static_cast<size_t>(inverter.advance(dt));
    }

    if (!lab::writeWavMono16(wavPath, audio, o.sampleRate)) {
        std::fprintf(stderr, "cannot write %s\n", wavPath.c_str());
        return 1;
    }

    std::printf("ideal frequency %.2f Hz, measured %.2f Hz\n",
                1.0f / lab::idealPeriodSeconds(inverter.params()),
                static_cast<double>(edges) / 2.0 / o.seconds);
    std::printf("wrote %s and %s\n", wavPath.c_str(), csvPath.c_str());
    return 0;
}

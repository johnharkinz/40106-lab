#include "WavWriter.h"

#include <algorithm>
#include <cmath>
#include <fstream>

namespace lab {

namespace {

void put16(std::ofstream& out, uint16_t value)
{
    const char bytes[2] = {static_cast<char>(value & 0xff), static_cast<char>(value >> 8)};
    out.write(bytes, 2);
}

void put32(std::ofstream& out, uint32_t value)
{
    put16(out, static_cast<uint16_t>(value & 0xffff));
    put16(out, static_cast<uint16_t>(value >> 16));
}

} // namespace

bool writeWavMono16(const std::string& path, const std::vector<float>& samples,
                    uint32_t sampleRate)
{
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        return false;
    }

    const uint16_t channels = 1;
    const uint16_t bitsPerSample = 16;
    const uint16_t blockAlign = channels * bitsPerSample / 8;
    const uint32_t dataBytes = static_cast<uint32_t>(samples.size()) * blockAlign;

    out.write("RIFF", 4);
    put32(out, 36 + dataBytes);
    out.write("WAVE", 4);
    out.write("fmt ", 4);
    put32(out, 16);
    put16(out, 1); // PCM
    put16(out, channels);
    put32(out, sampleRate);
    put32(out, sampleRate * blockAlign);
    put16(out, blockAlign);
    put16(out, bitsPerSample);
    out.write("data", 4);
    put32(out, dataBytes);

    for (float sample : samples) {
        const float clamped = std::clamp(sample, -1.0f, 1.0f);
        put16(out, static_cast<uint16_t>(static_cast<int16_t>(std::lround(clamped * 32767.0f))));
    }

    return static_cast<bool>(out);
}

} // namespace lab

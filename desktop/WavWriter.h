#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace lab {

// Writes mono 16-bit PCM. Samples are clamped to [-1, 1].
bool writeWavMono16(const std::string& path, const std::vector<float>& samples,
                    uint32_t sampleRate);

} // namespace lab

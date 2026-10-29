#include "Check.h"
#include "SchmittInverter.h"

#include <cmath>
#include <limits>

using lab::InverterParams;
using lab::ParamError;
using lab::SchmittInverter;

namespace {

constexpr float kSampleRate = 48000.0f;
constexpr float kDt = 1.0f / kSampleRate;

// Counts output transitions over the given duration, after a warm-up that
// skips the longer power-on half-cycle.
int countEdges(SchmittInverter& inverter, float seconds)
{
    inverter.reset();
    for (int i = 0; i < 4800; ++i) {
        inverter.advance(kDt);
    }
    int edges = 0;
    const int samples = static_cast<int>(seconds * kSampleRate);
    for (int i = 0; i < samples; ++i) {
        edges += inverter.advance(kDt);
    }
    return edges;
}

float measuredFrequency(const InverterParams& params, float seconds = 2.0f)
{
    SchmittInverter inverter;
    inverter.setParams(params);
    return static_cast<float>(countEdges(inverter, seconds)) / 2.0f / seconds;
}

} // namespace

TEST("defaults are valid")
{
    CHECK(lab::validate(InverterParams{}) == ParamError::None);
}

TEST("rejects thresholds that are not ordered")
{
    InverterParams p;
    p.thresholdLow = 0.6f;
    p.thresholdHigh = 0.4f;
    CHECK(lab::validate(p) == ParamError::ThresholdsNotOrdered);
    p.thresholdLow = p.thresholdHigh = 0.5f;
    CHECK(lab::validate(p) == ParamError::ThresholdsNotOrdered);
}

TEST("rejects thresholds at or outside the rails")
{
    InverterParams p;
    p.thresholdLow = 0.0f;
    CHECK(lab::validate(p) == ParamError::ThresholdOutOfRange);
    p.thresholdLow = 0.39f;
    p.thresholdHigh = 1.0f;
    CHECK(lab::validate(p) == ParamError::ThresholdOutOfRange);
}

TEST("rejects zero, negative and non-finite component values")
{
    InverterParams p;
    p.resistanceOhms = 0.0f;
    CHECK(lab::validate(p) == ParamError::NonPositiveResistance);
    p.resistanceOhms = -1.0f;
    CHECK(lab::validate(p) == ParamError::NonPositiveResistance);
    p = InverterParams{};
    p.capacitanceFarads = 0.0f;
    CHECK(lab::validate(p) == ParamError::NonPositiveCapacitance);
    p = InverterParams{};
    p.resistanceOhms = std::numeric_limits<float>::quiet_NaN();
    CHECK(lab::validate(p) == ParamError::NonFinite);
    p = InverterParams{};
    p.capacitanceFarads = std::numeric_limits<float>::infinity();
    CHECK(lab::validate(p) == ParamError::NonFinite);
}

TEST("rejects time constants too short to simulate safely")
{
    InverterParams p;
    p.resistanceOhms = 100.0f;
    p.capacitanceFarads = 1e-9f;
    CHECK(lab::validate(p) == ParamError::TimeConstantTooShort);
}

TEST("invalid parameters leave the previous ones in place")
{
    SchmittInverter inverter;
    InverterParams good;
    good.resistanceOhms = 47e3f;
    CHECK(inverter.setParams(good) == ParamError::None);
    InverterParams bad = good;
    bad.resistanceOhms = 0.0f;
    CHECK(inverter.setParams(bad) == ParamError::NonPositiveResistance);
    CHECK(inverter.params().resistanceOhms == 47e3f);
}

TEST("powers on discharged with output high and starts oscillating")
{
    SchmittInverter inverter;
    CHECK(inverter.capacitorVoltage() == 0.0f);
    CHECK(inverter.outputHigh());
    int edges = 0;
    for (int i = 0; i < 480; ++i) {
        edges += inverter.advance(kDt);
    }
    CHECK(edges > 2);
}

TEST("first edge happens when charging from zero reaches the upper threshold")
{
    SchmittInverter inverter;
    const InverterParams& p = inverter.params();
    const float rc = p.resistanceOhms * p.capacitanceFarads;
    const float expected = rc * std::log(1.0f / (1.0f - p.thresholdHigh));
    CHECK(inverter.advance(expected * 0.999f) == 0);
    CHECK(inverter.advance(expected * 0.002f) == 1);
    CHECK(!inverter.outputHigh());
}

TEST("capacitor stays between the thresholds once running")
{
    SchmittInverter inverter;
    const InverterParams& p = inverter.params();
    for (int i = 0; i < 100; ++i) {
        inverter.advance(kDt);
    }
    float lowest = 1.0f;
    float highest = 0.0f;
    for (int i = 0; i < 48000; ++i) {
        inverter.advance(kDt);
        lowest = std::fmin(lowest, inverter.capacitorVoltage());
        highest = std::fmax(highest, inverter.capacitorVoltage());
    }
    CHECK(lowest >= p.thresholdLow - 1e-6f);
    CHECK(highest <= p.thresholdHigh + 1e-6f);
    CHECK(lowest < p.thresholdLow + 0.01f);
    CHECK(highest > p.thresholdHigh - 0.01f);
}

TEST("switches exactly at the threshold")
{
    SchmittInverter inverter;
    const InverterParams& p = inverter.params();
    const float rc = p.resistanceOhms * p.capacitanceFarads;
    inverter.advance(rc * std::log(1.0f / (1.0f - p.thresholdHigh)) * 1.0001f);
    CHECK(!inverter.outputHigh());
    CHECK_NEAR(inverter.capacitorVoltage(), p.thresholdHigh, 1e-4);
}

TEST("frequency matches the ideal RC period")
{
    const InverterParams p;
    const float ideal = 1.0f / lab::idealPeriodSeconds(p);
    CHECK_NEAR(measuredFrequency(p), ideal, ideal * 0.005);
}

TEST("frequency is inversely proportional to R and to C")
{
    const InverterParams base;
    const float f = measuredFrequency(base);
    InverterParams doubleR = base;
    doubleR.resistanceOhms *= 2.0f;
    CHECK_NEAR(measuredFrequency(doubleR), f / 2.0f, f * 0.005);
    InverterParams doubleC = base;
    doubleC.capacitanceFarads *= 2.0f;
    CHECK_NEAR(measuredFrequency(doubleC), f / 2.0f, f * 0.005);
}

TEST("wider hysteresis lowers the frequency")
{
    InverterParams narrow;
    narrow.thresholdLow = 0.45f;
    narrow.thresholdHigh = 0.55f;
    InverterParams wide;
    wide.thresholdLow = 0.3f;
    wide.thresholdHigh = 0.7f;
    CHECK(measuredFrequency(wide) < measuredFrequency(narrow));
}

TEST("result does not depend on step size")
{
    SchmittInverter coarse;
    SchmittInverter fine;
    for (int i = 0; i < 4800; ++i) {
        coarse.advance(kDt);
        for (int j = 0; j < 8; ++j) {
            fine.advance(kDt / 8.0f);
        }
    }
    CHECK(coarse.outputHigh() == fine.outputHigh());
    CHECK_NEAR(coarse.capacitorVoltage(), fine.capacitorVoltage(), 1e-3);
}

TEST("runs are deterministic")
{
    SchmittInverter a;
    SchmittInverter b;
    for (int i = 0; i < 48000; ++i) {
        CHECK(a.advance(kDt) == b.advance(kDt));
    }
    CHECK(a.capacitorVoltage() == b.capacitorVoltage());
}

TEST("stays finite and bounded over a long run")
{
    SchmittInverter inverter;
    bool ok = true;
    for (int i = 0; i < 48000 * 60; ++i) {
        inverter.advance(kDt);
        const float v = inverter.capacitorVoltage();
        ok = ok && std::isfinite(v) && v >= 0.0f && v <= 1.0f;
    }
    CHECK(ok);
}

TEST("extreme rates are bounded per step and stay finite")
{
    SchmittInverter inverter;
    InverterParams p;
    p.resistanceOhms = 1000.0f;
    p.capacitanceFarads = 1e-9f; // RC = 1 us, about 1.2 MHz
    CHECK(inverter.setParams(p) == ParamError::None);
    for (int i = 0; i < 4800; ++i) {
        const int edges = inverter.advance(kDt);
        CHECK(edges <= lab::kMaxEdgesPerAdvance);
        CHECK(std::isfinite(inverter.capacitorVoltage()));
    }
}

TEST("very slow rates still advance")
{
    SchmittInverter inverter;
    InverterParams p;
    p.resistanceOhms = 10e6f;
    p.capacitanceFarads = 10e-6f; // RC = 100 s
    CHECK(inverter.setParams(p) == ParamError::None);
    for (int i = 0; i < 48000; ++i) {
        inverter.advance(kDt);
    }
    CHECK(inverter.capacitorVoltage() > 0.0f);
    CHECK(inverter.outputHigh());
}

TEST("moving a threshold past the capacitor switches on the next step")
{
    SchmittInverter inverter;
    // Charge to just below the default upper threshold.
    const InverterParams& p = inverter.params();
    const float rc = p.resistanceOhms * p.capacitanceFarads;
    inverter.advance(rc * std::log(1.0f / (1.0f - p.thresholdHigh)) * 0.95f);
    CHECK(inverter.outputHigh());
    InverterParams lowered = p;
    lowered.thresholdHigh = 0.45f;
    CHECK(inverter.setParams(lowered) == ParamError::None);
    CHECK(inverter.advance(kDt) >= 1);
    CHECK(!inverter.outputHigh());
}

int main()
{
    return check::runAll();
}

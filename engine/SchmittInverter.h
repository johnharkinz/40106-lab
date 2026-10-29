#pragma once

#include <cstdint>

namespace lab {

// Voltages are normalised to the supply: 0 is VSS, 1 is VDD.
struct InverterParams {
    float resistanceOhms = 100e3f;
    float capacitanceFarads = 10e-9f;
    // CD40106 typical thresholds as a fraction of VDD (TI / onsemi datasheets).
    float thresholdLow = 0.39f;
    float thresholdHigh = 0.59f;
};

enum class ParamError : uint8_t {
    None,
    NonFinite,
    NonPositiveResistance,
    NonPositiveCapacitance,
    TimeConstantTooShort,
    ThresholdOutOfRange,
    ThresholdsNotOrdered,
};

// Shortest RC accepted. Keeps the oscillator well under 1 MHz so a single
// sample never contains an unbounded number of edges.
constexpr float kMinTimeConstantSeconds = 1e-6f;

// Upper bound on output transitions processed in one advance() call. Hit only
// at rates far above audio; the remaining time then runs without switching.
constexpr int kMaxEdgesPerAdvance = 64;

ParamError validate(const InverterParams& params);
const char* describe(ParamError error);

// Ideal period of an isolated inverter: RC * (ln((1-Vl)/(1-Vh)) + ln(Vh/Vl)).
float idealPeriodSeconds(const InverterParams& params);

// One virtual CD40106 Schmitt-trigger stage driving its own input capacitor
// through a feedback resistance. Integrates the RC node exactly between
// output transitions and finds each threshold crossing analytically.
class SchmittInverter {
public:
    SchmittInverter();

    // Rejects invalid parameters, keeping the previous ones in that case.
    ParamError setParams(const InverterParams& params);
    const InverterParams& params() const { return params_; }

    // Power-on state: capacitor discharged, so the input is below the lower
    // threshold and the output is HIGH. The first half-cycle is therefore
    // longer than later ones (it charges from 0 rather than from Vlow).
    void reset();

    // Advances by dt seconds. Returns the number of output transitions.
    int advance(float dtSeconds);

    float capacitorVoltage() const { return capacitorVoltage_; }
    bool outputHigh() const { return outputHigh_; }

private:
    InverterParams params_;
    float timeConstant_ = 0.0f;
    float capacitorVoltage_ = 0.0f;
    bool outputHigh_ = true;
};

} // namespace lab

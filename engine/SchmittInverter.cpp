#include "SchmittInverter.h"

#include <cmath>

namespace lab {

ParamError validate(const InverterParams& p)
{
    if (!std::isfinite(p.resistanceOhms) || !std::isfinite(p.capacitanceFarads)
        || !std::isfinite(p.thresholdLow) || !std::isfinite(p.thresholdHigh)) {
        return ParamError::NonFinite;
    }
    if (p.resistanceOhms <= 0.0f) {
        return ParamError::NonPositiveResistance;
    }
    if (p.capacitanceFarads <= 0.0f) {
        return ParamError::NonPositiveCapacitance;
    }
    if (p.resistanceOhms * p.capacitanceFarads < kMinTimeConstantSeconds) {
        return ParamError::TimeConstantTooShort;
    }
    if (p.thresholdLow <= 0.0f || p.thresholdHigh >= 1.0f) {
        return ParamError::ThresholdOutOfRange;
    }
    if (p.thresholdLow >= p.thresholdHigh) {
        return ParamError::ThresholdsNotOrdered;
    }
    return ParamError::None;
}

const char* describe(ParamError error)
{
    switch (error) {
    case ParamError::None: return "ok";
    case ParamError::NonFinite: return "parameters must be finite";
    case ParamError::NonPositiveResistance: return "resistance must be > 0";
    case ParamError::NonPositiveCapacitance: return "capacitance must be > 0";
    case ParamError::TimeConstantTooShort: return "R*C must be at least 1 us";
    case ParamError::ThresholdOutOfRange: return "thresholds must lie strictly between 0 and 1";
    case ParamError::ThresholdsNotOrdered: return "threshold low must be below threshold high";
    }
    return "unknown error";
}

float idealPeriodSeconds(const InverterParams& p)
{
    const float rc = p.resistanceOhms * p.capacitanceFarads;
    const float charge = std::log((1.0f - p.thresholdLow) / (1.0f - p.thresholdHigh));
    const float discharge = std::log(p.thresholdHigh / p.thresholdLow);
    return rc * (charge + discharge);
}

SchmittInverter::SchmittInverter()
{
    setParams(InverterParams{});
    reset();
}

ParamError SchmittInverter::setParams(const InverterParams& params)
{
    const ParamError error = validate(params);
    if (error == ParamError::None) {
        params_ = params;
        timeConstant_ = params.resistanceOhms * params.capacitanceFarads;
    }
    return error;
}

void SchmittInverter::reset()
{
    capacitorVoltage_ = 0.0f;
    outputHigh_ = true;
}

int SchmittInverter::advance(float dtSeconds)
{
    float remaining = dtSeconds;
    int edges = 0;

    while (remaining > 0.0f) {
        // Vc(t) = target + (Vc(0) - target) * exp(-t / RC)
        const float target = outputHigh_ ? 1.0f : 0.0f;
        const float threshold = outputHigh_ ? params_.thresholdHigh : params_.thresholdLow;
        const float distance = capacitorVoltage_ - target;

        // Time to reach the switching threshold from the current voltage.
        // Non-positive when already at or past it (e.g. thresholds changed).
        const float crossing = timeConstant_ * std::log(distance / (threshold - target));

        if (crossing > remaining || edges == kMaxEdgesPerAdvance) {
            capacitorVoltage_ = target + distance * std::exp(-remaining / timeConstant_);
            break;
        }

        const float elapsed = crossing > 0.0f ? crossing : 0.0f;
        capacitorVoltage_ = threshold;
        outputHigh_ = !outputHigh_;
        remaining -= elapsed;
        ++edges;
    }

    return edges;
}

} // namespace lab

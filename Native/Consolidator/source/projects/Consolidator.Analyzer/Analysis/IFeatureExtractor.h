#pragma once

#include "SignalAnalysisWindow.h"

#include <string_view>

class IFeatureExtractor {
public:
    virtual ~IFeatureExtractor() = default;
    virtual std::string_view Id() const = 0;
    virtual double Extract(const SignalAnalysisWindow& window) = 0;
    virtual void Reset() {}
};

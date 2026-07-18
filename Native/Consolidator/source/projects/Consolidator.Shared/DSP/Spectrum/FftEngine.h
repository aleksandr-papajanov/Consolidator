#pragma once

#include "FftSettings.h"

#include <cmath>
#include <complex>
#include <cstddef>
#include <numbers>
#include <span>
#include <stdexcept>
#include <vector>

namespace consolidator::dsp {

class FftEngine final {
public:
    explicit FftEngine(FftSettings settings = {})
        : settings(settings) {
        if (!settings.IsValid()) {
            throw std::invalid_argument("FFT size must be a power of two");
        }
    }

    std::size_t Size() const {
        return settings.size;
    }

    void Forward(
        std::span<const double> input,
        std::span<std::complex<double>> output
    ) const {
        if (input.size() != settings.size || output.size() != settings.size) {
            throw std::invalid_argument("FFT input and output sizes must match the engine size");
        }

        for (std::size_t index = 0; index < settings.size; ++index) {
            output[index] = { input[index], 0.0 };
        }
        Transform(output, false);
    }

    std::vector<std::complex<double>> Forward(std::span<const double> input) const {
        std::vector<std::complex<double>> output(settings.size);
        Forward(input, output);
        return output;
    }

    void Inverse(
        std::span<const std::complex<double>> input,
        std::span<std::complex<double>> output
    ) const {
        if (input.size() != settings.size || output.size() != settings.size) {
            throw std::invalid_argument("FFT input and output sizes must match the engine size");
        }

        for (std::size_t index = 0; index < settings.size; ++index) {
            output[index] = input[index];
        }
        Transform(output, true);
    }

private:
    void Transform(std::span<std::complex<double>> values, bool inverse) const {
        BitReverse(values);

        for (std::size_t width = 2; width <= settings.size; width *= 2) {
            const double angle = (inverse ? 2.0 : -2.0) * std::numbers::pi / static_cast<double>(width);
            const std::complex<double> step{ std::cos(angle), std::sin(angle) };

            for (std::size_t offset = 0; offset < settings.size; offset += width) {
                std::complex<double> twiddle{ 1.0, 0.0 };
                const std::size_t halfWidth = width / 2;
                for (std::size_t index = 0; index < halfWidth; ++index) {
                    const auto even = values[offset + index];
                    const auto odd = values[offset + index + halfWidth] * twiddle;
                    values[offset + index] = even + odd;
                    values[offset + index + halfWidth] = even - odd;
                    twiddle *= step;
                }
            }
        }

        if (inverse) {
            const double scale = 1.0 / static_cast<double>(settings.size);
            for (auto& value : values) {
                value *= scale;
            }
        }
    }

    static void BitReverse(std::span<std::complex<double>> values) {
        std::size_t reversed = 0;
        for (std::size_t index = 1; index < values.size(); ++index) {
            std::size_t bit = values.size() / 2;
            while ((reversed & bit) != 0) {
                reversed ^= bit;
                bit /= 2;
            }
            reversed ^= bit;

            if (index < reversed) {
                std::swap(values[index], values[reversed]);
            }
        }
    }

    FftSettings settings;
};

} // namespace consolidator::dsp

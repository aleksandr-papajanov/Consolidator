#pragma once

#include "c74_min.h"

#include <array>
#include <initializer_list>
#include <string>
#include <utility>

class FilterControl {
public:
    using atoms = c74::min::atoms;

    FilterControl(std::string id, std::array<double, 4> position)
        : id_(std::move(id)), position_(position) {}

    const std::string& id() const { return id_; }

    const std::array<double, 4>& position() const { return position_; }

    void set_position(const std::array<double, 4>& position) {
        position_ = position;
    }

    atoms control_update(const std::string& action, std::initializer_list<double> values = {}) const {
        atoms result;
        result.push_back("control");
        result.push_back(id_);
        result.push_back(action);
        for (const auto value : values) {
            result.push_back(value);
        }
        return result;
    }

private:
    std::string id_;
    std::array<double, 4> position_{};
};

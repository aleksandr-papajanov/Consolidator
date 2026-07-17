#pragma once

#include "c74_min.h"
#include "MessageEnvelope.h"

#include <array>
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

class FilterControl {
public:
    FilterControl(std::string id, std::array<double, 4> position)
        : id_(std::move(id)), position_(position) {}

    const std::string& id() const { return id_; }

    const std::array<double, 4>& position() const { return position_; }

    void set_position(const std::array<double, 4>& position) {
        position_ = position;
    }

    consolidator::protocol::MessageEnvelope control_update(
        const std::string& action,
        std::initializer_list<double> values = {}) const {
        consolidator::protocol::MessageEnvelope message{ std::string{ "filter.control" } };
        message.set_payload_symbol("control", id_);
        message.set_payload_symbol("action", action);
        message.set_payload_numbers("values", std::vector<double>(values));
        return message;
    }

private:
    std::string id_;
    std::array<double, 4> position_{};
};

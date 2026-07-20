#pragma once

#include "AtomTypes.h"

#include <utility>

namespace consolidator::messaging {

class AtomWriter final {
public:
    AtomWriter& Write(std::int64_t value) {
        atoms.push_back(value);
        return *this;
    }

    AtomWriter& Write(double value) {
        atoms.push_back(value);
        return *this;
    }

    AtomWriter& Write(bool value) {
        atoms.push_back(value);
        return *this;
    }

    AtomWriter& Write(std::string value) {
        atoms.push_back(std::move(value));
        return *this;
    }

    AtomList Finish() && { return std::move(atoms); }

private:
    AtomList atoms;
};

} // namespace consolidator::messaging

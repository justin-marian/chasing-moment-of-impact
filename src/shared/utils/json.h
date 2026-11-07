/**
 * @file
 * Validate shared JSON value shapes before converting them into domain state.
 */

#pragma once

#include "shared/sim/math.h"

#include <nlohmann/json.hpp>
#include <stdexcept>

#include <cstddef>


namespace utils {

constexpr std::size_t VECTOR_COMPONENT_COUNT = 3;


/**
 * Encode a simulation vector as a JSON array in X, Y, Z order.
 *
 * @param value Simulation vector encoded in X, Y, Z order.
 * @return JSON array containing X, Y and Z components in that order.
 */
inline nlohmann::json vectorJson(sim::Vec3 value) {
    return nlohmann::json::array({value.x, value.y, value.z});
}


/**
 * Validate and decode one finite, three-component simulation vector from JSON.
 *
 * @param value JSON array expected to contain exactly three finite numeric components.
 * @return Validated finite simulation vector.
 * @throws std::runtime_error If the JSON is not an array of three finite numbers.
 */
inline sim::Vec3 jsonVector(const nlohmann::json& value) {
    if (!value.is_array() || value.size() != VECTOR_COMPONENT_COUNT)
        throw std::runtime_error("Expected vector3");

    sim::Vec3 result{
        value.at(0).get<sim::Real>(),
        value.at(1).get<sim::Real>(),
        value.at(2).get<sim::Real>()
    };

    if (!sim::finite(result))
        throw std::runtime_error("Expected finite vector3");

    return result;
}

} // namespace utils

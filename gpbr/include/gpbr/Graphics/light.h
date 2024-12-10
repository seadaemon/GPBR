/* light.h
 *
 * Provides structures to represent lights in a 3D scene.
 *
 */
#pragma once

#include <string>
#include <glm/vec3.hpp>

enum class LightType
{
    None,
    Directional,
    Point,
    Spot
};

struct Light
{
    std::string name{""};
    glm::vec3 position{0.f, 0.f, 0.f}; // World-space coordinates.
    glm::vec3 color{1.f, 1.f, 1.f};    // Linear RGB color.
    float intensity{1.f};
    LightType type{LightType::None};

    // Returns the integer representation of the current LightType
    uint32_t get_GPU_type() const
    {
        switch (type)
        {
        case LightType::Directional:
            return 0;
        case LightType::Point:
            return 1;
        case LightType::Spot:
            return 2;
        default:
            assert(false);
        }
        return 0;
    }
};

// Light data to be used in shaders.
struct GPULightData
{
    glm::vec3 position;
    glm::vec3 color;
    float intensity;
    std::uint32_t type;
    glm::vec4 extra[14];
};

static_assert(sizeof(GPULightData) == 256);
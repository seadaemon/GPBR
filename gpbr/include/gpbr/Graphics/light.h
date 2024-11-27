/*
 * Provides structures to represent lights in a 3D scene
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
    // Name of the light.
    std::string name{""};
    // Position of the light in world-space.
    glm::vec3 position{0.f, 0.f, 0.f};
    // RGB light color in linear space.
    glm::vec3 color{1.f, 1.f, 1.f};
    // Brightness of light.
    float intensity{1.f};
    // The type of the light (directional, point, spot).
    LightType type{LightType::None};

    // Returns the integer representation of the current LightType
    int get_GPU_type() const
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

// Light data to be used in shaders
struct GPULightData
{
    glm::vec3 position;
    glm::vec3 color;
    float intensity;
    std::uint32_t type;
    glm::vec4 extra[14];
};

static_assert(sizeof(GPULightData) == 256);
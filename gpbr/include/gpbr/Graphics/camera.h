/* camera.h
 *
 * Specifies structs and classes for a camera.
 * Intended to be used in 3D with a perspective projection.
 *
 */
#pragma once

#include "Vulkan/vk_types.h"
#include "SDL3/SDL_events.h"

// consider moving Plane and Frustum structs elsewhere?
// they could eventually have use cases unrelated to a camera

// A 3D plane described by a normal and a distance from some origin.
struct Plane
{
    glm::vec3 normal{0.f, 1.f, 0.f};
    float distance{0.f};
};

// A frustum described by 6 planes.
struct Frustum
{
    Plane top;
    Plane bottom;
    Plane left;
    Plane right;
    Plane near;
    Plane far;
};

// Tracks the state of user inputs for smooth processing.
struct InputState
{
    bool move_forwards;
    bool move_backwards;
    bool move_left;
    bool move_right;

    float yaw_target;
    float pitch_target;
};

// A configurable 3D camera.
class Camera
{
  public:
    Camera(float near = 0.1f, float far = 1000.f, float fovy = 1.22173f);
    ~Camera();

    float fovy;   // Vertical field-of-view in radians.
    float near;   // Distance to near clipping plane.
    float far;    // Distance to far clipping plane.
    float aspect; // Aspect ratio of the viewport.

    float max_velocity;
    glm::vec3 velocity; // Current translation velocity.
    glm::vec3 position; // Current position in world-space.

    float pitch;               // Vertical rotation (radians).
    float yaw;                 // Horitzontal rotation (radians).
    glm::mat4 rotation_mat;    // Current rotation-matrix.
    glm::mat4 translation_mat; // Current translation-matrix.
    glm::mat4 view_mat;        // Current view-matrix (A.K.A camera-matrix).

    // View-space axis vectors.

    glm::vec3 forward; // -Z in view-space.
    glm::vec3 right;   // +X in view-space.
    glm::vec3 up;      // +Y in view-space.

    Frustum frustum; // Perspective view frustum.

    InputState input; // Current state of user input.

    void process_SDL_event(SDL_Event& e);

    void update(); // Updates the current state of the camera.
};
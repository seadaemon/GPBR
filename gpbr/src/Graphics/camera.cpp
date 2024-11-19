#include "gpbr/Graphics/camera.h"
#include "glm/gtx/transform.hpp"
#include "glm/gtx/quaternion.hpp"

// Todo: use delta time so updates arent frame-rate dependent
void Camera::update()
{
    glm::mat4 camera_rotation = get_rotation_matrix();
    position += glm::vec3(camera_rotation * glm::vec4(velocity * 0.5f, 0.f));
}

glm::mat4 Camera::get_view_matrix()
{
    // to create a correct model view, we need to move the world in opposite
    // direction to the camera
    //  so we will create the camera model matrix and invert
    glm::mat4 camera_translation = glm::translate(glm::mat4(1.f), position);
    glm::mat4 camera_rotation    = get_rotation_matrix();

    return glm::inverse(camera_translation * camera_rotation);
}

glm::mat4 Camera::get_rotation_matrix()
{
    glm::quat pitch_rotation = glm::angleAxis(pitch, glm::vec3{1.f, 0.f, 0.f});
    glm::quat yaw_rotation   = glm::angleAxis(yaw, glm::vec3{0.f, -1.f, 0.f});

    return glm::toMat4(yaw_rotation) * glm::toMat4(pitch_rotation);
}

void Camera::process_SDL_event(SDL_Event& e)
{
    // Keyboard inputs WASD
    if (e.type == SDL_EVENT_KEY_DOWN)
    {
        if (e.key.key == SDLK_W)
        {
            velocity.z = -max_velocity;
        }
        if (e.key.key == SDLK_S)
        {
            velocity.z = max_velocity;
        }
        if (e.key.key == SDLK_A)
        {
            velocity.x = -max_velocity;
        }
        if (e.key.key == SDLK_D)
        {
            velocity.x = max_velocity;
        }
    }
    if (e.type == SDL_EVENT_KEY_UP)
    {
        if (e.key.key == SDLK_W)
        {
            velocity.z = 0;
        }
        if (e.key.key == SDLK_S)
        {
            velocity.z = 0;
        }
        if (e.key.key == SDLK_A)
        {
            velocity.x = 0;
        }
        if (e.key.key == SDLK_D)
        {
            velocity.x = 0;
        }
    }

    // Mouse Inputs
    if (e.type == SDL_EVENT_MOUSE_MOTION)
    {
        yaw += (float)e.motion.xrel / 200.f;
        pitch -= (float)e.motion.yrel / 200.f;
    }
}

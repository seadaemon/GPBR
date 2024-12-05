#include "gpbr/Graphics/camera.h"
#include "glm/gtx/transform.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtc/epsilon.hpp"

Camera::Camera(float near /*= 0.1f*/, float far /*= 1000.f*/, float fovy /*= 1.22173f*/)
    : near(near),
      far(far),
      fovy(fovy),

      aspect{1.3333333f},
      max_velocity{0.1f},
      velocity{0.f},
      position{0.f},
      pitch{0.f},
      yaw{0.f},
      rotation_mat{1.f},
      translation_mat{1.f},
      view_mat{1.f},
      forward{0.f, 0.f, -1.f},
      right{1.f, 0.f, 0.f},
      up{0.f, 1.f, 0.f},
      frustum{},
      input{false},
      mouse_sensitivity{100.f}
{
}

Camera::~Camera() {}

// consider using delta time so updates arent frame-rate dependent

void Camera::update()
{
    velocity = glm::vec3{0.f};

    if (input.move_forwards)
    {
        velocity.z = -max_velocity;
    }
    if (input.move_backwards)
    {
        velocity.z = max_velocity;
    }
    if (input.move_left)
    {
        velocity.x = -max_velocity;
    }
    if (input.move_right)
    {
        velocity.x = max_velocity;
    }

    // normalize only when velocity is non-zero
    if (glm::epsilonEqual(velocity, glm::vec3(0.f), 1e-3f) != glm::bvec3(true))
    {
        velocity = glm::normalize(velocity) * max_velocity;
    }

    yaw   = glm::mix(yaw, yaw + input.yaw_target, 0.1f);
    pitch = glm::mix(pitch, pitch - input.pitch_target, 0.1f);
    // yaw += input.yaw_target;
    // pitch -= input.pitch_target;

    input.yaw_target   = 0.f;
    input.pitch_target = 0.f;

    //= END OF INPUT HANDLING ==================================================

    glm::quat pitch_rotation = glm::angleAxis(pitch, glm::vec3{1.f, 0.f, 0.f});
    glm::quat yaw_rotation   = glm::angleAxis(yaw, glm::vec3{0.f, -1.f, 0.f});
    rotation_mat             = glm::toMat4(yaw_rotation) * glm::toMat4(pitch_rotation);

    position += glm::vec3(rotation_mat * glm::vec4(velocity * 0.5f, 0.f));
    translation_mat = glm::translate(glm::mat4{1.f}, position);

    view_mat = glm::inverse(translation_mat * rotation_mat);

    forward = -glm::normalize(glm::vec3(view_mat[0][2], view_mat[1][2], view_mat[2][2]));
    right   = glm::normalize(glm::vec3(view_mat[0][0], view_mat[1][0], view_mat[2][0]));
    up      = glm::normalize(glm::vec3(view_mat[0][1], view_mat[1][1], view_mat[2][1]));

    // Update viewing frustum (all normals face inward)

    // half-lengths of the vertical and horizontal sides of the far-plane
    const float half_v_side = far * glm::tan(fovy * 0.5f);
    const float half_h_side = half_v_side * aspect;

    frustum.near   = {forward, near};
    frustum.far    = {-forward, far};
    frustum.right  = {-glm::normalize(glm::cross((far * forward) + (half_h_side * right), up)), 0};
    frustum.left   = {-glm::normalize(glm::cross((far * forward) - (half_h_side * right), up)), 0};
    frustum.top    = {-glm::normalize(glm::cross((far * forward) + (half_v_side * up), right)), 0};
    frustum.bottom = {-glm::normalize(glm::cross((far * forward) - (half_v_side * up), right)), 0};
}

void Camera::process_SDL_event(SDL_Event& e)
{
    // Keyboard inputs
    if (e.type == SDL_EVENT_KEY_DOWN)
    {
        if (e.key.key == SDLK_W)
        {
            input.move_forwards = true;
        }
        if (e.key.key == SDLK_S)
        {
            input.move_backwards = true;
        }
        if (e.key.key == SDLK_A)
        {
            input.move_left = true;
        }
        if (e.key.key == SDLK_D)
        {
            input.move_right = true;
        }
    }
    if (e.type == SDL_EVENT_KEY_UP)
    {
        if (e.key.key == SDLK_W)
        {
            input.move_forwards = false;
        }
        if (e.key.key == SDLK_S)
        {
            input.move_backwards = false;
        }
        if (e.key.key == SDLK_A)
        {
            input.move_left = false;
        }
        if (e.key.key == SDLK_D)
        {
            input.move_right = false;
        }
    }

    // Mouse Inputs
    if (e.type == SDL_EVENT_MOUSE_MOTION)
    {
        input.yaw_target   = (float)e.motion.xrel / mouse_sensitivity;
        input.pitch_target = (float)e.motion.yrel / mouse_sensitivity;
    }
}

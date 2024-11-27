#include "gpbr/Graphics/camera.h"
#include "glm/gtx/transform.hpp"
#include "glm/gtx/quaternion.hpp"

Camera::Camera(float near /*= 0.1f*/, float far /*= 1000.f*/, float fovy /*= 1.22173f*/)
    : near(near), far(far), fovy(fovy)
{
}

Camera::~Camera() {}

// Todo: use delta time so updates arent frame-rate dependent

void Camera::update()
{
    glm::quat pitch_rotation = glm::angleAxis(pitch, glm::vec3{1.f, 0.f, 0.f});
    glm::quat yaw_rotation   = glm::angleAxis(yaw, glm::vec3{0.f, -1.f, 0.f});
    rotation_mat             = glm::toMat4(yaw_rotation) * glm::toMat4(pitch_rotation);

    // position of the camera in world-space coordinates
    position += glm::vec3(rotation_mat * glm::vec4(velocity * 0.5f, 0.f));

    glm::mat4 translation_mat = glm::translate(glm::mat4{1.f}, position);

    view_mat = get_view_matrix();

    forward = -glm::normalize(glm::vec3(view_mat[0][2], view_mat[1][2], view_mat[2][2]));

    right = glm::normalize(glm::vec3(view_mat[0][0], view_mat[1][0], view_mat[2][0]));

    up = glm::normalize(glm::vec3(view_mat[0][1], view_mat[1][1], view_mat[2][1]));

    // Update viewing frustum (note that far and near are swapped)
    // All normals of the frustum are facing inwards

    // half-lengths of the vertical and horizontal sides of the far-plane
    const float half_v_side = far * glm::tan(fovy * 0.5f);
    const float half_h_side = half_v_side * aspect;

    // frustum.face = {normal, distance}
    frustum.near   = {forward, near};
    frustum.far    = {-forward, far};
    frustum.right  = {-glm::normalize(glm::cross((far * forward) + (half_h_side * right), up)), 0};
    frustum.left   = {-glm::normalize(glm::cross((far * forward) - (half_h_side * right), up)), 0};
    frustum.top    = {-glm::normalize(glm::cross((far * forward) + (half_v_side * up), right)), 0};
    frustum.bottom = {-glm::normalize(glm::cross((far * forward) - (half_v_side * up), right)), 0};
}

// Returns a view-matrix calculated from the camera's transformation matrix.
glm::mat4 Camera::get_view_matrix()
{
    // The transformation matrix is the product of the Translation, Rotation, and Scalar matrices (TRS)

    // The view-matrix is the inverse of this matrix, because the world moves inversely with respect to the camera

    glm::mat4 translation_mat = glm::translate(glm::mat4(1.f), position);

    return glm::inverse(translation_mat * rotation_mat);
}

// Returns a matrix representing the current rotation of the camera.
glm::mat4 Camera::get_rotation_matrix()
{
    return rotation_mat;
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

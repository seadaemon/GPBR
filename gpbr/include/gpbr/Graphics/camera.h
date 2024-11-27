#include "Vulkan/vk_types.h"
#include "SDL3/SDL_events.h"

// TODO: move these elsewhere? Plane and Frustum could have other uses one day...

// A 3D plane described by a normal vector and a distance from some origin.
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

// A 3D camera object
class Camera
{

  public:
    Camera(float near = 0.1f, float far = 1000.f, float fovy = 1.22173f); // 70deg ~= 1.22173rad
    ~Camera();

    // Vertical field-of-view in radians
    float fovy{glm::radians(70.f)};

    // Distance to the near-plane.
    float near = 0.1f;
    // Distance to the far-plane.
    float far = 1000.f;

    float max_velocity{0.1f};
    // Current velocity of the camera.
    glm::vec3 velocity;
    // Current position of the camera in world-space.
    glm::vec3 position;
    // Current rotation-matrix computed in update().
    glm::mat4 rotation_mat = glm::mat4(1.f);
    // Current view-matrix computed in get_view_matrix().
    glm::mat4 view_mat = glm::mat4(1.f);

    // Forward direction of the camera. (-Z without rotations).
    glm::vec3 forward;
    // Right direction of the camera. (+X without rotations).
    glm::vec3 right;
    // Up direction of the camera. (+Y without rotations).
    glm::vec3 up;

    // A view frustum based on the camera's forward direction
    Frustum frustum;

    float pitch{0.f}; // Vertical rotation in radians.
    float yaw{0.f};   // Horitzontal rotation in radians.

    // Aspect ratio of the camera's viewport.
    float aspect{1.3333333f};

    glm::mat4 get_view_matrix();
    glm::mat4 get_rotation_matrix();

    void process_SDL_event(SDL_Event& e);

    // Updates the rotation matrix, position, forward and up vectors, and the viewing frustum
    void update();
};
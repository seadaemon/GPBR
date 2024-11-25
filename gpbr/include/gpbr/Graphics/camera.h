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
    Camera(float near = 1000.f, float far = 0.1f, float fov = 1.22173f); // 70deg ~= 1.22173rad
    ~Camera();

    // Field of view in radians
    float fov{glm::radians(70.f)};

    // near & far for the viewing frustum; uses reversed depth

    float near = 1000.f;

    float far = 0.1f;

    float max_velocity{0.1f};
    // Current velocity of the camera.
    glm::vec3 velocity;
    // Current position of the camera in world-space.
    glm::vec3 position;
    // Current rotation of the camera.
    glm::mat4 rotation_mat = glm::mat4(1.f);
    // Forward direction of the camera
    glm::vec3 forward;

    // A view frustum based on the camera's forward direction
    Frustum frustum;

    float pitch{0.f}; // vertical rotation
    float yaw{0.f};   // horitzontal rotation

    glm::mat4 get_view_matrix();
    glm::mat4 get_rotation_matrix();

    void process_SDL_event(SDL_Event& e);
    void update();
};
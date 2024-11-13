#include "Vulkan/vk_types.h"
#include "SDL3/SDL_events.h"

// A 3D camera
class Camera
{
  public:
    glm::vec3 velocity;
    glm::vec3 position;

    float pitch{0.f}; // vertical rotation
    float yaw{0.f};   // horitzontal rotation

    glm::mat4 get_view_matrix();
    glm::mat4 get_rotation_matrix();

    void process_SDL_event(SDL_Event& e);
    void update();
};
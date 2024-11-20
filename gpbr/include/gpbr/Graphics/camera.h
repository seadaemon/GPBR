#include "Vulkan/vk_types.h"
#include "SDL3/SDL_events.h"

// A 3D camera
class Camera
{
  public:
    float max_velocity{0.1f};
    glm::vec3 velocity;
    glm::vec3 position;
    glm::mat4 rotation_mat = glm::mat4(1.f);

    float pitch{0.f}; // vertical rotation
    float yaw{0.f};   // horitzontal rotation

    glm::mat4 get_view_matrix();
    glm::mat4 get_rotation_matrix();

    void process_SDL_event(SDL_Event& e);
    void update();
};
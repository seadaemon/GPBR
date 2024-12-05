/* vk_pipelines.h
 *
 * Provides methods to build graphics pipelines and shader modules.
 *
 */
#pragma once

#include "vk_types.h"

// Configures and builds a graphics pipeline.
class PipelineBuilder
{
  public:
    std::vector<VkPipelineShaderStageCreateInfo> _shader_stages;
    VkPipelineInputAssemblyStateCreateInfo _input_assembly;
    VkPipelineRasterizationStateCreateInfo _rasterizer;
    VkPipelineColorBlendAttachmentState _color_blend_attachment;
    VkPipelineMultisampleStateCreateInfo _multisampling;
    VkPipelineLayout _pipeline_layout;
    VkPipelineDepthStencilStateCreateInfo _depth_stencil;
    VkPipelineRenderingCreateInfo _render_info;
    VkFormat _color_attachment_format;

    PipelineBuilder() { clear(); }

    // Clears all members and sets .sTypes for CreateInfo structs.
    void clear();

    // Creates a new pipeline using the current settings.
    VkPipeline build_pipeline(VkDevice device);

    void set_shaders(VkShaderModule vertex_shader, VkShaderModule fragment_shader);
    void set_input_topology(VkPrimitiveTopology topology);
    void set_polygon_mode(VkPolygonMode mode);
    void set_cull_mode(VkCullModeFlags cull_mode, VkFrontFace front_face);

    void set_multisampling_none();
    // Todo: add MSAA

    void disable_blending();
    void enable_blending_additive();
    void enable_blending_alphablend();

    void set_color_attachment_format(VkFormat format);
    void set_depth_format(VkFormat format);

    void disable_depthtest();
    void enable_depthtest(bool depth_write_enable, VkCompareOp op);
};

namespace vkutil
{
// Creates a shader module from a shader source file.
// Returns false if the module is not created.
bool load_shader_module(const char* file_path, VkDevice device, VkShaderModule* out_shader_module);
}
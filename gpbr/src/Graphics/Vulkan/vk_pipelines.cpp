#include "Volk/volk.h"
#include "gpbr/Graphics/Vulkan/vk_pipelines.h"
#include "gpbr/Graphics/Vulkan/vk_initializers.h"
#include <fstream>

void PipelineBuilder::clear()
{
    _input_assembly = {.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};

    _rasterizer = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};

    _color_blend_attachment = {};

    _multisampling = {.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};

    _pipeline_layout = {};

    _depth_stencil = {.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};

    _render_info = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};

    _shader_stages.clear();
}

VkPipeline PipelineBuilder::build_pipeline(VkDevice device)
{
    // make viewport state from the stored viewport and scissor.
    VkPipelineViewportStateCreateInfo viewport_state = {};
    viewport_state.sType                             = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.pNext                             = nullptr;
    viewport_state.viewportCount                     = 1;
    viewport_state.scissorCount                      = 1;

    // default to no blending
    VkPipelineColorBlendStateCreateInfo color_blending = {};
    color_blending.sType                               = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending.pNext                               = nullptr;
    color_blending.logicOpEnable                       = VK_FALSE;
    color_blending.logicOp                             = VK_LOGIC_OP_COPY;
    color_blending.attachmentCount                     = 1;
    color_blending.pAttachments                        = &_color_blend_attachment;

    // completely clear VertexInputStateCreateInfo, as we have no need for it
    VkPipelineVertexInputStateCreateInfo vertex_input_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};

    VkGraphicsPipelineCreateInfo pipeline_info = {.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};

    pipeline_info.pNext               = &_render_info; // connect the renderInfo to the pNext extension mechanism
    pipeline_info.stageCount          = (uint32_t)_shader_stages.size();
    pipeline_info.pStages             = _shader_stages.data();
    pipeline_info.pVertexInputState   = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &_input_assembly;
    pipeline_info.pViewportState      = &viewport_state;
    pipeline_info.pRasterizationState = &_rasterizer;
    pipeline_info.pMultisampleState   = &_multisampling;
    pipeline_info.pColorBlendState    = &color_blending;
    pipeline_info.pDepthStencilState  = &_depth_stencil;
    pipeline_info.layout              = _pipeline_layout;

    VkDynamicState state[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamic_info = {.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamic_info.pDynamicStates                   = &state[0];
    dynamic_info.dynamicStateCount                = 2;

    pipeline_info.pDynamicState = &dynamic_info;

    VkPipeline new_pipeline;
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &new_pipeline) != VK_SUCCESS)
    {
        fmt::println("Failed to create pipeline!");
        return VK_NULL_HANDLE;
    }
    else
    {
        return new_pipeline;
    }
}

void PipelineBuilder::set_shaders(VkShaderModule vertex_shader, VkShaderModule fragment_shader)
{
    _shader_stages.clear();

    _shader_stages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, vertex_shader));

    _shader_stages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, fragment_shader));
}

void PipelineBuilder::set_input_topology(VkPrimitiveTopology topology)
{
    _input_assembly.topology               = topology;
    _input_assembly.primitiveRestartEnable = VK_FALSE;
}

void PipelineBuilder::set_polygon_mode(VkPolygonMode mode)
{
    _rasterizer.polygonMode = mode;
    _rasterizer.lineWidth   = 1.f;
}

void PipelineBuilder::set_cull_mode(VkCullModeFlags cull_mode, VkFrontFace front_face)
{
    _rasterizer.cullMode  = cull_mode;
    _rasterizer.frontFace = front_face;
}

void PipelineBuilder::set_multisampling_none()
{
    _multisampling.sampleShadingEnable = VK_FALSE;

    _multisampling.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT; // 1 sample per pixel
    _multisampling.minSampleShading      = 1.0f;
    _multisampling.pSampleMask           = nullptr;
    _multisampling.alphaToCoverageEnable = VK_FALSE;
    _multisampling.alphaToOneEnable      = VK_FALSE;
}

void PipelineBuilder::disable_blending()
{
    // default write mask
    _color_blend_attachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    _color_blend_attachment.blendEnable = VK_FALSE;
}

void PipelineBuilder::enable_blending_additive()
{
    _color_blend_attachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    _color_blend_attachment.blendEnable         = VK_TRUE;
    _color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    _color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
    _color_blend_attachment.colorBlendOp        = VK_BLEND_OP_ADD;
    _color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    _color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    _color_blend_attachment.alphaBlendOp        = VK_BLEND_OP_ADD;
}

void PipelineBuilder::enable_blending_alphablend()
{
    _color_blend_attachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    _color_blend_attachment.blendEnable         = VK_TRUE;
    _color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    _color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    _color_blend_attachment.colorBlendOp        = VK_BLEND_OP_ADD;
    _color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    _color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    _color_blend_attachment.alphaBlendOp        = VK_BLEND_OP_ADD;
}

void PipelineBuilder::set_color_attachment_format(VkFormat format)
{
    _color_attachment_format = format;

    _render_info.colorAttachmentCount    = 1;
    _render_info.pColorAttachmentFormats = &_color_attachment_format;
}

void PipelineBuilder::set_depth_format(VkFormat format)
{
    _render_info.depthAttachmentFormat = format;
}

void PipelineBuilder::disable_depthtest()
{
    _depth_stencil.depthTestEnable       = VK_FALSE;
    _depth_stencil.depthWriteEnable      = VK_FALSE;
    _depth_stencil.depthCompareOp        = VK_COMPARE_OP_NEVER;
    _depth_stencil.depthBoundsTestEnable = VK_FALSE;
    _depth_stencil.stencilTestEnable     = VK_FALSE;
    _depth_stencil.front                 = {};
    _depth_stencil.back                  = {};
    _depth_stencil.minDepthBounds        = 0.f;
    _depth_stencil.maxDepthBounds        = 1.f;
}

void PipelineBuilder::enable_depthtest(bool depth_write_enable, VkCompareOp op)
{
    _depth_stencil.depthTestEnable       = VK_TRUE;
    _depth_stencil.depthWriteEnable      = depth_write_enable;
    _depth_stencil.depthCompareOp        = op;
    _depth_stencil.depthBoundsTestEnable = VK_FALSE;
    _depth_stencil.stencilTestEnable     = VK_FALSE;
    _depth_stencil.front                 = {};
    _depth_stencil.back                  = {};
    _depth_stencil.minDepthBounds        = 0.0f;
    _depth_stencil.maxDepthBounds        = 1.0f;
}

bool vkutil::load_shader_module(const char* file_path, VkDevice device, VkShaderModule* out_shader_module)
{
    // open the file in binary mode and seek to the end.
    std::ifstream file(file_path, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        return false;
    }

    size_t file_size = (size_t)file.tellg(); // file size in bytes

    std::vector<uint32_t> buffer(file_size / sizeof(uint32_t)); // reserve a buffer for the shader

    file.seekg(0); // put file cursor at beginning

    file.read((char*)buffer.data(), file_size); // load the entire file into the buffer

    file.close();

    VkShaderModuleCreateInfo create_info = {};
    create_info.sType                    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.pNext                    = nullptr;
    create_info.codeSize                 = buffer.size() * sizeof(uint32_t); // code size in bytes
    create_info.pCode                    = buffer.data();

    VkShaderModule shader_module;
    if (vkCreateShaderModule(device, &create_info, nullptr, &shader_module) != VK_SUCCESS)
    {
        return false;
    }

    *out_shader_module = shader_module;
    return true;
}
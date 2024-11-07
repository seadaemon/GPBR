#include "Volk/volk.h"
#include "gpbr/Graphics/Vulkan/vk_pipelines.h"
#include "gpbr/Graphics/Vulkan/vk_initializers.h"
#include <fstream>

void PipelineBuilder::clear()
{
    // clear all of the structs we need back to 0 with their correct stype

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
    // make viewport state from our stored viewport and scissor.
    // at the moment we wont support multiple viewports or scissors
    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType                             = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.pNext                             = nullptr;

    viewportState.viewportCount = 1;
    viewportState.scissorCount  = 1;

    // setup dummy color blending. We arent using transparent objects yet
    // the blending is just "no blend", but we do write to the color attachment
    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType                               = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.pNext                               = nullptr;

    colorBlending.logicOpEnable   = VK_FALSE;
    colorBlending.logicOp         = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments    = &_color_blend_attachment;

    // completely clear VertexInputStateCreateInfo, as we have no need for it
    VkPipelineVertexInputStateCreateInfo _vertexInputInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};

    //< build_pipeline_1

    //> build_pipeline_2
    // build the actual pipeline
    // we now use all of the info structs we have been writing into into this one
    // to create the pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo = {.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    // connect the renderInfo to the pNext extension mechanism
    pipelineInfo.pNext = &_render_info;

    pipelineInfo.stageCount          = (uint32_t)_shader_stages.size();
    pipelineInfo.pStages             = _shader_stages.data();
    pipelineInfo.pVertexInputState   = &_vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &_input_assembly;
    pipelineInfo.pViewportState      = &viewportState;
    pipelineInfo.pRasterizationState = &_rasterizer;
    pipelineInfo.pMultisampleState   = &_multisampling;
    pipelineInfo.pColorBlendState    = &colorBlending;
    pipelineInfo.pDepthStencilState  = &_depth_stencil;
    pipelineInfo.layout              = _pipeline_layout;

    //< build_pipeline_2
    //> build_pipeline_3
    VkDynamicState state[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamicInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamicInfo.pDynamicStates                   = &state[0];
    dynamicInfo.dynamicStateCount                = 2;

    pipelineInfo.pDynamicState = &dynamicInfo;
    //< build_pipeline_3
    //> build_pipeline_4
    // its easy to error out on create graphics pipeline, so we handle it a bit
    // better than the common VK_CHECK case
    VkPipeline newPipeline;
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &newPipeline) != VK_SUCCESS)
    {
        fmt::println("failed to create pipeline");
        return VK_NULL_HANDLE; // failed to create graphics pipeline
    }
    else
    {
        return newPipeline;
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
    _input_assembly.topology = topology;
    // we are not going to use primitive restart on the entire tutorial so leave
    // it on false
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
    // multisampling defaulted to no multisampling (1 sample per pixel)
    _multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    _multisampling.minSampleShading     = 1.0f;
    _multisampling.pSampleMask          = nullptr;
    // no alpha to coverage either
    _multisampling.alphaToCoverageEnable = VK_FALSE;
    _multisampling.alphaToOneEnable      = VK_FALSE;
}

void PipelineBuilder::disable_blending()
{
    // default write mask
    _color_blend_attachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    // no blending
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
    // connect the format to the renderInfo  structure
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
    _depth_stencil.minDepthBounds        = 0.f;
    _depth_stencil.maxDepthBounds        = 1.f;
}

bool vkutil::load_shader_module(const char* file_path, VkDevice device, VkShaderModule* out_shader_module)
{
    // open the file. With cursor at the end
    std::ifstream file(file_path, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        return false;
    }

    // find what the size of the file is by looking up the location of the cursor
    // because the cursor is at the end, it gives the size directly in bytes
    size_t file_size = (size_t)file.tellg();

    // spirv expects the buffer to be on uint32, so make sure to reserve a int
    // vector big enough for the entire file
    std::vector<uint32_t> buffer(file_size / sizeof(uint32_t));

    // put file cursor at beginning
    file.seekg(0);

    // load the entire file into the buffer
    file.read((char*)buffer.data(), file_size);

    // now that the file is loaded into the buffer, we can close it
    file.close();

    // create a new shader module, using the buffer we loaded
    VkShaderModuleCreateInfo create_info = {};
    create_info.sType                    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.pNext                    = nullptr;

    // codeSize has to be in bytes, so multply the ints in the buffer by size of
    // int to know the real size of the buffer
    create_info.codeSize = buffer.size() * sizeof(uint32_t);
    create_info.pCode    = buffer.data();

    // check that the creation goes well.
    VkShaderModule shader_module;
    if (vkCreateShaderModule(device, &create_info, nullptr, &shader_module) != VK_SUCCESS)
    {
        return false;
    }
    *out_shader_module = shader_module;
    return true;
}
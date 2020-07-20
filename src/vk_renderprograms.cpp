#include <fstream>
#include <string>
#include "vk_renderprograms.h"
#include "vertex_type.h"

#define ARRAYSIZE(a) \
  ((sizeof(a) / sizeof(*(a))) / \
  static_cast<size_t>(!(sizeof(a) % sizeof(*(a)))))

struct VertexDescriptions_t {
    VkPipelineVertexInputStateCreateInfo inputState;
    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;
};

//Fwd declares
bool loadShader(Shader_t& shader, VkDevice device, std::string fileName, VkShaderStageFlagBits stage);
static VkShaderModule loadShaderModule(const char *fileName, VkDevice device);
static VertexDescriptions_t getVertexDescriptions();

Shader_t meshVS = {};
Shader_t goochFS = {};
Shader_t lambertFS = {};

VkDescriptorPool descPool = 0;
VkDescriptorSetLayout descSetLayout;
VkDescriptorSet descSets[1];
VkPipelineCache pipelineCache = 0;
VkPipelineLayout gfxPipeLayout = 0;
VkPipeline meshPipeline = 0;

void setupFirstTimeRenderprogs(u32 uboSize, std::vector<glm::vec4> &pc) {
    VertexDescriptions_t vtxDescs = getVertexDescriptions();

    bool res = false;
    res = loadShader(meshVS, vk_device, "../mesh.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    ASSERT(res);
    res = loadShader(goochFS, vk_device, "../gooch.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    ASSERT(res);
    res = loadShader(lambertFS, vk_device, "../lambert.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    ASSERT(res);

    descPool = createDescriptorPool();
    descSetLayout = createDescriptorSetLayout();
    allocateDescriptorSet(descPool, descSetLayout, descSets, 1);
    updateDescriptorSet(uboBuffer, /*offset*/0, /*range*/uboSize, descSets);

    VkPushConstantRange pushConstantRange;
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.size = sizeof(pc); // NOTE(anton): SW samples uses this instead of above, why?
    pushConstantRange.offset = 0;

    VkPipelineLayoutCreateInfo layout_create_info = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    layout_create_info.setLayoutCount = 1;
    layout_create_info.pSetLayouts = &descSetLayout;
    layout_create_info.pushConstantRangeCount = 1;
    layout_create_info.pPushConstantRanges = &pushConstantRange;

    VK_CHECK(vkCreatePipelineLayout(vk_device, &layout_create_info, nullptr, &gfxPipeLayout));
    meshPipeline = createGraphicsPipeline(vk_device, pipelineCache, renderPass,
                                                     meshVS, lambertFS, gfxPipeLayout,
                                                     &vtxDescs);
    Logger::Trace("created Mesh pipeline!");
}

static
VkDescriptorPool createDescriptorPool() {
    VkDescriptorPoolSize poolSizes[1];
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = 1;

    VkDescriptorPoolCreateInfo createInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    createInfo.poolSizeCount = ARRAYSIZE(poolSizes);
    createInfo.pPoolSizes = poolSizes;
    createInfo.maxSets = 16;

    VkDescriptorPool pool = 0;
    VK_CHECK(vkCreateDescriptorPool(vk_device, &createInfo, nullptr, &pool));
    return pool;
}

static
VkDescriptorSetLayout createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding uboLayoutBinding = {};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding bindings[1] = {uboLayoutBinding};

    VkDescriptorSetLayoutCreateInfo createInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    createInfo.bindingCount = ARRAYSIZE(bindings);
    createInfo.pBindings = bindings;

    VkDescriptorSetLayout layout = 0;
    VK_CHECK( vkCreateDescriptorSetLayout(vk_device, &createInfo, nullptr, &layout) );
    return layout;
}

static
void
allocateDescriptorSet(VkDescriptorPool pool, VkDescriptorSetLayout layout,
                      VkDescriptorSet *descSets, u32 numDescSets) {
    VkDescriptorSetLayout layouts[1] = {layout};
    ASSERT( ARRAYSIZE(layouts) == numDescSets);
    VkDescriptorSetAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    allocInfo.descriptorPool = pool;
    allocInfo.pSetLayouts = layouts;
    allocInfo.descriptorSetCount = numDescSets;

    VK_CHECK( vkAllocateDescriptorSets(vk_device, &allocInfo, descSets)  );
}


static
void updateDescriptorSet(Buffer_t ubo_buffer, u32 offset, u32 range,
                         VkDescriptorSet *descSets) {
    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = ubo_buffer.buffer;
    bufferInfo.offset = offset;
    bufferInfo.range  = range;

    VkWriteDescriptorSet writeDescriptorSet = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    writeDescriptorSet.dstSet = descSets[0];
    writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeDescriptorSet.dstBinding = 0;
    writeDescriptorSet.pBufferInfo = &bufferInfo;
    writeDescriptorSet.descriptorCount = 1;

    // We write to the descriptorSet specified in writeDescriptorSet.dstSet
    vkUpdateDescriptorSets(vk_device, 1, &writeDescriptorSet, 0, nullptr);
}

static
VkPipeline
createGraphicsPipeline(VkDevice device, VkPipelineCache cache, VkRenderPass rp, Shader_t &vs,
                       Shader_t &fs, VkPipelineLayout layout, VertexDescriptions_t *vtxDescs) {
    VkGraphicsPipelineCreateInfo createInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };

    std::vector<VkPipelineShaderStageCreateInfo> stages;
    {
        VkPipelineShaderStageCreateInfo v_stage = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        v_stage.stage = vs.stage;
        v_stage.module = vs.module;
        v_stage.pName = "main";
        stages.push_back(v_stage);

        VkPipelineShaderStageCreateInfo f_stage = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        f_stage.stage = fs.stage;
        f_stage.module = fs.module;
        f_stage.pName = "main";
        stages.push_back(f_stage);
    }

    createInfo.stageCount = (u32)stages.size();
    createInfo.pStages = stages.data();

    createInfo.pVertexInputState = &vtxDescs->inputState;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    createInfo.pInputAssemblyState = &inputAssembly;

    VkPipelineViewportStateCreateInfo viewportState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;
    createInfo.pViewportState = &viewportState;

    VkPipelineRasterizationStateCreateInfo rasterizationState = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
    rasterizationState.lineWidth = 1.f;
    rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizationState.cullMode = VK_CULL_MODE_NONE; //VK_CULL_MODE_BACK_BIT;
    createInfo.pRasterizationState = &rasterizationState;

    VkPipelineMultisampleStateCreateInfo multisampleState = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
    multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    createInfo.pMultisampleState = &multisampleState;

    VkPipelineDepthStencilStateCreateInfo depthStencilState = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
    depthStencilState.depthTestEnable = VK_TRUE;
    depthStencilState.depthWriteEnable = VK_TRUE;
    depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencilState.minDepthBounds = 0.0f; // Optional
    depthStencilState.maxDepthBounds = 1.0f; // Optional
    createInfo.pDepthStencilState = &depthStencilState;

    VkPipelineColorBlendAttachmentState colorAttachmentState = {};
    colorAttachmentState.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo colorBlendState = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
    colorBlendState.attachmentCount = 1;
    colorBlendState.pAttachments = &colorAttachmentState;
    createInfo.pColorBlendState = &colorBlendState;

    VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

    VkPipelineDynamicStateCreateInfo dynamicState = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
    dynamicState.dynamicStateCount = sizeof(dynamicStates) / sizeof(dynamicStates[0]);
    dynamicState.pDynamicStates = dynamicStates;
    createInfo.pDynamicState = &dynamicState;

    createInfo.layout = layout;
    createInfo.renderPass = rp;

    VkPipeline pipeline = 0;
    VK_CHECK(vkCreateGraphicsPipelines(device, pipelineCache, 1, &createInfo, 0, &pipeline));

    return pipeline;
}

static
VertexDescriptions_t getVertexDescriptions() {
    VertexDescriptions_t vtx_descs = {};

    vtx_descs.bindings.resize(1);
    vtx_descs.bindings[0].binding = 0;
    vtx_descs.bindings[0].stride = sizeof(Vertex_t);
    vtx_descs.bindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    vtx_descs.attributes.resize(2);
    vtx_descs.attributes[0].binding = 0;
    vtx_descs.attributes[0].location = 0;
    vtx_descs.attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    vtx_descs.attributes[0].offset = offsetof(Vertex_t, pos);

    vtx_descs.attributes[1].binding = 0;
    vtx_descs.attributes[1].location = 1;
    vtx_descs.attributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    vtx_descs.attributes[1].offset = offsetof(Vertex_t, normal);

    vtx_descs.inputState = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vtx_descs.inputState.vertexBindingDescriptionCount = (u32) vtx_descs.bindings.size();
    vtx_descs.inputState.pVertexBindingDescriptions = vtx_descs.bindings.data();
    vtx_descs.inputState.vertexAttributeDescriptionCount = (u32) vtx_descs.attributes.size();
    vtx_descs.inputState.pVertexAttributeDescriptions = vtx_descs.attributes.data();

    return vtx_descs;
}

static VkShaderModule loadShaderModule(const char *fileName, VkDevice device)
{
    std::ifstream is(fileName, std::ios::binary | std::ios::in | std::ios::ate);

    if (is.is_open())
    {
        size_t size = is.tellg();
        is.seekg(0, std::ios::beg);
        char* shaderCode = new char[size];
        is.read(shaderCode, size);
        is.close();

        ASSERT(size > 0);

        VkShaderModule shaderModule;
        VkShaderModuleCreateInfo moduleCreateInfo{};
        moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        moduleCreateInfo.codeSize = size;
        moduleCreateInfo.pCode = (u32*)shaderCode;

        VK_CHECK( vkCreateShaderModule(device, &moduleCreateInfo, NULL, &shaderModule) );

        delete[] shaderCode;

        return shaderModule;
    }
    else
    {
        Logger::Fatal("Could not open shader file %s", fileName);
        return VK_NULL_HANDLE;
    }
}

bool loadShader(Shader_t& shader, VkDevice device, std::string fileName, VkShaderStageFlagBits stage)
{
    VkShaderModule module = loadShaderModule(fileName.c_str(), device);
    if (module == VK_NULL_HANDLE)
    {
        return false;
    }
    shader.stage = stage;
    shader.module = module;

    return true;
}

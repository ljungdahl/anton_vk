struct Shader_t
{
    VkShaderModule module;
    VkShaderStageFlagBits stage;
};

struct RenderProg_t
{
    VkPipelineBindPoint bindPoint;
    VkPipelineLayout layout;
    VkDescriptorSetLayout setLayout;
};

struct VertexDescriptions_t {
    VkPipelineVertexInputStateCreateInfo inputState;
    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;
};

using Shaders = std::initializer_list<const Shader_t*>;

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
        Logger::Error("Error: Could not open shader file %s", fileName);
        return VK_NULL_HANDLE;
    }
}

bool loadShader(Shader_t* shader, VkDevice device, std::string fileName, VkShaderStageFlagBits stage)
{
    
    VkShaderModule module = loadShaderModule(fileName.c_str(), device);
    if (module == VK_NULL_HANDLE)
    {
        return false;
    }
    shader->stage = stage;
    shader->module = module;
    
    return true;
}

static VkDescriptorSetLayout createDescriptorSetLayout(VkDevice device)
{
    VkDescriptorSetLayoutBinding uboLayoutBinding = {};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr; //Optional

    VkDescriptorSetLayoutBinding bindings[1] = {uboLayoutBinding};

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = sizeof(bindings) / sizeof(bindings[0]);
    layoutInfo.pBindings = bindings;

    VkDescriptorSetLayout setLayout = VK_NULL_HANDLE;
    VK_CHECK( vkCreateDescriptorSetLayout(device,
                                          &layoutInfo, nullptr,
                                          &setLayout) );
    return setLayout;
}

static VkPipelineLayout createPipelineLayout(VkDevice device, VkDescriptorSetLayout setLayout)
{
    VkPipelineLayoutCreateInfo createInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    createInfo.setLayoutCount = 1;
    createInfo.pSetLayouts = &setLayout;

    VkPipelineLayout layout = 0;
    VK_CHECK(vkCreatePipelineLayout(device, &createInfo, 0, &layout));

    return layout;
}

RenderProg_t createRenderProg(VkDevice device, VkPipelineBindPoint bindPoint) {

    RenderProg_t prog = {};
    
    prog.bindPoint = bindPoint;
    
    prog.setLayout = createDescriptorSetLayout(device);
    ASSERT(prog.setLayout != VK_NULL_HANDLE);

    prog.layout = createPipelineLayout(device, prog.setLayout);
    ASSERT(prog.layout != VK_NULL_HANDLE);

    return prog;
}

VkPipeline createGraphicsPipeline(VkDevice device, VkPipelineCache pipelineCache,
                                  VkRenderPass renderPass, Shaders shaders,
                                  VkPipelineLayout layout, VertexDescriptions_t* vtxDescs)
{    
    VkGraphicsPipelineCreateInfo createInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
    
    std::vector<VkPipelineShaderStageCreateInfo> stages;
    for (const Shader_t* shader : shaders)
    {
        VkPipelineShaderStageCreateInfo stage = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        stage.stage = shader->stage;
        stage.module = shader->module;
        stage.pName = "main";

        stages.push_back(stage);
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
    rasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
    createInfo.pRasterizationState = &rasterizationState;

    VkPipelineMultisampleStateCreateInfo multisampleState = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
    multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    createInfo.pMultisampleState = &multisampleState;

    VkPipelineDepthStencilStateCreateInfo depthStencilState = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
    depthStencilState.depthTestEnable = true;
    depthStencilState.depthWriteEnable = true;
    depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;
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
    createInfo.renderPass = renderPass;

    VkPipeline pipeline = 0;
    VK_CHECK(vkCreateGraphicsPipelines(device, pipelineCache, 1, &createInfo, 0, &pipeline));

    return pipeline;
}

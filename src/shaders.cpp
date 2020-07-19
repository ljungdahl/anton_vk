struct Shader_t
{
    VkShaderModule module;
    VkShaderStageFlagBits stage;
};

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

VkDescriptorPool createDescriptorPool(VkDevice device, u32 numberOfModels)
{

    VkDescriptorPoolSize poolSizes[1];
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = numberOfModels;

    VkDescriptorPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    createInfo.poolSizeCount = ARRAYSIZE(poolSizes);
    createInfo.pPoolSizes = poolSizes;
    createInfo.maxSets = 16;

    VkDescriptorPool pool = 0;
    VK_CHECK( vkCreateDescriptorPool(device, &createInfo, nullptr, &pool) );
    return pool;
}

VkDescriptorSetLayout createDescriptorSetLayout(VkDevice device)
{
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
    VK_CHECK( vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &layout) );
    return layout;
}

void allocateDescriptorSet(VkDevice device, VkDescriptorPool pool, VkDescriptorSetLayout layout, VkDescriptorSet* descSets, u32 numDescSets)
{
    VkDescriptorSetLayout layouts[1] = {layout};
    ASSERT( ARRAYSIZE(layouts) == numDescSets);
    VkDescriptorSetAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    allocInfo.descriptorPool = pool;
    allocInfo.pSetLayouts = layouts;
    allocInfo.descriptorSetCount = numDescSets;

    VK_CHECK( vkAllocateDescriptorSets(device, &allocInfo, descSets)  );
}
void updateDescriptorSet(VkDevice device, Buffer_t uboBuffer, u32 offset, u32 range, VkDescriptorSet* descSets)
{
    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = uboBuffer.buffer;
    bufferInfo.offset = offset;
    bufferInfo.range = range;
            
    VkWriteDescriptorSet writeDescriptorSet = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    writeDescriptorSet.dstSet = descSets[0];
    writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeDescriptorSet.dstBinding = 0;
    writeDescriptorSet.pBufferInfo = &bufferInfo;
    writeDescriptorSet.descriptorCount = 1;

    // We write to the descriptorSet specified in writeDescriptorSet.dstSet
    vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);
}

VkPipeline createGraphicsPipeline(VkDevice device, VkPipelineCache pipelineCache,
                                  VkRenderPass renderPass, Shader_t& vs, Shader_t& fs,
                                  VkPipelineLayout layout, VertexDescriptions_t* vtxDescs)
{    
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
    createInfo.renderPass = renderPass;

    VkPipeline pipeline = 0;
    VK_CHECK(vkCreateGraphicsPipelines(device, pipelineCache, 1, &createInfo, 0, &pipeline));

    return pipeline;
}

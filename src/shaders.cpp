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

bool loadShader(Shader_t* shader, VkDevice device, std::string fileName, VkShaderStageFlagBits stage) {
    
    VkShaderModule module = loadShaderModule(fileName.c_str(), device);
    if (module == VK_NULL_HANDLE) {
        return false;
    }
    shader->stage = stage;
    shader->module = module;
    
    return true;
}


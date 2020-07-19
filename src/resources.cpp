struct Buffer_t
{
    VkBuffer buffer;
    VmaAllocation vmaAlloc;
    VmaAllocationInfo vmaInfo;
    size_t size;
};

struct Image_t
{
    VkImage image;
    VkImageView view;
    VmaAllocation vmaAlloc;
    VmaAllocationInfo vmaInfo;
};

void createBuffer(Buffer_t& result, 
                  VkBufferUsageFlags usage, VmaMemoryUsage vmaUsage,
                  u32 size,
                  VmaAllocator& vma_allocator)
{
    VkBufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    createInfo.size = size;
    createInfo.usage = usage;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo vmaCreateInfo = {};
    vmaCreateInfo.usage = vmaUsage; //NOTE(anton): If I specify usage they memory type flags will be setup automatically
        
    result.vmaAlloc = VK_NULL_HANDLE;
    result.vmaInfo = {};
    result.buffer = VK_NULL_HANDLE;
    VK_CHECK( vmaCreateBuffer(vma_allocator, &createInfo, &vmaCreateInfo,
                              &result.buffer,
                              &result.vmaAlloc,
                              &result.vmaInfo) );

    result.size = size;
}

void uploadBuffer(VkDevice device, VkCommandPool commandPool,
                  VkCommandBuffer commandBuffer,
                  VkQueue queue,
                  const Buffer_t& buffer, const Buffer_t& scratch,
                  const void* data, u32 size, VmaAllocator& vma_allocator)
{
    void *mappedData;
    vmaMapMemory(vma_allocator, scratch.vmaAlloc, &mappedData);
    ASSERT(scratch.size >= size);
    memcpy(mappedData, data, size);
    vmaUnmapMemory(vma_allocator, scratch.vmaAlloc);

    VK_CHECK( vkResetCommandPool(device, commandPool, 0));

    VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK( vkBeginCommandBuffer(commandBuffer, &beginInfo) );

    VkBufferCopy region = { 0, 0, (VkDeviceSize)size };
    vkCmdCopyBuffer(commandBuffer, scratch.buffer, buffer.buffer, 1, &region);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    VK_CHECK(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

    VK_CHECK(vkDeviceWaitIdle(device));
}

void createImage(Image_t& result, VkDevice device,
                 u32 width, u32 height, VkFormat format, VkImageUsageFlags usage,
                 VkImageAspectFlags aspectMask, VmaAllocator& vma_allocator)
{
    VkImageCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };

    createInfo.imageType = VK_IMAGE_TYPE_2D;
    createInfo.format = format;
    createInfo.extent = { width, height, 1 };
    createInfo.mipLevels = 1;
    createInfo.arrayLayers = 1;
    createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    createInfo.usage = usage;
    createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    
    VmaAllocationCreateInfo vmaCreateInfo = {};
    vmaCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    vmaCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    result.vmaAlloc = VK_NULL_HANDLE;
    result.vmaInfo = {};
    result.image = VK_NULL_HANDLE;
    VK_CHECK( vmaCreateImage(vma_allocator, &createInfo, &vmaCreateInfo,
                             &result.image,
                             &result.vmaAlloc,
                             &result.vmaInfo) );

    VkImageViewCreateInfo viewCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    viewCreateInfo.image = result.image;
    viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewCreateInfo.format = format;
    viewCreateInfo.subresourceRange.aspectMask = aspectMask;
    viewCreateInfo.subresourceRange.baseMipLevel = 0;
    viewCreateInfo.subresourceRange.levelCount = 1;
    viewCreateInfo.subresourceRange.layerCount = 1;

    VK_CHECK(vkCreateImageView(device, &viewCreateInfo, 0, &result.view));
}

void destroyImage(Image_t& image, VkDevice device, VmaAllocator& vma_allocator)
{
    vmaDestroyImage(vma_allocator, image.image, image.vmaAlloc);
}

VkImageMemoryBarrier imageMemoryBarrier(VkImage image, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageAspectFlags aspectMask)
{
    VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    barrier.srcAccessMask = srcAccessMask;
    barrier.dstAccessMask = dstAccessMask;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = aspectMask;
    barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

    return barrier;
}

void loadObj(std::string ModelPath, Mesh_t* mmModel) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    Logger::Trace("Loading obj model from path %s", ModelPath.c_str());
    
    if(!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, ModelPath.c_str())) {
        Logger::Fatal("tinyobj::LoadObj failed!");
    }

    std::unordered_map<Vertex_t, size_t> unique_vertices;
    
    for (const auto& shape: shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex_t vertex = {};

            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            vertex.normal = {
                attrib.normals[3 * index.normal_index + 0],
                attrib.normals[3 * index.normal_index + 1],
                attrib.normals[3 * index.normal_index + 2]                
            };            

            if(unique_vertices.count(vertex) == 0) {
                unique_vertices[vertex] = (u32)mmModel->vertices.size();
                mmModel->vertices.push_back(vertex);
            }

            mmModel->indices.push_back((u32)unique_vertices[vertex]);
        }
    }

    
    Logger::Trace("Number of vertices: %i", (u32)mmModel->vertices.size());
    Logger::Trace("Number of indices: %i", (u32)mmModel->indices.size());
    mmModel->indexCount = (u32)mmModel->indices.size();
}

void generateCube(Mesh_t& mesh)
{
    // Setup vertices indices for a colored cube
    std::vector<glm::vec3> cubeVerts =
        {
            glm::vec3( -1.0f, -1.0f,  1.0f ),
            glm::vec3(  1.0f, -1.0f,  1.0f ),
            glm::vec3(  1.0f,  1.0f,  1.0f ),
            glm::vec3( -1.0f,  1.0f,  1.0f ),
            glm::vec3( -1.0f, -1.0f, -1.0f ),
            glm::vec3(  1.0f, -1.0f, -1.0f ),
            glm::vec3(  1.0f,  1.0f, -1.0f ),
            glm::vec3( -1.0f,  1.0f, -1.0f ),
        };
    
    std::vector<u32> cubeIndices = { 
        0,1,2, 2,3,0, 1,5,6, 6,2,1, 7,6,5, 5,4,7, 4,0,3, 3,7,4, 4,5,1, 1,0,4, 3,2,6, 6,7,3, 
    };

    mesh.vertices.resize(cubeVerts.size());
    u32 i = 0;
    for (auto& vertex : mesh.vertices)
    {
        vertex.pos = cubeVerts[i];
        i++;
    }

    mesh.indices.resize(cubeIndices.size());
    i = 0;
    for (auto index : cubeIndices) {
        mesh.indices[i] = index;
        i++;
    }
    
    mesh.indexCount = (u32)cubeIndices.size();
}

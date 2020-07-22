#pragma once
#include "vk_common.h"

void setupFirstTimeRenderprogs(u32 uboSize, std::vector<glm::vec4> &pc);

static
VkDescriptorPool createDescriptorPool();

static
VkDescriptorSetLayout createDescriptorSetLayout();

static
void allocateDescriptorSet(VkDescriptorPool pool, VkDescriptorSetLayout layout,
                           VkDescriptorSet *descSets, u32 numDescSets);

static
void updateDescriptorSet(Buffer_t ubo_buffer, u32 offset, u32 range, VkDescriptorSet* descSets);

struct VertexDescriptions_t; //Fwd declare
static
VkPipeline createGraphicsPipeline(VkDevice device, VkPipelineCache cache,
                                  VkRenderPass rp, Shader_t& vs, Shader_t& fs,
                                  VkPipelineLayout layout, VertexDescriptions_t* vtxDescs);

static
VkPipelineLayout createPipelineLayout(VkDevice device, VkDescriptorSetLayout *dcLayout,
                                      VkPushConstantRange *pcRange);

void initialDescriptorSetup();
void initialShaderLoad();
void initialPipelineCreation();
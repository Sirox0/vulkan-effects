#ifndef VK_MODEL_H
#define VK_MODEL_H

#include <vulkan/vulkan.h>
#include <assimp/scene.h>
#include <cglm/cglm.h>

typedef struct {
    vec3 position;
    vec3 normal;
    vec3 tangent;
    vec2 uv;
} vk_model_vertex_t;

typedef struct {
    VkBuffer vertexBuffer;
    VkBuffer indexBuffer;
    VkBuffer indirectBuffer;
    u32 drawCount;
} vk_model_t;


/**
 * initialize the log stream
 */
void vkModelInitLogStream();

/**
 * attach (enable) log stream
 */
void vkModelAttachLogStream();

/**
 * detach (disable) log stream
 */
void vkModelDetachLogStream();

/**
 * loads scene from a file
 * @param path the relative path to model file
 */
const struct aiScene* vkModelLoadScene(const char* path);

/**
 * query the sizes (in bytes) of buffers for the model
 * @param scene the scene to query sizes from
 * @param pVertexSize pointer to an output integer for vertex buffer size
 * @param pIndexSize pointer to an output integer for index buffer size
 */
void vkModelGetSizes(const struct aiScene* scene, u32* pVertexSize, u32* pIndexSize, u32* pIndirectSize);

/**
 * initialize a model from a scene
 * @param scene scene object from which model will be initialized
 * @param tempCmdBuf command buffer to which transfer operations will be recorded
 * @param tempBuffer temporary HOST_VISIBLE (flushing must be handled by user, or use HOST_COHERENT) buffer from which data will be copied
 * @param tempBufferVertexOffset offset of vertex data within tempBuffer
 * @param tempBufferIndexOffset offset of index data within tempBuffer
 * @param tempBufferIndirectOffset offset of indirect data within tempBuffer
 * @param pTempBufferRaw pointer mapped memory of tempBuffer
 * @param pModel pointer to a vk_model_t object that will be initialized,
 *               vk_model_t::vertexBuffer and vk_model_t::indexBuffer are expected to be already allocated by user
*/
void vkModelInit(const struct aiScene* scene, VkCommandBuffer tempCmdBuf, VkBuffer tempBuffer, VkDeviceSize tempBufferVertexOffset, VkDeviceSize tempBufferIndexOffset, VkDeviceSize tempBufferIndirectOffset, void* pTempBufferRaw, vk_model_t* pModel);

/**
 * unload a scene object loaded with vkModelLoadScene
 * @param scene the scene to unload
 */
void vkModelUnloadScene(const struct aiScene* scene);

#endif
#ifndef VK_MODEL_H
#define VK_MODEL_H

#include <vulkan/vulkan.h>
#include <assimp/scene.h>
#include <cglm/cglm.h>

typedef struct {
    vec3 position;
} vk_model_vertex_t;

typedef struct {
    VkBuffer vertexBuffer;
    VkBuffer indexBuffer;
} vk_model_t;


/**
 * initialize the log stream
 */
void vkModelInitLogStream();

/**
 * detach (disable) log stream
 */
void vkModelDetachLogStream();

/**
 * loads scene from a file
 * @param path the relative path to model file
 */
struct aiScene* vkModelLoadScene(const char* path);

/**
 * query the sizes (in bytes) of buffers for the model
 * @param scene the scene to query sizes from
 * @param pVertexSize pointer to an output integer for vertex buffer size
 * @param pIndexSize pointer to an output integer for index buffer size
 */
void vkModelGetSizes(struct aiScene* scene, u32* pVertexSize, u32* pIndexSize);

/**
 * initialize a model from a scene
 * @param tempCmdBuf command buffer to which transfer operations will be recorded
 * @param tempBuffer temporary HOST_VISIBLE (flushing must be handled by user, or use HOST_COHERENT) buffer from which data will be copied
 * @param tempBufferOffset offset within tempBuffer to which model data will be written
 * @param pTempBufferRaw pointer mapped memory of tempBuffer
 * @param scene scene object from which model will be initialized
 * @param pModel pointer to a vk_model_t object that will be initialized,
 *               vk_model_t::vertexBuffer and vk_model_t::indexBuffer are expected to be already allocated by user
*/
void vkModelInit(VkCommandBuffer tempCmdBuf, VkBuffer tempBuffer, VkDeviceSize tempBufferVertexOffset, VkDeviceSize tempBufferIndexOffset, void* pTempBufferRaw, struct aiScene* scene, vk_model_t* pModel);

/**
 * unload a scene object loaded with vkModelLoadScene
 * @param scene the scene to unload
 */
void vkModelUnloadScene(struct aiScene* scene);

/**
 * loads scene from a file
 * @param path the relative path to model file
 */
struct aiScene* vkModelLoadScene(const char* path);

#endif
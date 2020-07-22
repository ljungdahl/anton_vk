#define TINYOBJLOADER_IMPLEMENTATION
#include "../external/tiny_obj_loader.h"

#include "scene.h"

static
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

void setupScene(std::vector<Mesh_t> &meshList, VPmatrices_t &vpMats, u32 width, u32 height) {

    {
        Mesh_t mesh1;
        loadObj("../../assets/coords2_soft.obj", &mesh1);

        mesh1.modelMatrix = glm::mat4(1.0f);
        meshList.push_back(mesh1);

        mesh1.isStatic = true;
    }

    {
        Mesh_t mesh2;
        loadObj("../../assets/cube.obj", &mesh2);

        mesh2.modelMatrix = glm::mat4(1.0f);
        meshList.push_back(mesh2);

        mesh2.isStatic = true;
    }

    {
        Mesh_t mesh3;
        loadObj("../../assets/bunny_soft.obj", &mesh3);

        mesh3.modelMatrix = glm::mat4(1.0f);
        mesh3.modelMatrix = glm::translate(mesh3.modelMatrix, glm::vec3(-2.0f, 0.0f, 0.0f));
        meshList.push_back(mesh3);

        mesh3.isStatic = true;
    }

    {
        glm::vec3 initCameraPos = glm::vec3(4.0f, 3.0f, 7.0f);

        glm::mat4 initView = glm::lookAt(initCameraPos, // eye
                                         glm::vec3(0.0f, 0.0f, 0.0f), // center
                                         glm::vec3(0.0f, 1.0f, 0.0f)); // up

        glm::mat4 initProj = glm::perspective(glm::radians(40.0f),
                                              (f32) width / (f32) height,
                                              0.1f, 256.0f);

        initProj *= -1;

        vpMats.view = initView;
        vpMats.proj = initProj;
    }

}


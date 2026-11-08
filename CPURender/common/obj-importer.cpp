#include "obj-importer.h"

#include <fstream>
#include <vector>
#include <unordered_map>
#include <cstring>
#include <cstdlib>

#include "geometry.h"

struct VertexKey {
    int pos, uv, norm;
    bool operator==(const VertexKey& other) const {
        return pos == other.pos && uv == other.uv && norm == other.norm;
    }
};

struct VertexKeyHash {
    size_t operator()(const VertexKey& k) const {
        return std::hash<int>()(k.pos) ^ (std::hash<int>()(k.uv) << 1) ^ (std::hash<int>()(k.norm) << 2);
    }
};

Geometry ObjImporter::loadFromFile(const std::string& path) {
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file.is_open()) return {};

    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> texCoords;
    std::vector<glm::vec3> normals;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::unordered_map<VertexKey, uint32_t, VertexKeyHash> vertexMap;

    // Preallocate some memory
    positions.reserve(10000);
    texCoords.reserve(10000);
    normals.reserve(10000);
    vertices.reserve(20000);
    indices.reserve(60000);

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        const char* str = line.c_str();

        if (strncmp(str, "v ", 2) == 0) {
            str += 2;
            float x = std::strtof(str, const_cast<char**>(&str));
            float y = std::strtof(str, const_cast<char**>(&str));
            float z = std::strtof(str, const_cast<char**>(&str));
            positions.push_back({ x, y, z });
        }
        else if (strncmp(str, "vn ", 3) == 0) {
            str += 3;
            float x = std::strtof(str, const_cast<char**>(&str));
            float y = std::strtof(str, const_cast<char**>(&str));
            float z = std::strtof(str, const_cast<char**>(&str));
            normals.push_back({ x, y, z });
        }
        else if (strncmp(str, "vt ", 3) == 0) {
            str += 3;
            float u = std::strtof(str, const_cast<char**>(&str));
            float v = std::strtof(str, const_cast<char**>(&str));
            texCoords.push_back({ u, /*1.0f -*/ v }); // Flip the v to have the origin in top left corner
        }
        else if (strncmp(str, "f ", 2) == 0) {
            str += 2;
            std::vector<uint32_t> faceIndices;

            while (*str) {
                while (*str == ' ') ++str;
                if (!*str) break;

                int posIdx = 0, uvIdx = 0, normIdx = 0;
                sscanf_s(str, "%d/%d/%d", &posIdx, &uvIdx, &normIdx);

                // Move to next space
                while (*str && *str != ' ') ++str;

                VertexKey key = { posIdx, uvIdx, normIdx };
                auto it = vertexMap.find(key);
                uint32_t vertIndex;
                if (it != vertexMap.end()) {
                    vertIndex = it->second;
                } else {
                    Vertex v = {};
                    v.color = {1.0f, 1.0f, 1.0f, 1.0f};
                    v.normal = {0.0f, 1.0f, 0.0f};
                    v.uv = {0.0f, 0.0f};

                    if (posIdx > 0 && static_cast<size_t>(posIdx) <= positions.size())
                        v.pos = positions[posIdx - 1];
                    if (uvIdx > 0 && static_cast<size_t>(uvIdx) <= texCoords.size())
                        v.uv = texCoords[uvIdx - 1];
                    if (normIdx > 0 && static_cast<size_t>(normIdx) <= normals.size())
                        v.normal = normals[normIdx - 1];

                    vertIndex = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(v);
                    vertexMap[key] = vertIndex;
                }

                faceIndices.push_back(vertIndex);
            }
            
            // Triangulate polygon
            for (size_t i = 1; i + 1 < faceIndices.size(); ++i) {
                indices.push_back(faceIndices[0]);
                indices.push_back(faceIndices[i]);
                indices.push_back(faceIndices[i + 1]);
                
                //Calculate Tangent and Bitangent
                glm::vec3 pos1 = vertices[faceIndices[0]].pos;
                glm::vec2 uv1 = vertices[faceIndices[0]].uv;
            
                glm::vec3 pos2 = vertices[faceIndices[i]].pos;
                glm::vec2 uv2 = vertices[faceIndices[i]].uv;
            
                glm::vec3 pos3 = vertices[faceIndices[i + 1]].pos;
                glm::vec2 uv3 = vertices[faceIndices[i + 1]].uv;
            
                glm::vec3 edge1 = pos2 - pos1;
                glm::vec3 edge2 = pos3 - pos1;
                glm::vec2 deltaUV1 = uv2 - uv1;
                glm::vec2 deltaUV2 = uv3 - uv1; 
            
                float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

                glm::vec3 tangent;
                tangent = f * (deltaUV2.y * edge1 - deltaUV1.y * edge2);
                vertices[faceIndices[0]].tangent += tangent;
                vertices[faceIndices[i]].tangent += tangent;
                vertices[faceIndices[i + 1]].tangent += tangent;
            }
        }
    }
    
    for (uint32 i = 0; i < vertices.size(); i++) {
        glm::vec3 T = glm::normalize(vertices[i].tangent);
        glm::vec3 N = glm::normalize(vertices[i].normal);
        T = glm::normalize(T - glm::dot(T, N) * N);
        vertices[i].biTangent = glm::cross(N,T);
    }

    file.close();

    if (vertices.empty() || indices.empty()) return {};
    
    return Geometry{vertices, indices};
}
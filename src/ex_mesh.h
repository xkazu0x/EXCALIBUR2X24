#pragma once

#include "ex_logger.h"
#include "ex_vertex.h"

#include <vector>

namespace ex {
    class mesh {
    public:
        void create(const char *path);
        void create_from_array(std::vector<ex::vertex> &vertices,
                               std::vector<uint32_t> &indices);
        std::vector<ex::vertex> vertices();
        std::vector<uint32_t> indices();
        
    private:
        std::vector<ex::vertex> m_vertices;
        std::vector<uint32_t> m_indices;
    };
}

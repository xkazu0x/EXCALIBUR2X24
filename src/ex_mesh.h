#pragma once

#include "ex_logger.h"
#include "ex_vertex.h"

#include <vector>

namespace ex {
    class mesh {
    public:
        void load_file(const char *file_path);
        void load_array(std::vector<ex::vertex> &vertices, std::vector<uint32_t> &indices);
        
        std::vector<ex::vertex> vertices() { return m_vertices; }
        std::vector<uint32_t> indices() { return m_indices; }
        
    private:
        std::vector<ex::vertex> m_vertices;
        std::vector<uint32_t> m_indices;
    };
}

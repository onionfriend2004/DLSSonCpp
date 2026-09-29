#pragma once
 
#include <cstdint>
#include <vector>
 
#include "graph.hpp"
 
namespace gc {
 
// Граф -> сжатый бинарный поток.
std::vector<uint8_t> Serialize(const std::vector<Edge>& edges);
// Сжатый поток -> рёбра (порядок и ориентация могут отличаться от исходных).
std::vector<Edge> Deserialize(const std::vector<uint8_t>& bytes);
 
}  // namespace gc
 
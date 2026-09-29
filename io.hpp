#pragma once
 
#include <cstdint>
#include <string>
#include <vector>
 
#include "graph.hpp"
 
namespace gc {
 
std::vector<uint8_t> ReadFile(const std::string& path);
void WriteFile(const std::string& path, const std::vector<uint8_t>& data);
 
std::vector<Edge> ReadTsv(const std::string& path);
void WriteTsv(const std::string& path, const std::vector<Edge>& edges);
 
}  // namespace gc
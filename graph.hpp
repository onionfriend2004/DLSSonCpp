#pragma once
 
#include <cstdint>
 
namespace gc {
 
// Неориентированное ребро: идентификаторы концов и вес.
struct Edge {
  uint32_t u;
  uint32_t v;
  uint8_t w;
};
 
}  // namespace gc
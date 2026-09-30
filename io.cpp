#include "io.hpp"

#include <fstream>
#include <stdexcept>

namespace gc {

std::vector<uint8_t> ReadFile(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) throw std::runtime_error("cannot open " + path);
  in.seekg(0, std::ios::end);
  std::vector<uint8_t> data(static_cast<size_t>(in.tellg()));
  in.seekg(0);
  in.read(reinterpret_cast<char*>(data.data()),
          static_cast<std::streamsize>(data.size()));
  return data;
}

void WriteFile(const std::string& path, const std::vector<uint8_t>& data) {
  std::ofstream out(path, std::ios::binary);
  if (!out) throw std::runtime_error("cannot write " + path);
  out.write(reinterpret_cast<const char*>(data.data()),
            static_cast<std::streamsize>(data.size()));
}

std::vector<Edge> ReadTsv(const std::string& path) {
  const std::vector<uint8_t> text = ReadFile(path);
  std::vector<Edge> edges;
  size_t pos = 0;

  // Читает следующее число, пропуская любые разделители (\t, \n, \r).
  auto next = [&](uint64_t& value) {
    while (pos < text.size() && (text[pos] < '0' || text[pos] > '9')) ++pos;
    if (pos == text.size()) return false;
    value = 0;
    while (pos < text.size() && text[pos] >= '0' && text[pos] <= '9')
      value = value * 10 + (text[pos++] - '0');
    return true;
  };

  uint64_t u, v, w;
  while (next(u) && next(v) && next(w)) {
    edges.push_back({static_cast<uint32_t>(u), static_cast<uint32_t>(v),
                     static_cast<uint8_t>(w)});
  }
  return edges;
}

void WriteTsv(const std::string& path, const std::vector<Edge>& edges) {
  std::string text;
  text.reserve(edges.size() * 28);
  for (const Edge& e : edges) {
    text += std::to_string(e.u);
    text += '\t';
    text += std::to_string(e.v);
    text += '\t';
    text += std::to_string(e.w);
    text += '\n';
  }
  WriteFile(path, std::vector<uint8_t>(text.begin(), text.end()));
}

}  // namespace gc
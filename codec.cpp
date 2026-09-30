#include "codec.hpp"

#include <algorithm>

#include "num_model.hpp"
#include "range_coder.hpp"

// Формат потока (всё идёт через один range coder):
//   1) N — число вершин (32 бита);
//   2) отсортированные id вершин: кодируются разности соседних id;
//   3) для каждой вершины hi = 0..N-1 (id заменены на индексы 0..N-1):
//        k — число соседей с индексом <= hi (каждое ребро хранится один раз,
//            у большего конца), затем k отсортированных соседей lo как разности
//            (+ 8 сырых бит веса на каждое ребро).
// Допущение из условия: кратных рёбер нет (иначе разности были бы < 0).

namespace gc {
namespace {

constexpr int kMaxGapContexts = 40;

// Ребро в индексах вершин, всегда hi >= lo (петля: hi == lo).
struct IndexedEdge {
  uint32_t hi;
  uint32_t lo;
  uint8_t w;
};

// Шаг 1: все встречающиеся id, отсортированные и без повторов.
std::vector<uint32_t> CollectIds(const std::vector<Edge>& edges) {
  std::vector<uint32_t> ids;
  ids.reserve(edges.size() * 2);
  for (const Edge& e : edges) {
    ids.push_back(e.u);
    ids.push_back(e.v);
  }
  std::sort(ids.begin(), ids.end());
  ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
  return ids;
}

// Шаг 2: id -> индекс (бинарный поиск), нормализация концов, сортировка.
std::vector<IndexedEdge> ToIndexed(const std::vector<Edge>& edges,
                                   const std::vector<uint32_t>& ids) {
  auto index_of = [&](uint32_t id) {
    return static_cast<uint32_t>(
        std::lower_bound(ids.begin(), ids.end(), id) - ids.begin());
  };
  std::vector<IndexedEdge> result;
  result.reserve(edges.size());
  for (const Edge& e : edges) {
    const uint32_t a = index_of(e.u), b = index_of(e.v);
    result.push_back({std::max(a, b), std::min(a, b), e.w});
  }
  std::sort(result.begin(), result.end(),
            [](const IndexedEdge& x, const IndexedEdge& y) {
              return x.hi != y.hi ? x.hi < y.hi : x.lo < y.lo;
            });
  return result;
}

int BitLength(uint64_t x) {
  int n = 0;
  for (; x != 0; x >>= 1) ++n;
  return n;
}

// Контекст для разностей: k соседей случайно лежат среди hi+1 кандидатов,
// поэтому средняя разность ~ (hi+1)/k — её порядок и задаёт модель.
int GapContext(uint32_t hi, uint32_t k) {
  return BitLength((static_cast<uint64_t>(hi) + 1) / k);
}

// Кодирование id — одна функция для обоих направлений: кодировщик передаёт
// заполненный массив, декодировщик — нулевой нужного размера.
template <class Coder>
void CodeIds(Coder& coder, std::vector<uint32_t>& ids) {
  NumModel model;
  uint64_t next = 0;  // минимально возможный следующий id
  for (uint32_t& id : ids) {
    id = static_cast<uint32_t>(
        next + CodeNumber(coder, model, static_cast<uint32_t>(id - next)));
    next = static_cast<uint64_t>(id) + 1;
  }
}

void EncodeEdges(Encoder& enc, uint32_t n,
                 const std::vector<IndexedEdge>& edges) {
  NumModel degree_model;
  std::vector<NumModel> gap_models(kMaxGapContexts);
  size_t pos = 0;
  for (uint32_t hi = 0; hi < n; ++hi) {
    size_t end = pos;  // [pos, end) — рёбра с этим hi
    while (end < edges.size() && edges[end].hi == hi) ++end;
    const uint32_t k = static_cast<uint32_t>(end - pos);
    CodeNumber(enc, degree_model, k);

    NumModel& gaps = gap_models[k ? GapContext(hi, k) : 0];
    uint32_t next = 0;
    for (; pos < end; ++pos) {
      CodeNumber(enc, gaps, edges[pos].lo - next);
      enc.Direct(edges[pos].w, 8);  // веса равномерны — сжимать нечего
      next = edges[pos].lo + 1;
    }
  }
}

std::vector<Edge> DecodeEdges(Decoder& dec, const std::vector<uint32_t>& ids) {
  NumModel degree_model;
  std::vector<NumModel> gap_models(kMaxGapContexts);
  std::vector<Edge> edges;
  for (uint32_t hi = 0; hi < ids.size(); ++hi) {
    const uint32_t k = CodeNumber(dec, degree_model, 0);
    NumModel& gaps = gap_models[k ? GapContext(hi, k) : 0];
    uint32_t next = 0;
    for (uint32_t j = 0; j < k; ++j) {
      const uint32_t lo = next + CodeNumber(dec, gaps, 0);
      const uint8_t w = static_cast<uint8_t>(dec.Direct(0, 8));
      edges.push_back({ids[hi], ids[lo], w});
      next = lo + 1;
    }
  }
  return edges;
}

}  // namespace

std::vector<uint8_t> Serialize(const std::vector<Edge>& edges) {
  std::vector<uint32_t> ids = CollectIds(edges);
  const std::vector<IndexedEdge> indexed = ToIndexed(edges, ids);

  Encoder enc;
  enc.Direct(static_cast<uint32_t>(ids.size()), 32);
  CodeIds(enc, ids);
  EncodeEdges(enc, static_cast<uint32_t>(ids.size()), indexed);
  return enc.Finish();
}

std::vector<Edge> Deserialize(const std::vector<uint8_t>& bytes) {
  Decoder dec(bytes);
  const uint32_t n = dec.Direct(0, 32);
  std::vector<uint32_t> ids(n, 0);
  CodeIds(dec, ids);
  return DecodeEdges(dec, ids);
}

}  // namespace gc
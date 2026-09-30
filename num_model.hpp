#pragma once

// Адаптивная модель для неотрицательных целых (Exp-Golomb + арифметика):
//   x = value + 1, n = floor(log2 x)
//   1) n кодируется в унарном виде, каждый бит имеет свою адаптивную вероятность;
//   2) n младших бит x: два старших — адаптивно, остальные — «сырые»
//      (в равномерных данных они всё равно несжимаемы).
// Для геометрически-подобных распределений это близко к энтропии.

#include <cstdint>

#include "range_coder.hpp"

namespace gc {

struct NumModel {
  uint16_t len[33];
  uint16_t mant[33][4];

  NumModel() {
    for (auto& p : len) p = kProbInit;
    for (auto& row : mant)
      for (auto& p : row) p = kProbInit;
  }
};

// Кодирует (Encoder) или декодирует (Decoder) число. При декодировании
// аргумент value игнорируется. Возвращает значение в обоих случаях.
template <class Coder>
uint32_t CodeNumber(Coder& coder, NumModel& m, uint32_t value) {
  const uint64_t x = static_cast<uint64_t>(value) + 1;
  int true_len = 0;  // floor(log2 x), нужен только кодировщику
  while ((x >> (true_len + 1)) != 0) ++true_len;

  // Шаг 1: длина в унарном коде («1» = ещё длиннее, «0» = стоп).
  int n = 0;
  while (n < 32 && coder.Bit(m.len[n], n < true_len)) ++n;

  // Шаг 2: биты мантиссы; r накапливает старшие биты (ведущая 1 в начале).
  uint64_t r = 1;
  for (int i = n - 1; i >= 0; --i) {
    const int bit = static_cast<int>((x >> i) & 1);
    const int out = (r < 4) ? coder.Bit(m.mant[n][r], bit)
                            : static_cast<int>(coder.Direct(bit, 1));
    r = (r << 1) | out;
  }
  return static_cast<uint32_t>(r - 1);
}

}  // namespace gc

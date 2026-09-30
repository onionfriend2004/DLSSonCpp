#pragma once

// Бинарный адаптивный range coder (схема LZMA).
// Encoder и Decoder имеют ОДИНАКОВЫЙ интерфейс (Bit / Direct), поэтому
// модели чисел пишутся один раз для обоих направлений:
// кодировщик возвращает переданное значение, декодировщик игнорирует
// аргумент и возвращает раскодированное.

#include <cstdint>
#include <vector>

namespace gc {

constexpr int kProbBits = 15;   // точность вероятности
constexpr int kAdaptShift = 6;  // скорость адаптации (больше = плавнее)
constexpr uint32_t kTopValue = 1u << 24;
constexpr uint16_t kProbInit = 1 << (kProbBits - 1);  // P(0) = 1/2

class Encoder {
 public:
  // Кодирует бит с адаптивной вероятностью prob = P(bit == 0).
  int Bit(uint16_t& prob, int bit) {
    const uint32_t bound = (range_ >> kProbBits) * prob;
    if (bit == 0) {
      range_ = bound;
      prob += ((1u << kProbBits) - prob) >> kAdaptShift;
    } else {
      low_ += bound;
      range_ -= bound;
      prob -= prob >> kAdaptShift;
    }
    Normalize();
    return bit;
  }

  // Кодирует nbits младших бит value с вероятностью 1/2 (без модели).
  uint32_t Direct(uint32_t value, int nbits) {
    for (int i = nbits - 1; i >= 0; --i) {
      range_ >>= 1;
      if ((value >> i) & 1) low_ += range_;
      Normalize();
    }
    return value;
  }

  // Сбрасывает остаток и возвращает готовый поток байт.
  std::vector<uint8_t> Finish() {
    for (int i = 0; i < 5; ++i) ShiftLow();
    return std::move(out_);
  }

 private:
  void Normalize() {
    while (range_ < kTopValue) {
      range_ <<= 8;
      ShiftLow();
    }
  }

  // Выталкивает старший байт low_ с учётом переноса.
  void ShiftLow() {
    if (static_cast<uint32_t>(low_) < 0xFF000000u || (low_ >> 32) != 0) {
      const uint8_t carry = static_cast<uint8_t>(low_ >> 32);
      uint8_t byte = cache_;
      do {
        out_.push_back(static_cast<uint8_t>(byte + carry));
        byte = 0xFF;
      } while (--cache_size_ != 0);
      cache_ = static_cast<uint8_t>(low_ >> 24);
    }
    ++cache_size_;
    low_ = (low_ & 0x00FFFFFFu) << 8;
  }

  uint64_t low_ = 0;
  uint32_t range_ = 0xFFFFFFFFu;
  uint8_t cache_ = 0;
  uint64_t cache_size_ = 1;
  std::vector<uint8_t> out_;
};

class Decoder {
 public:
  explicit Decoder(const std::vector<uint8_t>& data) : data_(data) {
    for (int i = 0; i < 5; ++i) code_ = (code_ << 8) | NextByte();
  }

  int Bit(uint16_t& prob, int /*ignored*/) {
    const uint32_t bound = (range_ >> kProbBits) * prob;
    int bit;
    if (code_ < bound) {
      range_ = bound;
      prob += ((1u << kProbBits) - prob) >> kAdaptShift;
      bit = 0;
    } else {
      code_ -= bound;
      range_ -= bound;
      prob -= prob >> kAdaptShift;
      bit = 1;
    }
    Normalize();
    return bit;
  }

  uint32_t Direct(uint32_t /*ignored*/, int nbits) {
    uint32_t value = 0;
    for (int i = 0; i < nbits; ++i) {
      range_ >>= 1;
      uint32_t bit = code_ >= range_;
      if (bit) code_ -= range_;
      value = (value << 1) | bit;
      Normalize();
    }
    return value;
  }

 private:
  uint8_t NextByte() { return pos_ < data_.size() ? data_[pos_++] : 0; }

  void Normalize() {
    while (range_ < kTopValue) {
      range_ <<= 8;
      code_ = (code_ << 8) | NextByte();
    }
  }

  const std::vector<uint8_t>& data_;
  size_t pos_ = 0;
  uint32_t code_ = 0;
  uint32_t range_ = 0xFFFFFFFFu;
};

}  // namespace gc
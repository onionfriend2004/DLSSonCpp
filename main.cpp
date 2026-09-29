// Использование:
//   ./run -s -i input.tsv -o graph.bin   (сериализация)
//   ./run -d -i graph.bin -o output.tsv  (десериализация)

#include <iostream>
#include <stdexcept>
#include <string>

#include "codec.hpp"
#include "io.hpp"

namespace {

enum class Mode { kSerialize, kDeserialize };

// Разбирает флаги командной строки. Бросает std::invalid_argument при любой
// ошибке (неизвестный флаг, пропущенное значение, не указан режим/пути).
Mode ParseFlags(int argc, char** argv, std::string& input_file,
                 std::string& output_file) {
  bool has_serialize = false;
  bool has_deserialize = false;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];

    if (arg == "-s") {
      has_serialize = true;
    } else if (arg == "-d") {
      has_deserialize = true;
    } else if (arg == "-i") {
      if (i + 1 >= argc) throw std::invalid_argument("-i requires a value");
      input_file = argv[++i];
    } else if (arg == "-o") {
      if (i + 1 >= argc) throw std::invalid_argument("-o requires a value");
      output_file = argv[++i];
    } else {
      throw std::invalid_argument("unknown flag: " + arg);
    }
  }

  if (has_serialize == has_deserialize) {
    throw std::invalid_argument("specify exactly one of -s or -d");
  }
  if (input_file.empty() || output_file.empty()) {
    throw std::invalid_argument("both -i and -o are required");
  }

  return has_serialize ? Mode::kSerialize : Mode::kDeserialize;
}

void PrintUsage() {
  std::cerr << "usage: run (-s|-d) -i <input> -o <output>\n";
}

}  // namespace

int main(int argc, char** argv) {
  std::string input_file, output_file;
  Mode mode;

  try {
    mode = ParseFlags(argc, argv, input_file, output_file);
  } catch (const std::invalid_argument& e) {
    std::cerr << "error: " << e.what() << "\n";
    PrintUsage();
    return 1;
  }

  try {
    if (mode == Mode::kSerialize) {
      gc::WriteFile(output_file, gc::Serialize(gc::ReadTsv(input_file)));
    } else {
      gc::WriteTsv(output_file, gc::Deserialize(gc::ReadFile(input_file)));
    }
  } catch (const std::exception& e) {
    std::cerr << "error: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
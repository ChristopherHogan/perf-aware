
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

using std::cout;
using std::endl;
using std::ifstream;
using std::ofstream;
using std::string;
using std::to_string;
using std::unordered_map;
using std::vector;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

typedef int8_t i8;
typedef int16_t i16;

// NOTE(chogan): 3 bits for register, 1 bit for w (8 or 16 bit)
unordered_map<u8, string> registers = {
  {0b0000, "al"},
  {0b0001, "ax"},
  {0b0010, "cl"},
  {0b0011, "cx"},
  {0b0100, "dl"},
  {0b0101, "dx"},
  {0b0110, "bl"},
  {0b0111, "bx"},
  {0b1000, "ah"},
  {0b1001, "sp"},
  {0b1010, "ch"},
  {0b1011, "bp"},
  {0b1100, "dh"},
  {0b1101, "si"},
  {0b1110, "bh"},
  {0b1111, "di"},
};

unordered_map<u8, string> effective_address_calculations = {
  {0b000, "[bx + si"},
  {0b001, "[bx + di"},
  {0b010, "[bp + si"},
  {0b011, "[bp + di"},
  {0b100, "[si"},
  {0b101, "[di"},
  {0b110, "[bp"},
  {0b111, "[bx"},
};

enum Instructions {
  Instructions_Mov
};

string instruction_strings[] = {
  "mov"
};

unordered_map<u8, Instructions> opcodes = {
  {0b10001000, Instructions_Mov},
  {0b10001001, Instructions_Mov},
  {0b10001010, Instructions_Mov},
  {0b10001011, Instructions_Mov},
  {0b11000110, Instructions_Mov},
  {0b11000111, Instructions_Mov},
  {0b10110000, Instructions_Mov},
  {0b10110001, Instructions_Mov},
  {0b10110010, Instructions_Mov},
  {0b10110011, Instructions_Mov},
  {0b10110100, Instructions_Mov},
  {0b10110101, Instructions_Mov},
  {0b10110110, Instructions_Mov},
  {0b10110111, Instructions_Mov},
  {0b10111000, Instructions_Mov},
  {0b10111001, Instructions_Mov},
  {0b10111010, Instructions_Mov},
  {0b10111011, Instructions_Mov},
  {0b10111100, Instructions_Mov},
  {0b10111101, Instructions_Mov},
  {0b10111110, Instructions_Mov},
  {0b10111111, Instructions_Mov},
  {0b10100000, Instructions_Mov},
  {0b10100001, Instructions_Mov},
  {0b10100010, Instructions_Mov},
  {0b10100011, Instructions_Mov}
};

enum MovVariants {
  MovVariants_RMtoFromR,
  MovVariants_ImmediateToRM,
  MovVariants_ImmediateToR,
  MovVariants_MemToAccumulator,
  MovVariants_AccumulatorToMem,
};

unordered_map<u8, MovVariants> mov_variants = {
  {0b10001000, MovVariants_RMtoFromR},
  {0b10001001, MovVariants_RMtoFromR},
  {0b10001010, MovVariants_RMtoFromR},
  {0b10001011, MovVariants_RMtoFromR},
  {0b11000110, MovVariants_ImmediateToRM},
  {0b11000111, MovVariants_ImmediateToRM},
  {0b10110000, MovVariants_ImmediateToR},
  {0b10110001, MovVariants_ImmediateToR},
  {0b10110010, MovVariants_ImmediateToR},
  {0b10110011, MovVariants_ImmediateToR},
  {0b10110100, MovVariants_ImmediateToR},
  {0b10110101, MovVariants_ImmediateToR},
  {0b10110110, MovVariants_ImmediateToR},
  {0b10110111, MovVariants_ImmediateToR},
  {0b10111000, MovVariants_ImmediateToR},
  {0b10111001, MovVariants_ImmediateToR},
  {0b10111010, MovVariants_ImmediateToR},
  {0b10111011, MovVariants_ImmediateToR},
  {0b10111100, MovVariants_ImmediateToR},
  {0b10111101, MovVariants_ImmediateToR},
  {0b10111110, MovVariants_ImmediateToR},
  {0b10111111, MovVariants_ImmediateToR},
  {0b10100000, MovVariants_MemToAccumulator},
  {0b10100001, MovVariants_MemToAccumulator},
  {0b10100010, MovVariants_AccumulatorToMem},
  {0b10100011, MovVariants_AccumulatorToMem}
};

void readEntireFile(vector<u8> &data, const char *fname) {
  fs::path p(fname);
  auto sz = fs::file_size(p);
  data.resize(sz);

  ifstream is(fname, std::ios::binary);
  if (is.is_open()) {
    is.read(reinterpret_cast<char*>(data.data()), sz);
  } else {
    // TODO(chogan): error handling
  }
}

string getRegister(u8 byte, u8 w_bit) {
  byte <<= 1;
  byte |= w_bit;
  string result = registers[byte];

  return result;
}

struct DecodedInstruction {
  string instruction;
  u8 d_bit;
  u8 w_bit;
  u8 mode;
  u8 reg;
  u8 rm;
  u8 disp_lo;
  u8 disp_hi;

  int decodeMov(const vector<u8> &data, size_t offset) {
    int num_bytes = 2;
    u8 b1 = data[offset];
    u8 b2 = data[offset + 1];
    instruction = instruction_strings[opcodes[b1]];
    instruction += " ";
    string dest;
    string source;

    switch (mov_variants[b1]) {
      case MovVariants_RMtoFromR: {
        d_bit = (b1 & 0b00000010) >> 1;
        w_bit = (b1 & 0b00000001);

        mode = (b2 & 0b11000000) >> 6;
        reg = (b2 & 0b00111000) >> 3;
        rm = (b2 & 0b00000111);

        if (mode == 0b01 || (mode == 0b00 && rm == 0b110 && !w_bit)) {
          disp_lo = data[offset + 2];
          num_bytes += 1;
        } else if (mode == 0b10 || (mode == 0b00 && rm == 0b110 && w_bit)) {
          disp_lo = data[offset + 2];
          disp_hi = data[offset + 3];
          num_bytes += 2;
        }

        if (mode == 0b11) {
          dest = getRegister(reg, w_bit);
          source = getRegister(rm, w_bit);
        } else if (mode == 0b00) {
          dest = getRegister(reg, w_bit);
          if (rm == 0b110) {
            source = "[";
            if (w_bit) {
              source += to_string(disp_lo | (disp_hi << 8));
            } else {
              source += to_string(disp_lo);
            }
          } else {
            source = effective_address_calculations[rm];
          }
          source += "]";
        } else if (mode == 0b01) {
          dest = getRegister(reg, w_bit);
          source = effective_address_calculations[rm];
          if ((i8)disp_lo < 0) {
            source += " - ";
            disp_lo = ~disp_lo + 1;
          } else {
            source += " + ";
          }
          source += to_string(disp_lo);
          source += "]";
        } else if (mode == 0b10) {
          dest = getRegister(reg, w_bit);
          source = effective_address_calculations[rm];
          u16 immediate = disp_lo | (disp_hi << 8);
          if ((i16)immediate < 0) {
            source += " - ";
            immediate = ~immediate + 1;
          } else {
            source += " + ";
          }
          source += to_string(immediate);
          source += "]";
        }

        if (!d_bit) {
          std::swap(dest, source);
        }

        break;
      }
      case MovVariants_ImmediateToRM: {
        w_bit = b1 & 0b00000001;
        mode = (b2 & 0b11000000) >> 6;
        rm = b2 & 0b00000111;

        if (mode == 0b01 || (mode == 0b00 && rm == 0b110 && !w_bit)) {
          disp_lo = data[offset + 2];
          num_bytes += 1;
        } else if (mode == 0b10 || (mode == 0b00 && rm == 0b110 && w_bit)) {
          disp_lo = data[offset + 2];
          disp_hi = data[offset + 3];
          num_bytes += 2;
        }

        if (mode == 0b11) {
          assert(false);
        } else if (mode == 0b00) {
          dest = effective_address_calculations[rm];
          if (rm == 0b110) {
            dest += " + ";
            if (w_bit) {
              dest += to_string(disp_lo | (disp_hi << 8));
            } else {
              dest += to_string(disp_lo);
            }
          }
          dest += "]";
        } else if (mode == 0b01) {
          dest = effective_address_calculations[rm];
          dest += " + ";
          dest += to_string(disp_lo);
          dest += "]";
        } else if (mode == 0b10) {
          dest = effective_address_calculations[rm];
          dest += " + ";
          dest += to_string(disp_lo | (disp_hi << 8));
          dest += "]";
        }

        u16 immediate = data[offset + num_bytes];
        num_bytes++;
        if (w_bit) {
          immediate |= (data[offset + num_bytes] << 8);
          num_bytes++;
        }

        source = w_bit ? "word " : "byte ";
        source += to_string(immediate);
        break;
      }
      case MovVariants_ImmediateToR: {
        w_bit = (b1 & 0b00001000) >> 3;
        reg = b1 & 0b00000111;
        u16 immediate = b2;
        if (w_bit) {
          immediate |= (data[offset + 2] << 8);
          num_bytes++;
        }
        dest = getRegister(reg, w_bit);
        source = to_string(immediate);
        break;
      }
      case MovVariants_MemToAccumulator: {
        w_bit = b1 & 0b00000001;
        u16 addr = b2;
        if (w_bit) {
          addr |= (data[offset + 2] << 8);
          num_bytes++;
        }
        dest = "ax";
        source = "[" + to_string(addr) + "]";

        break;
      }
      case MovVariants_AccumulatorToMem: {
        w_bit = b1 & 0b00000001;
        source = "ax";
        u16 addr = b2;
        if (w_bit) {
          addr |= (data[offset + 2] << 8);
          num_bytes++;
        }
        dest = "[" + to_string(addr) + "]";
        break;
      }
      default:
        break;
    }

    instruction += dest;
    instruction += ", ";
    instruction += source;

    return num_bytes;
  }

  void emitInstruction(ofstream &os) {
    os << instruction << endl;
  }
};

string getOutputFilename(char *fname) {
  int underscores = 2;
  char *cur = fname;
  while (underscores > 0) {
    cur++;
    if (cur && *cur == '_') {
      underscores--;
    }
  }

  string result(fname, cur - fname);

  return result + "_decoded.asm";
}

int main(int argc, char **argv) {
  char *fname = 0;

  if (argc > 0) {
    fname = argv[1];
  } else {
    // TODO(chogan): error handling
  }

  vector<u8> file_data;
  readEntireFile(file_data, fname);

  string output_fname = getOutputFilename(fname);
  ofstream output_file(output_fname);
  int register_size = 16;
  output_file << "bits " << to_string(register_size) << endl;

  size_t i = 0;
  size_t sz = file_data.size();
  while (i < sz) {
    int instruction_size = 0;
    DecodedInstruction instr = {};

    switch (opcodes[file_data[i]]) {
      case Instructions_Mov: {
        instruction_size += instr.decodeMov(file_data, i);
        instr.emitInstruction(output_file);
        break;
      }
      default:
        assert(false);
        break;
    }


    i += instruction_size;
  }

  return 0;
}

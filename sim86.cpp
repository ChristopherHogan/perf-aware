
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>

namespace fs = std::filesystem;

using std::cout;
using std::endl;
using std::ifstream;
using std::ofstream;
using std::string;
using std::to_string;
using std::unordered_map;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

typedef int8_t s8;
typedef int16_t s16;

const int kRegisterSize = 16;

#define KILOBYTES(n) (n * 1024)
#define MEGABYTES(n) (n * KILOBYTES(1) * KILOBYTES(1))

struct Memory {
  u32 used;
  u8 bytes[MEGABYTES(1)];
};

enum Registers {
  Registers_al,
  Registers_ax,
  Registers_cl,
  Registers_cx,
  Registers_dl,
  Registers_dx,
  Registers_bl,
  Registers_bx,
  Registers_ah,
  Registers_sp,
  Registers_ch,
  Registers_bp,
  Registers_dh,
  Registers_si,
  Registers_bh,
  Registers_di,
};

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
  Instructions_AddSubCmp,
  Instructions_Mov,
  Instructions_Add,
  Instructions_Sub,
  Instructions_Cmp
};

string instruction_strings[] = {
  "",
  "mov",
  "add",
  "sub",
  "cmp"
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
  {0b10100011, Instructions_Mov},

  {0b00000000, Instructions_Add},
  {0b00000001, Instructions_Add},
  {0b00000010, Instructions_Add},
  {0b00000011, Instructions_Add},

  {0b10000000, Instructions_AddSubCmp},
  {0b10000001, Instructions_AddSubCmp},
  {0b10000010, Instructions_AddSubCmp},
  {0b10000011, Instructions_AddSubCmp},

  {0b00000100, Instructions_Add},
  {0b00000101, Instructions_Add}

};

enum MovVariants {
  MovVariants_RMtoFromR,
  MovVariants_ImmediateToRM,
  MovVariants_ImmediateToR,
  MovVariants_MemToAccumulator,
  MovVariants_AccumulatorToMem,
};

enum AddVariants {
  AddVariants_RMtoFromR,
  AddVariants_ImmediateToRM,
  AddVariants_ImmediateToAccumulator
};

unordered_map<u8, AddVariants> add_variants = {
  {0b00000000, AddVariants_RMtoFromR},
  {0b00000001, AddVariants_RMtoFromR},
  {0b00000010, AddVariants_RMtoFromR},
  {0b00000011, AddVariants_RMtoFromR},
  {0b10000000, AddVariants_ImmediateToRM},
  {0b10000001, AddVariants_ImmediateToRM},
  {0b10000010, AddVariants_ImmediateToRM},
  {0b10000011, AddVariants_ImmediateToRM},
  {0b00000100, AddVariants_ImmediateToAccumulator},
  {0b00000101, AddVariants_ImmediateToAccumulator}
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

void readEntireFile(Memory *memory, const char *fname) {
  fs::path p(fname);
  auto sz = fs::file_size(p);

  ifstream is(fname, std::ios::binary);
  if (is.is_open()) {
    is.read(reinterpret_cast<char*>(memory->bytes), sz);
    memory->used = sz;
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

enum OpType {
  OpType_Reg,
  OpType_Eac,
  OpType_Immediate
};

struct Operand {
  OpType type;
  u16 disp;
  u16 immediate;
  u8 val;
};

struct DecodedInstruction {
  Operand dest;
  Operand source;
  string instruction;  // TODO(chogan): remove
  Instructions opcode;
  u8 d_bit;
  u8 s_bit;
  u8 w_bit;
  u8 mode;
  u8 reg;
  u8 rm;
  u8 disp_lo;
  u8 disp_hi;

  int decodeAdd(const u8 *data, size_t offset) {
    int num_bytes = 2;
    u8 b1 = data[offset];
    opcode = Instructions_Add;

    switch (add_variants[b1]) {
      case AddVariants_RMtoFromR: {
        rmToFromR(data, offset, num_bytes);
        break;
      }
      case AddVariants_ImmediateToRM: {
        immediateToRM(data, offset, num_bytes);
        break;
      }
      case AddVariants_ImmediateToAccumulator: {
        break;
      }
    }

    return num_bytes;
  }

  int getDisp(const u8 *data, size_t offset) {
    int num_bytes = 0;
    if (mode == 0b01 || (mode == 0b00 && rm == 0b110 && !w_bit)) {
      disp_lo = data[offset + 2];
      num_bytes += 1;
    } else if (mode == 0b10 || (mode == 0b00 && rm == 0b110 && w_bit)) {
      disp_lo = data[offset + 2];
      disp_hi = data[offset + 3];
      num_bytes += 2;
    }

    return num_bytes;
  }

  void immediateToRM(const u8 *data, size_t offset, int &num_bytes) {
    u8 b1 = data[offset];
    u8 b2 = data[offset + 1];
    d_bit = 1;
    w_bit = b1 & 0b00000001;
    s_bit = (b1 >> 1) & 1;
    mode = (b2 & 0b11000000) >> 6;
    rm = b2 & 0b00000111;

    num_bytes += getDisp(data, offset);

    if (mode == 0b11) {
      dest.type = OpType_Reg;
      dest.val = (rm << 1) | w_bit;
    } else {
      dest.type = OpType_Eac;
      dest.val = rm;
    }

    if ((mode == 0b00 && rm == 0b110 && w_bit) || mode == 0b10) {
      dest.disp = disp_lo | (disp_hi << 8);
    } else {
      dest.disp = disp_lo;
    }

    source.type = OpType_Immediate;
    u8 immediate = data[offset + num_bytes];
    num_bytes++;
    if (w_bit & !s_bit) {
      source.immediate = (data[offset + num_bytes] << 8) | immediate;
      num_bytes++;
    } else {
      source.immediate = immediate;
    }
  }

  void rmToFromR(const u8 *data, size_t offset, int &num_bytes) {
    u8 b1 = data[offset];
    u8 b2 = data[offset + 1];
    d_bit = (b1 & 0b00000010) >> 1;
    w_bit = b1 & 0b00000001;

    mode = (b2 & 0b11000000) >> 6;
    reg = (b2 & 0b00111000) >> 3;
    rm = (b2 & 0b00000111);

    num_bytes += getDisp(data, offset);

    if (mode == 0b11) {
      dest.type = OpType_Reg;
      dest.val = (reg << 1) | w_bit;
      source.type = OpType_Reg;
      source.val = (rm << 1) | w_bit;
    } else if (mode == 0b00) {
      dest.type = OpType_Reg;
      dest.val = (reg << 1) | w_bit;
      if (rm == 0b110) {
        if (w_bit) {
          source.disp = disp_lo | (disp_hi << 8);
        } else {
          source.disp = disp_lo;
        }
      } else {
        source.type = OpType_Eac;
        source.val = rm;
      }
    } else if (mode == 0b01) {
      dest.type = OpType_Reg;
      dest.val = (reg << 1) | w_bit;
      source.type = OpType_Eac;
      source.val = rm;
    } else if (mode == 0b10) {
      dest.type = OpType_Reg;
      dest.val = (reg << 1) | w_bit;
      source.type = OpType_Eac;
      source.val = rm;
      source.disp = disp_lo | (disp_hi << 8);
    }
  }

  int decodeMov(const u8 *data, size_t offset) {
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
        w_bit = b1 & 0b00000001;

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
          if ((s8)disp_lo < 0) {
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
          if ((s16)immediate < 0) {
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
    string instr = instruction_strings[opcode];
    instr += " ";
    string dst;
    string src;

    if (dest.type == OpType_Reg) {
      dst += registers[dest.val];
    } else if (dest.type == OpType_Eac) {
      dst += effective_address_calculations[dest.val];
    }

    if (source.type == OpType_Reg) {
      src += registers[source.val];
    } else if (source.type == OpType_Eac) {
      if (mode == 0b00 && source.disp != 0) {
        src += "[";
        src += to_string(source.disp);
      } else {
        src += effective_address_calculations[source.val];
      }
      u16 sign_extended_disp = source.disp;
      if ((s16)source.disp < 0) {
        src += " - ";
        sign_extended_disp = ~source.disp + 1;
      } else {
        src += " + ";
      }
      src += to_string(sign_extended_disp);
      src += "]";
    } else if (source.type == OpType_Immediate) {
      if (dest.type != OpType_Reg) {
        src += w_bit ? "word " : "byte ";
      }
      src += to_string(source.immediate);
    }

    if (!d_bit) {
      std::swap(dst, src);
    }

    instr += dst;
    instr += ", ";
    instr += src;
    os << instr << endl;
  }
};

Instructions dispatchAddSubCmp(const u8 *data, size_t offset) {
  u8 b2 = data[offset + 1];
  u8 bits = (b2 >> 3) & 0b111;

  Instructions result = {};
  if (bits == 0b000) {
    result = Instructions_Add;
  } else if (bits == 0b101) {
    result = Instructions_Sub;
  } else if (bits == 0b111) {
    result = Instructions_Cmp;
  } else {
    // TODO(chogan): ERROR
  }

  return result;
}

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

  Memory memory = {};
  readEntireFile(&memory, fname);

  string output_fname = getOutputFilename(fname);
  ofstream output_file(output_fname);
  output_file << "bits " << to_string(kRegisterSize) << endl;

  size_t i = 0;
  size_t sz = memory.used;
  while (i < sz) {
    int instruction_size = 0;
    DecodedInstruction instr = {};

    switch (opcodes[memory.bytes[i]]) {
      case Instructions_Mov: {
        instruction_size += instr.decodeMov(memory.bytes, i);
        output_file << instr.instruction << endl;
        break;
      }
      case Instructions_Add: {
          instruction_size += instr.decodeAdd(memory.bytes, i);
          instr.emitInstruction(output_file);
        break;
      }
      case Instructions_AddSubCmp: {
        Instructions inst = dispatchAddSubCmp(memory.bytes, i);
        if (inst == Instructions_Add) {
          instruction_size += instr.decodeAdd(memory.bytes, i);
          instr.emitInstruction(output_file);
        } else if (inst == Instructions_Sub) {

        } else if (inst == Instructions_Cmp) {

        } else {
          // TODO(chogan): ERROR
        }
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

#include <cassert>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>

#ifndef SIM86_MAIN
  #define SIM86_MAIN 1
#endif

namespace fs = std::filesystem;

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
const int kNumRegisters = 8;

#define KILOBYTES(n) (n * 1024)
#define MEGABYTES(n) (n * KILOBYTES(1) * KILOBYTES(1))

enum AddressingMode {
  AddressingMode_l,
  AddressingMode_h,
  AddressingMode_x
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

enum Instructions {
  Instructions_AddSubCmp,
  Instructions_Mov,
  Instructions_Add,
  Instructions_Sub,
  Instructions_Cmp,
  Instructions_Jnz
};

enum OpType {
  OpType_None,
  OpType_Reg,
  OpType_Eac,
  OpType_Immediate
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

enum SubVariants {
  SubVariants_RMtoFromR,
  SubVariants_ImmediateToRM,
  SubVariants_ImmediateToAccumulator
};

enum CmpVariants {
  CmpVariants_RMtoFromR,
  CmpVariants_ImmediateToRM,
  CmpVariants_ImmediateToAccumulator
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

string instruction_strings[] = {
  "",
  "mov",
  "add",
  "sub",
  "cmp",
  "jnz"
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

  {0b10000000, Instructions_AddSubCmp},
  {0b10000001, Instructions_AddSubCmp},
  {0b10000010, Instructions_AddSubCmp},
  {0b10000011, Instructions_AddSubCmp},

  {0b00000000, Instructions_Add},
  {0b00000001, Instructions_Add},
  {0b00000010, Instructions_Add},
  {0b00000011, Instructions_Add},
  {0b00000100, Instructions_Add},
  {0b00000101, Instructions_Add},

  {0b00101000, Instructions_Sub},
  {0b00101001, Instructions_Sub},
  {0b00101010, Instructions_Sub},
  {0b00101011, Instructions_Sub},
  {0b00101100, Instructions_Sub},
  {0b00101101, Instructions_Sub},

  {0b00111000, Instructions_Cmp},
  {0b00111001, Instructions_Cmp},
  {0b00111010, Instructions_Cmp},
  {0b00111011, Instructions_Cmp},
  {0b00111100, Instructions_Cmp},
  {0b00111101, Instructions_Cmp},

  {0b01110101, Instructions_Jnz}
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

unordered_map<u8, SubVariants> sub_variants = {
  {0b00101000, SubVariants_RMtoFromR},
  {0b00101001, SubVariants_RMtoFromR},
  {0b00101010, SubVariants_RMtoFromR},
  {0b00101011, SubVariants_RMtoFromR},
  {0b10000000, SubVariants_ImmediateToRM},
  {0b10000001, SubVariants_ImmediateToRM},
  {0b10000010, SubVariants_ImmediateToRM},
  {0b10000011, SubVariants_ImmediateToRM},
  {0b00101100, SubVariants_ImmediateToAccumulator},
  {0b00101101, SubVariants_ImmediateToAccumulator}
};

unordered_map<u8, CmpVariants> cmp_variants = {
  {0b00111000, CmpVariants_RMtoFromR},
  {0b00111001, CmpVariants_RMtoFromR},
  {0b00111010, CmpVariants_RMtoFromR},
  {0b00111011, CmpVariants_RMtoFromR},
  {0b10000000, CmpVariants_ImmediateToRM},
  {0b10000001, CmpVariants_ImmediateToRM},
  {0b10000010, CmpVariants_ImmediateToRM},
  {0b10000011, CmpVariants_ImmediateToRM},
  {0b00111100, CmpVariants_ImmediateToAccumulator},
  {0b00111101, CmpVariants_ImmediateToAccumulator}
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

struct Arguments {
  char *fname;
  bool exec;
};

struct Memory {
  u32 used;
  u8 bytes[MEGABYTES(1)];
};

struct Operand {
  OpType type;
  u16 disp;
  u16 immediate;
  u8 val;
  bool mem;
  bool relative;
};

struct RegisterAccess {
  u32 index;
  AddressingMode mode;
};

RegisterAccess access_patterns[] = {
  [Registers_al] = {0, AddressingMode_l},
  [Registers_ax] = {0, AddressingMode_x},
  [Registers_cl] = {2, AddressingMode_l},
  [Registers_cx] = {2, AddressingMode_x},
  [Registers_dl] = {3, AddressingMode_l},
  [Registers_dx] = {3, AddressingMode_x},
  [Registers_bl] = {1, AddressingMode_l},
  [Registers_bx] = {1, AddressingMode_x},
  [Registers_ah] = {0, AddressingMode_h},
  [Registers_sp] = {4, AddressingMode_x},
  [Registers_ch] = {2, AddressingMode_h},
  [Registers_bp] = {5, AddressingMode_x},
  [Registers_dh] = {3, AddressingMode_h},
  [Registers_si] = {6, AddressingMode_x},
  [Registers_bh] = {1, AddressingMode_h},
  [Registers_di] = {7, AddressingMode_x}
};

union Register {
  struct {
    u8 l;
    u8 h;
  } byte;
  u16 x;
};

struct MachineState {
  Register prev[8];
  Register registers[8];
};

struct DecodedInstruction {
  Operand dest;
  Operand source;
  Instructions opcode;
  u8 d_bit;
  u8 s_bit;
  u8 w_bit;
  u8 mode;
  u8 reg;
  u8 rm;
  u8 disp_lo;
  u8 disp_hi;

  int decodeMov(const u8 *data, size_t offset) {
    int num_bytes = 2;
    u8 b1 = data[offset];
    u8 b2 = data[offset + 1];
    opcode = opcodes[b1];

    switch (mov_variants[b1]) {
      case MovVariants_RMtoFromR: {
        rmToFromR(data, offset, num_bytes);
        break;
      }
      case MovVariants_ImmediateToRM: {
        immediateToRM(data, offset, num_bytes);
        break;
      }
      case MovVariants_ImmediateToR: {
        w_bit = (b1 & 0b00001000) >> 3;
        d_bit = 1;
        reg = b1 & 0b00000111;
        u16 immediate = b2;
        if (w_bit) {
          immediate |= (data[offset + 2] << 8);
          num_bytes++;
        }
        dest.type = OpType_Reg;
        dest.val = (reg << 1) | w_bit;
        source.type = OpType_Immediate;
        source.immediate = immediate;
        break;
      }
      case MovVariants_MemToAccumulator: {
        w_bit = b1 & 0b00000001;
        d_bit = 1;
        u16 addr = b2;
        if (w_bit) {
          addr |= (data[offset + 2] << 8);
          num_bytes++;
        }
        dest.type = OpType_Reg;
        dest.val = Registers_ax;
        source.type = OpType_Immediate;
        source.immediate = addr;
        source.mem = true;
        break;
      }
      case MovVariants_AccumulatorToMem: {
        w_bit = b1 & 0b00000001;
        d_bit = 1;
        u16 addr = b2;
        if (w_bit) {
          addr |= (data[offset + 2] << 8);
          num_bytes++;
        }
        dest.type = OpType_Immediate;
        dest.immediate = addr;
        dest.mem = true;
        source.type = OpType_Reg;
        source.val = Registers_ax;
        break;
      }
      default:
        break;
    }

    return num_bytes;
  }

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
        immediateToAccumulator(data, offset, num_bytes);
        break;
      }
    }

    return num_bytes;
  }

  int decodeSub(const u8 *data, size_t offset) {
    int num_bytes = 2;
    u8 b1 = data[offset];
    opcode = Instructions_Sub;

    switch (sub_variants[b1]) {
      case SubVariants_RMtoFromR: {
        rmToFromR(data, offset, num_bytes);
        break;
      }
      case SubVariants_ImmediateToRM: {
        immediateToRM(data, offset, num_bytes);
        break;
      }
      case SubVariants_ImmediateToAccumulator: {
        immediateToAccumulator(data, offset, num_bytes);
        break;
      }
    }

    return num_bytes;
  }

  int decodeCmp(const u8 *data, size_t offset) {
    int num_bytes = 2;
    u8 b1 = data[offset];
    opcode = Instructions_Cmp;

    switch (cmp_variants[b1]) {
      case CmpVariants_RMtoFromR: {
        rmToFromR(data, offset, num_bytes);
        break;
      }
      case CmpVariants_ImmediateToRM: {
        immediateToRM(data, offset, num_bytes);
        break;
      }
      case CmpVariants_ImmediateToAccumulator: {
        immediateToAccumulator(data, offset, num_bytes);
        break;
      }
    }

    return num_bytes;
  }

  int decodeJnz(const u8 *data, size_t offset) {
    int num_bytes = 2;
    u8 b1 = data[offset];
    // NOTE(chogan): Signify that the immediate is only 1 byte (IP-INC8)
    w_bit = 0;
    opcode = opcodes[b1];
    d_bit = 1;
    dest.type = OpType_Immediate;
    dest.relative = true;
    dest.immediate = (u16)data[offset + 1];
    dest.immediate += 2;
    source.type = OpType_None;

    return num_bytes;
  }

  int getDisp(const u8 *data, size_t offset) {
    int num_bytes = 0;
    if (mode == 0b01) {
      disp_lo = data[offset + 2];
      num_bytes += 1;
    } else if (mode == 0b10 || (mode == 0b00 && rm == 0b110)) {
      disp_lo = data[offset + 2];
      disp_hi = data[offset + 3];
      num_bytes += 2;
    }

    return num_bytes;
  }

  string opToString(Operand *op, bool needs_immediate_size) {
    string result;
    if (op->type == OpType_Reg) {
      result += registers[op->val];
    } else if (op->type == OpType_Eac) {
      if (mode == 0b00 && op->disp != 0) {
        result += "[";
        result += to_string(op->disp);
      } else {
        result += effective_address_calculations[op->val];
      }
      if ((s16)op->disp < 0) {
        result += " - ";
        result += to_string((s16)op->disp * -1);
      } else {
        result += " + ";
        result += to_string(op->disp);
      }
      result += "]";
    } else if (op->type == OpType_Immediate) {
      if (needs_immediate_size) {
        result += w_bit ? "word " : "byte ";
      }

      string imm;
      if (op->relative) {
        imm += "$";
        // TODO(chogan): 16 bit relative offset
        // if (w_bit) {
        //   s16 signed_immediate = (s16)op->immediate;
        // }
        s8 signed_immediate = (s8)op->immediate;
        if (signed_immediate < 0) {
          imm += "-";
          signed_immediate *= -1;
        } else {
          imm += "+";
        }
        imm += to_string(signed_immediate);
      } else if (op->mem) {
        imm += "[" + to_string(op->immediate) + "]";
      } else {
        imm += to_string(op->immediate);
      }

      result += imm;
    }
    return result;
  }

  void immediateToRM(const u8 *data, size_t offset, int &num_bytes) {
    u8 b1 = data[offset];
    u8 b2 = data[offset + 1];
    d_bit = 1;
    w_bit = b1 & 0b00000001;
    if (opcode == Instructions_Add || opcode == Instructions_Sub || opcode == Instructions_Cmp) {
      s_bit = (b1 >> 1) & 1;
    } else {
      s_bit = 0;
    }
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

    if (mode == 0b00 && rm == 0b110) {
      dest.type = OpType_Immediate;
      dest.immediate = disp_lo | (disp_hi << 8);
      dest.mem = true;
    } else if (mode == 0b10) {
      dest.disp = disp_lo | (disp_hi << 8);
    } else {
      dest.disp = disp_lo;
    }

    source.type = OpType_Immediate;
    u8 immediate = data[offset + num_bytes];
    num_bytes++;
    if (w_bit && !s_bit) {
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
        source.type = OpType_Immediate;
        source.immediate = disp_lo | (disp_hi << 8);
        source.mem = true;
      } else {
        source.type = OpType_Eac;
        source.val = rm;
      }
    } else if (mode == 0b01) {
      dest.type = OpType_Reg;
      dest.val = (reg << 1) | w_bit;
      source.type = OpType_Eac;
      source.val = rm;
      source.disp = disp_lo;
      if ((s8)disp_lo < 0) {
        source.disp = (0xFF << 8) | disp_lo;
      }
    } else if (mode == 0b10) {
      dest.type = OpType_Reg;
      dest.val = (reg << 1) | w_bit;
      source.type = OpType_Eac;
      source.val = rm;
      source.disp = disp_lo | (disp_hi << 8);
    }
  }

  void immediateToAccumulator(const u8 *data, size_t offset, int &num_bytes) {
    num_bytes--;
    u8 b1 = data[offset];
    d_bit = 1;
    w_bit = b1 & 0b00000001;

    dest.type = OpType_Reg;
    if (w_bit) {
      dest.val = Registers_ax;
    } else {
      dest.val = Registers_al;
    }

    source.type = OpType_Immediate;
    u8 immediate = data[offset + num_bytes];
    num_bytes++;
    if (w_bit) {
      source.immediate = (data[offset + num_bytes] << 8) | immediate;
      num_bytes++;
    } else {
      source.immediate = immediate;
    }
  }

  void emitInstruction(ofstream &os, MachineState *state) {
    string instr = instruction_strings[opcode];
    instr += " ";
    bool needs_immediate_size = dest.type == OpType_Eac || (dest.type == OpType_Immediate && dest.mem);
    string dst = opToString(&dest, false);
    string src = opToString(&source, needs_immediate_size);

    if (!d_bit) {
      std::swap(dst, src);
    }

    instr += dst;
    if (!src.empty()) {
      instr += ", ";
      instr += src;
    }
    os << instr;
    if (state) {
      int index = access_patterns[dest.val].index;
      u16 previous = state->prev[index].x;
      u16 current = state->registers[index].x;
      os << "  ; " << registers[dest.val] << ":0x" << std::hex << previous << "->0x" << current << std::dec;
    }
    os << endl;
  }
};

void execMov(DecodedInstruction *instr, MachineState *state) {
  if (instr->dest.type == OpType_Reg && instr->source.type == OpType_Immediate) {
    RegisterAccess access = access_patterns[instr->dest.val];
    u32 index = access.index;
    if (access.mode == AddressingMode_x) {
      assert(instr->w_bit);
      state->prev[index].x = state->registers[index].x;
      state->registers[index].x = instr->source.immediate;
    } else if (access.mode == AddressingMode_l) {
      state->prev[index].byte.l = state->registers[index].byte.l;
      state->registers[index].byte.l = (u8)instr->source.immediate;
    } else if (access.mode == AddressingMode_h) {
      state->prev[index].byte.h = state->registers[index].byte.h;
      state->registers[index].byte.h = (u8)instr->source.immediate;
    }
  }
}

void execInstruction(DecodedInstruction *instr, MachineState *state) {
  switch (instr->opcode) {
    case Instructions_Mov:
      execMov(instr, state);
      break;
    case Instructions_Add:
      break;
    case Instructions_Sub:
      break;
    case Instructions_Cmp:
      break;
    case Instructions_Jnz:
      break;
    default:
      break;
  }
}

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
    fprintf(stderr, "ERROR: 0x%x is not an add, sub, or cmp opcode\n", bits);
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
  result += "_decoded.asm";

  return result;
}

void run(Arguments *args, MachineState *state) {
  Memory memory = {};
  readEntireFile(&memory, args->fname);

  string output_fname = getOutputFilename(args->fname);
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
        break;
      }
      case Instructions_Add: {
          instruction_size += instr.decodeAdd(memory.bytes, i);
        break;
      }
      case Instructions_Sub: {
          instruction_size += instr.decodeSub(memory.bytes, i);
        break;
      }
      case Instructions_Cmp: {
        instruction_size += instr.decodeCmp(memory.bytes, i);
        break;
      }
      case Instructions_Jnz: {
        instruction_size += instr.decodeJnz(memory.bytes, i);
        break;
      }
      case Instructions_AddSubCmp: {
        Instructions inst = dispatchAddSubCmp(memory.bytes, i);
        if (inst == Instructions_Add) {
          instruction_size += instr.decodeAdd(memory.bytes, i);
        } else if (inst == Instructions_Sub) {
          instruction_size += instr.decodeSub(memory.bytes, i);
        } else if (inst == Instructions_Cmp) {
          instruction_size += instr.decodeCmp(memory.bytes, i);
        } else {
          // TODO(chogan): ERROR
        }
        break;
      }
      default:
        assert(false);
        break;
    }

    if (args->exec) {
      execInstruction(&instr, state);
      instr.emitInstruction(output_file, state);
    } else {
      instr.emitInstruction(output_file, NULL);
    }

    i += instruction_size;
  }

  if (args->exec) {
    printf("Final registers:\n");
    const char *reg_names[] = {"ax", "bx", "cx", "dx", "sp", "dp", "si", "di"};
    for (int i = 0; i < kNumRegisters; ++i) {
      const char *reg = reg_names[i];
      u16 hex = state->registers[i].x;
      printf("\t%s: 0x%04x (%d)\n", reg, hex, hex);
    }
  }

}

Arguments parseArgs(int argc, char **argv) {
  Arguments result = {};

  if (argc == 2) {
    result.fname = argv[1];
  } else if (argc == 3 && strcmp(argv[1], "-exec") == 0){
    result.exec = true;
    result.fname = argv[2];
  } else {
    fprintf(stderr, "USAGE: %s [-exec] <8086_asm_filename>\n", argv[0]);
    exit(1);
  }

  return result;
}

#if SIM86_MAIN == 1
int main(int argc, char **argv) {
  Arguments args = parseArgs(argc, argv);
  MachineState state = {};
  run(&args, &state);

  return 0;
}
#endif

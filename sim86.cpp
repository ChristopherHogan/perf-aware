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
const int kNumRegisters = 9;

#define KILOBYTES(n) (n * 1024)

enum Flags : u8 {
  Flags_None = 0,
  Flags_Carry = (1 << 0),
  Flags_Parity = (1 << 1),
  Flags_AuxiliaryCarry = (1 << 2),
  Flags_Zero = (1 << 3),
  Flags_Sign = (1 << 4),
  Flags_Overflow = (1 << 5)
};

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
  Registers_ip
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
  {Registers_al, "al"},
  {Registers_ax, "ax"},
  {Registers_cl, "cl"},
  {Registers_cx, "cx"},
  {Registers_dl, "dl"},
  {Registers_dx, "dx"},
  {Registers_bl, "bl"},
  {Registers_bx, "bx"},
  {Registers_ah, "ah"},
  {Registers_sp, "sp"},
  {Registers_ch, "ch"},
  {Registers_bp, "bp"},
  {Registers_dh, "dh"},
  {Registers_si, "si"},
  {Registers_bh, "bh"},
  {Registers_di, "di"},
  {Registers_ip, "ip"}
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
  bool dump;
};

struct Memory {
  u32 used;
  u8 bytes[KILOBYTES(64)];
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

struct MemoryAccess {
  u32 index;
  bool one_byte;
};

struct RmAccess {
  OpType type;
  union {
    RegisterAccess reg;
    MemoryAccess mem;
  } access;
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
  [Registers_di] = {7, AddressingMode_x},
  [Registers_ip] = {8, AddressingMode_x}
};

union Register {
  struct {
    u8 l;
    u8 h;
  } byte;
  u16 x;
};

struct MachineState {
  Memory mem;
  Register prev[kNumRegisters];
  Register registers[kNumRegisters];
  u8 prev_flags;
  u8 flags;
};

union ByteOrNibble{
  u8 bits8;
  u16 bits16;
};

struct ExecutionDetails {
  RmAccess dest;
  RmAccess source;
  ByteOrNibble source_val;
  ByteOrNibble result;
  bool hi;
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
  u8 size;

  void decodeMov(const u8 *data, size_t offset) {
    size = 2;
    u8 b1 = data[offset];
    u8 b2 = data[offset + 1];
    opcode = opcodes[b1];

    switch (mov_variants[b1]) {
      case MovVariants_RMtoFromR: {
        rmToFromR(data, offset);
        break;
      }
      case MovVariants_ImmediateToRM: {
        immediateToRM(data, offset);
        break;
      }
      case MovVariants_ImmediateToR: {
        w_bit = (b1 & 0b00001000) >> 3;
        d_bit = 1;
        reg = b1 & 0b00000111;
        u16 immediate = b2;
        if (w_bit) {
          immediate |= (data[offset + 2] << 8);
          size++;
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
          size++;
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
          size++;
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
  }

  void decodeAdd(const u8 *data, size_t offset) {
    size = 2;
    u8 b1 = data[offset];
    opcode = Instructions_Add;

    switch (add_variants[b1]) {
      case AddVariants_RMtoFromR: {
        rmToFromR(data, offset);
        break;
      }
      case AddVariants_ImmediateToRM: {
        immediateToRM(data, offset);
        break;
      }
      case AddVariants_ImmediateToAccumulator: {
        immediateToAccumulator(data, offset);
        break;
      }
    }
  }

  void decodeSub(const u8 *data, size_t offset) {
    size = 2;
    u8 b1 = data[offset];
    opcode = Instructions_Sub;

    switch (sub_variants[b1]) {
      case SubVariants_RMtoFromR: {
        rmToFromR(data, offset);
        break;
      }
      case SubVariants_ImmediateToRM: {
        immediateToRM(data, offset);
        break;
      }
      case SubVariants_ImmediateToAccumulator: {
        immediateToAccumulator(data, offset);
        break;
      }
    }
  }

  void decodeCmp(const u8 *data, size_t offset) {
    size = 2;
    u8 b1 = data[offset];
    opcode = Instructions_Cmp;

    switch (cmp_variants[b1]) {
      case CmpVariants_RMtoFromR: {
        rmToFromR(data, offset);
        break;
      }
      case CmpVariants_ImmediateToRM: {
        immediateToRM(data, offset);
        break;
      }
      case CmpVariants_ImmediateToAccumulator: {
        immediateToAccumulator(data, offset);
        break;
      }
    }
  }

  void decodeJnz(const u8 *data, size_t offset) {
    size = 2;
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
  }

  void getDisp(const u8 *data, size_t offset) {
    if (mode == 0b01) {
      disp_lo = data[offset + 2];
      size += 1;
    } else if (mode == 0b10 || (mode == 0b00 && rm == 0b110)) {
      disp_lo = data[offset + 2];
      disp_hi = data[offset + 3];
      size += 2;
    }
  }

  void immediateToRM(const u8 *data, size_t offset) {
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

    getDisp(data, offset);

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
    u8 immediate = data[offset + size];
    size++;
    if (w_bit && !s_bit) {
      source.immediate = (data[offset + size] << 8) | immediate;
      size++;
    } else {
      source.immediate = immediate;
    }
  }

  void rmToFromR(const u8 *data, size_t offset) {
    u8 b1 = data[offset];
    u8 b2 = data[offset + 1];
    d_bit = (b1 & 0b00000010) >> 1;
    w_bit = b1 & 0b00000001;

    mode = (b2 & 0b11000000) >> 6;
    reg = (b2 & 0b00111000) >> 3;
    rm = (b2 & 0b00000111);

    getDisp(data, offset);

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

  void immediateToAccumulator(const u8 *data, size_t offset) {
    size--;
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
    u8 immediate = data[offset + size];
    size++;
    if (w_bit) {
      source.immediate = (data[offset + size] << 8) | immediate;
      size++;
    } else {
      source.immediate = immediate;
    }
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

  void emitInstruction(ofstream &os, MachineState *state) {
    string instr = instruction_strings[opcode];
    instr += " ";
    bool needs_immediate_size = dest.type == OpType_Eac || (dest.type == OpType_Immediate && dest.mem);
    string dst = opToString(&dest, false);
    string src = opToString(&source, needs_immediate_size);

    instr += dst;
    if (!src.empty()) {
      instr += ", ";
      instr += src;
    }
    os << instr;
    if (state) {
      os << " ; ";
      if (dest.type == OpType_Reg) {
        int index = access_patterns[dest.val].index;
        u16 previous = state->prev[index].x;
        u16 current = state->registers[index].x;
        os << registers[dest.val] << ":0x" << std::hex << previous << "->0x" << current << std::dec;
      }

      u8 ip_index = access_patterns[Registers_ip].index;
      u16 ip_prev = state->prev[ip_index].x;
      u16 ip_cur = state->registers[ip_index].x;
      os << " ip:0x" << std::hex << ip_prev << "->0x" << ip_cur << std::dec;

      if ((state->flags || state->prev_flags) &&
          (opcode == Instructions_Add || opcode == Instructions_Sub || opcode == Instructions_Cmp)) {
        os << " flags:";
        if (state->prev_flags & Flags_Zero) {
          os << "Z";
        }
        if (state->prev_flags & Flags_Sign) {
          os << "S";
        }
        os << "->";
        if (state->flags & Flags_Zero) {
          os << "Z";
        }
        if (state->flags & Flags_Sign) {
          os << "S";
        }
      }
    }
    os << endl;
  }
};

u32 calculateEffectiveAddress(MachineState *state, u8 rm, u16 disp) {
  u32 result = 0;

  switch (rm) {
    case 0b000: {
      u16 bx = state->registers[access_patterns[Registers_bx].index].x;
      u16 si = state->registers[access_patterns[Registers_si].index].x;
      result = bx + si;
      break;
    }
    case 0b001: {
      u16 bx = state->registers[access_patterns[Registers_bx].index].x;
      u16 di = state->registers[access_patterns[Registers_di].index].x;
      result = bx + di;
      break;
    }
    case 0b010: {
      u16 bp = state->registers[access_patterns[Registers_bp].index].x;
      u16 si = state->registers[access_patterns[Registers_si].index].x;
      result = bp + si;
      break;
    }
    case 0b011: {
      u16 bp = state->registers[access_patterns[Registers_bp].index].x;
      u16 di = state->registers[access_patterns[Registers_di].index].x;
      result = bp + di;
      break;
    }
    case 0b100: {
      u16 si = state->registers[access_patterns[Registers_si].index].x;
      result = si;
      break;
    }
    case 0b101: {
      u16 di = state->registers[access_patterns[Registers_di].index].x;
      result = di;
      break;
    }
    case 0b110: {
      u16 bp = state->registers[access_patterns[Registers_bp].index].x;
      result = bp;
      break;
    }
    case 0b111: {
      u16 bx = state->registers[access_patterns[Registers_bx].index].x;
      result = bx;
      break;
    }
    default:
      break;
  }

  result += disp;

  return result;
}

void getExecutionDetails(ExecutionDetails *exec, DecodedInstruction *instr, MachineState *state) {
  exec->dest.type = instr->dest.type;
  exec->source.type = instr->source.type;

  if (instr->dest.type == OpType_Reg) {
    exec->dest.access.reg = access_patterns[instr->dest.val];
  } else if (instr->dest.type == OpType_Eac) {
    // NOTE(chogan): Assume all stores are 16 bit for now
    exec->dest.access.mem.index = calculateEffectiveAddress(state, instr->dest.val, instr->dest.disp);
  } else if (instr->dest.type == OpType_Immediate && instr->dest.mem) {
    exec->dest.access.mem.index = instr->dest.immediate;
  }

  if (instr->source.type == OpType_Immediate) {
    if (instr->source.mem) {
      u8 low_bits = state->mem.bytes[instr->source.immediate];
      u8 hi_bits = state->mem.bytes[instr->source.immediate + 1];
      exec->source_val.bits16 = (hi_bits << 8) | low_bits;
    } else {
      if (instr->w_bit) {
        exec->source_val.bits16 = instr->source.immediate;
      } else {
        exec->source_val.bits8 = (u8)instr->source.immediate;
      }
    }
  } else if (instr->source.type == OpType_Reg) {
    exec->source.access.reg = access_patterns[instr->source.val];
    if (exec->dest.access.reg.mode == AddressingMode_x) {
      exec->source_val.bits16 = state->registers[exec->source.access.reg.index].x;
    } else if (exec->dest.access.reg.mode == AddressingMode_l) {
      exec->source_val.bits8 = state->registers[exec->source.access.reg.index].byte.l;
    } else if (exec->dest.access.reg.mode == AddressingMode_h) {
      exec->hi = true;
      exec->source_val.bits8 = state->registers[exec->source.access.reg.index].byte.h;
    }
  } else if (instr->source.type == OpType_Eac) {
    exec->source.access.mem.index = calculateEffectiveAddress(state, instr->source.val, instr->source.disp);
    u8 low_bits = state->mem.bytes[exec->source.access.mem.index];
    u8 hi_bits = state->mem.bytes[exec->source.access.mem.index + 1];
    exec->source_val.bits16 = (hi_bits << 8) | low_bits;
  }
}

void setPreviousRegisterState(MachineState *state, ExecutionDetails *exec, u8 w_bit) {
  u32 index = exec->dest.access.reg.index;
  if (w_bit) {
    state->prev[index].x = state->registers[index].x;
  } else if (exec->hi) {
    state->prev[index].byte.h = state->registers[index].byte.h;
  } else {
    state->prev[index].byte.l = state->registers[index].byte.l;
  }
}

void setFlags(MachineState *state, ExecutionDetails *exec, u8 w_bit) {
  bool is_zero = false;
  bool is_signed = false;
  if (w_bit) {
    is_zero = exec->result.bits16 == 0;
    is_signed = exec->result.bits16 & 0x8000;
  } else {
    is_zero = exec->result.bits8 == 0;
    is_signed = exec->result.bits8 & 0x80;
  }

  if (is_zero) {
    state->flags |= Flags_Zero;
  } else {
    state->flags &= ~Flags_Zero;
  }
  if (is_signed) {
    state->flags |= Flags_Sign;
  } else {
    state->flags &= ~Flags_Sign;
  }
}

void execMov(DecodedInstruction *instr, MachineState *state) {
  ExecutionDetails exec = {};
  getExecutionDetails(&exec, instr, state);

  if (exec.dest.type == OpType_Reg) {
    setPreviousRegisterState(state, &exec, instr->w_bit);
    u32 index = exec.dest.access.reg.index;

    if (instr->w_bit) {
      state->registers[index].x = exec.source_val.bits16;
    } else if (exec.hi) {
      state->registers[index].byte.h = exec.source_val.bits8;
    } else {
      state->registers[index].byte.l = exec.source_val.bits8;
    }
  } else if (exec.dest.type == OpType_Eac || exec.dest.type == OpType_Immediate) {
    u16 val = exec.source_val.bits16;
    state->mem.bytes[exec.dest.access.mem.index] = (u8)(val & 0x00FF);
    state->mem.bytes[exec.dest.access.mem.index + 1] = (u8)(val >> 8);
  }
}

void execAdd(DecodedInstruction *instr, MachineState *state) {
  ExecutionDetails exec = {};
  getExecutionDetails(&exec, instr, state);
  setPreviousRegisterState(state, &exec, instr->w_bit);
  u32 index = exec.dest.access.reg.index;

  if (instr->w_bit) {
    state->registers[index].x += exec.source_val.bits16;
    exec.result.bits16 = state->registers[index].x;
  } else if (exec.hi) {
    state->registers[index].byte.h += exec.source_val.bits8;
    exec.result.bits8 = state->registers[index].byte.h;
  } else {
    state->registers[index].byte.l += exec.source_val.bits8;
    exec.result.bits8 = state->registers[index].byte.l;
  }

  setFlags(state, &exec, instr->w_bit);
}

void execSub(DecodedInstruction *instr, MachineState *state) {
  ExecutionDetails exec = {};
  getExecutionDetails(&exec, instr, state);
  setPreviousRegisterState(state, &exec, instr->w_bit);
  u32 index = exec.dest.access.reg.index;

  if (instr->w_bit) {
    state->registers[index].x -= exec.source_val.bits16;
    exec.result.bits16 = state->registers[index].x;
  } else if (exec.hi) {
    state->registers[index].byte.h -= exec.source_val.bits8;
    exec.result.bits8 = state->registers[index].byte.h;
  } else {
    state->registers[index].byte.l -= exec.source_val.bits8;
    exec.result.bits8 = state->registers[index].byte.l;
  }

  setFlags(state, &exec, instr->w_bit);
}

void execCmp(DecodedInstruction *instr, MachineState *state) {
  ExecutionDetails exec = {};
  getExecutionDetails(&exec, instr, state);
  setPreviousRegisterState(state, &exec, instr->w_bit);
  u32 index = exec.dest.access.reg.index;

  if (instr->w_bit) {
    exec.result.bits16 = state->registers[index].x - exec.source_val.bits16;
  } else if (exec.hi) {
    exec.result.bits8 = state->registers[index].byte.h - exec.source_val.bits8;
  } else {
    exec.result.bits8 = state->registers[index].byte.l - exec.source_val.bits8;
  }

  setFlags(state, &exec, instr->w_bit);
}

void execJnz(DecodedInstruction *instr, MachineState *state) {
  if (!(state->flags & Flags_Zero)) {
    u8 ip_index = access_patterns[Registers_ip].index;
    state->prev[ip_index].x = state->registers[ip_index].x;
    u16 val = instr->dest.immediate;
    // NOTE(chogan): Subtract 2 to counteract the +2 that nasm adds
    s8 offset = (s8)(val & 0xFF) - 2;
    if (offset < 0) {
      state->registers[ip_index].x -= (offset * -1);
    } else {
      state->registers[ip_index].x += offset;
    }
  }
}

void execInstruction(DecodedInstruction *instr, MachineState *state) {
  switch (instr->opcode) {
    case Instructions_Mov:
      execMov(instr, state);
      break;
    case Instructions_Add:
      execAdd(instr, state);
      break;
    case Instructions_Sub:
      execSub(instr, state);
      break;
    case Instructions_Cmp:
      execCmp(instr, state);
      break;
    case Instructions_Jnz:
      execJnz(instr, state);
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
  readEntireFile(&state->mem, args->fname);

  string output_fname = getOutputFilename(args->fname);
  ofstream output_file(output_fname);
  output_file << "bits " << to_string(kRegisterSize) << endl;

  u16 *ip = &state->registers[access_patterns[Registers_ip].index].x;
  size_t sz = state->mem.used;
  while (*ip < sz) {
    DecodedInstruction instr = {};
    size_t i = *ip;

    switch (opcodes[state->mem.bytes[i]]) {
      case Instructions_Mov: {
        instr.decodeMov(state->mem.bytes, i);
        break;
      }
      case Instructions_Add: {
        instr.decodeAdd(state->mem.bytes, i);
        break;
      }
      case Instructions_Sub: {
        instr.decodeSub(state->mem.bytes, i);
        break;
      }
      case Instructions_Cmp: {
        instr.decodeCmp(state->mem.bytes, i);
        break;
      }
      case Instructions_Jnz: {
        instr.decodeJnz(state->mem.bytes, i);
        break;
      }
      case Instructions_AddSubCmp: {
        Instructions inst = dispatchAddSubCmp(state->mem.bytes, i);
        if (inst == Instructions_Add) {
          instr.decodeAdd(state->mem.bytes, i);
        } else if (inst == Instructions_Sub) {
          instr.decodeSub(state->mem.bytes, i);
        } else if (inst == Instructions_Cmp) {
          instr.decodeCmp(state->mem.bytes, i);
        } else {
          // TODO(chogan): ERROR
        }
        break;
      }
      default:
        assert(false);
        break;
    }

    u8 ip_index = access_patterns[Registers_ip].index;
    state->prev[ip_index].x = *ip;
    *ip += instr.size;

    if (!instr.d_bit) {
      std::swap(instr.dest, instr.source);
    }

    if (args->exec) {
      execInstruction(&instr, state);
      instr.emitInstruction(output_file, state);
    } else {
      instr.emitInstruction(output_file, NULL);
    }
  }

  if (args->exec) {
    printf("Final registers:\n");
    const char *reg_names[] = {"ax", "bx", "cx", "dx", "sp", "bp", "si", "di", "ip"};
    for (int i = 0; i < kNumRegisters; ++i) {
      const char *reg = reg_names[i];
      u16 hex = state->registers[i].x;
      if (hex) {
        printf("\t\t%s: 0x%04x (%d)\n", reg, hex, hex);
      }
    }
    printf("\tflags: ");
    if (state->flags & Flags_Zero) {
      printf("Z");
    }
    if (state->flags & Flags_Sign) {
      printf("S");
    }
    printf("\n");

    if (args->dump) {
      ofstream mem_file("sim86_memory_0.data", std::ios::binary);
      mem_file.write((char *)state->mem.bytes, state->mem.used + 64 * 64 * 4);
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
  } else if (argc == 4 && strcmp(argv[1], "-exec") == 0 &&
             strcmp(argv[2], "-dump") == 0) {
    result.exec = true;
    result.dump = true;
    result.fname = argv[3];
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

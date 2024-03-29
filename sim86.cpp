
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
typedef uint32_t u32;

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

unordered_map<u8, string> opcodes = {
  {0b100010, "mov"},
  {0b001011, "mov"}
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
  u8 disp_h;
  u8 disp_l;

  void decodeMov(const vector<u8> &data, size_t offset) {
    u8 b1 = data[offset];
    u8 b2 = data[offset + 1];

    u8 key = (b1 & 0b11111100) >> 2;
    instruction = opcodes[key];
    d_bit = (b1 & 0b00000010) >> 1;
    w_bit = (b1 & 0b00000001);

    mode = (b2 & 0b11000000) >> 6;
    reg = (b2 & 0b00111000) >> 3;
    rm = (b2 & 0b00000111);
  }

  void emitInstruction(ofstream &os) {
    string source = getRegister(reg, w_bit);
    string dest = getRegister(rm, w_bit);

    os << instruction << " " << dest << ", " << source << endl;
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
    int instruction_size = 2;
    DecodedInstruction instr = {};

    // TODO(chogan): determine instruction type

    // TODO(chogan): dispatch to correct decode function
    instr.decodeMov(file_data, i);
    instr.emitInstruction(output_file);

    i += instruction_size;
  }

  return 0;
}

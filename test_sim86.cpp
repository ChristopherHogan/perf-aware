#define SIM86_MAIN 0

#include "sim86.cpp"

#define arraySize(arr) (sizeof(arr) / sizeof(arr[0]))

int main() {

  const char *kFiles[] = {
    "listing_0043_immediate_movs",
    "listing_0044_register_movs",
    "listing_0046_add_sub_cmp",
    "listing_0048_ip_register",
    "listing_0049_conditional_jumps"
  };

  const u8 kExpectedFlags[] = {
    0,
    0,
    Flags_Zero,
    Flags_Sign,
    Flags_Zero
  };

  const Register kExpectedRegisters[][kNumRegisters] = {
    {{.x = 1}, {.x = 2}, {.x = 3}, {.x = 4}, {.x = 5}, {.x = 6}, {.x = 7}, {.x = 8}, {.x = 0}},
    {{.x = 4}, {.x = 3}, {.x = 2}, {.x = 1}, {.x = 1}, {.x = 2}, {.x = 3}, {.x = 4}, {.x = 0}},
    {{.x = 0}, {.x = 57602}, {.x = 3841}, {.x = 0}, {.x = 998}, {.x = 0}, {.x = 0}, {.x = 0}, {.x = 0}},
    {{.x = 0}, {.x = 2000}, {.x = 64736}, {.x = 0}, {.x = 0}, {.x = 0}, {.x = 0}, {.x = 0}, {.x = 14}},
    {{.x = 0}, {.x = 1030}, {.x = 0}, {.x = 0}, {.x = 0}, {.x = 0}, {.x = 0}, {.x = 0}, {.x = 14}}
  };

  for (size_t i = 0; i < arraySize(kFiles); ++i) {
    Arguments args = {};
    args.fname = (char*)kFiles[i];
    args.exec = true;
    MachineState state = {};
    run(&args, &state);

    for (int j = 0; j < kNumRegisters; ++j) {
      if (j == kNumRegisters - 1 && kExpectedRegisters[i][j].x == 0) {
        continue;
      }
      assert(state.registers[j].x == kExpectedRegisters[i][j].x);
    }
    assert(state.flags == kExpectedFlags[i]);
  }

  return 0;
}

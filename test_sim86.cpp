#define SIM86_MAIN 0

#include "sim86.cpp"

#define arraySize(arr) (sizeof(arr) / sizeof(arr[0]))

int main() {

  const char *files[] = {
    "listing_0043_immediate_movs",
    "listing_0044_register_movs"
  };

  const Register expected[][kNumRegisters] = {
    {{.x = 1}, {.x = 2}, {.x = 3}, {.x = 4}, {.x = 5}, {.x = 6}, {.x = 7}, {.x = 8}},
    {{.x = 4}, {.x = 3}, {.x = 2}, {.x = 1}, {.x = 1}, {.x = 2}, {.x = 3}, {.x = 4}}
  };

  for (size_t i = 0; i < arraySize(files); ++i) {
    Arguments args = {};
    args.fname = (char*)files[i];
    args.exec = true;
    MachineState state = {};
    run(&args, &state);

    for (int j = 0; j < kNumRegisters; ++j) {
      assert(state.registers[j].x == expected[i][j].x);
    }
  }

  return 0;
}

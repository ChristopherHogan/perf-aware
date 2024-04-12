#include <stdio.h>

#include "perfaware_json_parser.h"
#include "perfaware_memory.h"

EntireFile readEntireFile(Arena *arena, const char *path) {
  EntireFile result = {};
  FILE *fstream = fopen(path, "r");

  if (fstream) {
    int fseek_result = fseek(fstream, 0, SEEK_END);

    if (fseek_result == 0) {
      long file_size = ftell(fstream);

      if (file_size > 0) {
        if ((u32)file_size <= arena->capacity) {
          fseek(fstream, 0, SEEK_SET);
          result.data = pushArray<u8>(arena, file_size);
          [[maybe_unused]] int items_read = fread(result.data, file_size, 1, fstream);
          assert(items_read == 1);
          result.size = file_size;
        } else {
          // LOG(FATAL) << "Arena capacity (" << arena->capacity
          //            << ") too small to read file of size (" << file_size
          //            << ")" << std::endl;
        }

      } else {
      }
    } else {
      // FailedLibraryCall("fseek");
    }

    if (fclose(fstream) != 0) {
      // FailedLibraryCall("fclose");
    }

  } else {
    // FailedLibraryCall("fopen");
  }

  return result;
}

bool isWhitespace(char c) {
  bool result = c == ' ' || c == '\t' || c == '\n' || c == '\r';

  return result;
}

bool beginsNumber(char c) {
  bool result = (c >= '0' && c <= '9') || (c == '.') || (c == '-');

  return result;
}

bool endOfNumber(char c) {
  bool result = (c == ',') || (c == '}');

  return result;
}

bool isEndOfLine(char **at, char *end) {
  // Linux style
  bool result = (*at)[0] == '\n';

  // Windows style
  if (*at + 1 < end && !result) {
    result = ((*at)[0] == '\r' && (*at)[1] == '\n');
    if (result) {
      (*at)++;
    }
  }

  return result;
}

Token *getLastToken(TokenArray *arr) {
  Token *result = NULL;

  if (arr && arr->head) {
    result = &arr->head[arr->count - 1];
  } else {
    fprintf(stderr, "ERROR: arr is NULL: %s\n", __PRETTY_FUNCTION__);
  }

  return result;
}

void addTokenToArray(Arena *arena, TokenArray *arr, TokenType type,
                    u32 line_number) {
  Token *tok = pushClearedStruct<Token>(arena);
  tok->type = type;
  tok->line = line_number;
  if (!arr->head) {
    arr->head = tok;
  }
  arr->count++;
}

TokenArray tokenize(Arena *arena, EntireFile *entire_file) {
  TimeFunction;
  u32 line_number = 1;
  TokenArray result = {};

  char *at = (char *)entire_file->data;
  char *end = at + entire_file->size;

  while (at < end) {
    if (isWhitespace(*at)) {
      if (isEndOfLine(&at, end)) {
        line_number++;
      }

      ++at;
      continue;
    }

    if (beginsNumber(*at)) {
      addTokenToArray(arena, &result, TokenType::Number, line_number);
      getLastToken(&result)->data = at;

      while (at && !endOfNumber(*at)) {
        getLastToken(&result)->size++;
        at++;
      }
      result.num_points++;
    } else {
      switch (*at) {
        case ',': {
          addTokenToArray(arena, &result, TokenType::Comma, line_number);
          break;
        }
        case '{': {
          addTokenToArray(arena, &result, TokenType::OpenCurlyBrace, line_number);
          break;
        }
        case '}': {
          addTokenToArray(arena, &result, TokenType::CloseCurlyBrace, line_number);
          break;
        }
        case '[': {
          addTokenToArray(arena, &result, TokenType::OpenBrace, line_number);
          break;
        }
        case ']': {
          addTokenToArray(arena, &result, TokenType::CloseBrace, line_number);
          break;
        }
        case ':': {
          addTokenToArray(arena, &result, TokenType::Colon, line_number);
          break;
        }
        case '"': {
          addTokenToArray(arena, &result, TokenType::String, line_number);
          at++;
          getLastToken(&result)->data = at;

          while (at && *at != '"') {
            getLastToken(&result)->size++;
            at++;
          }
          break;
        }
        default: {
          fprintf(stderr, "Config parser encountered unexpected token on line %u: %c\n", line_number, *at);
          break;
        }
      }
      at++;
    }
  }

  return result;
}

PointArray parseTokens(Arena *arena, TokenArray *tokens) {
  TimeFunction;
  u32 num_points = tokens->num_points / 4.0;
  Point *data = pushArray<Point>(arena, num_points);
  PointArray result = {};
  result.data = data;
  result.num_points = num_points;

  Token *at = tokens->head;
  Token *end = at + tokens->count - 2;
  u32 offset = 0;

  ++at;  // {
  ++at;  // pairs
  ++at;  // :
  ++at;  // [

  while (at < end) {
    ++at;  // {
    ++at;  // x0
    ++at;  // :
    f64 x0 = atof(at->data);
    ++at;

    ++at;  // ,
    ++at;  // y0
    ++at;  // :
    f64 y0 = atof(at->data);
    ++at;

    ++at;  // ,
    ++at;  // x1
    ++at;  // :
    f64 x1 = atof(at->data);
    ++at;

    ++at;  // ,
    ++at;  // y1
    ++at;  // :
    f64 y1 = atof(at->data);
    ++at;

    ++at;  // }
    ++at;  // ,

    result.data[offset].x0 = x0;
    result.data[offset].y0 = y0;
    result.data[offset].x1 = x1;
    result.data[offset].y1 = y1;
    offset++;
  }

  return result;
}

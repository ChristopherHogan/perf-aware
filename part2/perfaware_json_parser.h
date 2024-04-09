#ifndef PERFAWARE_JSON_PARSER_H_
#define PERFAWARE_JSON_PARSER_H_

enum TokenType {
  Number,
  String,
  OpenBrace,
  CloseBrace,
  OpenCurlyBrace,
  CloseCurlyBrace,
  Comma,
  Colon,

  Count
};

struct Token {
  char *data;
  u32 size;
  u32 line;
  TokenType type;
};

struct TokenArray {
  Token *head;
  u32 count;
  u32 num_points;
};

struct EntireFile {
  u8 *data;
  u64 size;
};

TokenArray tokenize(Arena *arena, EntireFile entire_file);
void parseTokens(TokenArray *tokens);

#endif  // PERFAWARE_JSON_PARSER_H_

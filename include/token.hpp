#pragma once

#include <source_range.hpp>
#include <symbol.hpp>
#include <cstdint>
#include <cstdio>

enum class token_kind : std::int8_t
{
#define TOK(x,y,z) x = z,
#include <token_list.hpp>
#undef TOK
};

std::string_view kind_to_str(token_kind kind);

class token
{
public:
  token(token_kind kind, symbol data, source_range range)
    : kind(kind), data(data), loc(range)
  {  }

  token()
    : kind(token_kind::Undef), data(""), loc()
  {  }

  token_kind kind;
  symbol data;
  source_range loc;
};


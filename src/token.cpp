#include <token.hpp>
#include <cassert>

std::string_view kind_to_str(token_kind kind)
{
  switch(kind)
  {
  default:
#define TOK(x,y,z) case token_kind::x: return y;
#include <token_list.hpp>
#undef TOK
  }
}


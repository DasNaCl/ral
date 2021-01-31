#include <stream_lookup.hpp>
#include <token.hpp>
#include <type.hpp>
#include <ast.hpp>

#include <iostream>
#include <sstream>
#include <set>

auto token_precedence_map = tsl::robin_map<token_kind, int>( {
  {token_kind::Plus, 5},
  {token_kind::Minus, 5},
  {token_kind::Asterisk, 6},
  {token_kind::Slash, 6},
  {token_kind::LParen, 65535},
  {token_kind::Identifier, 1},
  {token_kind::LiteralNumber, 65535},
  {token_kind::Less, 4},
  {token_kind::LessEqual, 4},
  {token_kind::Equal, 4},
  {token_kind::ExclamationEqual, 4},
  {token_kind::GreaterEqual, 4},
  {token_kind::Greater, 4},
});

class parser
{
  friend std::vector<Fn::Ptr> read(std::string_view module);
  friend std::vector<Fn::Ptr> read_text(const std::string& module);
public:
  static constexpr std::size_t lookahead_size = 2;

  static Node::Ptr read(std::string_view module);
  static Node::Ptr read_text(const std::string& str);

  std::set<Object> locs;
private:
  parser(std::string_view module)
    : module(module)
    , is(stream_lookup[module])
    , linebuf()
    , col(0)
    , row(1)
    , uses_reader(true)
  {
    for(std::size_t i = 0; i < next_toks.size(); ++i)
      consume();

    // need one additional consume to initialize `current`
    consume(); 
  }

  parser(std::istream& is)
    : module("#TXT#")
    , is(is)
    , linebuf()
    , col(0)
    , row(1)
    , uses_reader(false)
  {
    for(std::size_t i = 0; i < next_toks.size(); ++i)
      consume();

    // need one additional consume to initialize `current`
    consume(); 
  }

  ~parser()
  { if(uses_reader) stream_lookup.drop(module); }

  token gett();

  void expect(char c);
  void expect(token_kind c);
  bool accept(char c);
  bool accept(token_kind c);
  void consume();
private:
  char getc();
private:
  // always uses old for the error
  Node::Ptr mk_error();

  Type::Ptr parse_type();

  Fn::Ptr parse_fn();

  Node::Ptr parse_expr_stmt();
  Node::Ptr parse_block();
  Node::Ptr parse_let(bool is_unlet = false);
  Node::Ptr parse_do_yield_undo();
  Node::Ptr parse_if();
  Node::Ptr parse_loop();
  Node::Ptr parse_swap();

  Node::Ptr parse_call();
  Node::Ptr parse_identifier();
  Node::Ptr parse_statement();
  Node::Ptr parse_prefix();
  Node::Ptr parse_expression(int precedence = 0);
  int precedence();

private:
  std::string_view module;
  std::istream& is;

  std::string linebuf;

  std::size_t col;
  std::size_t row;

  token old;
  token current;
  std::array<token, lookahead_size> next_toks;

  bool uses_reader;
};

void parser::consume()
{
  old = current;
  current = next_toks[0];

  for(std::size_t i = 0, j = 1; j < next_toks.size(); ++i, ++j)
    std::swap(next_toks[i], next_toks[j]);

  next_toks.back() = gett();
}

bool parser::accept(char c)
{
  if(current.kind != static_cast<token_kind>(c))
    return false;

  consume();
  return true;
}

bool parser::accept(token_kind c)
{
  if(current.kind != c)
    return false;

  consume();
  return true;
}

void parser::expect(char c)
{
  if(current.kind != static_cast<token_kind>(c))
  {
    std::cerr << "Expected " << c << " but got "
              << kind_to_str(current.kind) << "\n";
    assert(false);
  }
  consume();
}

void parser::expect(token_kind c)
{
  if(current.kind != c)
  {
    std::cerr << "Expected " << kind_to_str(c) << " but got "
              << kind_to_str(current.kind) << "\n";
    assert(false);
  }
  consume();
}

Node::Ptr parser::mk_error()
{
  // TODO
  assert(false);
}

////// Parsing 

Type::Ptr parser::parse_type()
{
  if(accept(token_kind::LParen))
  {
    if(accept(token_kind::RParen))
      return void_type();

    auto ty = parse_type();
    expect(token_kind::RParen);
    return ty;
  }
  parse_identifier();

  auto ty = str2typ(old.data.str());
  if(!ty)
    return nullptr;

  return ty;
}

// fn IDENTIFIER ( IDENTIFIER : TYPE,* ) -> TYPE := stmt
Fn::Ptr parser::parse_fn()
{
  consume();
  if(old.data != "fn")
    return nullptr;

  parse_identifier();
  auto name = old.data.str();
  expect(token_kind::LParen);

  bool first = true;
  std::vector<Object> params;
  std::vector<Type::Ptr> par_typs;
  while(current.kind != token_kind::RParen && current.kind != token_kind::EndOfFile)
  {
    if(!first)
      expect(token_kind::Comma);

    parse_identifier();
    auto par_name = old.data.str();
    expect(token_kind::DoubleColon);

    auto typ = parse_type();

    par_typs.emplace_back(typ);
    params.emplace_back(par_name, std::move(typ));
    first = false;
  }
  expect(token_kind::RParen);
  expect(token_kind::MinusGreater);

  auto fn_typ = fn_type(std::move(par_typs), parse_type());

  expect(token_kind::DoubleColonEqual);
  auto body = parse_statement();

  return std::make_unique<Fn>(std::move(name), std::move(params), std::move(fn_typ), std::move(body));
}

Node::Ptr parser::parse_expr_stmt()
{
  return make_node(NodeKind::Stmt, parse_expression());
}

Node::Ptr parser::parse_let(bool is_unlet)
{
  consume();
  auto id = parse_identifier();
  expect(token_kind::DoubleColonEqual);
  
  auto exp = parse_expression();

  return make_node(is_unlet ? NodeKind::Unlet : NodeKind::Let, std::move(id), std::move(exp));
}

// do S₁ yield S₂ undo
Node::Ptr parser::parse_do_yield_undo()
{
  consume();
  if(old.data != "do")
    return mk_error();
  auto bb = parse_statement();

  expect(token_kind::Keyword);
  if(old.data != "yield")
    return mk_error();

  auto bc = parse_statement();

  expect(token_kind::Keyword);
  if(old.data != "undo")
    return mk_error();

  return make_node(NodeKind::DoYieldUndo, std::move(bb), std::move(bc));
}

// from E₁ do S until E₂
Node::Ptr parser::parse_loop()
{
  consume();
  if(old.data != "from")
    return mk_error();

  auto cond = parse_expression();
  expect(token_kind::Keyword);
  if(old.data != "do")
    return mk_error();

  auto loop = parse_block();
  
  expect(token_kind::Keyword);
  if(old.data != "until")
    return mk_error();
  auto cond2 = parse_expression();

  return make_node(NodeKind::Loop, std::move(cond), std::move(loop), std::move(cond2));
}

// IDENT <> IDENT
Node::Ptr parser::parse_swap()
{
  auto id = parse_identifier();

  expect(token_kind::LessGreater);

  auto e = parse_identifier();
  return make_node(NodeKind::Swap, std::move(id), std::move(e));
}

Node::Ptr parser::parse_if()
{
  consume();

  std::vector<Node::Ptr> args;
  args.emplace_back(parse_expression());

  args.emplace_back(parse_block());
  if(accept(token_kind::Keyword))
  {
    if(old.data != "else")
      assert(false);

    // allow `else if`
    if(current.kind == token_kind::Keyword && current.data == "if")
      args.emplace_back(parse_if());
    else
      args.emplace_back(parse_block());
  }
  return make_node(NodeKind::If, std::move(args));
}

Node::Ptr parser::parse_block()
{
  expect('{');

  std::vector<Node::Ptr> stmts;
  bool first = true;
  while(current.kind != token_kind::RBrace && current.kind != token_kind::EndOfFile)
  {
    if(!first)
      expect(';');

    // handle empty statement
    if(!accept(';'))
      stmts.emplace_back(parse_statement());
    first = false;
  }
  expect('}');

  return make_node(NodeKind::Block, std::move(stmts));
}

Node::Ptr parser::parse_statement()
{
  if(current.kind == token_kind::Keyword)
  {
    if(current.data == "let")
      return parse_let();
    else if(current.data == "unlet")
      return parse_let(true);
    else if(current.data == "do")
      return parse_do_yield_undo();
    else if(current.data == "if")
      return parse_if();
    else if(current.data == "from")
      return parse_loop();
    else if(current.data == "call")
      return parse_call();
    else
      return mk_error(); // TODO
  }
  if(current.kind == token_kind::Identifier)
  {
    const auto parse_bin = [this](BinOpTypes typ)
    {
      auto lhs = parse_identifier();
      consume(); // <- already check'd

      auto rhs = parse_expression();

      return make_node(NodeKind::OpEq, std::move(typ), std::move(lhs), std::move(rhs));
    };
    switch(next_toks[0].kind)
    {
    default: return mk_error();
    case token_kind::LessGreater:   return parse_swap();
    case token_kind::PlusEqual:     return parse_bin(BinOpTypes::Add);
    case token_kind::MinusEqual:    return parse_bin(BinOpTypes::Sub);
    case token_kind::AsteriskEqual: return parse_bin(BinOpTypes::Mul);
    case token_kind::SlashEqual:    return parse_bin(BinOpTypes::Div);
    }
  }
  else if(current.kind == token_kind::LBrace)
    return parse_block();
  else
    return parse_expr_stmt();
  return mk_error();
}

Node::Ptr parser::parse_call()
{
  consume();
  if(old.data != "call")
    assert(false);

  auto id = parse_identifier();

  std::vector<Node::Ptr> params;
  params.emplace_back(std::move(id));
  expect(token_kind::LParen);
  bool first = true;
  while(current.kind != token_kind::RParen && current.kind != token_kind::EndOfFile)
  {
    if(!first)
      expect(token_kind::Comma);
    
    params.emplace_back(parse_expression());
  }
  expect(token_kind::RParen);
  return make_node(NodeKind::Call, std::move(params));
}

Node::Ptr parser::parse_identifier()
{
  consume();

  Object obj = { old.data.str(), nullptr };
  if(locs.count(obj) == 0)
    locs.insert(obj);

  auto it = locs.find(obj);
  assert(it != locs.end());
  return make_node(NodeKind::Var, Node::Data { *it });
}

Node::Ptr parser::parse_prefix()
{
  switch(current.kind)
  {
  default: assert(false); // TODO

  case token_kind::Identifier:
  {
    return parse_identifier();
  }

  case token_kind::LiteralNumber:
  {
    consume();
    const std::size_t num = std::stoull(old.data.str());
    return make_node(NodeKind::Num, Node::Data { num });
  }

  case token_kind::LParen:
  {
    consume();
    auto res = parse_expression();
    expect(')');

    return res;
  }
  }
}

Node::Ptr parser::parse_expression(int prec)
{
  auto pref = parse_prefix();
  auto parse_cmp = [this,&pref](CmpTypes&& type) {
    consume();
    auto right = parse_expression(token_precedence_map[old.kind]);

    pref = make_node(NodeKind::Cmp, std::move(type), std::move(pref), std::move(right));
  };

  while(prec < precedence())
  {
    switch(current.kind)
    {
    case token_kind::EndOfFile:
    case token_kind::Semi:
    case token_kind::RParen:
    case token_kind::RBrace:
      break;

    case token_kind::Less:             parse_cmp(CmpTypes::Less); break;
    case token_kind::LessEqual:        parse_cmp(CmpTypes::LessEqual); break;
    case token_kind::Equal:            parse_cmp(CmpTypes::Equal); break;
    case token_kind::ExclamationEqual: parse_cmp(CmpTypes::InEqual); break;
    case token_kind::Greater:          parse_cmp(CmpTypes::Greater); break;
    case token_kind::GreaterEqual:     parse_cmp(CmpTypes::GreaterEqual); break;

    default:
      assert(false);
    }
    if(!pref)
      break;
  }

  return pref;
}

int parser::precedence()
{
  token_kind prec = current.kind;
  if (prec == token_kind::Undef || prec == token_kind::EndOfFile || prec == token_kind::Semi)
    return 0;
  return token_precedence_map[prec];
}



///// Tokenization
using namespace std::literals::string_view_literals;

auto operator_symbols_map = tsl::robin_map<std::string_view, token_kind>({
  {";"sv, token_kind::Semi},
  {"+"sv, token_kind::Plus},
  {"-"sv, token_kind::Minus},
  {"*"sv,  token_kind::Asterisk},
  {"/"sv,  token_kind::Slash},
  {"+="sv, token_kind::PlusEqual },
  {"-="sv, token_kind::MinusEqual },
  {"*="sv, token_kind::AsteriskEqual },
  {"/="sv, token_kind::SlashEqual },
  {"="sv,  token_kind::Equal},
  {"("sv,  token_kind::LParen},
  {")"sv,  token_kind::RParen},
  {"<>"sv, token_kind::LessGreater},
  {"<"sv,  token_kind::Less},
  {"<="sv, token_kind::LessEqual},
  {"="sv,  token_kind::Equal},
  {"!="sv, token_kind::ExclamationEqual},
  {">="sv, token_kind::GreaterEqual},
  {":="sv, token_kind::DoubleColonEqual},
  {">"sv,  token_kind::Greater},
});
auto keyword_set = tsl::robin_set<std::string_view>({
  "fn",
  "let",
  "unlet",
  "call",
  "yield",
  "do",
  "undo",
  "if",
  "else",
  "from",
  "until",
});

char parser::getc()
{
  // yields the next char that is not a basic whitespace character i.e. NOT stuff like zero width space
  char ch = 0;
  bool skipped_line = false;
  do
  {
    if(col >= linebuf.size())
    {
      if(!linebuf.empty())
        row++;
      if(!(std::getline(is, linebuf)))
      {
        col = 1;
        linebuf = "";
        return EOF;
      }
      else
      {
        while(linebuf.empty())
        {
          row++;
          if(!(std::getline(is, linebuf)))
          {
            col = 1;
            linebuf = "";
            return EOF;
          }
        }
      }
      col = 0;
    }
    ch = linebuf[col++];
    if(std::isspace(ch))
    {
      skipped_line = true;
      while(col < linebuf.size())
      {
        ch = linebuf[col++];
        if(!(std::isspace(ch)))
        {
          skipped_line = false;
          break;
        }
      }
    }
    else
      skipped_line = false;
  } while(skipped_line);

  return ch;
}
token parser::gett()
{
restart_get:
  symbol data("");
  token_kind kind = token_kind::Undef;

  char ch = getc();
  std::size_t beg_row = row;
  std::size_t beg_col = col - 1;

  bool starts_with_zero = false;
  switch(ch)
  {
  default:
    if(!std::iscntrl(ch) && !std::isspace(ch))
    {
      // Optimistically allow any kind of identifier to allow for unicode
      std::string name;
      name.push_back(ch);
      while(col < linebuf.size())
      {
        // don't use get<char>() here, identifiers are not connected with a '\n'
        ch = linebuf[col++];

        // we look up in the operator map only for the single char. otherwise we wont parse y:= not correctly for example
        std::string ch_s;
        ch_s.push_back(ch);
        // break if we hit whitespace (or other control chars) or any other operator symbol char
        if(std::iscntrl(ch) || std::isspace(ch)
        || operator_symbols_map.contains(ch_s.c_str())
        || keyword_set.contains(name.c_str()))
        {
          col--;
          break;
        }
        name.push_back(ch);
      }
      kind = (operator_symbols_map.contains(name.c_str())
              ? operator_symbols_map[name.c_str()]
              : token_kind::Identifier);
      kind = (keyword_set.contains(name.c_str())
              ? token_kind::Keyword
              : kind);

      data = symbol(name);
    }
    else
    {
      assert(false && "Control or space character leaked into get<token>!");
    }
    break;

  case '0': starts_with_zero = true; case '1': case '2': case '3': case '4':
  case '5': case '6': case '7': case '8': case '9':
    {
      // readin until there is no number anymore. Disallow 000840
      std::string name;
      name.push_back(ch);
      bool emit_not_a_number_error = false;
      bool emit_starts_with_zero_error = false;
      while(col < linebuf.size())
      {
        // don't use get<char>() here, numbers are not connected with a '\n'
        ch = linebuf[col++];

        // we look up in the operator map only for the single char. otherwise we wont parse 2; not correctly for example
        // This could give problems with floaing point numbers since 2.3 will now be parsed as "2" "." "3" so to literals and a point
        std::string ch_s;
        ch_s.push_back(ch);
        // break if we hit whitespace (or other control chars) or any other token char
        if(std::iscntrl(ch) || std::isspace(ch) || operator_symbols_map.count(ch_s.c_str()) || keyword_set.count(name.c_str()))
        {
          col--;
          break;
        }
        if(!std::isdigit(ch))
          emit_not_a_number_error = true;
        if(starts_with_zero && !emit_not_a_number_error)
          emit_starts_with_zero_error = true;
        name.push_back(ch);
      }
      if(emit_starts_with_zero_error)
      {
        // TODO
        goto restart_get; // <- SO WHAT
      }
      if(emit_not_a_number_error)
      {
        // TODO
        goto restart_get;
      }
      kind = token_kind::LiteralNumber;
      data = symbol(name);
    } break;
  case '>':
  {
    if(col < linebuf.size() && linebuf[col] == '=')
    {
      kind = token_kind::GreaterEqual;
      data = ">=";
      ch = linebuf[col++];
    }
    else
    {
      kind = token_kind::Greater;
      data = ">";
    }
  } break;
  case '<':
  {
    if(col < linebuf.size() && linebuf[col] == '=')
    {
      kind = token_kind::LessEqual;
      data = "<=";
      ch = linebuf[col++];
    }
    else if(col < linebuf.size() && linebuf[col] == '>')
    {
      kind = token_kind::LessGreater;
      data = "<>";
      ch = linebuf[col++];
    }
    else
    {
      kind = token_kind::Less;
      data = "<";
    }
  } break;
  case '=':
  {
    kind = token_kind::Equal;
    data = "=";
  } break;
  case '*':
  {
    if(col < linebuf.size() && linebuf[col] == '=')
    {
      kind = token_kind::AsteriskEqual;
      data = "*=";
      ch = linebuf[col++];
    }
    else
    {
      kind = token_kind::Asterisk;
      data = "*";
    }
  } break;
  case '/':
  {
    if(col < linebuf.size() && linebuf[col] == '=')
    {
      kind = token_kind::SlashEqual;
      data = "/=";
      ch = linebuf[col++];
    }
    else
    {
      kind = token_kind::Slash;
      data = "/";
    }
  } break;
  case '+':
  {
    if(col < linebuf.size() && linebuf[col] == '=')
    {
      kind = token_kind::PlusEqual;
      data = "+=";
      ch = linebuf[col++];
    }
    else
    {
      kind = token_kind::Plus;
      data = "+";
    }
  } break;
  case '-':
  {
    if(col < linebuf.size() && linebuf[col] == '=')
    {
      kind = token_kind::MinusEqual;
      data = "-=";
      ch = linebuf[col++];
    }
    else if(col < linebuf.size() && linebuf[col] == '>')
    {
      kind = token_kind::MinusGreater;
      data = "->";
      ch = linebuf[col++];
    }
    else
    {
      kind = token_kind::Minus;
      data = "-";
    }
  } break;
  case ';':
  {
    kind = token_kind::Semi;
    data = ";";
  } break;
  case ',':
  {
    kind = token_kind::Comma;
    data = ",";
  } break;
  case ':':
  {
    if(col < linebuf.size() && linebuf[col] == '=')
    {
      kind = token_kind::DoubleColonEqual;
      data = ":=";
      ch = linebuf[col++];
    }
    else
    {
      kind = token_kind::DoubleColon;
      data = ":";
    }
  } break;
  case '!':
  {
    if(col < linebuf.size() && linebuf[col] == '=')
    {
      kind = token_kind::ExclamationEqual;
      data = "!=";
      ch = linebuf[col++];
    }
  } break;
  case '{':
  {
    kind = token_kind::LBrace;
    data = "{";
  } break;
  case '}':
  {
    kind = token_kind::RBrace;
    data = "}";
  } break;
  case '(':
  {
    kind = token_kind::LParen;
    data = "(";
  } break;
  case ')':
  {
    kind = token_kind::RParen;
    data = ")";
  } break;
  case EOF:
      kind = token_kind::EndOfFile;
      data = "EOF";
    break;
  }
  return token(kind, data, {module, beg_col + 1, beg_row, col + 1, row + 1});
}

std::vector<Fn::Ptr> read(std::string_view module)
{
  parser r(module);

  std::vector<Fn::Ptr> stmts;
  while(r.current.kind != token_kind::EndOfFile)
  {
    auto stmt = r.parse_fn();

    stmts.emplace_back(std::move(stmt));
  }

  return stmts;
}

std::vector<Fn::Ptr> read_text(const std::string& str)
{
  std::stringstream ss(str);
  parser r(ss);

  std::vector<Fn::Ptr> stmts;
  while(r.current.kind != token_kind::EndOfFile)
  {
    auto stmt = r.parse_fn();

    stmts.emplace_back(std::move(stmt));
  }

  return stmts;
}


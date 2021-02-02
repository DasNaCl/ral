#pragma once

#include <variant>
#include <memory>
#include <string>
#include <vector>

struct Type;

struct Object
{
  Object(const std::string& name, std::shared_ptr<Type> type)
    : name(name)
    , type(type)
  {  }

  std::string name;
  std::shared_ptr<Type> type;
};
inline bool operator<(const Object& lhs, const Object& rhs)
{ return lhs.name < rhs.name; }

struct Node;

enum class NodeKind {
  Unit,
  Num,
  Var,
  OpEq,
  Cmp,
  Let,
  Unlet,
  If,
  DoYieldUndo,
  Block,
  Loop,
  Swap,
  Call,
  Uncall,
  Stmt,
  Fn,
};

struct Fn
{
  using Ptr = std::unique_ptr<Fn>;

  Fn(std::string&& name, std::vector<Object>&& params, std::shared_ptr<Type>&& typ, std::unique_ptr<Node>&& body)
    : name(std::move(name))
    , params(std::move(params))
    , body(std::move(body))
    , typ(std::move(typ))
  {  }

  std::string name;
  bool exported;
  bool imported;

  std::vector<Object> params;
  std::unique_ptr<Node> body;

  std::shared_ptr<Type> typ;
};

// +=, -=, *=, /=
enum class BinOpTypes
{
  Add,
  Sub,
  Mul,
  Div,
};

enum class CmpTypes
{
  Less,
  LessEqual,
  Equal,
  InEqual,
  GreaterEqual,
  Greater,
};

struct Node {
  using Ptr = std::unique_ptr<Node>;
  using Data = std::variant<std::monostate, std::size_t, Object, CmpTypes, BinOpTypes, std::string>;

  Node(NodeKind&& kind, Data&& dat)
    : kind(kind)
    , lhs()
    , data(std::move(dat))
  {  }

  Node(NodeKind&& kind, std::vector<Ptr>&& dat)
    : kind(kind)
    , lhs(std::move(dat))
    , data(std::monostate{})
  {  }

  Node(NodeKind&& kind, Data&& dat, std::vector<Ptr>&& vec)
    : kind(kind)
    , lhs(std::move(vec))
    , data(std::move(dat))
  {  }

  Node(Node&& other)
    : kind(other.kind)
    , lhs(std::move(other.lhs))
    , data(std::move(other.data))
  {  }

  static std::string_view kind_to_str(NodeKind kind);

  NodeKind kind;
  std::vector<Ptr> lhs;

  Data data;

  std::shared_ptr<Type> typ;
};


inline Node::Ptr make_node(NodeKind&& kind, Node::Data&& dat)
{ return std::make_unique<Node>(std::move(kind), std::move(dat)); }

inline Node::Ptr make_node(NodeKind&& kind, Node::Ptr&& arg0)
{
  std::vector<Node::Ptr> v;
  v.emplace_back(std::move(arg0));
  return std::make_unique<Node>(std::move(kind), std::move(v));
}
inline Node::Ptr make_node(NodeKind&& kind, Node::Ptr&& arg0, Node::Ptr&& arg1)
{
  std::vector<Node::Ptr> v;
  v.emplace_back(std::move(arg0));
  v.emplace_back(std::move(arg1));
  return std::make_unique<Node>(std::move(kind), std::move(v));
}
inline Node::Ptr make_node(NodeKind&& kind, Node::Ptr&& arg0, Node::Ptr&& arg1, Node::Ptr&& arg2)
{
  std::vector<Node::Ptr> v;
  v.emplace_back(std::move(arg0));
  v.emplace_back(std::move(arg1));
  v.emplace_back(std::move(arg2));
  return std::make_unique<Node>(std::move(kind), std::move(v));
}
inline Node::Ptr make_node(NodeKind&& kind, std::vector<Node::Ptr>&& args)
{
  return std::make_unique<Node>(std::move(kind), std::move(args));
}

inline Node::Ptr make_node(NodeKind&& kind, Node::Data&& dat, std::vector<Node::Ptr>&& args)
{
  return std::make_unique<Node>(std::move(kind), std::move(dat), std::move(args));
}
inline Node::Ptr make_node(NodeKind&& kind, CmpTypes&& cmp, Node::Ptr&& arg0, Node::Ptr&& arg1)
{
  std::vector<Node::Ptr> v;
  v.emplace_back(std::move(arg0));
  v.emplace_back(std::move(arg1));
  return std::make_unique<Node>(std::move(kind), std::move(cmp), std::move(v));
}
inline Node::Ptr make_node(NodeKind&& kind, BinOpTypes&& bin, Node::Ptr&& arg0, Node::Ptr&& arg1)
{
  std::vector<Node::Ptr> v;
  v.emplace_back(std::move(arg0));
  v.emplace_back(std::move(arg1));
  return std::make_unique<Node>(std::move(kind), std::move(bin), std::move(v));
}

inline bool is_stmt(NodeKind kind)
{
  switch(kind)
  {
  default:
    return false;

  case NodeKind::Let:
  case NodeKind::DoYieldUndo:
  case NodeKind::If:
  case NodeKind::Block:
  case NodeKind::Swap:
  case NodeKind::Loop:
  case NodeKind::Stmt:
  case NodeKind::Fn:
    return true;
  }
}
inline bool is_expr(NodeKind kind)
{
  switch(kind)
  {
  default:
    return true;

  case NodeKind::Let:
  case NodeKind::DoYieldUndo:
  case NodeKind::If:
  case NodeKind::Block:
  case NodeKind::Swap:
  case NodeKind::Loop:
  case NodeKind::Stmt:
  case NodeKind::Fn:
    return false;
  }
}


#include <interpret.hpp>
#include <ast.hpp>

#include <algorithm>
#include <iostream>
#include <cassert>
#include <map>

struct Interpreter
{
  Interpreter(std::ostream& os)
    : os(os)
    , stack()
  { stack.reserve(1024 * 1024); }

  void run(const Node* n)
  { (*this)(n); }

  Node::Ptr invert(const Node* n)
  {
    switch(n->kind)
    {
    case NodeKind::Cmp:
    case NodeKind::Num:
    case NodeKind::Swap:
    case NodeKind::Var:

    case NodeKind::Call:  // <- TODO! We need an UNCALL
    {
      auto k = n->kind;
      auto d = n->data;

      std::vector<Node::Ptr> l;
      for(auto& x : n->lhs)
        l.emplace_back(invert(x.get()));
      return make_node(std::move(k), std::move(d), std::move(l));
    }
    case NodeKind::Loop:
    case NodeKind::Block:
    {
      auto k = n->kind;
      std::vector<Node::Ptr> stmts;
      for(auto& x : n->lhs)
        stmts.emplace(stmts.begin(), invert(x.get()));
      return make_node(std::move(k), std::move(stmts));
    }
    case NodeKind::DoYieldUndo:
    {
      assert(false && "Nested do-yield-undo not allowed.");
      return nullptr;
    }
    case NodeKind::Stmt:
    {
      auto ex = invert(n->lhs[0].get());
      return make_node(NodeKind::Stmt, std::move(ex));
    }

    case NodeKind::Unlet:
    case NodeKind::Let:
    {
      auto k = (n->kind == NodeKind::Let ? NodeKind::Unlet : NodeKind::Let);

      return make_node(std::move(k), invert(n->lhs[0].get()), invert(n->lhs[1].get()));
    }

    case NodeKind::If:
    {
      std::vector<Node::Ptr> stmts;
      stmts.emplace_back(invert(n->lhs[3].get())); // <- cond2
      stmts.emplace_back(invert(n->lhs[1].get()));
      stmts.emplace_back(invert(n->lhs[2].get()));
      stmts.emplace_back(invert(n->lhs[0].get())); // <- cond1

      return make_node(NodeKind::If, std::move(stmts));
    }

    case NodeKind::OpEq:
    {
      BinOpTypes bin = BinOpTypes::Add;

      switch(std::get<BinOpTypes>(n->data))
      {
      case BinOpTypes::Add: bin = BinOpTypes::Sub; break;
      case BinOpTypes::Sub: bin = BinOpTypes::Add; break;
      case BinOpTypes::Mul: bin = BinOpTypes::Div; break;
      case BinOpTypes::Div: bin = BinOpTypes::Mul; break;
      }

      return make_node(NodeKind::OpEq, std::move(bin), invert(n->lhs[0].get()), invert(n->lhs[1].get()));
    }
    }
    return nullptr;
  }

  void operator()(const Node* n)
  {
    switch(n->kind)
    {
    case NodeKind::Num: 
    {
      stack.emplace_back(std::get<std::size_t>(n->data));
      return;
    }
    case NodeKind::Var:
    {
      auto name = std::get<Object>(n->data).name;
      auto pos = vars.find(name);
      assert(pos != vars.end() && "Unbound variable!");

      stack.emplace_back(pos->second);
      return;
    }
    case NodeKind::OpEq:
    {
      auto id = std::get<Object>(n->lhs[0]->data).name;
      auto& var = vars[id];

      run(n->lhs[1].get());
      auto rhs = stack.back(); stack.pop_back();

      switch(std::get<BinOpTypes>(n->data))
      {
      case BinOpTypes::Add: var += rhs; break;
      case BinOpTypes::Sub: var -= rhs; break;
      case BinOpTypes::Mul: var *= rhs; break;
      case BinOpTypes::Div: var /= rhs; break;
      }
      return;
    }
    case NodeKind::Cmp:
    {
      run(n->lhs[1].get());
      run(n->lhs[0].get());

      auto a = stack.back(); stack.pop_back();
      auto b = stack.back(); stack.pop_back();

      switch(std::get<CmpTypes>(n->data))
      {
      case CmpTypes::Equal:        stack.emplace_back(a == b); break;
      case CmpTypes::InEqual:      stack.emplace_back(a != b); break;
      case CmpTypes::Less:         stack.emplace_back(a < b); break;
      case CmpTypes::LessEqual:    stack.emplace_back(a <= b); break;
      case CmpTypes::GreaterEqual: stack.emplace_back(a >= b); break;
      case CmpTypes::Greater:      stack.emplace_back(a > b); break;
      }
      return;
    }
    case NodeKind::Let:
    {
      auto obj = std::get<Object>(n->lhs[0]->data);
      auto it = vars.find(obj.name);

      assert(it == vars.end());

      run(n->lhs[1].get());
      auto val = stack.back(); stack.pop_back();
      vars.emplace(obj.name, val);
      return;
    }
    case NodeKind::Unlet:
    {
      auto obj = std::get<Object>(n->lhs[0]->data);
      auto it = vars.find(obj.name);

      assert(it != vars.end());

      run(n->lhs[1].get());
      auto val = stack.back(); stack.pop_back();

      assert(it->second == val);
      vars.erase(it);
      return;
    }
    case NodeKind::Block:
    {
      for(auto& v : n->lhs)
        run(v.get());
      return;
    }
    case NodeKind::Swap:
    {
      auto lhs = std::get<Object>(n->lhs[0]->data).name;
      auto rhs = std::get<Object>(n->lhs[1]->data).name;

      std::swap(vars[lhs], vars[rhs]);
      return;
    }
    case NodeKind::Stmt:
    {
      run(n->lhs[0].get());
      return;
    }
    case NodeKind::DoYieldUndo:
    {
      // run the do block
      run(n->lhs[0].get());
      // run the yield
      run(n->lhs[1].get());

      auto inv = invert(n->lhs[0].get());
      run(inv.get());
      return;
    }
    case NodeKind::If:
    {
      run(n->lhs[0].get());
      auto cond = stack.back(); stack.pop_back();

      if(cond != 0)
        run(n->lhs[1].get());
      else
        run(n->lhs[2].get());
      return;
    }
    case NodeKind::Loop:
    {
      run(n->lhs[0].get());
      auto cond1 = stack.back(); stack.pop_back();

      if(cond1 != 0)
      {
        std::size_t cond2 = 0;
        do
        {
          // eval statement
          run(n->lhs[1].get());

          // eval condition
          run(n->lhs[2].get());
          cond2 = stack.back(); stack.pop_back();
        } while(cond2 == 0);
      }
      return;
    }
    case NodeKind::Call:
    {
      auto str = std::get<Object>(n->lhs[0]->data).name;
      if(str == "print")
      {
        run(n->lhs[1].get());
        auto v = stack.back(); stack.pop_back();

        std::cout << v << "\n";
      }
      else if(str == "read")
      {
        assert(n->lhs[1]->kind == NodeKind::Var);
        auto str = std::get<Object>(n->lhs[1]->data).name;

        std::cin >> vars[str];
      }
      return;
    }
    }
  }

  void call(Fn* foo, std::vector<std::size_t>&& args)
  {
    assert(foo && foo->params.size() == args.size());

    // TODO: Consider export/import
    //TODO: only let/unlet non-mut args
    // Let the arguments
    for(std::size_t i = 0; i < foo->params.size(); ++i)
    {
      auto& p = foo->params[i];

      auto it = vars.find(p.name);
      assert(it == vars.end());

      vars.emplace(p.name, args[i]);
    }
    // Run function body
    run(foo->body.get());
    // Unlet the arguments
    for(std::size_t i = 0; i < foo->params.size(); ++i)
    {
      auto& p = foo->params[i];

      auto it = vars.find(p.name);
      assert(it != vars.end());

      assert(it->second == args[i]);
      vars.erase(it);
    }
  }

  std::ostream& os;
  std::vector<std::size_t> stack;
  std::map<std::string, std::size_t> vars;
};

void interpret(std::ostream& os, const std::vector<Fn::Ptr>& nods)
{
  Interpreter interp(os);

  Fn* main = nullptr;
  for(auto& x : nods)
    if(x->name == "main")
      main = x.get();
  assert(main && "No entry point");
  // TODO: check main for correct return type

  // TODO: check for argc/argv with correct types
  assert(main->params.size() == 1 && "Entry point must have exactly one argument.");

  interp.call(main, { 0 });

  assert(interp.vars.empty() && "all lets must be cleaned up with an unlet");
}


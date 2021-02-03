#include <interpret.hpp>
#include <ast.hpp>

#include <algorithm>
#include <iostream>
#include <cassert>
#include <map>

struct Interpreter
{
  using DataType = std::variant<std::monostate, std::size_t>;

  Interpreter(std::ostream& os)
    : os(os)
    , stack()
    , fns()
  { stack.reserve(1024 * 1024); }

  void register_fn(const Fn* fn)
  {
    fns[fn->name] = fn;
  }

  void run(const Node* n)
  { (*this)(n); }

  Node::Ptr invert(const Node* n)
  {
    switch(n->kind)
    {
    case NodeKind::Num:
    case NodeKind::Unit:
    case NodeKind::Var:

    case NodeKind::Cmp:
    case NodeKind::Swap:

    case NodeKind::Uncall:
    case NodeKind::Call:  // <- TODO! We need an UNCALL
    {
      auto k = n->kind == NodeKind::Call   ? NodeKind::Uncall 
             :(n->kind == NodeKind::Uncall ? NodeKind::Call
                                           : n->kind);
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
    case NodeKind::Unit:
    {
      stack.emplace_back(DataType { std::monostate {} });
      return;
    }
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
      auto var = std::get<std::size_t>(vars[id]);

      run(n->lhs[1].get());
      auto rhs_v = stack.back(); stack.pop_back();
      auto rhs = std::get<std::size_t>(rhs_v);

      switch(std::get<BinOpTypes>(n->data))
      {
      case BinOpTypes::Add: var += rhs; break;
      case BinOpTypes::Sub: var -= rhs; break;
      case BinOpTypes::Mul: var *= rhs; break;
      case BinOpTypes::Div: var /= rhs; break;
      }
      // put it back into the variant
      vars[id] = var;
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
      auto cond_v = stack.back(); stack.pop_back();
      auto cond = std::get<std::size_t>(cond_v);

      if(cond != 0)
        run(n->lhs[1].get());
      else
        run(n->lhs[2].get());
      return;
    }
    case NodeKind::Loop:
    {
      run(n->lhs[0].get());
      auto cond1_v = stack.back(); stack.pop_back();
      auto cond1 = std::get<std::size_t>(cond1_v);

      if(cond1 != 0)
      {
        DataType cond2 = std::size_t(0);
        do
        {
          // eval statement
          run(n->lhs[1].get());

          // eval condition
          run(n->lhs[2].get());
          cond2 = stack.back(); stack.pop_back();
        } while(std::get<std::size_t>(cond2) == 0);
      }
      return;
    }
    case NodeKind::Call:
    {
      auto store = std::get<Object>(n->lhs[0]->data).name;
      auto fn_name = std::get<Object>(n->lhs[1]->data).name;

      auto it = fns.find(fn_name);
      if(it != fns.end())
      {
        auto fn = it->second;
        assert(fn->params.size() == n->lhs.size() - 2 && "function call arguments must match");

        // set parameter values
        std::vector<std::size_t> args;
        args.reserve(fn->params.size());
        for(std::size_t i = 0; i < fn->params.size(); ++i)
        {
          run(n->lhs[i + 2].get());

          args.emplace_back(std::get<std::size_t>(stack.back())); stack.pop_back();
        }
        call(fn, std::move(args));

        // TODO: Fix this! We may want to return an int or anything like that as well
        vars[store] = std::monostate{};
        return;
      }

      if(fn_name == "print")
      {
        run(n->lhs[2].get());
        auto v_v = stack.back(); stack.pop_back();

        std::cout << std::get<std::size_t>(v_v) << "\n";

        vars[store] = std::monostate {};
      }
      else if(fn_name == "read")
      {
        std::size_t tmp = 0;
        std::cin >> tmp;

        vars[store] = tmp;
      }
      else
        assert(false);
      return;
    }
    }
  }

  void call(const Fn* foo, std::vector<std::size_t>&& args)
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

      assert(std::get<std::size_t>(it->second) == args[i]);
      vars.erase(it);
    }
  }

  std::ostream& os;
  std::vector<DataType> stack;
  std::map<std::string, DataType> vars;
  std::map<std::string, const Fn*> fns;
};

void interpret(std::ostream& os, const std::vector<Fn::Ptr>& nods)
{
  Interpreter interp(os);

  for(auto& x : nods)
    interp.register_fn(x.get());

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


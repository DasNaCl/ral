#include <type.hpp>
#include <ast.hpp>
#include <cassert>

bool is_int(Type& typ)
{ return typ.kind == TypeKind::Int; }

Type::Ptr mk_pointer(Type::Ptr typ)
{
  std::vector<Type::Ptr> v;
  v.emplace_back(std::move(typ));
  return std::make_shared<Type>(TypeKind::Ptr, std::move(v));
}

Type::Ptr str2typ(const std::string& str)
{
  // TODO: Add more
  if(str == "()")
    return void_type();
  else if(str == "i32" || str == "int")
    return int_type();
  return nullptr;
}

Type::Ptr void_type()
{
  return std::make_shared<Type>(TypeKind::Void, std::vector<Type::Ptr>{});
}

Type::Ptr int_type()
{
  return std::make_shared<Type>(TypeKind::Int, std::vector<Type::Ptr>{});
}

Type::Ptr fn_type(std::vector<Type::Ptr>&& params, Type::Ptr&& ret)
{
  std::vector<Type::Ptr> pars = params;
  pars.emplace(pars.begin(), ret);
  return std::make_shared<Type>(TypeKind::Fn, std::move(pars));
}

Type::Ptr Type::Fn::ret(Type::Ptr fn)
{
  assert(fn && fn->kind == TypeKind::Fn);
  return fn->args.front();
}

std::vector<Type::Ptr> Type::Fn::params(Type::Ptr fn)
{
  assert(fn && fn->kind == TypeKind::Fn);

  std::vector<Type::Ptr> params = fn->args;
  params.erase(params.begin());
  return params;
}

void infer(Node* n)
{
  if(!n || n->typ)
    return;

  // infer children
  for(auto& x : n->lhs)
    infer(x.get());

  switch(n->kind)
  {
  case NodeKind::Let:
    n->typ = n->lhs.front()->typ;
    return;

  case NodeKind::Cmp:
  case NodeKind::Num:
  case NodeKind::Var:
    n->typ = int_type();
    return;
  }
}

void infer(Fn* f)
{
  // TODO: move this into a class and collect param typs in an environment
  infer(f->body.get());
}



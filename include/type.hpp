#pragma once

#include <memory>
#include <vector>
#include <set>

enum class TypeKind {
  Int,
  Ptr,
  Fn,
};

struct Type {
  using Ptr = std::shared_ptr<Type>;

  struct Fn
  {
    static Type::Ptr ret(Type::Ptr fn);
    static std::vector<Type::Ptr> params(Type::Ptr fn);
  };

  Type(TypeKind kind, std::vector<Type::Ptr>&& args)
    : kind(kind)
    , args(std::move(args))
  {  }

  TypeKind kind;
  std::vector<Type::Ptr> args;
};


Type::Ptr str2typ(const std::string& str);

Type::Ptr fn_type(std::vector<Type::Ptr>&& params, Type::Ptr&& ret);
Type::Ptr int_type();

bool is_int(Type& typ);

struct Fn;
void infer(Fn* n);



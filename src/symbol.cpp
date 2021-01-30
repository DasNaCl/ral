#include <symbol.hpp>

#include <mutex>

struct nonesuch {
  ~nonesuch() = delete;
  nonesuch(nonesuch const&) = delete;
  void operator=(nonesuch const&) = delete;
};

namespace detail
{
  template <class Default, class AlwaysVoid, template<class...> class Op, class... Args>
  struct detector
  {
    using value_t = std::false_type;
    using type = Default;
  };

  template <class Default, template<class...> class Op, class... Args>
  struct detector<Default, std::void_t<Op<Args...>>, Op, Args...>
  {
    using value_t = std::true_type;
    using type = Op<Args...>;
  };

}
template <template<class...> class Op, class... Args>
using is_detected = typename detail::detector<nonesuch, void, Op, Args...>::value_t;

template<typename T>
using has_method_empty_trait = decltype(std::declval<T>().empty());

template<typename T>
constexpr bool has_method_empty()
{ return is_detected<has_method_empty_trait, T>::value; }

template<typename T, typename H = std::uint_fast32_t>
constexpr H hash_string(T str)
{
  if constexpr(has_method_empty<T>())
  {
    // probably a std::string like thing
    if(str.empty()) return 0;
  }
  else
  {
    // probably a c style string thing    (could also use a trait and throw error if this interface is also not provided)
    if(!str) return 0;
  }
  H hash = str[0];
  for(auto* p = &str[0]; p && *p != '\0'; p++)
    hash ^= (hash * 31) + (*p);
  return hash;
}

static std::mutex symbol_table_mutex;

tsl::robin_map<std::uint_fast64_t, std::string> symbol::symbols = {};

symbol::symbol(const std::string& str)
  : hash_(hash_string(str))
{ lookup_or_emplace(hash(), str.c_str()); }

symbol::symbol(const char* str)
  : hash_(hash_string(str))
{ lookup_or_emplace(hash(), str); }

symbol::symbol(const symbol& s)
  : hash_(s.hash())
{  }

symbol::symbol(symbol&& s)
  : hash_(s.hash())
{  }

symbol::~symbol() noexcept
{  }

symbol& symbol::operator=(const std::string& str)
{
  hash_ = hash_string(str);
  lookup_or_emplace(hash(), str.c_str());

  return *this;
}

symbol& symbol::operator=(const char* str)
{
  hash_ = hash_string(str);
  lookup_or_emplace(hash(), str);

  return *this;
}

symbol& symbol::operator=(const symbol& s)
{
  hash_ = s.hash();

  return *this;
}

symbol& symbol::operator=(symbol&& s)
{
  hash_ = s.hash();

  return *this;
}

std::ostream& operator<<(std::ostream& os, const symbol& symb)
{
  os << symbol::symbols[symb.hash()];
  return os;
}

std::string& symbol::lookup_or_emplace(std::uint_fast64_t hash, const char* str)
{
  auto it = symbols.find(hash);
  if(it != symbols.end())
    return symbols[hash];

  std::lock_guard<std::mutex> lock(symbol_table_mutex);
  symbols[hash] = str;
  return symbols[hash];
}

std::ostream& operator<<(std::ostream& os, const std::vector<symbol>& symbs)
{
  for(auto& s : symbs)
    os << s;
  return os;
}

std::uint_fast64_t symbol::hash() const
{ return hash_; }

const std::string& symbol::str() const
{ return symbols[hash()]; }

bool operator==(const symbol& a, const symbol& b)
{ return a.hash() == b.hash(); }

bool operator!=(const symbol& a, const symbol& b)
{ return a.hash() != b.hash(); }


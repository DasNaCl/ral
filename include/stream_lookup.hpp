#pragma once

#include <unordered_map>
#include <string_view>
#include <fstream>
#include <memory>
#include <vector>
#include <iosfwd>

struct stream_lookup_t
{
  stream_lookup_t();
  ~stream_lookup_t();

  std::istream& operator[](std::string_view str);
  void drop(std::string_view str);

private:
  void process_stdin();
private:
  std::vector<std::unique_ptr<std::ifstream>> files;

  std::unordered_map<std::string_view, std::vector<std::unique_ptr<std::ifstream>>::iterator> map;

  std::string stdin_module;

  bool stdin_processed { false };
};

inline stream_lookup_t stream_lookup;


#pragma once

#include <stream_lookup.hpp>
#include <source_range.hpp>
#include <symbol.hpp>
#include <token.hpp>
#include <ast.hpp>

#include <unordered_map>
#include <cassert>
#include <cstdio>
#include <memory>
#include <iosfwd>
#include <vector>
#include <string>


std::vector<Fn::Ptr> read(std::string_view module);
std::vector<Fn::Ptr> read_text(const std::string& str);


#pragma once

#include <iosfwd>
#include <memory>
#include <vector>

struct Fn;

void interpret(std::ostream& os, const std::vector<std::unique_ptr<Fn>>& n);


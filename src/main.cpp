#include <parser.hpp>
#include <interpret.hpp>
#include <type.hpp>

#include <iostream>
#include <fstream>
#include <sstream>

int main(int argc, char** argv)
{
  if (argc != 1)
  {
    std::cerr << "insufficient arguments\n";
    return 1;
  }

  std::string input;
  for(std::string str; std::getline(std::cin, str); input += str + "\n")
    ;
  auto v = read_text(input);

  for(auto& x : v)
    infer(x.get());
  interpret(std::cout, v);

  return 0;
}



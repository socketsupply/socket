#include <stdio.h>
#include <iostream>
#include "common.hh"

int main () {
  char* name = "1.1.1.1";
  char* port = "8080";
  auto s1 = Opkit::encodeURIComponent(Opkit::format(R"({
    "data": {
      "status": "READY",
      "ip": "$C",
      "port": "$C"
    }
  })", name, port));

  std::string a = "Allice";
  std::string b = "Bob";

  auto s2 = Opkit::format(
    "Hello, $S."
    "There are $i copies of $c for $S.",
    a, 100, 'x', b);

  std::cout << s1 << std::endl;
  std::cout << s2 << std::endl;

  std::string x("8080");
  char* p = (char* const) x.c_str();
  std::cout << p << std::endl;
}

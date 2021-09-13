#include "vcd_data.h"
#include <iostream>

namespace vcdparse {
std::ostream &operator<<(std::ostream &os, LogicValue val) {
  if (val == LogicValue::ONE) {
    os << "1";
  } else if (val == LogicValue::ZERO) {
    os << "0";
  } else if (val == LogicValue::UNKOWN) {
    os << "x";
  } else if (val == LogicValue::HIGHZ) {
    os << "z";
  } else {
    assert(false);
  }
  return os;
}

std::ostream &operator<<(std::ostream &os, Var::Type type) {
  if (type == Var::Type::WIRE) {
    os << "wire";
  } else if (type == Var::Type::REG) {
    os << "reg";
  } else if (type == Var::Type::PARAMETER) {
    os << "parameter";
  } else {
    assert(false);
  }
  return os;
}
} // namespace vcdparse

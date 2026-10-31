// WebAssembly bindings for mini-gmp-plus-plus (Emscripten embind).
//
// Exposes the header-only C++ wrappers MiniMPZ (arbitrary-precision integers)
// and MiniMPF (mantissa * 2^exponent floating point) to JavaScript/TypeScript
// as an ES6 module. Big values must be constructed from strings - JavaScript
// numbers cannot hold them:
//
//   const z = new MiniGmp.MiniMPZ("123456789012345678901234567890", 10);
//   z.mul(z).toString(10);

#include <emscripten/bind.h>

#include <string>

#include "MiniMPF.hpp"
#include "MiniMPZ.hpp"

namespace em = emscripten;

namespace
{
  // embind's class_::function only accepts member function pointers, and
  // MiniMPF's arithmetic operators are free (friend) functions, so they are
  // surfaced through this thin derived wrapper. MiniMPZ's operators are
  // members and are bound directly with select_overload.
  struct MiniMPFWasm : MiniMPF
  {
    MiniMPFWasm() = default;
    MiniMPFWasm(double val) : MiniMPF(val) {}
    MiniMPFWasm(const MiniMPZ& mantissa, int exponent) : MiniMPF(mantissa, exponent) {}
    MiniMPFWasm(const MiniMPF& other) : MiniMPF(other) {}

    MiniMPFWasm add(const MiniMPFWasm& other) const { return MiniMPFWasm(*this + other); }
    MiniMPFWasm sub(const MiniMPFWasm& other) const { return MiniMPFWasm(*this - other); }
    MiniMPFWasm mul(const MiniMPFWasm& other) const { return MiniMPFWasm(*this * other); }
    MiniMPFWasm neg() const { return MiniMPFWasm(-*this); }
    int cmp(const MiniMPFWasm& other) const { return compare(other); }
    bool equals(const MiniMPFWasm& other) const { return *this == other; }
    int sign() const { return Sign(); }
    bool isZero() const { return IsZero(); }
    bool isPositive() const { return IsPositive(); }
    int exponent() const { return Exponent(); }
    MiniMPZ mantissa() const { return Mantisse(); }
    std::string visu() const { return Visu(); }
    double estimate() const { return Estimate(); }
  };
}

EMSCRIPTEN_BINDINGS(mini_gmp_plus_plus)
{
  em::class_<MiniMPZ>("MiniMPZ")
      .constructor<>()
      .constructor<double>()
      .constructor<const std::string&, int>()
      .function("add", em::select_overload<MiniMPZ(const MiniMPZ&) const>(&MiniMPZ::operator+))
      .function("sub", em::select_overload<MiniMPZ(const MiniMPZ&) const>(&MiniMPZ::operator-))
      .function("mul", em::select_overload<MiniMPZ(const MiniMPZ&) const>(&MiniMPZ::operator*))
      .function("div", em::select_overload<MiniMPZ(const MiniMPZ&) const>(&MiniMPZ::operator/))
      .function("mod", em::select_overload<MiniMPZ(const MiniMPZ&) const>(&MiniMPZ::operator%))
      .function("neg", em::select_overload<MiniMPZ() const>(&MiniMPZ::operator-))
      .function("abs", em::select_overload<MiniMPZ() const>(&MiniMPZ::abs))
      .function("pow", em::select_overload<MiniMPZ(unsigned long) const>(&MiniMPZ::pow))
      .function("sqrt", em::select_overload<MiniMPZ() const>(&MiniMPZ::sqrt))
      .function("lt", em::select_overload<bool(const MiniMPZ&) const>(&MiniMPZ::operator<))
      .function("le", em::select_overload<bool(const MiniMPZ&) const>(&MiniMPZ::operator<=))
      .function("gt", em::select_overload<bool(const MiniMPZ&) const>(&MiniMPZ::operator>))
      .function("ge", em::select_overload<bool(const MiniMPZ&) const>(&MiniMPZ::operator>=))
      .function("equals", em::select_overload<bool(const MiniMPZ&) const>(&MiniMPZ::operator==))
      .function("sign", em::select_overload<int() const>(&MiniMPZ::sign))
      .function("isEven", em::select_overload<bool() const>(&MiniMPZ::is_even))
      .function("isOdd", em::select_overload<bool() const>(&MiniMPZ::is_odd))
      .function("toString", em::select_overload<std::string(int) const>(&MiniMPZ::to_string))
      .function("toDouble", em::select_overload<double() const>(&MiniMPZ::to_double))
      .function("toLong", em::select_overload<long() const>(&MiniMPZ::to_long));

  em::class_<MiniMPFWasm>("MiniMPF")
      .constructor<>()
      .constructor<double>()
      .constructor<const MiniMPZ&, int>()
      .function("add", &MiniMPFWasm::add)
      .function("sub", &MiniMPFWasm::sub)
      .function("mul", &MiniMPFWasm::mul)
      .function("neg", &MiniMPFWasm::neg)
      .function("cmp", &MiniMPFWasm::cmp)
      .function("equals", &MiniMPFWasm::equals)
      .function("sign", &MiniMPFWasm::sign)
      .function("isZero", &MiniMPFWasm::isZero)
      .function("isPositive", &MiniMPFWasm::isPositive)
      .function("exponent", &MiniMPFWasm::exponent)
      .function("mantissa", &MiniMPFWasm::mantissa)
      .function("visu", &MiniMPFWasm::visu)
      .function("estimate", &MiniMPFWasm::estimate);
}

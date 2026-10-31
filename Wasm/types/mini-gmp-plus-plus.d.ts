/**
 * TypeScript declarations for the mini-gmp-plus-plus WebAssembly module
 * (bin/mini-gmp-plus-plus.js + bin/mini-gmp-plus-plus.wasm).
 *
 * The build is an ES6 module (MODULARIZE + EXPORT_ES6): import the factory,
 * await it once, then create values.
 *
 *   import createMiniGmpPlusPlus from "./mini-gmp-plus-plus.js";
 *   const MiniGmp = await createMiniGmpPlusPlus();
 *   const z = new MiniGmp.MiniMPZ("123456789012345678901234567890", 10);
 *   console.log(z.mul(z).toString(10));
 *
 * Big values must be constructed from strings (base 10 or 16); JavaScript
 * numbers cannot represent them. toLong() truncates like a 32-bit long on
 * wasm32 and is only useful for values that fit in an int32.
 */

export interface MiniMPZ {
  /** Zero. */
  new (): MiniMPZ;
  /** From a double (exact only up to 2^53). */
  new (value: number): MiniMPZ;
  /** From a decimal/hex string, e.g. new MiniMPZ("ff", 16). */
  new (str: string, base: number): MiniMPZ;

  add(other: MiniMPZ): MiniMPZ;
  sub(other: MiniMPZ): MiniMPZ;
  mul(other: MiniMPZ): MiniMPZ;
  /** Truncating division (mpz_tdiv_q). */
  div(other: MiniMPZ): MiniMPZ;
  mod(other: MiniMPZ): MiniMPZ;
  neg(): MiniMPZ;
  abs(): MiniMPZ;
  /** this^exp, exponent is a wasm32 unsigned long (32-bit). */
  pow(exp: number): MiniMPZ;
  sqrt(): MiniMPZ;
  lt(other: MiniMPZ): boolean;
  le(other: MiniMPZ): boolean;
  gt(other: MiniMPZ): boolean;
  ge(other: MiniMPZ): boolean;
  equals(other: MiniMPZ): boolean;
  /** -1, 0 or 1. */
  sign(): number;
  isEven(): boolean;
  isOdd(): boolean;
  /** Decimal (base 10) or hex (base 16) representation. */
  toString(base: number): string;
  toDouble(): number;
  /** Truncates to a 32-bit long on wasm32; prefer toString for big values. */
  toLong(): number;
}

export interface MiniMPF {
  /** Zero. */
  new (): MiniMPF;
  /** From a double. */
  new (value: number): MiniMPF;
  /** value = mantissa * 2^exponent. */
  new (mantissa: MiniMPZ, exponent: number): MiniMPF;

  add(other: MiniMPF): MiniMPF;
  sub(other: MiniMPF): MiniMPF;
  mul(other: MiniMPF): MiniMPF;
  neg(): MiniMPF;
  /** -1, 0 or 1. */
  cmp(other: MiniMPF): number;
  equals(other: MiniMPF): boolean;
  sign(): number;
  isZero(): boolean;
  isPositive(): boolean;
  exponent(): number;
  mantissa(): MiniMPZ;
  /** Debug form: "<mantissa> * 2^<exponent>". */
  visu(): string;
  /** Approximate value as a double (mantissa.toDouble() * 2^exponent). */
  estimate(): number;
}

export interface MiniGmpPlusPlusModule {
  MiniMPZ: MiniMPZ;
  MiniMPF: MiniMPF;
}

declare function createMiniGmpPlusPlus(): Promise<MiniGmpPlusPlusModule>;
export default createMiniGmpPlusPlus;

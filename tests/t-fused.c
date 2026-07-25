/* Regression tests for the mini-gmp-plus fused small-linear-algebra
   primitives: mpz_mul_add_mul, mpz_mul_sub_mul and mpz_dot_product.

Copyright 2026 Free Software Foundation, Inc.

This file is part of the GNU MP Library test suite.

The GNU MP Library test suite is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 3 of the License,
or (at your option) any later version.

The GNU MP Library test suite is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
Public License for more details.

You should have received a copy of the GNU General Public License along with
the GNU MP Library test suite.  If not, see https://www.gnu.org/licenses/.  */

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "testutils.h"

/* Spans the 32-limb (2048-bit) stack-scratch cap of the fused primitives so
   that both the fast path and the generic path are exercised. */
#define MAXBITS 1400
#define COUNT 3000
#define DOT_TERMS 9

static void
check (const char *name, const mpz_t got, const mpz_t ref,
       const mpz_t a, const mpz_t b, const mpz_t c, const mpz_t d)
{
  if (mpz_cmp (got, ref) != 0)
    {
      fprintf (stderr, "%s failed:\n", name);
      dump ("a", a);
      dump ("b", b);
      dump ("c", c);
      dump ("d", d);
      dump ("r", got);
      dump ("ref", ref);
      abort ();
    }
}

/* r = a*b + c*d, or a*b - c*d, computed the long way. */
static void
reference (mpz_t ref, const mpz_t a, const mpz_t b,
	   const mpz_t c, const mpz_t d, int sub)
{
  mpz_t t, u;
  mpz_init (t);
  mpz_init (u);
  mpz_mul (t, a, b);
  mpz_mul (u, c, d);
  if (sub)
    mpz_sub (ref, t, u);
  else
    mpz_add (ref, t, u);
  mpz_clear (t);
  mpz_clear (u);
}

static void
test_mul_aors_mul (void)
{
  mpz_t a, b, c, d, res, ref;
  unsigned i;

  mpz_init (a); mpz_init (b); mpz_init (c); mpz_init (d);
  mpz_init (res); mpz_init (ref);

  for (i = 0; i < COUNT; i++)
    {
      int sub = i & 1;

      mini_urandomb (a, 1 + i % MAXBITS);
      mini_urandomb (b, 1 + (i * 7) % MAXBITS);
      mini_urandomb (c, 1 + (i * 13) % MAXBITS);
      mini_urandomb (d, 1 + (i * 29) % MAXBITS);
      if (i & 2) mpz_neg (a, a);
      if (i & 4) mpz_neg (b, b);
      if (i & 8) mpz_neg (c, c);
      if (i & 16) mpz_neg (d, d);

      /* Occasionally force one or both products to zero. */
      if (i % 61 == 0) mpz_set_ui (a, 0);
      if (i % 79 == 0) mpz_set_ui (d, 0);

      reference (ref, a, b, c, d, sub);
      if (sub)
	mpz_mul_sub_mul (res, a, b, c, d);
      else
	mpz_mul_add_mul (res, a, b, c, d);
      check (sub ? "mpz_mul_sub_mul" : "mpz_mul_add_mul", res, ref, a, b, c, d);

      /* r aliasing each operand in turn must give the same answer. */
      {
	int which;
	for (which = 0; which < 4; which++)
	  {
	    mpz_t A, B, C, D;
	    mpz_ptr target;
	    mpz_init_set (A, a); mpz_init_set (B, b);
	    mpz_init_set (C, c); mpz_init_set (D, d);
	    target = (which == 0) ? A : (which == 1) ? B : (which == 2) ? C : D;
	    if (sub)
	      mpz_mul_sub_mul (target, A, B, C, D);
	    else
	      mpz_mul_add_mul (target, A, B, C, D);
	    check ("fused aliasing", target, ref, a, b, c, d);
	    mpz_clear (A); mpz_clear (B); mpz_clear (C); mpz_clear (D);
	  }
      }
    }

  /* Exact cancellation: a*b - a*b must be exactly zero. */
  for (i = 0; i < 200; i++)
    {
      mini_urandomb (a, 1 + i % MAXBITS);
      mini_urandomb (b, 1 + (i * 11) % MAXBITS);
      if (i & 1) mpz_neg (a, a);
      mpz_mul_sub_mul (res, a, b, a, b);
      if (mpz_sgn (res) != 0)
	{
	  fprintf (stderr, "mpz_mul_sub_mul cancellation failed:\n");
	  dump ("a", a);
	  dump ("b", b);
	  dump ("r", res);
	  abort ();
	}
      /* Reversed factors denote the same product. */
      mpz_mul_sub_mul (res, a, b, b, a);
      if (mpz_sgn (res) != 0)
	{
	  fprintf (stderr, "mpz_mul_sub_mul cancellation (swapped) failed:\n");
	  dump ("a", a);
	  dump ("b", b);
	  dump ("r", res);
	  abort ();
	}
    }

  mpz_clear (a); mpz_clear (b); mpz_clear (c); mpz_clear (d);
  mpz_clear (res); mpz_clear (ref);
}

static void
test_dot_product (void)
{
  mpz_t u[DOT_TERMS], v[DOT_TERMS];
  mpz_srcptr up[DOT_TERMS], vp[DOT_TERMS];
  mpz_t res, ref;
  unsigned i, k;

  for (k = 0; k < DOT_TERMS; k++)
    {
      mpz_init (u[k]);
      mpz_init (v[k]);
      up[k] = u[k];
      vp[k] = v[k];
    }
  mpz_init (res);
  mpz_init (ref);

  for (i = 0; i < COUNT; i++)
    {
      size_t n = 1 + i % DOT_TERMS;

      for (k = 0; k < n; k++)
	{
	  mini_urandomb (u[k], 1 + (i + 3 * k) % MAXBITS);
	  mini_urandomb (v[k], 1 + (i + 5 * k) % MAXBITS);
	  if ((i + k) & 1) mpz_neg (u[k], u[k]);
	  if ((i + k) & 2) mpz_neg (v[k], v[k]);
	  /* Sprinkle in zero terms. */
	  if ((i + k) % 37 == 0) mpz_set_ui (u[k], 0);
	}

      mpz_set_ui (ref, 0);
      for (k = 0; k < n; k++)
	mpz_addmul (ref, u[k], v[k]);

      mpz_dot_product (res, n, up, vp);
      if (mpz_cmp (res, ref) != 0)
	{
	  fprintf (stderr, "mpz_dot_product failed (n = %lu):\n",
		   (unsigned long) n);
	  for (k = 0; k < n; k++)
	    {
	      dump ("u", u[k]);
	      dump ("v", v[k]);
	    }
	  dump ("r", res);
	  dump ("ref", ref);
	  abort ();
	}

      /* Aliasing: r is one of the inputs. */
      mpz_set (res, u[0]);
      mpz_dot_product (u[0], n, up, vp);
      if (mpz_cmp (u[0], ref) != 0)
	{
	  fprintf (stderr, "mpz_dot_product aliasing failed (n = %lu):\n",
		   (unsigned long) n);
	  dump ("r", u[0]);
	  dump ("ref", ref);
	  abort ();
	}
      mpz_set (u[0], res);
    }

  /* n == 0 is an empty sum. */
  mpz_set_ui (res, 12345);
  mpz_dot_product (res, 0, up, vp);
  if (mpz_sgn (res) != 0)
    {
      fprintf (stderr, "mpz_dot_product of zero terms is not zero:\n");
      dump ("r", res);
      abort ();
    }

  /* Fully cancelling sum: u.v with v = -u must vanish. */
  for (k = 0; k < DOT_TERMS; k++)
    {
      mini_urandomb (u[k], 1 + 17 * (k + 1));
      mpz_neg (v[k], u[k]);
    }
  mpz_dot_product (res, DOT_TERMS, up, vp);
  mpz_set_ui (ref, 0);
  for (k = 0; k < DOT_TERMS; k++)
    mpz_addmul (ref, u[k], v[k]);
  if (mpz_cmp (res, ref) != 0)
    {
      fprintf (stderr, "mpz_dot_product negated-vector case failed:\n");
      dump ("r", res);
      dump ("ref", ref);
      abort ();
    }

  for (k = 0; k < DOT_TERMS; k++)
    {
      mpz_clear (u[k]);
      mpz_clear (v[k]);
    }
  mpz_clear (res);
  mpz_clear (ref);
}

void
testmain (int argc, char **argv)
{
  test_mul_aors_mul ();
  test_dot_product ();
}

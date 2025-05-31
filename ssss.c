/*
 *  ssss version 0.5                    -  Copyright 2005,2006 B. Poettering
 *  ssss version 0.5.1+ (changes only)  -  Copyright held by respective contributors
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307 USA
 */

/*
 * http://point-at-infinity.org/ssss/
 * https://github.com/MrJoy/ssss
 *
 * This is an implementation of Shamir's Secret Sharing Scheme. See
 * the project's homepage http://point-at-infinity.org/ssss/ for more
 * information on this topic.
 *
 * This code links against the GNU multiprecision library "libgmp".
 * Original author compiled the code successfully with gmp 4.1.4.
 * Jon Frisby compiled the code successfully with gmp 5.0.2, and 6.1.2.
 *
 * You will need a system that has a /dev/urandom entropy source.
 *
 * Compile with -DNOMLOCK to obtain a version without memory locking.
 *
 * If you encounter compile issues, compile with USE_RESTORE_SECRET_WORKAROUND.
 *
 * Report bugs to: ssss AT point-at-infinity.org
 * Also report compilation / usability issues to: jfrisby AT mrjoy.com
 *
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <termios.h>
#include <sys/mman.h>

#include <gmp.h>

#define VERSION "0.5.7"
#define RANDOM_SOURCE "/dev/urandom"
#define MAXDEGREE 1024
#define MAXTOKENLEN 128
#define MAXLINELEN (MAXTOKENLEN + 1 + 10 + 1 + MAXDEGREE / 4 + 10)

/* coefficients of some irreducible polynomials over GF(2) */
static const unsigned char irred_coeff[] = {
  4,3,1,5,3,1,4,3,1,7,3,2,5,4,3,5,3,2,7,4,2,4,3,1,10,9,3,9,4,2,7,6,2,10,9,
  6,4,3,1,5,4,3,4,3,1,7,2,1,5,3,2,7,4,2,6,3,2,5,3,2,15,3,2,11,3,2,9,8,7,7,
  2,1,5,3,2,9,3,1,7,3,1,9,8,3,9,4,2,8,5,3,15,14,10,10,5,2,9,6,2,9,3,2,9,5,
  2,11,10,1,7,3,2,11,2,1,9,7,4,4,3,1,8,3,1,7,4,1,7,2,1,13,11,6,5,3,2,7,3,2,
  8,7,5,12,3,2,13,10,6,5,3,2,5,3,2,9,5,2,9,7,2,13,4,3,4,3,1,11,6,4,18,9,6,
  19,18,13,11,3,2,15,9,6,4,3,1,16,5,2,15,14,6,8,5,2,15,11,2,11,6,2,7,5,3,8,
  3,1,19,16,9,11,9,6,15,7,6,13,4,3,14,13,3,13,6,3,9,5,2,19,13,6,19,10,3,11,
  6,5,9,2,1,14,3,2,13,3,1,7,5,4,11,9,8,11,6,5,23,16,9,19,14,6,23,10,2,8,3,
  2,5,4,3,9,6,4,4,3,2,13,8,6,13,11,1,13,10,3,11,6,5,19,17,4,15,14,7,13,9,6,
  9,7,3,9,7,1,14,3,2,11,8,2,11,6,4,13,5,2,11,5,1,11,4,1,19,10,3,21,10,6,13,
  3,1,15,7,5,19,18,10,7,5,3,12,7,2,7,5,1,14,9,6,10,3,2,15,13,12,12,11,9,16,
  9,7,12,9,3,9,5,2,17,10,6,24,9,3,17,15,13,5,4,3,19,17,8,15,6,3,19,6,1 };

int opt_showversion = 0;
int opt_help = 0;
int opt_quiet = 0;
int opt_QUIET = 0;
int opt_hex = 0;
int opt_diffusion = 1;
int opt_security = 0;
int opt_threshold = -1;
int opt_number = -1;
char *opt_token = NULL;
int opt_recovery = 0;

unsigned int degree = 0;
mpz_t poly;
int cprng;
struct termios echo_orig, echo_off;

enum ssss_errcode {
  ssss_ec_ok = 0,
  ssss_err_input_string_too_long,
  ssss_err_invalid_syntax,
  ssss_err_open_random,
  ssss_err_close_random,
  ssss_err_read_random,
  ssss_err_io_reading_secret,
  ssss_err_invalid_security_level,
  ssss_err_io_reading_shares,
  ssss_err_illegal_share_length,
  ssss_err_shares_different_security_levels,
  ssss_err_invalid_share,
  ssss_err_inconsistent_shares,
  ssss_err_unknown
};

static const char *ssss_errmsg[] = {
  "ok",
  "input string too long",
  "invalid syntax",
  "couldn't open " RANDOM_SOURCE,
  "couldn't close " RANDOM_SOURCE,
  "couldn't read from " RANDOM_SOURCE,
  "I/O error while reading secret",
  "security level invalid (secret too long?)",
  "I/O error while reading shares",
  "share has illegal length",
  "shares have different security levels",
  "invalid share",
  "shares inconsistent. Perhaps a single share was used twice",
  "unknown error"
};

void secure_zero(void *s, size_t n);

#define mpz_lshift(A, B, l) mpz_mul_2exp(A, B, l)
#define mpz_sizeinbits(A) (mpz_cmp_ui(A, 0) ? mpz_sizeinbase(A, 2) : 0)

/* emergency abort and warning functions */

void fatal(const char *msg)
{
  tcsetattr(0, TCSANOW, &echo_orig);
  fprintf(stderr, "%sFATAL: %s.\n", isatty(2) ? "\a" : "", msg);
  exit(1);
}

void fatal_errcode(enum ssss_errcode ec)
{
  fatal(ssss_errmsg[(ec > ssss_ec_ok && ec < ssss_err_unknown) ? ec : ssss_err_unknown]);
}

void warning(char *msg)
{
  if (! opt_QUIET)
    fprintf(stderr, "%sWARNING: %s.\n", isatty(2) ? "\a" : "", msg);
}

/* field arithmetic routines */

int field_size_valid(int deg)
{
  return (deg >= 8) && (deg <= MAXDEGREE) && (deg % 8 == 0);
}

/* initialize 'poly' to a bitfield representing the coefficients of an
   irreducible polynomial of degree 'deg' */

void field_init(int deg)
{
  assert(field_size_valid(deg));
  if (! degree) {
    mpz_init_set_ui(poly, 0);
    mpz_setbit(poly, deg);
    mpz_setbit(poly, irred_coeff[3 * (deg / 8 - 1) + 0]);
    mpz_setbit(poly, irred_coeff[3 * (deg / 8 - 1) + 1]);
    mpz_setbit(poly, irred_coeff[3 * (deg / 8 - 1) + 2]);
    mpz_setbit(poly, 0);
    degree = deg;
  }
}

void field_deinit(void)
{
  mpz_clear(poly);
  degree = 0;
}

/* I/O routines for GF(2^deg) field elements */
/* clears x on error */

enum ssss_errcode field_import(mpz_t x, const char *s, int hexmode)
{
  enum ssss_errcode ec = ssss_ec_ok;
  if (hexmode) {
    if (strlen(s) > degree / 4)
      ec = ssss_err_input_string_too_long;
    else if (strlen(s) < degree / 4)
      warning("input string too short, adding null padding on the left");
    if (ec == ssss_ec_ok && (mpz_set_str(x, s, 16) || (mpz_cmp_ui(x, 0) < 0)))
      ec = ssss_err_invalid_syntax;
  }
  else {
    int i;
    int warn = 0;
    if (strlen(s) > degree / 8)
      ec = ssss_err_input_string_too_long;
    if (ec == ssss_ec_ok) {
      for(i = strlen(s) - 1; i >= 0; i--)
        warn = warn || (s[i] < 32) || (s[i] >= 127);
      if (warn)
        warning("binary data detected, use -x mode instead");
      mpz_import(x, strlen(s), 1, 1, 0, 0, s);
    }
  }
  if (ec != ssss_ec_ok)
    mpz_clear(x);
  return ec;
}

void field_print(FILE* stream, const mpz_t x, int hexmode)
{
  int i;
  if (hexmode) {
    for(i = degree / 4 - mpz_sizeinbase(x, 16); i; i--)
      fprintf(stream, "0");
    mpz_out_str(stream, 16, x);
    fprintf(stream, "\n");
  }
  else {
    char buf[MAXDEGREE / 8 + 1];
    size_t t;
    unsigned int i;
    int printable, warn = 0;
    memset(buf, 0, degree / 8 + 1);
    mpz_export(buf, &t, 1, 1, 0, 0, x);
    for(i = 0; i < t; i++) {
      printable = (buf[i] >= 32) && (buf[i] < 127);
      warn = warn || ! printable;
      fprintf(stream, "%c", printable ? buf[i] : '.');
    }
    fprintf(stream, "\n");
    if (warn)
      warning("binary data detected, use -x mode instead");
    secure_zero(buf, sizeof(buf));
  }
}

/* basic field arithmetic in GF(2^deg) */

/* field_sub is the same as field_add in this arithmetic. */

void field_add(mpz_t z, const mpz_t x, const mpz_t y)
{
  mpz_xor(z, x, y);
}

void field_mult(mpz_t z, const mpz_t x, const mpz_t y)
{
  mpz_t b;
  unsigned int i;
  assert(z != y);
  mpz_init_set(b, x);
  if (mpz_tstbit(y, 0))
    mpz_set(z, b);
  else
    mpz_set_ui(z, 0);
  for(i = 1; i < degree; i++) {
    mpz_lshift(b, b, 1);
    if (mpz_tstbit(b, degree))
      mpz_xor(b, b, poly);
    if (mpz_tstbit(y, i))
      mpz_xor(z, z, b);
  }
  mpz_clear(b);
}

void field_invert(mpz_t z, const mpz_t x)
{
  mpz_t u, v, g, h;
  int i;
  assert(mpz_cmp_ui(x, 0));
  mpz_init_set(u, x);
  mpz_init_set(v, poly);
  mpz_init_set_ui(g, 0);
  mpz_set_ui(z, 1);
  mpz_init(h);
  while (mpz_cmp_ui(u, 1)) {
    i = mpz_sizeinbits(u) - mpz_sizeinbits(v);
    if (i < 0) {
      mpz_swap(u, v);
      mpz_swap(z, g);
      i = -i;
    }
    mpz_lshift(h, v, i);
    mpz_xor(u, u, h);
    mpz_lshift(h, g, i);
    mpz_xor(z, z, h);
  }
  mpz_clear(u); mpz_clear(v); mpz_clear(g); mpz_clear(h);
}

/* routines for the random number generator */

enum ssss_errcode cprng_init(void)
{
  return ((cprng = open(RANDOM_SOURCE, O_RDONLY)) < 0) ?
    ssss_err_open_random : ssss_ec_ok;
}

enum ssss_errcode cprng_deinit(void)
{
  return (close(cprng) < 0) ?
    ssss_err_close_random : ssss_ec_ok;
}

enum ssss_errcode cprng_read(mpz_t x)
{
  enum ssss_errcode ec = ssss_ec_ok;
  char buf[MAXDEGREE / 8];
  unsigned int count;
  int i;
  for(count = 0; count < degree / 8; count += i)
    if ((i = read(cprng, buf + count, degree / 8 - count)) < 0) {
      close(cprng);
      ec = ssss_err_read_random;
      break;
    }
  if (ec == ssss_ec_ok)
    mpz_import(x, degree / 8, 1, 1, 0, 0, buf);
  secure_zero(buf, sizeof(buf));
  return ec;
}

/* a 64 bit pseudo random permutation (based on the XTEA cipher) */

void encipher_block(uint32_t *v)
{
  uint32_t sum = 0, delta = 0x9E3779B9;
  int i;
  for(i = 0; i < 32; i++) {
    v[0] += (((v[1] << 4) ^ (v[1] >> 5)) + v[1]) ^ sum;
    sum += delta;
    v[1] += (((v[0] << 4) ^ (v[0] >> 5)) + v[0]) ^ sum;
  }
}

void decipher_block(uint32_t *v)
{
  uint32_t sum = 0xC6EF3720, delta = 0x9E3779B9;
  int i;
  for(i = 0; i < 32; i++) {
    v[1] -= ((v[0] << 4 ^ v[0] >> 5) + v[0]) ^ sum;
    sum -= delta;
    v[0] -= ((v[1] << 4 ^ v[1] >> 5) + v[1]) ^ sum;
  }
}

void encode_slice(uint8_t *data, int idx, int len,
                  void (*process_block)(uint32_t*))
{
  uint32_t v[2];
  int i;
  for(i = 0; i < 2; i++)
    v[i] = data[(idx + 4 * i) % len] << 24 |
      data[(idx + 4 * i + 1) % len] << 16 |
      data[(idx + 4 * i + 2) % len] << 8 |
      data[(idx + 4 * i + 3) % len];
  process_block(v);
  for(i = 0; i < 2; i++) {
    data[(idx + 4 * i + 0) % len] = v[i] >> 24;
    data[(idx + 4 * i + 1) % len] = (v[i] >> 16) & 0xff;
    data[(idx + 4 * i + 2) % len] = (v[i] >> 8) & 0xff;
    data[(idx + 4 * i + 3) % len] = v[i] & 0xff;
  }
}

enum encdec {ENCODE, DECODE};

void encode_mpz(mpz_t x, enum encdec encdecmode)
{
  uint8_t v[(MAXDEGREE + 8) / 16 * 2];
  size_t t;
  int i;
  memset(v, 0, (degree + 8) / 16 * 2);
  mpz_export(v, &t, -1, 2, 1, 0, x);
  if (degree % 16 == 8)
    v[degree / 8 - 1] = v[degree / 8];
  if (encdecmode == ENCODE)             /* 40 rounds are more than enough!*/
    for(i = 0; i < 40 * ((int)degree / 8); i += 2)
      encode_slice(v, i, degree / 8, encipher_block);
  else
    for(i = 40 * (degree / 8) - 2; i >= 0; i -= 2)
      encode_slice(v, i, degree / 8, decipher_block);
  if (degree % 16 == 8) {
    v[degree / 8] = v[degree / 8 - 1];
    v[degree / 8 - 1] = 0;
  }
  mpz_import(x, (degree + 8) / 16, -1, 2, 1, 0, v);
  assert(mpz_sizeinbits(x) <= degree);
}

/* evaluate polynomials efficiently
 * Note that this implementation adds an additional x^k term. This term is
 * subtracted off on recombining. This additional term neither adds nor removes
 * security but is left solely for legacy reasons.
 */

void horner(int n, mpz_t y, const mpz_t x, const mpz_t coeff[])
{
  int i;
  mpz_set(y, x);
  for(i = n - 1; i; i--) {
    field_add(y, y, coeff[i]);
    field_mult(y, y, x);
  }
  field_add(y, y, coeff[0]);
}

/* horner() with reversed coeff[] */
/* coeff_rev[i] is a coefficient of x^(n - 1 - i) */

void horner_r(int n, mpz_t y, const mpz_t x, const mpz_t coeff_rev[])
{
  int i;
  mpz_set(y, x);
  for(i = 0; i < n - 1; i++) {
    field_add(y, y, coeff_rev[i]);
    field_mult(y, y, x);
  }
  field_add(y, y, coeff_rev[i]);
}

/* calculate the secret from a set of shares solving a linear equation system */

#define MPZ_SWAP(A, B) \
  do { mpz_set(h, A); mpz_set(A, B); mpz_set(B, h); } while(0)

int restore_secret(int n,
#ifdef USE_RESTORE_SECRET_WORKAROUND
                   void *A,
#else
                   mpz_t (*A)[n],
#endif
                   mpz_t b[], int recovery)
{
  mpz_t (*AA)[n] = (mpz_t (*)[n])A;
  int i, j, k, found;
  mpz_t h;
  mpz_init(h);
  /* Gaussian elimination. To imagine transformation into an upper triangular
   * matrix, treat AA[i][j] as AA[column][row] (perhaps this is because
   * Fortran matrix storage layout was reinterpreted as C storage layout).
   * Remember the field_add and field_sub sameness in this field arithmetic. */
  for(i = 0; i < n; i++) {
    if (! mpz_cmp_ui(AA[i][i], 0)) {
      for(found = 0, j = i + 1; j < n; j++)
        if (mpz_cmp_ui(AA[i][j], 0)) {
          found = 1;
          break;
        }
      if (! found)
        return -1;
      for(k = i; k < n; k++)
        MPZ_SWAP(AA[k][i], AA[k][j]);
      MPZ_SWAP(b[i], b[j]);
    }
    for(j = i + 1; j < n; j++) {
      if (mpz_cmp_ui(AA[i][j], 0)) {
        for(k = i + 1; k < n; k++) {
          field_mult(h, AA[k][i], AA[i][j]);
          field_mult(AA[k][j], AA[k][j], AA[i][i]);
          field_add(AA[k][j], AA[k][j], h);
        }
        field_mult(h, b[i], AA[i][j]);
        field_mult(b[j], b[j], AA[i][i]);
        field_add(b[j], b[j], h);
      }
    }
  }
  /* The matrix is in upper triangular form now.
   * Calculate the last coefficient (the secret). */
  field_invert(h, AA[n - 1][n - 1]);
  field_mult(b[n - 1], b[n - 1], h);
  if (recovery) {
    /* Transform AA to identity matrix and calculate other coefficients
     * to recover other shares. */
    for(i = n - 2; i >= 0; i--) {
      for(j = n - 1; j > i; j--) {
        field_mult(h, b[j], AA[j][i]);
        field_add(b[i], b[i], h);
      }
      field_invert(h, AA[i][i]);
      field_mult(b[i], b[i], h);
    }
  }
  mpz_clear(h);
  return 0;
}

/* ask for a secret */
/* clears secret on error */

enum ssss_errcode ask_secret(mpz_t secret)
{
  enum ssss_errcode ec = ssss_ec_ok;
  char buf[MAXLINELEN];
  int deg;
  if (! opt_quiet) {
    deg = opt_security ? opt_security : MAXDEGREE;
    fprintf(stderr, "Enter the secret, ");
    if (opt_hex)
      fprintf(stderr, "as most %d hex digits: ", deg / 4);
    else
      fprintf(stderr, "at most %d ASCII characters: ", deg / 8);
  }
  tcsetattr(0, TCSANOW, &echo_off);
  if (! fgets(buf, sizeof(buf), stdin))
    ec = ssss_err_io_reading_secret;
  if (ec == ssss_ec_ok) {
    tcsetattr(0, TCSANOW, &echo_orig);
    fprintf(stderr, "\n");
    buf[strcspn(buf, "\r\n")] = '\0';

    if (! opt_security) {
      opt_security = opt_hex ? 4 * ((strlen(buf) + 1) & ~1): 8 * strlen(buf);
      if (! field_size_valid(opt_security))
        ec = ssss_err_invalid_security_level;
      if (ec == ssss_ec_ok) {
        if (! opt_quiet)
          fprintf(stderr, "Using a %d bit security level.\n", opt_security);
      }
    }
  }

  if (ec == ssss_ec_ok) {
    field_init(opt_security);
    ec = field_import(secret, buf, opt_hex);
  } else
    mpz_clear(secret);

  if (ec == ssss_ec_ok)
    if (opt_diffusion) {
      if (degree >= 64)
        encode_mpz(secret, ENCODE);
      else
        warning("security level too small for the diffusion layer");
    }

  secure_zero(buf, sizeof(buf));
  return ec;
}

void calculate_shares_r(const mpz_t coeff_rev[]);

/* Prompt for a secret, generate shares for it */

enum ssss_errcode split(void)
{
  enum ssss_errcode ec = ssss_ec_ok;
  mpz_t coeff[opt_threshold];
  int i;
  if (! opt_quiet) {
    fprintf(stderr, "Generating shares using a (%d,%d) scheme with ",
            opt_threshold, opt_number);
    if (opt_security)
      fprintf(stderr, "a %d bit", opt_security);
    else
      fprintf(stderr, "dynamic");
    fprintf(stderr, " security level.\n");
  }
  i = opt_threshold - 1;
  mpz_init(coeff[i]);
  ec = ask_secret(coeff[i]);
  i--;

  if (ec == ssss_ec_ok)
    ec = cprng_init();
  for(; i >= 0 && ec == ssss_ec_ok; i--) {
    mpz_init(coeff[i]);
    ec = cprng_read(coeff[i]);
  }
  if (ec == ssss_ec_ok)
    ec = cprng_deinit();

  if (ec == ssss_ec_ok)
    calculate_shares_r(coeff);

  for(i = i + 1; i < opt_threshold; i++)
    mpz_clear(coeff[i]);
  field_deinit();
  return ec;
}

/* calculate shares */

void calculate_shares_r(const mpz_t coeff_rev[])
{
  unsigned int fmt_len;
  mpz_t x, y;
  int i;
  for(fmt_len = 1, i = opt_number; i >= 10; i /= 10, fmt_len++);
  mpz_init(x);
  mpz_init(y);
  for(i = 0; i < opt_number; i++) {
    mpz_set_ui(x, i + 1);
    horner_r(opt_threshold, y, x, coeff_rev);
    if (opt_token)
      fprintf(stdout, "%s-", opt_token);
    fprintf(stdout, "%0*d-", fmt_len, i + 1);
    field_print(stdout, y, 1);
  }
  mpz_clear(x);
  mpz_clear(y);
}

/* ask for i-th share (*s - share size (in/out parameter)) */
/* clears share on error, but leaves x */

enum ssss_errcode ask_share(mpz_t x, mpz_t share, unsigned *s, int i)
{
  enum ssss_errcode ec = ssss_ec_ok;
  char buf[MAXLINELEN];
  char *a, *b;
  int j;
  assert(s);
  if (! opt_quiet)
    fprintf(stderr, "Share [%d/%d]: ", i + 1, opt_threshold);

  if (! fgets(buf, sizeof(buf), stdin))
    ec = ssss_err_io_reading_shares;
  if (ec == ssss_ec_ok) {
    buf[strcspn(buf, "\r\n")] = '\0';
    if (! (b = strrchr(buf, '-')))
      ec = ssss_err_invalid_syntax;
  }
  if (ec == ssss_ec_ok) {
    *b++ = 0;
    if ((a = strrchr(buf, '-')))
      *a++ = 0;
    else
      a = buf;

    if (! *s) {
      *s = 4 * strlen(b);
      if (! field_size_valid(*s))
        ec = ssss_err_illegal_share_length;
      else
        field_init(*s);
    } else if (*s != 4 * strlen(b))
        ec = ssss_err_shares_different_security_levels;
  }

  if (ec == ssss_ec_ok) {
    if (! (j = atoi(a)))
      ec = ssss_err_invalid_share;
  }
  if (ec == ssss_ec_ok) {
      mpz_set_ui(x, j);
      ec = field_import(share, b, 1);
  } else
    mpz_clear(share);

  secure_zero(buf, sizeof(buf));
  return ec;
}

/* Prompt for shares, calculate the secret, recalculate same shares
 * if necessary */

enum ssss_errcode combine(int with_secret)
{
  enum ssss_errcode ec = ssss_ec_ok;
  mpz_t A[opt_threshold][opt_threshold], y[opt_threshold], x;
  int i, j;
  unsigned s = 0;

  mpz_init(x);
  if (! opt_quiet)
    fprintf(stderr, "Enter %d shares separated by newlines:\n", opt_threshold);
  for (i = 0; i < opt_threshold; i++) {
    mpz_init(y[i]);
    if (with_secret && i == 0) {
      /* For recovering purpose treat the secret as a share. */
      ec = ask_secret(y[i]);
      if (ec != ssss_ec_ok)
        break;
      s = opt_security;
      mpz_set_ui(x, 0);
    } else {
      ec = ask_share(x, y[i], &s, i);
      if (ec != ssss_ec_ok)
        break;
    }
    mpz_init_set_ui(A[opt_threshold - 1][i], 1);
    for(j = opt_threshold - 2; j >= 0; j--) {
      mpz_init(A[j][i]);
      field_mult(A[j][i], A[j + 1][i], x);
    }
    /* Remove x^k term. See comment at top of horner() */
    field_mult(x, x, A[0][i]);
    field_add(y[i], y[i], x);
  }
  if (ec == ssss_ec_ok)
    if (restore_secret(opt_threshold, A, y, opt_recovery))
      ec = ssss_err_inconsistent_shares;

  if (ec == ssss_ec_ok) {
    if (! with_secret) {
      mpz_set(x, y[opt_threshold - 1]);
      if (opt_diffusion) {
        if (degree >= 64)
          encode_mpz(x, DECODE);
        else
          warning("security level too small for the diffusion layer");
      }
      if (! opt_quiet)
        fprintf(stderr, "Resulting secret: ");
      field_print(stdout, x, opt_hex);
    }
    if (opt_recovery)
      calculate_shares_r(y);
  }

  mpz_clear(x);
  for (i = i - 1; i >= 0; i--) {
    for (j = opt_threshold - 1; j >= 0; j--)
      mpz_clear(A[j][i]);
    mpz_clear(y[i]);
  }
  field_deinit();
  return ec;
}

/* secure memory manipulation functions */

void secure_zero(void *s, size_t n)
{
  /* Do not use memset here, see https://www.viva64.com/en/b/0388/ */
  volatile char *p = s;
  if (p)
    while (n--) *p++ = '\0';
}

void secure_free(void *ptr, size_t size)
{
  secure_zero(ptr, size);
  free(ptr);
}

void * secure_realloc(void *ptr, size_t old_size, size_t new_size)
{
  void *new_ptr = malloc(new_size);
  if (new_ptr)
    memcpy(new_ptr, ptr, old_size < new_size ? old_size : new_size);
  secure_zero(ptr, old_size);
  free(ptr);
  return new_ptr;
}

int main(int argc, char *argv[])
{
  enum ssss_errcode ec = ssss_ec_ok;
  char *name;
  int i;

#if ! NOMLOCK
  int failedMemoryLock = 0;
  if (mlockall(MCL_CURRENT | MCL_FUTURE) < 0)
  {
    failedMemoryLock = 1;
    switch(errno) {
    case ENOMEM:
      warning("couldn't get memory lock (ENOMEM, try to adjust RLIMIT_MEMLOCK!)");
      break;
    case EPERM:
      warning("couldn't get memory lock (EPERM, try UID 0!)");
      break;
    case ENOSYS:
      warning("couldn't get memory lock (ENOSYS, kernel doesn't allow page locking)");
      break;
    default:
      warning("couldn't get memory lock");
      break;
    }
  }
#endif

  mp_set_memory_functions(NULL, secure_realloc, secure_free);

  if (getuid() != geteuid())
    seteuid(getuid());

  tcgetattr(0, &echo_orig);
  echo_off = echo_orig;
  echo_off.c_lflag &= ~ECHO;

  opt_help = argc == 1;
  const char* flags =
#if ! NOMLOCK
    "MvDhqQxrs:t:n:w:";
#else
    "vDhqQxrs:t:n:w:";
#endif

  while((i = getopt(argc, argv, flags)) != -1)
    switch(i) {
    case 'v': opt_showversion = 1; break;
    case 'h': opt_help = 1; break;
    case 'q': opt_quiet = 1; break;
    case 'Q': opt_QUIET = opt_quiet = 1; break;
    case 'x': opt_hex = 1; break;
    case 's': opt_security = atoi(optarg); break;
    case 't': opt_threshold = atoi(optarg); break;
    case 'n': opt_number = atoi(optarg); break;
    case 'w': opt_token = optarg; break;
    case 'D': opt_diffusion = 0; break;
    case 'r': opt_recovery = 1; break;
#if ! NOMLOCK
    case 'M':
      if(failedMemoryLock != 0)
        fatal("memory lock is required to proceed");
      break;
#endif
    default:
      exit(1);
    }
  if (! opt_help && (argc != optind))
    fatal("invalid argument");

  if ((name = strrchr(argv[0], '/')) == NULL)
    name = argv[0];

  if (strstr(name, "split")) {
    if (opt_help || opt_showversion) {
      fputs("Split secrets using Shamir's Secret Sharing Scheme.\n"
            "\n"
            "ssss-split -t threshold -n shares [-w token] [-s level]"
#if ! NOMLOCK
            " [-M]"
#endif
            " [-r] [-x] [-q] [-Q] [-D] [-v]\n",
            stderr);
      if (opt_showversion)
        fputs("\nVersion: " VERSION, stderr);
      exit(0);
    }

    if (opt_threshold < 2)
      fatal("invalid parameters: invalid threshold value");

    if (opt_number < opt_threshold)
      fatal("invalid parameters: number of shares smaller than threshold");

    if (opt_security && ! field_size_valid(opt_security))
      fatal("invalid parameters: invalid security level");

    if (opt_token && (strlen(opt_token) > MAXTOKENLEN))
      fatal("invalid parameters: token too long");

    /* Splitting in recovery mode is the same as combining, where one share
     * is the secret itself. */
    ec = (opt_recovery ? combine(1) : split());
  }
  else {
    if (opt_help || opt_showversion) {
      fputs("Combine shares using Shamir's Secret Sharing Scheme.\n"
            "\n"
            "ssss-combine -t threshold"
#if ! NOMLOCK
            " [-M]"
#endif
            " [-r -n shares] [-x] [-q] [-Q] [-D] [-v]\n",
            stderr);
      if (opt_showversion)
        fputs("\nVersion: " VERSION, stderr);
      exit(0);
    }

    if (opt_threshold < 2)
      fatal("invalid parameters: invalid threshold value");

    ec = combine(0);
  }
  if (ec != ssss_ec_ok)
    fatal_errcode(ec);
  return 0;
}

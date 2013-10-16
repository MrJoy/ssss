/*
 *  ssss version 0.1  -  Copyright 2005 B. Poettering
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
 * This is an implementation of Shamir's Secret Sharing Scheme. See 
 * http://en.wikipedia.org/wiki/Secret_sharing for more information 
 * on this topic.
 *
 * This code links against the GNU multiprecision library "libgmp".
 * I compiled the code successfully with gmp 4.1.4.
 * You will need a system that has a /dev/random entropy source.
 *
 * Compile with "gcc -O2 -lgmp -o ssss ssss.c"
 *
 * Report bugs to: bpssss AT point-at-infinity.org
 */

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>

#include <gmp.h>

static const char help_message[] = 
"\n"
"An implementation of Shamir's Secret Sharing Scheme.\n"
"\n"
"Syntax:\n"
"\tssss split -t threshold -n shares [-s level] [-w token] [-x] [-q]\n"
"\tssss combine -t threshold [-x] [-q]\n"
"\n"
"Commands:\n"
"\tsplit: prompt the user for a secret and calculate a set of\n"
"\tcorresponding shares.\n"
"\n"
"\tcombine: read in a set of shares and reconstruct the secret.\n"
"\n"
"Options:\n"
"\t-t threshold\n"
"\tthe number of shares necessary to reconstruct the secret.\n"
"\n"
"\t-n shares\n"
"\tthe number of shares to be generated.\n"
"\n"
"\t-s level\n"
"\tsecurity level: the scheme's security level in bits. The security\n"
"\tlevel is an upper bound for the length of the shared secret\n"
"\t(shorter secrets are padded). Allowed values are 80, 112, 128,\n"
"\t192, 256, 512 and 1024. The default is 128.\n"
"\n"
"\t-w token\n"
"\ttext token to name shares in order to avoid confusion in case you\n"
"\tutilize secret sharing to protect several independent secrets. The\n"
"\tgenerated shares are prefixed by these tokens.\n"
"\n"
"\t-x\n"
"\thex mode: use hexadecimal digits in place of ASCII characters for\n"
"\tI/O. This is useful if you want to protect binary data, like\n"
"\tblock cipher keys.\n"
"\n"
"\t-q\n"
"\tquiet mode: disable all unnecessary output. Useful in scripts.\n"
"\tNote: the option -Q works like -q, but warnings are suppressed also.\n"
"\n"
"Example:\n"
"\tSuppose you want to protect your login password with a set of 10\n"
"\tshares, in such a way that any 3 of them can reconstruct the\n"
"\tpassword, simply run the command\n"
"\n"
"\t  ssss split -t 3 -n 10 -w passwd\n"
"\n"
"\tTo reconstruct the password pass 3 of the generated shares (in any\n"
"\torder) to\n"
"\n"
"\t  ssss combine -t 3\n"
"\n"
"Further reading:\n"
"\thttp://en.wikipedia.org/wiki/Secret_sharing\n"
"\n"
"Author & Contact:\n"
"\tThis software was written in 2005 by B. Poettering. Please send bug\n"
"\treports to bpssss@point-at-infinity.org. Version number: 0.1\n";

#define RANDOM_SOURCE "/dev/random"
#define MAXDEGREE 1024
#define MAXTOKENLEN 128
#define MAXLINELEN (MAXTOKENLEN + 1 + 10 + 1 + MAXDEGREE / 4 + 10)

int opt_help = 0;
int opt_quiet = 0;
int opt_QUIET = 0;
int opt_hex = 0;
int opt_security = 128;
int opt_threshold = -1;
int opt_number = -1;
char *opt_token = NULL;

unsigned int degree;
mpz_t poly;
int cprng;

#define mpz_lshift(A, B, l) mpz_mul_2exp(A, B, l)
#define mpz_sizeinbits(A) (mpz_cmp_ui(A, 0) ? mpz_sizeinbase(A, 2) : 0)

void fatal(char *msg)
{
  fprintf(stderr, "\aFATAL: %s.\n", msg);
  exit(1);
}

void warning(char *msg)
{
  if (! opt_QUIET)
    fprintf(stderr, "\aWARNING: %s.\n", msg);
}

int field_size_valid(int deg)
{
  return (deg == 80) || (deg == 112) || (deg == 128) || (deg == 192) 
    || (deg == 256) || (deg == 512) || (deg == 1024);
  assert(deg <= MAXDEGREE);
}

/* initialize 'poly' to a bitfield representing the coefficients of an
   irreducible polynomial of degree 'deg' */

void field_init(int deg)
{
  mpz_init_set_ui(poly, 0);
  switch (deg) {
  case 80: 
    mpz_setbit(poly, 9);
    mpz_setbit(poly, 4);
    mpz_setbit(poly, 2);
    break;
  case 112:
    mpz_setbit(poly, 5);
    mpz_setbit(poly, 4);
    mpz_setbit(poly, 3);
    break;
  case 128:
  case 192:
    mpz_setbit(poly, 7);
    mpz_setbit(poly, 2);
    mpz_setbit(poly, 1);
    break;
  case 256:
    mpz_setbit(poly, 10);
    mpz_setbit(poly, 5);
    mpz_setbit(poly, 2);
    break;
  case 512:
    mpz_setbit(poly, 8);
    mpz_setbit(poly, 5);
    mpz_setbit(poly, 2);
    break;
  case 1024:
    mpz_setbit(poly, 19);
    mpz_setbit(poly, 6);
    mpz_setbit(poly, 1);
    break;
  default:
    assert(0);
  }
  mpz_setbit(poly, deg);
  mpz_setbit(poly, 0);
  degree = deg;
}

void field_deinit(void)
{
  mpz_clear(poly);
}

/* I/O routines for GF(2^deg) field elements */

void field_import(mpz_t x, const char *s, int hexmode)
{
  if (hexmode) {
    if (strlen(s) > degree / 4)
      fatal("string too long");
    if (strlen(s) < degree / 4)
      warning("string too short, adding null padding on the left");
    if (mpz_set_str(x, s, 16) || (mpz_cmp_ui(x, 0) < 0))
      fatal("invalid syntax");
  }
  else {
    int i;
    int warn = 0;
    if (strlen(s) > degree / 8)
      fatal("string too long");
    for(i = strlen(s) - 1; i >= 0; i--)
      warn = warn || (s[i] <= 32) || (s[i] >= 127);
    if (warn)
      warning("binary data detected, use -x mode instead");
    mpz_import(x, strlen(s), 1, 1, 0, 0, s);
  }
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
    memset(buf, degree / 8 + 1, 0);
    mpz_export(buf, &t, 1, 1, 0, 0, x);
    for(i = 0; i < t; i++) {
      printable = (buf[i] > 32) && (buf[i] < 127);
      warn = warn || ! printable;
      fprintf(stream, "%c", printable ? buf[i] : '.');
    }
    fprintf(stream, "\n");
    if (warn)
      warning("binary data detected, use -x mode instead");
  }
}

/* basic field arithmetic in GF(2^deg) */

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
  int j;
  assert(mpz_cmp_ui(x, 0));
  mpz_init_set(u, x);
  mpz_init_set(v, poly);
  mpz_init_set_ui(g, 0);
  mpz_set_ui(z, 1);
  mpz_init(h);
  while (mpz_cmp_ui(u, 1)) {
    j = (mpz_sizeinbits(u) + 1) - (mpz_sizeinbits(v) + 1);
    if (j < 0) {
      mpz_set(h, u); mpz_set(u, v); mpz_set(v, h);
      mpz_set(h, z); mpz_set(z, g); mpz_set(g, h);
      j = -j;
    }
    mpz_lshift(h, v, j);
    mpz_xor(u, u, h);
    mpz_lshift(h, g, j);
    mpz_xor(z, z, h);
  }
  mpz_clear(u); mpz_clear(v); mpz_clear(g); mpz_clear(h);
}

/* routines for the random number generator */

void cprng_init(void)
{
  if ((cprng = open(RANDOM_SOURCE, O_RDONLY)) < 0)
    fatal("couldn't open " RANDOM_SOURCE);
}

void cprng_deinit(void)
{
  if (close(cprng) < 0)
    fatal("couldn't close " RANDOM_SOURCE);
}

void cprng_read(mpz_t x)
{
  char buf[MAXDEGREE / 8];
  unsigned int count;
  int i;
  for(count = 0; count < degree / 8; count += i)
    if ((i = read(cprng, buf + count, degree / 8 - count)) < 0) {
      close(cprng);
      fatal("couldn't read from " RANDOM_SOURCE);
    }
  mpz_import(x, degree / 8, 1, 1, 0, 0, buf);
}

/* evaluate polynomials efficiently */

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

/* calculate the secret from a set of shares solving a linear equation system */

#define MPZ_SWAP(A, B) \
  do { mpz_set(h, A); mpz_set(A, B); mpz_set(B, h); } while(0)

int restore_secret(int n, mpz_t A[][], mpz_t b[]) 
{
  mpz_t (*AA)[n] = (mpz_t (*)[n])A;
  int i, j, k, found;
  mpz_t h;
  mpz_init(h);
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
  field_invert(h, AA[n - 1][n - 1]);
  field_mult(b[n - 1], b[n - 1], h);
  mpz_clear(h);
  return 0;
}

/* Prompt for a secret, generate shares for it */

void split(void)
{
  unsigned int fmt_len;
  mpz_t x, y, coeff[opt_threshold];
  char buf[MAXLINELEN];
  int i;
  for(fmt_len = 1, i = opt_number; i >= 10; i /= 10, fmt_len++);
  field_init(opt_security);

  if (! opt_quiet)
    printf("Generating shares using a (%d,%d) scheme with a %d bit "
	   "security level.\n", opt_threshold, opt_number, opt_security);

  if (! opt_quiet) {
    fprintf(stderr, "Enter the secret, ");
    if (opt_hex)
      fprintf(stderr, "%d hex digits: ", degree / 4);
    else 
      fprintf(stderr, "at most %d ASCII characters: ", degree / 8);
  }
  if (! fgets(buf, sizeof(buf), stdin))
    fatal("I/O error while reading secret");
  buf[strcspn(buf, "\r\n")] = '\0';
  mpz_init(coeff[0]);
  field_import(coeff[0], buf, opt_hex);

  cprng_init();
  for(i = 1; i < opt_threshold; i++) {
    mpz_init(coeff[i]);
    cprng_read(coeff[i]);
  }
  cprng_deinit();

  mpz_init(x);
  mpz_init(y);
  for(i = 0; i < opt_number; i++) {
    mpz_set_ui(x, i + 1);
    horner(opt_threshold, y, x, (const mpz_t*)coeff);
    if (opt_token)
      printf("%s-", opt_token);
    printf("%0*d-", fmt_len, i + 1);
    field_print(stdout, y, 1);
  }
  mpz_clear(x);
  mpz_clear(y);

  for(i = 0; i < opt_threshold; i++)
    mpz_clear(coeff[i]);
  field_deinit();
}

/* Prompt for shares, calculate the secret */

void combine(void)
{
  mpz_t A[opt_threshold][opt_threshold], y[opt_threshold], x;
  char buf[MAXLINELEN];
  char *a, *b;
  int i, j;
  unsigned s = 0;

  mpz_init(x);
  if (! opt_quiet)
    printf("Enter %d shares separated by newlines:\n", opt_threshold);
  for (i = 0; i < opt_threshold; i++) {
    if (! opt_quiet)
      printf("Share [%d/%d]: ", i + 1, opt_threshold);
  
    if (! fgets(buf, sizeof(buf), stdin))
      fatal("I/O error while reading shares");
    buf[strcspn(buf, "\r\n")] = '\0';
    if (! (a = strchr(buf, '-')))
      fatal("invalid syntax");
    *a++ = 0;
    if ((b = strchr(a, '-')))
      *b++ = 0;
    else
      b = a, a = buf;

    if (! s) {
      s = 4 * strlen(b);
      if (! field_size_valid(s))
	fatal("share has illegal length");
      field_init(s);
    }
    else
      if (s != 4 * strlen(b))
	fatal("shares have different security levels");

    if (! (j = atoi(a)))
      fatal("invalid share");
    mpz_set_ui(x, j);
    mpz_init_set_ui(A[opt_threshold - 1][i], 1);
    for(j = opt_threshold - 2; j >= 0; j--) {
      mpz_init(A[j][i]);
      field_mult(A[j][i], A[j + 1][i], x);
    }
    mpz_init(y[i]);
    field_import(y[i], b, 1);
    field_mult(x, x, A[0][i]);
    field_add(y[i], y[i], x);
  }
  mpz_clear(x);
  if (restore_secret(opt_threshold, A, y))
    fatal("shares inconsistent. Perhaps a single share was used twice?");

  if (! opt_quiet)
    fprintf(stderr, "Resulting secret: "); 
  field_print(stderr, y[opt_threshold - 1], opt_hex);
  
  for (i = 0; i < opt_threshold; i++) {
    for (j = 0; j < opt_threshold; j++)
      mpz_clear(A[i][j]);
    mpz_clear(y[i]);
  }
  field_deinit();
}

int main(int argc, char *argv[])
{
  int i;
  opt_help = argc == 1;
  while((i = getopt(argc, argv, "hqQxs:t:n:w:")) != -1)
    switch(i) {
    case 'h': opt_help = 1; break;
    case 'q': opt_quiet = 1; break;
    case 'Q': opt_QUIET = opt_quiet = 1; break;
    case 'x': opt_hex = 1; break;
    case 's': opt_security = atoi(optarg); break;
    case 't': opt_threshold = atoi(optarg); break;
    case 'n': opt_number = atoi(optarg); break;
    case 'w': opt_token = optarg; break;
    default:
      exit(1);
    }

  if (opt_help) {
    puts(help_message);
    exit(0);
  }

  if (argc - optind < 1)
    fatal("invalid parameters: no command given");
  if (argc - optind > 1)
    fatal("invalid parameters: too many commands given");

  if (! field_size_valid(opt_security))
    fatal("invalid parameters: invalid security level");

  if (opt_threshold < 2)
    fatal("invalid parameters: threshold value too small");

  if (opt_token && (strlen(opt_token) > MAXTOKENLEN))
    fatal("invalid parameters: token too long");

  if (! strcmp(argv[optind], "split")) {
    if (opt_number < opt_threshold)
      fatal("invalid parameters: number of shares smaller than threshold");
    split();
  }
  else if (! strcmp(argv[optind], "combine"))
    combine();
  else
    fatal("invalid parameters: unknown command");

  return 0;
}

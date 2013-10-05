/*

Copyright (c) 2013, Daniel S. Standage <daniel.standage@gmail.com>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

*/

#include <getopt.h>
#include <stdio.h>
#include "khash.h"

//------------------------------------------------------------------------------
// Definitions/prototypes/initializations for data structures, functions, etc.
//------------------------------------------------------------------------------
#define MAX_LINE_LENGTH 4096
KHASH_MAP_INIT_STR(m32, unsigned)

typedef struct
{
  const char *outfile;
  FILE *outstream;
  const char *idfmt;
  const char *infile;
  FILE *instream;
  int noids;
  unsigned rangestart;
  unsigned rangeend;
} SmrOptions;

void smr_init_options(SmrOptions *options);
void smr_parse_options(SmrOptions *options, int argc, char **argv);
void smr_print_usage(FILE *outstream);


//------------------------------------------------------------------------------
// Function implementations
//------------------------------------------------------------------------------

int main(int argc, char **argv)
{
  char buffer[MAX_LINE_LENGTH];
  khash_t(m32) *map;
  khint_t key;
  SmrOptions options;

  smr_init_options(&options);
  smr_parse_options(&options, argc, argv);

  map = kh_init(m32);
  while(fgets(buffer, MAX_LINE_LENGTH, options.instream) != NULL)
  {
    const char *tok = strtok(buffer, "\t\n");
    tok = strtok(NULL, "\t\n");
    tok = strtok(NULL, "\t\n");

    key = kh_get(m32, map, tok);
    if(key == kh_end(map))
    {
      int code;
      const char *tsaid = strdup(tok);
      key = kh_put(m32, map, tsaid, &code);
      if(!code)
      {
        fprintf(stderr, "error: failure storing key '%s'\n", tsaid);
        kh_del(m32, map, key);
      }
      else
        kh_value(map, key) = 0;
    }
    unsigned tsareadcount = kh_value(map, key);
    kh_value(map, key) = tsareadcount + 1;
  }

  unsigned i;
  for(i = options.rangestart; i <= options.rangeend; i++) // 209675
  {
    char tsaid[32];
    sprintf(tsaid, options.idfmt, i);

    key = kh_get(m32, map, tsaid);
    if(key == kh_end(map))
    {
      if(options.noids)
        fputs("0\n", options.outstream);
      else
        fprintf(options.outstream, "%s\t0\n", tsaid);
    }
    else
    {
      unsigned tsareadcount = kh_value(map, key);
      if(options.noids)
        fprintf(options.outstream, "%u\n", tsareadcount);
      else
        fprintf(options.outstream, "%s\t%u\n", tsaid, tsareadcount);
      char *keystr = (char *)kh_key(map, key);
      free(keystr);
    }
  }

  kh_destroy(m32, map);
  fclose(options.outstream);
  fclose(options.instream);
  return 0;
}

void smr_init_options(SmrOptions *options)
{
  options->outfile    = "stdout";
  options->outstream  = stdout;
  options->idfmt      = "gene%05d";
  options->infile     = NULL;
  options->instream   = NULL;
  options->noids      = 0;
  options->rangestart = 1;
  options->rangeend   = 10;
}

void smr_parse_options(SmrOptions *options, int argc, char **argv)
{
  int opt = 0;
  int optindex = 0;
  const char *optstr = "hi:no:r:";
  const struct option smr_options[] =
  {
    { "help",    no_argument,       NULL, 'h' },
    { "idfmt",   required_argument, NULL, 'i' },
    { "noids",   no_argument,       NULL, 'n' },
    { "outfile", required_argument, NULL, 'o' },
    { "idrange", required_argument, NULL, 'r' },
    { NULL,      no_argument,       NULL,  0  },
  };

  for(opt = getopt_long(argc, argv, optstr, smr_options, &optindex);
      opt != -1;
      opt = getopt_long(argc, argv, optstr, smr_options, &optindex))
  {
    const char *tok;
    switch(opt)
    {
      case 'h':
        smr_print_usage(stdout);
        exit(0);
        break;
      case 'i':
        options->idfmt = optarg;
        break;
      case 'n':
        options->noids = 1;
        break;
      case 'o':
        options->outfile = optarg;
        break;
      case 'r':
        tok = strtok(optarg, "-\n");
        options->rangestart = atoi(tok);
        tok = strtok(NULL, "-\n");
        options->rangeend = atoi(tok);
        if(options->rangestart > options->rangeend)
        {
          fprintf(stderr, "error: range %u-%u is invalid, start must be "
                  "smaller than end\n", options->rangestart, options->rangeend);
          smr_print_usage(stderr);
          exit(1);
        }
        break;
      default:
        fprintf(stderr, "error: unknown option '%c'\n", opt);
        smr_print_usage(stderr);
        break;
    }
  }

  if(strcmp(options->outfile, "stdout") != 0)
  {
    options->outstream = fopen(options->outfile, "w");
    if(options->outstream == NULL)
    {
      fprintf(stderr, "error: unable to open output file '%s'\n",
              options->outfile);
      exit(1);
    }
  }

  int numfiles = argc - optind;
  if(numfiles != 1)
  {
    fprintf(stderr, "expected 1 input file, but found %d\n", numfiles);
    smr_print_usage(stderr);
    exit(1);
  }
  options->infile = argv[optind];
  options->instream = fopen(options->infile, "r");
  if(options->instream == NULL)
  {
    fprintf(stderr, "error: unable to open input file '%s'\n", options->infile);
    exit(1);
  }
}

void smr_print_usage(FILE *outstream)
{
  fputs("Usage: smr [options] reads.sam\n"
"  Options:\n"
"    -h|--help                print this help message and exit\n"
"    -i|--idfmt: STRING       format of the molecule IDs in printf style;\n"
"                             default is 'gene%05d'\n"
"    -n|--noids:              do not print molecule IDs, only the read\n"
"                             counts; default is to print both, tab-delimited\n"
"    -o|--outfile: FILE       name of file to which read counts will be\n"
"                             written; default is terminal (stdout)\n"
"    -r|--idrange: INT-INT    range of IDs to print; if your data set\n"
"                             includes 10,000 molecules, then you should use\n"
"                             '1-10000'; the unrealistic default is '1-10'\n",
        outstream);
}


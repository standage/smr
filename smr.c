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
#define MAX_LINE_LENGTH 8192
#define MAX_ID_LENGTH 1024
KHASH_MAP_INIT_STR(m32, unsigned)

typedef struct
{
  const char *outfile;
  FILE *outstream;
  const char *idfmt;
  int noids;
  unsigned rangestart;
  unsigned rangeend;
  unsigned numfiles;
} SmrOptions;

void smr_init_options(SmrOptions *options);
khash_t(m32) *smr_load_file(const char *filename);
void smr_parse_options(SmrOptions *options, int argc, char **argv);
void smr_print_matrix(SmrOptions *options, khash_t(m32) **maps);
void smr_print_usage(FILE *outstream);
void smr_terminate(SmrOptions *options, khash_t(m32) **maps);

//------------------------------------------------------------------------------
// Main method
//------------------------------------------------------------------------------

int main(int argc, char **argv)
{
  SmrOptions options;
  smr_init_options(&options);
  smr_parse_options(&options, argc, argv);

  unsigned i;
  khash_t(m32) **maps = malloc( sizeof(void *) * options.numfiles );
  for(i = 0; i < options.numfiles; i++)
    maps[i] = smr_load_file(argv[optind+i]);
  smr_print_matrix(&options, maps);

  smr_terminate(&options, maps);
  return 0;
}

//------------------------------------------------------------------------------
// Function implementations
//------------------------------------------------------------------------------

void smr_init_options(SmrOptions *options)
{
  options->outfile    = "stdout";
  options->outstream  = stdout;
  options->idfmt = "gene%05d";
  options->noids = 0;
  options->rangestart = 1;
  options->rangeend = 10;
  options->numfiles   = 0;
}

khash_t(m32) *smr_load_file(const char *filename)
{
  FILE *instream = fopen(filename, "r");
  if(instream == NULL)
  {
    fprintf(stderr, "error opening file '%s'\n", filename);
    exit(1);
  }

  khash_t(m32) *map = kh_init(m32);
  char buffer[MAX_LINE_LENGTH];
  while(fgets(buffer, MAX_LINE_LENGTH, instream) != NULL)
  {
    const char *tok = strtok(buffer, "\t\n");
    tok = strtok(NULL, "\t\n");
    int bflag = atoi(tok);
    if(bflag & 0x4)
      continue;

    tok = strtok(NULL, "\t\n");
    khint_t key = kh_get(m32, map, tok);

    if(key == kh_end(map))
    {
      int code;
      const char *molid = strdup(tok);
      key = kh_put(m32, map, molid, &code);
      if(!code)
      {
        fprintf(stderr, "error: failure storing key '%s'\n", molid);
        kh_del(m32, map, key);
      }
      else
        kh_value(map, key) = 0;
    }
    unsigned tsareadcount = kh_value(map, key);
    kh_value(map, key) = tsareadcount + 1;
  }

  fclose(instream);
  return map;
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

  options->numfiles = argc - optind;
  if(options->numfiles < 1)
  {
    fputs("expected 1 or more input files\n", stderr);
    smr_print_usage(stderr);
    exit(1);
  }
}

void smr_print_matrix(SmrOptions *options, khash_t(m32) **maps)
{
  unsigned i, j;
  for(i = options->rangestart; i <= options->rangeend; i++)
  {
    char molid[MAX_ID_LENGTH];
    sprintf(molid, options->idfmt, i);
    if(!options->noids)
      fprintf(options->outstream, "%s,", molid);

    for(j = 0; j < options->numfiles; j++)
    {
      if(j > 0)
        fputc(',', options->outstream);

      khash_t(m32) *map = maps[j];
      khint_t key = kh_get(m32, map, molid);

      if(key == kh_end(map))
        fputs("0", options->outstream);
      else
      {
        unsigned tsareadcount = kh_value(map, key);
        fprintf(options->outstream, "%u", tsareadcount);
        char *keystr = (char *)kh_key(map, key);
        free(keystr);
      }
    }
    fputc('\n', options->outstream);
  }
}

void smr_print_usage(FILE *outstream)
{
  fputs("\nSMR: SAM mapped reads\n\n"
"The input to SMR is 1 or more SAM files. The output is a table (1 column for\n"
"each input file) showing the number of reads that map to each sequence.\n\n"
"Usage: smr [options] sample-1.sam sample-2.sam ... sample-n.sam\n"
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

void smr_terminate(SmrOptions *options, khash_t(m32) **maps)
{
  unsigned i;
  for(i = 0; i < options->numfiles; i++)
  {
    khash_t(m32) *map = maps[i];
    kh_destroy(m32, map);
  }
  free(maps);
  fclose(options->outstream);
}

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

#include <unistd.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

typedef std::unordered_set<std::string> uset;
typedef std::unordered_map<std::string, unsigned> umap;
struct SmrOptions
{
  char delim;
  std::string outfile;
  std::ostream& outstream;
  unsigned numfiles;
  std::vector<std::string> infiles;
  SmrOptions(char d, std::string of, std::ostream& os, unsigned nf,
             std::vector<std::string> infls)
  : delim(d), outfile(of), outstream(os), numfiles(nf), infiles(infls) {}
};
typedef struct SmrOptions SmrOptions;

uset smr_collect_molids(SmrOptions& options, std::vector<umap>& rm2seqPerSample);
umap smr_load_file(std::istream& instream, char delim);
SmrOptions smr_parse_options(int argc, char **argv);
void smr_print_matrix(SmrOptions& options, std::vector<umap>& rm2seqPerSample);
void smr_print_usage(std::ostream& outstream);
std::vector<std::string>& smr_string_split(const std::string &s, char delim,
                                           std::vector<std::string> &elems);

//------------------------------------------------------------------------------
// Main method
//------------------------------------------------------------------------------

int main(int argc, char **argv)
{
  SmrOptions options = smr_parse_options(argc, argv);

  std::vector<umap> rm2seqPerSample;
  for(unsigned i = 0; i < options.numfiles; i++)
  {
    std::ifstream ifs (std::string(options.infiles[i]));
    if(!ifs.is_open())
    {
      std::cerr << "error opening input file "
                << options.infiles[i]
                << std::endl;
      exit(1);
    }
    rm2seqPerSample.push_back(smr_load_file(ifs, options.delim));
    ifs.close();
  }
  smr_print_matrix(options, rm2seqPerSample);

  //options.outstream.close();
  return 0;
}

//------------------------------------------------------------------------------
// Function implementations
//------------------------------------------------------------------------------
uset smr_collect_molids(SmrOptions& options, std::vector<umap>& rm2seqPerSample)
{
  uset molids;
  for(std::vector<umap>::iterator ssiter = rm2seqPerSample.begin();
      ssiter != rm2seqPerSample.end();
      ssiter++)
  {
    umap& rm2seq = *ssiter;
    for(umap::iterator iter = rm2seq.begin(); iter != rm2seq.end(); iter++)
    {
      molids.emplace(iter->first);
    }
  }
  return molids;
}

umap smr_load_file(std::istream& instream, char delim)
{
  umap rm2seq;

  std::string buffer;
  while(std::getline(instream, buffer))
  {
    if(buffer[0] == '@')
      continue;
    
    std::vector<std::string> tokens;
    smr_string_split(buffer, '\t', tokens);
    std::string molid = tokens[2];
    std::string bflag_str = tokens[1];
    int bflag = std::stoi(bflag_str);
    if(bflag & 0x4)
      continue;

    umap::iterator keyvaluepair = rm2seq.find(molid);
    if(keyvaluepair == rm2seq.end())
      rm2seq.emplace(molid, 1);
    else
      rm2seq[molid] += 1;
  }

  return rm2seq;
}

SmrOptions smr_parse_options(int argc, char **argv)
{
  char delim = ',';
  std::string outfile = "stdout";

  char opt;
  const char *arg;
  while((opt = getopt(argc, argv, "d:ho:")) != -1)
  {
    switch(opt)
    {
      case 'd':
        arg = optarg;
        if(strcmp(optarg, "\\t") == 0)
          arg = "\t";
        else if(strlen(optarg) > 1)
        {
          std::cerr << "warning: string '"
                    << arg
                    << "' provided for delimiter, using only '"
                    << arg[0]
                    << "'"
                    << std::endl;
        }
        delim = arg[0];
        break;
      case 'h':
        smr_print_usage(std::cout);
        exit(0);
        break;
      case 'o':
        outfile = optarg;
        break;
      default:
        fprintf(stderr, "error: unknown option '%c'\n", opt);
        smr_print_usage(std::cerr);
        break;
    }
  }

  std::ofstream outfilestream;
  bool usestdout = (outfile.compare(std::string("stdout")) == 0);
  if(!usestdout)
    outfilestream.open(outfile, std::ios::out);
  std::ostream& outstream = (usestdout ? std::cout : outfilestream);

  unsigned numfiles = argc - optind;
  if(numfiles < 1)
  {
    std::cerr << "expected 1 or more input files" << std::endl;
    smr_print_usage(std::cerr);
    exit(1);
  }

  std::vector<std::string> infiles;
  for(unsigned i = 0; i < numfiles; i++)
  {
    int ind = optind + i;
    std::string filename (argv[ind]);
    infiles.push_back(filename);
  }

  return SmrOptions (delim, outfile, outstream, numfiles, infiles);
}

void smr_print_matrix(SmrOptions& options, std::vector<umap>& rm2seqPerSample)
{
  uset molids = smr_collect_molids(options, rm2seqPerSample);
  for(uset::iterator iter = molids.begin(); iter != molids.end(); iter++)
  {
    std::string molid = *iter;
    options.outstream << molid << options.delim;

    for(std::vector<umap>::iterator sampleiter = rm2seqPerSample.begin();
        sampleiter != rm2seqPerSample.end();
        sampleiter++)
    {
      if(sampleiter != rm2seqPerSample.begin())
        options.outstream << options.delim;

      umap rm2seq = *sampleiter;
      umap::const_iterator keyvaluepair = rm2seq.find(molid);
      if(keyvaluepair == rm2seq.end())
        options.outstream << '0';
      else
        options.outstream << keyvaluepair->second;
    }
    options.outstream << std::endl;
  }
}

void smr_print_usage(std::ostream& outstream)
{
  outstream << std::endl << "SMR: SAM mapped reads" << std::endl << std::endl
            << "The input to SMR is 1 or more SAM files. The output is a table (1 column for" << std::endl
            << "each input file) showing the number of reads that map to each sequence." << std::endl << std::endl
            << "Usage: smr [options] sample-1.sam sample-2.sam ... sample-n.sam" << std::endl
            << "  Options:" << std::endl
            << "    -d|--delim: CHAR         delimiter for output data; default is comma" << std::endl
            << "    -h|--help                print this help message and exit" << std::endl
            << "    -o|--outfile: FILE       name of file to which read counts will be" << std::endl
            << "                             written; default is terminal (stdout)" << std::endl << std::endl;
}

std::vector<std::string>& smr_string_split(const std::string &s, char delim,
                                           std::vector<std::string> &elems)
{
    std::stringstream ss(s);
    std::string item;
    while(std::getline(ss, item, delim))
    {
      elems.push_back(item);
    }
    return elems;
}

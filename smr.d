import std.conv, std.getopt, std.stdio, std.string;

// Simple data structure for command-line arguments, program options
struct OptsArgs
{
  char delim;
  bool help;
  string outfile;
  string[] files;
}

// Custom exception class prints usage if incorrect arguments provided
class ArgumentException : Exception
{
  this(string msg)
  {
    super(msg);
    print_usage(stderr);
  }
}

// Main procedure
void main(string[] args)
{
  OptsArgs opts = parse_cli_args(args);
  ulong[string][] maps = load_input(opts);
  string[] molids = collect_molids(maps);
  print_table(molids, maps, opts.outfile, opts.delim);
}

// Given an array of maps (molid => read count, 1 map per input file), create an
// array containing the union of all the maps' keys
string[] collect_molids(ref ulong[string][] maps)
{
  bool[string] molids;
  foreach(map; maps)
  {
    string[] ids = map.keys;
    foreach(id; ids)
    {
      if(!(id in molids))
        molids[id] = true;
    }
  }
  return molids.keys;
}

// For each input file, construct a map (associative array) where each key is a
// molecule ID and each value is the number of reads mapped to that molecule;
// return a list containing the map for each file
ulong[string][] load_input(OptsArgs opts)
{
  ulong[string][] maps;
  foreach(infile; opts.files)
  {
    ulong[string] map;
    string buffer;
    File instream = File(infile, "r");
    while(instream.readln(buffer))
    {
      if(buffer[0] == '@')
        continue;

      string[] tokens = split(buffer, "\t");
      int bitwiseflag = to!(int)(tokens[1]);
      if(bitwiseflag & 4)
        continue;

      string molid = tokens[2];
      if(!(molid in map))
        map[molid] = 0;
      map[molid] += 1;
    }
    instream.close();
    maps ~= map;
  }
  return maps;
}

// Parse and store command line arguments and options
OptsArgs parse_cli_args(string[] args)
{
  OptsArgs opts;
  opts.delim = ',';
  opts.help = false;
  opts.outfile = "stdout";

  getopt(args, "delim|d", &opts.delim,
               "help|h",  &opts.help,
               "out|o",   &opts.outfile);

  if(opts.help)
    print_usage(stdout);
  else if(args.length <= 1)
    throw new ArgumentException("0 input files provided");

  opts.files = args[1 .. $];

  return opts;
}

// Print the number of reads mapped to each molecule from each input file
void print_table(ref string[] molids, ref ulong[string][] maps, string outfile,
                 char delim)
{
  File outstream = stdout;
  if(cmp(outfile, "stdout") != 0)
    outstream = File(outfile, "w");

  foreach(molid; molids)
  {
    outstream.writef("%s%c", molid, delim);
    foreach(i, map; maps)
    {
      if(i > 0)
        outstream.write(delim);

      if(!(molid in map))
        outstream.write('0');
      else
        outstream.write(map[molid]);
    }
    outstream.writeln();
  }
}

// Print a descriptive usage statement
void print_usage(File outstream)
{
  outstream.writeln("\nSMR: SAM mapped reads\n\n"
"The input to SMR is 1 or more SAM files. The output is a table (1 column for\n"
"each input file) showing the number of reads that map to each sequence.\n\n"
"Usage: smr [options] sample-1.sam sample-2.sam ... sample-n.sam\n"
"  Options:\n"
"    -d|--delim: CHAR      delimiter for output data; default is comma\n"
"    -h|--help             print this help message and exit\n"
"    -o|--outfile: FILE    name of file to which read counts will be\n"
"                          written; default is terminal (stdout)\n");
}


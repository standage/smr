## SMR: SAM mapped reads

Copyright (c) 2013, Daniel S. Standage <daniel.standage@gmail.com>

The input to SMR is 1 or more SAM files. The output is a table (1 column for each input file) showing the number of reads that map to each sequence.

Building SMR requires only a C compiler. If you have GNU make installed, just type ``make`` to compile SMR. If not, look at the Makefile for the compilation command.

Once SMR is compiled, run ``./smr -h`` or just ``./smr`` for a usage statement.

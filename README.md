# SDM_bzip2
Parallelized bzip2 using SDM: the SDM execution model features speculative pipelining with three stages: Scanner, Decompressor, and Merger.

1. Installation

make

2. Configuration parameters for SDM execution are defined in papl.conf file.

vi papl.conf

example:
num_process 4   // Number of decompressor processes
chunk_size 1    // Chunk size in independently decompressible units
merge_method 0  // Commit mode: 0 = CENT (centralized), 1 = DIST (distributed)

3. parallelized bzip2

vi papl.h

#define papltime // print execution time
#define papl_par // if you want to execute original version, comment out this

4. SDM API

The SDM API functions are classified into two categories as summarized in papl_api.c file. The first category includes functions already implemented by SDM to initiate and orchestrate pipeline execution. The second category defines interface functions that encapsulate algorithm-specific prediction and decompression codes and must be implemented by the programmer.

5. Publication

[LCTES '13] Hakbeom Jang, Channoh Kim, and Jae W. Lee, "Practical Speculative Parallelization of Variable-Length Decompression Algorithms", 14th ACM SIGPLAN/SIGBED International Conference on Languages, Compilers, and Tools for Embedded Systems (LCTES), Seattle, Washington, June 2013. (Also appeared in ACM SIGPLAN Notices, 48(5), May 2013.)

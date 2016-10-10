## **SDM_bzip2**

SDM_bzip2 is a parallel implementation of the bzip2 decompressor that uses pattern matching based prediction (called magic header: 0x314159265359)  algorithm and achieves speedups of **2.50x** and **8.53×** on 4-core and 36-core embedded platforms, respectively. 

## **Download**

Click to download the latest version.

## **Building, configuaring and Running this program**

* ### **Build**

	$ cd SDM_bzip2/

	$ make

* ### Configuration parameters for SDM execution are defined in papl.conf file.

	$ vim papl.conf

	num_process 4   // Number of decompressor processes
	
	chunk_size 1    // Chunk size in independently decompressible units
	
	merge_method 0  // Commit mode: 0 = CENT (centralized), 1 = DIST (distributed)

* ### **Run**

	$ ./bzip2 -d target_file.bz2

## **Benchmark results**
The following benchmarks were performed using Samsung Exynos 4412 quad-core platform (representing “fat” cores) and Tilera TILE-Gx8036 36-core platform (representing “thin” manycores). Three inputs taken from public domains are used for each program (Firefox 1.7.13, Linux kernel 3.6.1, and SPEC2000 ref input).

* [Benchmark results](https://github.com/skku-swpc/SDM_bzip2/blob/master/benchmark_results.pdf)

## **Documentation**

* [SDM API documentation](https://github.com/skku-swpc/SDM_bzip2/blob/master/SDM-API.docx)

## **Contact**

Hakbeom Jang (<hakbeom@.skku.edu>), Channoh Kim (<channoh@skku.edu>) and Jae W. Lee (<jaewlee@snu.ac.kr>)
* [LCTES '13] Hakbeom Jang, Channoh Kim, and Jae W. Lee, "[practical speculative parallelization of variable-length decompression algorithms](http://dl.acm.org/citation.cfm?id=2465557)", 14th ACM SIGPLAN/SIGBED International Conference on L nguages, Compilers, and Tools for Embedded Systems (LCTES), Seattle, Washington, June 2013. (Also appeared in ACM SIGPLAN Notices, 48(5), May 2013.)

Architecture and Code Optimization Lab, Seoul National University

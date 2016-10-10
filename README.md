## **SDM library**

The SDM presents novel algorithms to efficiently predict block boundaries and a runtime system that enables efficient block-level parallel decompression. The SDM execution model features speculative pipelining with three stages: Scanner, Decompressor, and Merger.

## **Linking library**

Copy the libsdm.so.1.0.1 to your target_app directory

	$ cd libsdm/

	$ cp libsdm.so.1.0.1 target_app/

	$ export LD_LIBRARY_PATH=`echo $LD_LIBRARY_PATH`:`pwd`

	$ ln -s libsdm.so.1.0.1 libsdm.so.1

	$ gcc -o target_app libsdm.so.1


## **Example: SDM_bzip2**

SDM_bzip2 is a parallel implementation of the bzip2 decompressor that uses pattern matching based prediction (called magic header: 0x314159265359)  algorithm.

### **Building and Running this example**

* #### **Build**

	$ cd bzip2/

	$ make

* #### **Run**

	$ ./bzip2 -d target_file.bz2

* #### [Benchmark results](https://github.com/skku-swpc/SDM_bzip2/blob/master/benchmark_results.pdf)
The following benchmarks were performed using Samsung Exynos 4412 quad-core platform (representing “fat” cores) and Tilera TILE-Gx8036 36-core platform (representing “thin” manycores). Three inputs taken from public domains are used for each program (Firefox 1.7.13, Linux kernel 3.6.1, and SPEC2000 ref input).

## **Documentation**

* [SDM API documentation](https://github.com/skku-swpc/SDM_bzip2/blob/master/SDM-API.docx)

## **Contact**

Hakbeom Jang (<hakbeom@.skku.edu>), Channoh Kim (<channoh@skku.edu>) and Jae W. Lee (<jaewlee@snu.ac.kr>)
* [LCTES '13] Hakbeom Jang, Channoh Kim, and Jae W. Lee, "[practical speculative parallelization of variable-length decompression algorithms](http://dl.acm.org/citation.cfm?id=2465557)", 14th ACM SIGPLAN/SIGBED International Conference on L nguages, Compilers, and Tools for Embedded Systems (LCTES), Seattle, Washington, June 2013. (Also appeared in ACM SIGPLAN Notices, 48(5), May 2013.)

Architecture and Code Optimization Lab, Seoul National University

# Micro-benchmarks

From [this Thesis on micro-benchmarking memory systems](https://pdfs.semanticscholar.org/1d32/09cc498254eac8fc1fea0afd8a4d285b0be9.pdf):
memory performance can be characterized in terms of thefollowing parameters:
1. back-to-back latency: the time to service a cache miss, assuming that the instruc-tions before and after the miss are also cache misses. This performance parameter includesthe cache fill penalty, either because the processor is unable to restart upon receiving thecritical word or because of conflict with the previous miss at the cache. Back-to-backlatency is measured in time units, typically in nanoseconds (ns).
2. restart latency: the time to receive the critical word, assuming all previous cachemisses have completed and consequently no interference with other misses in the systemsis observed. Restart latency is measured in time units, typically in nanoseconds (ns).
3. pipelined bandwidth: the sustained rate at which a single processor can issuerequests to the memory system and place the memory data into its caches. Pipelined bandwidth is typically measured in megabytes per second (MB/s).

- "micro benchmarks in this chapter measure the perfor-mance of memory reads only. Since most processors do write allocations, no micro bench-marks for measuring write performance are necessary, given that write performance is similar to read performance
  - is the above statement still valid?

## Types of test
### Cause multiple consecutive cache misses
Implementation:
- Read/writes with increasing strides

What does this answer?
- Are there optimizations for excessive amounts of consecutive misses?
- Are there optimizations for misses that come in a pattern. E.g., H M M M H M M M H ...

### Non-overlapped cache-misses
Implementation:
- Linked-list pointer chasing: An array is initialized with pointer values at a given stride, and the time to traverse the listis then measured. By making all the loads dependent upon the value from the previousload, the cache misses cannot be overlapped at the memory

What does this answer?
- Provides non-pipelined reference stream

### Coherence
The micro benchmarks measure the performance of a "master" processor M which executes a set of memory operations. The other processors in the system, which we call P1, P2, etc. are used to establish the cache coherency state of the data accessed by M. Threads are created and placed on the various processors using library thread routines.To initialize the dirty state:
  - P1 simply writes the data to be accessed by M.
To initialize the clean exclusive state:
  - P writes the data to be accessed by M
  - Then P1 writes an array of size equal to the secondary cache size. This second opera-tion moves the data out to memory1.
  - Subsequently, Pl reads the data and hence establishes the clean exclusive state.
Finally, to initialize the shared state:
  - 1. A direct-mapped cache or a least-recently-used (LRU) cache replacement algorithm is assumed
  - P1 first writes M's data.
  - Subsequently, P2, P3, etc. read it. This sets the state to shared.
Depending on the virtual-to-physical address mapping scheme, different arrayaddresses may be mapped onto the same cache location. By removing sections of the datafrom the cache which the micro benchmark assumes to be present in the cache, this limitsthe ability of the micro-benchmark to measure the memory performance under the desired cache coherency conditions. To counteract this problem, the initialization routine is modi-fied to request pages from the operating system which are large enough as to hold the entire experimental data. On systems where this is not possible, depending on the virtual-to-physical address mapping scheme, the user might have to reduce the size of the array if intended to be placed in the cache in its entirety. Moreover, the cache coverage aimed at removing data from the cache is to be done using arrays of sizes much greater than thecache size. Care must be taken in reducing the array size as not to increase timing sensitivity to system load.

# References
- [Thesis on micro-benchmarking memory systems](https://pdfs.semanticscholar.org/1d32/09cc498254eac8fc1fea0afd8a4d285b0be9.pdf):

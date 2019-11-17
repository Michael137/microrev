# Reverse Engineering μ-architectures
## Cache coherence protocols
- [Question about Intel's cache coherence](https://software.intel.com/en-us/forums/intel-moderncode-for-parallel-architectures/topic/777852)
- [RSP](https://www.raspberrypi.org/products/raspberry-pi-4-model-b/specifications/) runs on a ARM Cortex-A72
  - According to ARM's [infocenter](http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.100095_0003_06_en/Chunk905102933.html) this chip uses the MESI protocol for its L1 cache
- More information on [Intel's cache hierarchy](https://stackoverflow.com/questions/28891349/how-are-the-modern-intel-cpu-l3-caches-organized)
- Cache coherence [microbenchmarks](https://pdfs.semanticscholar.org/1d32/09cc498254eac8fc1fea0afd8a4d285b0be9.pdf)
- Cache coherence [implementation notes](http://lastweek.io/notes/cache_coherence/)
- Cache coherence protocols [evaluation](https://dl.acm.org/citation.cfm?id=6514)
- Info on [ARM Cortex Coherence](https://www.blackhat.com/docs/eu-16/materials/eu-16-Lipp-ARMageddon-How-Your-Smartphone-CPU-Breaks-Software-Level-Security-And-Privacy-wp.pdf)
- [Microbenchmarking GPU Memory architecture](https://arxiv.org/pdf/1509.02308.pdf)
- [Demystifying GPU Memory Hierarchy](https://ieeexplore.ieee.org/abstract/document/5452013/)
- [Intel’s CLDEMOTE instruction](https://sites.utexas.edu/jdm4372/category/computer-hardware/cache-coherence-implementations/)
- [Intel's HitME cache](https://patents.google.com/patent/US8631210)

### Milestones
- [x] (31st October): Choose range of platforms to verify end system on
    * (1 ARM, 2 Intel):
- [x] (31st October): Choose main investigative architecture (RSP, BBB, Intel Core-i5)
    * Personal Linux laptop: Intel(R) Core(TM) i5-2520M CPU @ 2.50GHz
    * Condor server: 24 Core Intel(R) Xeon(R) Platinum 8275CL CPU @ 3.00GHz
      * [Architecture overview](https://software.intel.com/en-us/articles/intel-xeon-processor-scalable-family-technical-overview)
    * [RSP Model 4B](https://www.raspberrypi.org/products/raspberry-pi-4-model-b/specifications/)
       * [Quad core Cortex-A72 (ARM v8) 64-bit SoC @ 1.5GHz](http://infocenter.arm.com/help/topic/com.arm.doc.100095_0003_06_en/cortex_a72_mpcore_trm_100095_0003_06_en.pdf)
       * [Some slides on RSP cache](https://cseweb.ucsd.edu/classes/wi17/cse237A-a/handouts/03.mem.pdf)
- [x] (12th November): Devise which μ-architectural features we want to extract
  - [Intel's Smart Cache](https://software.intel.com/en-us/articles/software-techniques-for-shared-cache-multi-core-systems/?wapkw=smart+cache) outlines useful cache design choices
  - State machine: does cache behaviour conform to MESIF/MESI? How can the benchmark be useful while assuming an incorrect protocol?
  - Replacement policies: LRU? Custom?
  - How does the coherence protocol change with numbers of cores? (4 on RPI vs. 22 on server)
  - How does the coherence protocol change with OS? (Linux vs. FreeBSD)
  - Implementation: Directory-based, snooping, invalidation-based, update-based, write-allocation, bus-system, inclusion policy, lock-up free etc.
  - Smart cache:
    - "Intel Smart Cache also features a new power-saving mechanism that enables the L2 Intel Smart Cache to dynamically flush its ways into system memory, based on demand, or during periods of inactivity"
      - Can we test this?
  - Are there any optimizations for common cache access patterns, e.g., false sharing, stampede, deadlocks, data contention etc.
  - Are there optimizations for excessive amounts of consecutive misses?
  - Are there optimizations for misses that come in a pattern. E.g., H M M M H M M M H ...
- [x] (12th November): Devise which microbenchmarks could uncover above features
- [x] (12th November): Write initial benchmarks and write measurement/benchmark infrastructure (likely using perf)
- [ ] (17th November):
  - Able to set coherence state programmatically
  - Benchmark suite for profiling coherence information
  
# Notes
- Can we take inspiration from Fuzzers? E.g., [CSmith](https://github.com/csmith-project/csmith)
- Can we take insipration from multi-core litmus tests? E.g., [Herd](https://github.com/herd/herdtools/)

# Tools
- [Emon](https://software.intel.com/sites/default/files/emon_user_guide_2019u3.pdf)
- [Herd](https://github.com/herd/herdtools/)

## Branch predictors
- [ARM Cortex M3 Pipeline Forum Question](https://community.arm.com/developer/ip-products/processors/f/cortex-m-forum/3190/cortex-m3-pipeline-stages-branch-prediction)
- [Arduino ARM Cortex-A8](https://www.arduino.cc/en/Main/ArduinoBoardTre)
- [ARM Cortex A8 Branch Predictor Infocenter](http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0337e/CACDJFCF.html#)
- Possible cheap Intel boards:
  - [Arduino Intel Galileo Gen2](https://www.arduino.cc/en/ArduinoCertified/IntelGalileoGen2)
  - [Arduino Intel Edison](https://www.arduino.cc/en/ArduinoCertified/IntelEdison)
    - Intel Atom's CPU (used in Intel Edison) does have a branch predictor as detailed in this [manual](https://www.intel.com/content/dam/www/public/us/en/documents/manuals/64-ia-32-architectures-optimization-manual.pdf)
    - Alternatively, could use the Intel Xeon Platinum processor as on the condor server because of its lack of documentation
    - Possibly could microbenchmark generation framework similar to [this](https://ieeexplore.ieee.org/abstract/document/7818338/)

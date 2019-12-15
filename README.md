# Reverse Engineering μ-architectures
See [INSTALL.md](INSTALL.md) for installation instructions

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
- [x] (17th November):
  - Able to set coherence state programmatically
  - Benchmark suite for profiling coherence information
  
# Notes
- Can we take inspiration from Fuzzers? E.g., [CSmith](https://github.com/csmith-project/csmith)
- Can we take insipration from multi-core litmus tests? E.g., [Herd](https://github.com/herd/herdtools/)

# CPU Info
## Server
```
$> lscpu

Architecture:        x86_64
CPU op-mode(s):      32-bit, 64-bit
Byte Order:          Little Endian
CPU(s):              96
On-line CPU(s) list: 0-95
Thread(s) per core:  2
Core(s) per socket:  24
Socket(s):           2
NUMA node(s):        2
Vendor ID:           GenuineIntel
CPU family:          6
Model:               85
Model name:          Intel(R) Xeon(R) Platinum 8275CL CPU @ 3.00GHz
Stepping:            7
CPU MHz:             1200.021
CPU max MHz:         3900.0000
CPU min MHz:         1200.0000
BogoMIPS:            6000.00
Virtualization:      VT-x
L1d cache:           32K
L1i cache:           32K
L2 cache:            1024K
L3 cache:            36608K
NUMA node0 CPU(s):   0-23,48-71
NUMA node1 CPU(s):   24-47,72-95
Flags:               fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush dts acpi mmx fxsr sse sse2 ss ht tm pbe syscall nx pdpe1gb rdtscp lm constant_tsc art arch_perfmon pebs bts rep_good nopl xtopology nonstop_tsc cpuid aperfmperf pni pclmulqdq dtes64 monitor ds_cpl vmx smx est tm2 ssse3 sdbg fma cx16 xtpr pdcm pcid dca sse4_1 sse4_2 x2apic movbe popcnt tsc_deadline_timer aes xsave avx f16c rdrand lahf_lm abm 3dnowprefetch cpuid_fault epb cat_l3 cdp_l3 invpcid_single ssbd mba ibrs ibpb stibp ibrs_enhanced tpr_shadow vnmi flexpriority ept vpid fsgsbase tsc_adjust bmi1 hle avx2 smep bmi2 erms invpcid rtm cqm mpx rdt_a avx512f avx512dq rdseed adx smap clflushopt clwb intel_pt avx512cd avx512bw avx512vl xsaveopt xsavec xgetbv1 xsaves cqm_llc cqm_occup_llc cqm_mbm_total cqm_mbm_local dtherm ida arat pln pts hwp hwp_act_window hwp_epp hwp_pkg_req pku ospke avx512_vnni md_clear flush_l1d arch_capabilities
```

### Flag Index
Legend can be found [here](https://unix.stackexchange.com/questions/43539/what-do-the-flags-in-proc-cpuinfo-mean)
- clflush: supports cache line flush instruction
- dca: direct cache access
- cat_l3: Cache Allocation Technology L3
- cdp_l3: Code and Data Prioritization L3
- cqm: Cache QoS Monitoring
- cqm_llc: LLC QoS
- cqm_occup_llc: LLC occupancy monitoring
- cqm_mbm_total: LLC total MBM monitoring
- cqm_mbm_local: LLC local MBM monitoring

### Implications
This processor is based on the [Cascade Lake](https://en.wikichip.org/wiki/intel/xeon_platinum#8200-Series_.28Cascade_Lake.29) micro-architecture (see [manual]()).

According to the IA64 optimization manual section 7.3.3.3 Cascade Lake processor with 2 sockets contains **two CL advanced performance packages where each package is made of two processor dies connected via a Intel Ultra Path Interconnect creating four NUMA domains**

This architecture has a larger than usual L2 and L3 cache and its L1 cache is split into data and instruction cache of 32KB each. The processor has 2 sockets with 24 cores each. In the Intel Xeon architecture this implies that 


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

# Reverse Engineering μ-architectures
## Cache coherence protocols
- [Question about Intel's cache coherence](https://software.intel.com/en-us/forums/intel-moderncode-for-parallel-architectures/topic/777852)
- [RSP](https://www.raspberrypi.org/products/raspberry-pi-4-model-b/specifications/) runs on a ARM Cortex-A72
  - According to ARM's [infocenter](http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.100095_0003_06_en/Chunk905102933.html) this chip uses the MESI protocol for its L1 cache
- More information on [Intel's cache hierarchy](https://stackoverflow.com/questions/28891349/how-are-the-modern-intel-cpu-l3-caches-organized)
- Cache coherence [microbenchmarks](https://pdfs.semanticscholar.org/1d32/09cc498254eac8fc1fea0afd8a4d285b0be9.pdf)

### Milestones
- [ ] (31st October): Choose range of platforms to verify end system on (1 ARM, 2 Intel)
- [ ] (31st October): Choose main investigative architecture (RSP, BBB, Intel Core-i5)
- [ ] (31st October): Devise which μ-architectural features we want to extract
  - Replacement policies, protocol type, etc.
- [ ] (31st October): Devise which microbenchmarks could uncover above features
- [ ] (2nd November): Write initial benchmarks and write measurement/benchmark infrastructure (likely using perf)

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

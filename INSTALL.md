# Requirements
- CMake 3.2 or above
- PAPI 5.7 or below

# Installation
## Full build
From the project's top-level directory run:
`make full`

Note: this will remove the `build` and `install` directories and rebuild the whole project

## Partial build
From the `build` directory run:
`make -j4`

To install the compiled changes run following from the `build` directory:
`make install`

Note: This will move all the headers, libraries and executables to the `install` directory

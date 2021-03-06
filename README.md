# MD5 hash bruteforce using MPI

> **PLEASE REMEMBER:**  export/import and/or use of strong cryptography software,
providing cryptography hooks, or even just communicating technical details about cryptography software **is ILLEGAL in some parts of the world**.

## About the project

### Stack of technologies

 - _"C"_ language
 - _"MPI"_ language
 - **OpenSSL** - MD5 cryptographic function implementation
 - **OpenMPI** - MPI application compilation and runtime

### Main goal

Project was aimed at getting an _excellent mark_ on my coursework for _"Parallel Computing Technologies"_ class in _[university](https://https://sibsutis.ru/)_.

Project **doesn't solve** any more problems: it **doesn't show** any MD5 specific vulnerabilities and works **the slowest** among the other MD5 hash collision seeking methods
(rainbow tables, chosen-prefix collision attack, etc.).

**BUT** this implementation works the fastest among the other MD5 bruteforcing methods tested on universitie's _["JET"](https://cpct.sibsutis.ru/index.php/Main/Jet)_ computing cluster.

> _P.S._ The teacher didn't show much emotion during the program showcasing,
but as soon as he listed through my coursework documentation the desired mark was given right away! X)

> _P.P.S._ I think it was due to this coursework being presented as late as at the very day of exam :D :D :D (not my proudest moment)

## Brief description of project's main concepts

### [MD5](https://en.wikipedia.org/wiki/MD5)
The **MD5 *message-digest*** algorithm designed by Ronald Rivest in 1991 is a widely used hash function producing a **128-bit hash value**. 
Although MD5 was initially designed to be used as a cryptographic hash function, it has been found to **suffer from extensive vulnerabilities**. 

It can still be used as a checksum to verify data integrity, but only against unintentional corruption.
It remains suitable for other non-cryptographic purposes, for example for determining the partition for a particular key in a partitioned database.


### [MPI](https://en.wikipedia.org/wiki/Message_Passing_Interface)
MPI _(Message Passing Interface)_ is a portable message-passing **standard** describing data transfers between multiple processes in OS. 
Designed to achieve parallel computing on a wide variety of architectures.
Available implementations are written in languages: _C, C++, Fortran_.

Parallel computing with processes (unlike threads) focuses on computations in **isolated memory** of each process and **allows for multiple nodes** in an architecture.

### ***
# Binaries compilation

The project includes code for _serial_ execution version as well as _parallel_ version for comparsion in execution time between the two.

But it's not all there's to it - a whole bunch of user's specific ***compilation flags and variables*** is included!

Located in the file ***"/source/libs/compilation_info.h"*** rests the most interesting project functionality:
- Enable/disable _Greedy_ search algorythm (exit on first collision)
- Set variables to control syncronisation for greedy algorythm (avoiding _"MPI_Abort()"_)
- Enable dynamic array allocation (heap) instead of preset size static ones (stack) 
- Set variables controlling the static version array sizes
- Enable/disable statistics on permutations (before initiating search) that are about to be performed on each process 
- Enable/disable various time measurements (benchmark) or disable all at once for maximum performance
- Set a list of processors to be included in time statistics (first + amount)
- Enable/disable separate output in wide table format (convenient for pasting into "MS Excel"-like software for analysis).

The repository version of the _"compilation_info.h"_ is preset to include all the time benchmark and permutation statistics as well as dynamic array allocation. 

_Parallel_ version needs to be compiled using ***"mpicc"*** compiler. It is included in MPI libraries sush as ***"MPICH"*** and ***"OpenMPI"*** which are free.

_Serial_ version can be compiled by any C/C++ compiler that has access to _"math"_ and _"openssl"_ libraries.

***OpenSSL*** is a free cryptographic library used in this project, but it is not present dy default in many operating systems and needs to be installed manually.

## Installing compilation dependancies:

### On UNIX-like systems:

**Install OpenSSL and OpenMPI development libraries using apt-get (requires root access):**

`sudo apt-get install libssl-dev`

`sudo apt-get install libopenmpi-dev`

### On WSL (Windows Subsystem Linux):

Install OpenSSL and OpenMPI as shown above, they work pretty well on WSL too.

Executing MPI program on WSL can result in an **error** when requesting CMA (Contiguous Memory Allocator) support due to ***restrictive ptrace settings***. To disable restrictive settings you need (requires root access):

`sudo echo 0 > /proc/sys/kernel/yama/ptrace_scope`

### On Windows OS (tutorials):

**1. Download precompiled OpenSSL:**

https://www.npcglib.org/~stathis/blog/precompiled-openssl/

extract and keep folder in C drive use readme_precompile.txt for instructions.

**2. Link OpenSSL to VS C++:**

Open Visual C++ project and followup procedure given in below to include and Linker options.

Make sure the following settings are setup in the project property pages:

- [C/C++ -> General -> Additional Include Directories]
  - OpenSSL’s include directory in your machine (e.g C:\openssl\include) or (e.g C:\openssl\include64)
- [Linker -> General -> Additional Library Directories]
  - OpenSSL’s lib directory in your machine (e.g C:\openssl\lib) or (e.g C:\openssl\lib64)
- [Linker -> Input -> Additional Dependencies]
  - ws2_32.lib
  - libsslMT.lib
  - Crypt32.lib
  - libcryptoMT.lib
  
**3. Install MPI:**

https://docs.microsoft.com/en-us/message-passing-interface/microsoft-mpi?redirectedfrom=MSDN

**4. Compile with MPI:**

https://docs.microsoft.com/en-us/archive/blogs/windowshpc/how-to-compile-and-run-a-simple-ms-mpi-program

## Compilation commands:

**Compile _serial_ code version (gcc example):**

`gcc -Wall -o bin/md5single source/main/md5_breaker_serial.c -lcrypto -lm`

**Compile _parallel_ code version:**

`mpicc -Wall -o bin/md5parallel source/main/md5_breaker_parallel.c -lcrypto -lm`


# Binaries execution

## Arguments

Binaries execution commands require several arguments in order:
- **MD5 hash to bruteforce: [string of 32 characters]** 
  - > **Errors** if length != 32
- **Alphabet: [ [string,[range of characters],... ] ]** 
  - Programm takes any string, unfolds all *ranges* including negative ones, then removes all duplicates and sorts in alphabetical order
  -  *Ranges* follow the pattern *"`x-y,`"* where *`x`* and *`y`* are single letters that create a range of any symbols (reverse order too) and "`,`" + "`-`" are used for indentation and are ommitted.
  - In ranges "`,`" at the end of argument is not required and if present is not counted as a symbol.
  -  So, for example: *`Sa-sS`* results to *`-asS`* and *`A-S,s`* results to *`sABC..QRS`*
  - > **Watch out** when trying to include "`,`" and "`-`" as letters in the alphabet.
- **Word length(s): [ [maximal number] / [range of numbers] ]**
  - Ranges work in both directions
  - > **Errors** if number(s) is/are negative

For parallel version there is optional last argument:
- **Range of processes (ranks) that display individual times: [range of numbers]** 
  - Ranges work in both directions
  - "-1" is a special value to display all individual times
  - Omit to not display times for individual processes

At execution the application lets you know if one of your argument was invalid and which was it.

## Launch commands

Launch _Serial_:

`./bin/md5single [args]`

Launch _Parallel_:

`mpiexec -np [number of processes] ./bin/md5parallel [args]`

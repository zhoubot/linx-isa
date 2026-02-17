# TSVC 2

Updated version of TSVC to capture the same information but hopefully in a better format to make it easier to add things. We've fixed a few bugs along the way too.

The original paper which laid out the details of the suite and provided results (D. Callahan, J. Dongarra, and D. Levine. Vectorizing compilers: a test suite and results. Proceedings. Supercomputing ’88) originally had the loops in Fortran.

The C version of the benchmark used as a base for TSVC-2 was found [here](http://polaris.cs.uiuc.edu/~maleki1/TSVC.tar.gz). The old C version had some problems to do with compilers being able to inline all of the initialisation and checksum loops, which wildly skews the results and the timing. There are also some bugs with writing off the end of arrays and some of the initialisation routines seem to not initialise the right arrays. 

This version should (hopefully) be free(er) of bugs and should stop the compiler from being able to completely eliminate code.

## Compiling

There are a few supplied makefiles for each compiler so just typing `make` along with specifying the compiler should work. The flags in these makefiles are generally used flags such as `-O3`, with some compilers having a few extra flags which help a lot with vectorisation.

For example, with the Cray compiler:

    make COMPILER=cray

This will create a folder called `bin/cray/` with 3 vectorised and non vectorised versions of the program.

+ `tsvc_[no]vec_relaxed` - 'relaxed' math flags. This specifies that the compiler can make any kind of floating point optimisations it thinks will speed it up
+ `tsvc_[no]vec_precise` - 'precise' math flags. This specifies that some optimisations such as reordering operations that may affect the result are not allowed.
+ `tsvc_[no]vec_default` - 'default' math flags. This is the compiler default.

Some compilers (GNU, clang) do not have a 'precise' flag, and the PGI compiler will ignore the IEEE math flag with vectorisation enabled.

*NB - none of the supplied makefiles have any architecture-specific flags and you will need to edit the makefiles to supply them*. For example, you might want to change the GNU makefile to:

    ARCH=-march=core-avx2

    CC=gcc -std=c99 $(ARCH)
    CXX=g++ $(ARCH)
    FC=gfortran $(ARCH)
    ...

Link time optimisation flags are also not enabled as a necessity for this benchmark.

Fortran compilers and some Fortran specific flags, as well as flags to enable or disable OpenMP for each compiler are provided with the hope that these makefiles would be useful for other purposes.

If you find any issues or have any requests, please let us know!

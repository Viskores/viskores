# Sped up compilation of ArrayRangeCompute.cxx

The file `ArrayRangeCompute.cxx` was taking a long time to compile with
some device compilers. This is because it precompiles the range computation
for many types of array structures. It thus compiled the same operation
many times over.

The new implementation compiles just as many cases. However, the
compilation is split into many different translation units using the
instantiations feature of VTK-m's configuration. Although this rarely
reduces the overall CPU time spent during compiling, it prevents parallel
compiles from waiting for this one build to complete. It also avoids
potential issues with compilers running out of resources as it tries to
build a monolithic file.

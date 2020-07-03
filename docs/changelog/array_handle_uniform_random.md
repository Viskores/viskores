# Implemented ArrayHandleRandomUniformBits and ArrayHandleRandomUniformReal

ArrayHandleRandomUniformBits and ArrayHandleRandomUniformReal were added to provide
an efficient way to generate pseudo random numbers in parallel. They are based on the
Philox parallel pseudo random number generator. ArrayHandleRandomUniformBits provides
64-bits random bits in the whole range of UInt64 as its content while
ArrayHandleRandomUniformReal provides random Float64 in the range of [0, 1). User can
either provide a seed in the form of Vec<vtkm::Uint32, 1> or use the default random
source provided by the C++ standard library. Both of the ArrayHandles  are lazy evaluated
as other Fancy ArrayHandles such that they only have O(1) memory overhead. They are
stateless and functional and does not change once constructed. To generate a new set of
random numbers, for example as part of a iterative algorithm, a  new ArrayHandle
needs to be constructed in each iteration. See the user's guide for more detail and
examples.

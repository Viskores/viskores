##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================


set(headers
  BranchDecompositionBlock.h
  SelectTopVolumeBranchesBlock.h
  ExtractTopVolumeContoursBlock.h
  ComputeBlockIndices.h
  ComputeDistributedBranchDecompositionFunctor.h
  UpdateParentBranchFunctor.h
  SelectTopVolumeBranchesFunctor.h
  ExchangeBranchEndsFunctor.h
  )
#-----------------------------------------------------------------------------

# Note: The C++ source files BranchDecompositionBlock.cxx and
# ComputeDistributedBranchDecompositionFunctor.cxx are added to the scalar
# topology library in the CMakeLists.txt in our parent directory.

viskores_declare_headers(${headers})

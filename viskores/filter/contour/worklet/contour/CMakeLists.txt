##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================


set(headers
  CommonState.h
  FieldPropagation.h
  FlyingEdges.h
  FlyingEdgesHelpers.h
  FlyingEdgesPass1.h
  FlyingEdgesPass2.h
  FlyingEdgesPass4.h
  FlyingEdgesPass4Common.h
  FlyingEdgesPass4X.h
  FlyingEdgesPass4XWithNormals.h
  FlyingEdgesPass4Y.h
  FlyingEdgesTables.h
  MarchingCells.h
  MarchingCellTables.h
  )

#-----------------------------------------------------------------------------
viskores_declare_headers(${headers})

viskores_pyexpander_generated_file(MarchingCellTables.h)

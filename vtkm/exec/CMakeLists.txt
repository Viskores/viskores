##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

set(headers
  AtomicArrayExecutionObject.h
  BoundaryState.h
  CellDerivative.h
  CellEdge.h
  CellFace.h
  CellInside.h
  CellInterpolate.h
  CellLocatorBoundingIntervalHierarchy.h
  CellLocatorMultiplexer.h
  CellLocatorRectilinearGrid.h
  CellLocatorTwoLevel.h
  CellLocatorUniformGrid.h
  CellMeasure.h
  ColorTable.h
  ConnectivityExplicit.h
  ConnectivityExtrude.h
  ConnectivityPermuted.h
  ConnectivityStructured.h
  ExecutionWholeArray.h
  FieldNeighborhood.h
  FunctorBase.h
  ParametricCoordinates.h
  PointLocatorSparseGrid.h
  TaskBase.h
  )

set(header_impls
  ColorTable.hxx
  )

if (NOT VTKm_NO_DEPRECATED_VIRTUAL)
  set(headers ${headers}
    CellLocator.h
    PointLocator.h
    )
endif()

#-----------------------------------------------------------------------------
add_subdirectory(internal)
add_subdirectory(arg)

vtkm_declare_headers(${headers}
                    ${header_impls}
                    )


#-----------------------------------------------------------------------------
add_subdirectory(serial)
add_subdirectory(tbb)
add_subdirectory(openmp)
add_subdirectory(cuda)
add_subdirectory(kokkos)

#-----------------------------------------------------------------------------
if (VTKm_ENABLE_TESTING)
  add_subdirectory(testing)
endif ()

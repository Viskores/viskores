//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#ifndef vtk_m_worklet_IsosurfaceUniformGrid_h
#define vtk_m_worklet_IsosurfaceUniformGrid_h

#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/DynamicArrayHandle.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/Pair.h>

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/Field.h>
#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/WorkletMapTopology.h>
#include <vtkm/VectorAnalysis.h>

#include "MarchingCubesDataTables.h"

typedef VTKM_DEFAULT_DEVICE_ADAPTER_TAG DeviceAdapter;

namespace vtkm {
namespace worklet {

namespace internal {

/// \brief Computes number of vertices for Marching Cubes case
class ClassifyCell : public vtkm::worklet::WorkletMapTopology
{
public:
  typedef void ControlSignature(FieldInFrom<Scalar> inNodes,
                                TopologyIn topology,
                                FieldOut<IdType> outNumVertices);
  typedef void ExecutionSignature(_1, _3);
  typedef _2 InputDomain;

  typedef vtkm::cont::ArrayHandle<vtkm::Id> IdArrayHandle;
  typedef IdArrayHandle::ExecutionTypes<DeviceAdapter>::PortalConst IdPortalType;
  IdPortalType vertexTable;
  vtkm::Float32 isovalue;

  VTKM_CONT_EXPORT
  ClassifyCell(IdPortalType vertexTable, float isovalue) :
               vertexTable(vertexTable),
               isovalue(isovalue) {};

  template<typename InPointVecType>
  VTKM_EXEC_EXPORT
  void operator()(const InPointVecType &pointValues,
                  vtkm::Id& numVertices) const
  {
    vtkm::Id caseNumber  = (pointValues[0] > isovalue);
    caseNumber += (pointValues[1] > isovalue)*2;
    caseNumber += (pointValues[3] > isovalue)*4;
    caseNumber += (pointValues[2] > isovalue)*8;
    caseNumber += (pointValues[4] > isovalue)*16;
    caseNumber += (pointValues[5] > isovalue)*32;
    caseNumber += (pointValues[7] > isovalue)*64;
    caseNumber += (pointValues[6] > isovalue)*128;
    numVertices = vertexTable.Get(caseNumber) / 3;
  }
};


/// \brief Compute isosurface vertices and scalars
template <typename FieldType>
class IsosurfaceSingleTri : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn<IdType> inputCellId,
                                FieldIn<IdType> inputIteration);
  typedef void ExecutionSignature(WorkIndex, _1, _2);
  typedef _1 InputDomain;

  typedef typename vtkm::cont::ArrayHandle<FieldType>::template ExecutionTypes<DeviceAdapter>::PortalConst FieldPortalType;
  FieldPortalType field, source;

  typedef typename vtkm::cont::ArrayHandle<FieldType>::template ExecutionTypes<DeviceAdapter>::Portal OutputPortalType;
  OutputPortalType scalars;

  typedef typename vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3> >::template ExecutionTypes<DeviceAdapter>::Portal VectorPortalType;
  VectorPortalType vertices;

  typedef typename vtkm::cont::ArrayHandle<vtkm::Id> IdArrayHandle;
  typedef typename IdArrayHandle::ExecutionTypes<DeviceAdapter>::PortalConst IdPortalType;
  IdPortalType triTable;

  const int xdim, ydim, zdim, cellsPerLayer, pointsPerLayer;
  const float isovalue, xmin, ymin, zmin, xmax, ymax, zmax;

  template<typename U, typename W, typename X>
  VTKM_CONT_EXPORT
  IsosurfaceSingleTri(const float isovalue, const int dims[3], IdPortalType triTable,
                      const U & field, const U & source, const W & vertices, const X & scalars) :
                      isovalue(isovalue), xdim(dims[0]), ydim(dims[1]), zdim(dims[2]),
                      xmin(-1), ymin(-1), zmin(-1), xmax(1), ymax(1), zmax(1),
                      triTable(triTable),
                      field( field.PrepareForInput( DeviceAdapter() ) ),
                      source( source.PrepareForInput( DeviceAdapter() ) ),
                      vertices(vertices), scalars(scalars),
                      cellsPerLayer((xdim-1) * (ydim-1)), pointsPerLayer (xdim*ydim) { }

  VTKM_EXEC_EXPORT
  void operator()(vtkm::Id outputCellId, vtkm::Id inputCellId, vtkm::Id inputLowerBounds) const
  {
    // Get data for this cell
    const int verticesForEdge[] = { 0, 1, 1, 2, 3, 2, 0, 3,
                                    4, 5, 5, 6, 7, 6, 4, 7,
                                    0, 4, 1, 5, 2, 6, 3, 7 };

    const int x = inputCellId % (xdim - 1);
    const int y = (inputCellId / (xdim - 1)) % (ydim -1);
    const int z = inputCellId / cellsPerLayer;

    // Compute indices for the eight vertices of this cell
    const int i0 = x    + y*xdim + z * pointsPerLayer;
    const int i1 = i0   + 1;
    const int i2 = i0   + 1 + xdim;
    const int i3 = i0   + xdim;
    const int i4 = i0   + pointsPerLayer;
    const int i5 = i1   + pointsPerLayer;
    const int i6 = i2   + pointsPerLayer;
    const int i7 = i3   + pointsPerLayer;

    // Get the field values at these eight vertices
    float f[8];
    f[0] = this->field.Get(i0);
    f[1] = this->field.Get(i1);
    f[2] = this->field.Get(i2);
    f[3] = this->field.Get(i3);
    f[4] = this->field.Get(i4);
    f[5] = this->field.Get(i5);
    f[6] = this->field.Get(i6);
    f[7] = this->field.Get(i7);

    // Compute the Marching Cubes case number for this cell
    unsigned int cubeindex = 0;
    cubeindex += (f[0] > isovalue);
    cubeindex += (f[1] > isovalue)*2;
    cubeindex += (f[2] > isovalue)*4;
    cubeindex += (f[3] > isovalue)*8;
    cubeindex += (f[4] > isovalue)*16;
    cubeindex += (f[5] > isovalue)*32;
    cubeindex += (f[6] > isovalue)*64;
    cubeindex += (f[7] > isovalue)*128;

    // Compute the coordinates of the uniform regular grid at each of the cell's eight vertices
    vtkm::Vec<FieldType, 3> p[8];
    {
      // If we have offset and spacing, can we simplify this computation
      vtkm::Vec<FieldType, 3> offset = vtkm::make_Vec(xmin+(xmax-xmin),
                                                      ymin+(ymax-ymin),
                                                      zmin+(zmax-zmin) );

      vtkm::Vec<FieldType, 3> spacing = vtkm::make_Vec( 1.0 /(xdim-1),
                                                        1.0 /(ydim-1),
                                                        1.0 /(zdim-1));

      vtkm::Vec<FieldType, 3> firstPoint = offset * spacing *  vtkm::make_Vec( x, y, z );
      vtkm::Vec<FieldType, 3> secondPoint = offset * spacing * vtkm::make_Vec( x+1, y+1, z+1 );

      p[0] = vtkm::make_Vec( firstPoint[0],   firstPoint[1],   firstPoint[2]);
      p[1] = vtkm::make_Vec( secondPoint[0],  firstPoint[1],   firstPoint[2]);
      p[2] = vtkm::make_Vec( secondPoint[0],  secondPoint[1],  firstPoint[2]);
      p[3] = vtkm::make_Vec( firstPoint[0],   secondPoint[1],  firstPoint[2]);
      p[4] = vtkm::make_Vec( firstPoint[0],   firstPoint[1],   secondPoint[2]);
      p[5] = vtkm::make_Vec( secondPoint[0],  firstPoint[1],   secondPoint[2]);
      p[6] = vtkm::make_Vec( secondPoint[0],  secondPoint[1],  secondPoint[2]);
      p[7] = vtkm::make_Vec( firstPoint[0],   secondPoint[1],  secondPoint[2]);
    }

    // Get the scalar source values at the eight vertices
    float s[8];
    s[0] = this->source.Get(i0);
    s[1] = this->source.Get(i1);
    s[2] = this->source.Get(i2);
    s[3] = this->source.Get(i3);
    s[4] = this->source.Get(i4);
    s[5] = this->source.Get(i5);
    s[6] = this->source.Get(i6);
    s[7] = this->source.Get(i7);

    // Interpolate for vertex positions and associated scalar values
    const vtkm::Id inputIteration = (outputCellId - inputLowerBounds);
    const vtkm::Id outputVertId = outputCellId * 3;
    const vtkm::Id cellOffset = cubeindex*16 + (inputIteration * 3);
    for (int v = 0; v < 3; v++)
    {
      const int edge = triTable.Get(cellOffset + v);
      const int v0   = verticesForEdge[2*edge];
      const int v1   = verticesForEdge[2*edge + 1];
      const float t  = (isovalue - f[v0]) / (f[v1] - f[v0]);

      this->vertices.Set(outputVertId + v, vtkm::Lerp(p[v0], p[v1], t));
      this->scalars.Set(outputVertId + v, vtkm::Lerp(s[v0], s[v1], t));
    }
  }
};
}


/// \brief Compute the isosurface for a uniform grid data set
template <typename FieldType>
class IsosurfaceFilterUniformGrid
{
public:
  IsosurfaceFilterUniformGrid(const vtkm::Id &dim, const vtkm::cont::DataSet &dataSet) : dim(dim), dataSet(dataSet) { };

  vtkm::Id dim;
  vtkm::cont::DataSet dataSet;

  void computeIsosurface(const float &isovalue, vtkm::cont::ArrayHandle<vtkm::Vec<FieldType,3> > &verticesArray, vtkm::cont::ArrayHandle<FieldType> &scalarsArray)
  {
    int vdim = dim + 1;  int dim3 = dim*dim*dim;
    int vdims[3] = { vdim, vdim, vdim };
    
    // Set up the Marching Cubes case tables
    vtkm::cont::ArrayHandle<vtkm::Id> vertexTableArray = vtkm::cont::make_ArrayHandle(vtkm::worklet::internal::numVerticesTable, 256);
    vtkm::cont::ArrayHandle<vtkm::Id> triangleTableArray = vtkm::cont::make_ArrayHandle(vtkm::worklet::internal::triTable, 256*16);
    
    // Call the ClassifyCell functor to compute the Marching Cubes case numbers for each cell, and the number of vertices to be generated
    vtkm::cont::Field numVerticesField("numverts", 1, vtkm::cont::Field::ASSOC_CELL_SET, std::string("cells"), vtkm::Id());
    dataSet.AddField(numVerticesField);
    vtkm::worklet::internal::ClassifyCell classifyCellFunctor(vertexTableArray.PrepareForInput(DeviceAdapter()), isovalue);
    vtkm::worklet::DispatcherMapTopology<vtkm::worklet::internal::ClassifyCell>(classifyCellFunctor).Invoke(
                   dataSet.GetField("nodevar").GetData(), dataSet.GetCellSet(0), dataSet.GetField("numverts").GetData()); 
    vtkm::cont::ArrayHandle<vtkm::Id> numOutputTrisPerCell;
    numOutputTrisPerCell = dataSet.GetField("numverts").GetData().CastToArrayHandle(vtkm::Id(), VTKM_DEFAULT_STORAGE_TAG());
    
    // Compute the number of valid input cells and those ids
    const vtkm::Id numOutputCells = vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::ScanInclusive(numOutputTrisPerCell, numOutputTrisPerCell);

    // Terminate if no cells has triangles left
    if (numOutputCells == 0) return;

    vtkm::cont::ArrayHandle<vtkm::Id> validCellIndicesArray, inputCellIterationNumber;
    vtkm::cont::ArrayHandleCounting<vtkm::Id> validCellCountImplicitArray(0, numOutputCells);
    vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::UpperBounds(numOutputTrisPerCell, validCellCountImplicitArray, validCellIndicesArray);
    numOutputTrisPerCell.ReleaseResources();

    // Compute for each output triangle what iteration of the input cell generates it
    vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::LowerBounds(validCellIndicesArray, validCellIndicesArray, inputCellIterationNumber);

    // Generate a single triangle per cell
    const vtkm::Id numTotalVertices = numOutputCells * 3;
    typedef vtkm::worklet::internal::IsosurfaceSingleTri<FieldType> IsoSurfaceFunctor;
    vtkm::cont::ArrayHandle<FieldType> field;
    field = dataSet.GetField("nodevar").GetData().CastToArrayHandle(vtkm::Float32(), VTKM_DEFAULT_STORAGE_TAG());
    IsoSurfaceFunctor isosurface(isovalue, vdims, triangleTableArray.PrepareForInput(DeviceAdapter()), field, field,
                                 verticesArray.PrepareForOutput(numTotalVertices, DeviceAdapter()),
                                 scalarsArray.PrepareForOutput(numTotalVertices, DeviceAdapter()));
    vtkm::worklet::DispatcherMapField< IsoSurfaceFunctor > isosurfaceDispatcher(isosurface);
    isosurfaceDispatcher.Invoke(validCellIndicesArray, inputCellIterationNumber);
  }
};

}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_IsosurfaceUniformGrid_h

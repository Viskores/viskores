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

namespace vtkm {
namespace worklet {

/// \brief Compute the isosurface for a uniform grid data set
template <typename FieldType, typename DeviceAdapter>
class IsosurfaceFilterUniformGrid
{
public:

  class ClassifyCell : public vtkm::worklet::WorkletMapTopologyPointToCell
  {
  public:
    typedef void ControlSignature(FieldInFrom<Scalar> inNodes,
                                  TopologyIn topology,
                                  FieldOut<IdType> outNumVertices);
    typedef void ExecutionSignature(_1, _3);
    typedef _2 InputDomain;

    typedef vtkm::cont::ArrayHandle<vtkm::Id> IdArrayHandle;
    typedef typename IdArrayHandle::ExecutionTypes<DeviceAdapter>::PortalConst IdPortalType;
    IdPortalType VertexTable;
    FieldType Isovalue;

    VTKM_CONT_EXPORT
    ClassifyCell(IdPortalType vTable, FieldType isovalue) :
      VertexTable(vTable),
      Isovalue(isovalue)
    {
    }

    template<typename InPointVecType>
    VTKM_EXEC_EXPORT
    void operator()(const InPointVecType &pointValues,
                    vtkm::Id& numVertices) const
    {
      vtkm::Id caseNumber  = (pointValues[0] > this->Isovalue);
      caseNumber += (pointValues[1] > this->Isovalue)*2;
      caseNumber += (pointValues[2] > this->Isovalue)*4;
      caseNumber += (pointValues[3] > this->Isovalue)*8;
      caseNumber += (pointValues[4] > this->Isovalue)*16;
      caseNumber += (pointValues[5] > this->Isovalue)*32;
      caseNumber += (pointValues[6] > this->Isovalue)*64;
      caseNumber += (pointValues[7] > this->Isovalue)*128;
      numVertices = this->VertexTable.Get(caseNumber) / 3;
    }
  };

  /// \brief Compute isosurface vertices and scalars
  class IsoSurfaceGenerate : public vtkm::worklet::WorkletMapField
  {
  public:
    typedef void ControlSignature(FieldIn<IdType> inputCellId,
                                  FieldIn<IdType> inputIteration);
    typedef void ExecutionSignature(WorkIndex, _1, _2);
    typedef _1 InputDomain;

    const FieldType Isovalue;
    vtkm::Id xdim, ydim, zdim;
    const float xmin, ymin, zmin, xmax, ymax, zmax;

    typedef typename vtkm::cont::ArrayHandle<FieldType>::template ExecutionTypes<DeviceAdapter>::PortalConst FieldPortalType;
    FieldPortalType Field, Source;

    typedef typename vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3> >::template ExecutionTypes<DeviceAdapter>::Portal VectorPortalType;
    VectorPortalType Vertices;
    VectorPortalType Normals;

    typedef typename vtkm::cont::ArrayHandle<FieldType>::template ExecutionTypes<DeviceAdapter>::Portal OutputPortalType;
    OutputPortalType Scalars;

    typedef typename vtkm::cont::ArrayHandle<vtkm::Id> IdArrayHandle;
    typedef typename IdArrayHandle::ExecutionTypes<DeviceAdapter>::PortalConst IdPortalType;
    IdPortalType TriTable;

    const vtkm::Id cellsPerLayer, pointsPerLayer;

    template<typename U, typename W, typename X>
    VTKM_CONT_EXPORT
    IsoSurfaceGenerate(FieldType ivalue, const vtkm::Id3 cdims, IdPortalType triTablePortal,
                       const U & field, const U & source, const W & vertices, const W & normals, const X & scalars) :
      Isovalue(ivalue),
      xdim(cdims[0]), ydim(cdims[1]), zdim(cdims[2]),
      xmin(-1), ymin(-1), zmin(-1), xmax(1), ymax(1), zmax(1),
      Field( field.PrepareForInput( DeviceAdapter() ) ),
      Source( source.PrepareForInput( DeviceAdapter() ) ),
      Vertices(vertices),
      Normals(normals),
      Scalars(scalars),
      TriTable(triTablePortal),
      cellsPerLayer(xdim * ydim),
      pointsPerLayer ((xdim+1)*(ydim+1))
    {
    }

    VTKM_EXEC_EXPORT
    void operator()(vtkm::Id outputCellId, vtkm::Id inputCellId, vtkm::Id inputLowerBounds) const
    {
      // Get data for this cell
      const int verticesForEdge[] = { 0, 1, 1, 2, 3, 2, 0, 3,
                                      4, 5, 5, 6, 7, 6, 4, 7,
                                      0, 4, 1, 5, 2, 6, 3, 7 };

      const vtkm::Id x = inputCellId % xdim;
      const vtkm::Id y = (inputCellId / xdim) % ydim;
      const vtkm::Id z = inputCellId / cellsPerLayer;

      // Compute indices for the eight vertices of this cell
      const vtkm::Id i0 = x    + y*(xdim+1) + z * pointsPerLayer;
      const vtkm::Id i1 = i0   + 1;
      const vtkm::Id i2 = i0   + 1 + (xdim + 1); //xdim is cell dim
      const vtkm::Id i3 = i0   + (xdim + 1);     //xdim is cell dim
      const vtkm::Id i4 = i0   + pointsPerLayer;
      const vtkm::Id i5 = i1   + pointsPerLayer;
      const vtkm::Id i6 = i2   + pointsPerLayer;
      const vtkm::Id i7 = i3   + pointsPerLayer;

      // Get the field values at these eight vertices
      FieldType f[8];
      f[0] = this->Field.Get(i0);
      f[1] = this->Field.Get(i1);
      f[2] = this->Field.Get(i2);
      f[3] = this->Field.Get(i3);
      f[4] = this->Field.Get(i4);
      f[5] = this->Field.Get(i5);
      f[6] = this->Field.Get(i6);
      f[7] = this->Field.Get(i7);

      // Compute the Marching Cubes case number for this cell
      unsigned int cubeindex = 0;
      cubeindex += (f[0] > this->Isovalue);
      cubeindex += (f[1] > this->Isovalue)*2;
      cubeindex += (f[2] > this->Isovalue)*4;
      cubeindex += (f[3] > this->Isovalue)*8;
      cubeindex += (f[4] > this->Isovalue)*16;
      cubeindex += (f[5] > this->Isovalue)*32;
      cubeindex += (f[6] > this->Isovalue)*64;
      cubeindex += (f[7] > this->Isovalue)*128;
      // printf("inputCellId: %i \n",inputCellId);
      // printf("x: %i, y: %i, z: %i \n",x, y, z);
      // printf("i0: %i \n",i0);
      // printf("f0 %F\n", f[0]);
      // printf("cubeindex: %i \n",cubeindex);
      // printf("numCells: %i \n",(vtkm::worklet::internal::numVerticesTable[cubeindex]/3) );

      // Compute the coordinates of the uniform regular grid at each of the cell's eight vertices
      vtkm::Vec<FieldType, 3> p[8];

      // If we have offset and spacing, can we simplify this computation
      {
        vtkm::Vec<FieldType, 3> offset = vtkm::make_Vec(xmin+(xmax-xmin),
                                                        ymin+(ymax-ymin),
                                                        zmin+(zmax-zmin) );

        vtkm::Vec<FieldType, 3> spacing = vtkm::make_Vec( 1.0f /((float)(xdim-1)),
                                                          1.0f /((float)(ydim-1)),
                                                          1.0f /((float)(zdim-1)));

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
      FieldType s[8];
      s[0] = this->Source.Get(i0);
      s[1] = this->Source.Get(i1);
      s[2] = this->Source.Get(i2);
      s[3] = this->Source.Get(i3);
      s[4] = this->Source.Get(i4);
      s[5] = this->Source.Get(i5);
      s[6] = this->Source.Get(i6);
      s[7] = this->Source.Get(i7);

      // Interpolate for vertex positions and associated scalar values
      const vtkm::Id inputIteration = (outputCellId - inputLowerBounds);
      const vtkm::Id outputVertId = outputCellId * 3;
      const vtkm::Id cellOffset = static_cast<vtkm::Id>(cubeindex*16) + (inputIteration * 3);
      for (int v = 0; v < 3; v++)
      {
        const vtkm::Id edge = this->TriTable.Get(cellOffset + v);
        const int v0   = verticesForEdge[2*edge];
        const int v1   = verticesForEdge[2*edge + 1];
        const FieldType t  = (this->Isovalue - f[v0]) / (f[v1] - f[v0]);
        this->Vertices.Set(outputVertId + v, vtkm::Lerp(p[v0], p[v1], t));
        this->Scalars.Set(outputVertId + v, vtkm::Lerp(s[v0], s[v1], t));
      }

      vtkm::Vec<FieldType, 3> vertex0 = this->Vertices.Get(outputVertId + 0);
      vtkm::Vec<FieldType, 3> vertex1 = this->Vertices.Get(outputVertId + 1);
      vtkm::Vec<FieldType, 3> vertex2 = this->Vertices.Get(outputVertId + 2);

      vtkm::Vec<FieldType, 3> curNorm = vtkm::Cross(vertex1-vertex0, vertex2-vertex0);
      vtkm::Normalize(curNorm);
      this->Normals.Set(outputVertId + 0, curNorm);
      this->Normals.Set(outputVertId + 1, curNorm);
      this->Normals.Set(outputVertId + 2, curNorm);
    }
  };


  IsosurfaceFilterUniformGrid(const vtkm::Id3 &dims,
                              const vtkm::cont::DataSet &dataSet) :
    CDims(dims),
    DataSet(dataSet)
  {
  }

  vtkm::Id3 CDims;
  vtkm::cont::DataSet DataSet;

  template<typename CoordinateType>
  void Run(const float &isovalue,
           const vtkm::cont::DynamicArrayHandle& isoField,
           vtkm::cont::ArrayHandle< vtkm::Vec<CoordinateType,3> > verticesArray,
           vtkm::cont::ArrayHandle< vtkm::Vec<CoordinateType,3> > normalsArray,
           vtkm::cont::ArrayHandle<FieldType> scalarsArray)
  {
    //todo this needs to change so that we don't presume the storage type
    vtkm::cont::ArrayHandle<FieldType> field;
    field = isoField.CastToArrayHandle(FieldType(), VTKM_DEFAULT_STORAGE_TAG());
    this->Run(isovalue, field, verticesArray, normalsArray, scalarsArray);

  }

  template<typename StorageTag, typename CoordinateType>
  void Run(const float &isovalue,
           const vtkm::cont::ArrayHandle<FieldType, StorageTag>& field,
           vtkm::cont::ArrayHandle< vtkm::Vec<CoordinateType,3> > verticesArray,
           vtkm::cont::ArrayHandle< vtkm::Vec<CoordinateType,3> > normalsArray,
           vtkm::cont::ArrayHandle<FieldType> scalarsArray)
  {
    typedef typename vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter> DeviceAlgorithms;

    // Set up the Marching Cubes case tables
    vtkm::cont::ArrayHandle<vtkm::Id> vertexTableArray =
        vtkm::cont::make_ArrayHandle(vtkm::worklet::internal::numVerticesTable,
                                     256);
    vtkm::cont::ArrayHandle<vtkm::Id> triangleTableArray =
        vtkm::cont::make_ArrayHandle(vtkm::worklet::internal::triTable,
                                     256*16);

    // Call the ClassifyCell functor to compute the Marching Cubes case numbers
    // for each cell, and the number of vertices to be generated
    ClassifyCell classifyCell(vertexTableArray.PrepareForInput(DeviceAdapter()),
                              isovalue);

    typedef typename vtkm::worklet::DispatcherMapTopology<
                                      ClassifyCell,
                                      DeviceAdapter> ClassifyCellDispatcher;
    ClassifyCellDispatcher classifyCellDispatcher(classifyCell);

    vtkm::cont::ArrayHandle<vtkm::Id> numOutputTrisPerCell;
    classifyCellDispatcher.Invoke( field,
                                   this->DataSet.GetCellSet(0),
                                   numOutputTrisPerCell);

    // Compute the number of valid input cells and those ids
    const vtkm::Id numOutputCells = DeviceAlgorithms::ScanInclusive(numOutputTrisPerCell,
                                                                    numOutputTrisPerCell);

    // Terminate if no cells has triangles left
    if (numOutputCells == 0) return;

    vtkm::cont::ArrayHandle<vtkm::Id> validCellIndicesArray, inputCellIterationNumber;
    vtkm::cont::ArrayHandleCounting<vtkm::Id> validCellCountImplicitArray(0, numOutputCells);
    DeviceAlgorithms::UpperBounds(numOutputTrisPerCell,
                                  validCellCountImplicitArray,
                                  validCellIndicesArray);
    numOutputTrisPerCell.ReleaseResources();

    // Compute for each output triangle what iteration of the input cell generates it
    DeviceAlgorithms::LowerBounds(validCellIndicesArray,
                                  validCellIndicesArray,
                                  inputCellIterationNumber);

    // Generate a single triangle per cell
    const vtkm::Id numTotalVertices = numOutputCells * 3;

    IsoSurfaceGenerate isosurface(isovalue,
                                  this->CDims,
                                  triangleTableArray.PrepareForInput(DeviceAdapter()),
                                  field,
                                  field,
                                  verticesArray.PrepareForOutput(numTotalVertices, DeviceAdapter()),
                                  normalsArray.PrepareForOutput(numTotalVertices, DeviceAdapter()),
                                  scalarsArray.PrepareForOutput(numTotalVertices, DeviceAdapter())
                                  );

    typedef typename vtkm::worklet::DispatcherMapField< IsoSurfaceGenerate,
                                                        DeviceAdapter> IsoSurfaceDispatcher;
    IsoSurfaceDispatcher isosurfaceDispatcher(isosurface);
    isosurfaceDispatcher.Invoke(validCellIndicesArray,
                                inputCellIterationNumber);
  }
};

}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_IsosurfaceUniformGrid_h

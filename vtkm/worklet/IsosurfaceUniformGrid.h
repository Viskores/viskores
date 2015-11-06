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

#include <vtkm/Pair.h>
#include <vtkm/VectorAnalysis.h>

#include <vtkm/exec/CellDerivative.h>
#include <vtkm/exec/ExecutionWholeArray.h>
#include <vtkm/exec/ParametricCoordinates.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleGroupVec.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/DynamicArrayHandle.h>
#include <vtkm/cont/Field.h>

#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/ScatterCounting.h>
#include <vtkm/worklet/WorkletMapTopology.h>


#include "MarchingCubesDataTables.h"

namespace vtkm {
namespace worklet {

/// \brief Compute the isosurface for a uniform grid data set
template <typename FieldType, typename DeviceAdapter>
class IsosurfaceFilterUniformGrid
{
public:

  class ClassifyCell : public vtkm::worklet::WorkletMapPointToCell
  {
  public:
    typedef void ControlSignature(FieldInPoint<Scalar> inNodes,
                                  TopologyIn topology,
                                  FieldOutCell<> outNumVertices,
                                  ExecObject numVerticesTable);
    typedef void ExecutionSignature(_1, _3, _4);
    typedef _2 InputDomain;

    FieldType Isovalue;

    VTKM_CONT_EXPORT
    ClassifyCell(FieldType isovalue) :
      Isovalue(isovalue)
    {
    }

    template<typename InPointVecType,
             typename NumVerticesTablePortalType>
    VTKM_EXEC_EXPORT
    void operator()(const InPointVecType &pointValues,
                    vtkm::IdComponent &numVertices,
                    const NumVerticesTablePortalType &numVerticesTable) const
    {
      vtkm::Id caseNumber  = (pointValues[0] > this->Isovalue);
      caseNumber += (pointValues[1] > this->Isovalue)*2;
      caseNumber += (pointValues[2] > this->Isovalue)*4;
      caseNumber += (pointValues[3] > this->Isovalue)*8;
      caseNumber += (pointValues[4] > this->Isovalue)*16;
      caseNumber += (pointValues[5] > this->Isovalue)*32;
      caseNumber += (pointValues[6] > this->Isovalue)*64;
      caseNumber += (pointValues[7] > this->Isovalue)*128;
      numVertices = numVerticesTable.Get(caseNumber) / 3;
    }
  };

  /// \brief Compute isosurface vertices and scalars
  class IsoSurfaceGenerate : public vtkm::worklet::WorkletMapPointToCell
  {
  public:
    typedef void ControlSignature(
        TopologyIn topology, // Cell set
        FieldInPoint<> fieldIn, // Input point field defining the contour
        FieldInPoint<Vec3> pcoordIn, // Input point coordinates
        FieldOutCell<> vertexOut, // Vertices for output triangles
        // TODO: Have a better way to iterate over and interpolate fields
        FieldInPoint<Scalar> scalarsIn, // Scalars to interpolate
        FieldOutCell<> scalarsOut, // Interpolated scalars (one per tri vertex)
        FieldOutCell<> normalsOut, // Estimated normals (one per tri vertex)
        ExecObject TriTable // An array portal with the triangle table
        );
    typedef void ExecutionSignature(
        CellShape, _2, _3, _4, _5, _6, _7, _8, VisitIndex);

    typedef vtkm::worklet::ScatterCounting ScatterType;
    VTKM_CONT_EXPORT
    ScatterType GetScatter() const
    {
      return this->Scatter;
    }

    template<typename CountArrayType, typename Device>
    VTKM_CONT_EXPORT
    IsoSurfaceGenerate(FieldType isovalue,
                       const CountArrayType &countArray,
                       Device)
      : Isovalue(isovalue), Scatter(countArray, Device()) {  }

    template<typename CellShapeTag,
             typename FieldInType, // Vec-like, one per input point
             typename CoordType, // Vec-like (one per input point) of Vec-3
             typename VertexOutType, // Vec-3 of Vec-3 coordinates (for triangle)
             typename ScalarInType, // Vec-like, one per input point
             typename ScalarOutType, // Vec-3 (one value per tri vertex)
             typename NormalOutType, // Vec-3 of Vec-3
             typename TriTablePortalType> // Array portal
    VTKM_EXEC_EXPORT
    void operator()(
        CellShapeTag shape,
        const FieldInType &fieldIn, // Input point field defining the contour
        const CoordType &coords, // Input point coordinates
        VertexOutType &vertexOut, // Vertices for output triangles
        // TODO: Have a better way to iterate over and interpolate fields
        const ScalarInType &scalarsIn, // Scalars to interpolate
        ScalarOutType &scalarsOut, // Interpolated scalars (one per tri vertex)
        NormalOutType &normalsOut, // Estimated normals (one per tri vertex)
        const TriTablePortalType &triTable, // An array portal with the triangle table
        vtkm::IdComponent visitIndex
        ) const
    {
      // Get data for this cell
      const vtkm::IdComponent verticesForEdge[] = { 0, 1, 1, 2, 3, 2, 0, 3,
                                                    4, 5, 5, 6, 7, 6, 4, 7,
                                                    0, 4, 1, 5, 2, 6, 3, 7 };

      // Compute the Marching Cubes case number for this cell
      vtkm::IdComponent cubeindex = 0;
      cubeindex += (fieldIn[0] > this->Isovalue);
      cubeindex += (fieldIn[1] > this->Isovalue)*2;
      cubeindex += (fieldIn[2] > this->Isovalue)*4;
      cubeindex += (fieldIn[3] > this->Isovalue)*8;
      cubeindex += (fieldIn[4] > this->Isovalue)*16;
      cubeindex += (fieldIn[5] > this->Isovalue)*32;
      cubeindex += (fieldIn[6] > this->Isovalue)*64;
      cubeindex += (fieldIn[7] > this->Isovalue)*128;

      // Interpolate for vertex positions and associated scalar values
      const vtkm::Id triTableOffset =
          static_cast<vtkm::Id>(cubeindex*16 + visitIndex*3);
      for (vtkm::IdComponent triVertex = 0;
           triVertex < 3;
           triVertex++)
      {
        const vtkm::IdComponent edgeIndex =
            triTable.Get(triTableOffset + triVertex);
        const vtkm::IdComponent edgeVertex0 = verticesForEdge[2*edgeIndex + 0];
        const vtkm::IdComponent edgeVertex1 = verticesForEdge[2*edgeIndex + 1];
        const FieldType fieldValue0 = fieldIn[edgeVertex0];
        const FieldType fieldValue1 = fieldIn[edgeVertex1];
        const FieldType interpolant =
            (this->Isovalue - fieldValue0) / (fieldValue1 - fieldValue0);
        vertexOut[triVertex] = vtkm::Lerp(coords[edgeVertex0],
                                          coords[edgeVertex1],
                                          interpolant);
        scalarsOut[triVertex] = vtkm::Lerp(scalarsIn[edgeVertex0],
                                           scalarsIn[edgeVertex1],
                                           interpolant);
        const vtkm::Vec<vtkm::FloatDefault,3> edgePCoord0 =
            vtkm::exec::ParametricCoordinatesPoint(
              fieldIn.GetNumberOfComponents(), edgeVertex0, shape, *this);
        const vtkm::Vec<vtkm::FloatDefault,3> edgePCoord1 =
            vtkm::exec::ParametricCoordinatesPoint(
              fieldIn.GetNumberOfComponents(), edgeVertex1, shape, *this);
        const vtkm::Vec<vtkm::FloatDefault,3> interpPCoord =
            vtkm::Lerp(edgePCoord0, edgePCoord1, interpolant);
        normalsOut[triVertex] =
            vtkm::Normal(vtkm::exec::CellDerivative(
                           fieldIn, coords, interpPCoord, shape, *this));
      }
    }

  private:
    const FieldType Isovalue;
    ScatterType Scatter;
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
    // Set up the Marching Cubes case tables
    vtkm::cont::ArrayHandle<vtkm::IdComponent> vertexTableArray =
        vtkm::cont::make_ArrayHandle(vtkm::worklet::internal::numVerticesTable,
                                     256);
    vtkm::cont::ArrayHandle<vtkm::IdComponent> triangleTableArray =
        vtkm::cont::make_ArrayHandle(vtkm::worklet::internal::triTable,
                                     256*16);

    typedef vtkm::exec::ExecutionWholeArrayConst<vtkm::IdComponent, VTKM_DEFAULT_STORAGE_TAG, DeviceAdapter>
        TableArrayExecObjectType;

    // Call the ClassifyCell functor to compute the Marching Cubes case numbers
    // for each cell, and the number of vertices to be generated
    // TODO: Make the way we pass vertexTableArray and triangleTableArray
    // conistent.
    ClassifyCell classifyCell(isovalue);

    typedef typename vtkm::worklet::DispatcherMapTopology<
                                      ClassifyCell,
                                      DeviceAdapter> ClassifyCellDispatcher;
    ClassifyCellDispatcher classifyCellDispatcher(classifyCell);

    vtkm::cont::ArrayHandle<vtkm::IdComponent> numOutputTrisPerCell;
    classifyCellDispatcher.Invoke( field,
                                   this->DataSet.GetCellSet(0),
                                   numOutputTrisPerCell,
                                   TableArrayExecObjectType(vertexTableArray));

    IsoSurfaceGenerate isosurface(isovalue, numOutputTrisPerCell, DeviceAdapter());

    vtkm::worklet::DispatcherMapTopology<IsoSurfaceGenerate, DeviceAdapter>
        isosurfaceDispatcher(isosurface);
    isosurfaceDispatcher.Invoke(
          // Currently forcing cell set to be structured. Eventually we should
          // relax this as we support other grid types.
          this->DataSet.GetCellSet(0).ResetCellSetList(
            vtkm::ListTagBase<vtkm::cont::CellSetStructured<3> >()),
          field,
          this->DataSet.GetCoordinateSystem(0).GetData(),
          vtkm::cont::make_ArrayHandleGroupVec<3>(verticesArray),
          field, // This is silly. The field will interp to isovalue
          vtkm::cont::make_ArrayHandleGroupVec<3>(scalarsArray),
          vtkm::cont::make_ArrayHandleGroupVec<3>(normalsArray),
          TableArrayExecObjectType(triangleTableArray)
          );
  }
};

}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_IsosurfaceUniformGrid_h

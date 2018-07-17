//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_rendering_Triangulator_h
#define vtk_m_rendering_Triangulator_h

#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/CellSetPermutation.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/rendering/raytracing/MeshConnectivityBuilder.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/WorkletMapTopology.h>
namespace vtkm
{
namespace rendering
{
/// \brief Triangulator creates a minimal set of triangles from a cell set.
///
///  This class creates a array of triangle indices from both 3D and 2D
///  explicit cell sets. This list can serve as input to opengl and the
///  ray tracer scene renderers. TODO: Add regular grid support
///
template <typename Device>
class Triangulator
{
private:
  using IdArrayHandle = typename vtkm::cont::ArrayHandle<vtkm::Id>;
  using Vec4ArrayHandle = typename vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 4>>;
  using Vec4ArrayPortalType = typename Vec4ArrayHandle::ExecutionTypes<Device>::Portal;
  using IdPortalConstType = typename IdArrayHandle::ExecutionTypes<Device>::PortalConst;

public:
  template <class T>
  class MemSet : public vtkm::worklet::WorkletMapField
  {
    T Value;

  public:
    VTKM_CONT
    MemSet(T value)
      : Value(value)
    {
    }
    using ControlSignature = void(FieldOut<>);
    using ExecutionSignature = void(_1);
    VTKM_EXEC
    void operator()(T& outValue) const { outValue = Value; }
  }; //class MemSet

  class CountTriangles : public vtkm::worklet::WorkletMapPointToCell
  {
  public:
    VTKM_CONT
    CountTriangles() {}
    using ControlSignature = void(CellSetIn cellset, FieldOut<>);
    using ExecutionSignature = void(CellShape, _2);

    VTKM_EXEC
    void operator()(vtkm::CellShapeTagGeneric shapeType, vtkm::Id& triangles) const
    {
      if (shapeType.Id == vtkm::CELL_SHAPE_TRIANGLE)
        triangles = 1;
      else if (shapeType.Id == vtkm::CELL_SHAPE_QUAD)
        triangles = 2;
      else if (shapeType.Id == vtkm::CELL_SHAPE_TETRA)
        triangles = 4;
      else if (shapeType.Id == vtkm::CELL_SHAPE_HEXAHEDRON)
        triangles = 12;
      else if (shapeType.Id == vtkm::CELL_SHAPE_WEDGE)
        triangles = 8;
      else if (shapeType.Id == vtkm::CELL_SHAPE_PYRAMID)
        triangles = 6;
      else
        triangles = 0;
    }

    VTKM_EXEC
    void operator()(vtkm::CellShapeTagHexahedron vtkmNotUsed(shapeType), vtkm::Id& triangles) const
    {
      triangles = 12;
    }

    VTKM_EXEC
    void operator()(vtkm::CellShapeTagQuad vtkmNotUsed(shapeType), vtkm::Id& triangles) const
    {
      triangles = 2;
    }
  }; //class CountTriangles

  template <int DIM>
  class TrianglulateStructured : public vtkm::worklet::WorkletMapPointToCell
  {
  private:
    Vec4ArrayPortalType OutputIndices;

  public:
    using ControlSignature = void(CellSetIn cellset, FieldInTo<>);
    using ExecutionSignature = void(FromIndices, _2);
    //using InputDomain = _1;
    VTKM_CONT
    TrianglulateStructured(vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 4>>& outputIndices)
    {
      this->OutputIndices =
        outputIndices.PrepareForOutput(outputIndices.GetNumberOfValues(), Device());
    }

#if defined(VTKM_MSVC)
#pragma warning(push)
#pragma warning(disable : 4127) //conditional expression is constant
#endif
    //TODO: Remove the if/then with templates.
    template <typename CellNodeVecType>
    VTKM_EXEC void operator()(const CellNodeVecType& cellIndices, const vtkm::Id& cellIndex) const
    {
      vtkm::Vec<vtkm::Id, 4> triangle;
      if (DIM == 2)
      {
        const vtkm::Id triangleOffset = cellIndex * 2;
        // 0-1-2
        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[1];
        triangle[3] = cellIndices[2];
        triangle[0] = cellIndex;
        OutputIndices.Set(triangleOffset, triangle);
        // 0-3-2
        triangle[2] = cellIndices[3];
        OutputIndices.Set(triangleOffset + 1, triangle);
      }
      else if (DIM == 3)
      {
        const vtkm::Id triangleOffset = cellIndex * 12;

        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[1];
        triangle[3] = cellIndices[5];
        triangle[0] = cellIndex;
        OutputIndices.Set(triangleOffset, triangle);

        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[5];
        triangle[3] = cellIndices[4];
        OutputIndices.Set(triangleOffset + 1, triangle);

        triangle[1] = cellIndices[1];
        triangle[2] = cellIndices[2];
        triangle[3] = cellIndices[6];
        OutputIndices.Set(triangleOffset + 2, triangle);

        triangle[1] = cellIndices[1];
        triangle[2] = cellIndices[6];
        triangle[3] = cellIndices[5];
        OutputIndices.Set(triangleOffset + 3, triangle);

        triangle[1] = cellIndices[3];
        triangle[2] = cellIndices[7];
        triangle[3] = cellIndices[6];
        OutputIndices.Set(triangleOffset + 4, triangle);

        triangle[1] = cellIndices[3];
        triangle[2] = cellIndices[6];
        triangle[3] = cellIndices[2];
        OutputIndices.Set(triangleOffset + 5, triangle);

        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[4];
        triangle[3] = cellIndices[7];
        OutputIndices.Set(triangleOffset + 6, triangle);

        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[7];
        triangle[3] = cellIndices[3];
        OutputIndices.Set(triangleOffset + 7, triangle);

        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[3];
        triangle[3] = cellIndices[2];
        OutputIndices.Set(triangleOffset + 8, triangle);

        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[2];
        triangle[3] = cellIndices[1];
        OutputIndices.Set(triangleOffset + 9, triangle);

        triangle[1] = cellIndices[4];
        triangle[2] = cellIndices[5];
        triangle[3] = cellIndices[6];
        OutputIndices.Set(triangleOffset + 10, triangle);

        triangle[1] = cellIndices[4];
        triangle[2] = cellIndices[6];
        triangle[3] = cellIndices[7];
        OutputIndices.Set(triangleOffset + 11, triangle);
      }
    }
#if defined(VTKM_MSVC)
#pragma warning(pop)
#endif
  };


  class IndicesSort : public vtkm::worklet::WorkletMapField
  {
  public:
    VTKM_CONT
    IndicesSort() {}
    using ControlSignature = void(FieldInOut<>);
    using ExecutionSignature = void(_1);
    VTKM_EXEC
    void operator()(vtkm::Vec<vtkm::Id, 4>& triangleIndices) const
    {
      // first field contains the id of the cell the
      // trianlge belongs to
      vtkm::Id temp;
      if (triangleIndices[1] > triangleIndices[3])
      {
        temp = triangleIndices[1];
        triangleIndices[1] = triangleIndices[3];
        triangleIndices[3] = temp;
      }
      if (triangleIndices[1] > triangleIndices[2])
      {
        temp = triangleIndices[1];
        triangleIndices[1] = triangleIndices[2];
        triangleIndices[2] = temp;
      }
      if (triangleIndices[2] > triangleIndices[3])
      {
        temp = triangleIndices[2];
        triangleIndices[2] = triangleIndices[3];
        triangleIndices[3] = temp;
      }
    }
  }; //class IndicesSort

  struct IndicesLessThan
  {
    VTKM_EXEC_CONT
    bool operator()(const vtkm::Vec<vtkm::Id, 4>& a, const vtkm::Vec<vtkm::Id, 4>& b) const
    {
      if (a[1] < b[1])
        return true;
      if (a[1] > b[1])
        return false;
      if (a[2] < b[2])
        return true;
      if (a[2] > b[2])
        return false;
      if (a[3] < b[3])
        return true;
      return false;
    }
  };

  class UniqueTriangles : public vtkm::worklet::WorkletMapField
  {
  public:
    VTKM_CONT
    UniqueTriangles() {}

    using ControlSignature = void(WholeArrayIn<vtkm::ListTagBase<vtkm::Vec<vtkm::Id, 4>>>,
                                  WholeArrayOut<vtkm::ListTagBase<vtkm::UInt8>>);
    using ExecutionSignature = void(_1, _2, WorkIndex);

    VTKM_EXEC
    bool IsTwin(const vtkm::Vec<vtkm::Id, 4>& a, const vtkm::Vec<vtkm::Id, 4>& b) const
    {
      return (a[1] == b[1] && a[2] == b[2] && a[3] == b[3]);
    }

    template <typename IndicesPortalType, typename OutputFlagsPortalType>
    VTKM_EXEC void operator()(const IndicesPortalType& indices,
                              OutputFlagsPortalType& outputFlags,
                              const vtkm::Id& index) const
    {
      if (index == 0)
        return;
      //if we are a shared face, mark ourself and neighbor for destruction
      if (IsTwin(indices.Get(index), indices.Get(index - 1)))
      {
        outputFlags.Set(index, 0);
        outputFlags.Set(index - 1, 0);
      }
    }
  }; //class UniqueTriangles

  class Trianglulate : public vtkm::worklet::WorkletMapPointToCell
  {
  private:
    Vec4ArrayPortalType OutputIndices;

  public:
    VTKM_CONT
    Trianglulate(vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 4>>& outputIndices,
                 const vtkm::Id& size)
    {
      this->OutputIndices = outputIndices.PrepareForOutput(size, Device());
    }
    using ControlSignature = void(CellSetIn cellset, FieldInCell<>);
    using ExecutionSignature = void(_2, CellShape, PointIndices, WorkIndex);

    template <typename VecType>
    VTKM_EXEC void operator()(const vtkm::Id& triangleOffset,
                              vtkm::CellShapeTagQuad vtkmNotUsed(shapeType),
                              const VecType& cellIndices,
                              const vtkm::Id& cellId) const
    {
      vtkm::Vec<vtkm::Id, 4> triangle;


      triangle[1] = cellIndices[0];
      triangle[2] = cellIndices[1];
      triangle[3] = cellIndices[2];
      triangle[0] = cellId;
      OutputIndices.Set(triangleOffset, triangle);

      triangle[2] = cellIndices[3];
      OutputIndices.Set(triangleOffset + 1, triangle);
    }

    template <typename VecType>
    VTKM_EXEC void operator()(const vtkm::Id& triangleOffset,
                              vtkm::CellShapeTagHexahedron vtkmNotUsed(shapeType),
                              const VecType& cellIndices,
                              const vtkm::Id& cellId) const
    {
      vtkm::Vec<vtkm::Id, 4> triangle;

      triangle[1] = cellIndices[0];
      triangle[2] = cellIndices[1];
      triangle[3] = cellIndices[5];
      triangle[0] = cellId;
      OutputIndices.Set(triangleOffset, triangle);

      triangle[1] = cellIndices[0];
      triangle[2] = cellIndices[5];
      triangle[3] = cellIndices[4];
      OutputIndices.Set(triangleOffset + 1, triangle);

      triangle[1] = cellIndices[1];
      triangle[2] = cellIndices[2];
      triangle[3] = cellIndices[6];
      OutputIndices.Set(triangleOffset + 2, triangle);

      triangle[1] = cellIndices[1];
      triangle[2] = cellIndices[6];
      triangle[3] = cellIndices[5];
      OutputIndices.Set(triangleOffset + 3, triangle);

      triangle[1] = cellIndices[3];
      triangle[2] = cellIndices[7];
      triangle[3] = cellIndices[6];
      OutputIndices.Set(triangleOffset + 4, triangle);

      triangle[1] = cellIndices[3];
      triangle[2] = cellIndices[6];
      triangle[3] = cellIndices[2];
      OutputIndices.Set(triangleOffset + 5, triangle);

      triangle[1] = cellIndices[0];
      triangle[2] = cellIndices[4];
      triangle[3] = cellIndices[7];
      OutputIndices.Set(triangleOffset + 6, triangle);

      triangle[1] = cellIndices[0];
      triangle[2] = cellIndices[7];
      triangle[3] = cellIndices[3];
      OutputIndices.Set(triangleOffset + 7, triangle);

      triangle[1] = cellIndices[0];
      triangle[2] = cellIndices[3];
      triangle[3] = cellIndices[2];
      OutputIndices.Set(triangleOffset + 8, triangle);

      triangle[1] = cellIndices[0];
      triangle[2] = cellIndices[2];
      triangle[3] = cellIndices[1];
      OutputIndices.Set(triangleOffset + 9, triangle);

      triangle[1] = cellIndices[4];
      triangle[2] = cellIndices[5];
      triangle[3] = cellIndices[6];
      OutputIndices.Set(triangleOffset + 10, triangle);

      triangle[1] = cellIndices[4];
      triangle[2] = cellIndices[6];
      triangle[3] = cellIndices[7];
      OutputIndices.Set(triangleOffset + 11, triangle);
    }

    template <typename VecType>
    VTKM_EXEC void operator()(const vtkm::Id& triangleOffset,
                              vtkm::CellShapeTagGeneric shapeType,
                              const VecType& cellIndices,
                              const vtkm::Id& cellId) const
    {
      vtkm::Vec<vtkm::Id, 4> triangle;

      if (shapeType.Id == vtkm::CELL_SHAPE_TRIANGLE)
      {

        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[1];
        triangle[3] = cellIndices[2];
        triangle[0] = cellId;
        OutputIndices.Set(triangleOffset, triangle);
      }
      if (shapeType.Id == vtkm::CELL_SHAPE_QUAD)
      {

        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[1];
        triangle[3] = cellIndices[2];
        triangle[0] = cellId;
        OutputIndices.Set(triangleOffset, triangle);

        triangle[2] = cellIndices[3];
        OutputIndices.Set(triangleOffset + 1, triangle);
      }
      if (shapeType.Id == vtkm::CELL_SHAPE_TETRA)
      {
        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[3];
        triangle[3] = cellIndices[1];
        triangle[0] = cellId;
        OutputIndices.Set(triangleOffset, triangle);

        triangle[1] = cellIndices[1];
        triangle[2] = cellIndices[2];
        triangle[3] = cellIndices[3];
        OutputIndices.Set(triangleOffset + 1, triangle);

        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[2];
        triangle[3] = cellIndices[3];
        OutputIndices.Set(triangleOffset + 2, triangle);

        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[2];
        triangle[3] = cellIndices[1];
        OutputIndices.Set(triangleOffset + 3, triangle);
      }
      if (shapeType.Id == vtkm::CELL_SHAPE_HEXAHEDRON)
      {
        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[1];
        triangle[3] = cellIndices[5];
        triangle[0] = cellId;
        OutputIndices.Set(triangleOffset, triangle);

        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[5];
        triangle[3] = cellIndices[4];
        OutputIndices.Set(triangleOffset + 1, triangle);

        triangle[1] = cellIndices[1];
        triangle[2] = cellIndices[2];
        triangle[3] = cellIndices[6];
        OutputIndices.Set(triangleOffset + 2, triangle);

        triangle[1] = cellIndices[1];
        triangle[2] = cellIndices[6];
        triangle[3] = cellIndices[5];
        OutputIndices.Set(triangleOffset + 3, triangle);

        triangle[1] = cellIndices[3];
        triangle[2] = cellIndices[7];
        triangle[3] = cellIndices[6];
        OutputIndices.Set(triangleOffset + 4, triangle);

        triangle[1] = cellIndices[3];
        triangle[2] = cellIndices[6];
        triangle[3] = cellIndices[2];
        OutputIndices.Set(triangleOffset + 5, triangle);

        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[4];
        triangle[3] = cellIndices[7];
        OutputIndices.Set(triangleOffset + 6, triangle);

        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[7];
        triangle[3] = cellIndices[3];
        OutputIndices.Set(triangleOffset + 7, triangle);

        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[3];
        triangle[3] = cellIndices[2];
        OutputIndices.Set(triangleOffset + 8, triangle);

        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[2];
        triangle[3] = cellIndices[1];
        OutputIndices.Set(triangleOffset + 9, triangle);

        triangle[1] = cellIndices[4];
        triangle[2] = cellIndices[5];
        triangle[3] = cellIndices[6];
        OutputIndices.Set(triangleOffset + 10, triangle);

        triangle[1] = cellIndices[4];
        triangle[2] = cellIndices[6];
        triangle[3] = cellIndices[7];
        OutputIndices.Set(triangleOffset + 11, triangle);
      }
      if (shapeType.Id == vtkm::CELL_SHAPE_WEDGE)
      {
        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[1];
        triangle[3] = cellIndices[2];
        triangle[0] = cellId;
        OutputIndices.Set(triangleOffset, triangle);

        triangle[1] = cellIndices[3];
        triangle[2] = cellIndices[5];
        triangle[3] = cellIndices[4];
        OutputIndices.Set(triangleOffset + 1, triangle);

        triangle[1] = cellIndices[3];
        triangle[2] = cellIndices[0];
        triangle[3] = cellIndices[2];
        OutputIndices.Set(triangleOffset + 2, triangle);

        triangle[1] = cellIndices[3];
        triangle[2] = cellIndices[2];
        triangle[3] = cellIndices[5];
        OutputIndices.Set(triangleOffset + 3, triangle);

        triangle[1] = cellIndices[1];
        triangle[2] = cellIndices[4];
        triangle[3] = cellIndices[5];
        OutputIndices.Set(triangleOffset + 4, triangle);

        triangle[1] = cellIndices[1];
        triangle[2] = cellIndices[5];
        triangle[3] = cellIndices[2];
        OutputIndices.Set(triangleOffset + 5, triangle);

        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[3];
        triangle[3] = cellIndices[4];
        OutputIndices.Set(triangleOffset + 6, triangle);

        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[4];
        triangle[3] = cellIndices[1];
        OutputIndices.Set(triangleOffset + 7, triangle);
      }
      if (shapeType.Id == vtkm::CELL_SHAPE_PYRAMID)
      {
        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[4];
        triangle[3] = cellIndices[1];
        triangle[0] = cellId;
        OutputIndices.Set(triangleOffset, triangle);

        triangle[1] = cellIndices[1];
        triangle[2] = cellIndices[2];
        triangle[3] = cellIndices[4];
        OutputIndices.Set(triangleOffset + 1, triangle);

        triangle[1] = cellIndices[2];
        triangle[2] = cellIndices[3];
        triangle[3] = cellIndices[4];
        OutputIndices.Set(triangleOffset + 2, triangle);

        triangle[1] = cellIndices[0];
        triangle[2] = cellIndices[4];
        triangle[3] = cellIndices[3];
        OutputIndices.Set(triangleOffset + 3, triangle);

        triangle[1] = cellIndices[3];
        triangle[2] = cellIndices[2];
        triangle[3] = cellIndices[1];
        OutputIndices.Set(triangleOffset + 4, triangle);

        triangle[1] = cellIndices[3];
        triangle[2] = cellIndices[1];
        triangle[3] = cellIndices[0];
        OutputIndices.Set(triangleOffset + 5, triangle);
      }
    }
  }; //class Trianglulate

public:
  VTKM_CONT
  Triangulator() {}

  VTKM_CONT
  void ExternalTrianlges(vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 4>>& outputIndices,
                         vtkm::Id& outputTriangles)
  {
    //Eliminate unseen triangles
    vtkm::worklet::DispatcherMapField<IndicesSort>().Invoke(outputIndices);
    vtkm::cont::DeviceAdapterAlgorithm<Device>::Sort(outputIndices, IndicesLessThan());
    vtkm::cont::ArrayHandle<vtkm::UInt8> flags;
    flags.Allocate(outputTriangles);
    vtkm::worklet::DispatcherMapField<MemSet<vtkm::UInt8>>(MemSet<vtkm::UInt8>(1)).Invoke(flags);
    //Unique triangles will have a flag = 1
    vtkm::worklet::DispatcherMapField<UniqueTriangles>().Invoke(outputIndices, flags);

    vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 4>> subset;
    vtkm::cont::DeviceAdapterAlgorithm<Device>::CopyIf(outputIndices, flags, subset);
    outputIndices = subset;
    outputTriangles = subset.GetNumberOfValues();
  }

  VTKM_CONT
  void Run(const vtkm::cont::DynamicCellSet& cellset,
           vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 4>>& outputIndices,
           vtkm::Id& outputTriangles)
  {
    bool fastPath = false;
    if (cellset.IsSameType(vtkm::cont::CellSetStructured<3>()))
    {
      vtkm::cont::CellSetStructured<3> cellSetStructured3D =
        cellset.Cast<vtkm::cont::CellSetStructured<3>>();

      raytracing::MeshConnectivityBuilder<Device> builder;
      outputIndices = builder.ExternalTrianglesStructured(cellSetStructured3D);
      outputTriangles = outputIndices.GetNumberOfValues();
      fastPath = true;
    }
    else if (cellset.IsSameType(vtkm::cont::CellSetStructured<2>()))
    {
      vtkm::cont::CellSetStructured<2> cellSetStructured2D =
        cellset.Cast<vtkm::cont::CellSetStructured<2>>();
      const vtkm::Id numCells = cellSetStructured2D.GetNumberOfCells();

      vtkm::cont::ArrayHandleCounting<vtkm::Id> cellIdxs(0, 1, numCells);
      outputIndices.Allocate(numCells * 2);
      vtkm::worklet::DispatcherMapTopology<TrianglulateStructured<2>>(
        TrianglulateStructured<2>(outputIndices))
        .Invoke(cellSetStructured2D, cellIdxs);

      outputTriangles = numCells * 2;
    }
    else
    {
      vtkm::cont::ArrayHandle<vtkm::Id> trianglesPerCell;
      vtkm::worklet::DispatcherMapTopology<CountTriangles>(CountTriangles())
        .Invoke(cellset, trianglesPerCell);

      vtkm::Id totalTriangles = 0;
      totalTriangles =
        vtkm::cont::DeviceAdapterAlgorithm<Device>::Reduce(trianglesPerCell, vtkm::Id(0));

      vtkm::cont::ArrayHandle<vtkm::Id> cellOffsets;
      vtkm::cont::DeviceAdapterAlgorithm<Device>::ScanExclusive(trianglesPerCell, cellOffsets);
      outputIndices.Allocate(totalTriangles);

      vtkm::worklet::DispatcherMapTopology<Trianglulate>(
        Trianglulate(outputIndices, totalTriangles))
        .Invoke(cellset, cellOffsets);

      outputTriangles = totalTriangles;
    }

    //get rid of any triagles we cannot see
    if (!fastPath)
    {
      ExternalTrianlges(outputIndices, outputTriangles);
    }
  }
}; // class Triangulator
}
} //namespace vtkm::rendering
#endif //vtk_m_rendering_Triangulator_h

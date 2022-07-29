//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_worklet_particle_advection_cell_interpolation_helper
#define vtk_m_worklet_particle_advection_cell_interpolation_helper

#include <vtkm/CellShape.h>
#include <vtkm/Types.h>
#include <vtkm/VecVariable.h>

#include <vtkm/cont/ArrayGetValues.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/ExecutionObjectBase.h>
#include <vtkm/cont/UnknownCellSet.h>
#include <vtkm/exec/CellInterpolate.h>

/*
 * Interface to define the helper classes that can return mesh data
 * on a cell by cell basis.
 */
namespace vtkm
{
namespace exec
{

class CellInterpolationHelper
{
private:
  using ShapeType = vtkm::cont::ArrayHandle<vtkm::UInt8>;
  using OffsetType = vtkm::cont::ArrayHandle<vtkm::Id>;
  using ConnType = vtkm::cont::ArrayHandle<vtkm::Id>;
  using ShapePortalType = typename ShapeType::ReadPortalType;
  using OffsetPortalType = typename OffsetType::ReadPortalType;
  using ConnPortalType = typename ConnType::ReadPortalType;

public:
  enum class HelperType
  {
    STRUCTURED,
    EXPSINGLE,
    EXPLICIT
  };

  VTKM_CONT
  CellInterpolationHelper() = default;

  VTKM_CONT
  CellInterpolationHelper(const vtkm::Id3& cellDims, const vtkm::Id3& pointDims, bool is3D)
    : CellDims(cellDims)
    , PointDims(pointDims)
    , Is3D(is3D)
  {
    this->Type = HelperType::STRUCTURED;
  }

  CellInterpolationHelper(const vtkm::UInt8 cellShape,
                          const vtkm::IdComponent pointsPerCell,
                          const ConnType& connectivity,
                          vtkm::cont::DeviceAdapterId device,
                          vtkm::cont::Token& token)
    : CellShape(cellShape)
    , PointsPerCell(pointsPerCell)
    , Connectivity(connectivity.PrepareForInput(device, token))
  {
    this->Type = HelperType::EXPSINGLE;
  }


  VTKM_CONT
  CellInterpolationHelper(const ShapeType& shape,
                          const OffsetType& offset,
                          const ConnType& connectivity,
                          vtkm::cont::DeviceAdapterId device,
                          vtkm::cont::Token& token)
    : Shape(shape.PrepareForInput(device, token))
    , Offset(offset.PrepareForInput(device, token))
    , Connectivity(connectivity.PrepareForInput(device, token))
  {
    this->Type = HelperType::EXPLICIT;
  }

  VTKM_EXEC
  void GetCellInfo(const vtkm::Id& cellId,
                   vtkm::UInt8& cellShape,
                   vtkm::IdComponent& numVerts,
                   vtkm::VecVariable<vtkm::Id, 8>& indices) const
  {
    switch (this->Type)
    {
      case HelperType::STRUCTURED:
      {
        vtkm::Id3 logicalCellId;
        logicalCellId[0] = cellId % this->CellDims[0];
        logicalCellId[1] = (cellId / this->CellDims[0]) % this->CellDims[1];
        if (this->Is3D)
        {
          logicalCellId[2] = cellId / (this->CellDims[0] * this->CellDims[1]);
          indices.Append((logicalCellId[2] * this->PointDims[1] + logicalCellId[1]) *
                           this->PointDims[0] +
                         logicalCellId[0]);
          indices.Append(indices[0] + 1);
          indices.Append(indices[1] + this->PointDims[0]);
          indices.Append(indices[2] - 1);
          indices.Append(indices[0] + this->PointDims[0] * this->PointDims[1]);
          indices.Append(indices[4] + 1);
          indices.Append(indices[5] + this->PointDims[0]);
          indices.Append(indices[6] - 1);
          cellShape = static_cast<vtkm::UInt8>(vtkm::CELL_SHAPE_HEXAHEDRON);
          numVerts = 8;
        }
        else
        {
          indices.Append(logicalCellId[1] * this->PointDims[0] + logicalCellId[0]);
          indices.Append(indices[0] + 1);
          indices.Append(indices[1] + this->PointDims[0]);
          indices.Append(indices[2] - 1);
          cellShape = static_cast<vtkm::UInt8>(vtkm::CELL_SHAPE_QUAD);
          numVerts = 4;
        }
      }
      break;

      case HelperType::EXPSINGLE:
      {
        cellShape = this->CellShape;
        numVerts = this->PointsPerCell;
        vtkm::Id n = static_cast<vtkm::Id>(PointsPerCell);
        vtkm::Id offset = cellId * n;
        for (vtkm::Id i = 0; i < n; i++)
          indices.Append(Connectivity.Get(offset + i));
      }
      break;

      case HelperType::EXPLICIT:
      {
        cellShape = this->Shape.Get(cellId);
        const vtkm::Id offset = this->Offset.Get(cellId);
        numVerts = static_cast<vtkm::IdComponent>(this->Offset.Get(cellId + 1) - offset);
        for (vtkm::IdComponent i = 0; i < numVerts; i++)
          indices.Append(this->Connectivity.Get(offset + i));
      }
      break;

      default:
      {
        // Code path not expected to execute in correct cases
        // Supress unused variable warning
        cellShape = vtkm::UInt8(0);
        numVerts = vtkm::IdComponent(0);
      }
    }
  }

private:
  HelperType Type;
  // variables for structured type
  vtkm::Id3 CellDims;
  vtkm::Id3 PointDims;
  bool Is3D = true;
  // variables for single explicit type
  vtkm::UInt8 CellShape;
  vtkm::IdComponent PointsPerCell;
  // variables for explicit type
  ShapePortalType Shape;
  OffsetPortalType Offset;
  ConnPortalType Connectivity;
};

} // namespace exec

/*
 * Control side base object.
 */
namespace cont
{

class CellInterpolationHelper : public vtkm::cont::ExecutionObjectBase
{
private:
  using ExecutionType = vtkm::exec::CellInterpolationHelper;
  using Structured2DType = vtkm::cont::CellSetStructured<2>;
  using Structured3DType = vtkm::cont::CellSetStructured<3>;
  using SingleExplicitType = vtkm::cont::CellSetSingleType<>;
  using ExplicitType = vtkm::cont::CellSetExplicit<>;

public:
  VTKM_CONT
  CellInterpolationHelper() = default;

  VTKM_CONT
  CellInterpolationHelper(const vtkm::cont::UnknownCellSet& cellSet)
  {
    if (cellSet.CanConvert<Structured2DType>())
    {
      this->Is3D = false;
      vtkm::Id2 cellDims =
        cellSet.AsCellSet<Structured2DType>().GetSchedulingRange(vtkm::TopologyElementTagCell());
      vtkm::Id2 pointDims =
        cellSet.AsCellSet<Structured2DType>().GetSchedulingRange(vtkm::TopologyElementTagPoint());
      this->CellDims = vtkm::Id3(cellDims[0], cellDims[1], 0);
      this->PointDims = vtkm::Id3(pointDims[0], pointDims[1], 1);
      this->Type = vtkm::exec::CellInterpolationHelper::HelperType::STRUCTURED;
    }
    else if (cellSet.CanConvert<Structured3DType>())
    {
      this->Is3D = true;
      this->CellDims =
        cellSet.AsCellSet<Structured3DType>().GetSchedulingRange(vtkm::TopologyElementTagCell());
      this->PointDims =
        cellSet.AsCellSet<Structured3DType>().GetSchedulingRange(vtkm::TopologyElementTagPoint());
      this->Type = vtkm::exec::CellInterpolationHelper::HelperType::STRUCTURED;
    }
    else if (cellSet.CanConvert<SingleExplicitType>())
    {
      SingleExplicitType CellSet = cellSet.AsCellSet<SingleExplicitType>();
      const auto cellShapes =
        CellSet.GetShapesArray(vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint());
      const auto numIndices =
        CellSet.GetNumIndicesArray(vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint());
      CellShape = vtkm::cont::ArrayGetValue(0, cellShapes);
      PointsPerCell = vtkm::cont::ArrayGetValue(0, numIndices);
      Connectivity = CellSet.GetConnectivityArray(vtkm::TopologyElementTagCell(),
                                                  vtkm::TopologyElementTagPoint());
      this->Type = vtkm::exec::CellInterpolationHelper::HelperType::EXPSINGLE;
    }
    else if (cellSet.CanConvert<ExplicitType>())
    {
      vtkm::cont::CellSetExplicit<> CellSet = cellSet.AsCellSet<vtkm::cont::CellSetExplicit<>>();
      Shape =
        CellSet.GetShapesArray(vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint());
      Offset =
        CellSet.GetOffsetsArray(vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint());
      Connectivity = CellSet.GetConnectivityArray(vtkm::TopologyElementTagCell(),
                                                  vtkm::TopologyElementTagPoint());
      this->Type = vtkm::exec::CellInterpolationHelper::HelperType::EXPLICIT;
    }
    else
      throw vtkm::cont::ErrorInternal("Unsupported cellset type");
  }

  VTKM_CONT
  const ExecutionType PrepareForExecution(vtkm::cont::DeviceAdapterId device,
                                          vtkm::cont::Token& token) const
  {
    switch (this->Type)
    {
      case ExecutionType::HelperType::STRUCTURED:
        return ExecutionType(this->CellDims, this->PointDims, this->Is3D);

      case ExecutionType::HelperType::EXPSINGLE:
        return ExecutionType(
          this->CellShape, this->PointsPerCell, this->Connectivity, device, token);

      case ExecutionType::HelperType::EXPLICIT:
        return ExecutionType(this->Shape, this->Offset, this->Connectivity, device, token);
    }
    throw vtkm::cont::ErrorInternal("Undefined case for building cell interpolation helper");
  }

private:
  // Variables required for strucutred grids
  vtkm::Id3 CellDims;
  vtkm::Id3 PointDims;
  bool Is3D = true;
  // variables for single explicit type
  vtkm::UInt8 CellShape;
  vtkm::IdComponent PointsPerCell;
  // Variables required for unstructured grids
  vtkm::cont::ArrayHandle<vtkm::UInt8> Shape;
  vtkm::cont::ArrayHandle<vtkm::Id> Offset;
  vtkm::cont::ArrayHandle<vtkm::Id> Connectivity;
  ExecutionType::HelperType Type;
};

} //namespace cont
} //namespace vtkm

#endif //vtk_m_worklet_particle_advection_cell_interpolation_helper

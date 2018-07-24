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
#ifndef vtk_m_cont_internal_ConnectivityExplicitInternals_h
#define vtk_m_cont_internal_ConnectivityExplicitInternals_h

#include <vtkm/CellShape.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/internal/DeviceAdapterError.h>
#include <vtkm/cont/internal/ReverseConnectivityBuilder.h>
#include <vtkm/exec/ExecutionWholeArray.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

namespace vtkm
{
namespace cont
{
namespace internal
{

template <typename NumIndicesArrayType, typename IndexOffsetArrayType, typename DeviceAdapterTag>
void buildIndexOffsets(const NumIndicesArrayType& numIndices,
                       IndexOffsetArrayType& offsets,
                       DeviceAdapterTag,
                       std::true_type)
{
  //We first need to make sure that NumIndices and IndexOffsetArrayType
  //have the same type so we can call scane exclusive
  using CastedNumIndicesType = vtkm::cont::ArrayHandleCast<vtkm::Id, NumIndicesArrayType>;

  // Although technically we are making changes to this object, the changes
  // are logically consistent with the previous state, so we consider it
  // valid under const.
  using Algorithm = vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapterTag>;
  Algorithm::ScanExclusive(CastedNumIndicesType(numIndices), offsets);
}

template <typename NumIndicesArrayType, typename IndexOffsetArrayType, typename DeviceAdapterTag>
void buildIndexOffsets(const NumIndicesArrayType&,
                       IndexOffsetArrayType&,
                       DeviceAdapterTag,
                       std::false_type)
{
  //this is a no-op as the storage for the offsets is an implicit handle
  //and should already be built. This signature exists so that
  //the compiler doesn't try to generate un-used code that will
  //try and run Algorithm::ScanExclusive on an implicit array which will
  //cause a compile time failure.
}

template <typename ArrayHandleIndices, typename ArrayHandleOffsets, typename DeviceAdapterTag>
void buildIndexOffsets(const ArrayHandleIndices& numIndices,
                       ArrayHandleOffsets offsets,
                       DeviceAdapterTag tag)
{
  using IsWriteable = vtkm::cont::internal::IsWriteableArrayHandle<ArrayHandleOffsets>;
  buildIndexOffsets(numIndices, offsets, tag, typename IsWriteable::type());
}

template <typename ShapeStorageTag = VTKM_DEFAULT_STORAGE_TAG,
          typename NumIndicesStorageTag = VTKM_DEFAULT_STORAGE_TAG,
          typename ConnectivityStorageTag = VTKM_DEFAULT_STORAGE_TAG,
          typename IndexOffsetStorageTag = VTKM_DEFAULT_STORAGE_TAG>
struct ConnectivityExplicitInternals
{
  using ShapeArrayType = vtkm::cont::ArrayHandle<vtkm::UInt8, ShapeStorageTag>;
  using NumIndicesArrayType = vtkm::cont::ArrayHandle<vtkm::IdComponent, NumIndicesStorageTag>;
  using ConnectivityArrayType = vtkm::cont::ArrayHandle<vtkm::Id, ConnectivityStorageTag>;
  using IndexOffsetArrayType = vtkm::cont::ArrayHandle<vtkm::Id, IndexOffsetStorageTag>;

  ShapeArrayType Shapes;
  NumIndicesArrayType NumIndices;
  ConnectivityArrayType Connectivity;
  mutable IndexOffsetArrayType IndexOffsets;

  bool ElementsValid;
  mutable bool IndexOffsetsValid;

  VTKM_CONT
  ConnectivityExplicitInternals()
    : ElementsValid(false)
    , IndexOffsetsValid(false)
  {
  }

  VTKM_CONT
  vtkm::Id GetNumberOfElements() const
  {
    VTKM_ASSERT(this->ElementsValid);

    return this->Shapes.GetNumberOfValues();
  }

  VTKM_CONT
  void ReleaseResourcesExecution()
  {
    this->Shapes.ReleaseResourcesExecution();
    this->NumIndices.ReleaseResourcesExecution();
    this->Connectivity.ReleaseResourcesExecution();
    this->IndexOffsets.ReleaseResourcesExecution();
  }

  template <typename Device>
  VTKM_CONT void BuildIndexOffsets(Device) const
  {
    VTKM_ASSERT(this->ElementsValid);

    if (!this->IndexOffsetsValid)
    {
      buildIndexOffsets(this->NumIndices, this->IndexOffsets, Device());
      this->IndexOffsetsValid = true;
    }
  }

  VTKM_CONT
  void BuildIndexOffsets(vtkm::cont::DeviceAdapterTagError) const
  {
    if (!this->IndexOffsetsValid)
    {
      throw vtkm::cont::ErrorBadType(
        "Cannot build indices using the error device. Must be created previously.");
    }
  }

  VTKM_CONT
  void PrintSummary(std::ostream& out) const
  {
    if (this->ElementsValid)
    {
      out << "     Shapes: ";
      vtkm::cont::printSummary_ArrayHandle(this->Shapes, out);
      out << "     NumIndices: ";
      vtkm::cont::printSummary_ArrayHandle(this->NumIndices, out);
      out << "     Connectivity: ";
      vtkm::cont::printSummary_ArrayHandle(this->Connectivity, out);
      if (this->IndexOffsetsValid)
      {
        out << "     IndexOffsets: ";
        vtkm::cont::printSummary_ArrayHandle(this->IndexOffsets, out);
      }
      else
      {
        out << "     IndexOffsets: Not Allocated" << std::endl;
      }
    }
    else
    {
      out << "     Not Allocated" << std::endl;
    }
  }
};

// Pass through (needed for ReverseConnectivityBuilder)
struct PassThrough
{
  VTKM_EXEC vtkm::Id operator()(const vtkm::Id& val) const { return val; }
};

// Compute cell id from input connectivity:
// Find the upper bound of the conn idx in the offsets table and subtract 1
//
// Example:
// Offsets: |  0        |  3        |  6           |  10       |
// Conn:    |  0  1  2  |  0  1  3  |  2  4  5  6  |  1  3  5  |
// ConnIdx: |  0  1  2  |  3  4  5  |  6  7  8  9  |  10 11 12 |
// UpprBnd: |  1  1  1  |  2  2  2  |  3  3  3  3  |  4  4  4  |
// CellIdx: |  0  0  0  |  1  1  1  |  2  2  2  2  |  3  3  3  |
template <typename OffsetsPortalType>
struct ConnIdxToCellIdCalc
{
  OffsetsPortalType Offsets;

  VTKM_CONT
  ConnIdxToCellIdCalc(const OffsetsPortalType& offsets)
    : Offsets(offsets)
  {
  }

  VTKM_EXEC
  vtkm::Id operator()(vtkm::Id inIdx) const
  {
    // Compute the upper bound index:
    vtkm::Id upperBoundIdx;
    {
      vtkm::Id first = 0;
      vtkm::Id length = this->Offsets.GetNumberOfValues();

      while (length > 0)
      {
        vtkm::Id half = length / 2;
        vtkm::Id pos = first + half;
        vtkm::Id val = this->Offsets.Get(pos);
        if (val <= inIdx)
        {
          first = pos + 1;
          length -= half + 1;
        }
        else
        {
          length = half;
        }
      }

      upperBoundIdx = first;
    }

    return upperBoundIdx - 1;
  }
};

template <typename PointToCell, typename C2PShapeStorageTag, typename Device>
void ComputeCellToPointConnectivity(ConnectivityExplicitInternals<C2PShapeStorageTag>& cell2Point,
                                    const PointToCell& point2Cell,
                                    vtkm::Id numberOfPoints,
                                    Device)
{
  if (cell2Point.ElementsValid)
  {
    return;
  }

  auto& conn = point2Cell.Connectivity;
  auto& rConn = cell2Point.Connectivity;
  auto& rNumIndices = cell2Point.NumIndices;
  auto& rIndexOffsets = cell2Point.IndexOffsets;
  vtkm::Id rConnSize = conn.GetNumberOfValues();

  auto offInPortal = point2Cell.IndexOffsets.PrepareForInput(Device{});

  PassThrough idxCalc{};
  ConnIdxToCellIdCalc<decltype(offInPortal)> cellIdCalc{ offInPortal };

  vtkm::cont::internal::ReverseConnectivityBuilder builder;
  builder.Run(conn,
              rConn,
              rNumIndices,
              rIndexOffsets,
              idxCalc,
              cellIdCalc,
              numberOfPoints,
              rConnSize,
              Device());

  // Set the CellToPoint information
  cell2Point.Shapes = vtkm::cont::make_ArrayHandleConstant(
    static_cast<vtkm::UInt8>(CELL_SHAPE_VERTEX), numberOfPoints);
  cell2Point.ElementsValid = true;
  cell2Point.IndexOffsetsValid = true;
}
}
}
} // namespace vtkm::cont::internal

#endif //vtk_m_cont_internal_ConnectivityExplicitInternals_h

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_internal_ConnectivityExplicitInternals_h
#define vtk_m_cont_internal_ConnectivityExplicitInternals_h

#include <vtkm/CellShape.h>
#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayGetValues.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/internal/ReverseConnectivityBuilder.h>
#include <vtkm/exec/ExecutionWholeArray.h>

namespace vtkm
{
namespace cont
{
namespace internal
{

template <typename ShapesStorageTag = VTKM_DEFAULT_STORAGE_TAG,
          typename ConnectivityStorageTag = VTKM_DEFAULT_STORAGE_TAG,
          typename OffsetsStorageTag = VTKM_DEFAULT_STORAGE_TAG>
struct ConnectivityExplicitInternals
{
  using ShapesArrayType = vtkm::cont::ArrayHandle<vtkm::UInt8, ShapesStorageTag>;
  using ConnectivityArrayType = vtkm::cont::ArrayHandle<vtkm::Id, ConnectivityStorageTag>;
  using OffsetsArrayType = vtkm::cont::ArrayHandle<vtkm::Id, OffsetsStorageTag>;

  ShapesArrayType Shapes;
  ConnectivityArrayType Connectivity;
  OffsetsArrayType Offsets;

  bool ElementsValid;

  VTKM_CONT
  ConnectivityExplicitInternals()
    : ElementsValid(false)
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
    this->Connectivity.ReleaseResourcesExecution();
    this->Offsets.ReleaseResourcesExecution();
  }

  VTKM_CONT
  void PrintSummary(std::ostream& out) const
  {
    if (this->ElementsValid)
    {
      out << "     Shapes: ";
      vtkm::cont::printSummary_ArrayHandle(this->Shapes, out);
      out << "     Connectivity: ";
      vtkm::cont::printSummary_ArrayHandle(this->Connectivity, out);
      out << "     Offsets: ";
      vtkm::cont::printSummary_ArrayHandle(this->Offsets, out);
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
        vtkm::Id halfway = length / 2;
        vtkm::Id pos = first + halfway;
        vtkm::Id val = this->Offsets.Get(pos);
        if (val <= inIdx)
        {
          first = pos + 1;
          length -= halfway + 1;
        }
        else
        {
          length = halfway;
        }
      }

      upperBoundIdx = first;
    }

    return upperBoundIdx - 1;
  }
};

// Much easier for CellSetSingleType:
struct ConnIdxToCellIdCalcSingleType
{
  vtkm::IdComponent CellSize;

  VTKM_CONT
  ConnIdxToCellIdCalcSingleType(vtkm::IdComponent cellSize)
    : CellSize(cellSize)
  {
  }

  VTKM_EXEC
  vtkm::Id operator()(vtkm::Id inIdx) const { return inIdx / this->CellSize; }
};

template <typename ConnTableT, typename RConnTableT, typename Device>
void ComputeRConnTable(RConnTableT& rConnTable,
                       const ConnTableT& connTable,
                       vtkm::Id numberOfPoints,
                       Device)
{
  if (rConnTable.ElementsValid)
  {
    return;
  }

  const auto& conn = connTable.Connectivity;
  auto& rConn = rConnTable.Connectivity;
  auto& rOffsets = rConnTable.Offsets;
  const vtkm::Id rConnSize = conn.GetNumberOfValues();

  const auto offInPortal = connTable.Offsets.PrepareForInput(Device{});

  PassThrough idxCalc{};
  ConnIdxToCellIdCalc<decltype(offInPortal)> cellIdCalc{ offInPortal };

  vtkm::cont::internal::ReverseConnectivityBuilder builder;
  builder.Run(conn, rConn, rOffsets, idxCalc, cellIdCalc, numberOfPoints, rConnSize, Device());

  rConnTable.Shapes = vtkm::cont::make_ArrayHandleConstant(
    static_cast<vtkm::UInt8>(CELL_SHAPE_VERTEX), numberOfPoints);
  rConnTable.ElementsValid = true;
}

// Specialize for CellSetSingleType:
template <typename RConnTableT, typename ConnectivityStorageTag, typename Device>
void ComputeRConnTable(RConnTableT& rConnTable,
                       const ConnectivityExplicitInternals< // SingleType specialization types:
                         typename vtkm::cont::ArrayHandleConstant<vtkm::UInt8>::StorageTag,
                         ConnectivityStorageTag,
                         typename vtkm::cont::ArrayHandleCounting<vtkm::Id>::StorageTag>& connTable,
                       vtkm::Id numberOfPoints,
                       Device)
{
  if (rConnTable.ElementsValid)
  {
    return;
  }

  const auto& conn = connTable.Connectivity;
  auto& rConn = rConnTable.Connectivity;
  auto& rOffsets = rConnTable.Offsets;
  const vtkm::Id rConnSize = conn.GetNumberOfValues();

  const vtkm::IdComponent cellSize = [&]() -> vtkm::IdComponent {
    if (connTable.Offsets.GetNumberOfValues() >= 2)
    {
      const auto firstTwo = vtkm::cont::ArrayGetValues({ 0, 1 }, connTable.Offsets);
      return static_cast<vtkm::IdComponent>(firstTwo[1] - firstTwo[0]);
    }
    return 0;
  }();

  PassThrough idxCalc{};
  ConnIdxToCellIdCalcSingleType cellIdCalc{ cellSize };

  vtkm::cont::internal::ReverseConnectivityBuilder builder;
  builder.Run(conn, rConn, rOffsets, idxCalc, cellIdCalc, numberOfPoints, rConnSize, Device());

  rConnTable.Shapes = vtkm::cont::make_ArrayHandleConstant(
    static_cast<vtkm::UInt8>(CELL_SHAPE_VERTEX), numberOfPoints);
  rConnTable.ElementsValid = true;
}
}
}
} // namespace vtkm::cont::internal

#endif //vtk_m_cont_internal_ConnectivityExplicitInternals_h

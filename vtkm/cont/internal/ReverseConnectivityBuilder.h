//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_internal_ReverseConnectivityBuilder_h
#define vtk_m_cont_internal_ReverseConnectivityBuilder_h

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/CellSetExplicit.h>

#include <vtkm/cont/AtomicArray.h>
#include <vtkm/exec/FunctorBase.h>

#include <utility>

namespace vtkm
{
namespace cont
{
namespace internal
{

namespace rcb
{

template <typename AtomicHistogram, typename ConnInPortal, typename RConnToConnIdxCalc>
struct BuildHistogram : public vtkm::exec::FunctorBase
{
  AtomicHistogram Histo;
  ConnInPortal Conn;
  RConnToConnIdxCalc IdxCalc;

  VTKM_CONT
  BuildHistogram(const AtomicHistogram& histo,
                 const ConnInPortal& conn,
                 const RConnToConnIdxCalc& idxCalc)
    : Histo(histo)
    , Conn(conn)
    , IdxCalc(idxCalc)
  {
  }

  VTKM_EXEC
  void operator()(vtkm::Id rconnIdx) const
  {
    // Compute the connectivity array index (skipping cell length entries)
    const vtkm::Id connIdx = this->IdxCalc(rconnIdx);
    const vtkm::Id ptId = this->Conn.Get(connIdx);
    this->Histo.Add(ptId, 1);
  }
};

template <typename AtomicHistogram,
          typename ConnInPortal,
          typename ROffsetInPortal,
          typename RConnOutPortal,
          typename RConnToConnIdxCalc,
          typename ConnIdxToCellIdxCalc>
struct GenerateRConn : public vtkm::exec::FunctorBase
{
  AtomicHistogram Histo;
  ConnInPortal Conn;
  ROffsetInPortal ROffsets;
  RConnOutPortal RConn;
  RConnToConnIdxCalc IdxCalc;
  ConnIdxToCellIdxCalc CellIdCalc;

  VTKM_CONT
  GenerateRConn(const AtomicHistogram& histo,
                const ConnInPortal& conn,
                const ROffsetInPortal& rOffsets,
                const RConnOutPortal& rconn,
                const RConnToConnIdxCalc& idxCalc,
                const ConnIdxToCellIdxCalc& cellIdCalc)
    : Histo(histo)
    , Conn(conn)
    , ROffsets(rOffsets)
    , RConn(rconn)
    , IdxCalc(idxCalc)
    , CellIdCalc(cellIdCalc)
  {
  }

  VTKM_EXEC
  void operator()(vtkm::Id inputIdx) const
  {
    // Compute the connectivity array index (skipping cell length entries)
    const vtkm::Id connIdx = this->IdxCalc(inputIdx);
    const vtkm::Id ptId = this->Conn.Get(connIdx);

    // Compute the cell id:
    const vtkm::Id cellId = this->CellIdCalc(connIdx);

    // Find the base offset for this point id:
    const vtkm::Id baseOffset = this->ROffsets.Get(ptId);

    // Find the next unused index for this point id
    const vtkm::Id nextAvailable = this->Histo.Add(ptId, 1);

    // Update the final location in the RConn table with the cellId
    const vtkm::Id rconnIdx = baseOffset + nextAvailable;
    this->RConn.Set(rconnIdx, cellId);
  }
};
}
/// Takes a connectivity array handle (conn) and constructs a reverse
/// connectivity table suitable for use by VTK-m (rconn).
///
/// This code is generalized for use by VTK and VTK-m.
///
/// The Run(...) method is the main entry point. The template parameters are:
/// @param RConnToConnIdxCalc defines `vtkm::Id operator()(vtkm::Id in) const`
/// which computes the index of the in'th point id in conn. This is necessary
/// for VTK-style cell arrays that need to skip the cell length entries. In
/// vtk-m, this is a no-op passthrough.
/// @param ConnIdxToCellIdxCalc Functor that computes the cell id from an
/// index into conn.
/// @param ConnTag is the StorageTag for the input connectivity array.
///
/// See usages in vtkmCellSetExplicit and vtkmCellSetSingleType for examples.
class ReverseConnectivityBuilder
{
public:
  VTKM_CONT
  template <typename ConnArray,
            typename RConnArray,
            typename ROffsetsArray,
            typename RConnToConnIdxCalc,
            typename ConnIdxToCellIdxCalc>
  inline void Run(const ConnArray& conn,
                  RConnArray& rConn,
                  ROffsetsArray& rOffsets,
                  const RConnToConnIdxCalc& rConnToConnCalc,
                  const ConnIdxToCellIdxCalc& cellIdCalc,
                  vtkm::Id numberOfPoints,
                  vtkm::Id rConnSize,
                  vtkm::cont::DeviceAdapterId device)
  {
    vtkm::cont::Token connToken;
    auto connPortal = conn.PrepareForInput(device, connToken);
    auto zeros = vtkm::cont::make_ArrayHandleConstant(vtkm::IdComponent{ 0 }, numberOfPoints);

    // Compute RConn offsets by atomically building a histogram and doing an
    // extended scan.
    //
    // Example:
    // (in)  Conn:  | 3  0  1  2  |  3  0  1  3  |  3  0  3  4  |  3  3  4  5  |
    // (out) RNumIndices:  3  2  1  3  2  1
    // (out) RIdxOffsets:  0  3  5  6  9 11 12
    vtkm::cont::ArrayHandle<vtkm::IdComponent> rNumIndices;
    { // allocate and zero the numIndices array:
      vtkm::cont::Algorithm::Copy(device, zeros, rNumIndices);
    }

    { // Build histogram:
      vtkm::cont::AtomicArray<vtkm::IdComponent> atomicCounter{ rNumIndices };
      vtkm::cont::Token token;
      auto ac = atomicCounter.PrepareForExecution(device, token);
      using BuildHisto =
        rcb::BuildHistogram<decltype(ac), decltype(connPortal), RConnToConnIdxCalc>;
      BuildHisto histoGen{ ac, connPortal, rConnToConnCalc };

      vtkm::cont::Algorithm::Schedule(device, histoGen, rConnSize);
    }

    { // Compute offsets:
      vtkm::cont::Algorithm::ScanExtended(
        device, vtkm::cont::make_ArrayHandleCast<vtkm::Id>(rNumIndices), rOffsets);
    }

    { // Reset the numIndices array to 0's:
      vtkm::cont::Algorithm::Copy(device, zeros, rNumIndices);
    }

    // Fill the connectivity table:
    // 1) Lookup each point idx base offset.
    // 2) Use the atomic histogram to find the next available slot for this
    //    pt id in RConn.
    // 3) Compute the cell id from the connectivity index
    // 4) Update RConn[nextSlot] = cellId
    //
    // Example:
    // (in)    Conn:  | 3  0  1  2  |  3  0  1  3  |  3  0  3  4  |  3  3  4  5  |
    // (inout) RNumIndices:  0  0  0  0  0  0  (Initial)
    // (inout) RNumIndices:  3  2  1  3  2  1  (Final)
    // (in)    RIdxOffsets:  0  3  5  6  9  11
    // (out)   RConn: | 0  1  2  |  0  1  |  0  |  1  2  3  |  2  3  |  3  |
    {
      vtkm::cont::AtomicArray<vtkm::IdComponent> atomicCounter{ rNumIndices };
      vtkm::cont::Token token;
      auto ac = atomicCounter.PrepareForExecution(device, token);
      auto rOffsetPortal = rOffsets.PrepareForInput(device, token);
      auto rConnPortal = rConn.PrepareForOutput(rConnSize, device, token);

      using GenRConnT = rcb::GenerateRConn<decltype(ac),
                                           decltype(connPortal),
                                           decltype(rOffsetPortal),
                                           decltype(rConnPortal),
                                           RConnToConnIdxCalc,
                                           ConnIdxToCellIdxCalc>;
      GenRConnT rConnGen{ ac, connPortal, rOffsetPortal, rConnPortal, rConnToConnCalc, cellIdCalc };

      vtkm::cont::Algorithm::Schedule(device, rConnGen, rConnSize);
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

template <typename ConnTableT, typename RConnTableT>
void ComputeRConnTable(RConnTableT& rConnTable,
                       const ConnTableT& connTable,
                       vtkm::Id numberOfPoints,
                       vtkm::cont::DeviceAdapterId device)
{
  if (rConnTable.ElementsValid)
  {
    return;
  }

  const auto& conn = connTable.Connectivity;
  auto& rConn = rConnTable.Connectivity;
  auto& rOffsets = rConnTable.Offsets;
  const vtkm::Id rConnSize = conn.GetNumberOfValues();

  {
    vtkm::cont::Token token;
    const auto offInPortal = connTable.Offsets.PrepareForInput(device, token);

    PassThrough idxCalc{};
    ConnIdxToCellIdCalc<decltype(offInPortal)> cellIdCalc{ offInPortal };

    vtkm::cont::internal::ReverseConnectivityBuilder builder;
    builder.Run(conn, rConn, rOffsets, idxCalc, cellIdCalc, numberOfPoints, rConnSize, device);
  }

  rConnTable.Shapes = vtkm::cont::make_ArrayHandleConstant(
    static_cast<vtkm::UInt8>(CELL_SHAPE_VERTEX), numberOfPoints);
  rConnTable.ElementsValid = true;
}

// Specialize for CellSetSingleType:
template <typename RConnTableT, typename ConnectivityStorageTag>
void ComputeRConnTable(RConnTableT& rConnTable,
                       const ConnectivityExplicitInternals< // SingleType specialization types:
                         typename vtkm::cont::ArrayHandleConstant<vtkm::UInt8>::StorageTag,
                         ConnectivityStorageTag,
                         typename vtkm::cont::ArrayHandleCounting<vtkm::Id>::StorageTag>& connTable,
                       vtkm::Id numberOfPoints,
                       vtkm::cont::DeviceAdapterId device)
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
  builder.Run(conn, rConn, rOffsets, idxCalc, cellIdCalc, numberOfPoints, rConnSize, device);

  rConnTable.Shapes = vtkm::cont::make_ArrayHandleConstant(
    static_cast<vtkm::UInt8>(CELL_SHAPE_VERTEX), numberOfPoints);
  rConnTable.ElementsValid = true;
}

}
}
} // end namespace vtkm::cont::internal

#endif // ReverseConnectivityBuilder_h

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/CellClassification.h>
#include <vtkm/RangeId.h>
#include <vtkm/RangeId2.h>
#include <vtkm/RangeId3.h>
#include <vtkm/Types.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/filter/internal/CreateResult.h>

#include <vtkm/worklet/WorkletMapField.h>

namespace
{
struct TypeUInt8 : vtkm::ListTagBase<vtkm::UInt8>
{
};

class SetStructuredGhostZones1D : public vtkm::worklet::WorkletMapField
{
public:
  SetStructuredGhostZones1D(const vtkm::Id& dim, const vtkm::Id& numLayers = 1)
    : Dim(dim)
    , NumLayers(numLayers)
    , Range(NumLayers, Dim - NumLayers)
  {
  }

  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);

  template <typename T>
  VTKM_EXEC void operator()(const vtkm::Id& cellIndex, T& value) const
  {
    value = (Range.Contains(cellIndex) ? NormalCell : DuplicateCell);
  }

private:
  vtkm::Id Dim;
  vtkm::Id NumLayers;
  vtkm::RangeId Range;
  static constexpr vtkm::UInt8 NormalCell =
    static_cast<vtkm::UInt8>(vtkm::CellClassification::NORMAL);
  static constexpr vtkm::UInt8 DuplicateCell =
    static_cast<vtkm::UInt8>(vtkm::CellClassification::GHOST);
};

class SetStructuredGhostZones2D : public vtkm::worklet::WorkletMapField
{
public:
  SetStructuredGhostZones2D(const vtkm::Id2& dims, const vtkm::Id& numLayers = 1)
    : Dims(dims)
    , NumLayers(numLayers)
    , Range(NumLayers, Dims[0] - NumLayers, NumLayers, Dims[1] - NumLayers)
  {
  }

  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);

  template <typename T>
  VTKM_EXEC void operator()(const vtkm::Id& cellIndex, T& value) const
  {
    vtkm::Id2 ij(cellIndex % Dims[0], cellIndex / Dims[0]);

    value = (Range.Contains(ij) ? NormalCell : DuplicateCell);
  }

private:
  vtkm::Id2 Dims;
  vtkm::Id NumLayers;
  vtkm::RangeId2 Range;
  static constexpr vtkm::UInt8 NormalCell =
    static_cast<vtkm::UInt8>(vtkm::CellClassification::NORMAL);
  static constexpr vtkm::UInt8 DuplicateCell =
    static_cast<vtkm::UInt8>(vtkm::CellClassification::GHOST);
};

class SetStructuredGhostZones3D : public vtkm::worklet::WorkletMapField
{
public:
  SetStructuredGhostZones3D(const vtkm::Id3& dims, const vtkm::Id& numLayers = 1)
    : Dims(dims)
    , NumLayers(numLayers)
    , Range(NumLayers,
            Dims[0] - NumLayers,
            NumLayers,
            Dims[1] - NumLayers,
            NumLayers,
            Dims[2] - NumLayers)
  {
  }

  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);

  template <typename T>
  VTKM_EXEC void operator()(const vtkm::Id& cellIndex, T& value) const
  {
    vtkm::Id3 ijk(
      cellIndex % Dims[0], (cellIndex / Dims[0]) % Dims[1], cellIndex / (Dims[0] * Dims[1]));

    value = (Range.Contains(ijk) ? NormalCell : DuplicateCell);
  }

private:
  vtkm::Id3 Dims;
  vtkm::Id NumLayers;
  vtkm::RangeId3 Range;
  static constexpr vtkm::UInt8 NormalCell =
    static_cast<vtkm::UInt8>(vtkm::CellClassification::NORMAL);
  static constexpr vtkm::UInt8 DuplicateCell =
    static_cast<vtkm::UInt8>(vtkm::CellClassification::GHOST);
};
};

namespace vtkm
{
namespace filter
{

inline VTKM_CONT AddGhostZone::AddGhostZone()
{
}

template <typename Policy>
inline VTKM_CONT vtkm::cont::DataSet AddGhostZone::DoExecute(const vtkm::cont::DataSet& input,
                                                             vtkm::filter::PolicyBase<Policy>)
{
  const vtkm::cont::DynamicCellSet& cellset = input.GetCellSet(this->GetActiveCellSetIndex());
  vtkm::Id numCells = cellset.GetNumberOfCells();
  vtkm::cont::ArrayHandleIndex indexArray(numCells);
  vtkm::cont::ArrayHandle<vtkm::UInt8> ghosts;

  ghosts.Allocate(numCells);

  //Structured cases are easy...
  if (cellset.template IsType<vtkm::cont::CellSetStructured<1>>())
  {
    if (numCells <= 2)
      throw vtkm::cont::ErrorFilterExecution("insufficient number of cells for AddGhostZone.");

    vtkm::cont::CellSetStructured<1> cellset1d = cellset.Cast<vtkm::cont::CellSetStructured<1>>();
    SetStructuredGhostZones1D structuredGhosts1D(cellset1d.GetCellDimensions());
    vtkm::worklet::DispatcherMapField<SetStructuredGhostZones1D> dispatcher(structuredGhosts1D);
    dispatcher.Invoke(indexArray, ghosts);
  }
  else if (cellset.template IsType<vtkm::cont::CellSetStructured<2>>())
  {
    if (numCells <= 4)
      throw vtkm::cont::ErrorFilterExecution("insufficient number of cells for AddGhostZone.");

    vtkm::cont::CellSetStructured<2> cellset2d = cellset.Cast<vtkm::cont::CellSetStructured<2>>();
    SetStructuredGhostZones2D structuredGhosts2D(cellset2d.GetCellDimensions());
    vtkm::worklet::DispatcherMapField<SetStructuredGhostZones2D> dispatcher(structuredGhosts2D);
    dispatcher.Invoke(indexArray, ghosts);
  }
  else if (cellset.template IsType<vtkm::cont::CellSetStructured<3>>())
  {
    if (numCells <= 8)
      throw vtkm::cont::ErrorFilterExecution("insufficient number of cells for AddGhostZone.");

    vtkm::cont::CellSetStructured<3> cellset3d = cellset.Cast<vtkm::cont::CellSetStructured<3>>();
    SetStructuredGhostZones3D structuredGhosts3D(cellset3d.GetCellDimensions());
    vtkm::worklet::DispatcherMapField<SetStructuredGhostZones3D> dispatcher(structuredGhosts3D);
    dispatcher.Invoke(indexArray, ghosts);
  }
  else
    throw vtkm::cont::ErrorFilterExecution("Unsupported cellset type for AddGhostZone.");

  vtkm::cont::DataSet output = internal::CreateResult(
    input, ghosts, "vtkmGhostCells", vtkm::cont::Field::Association::CELL_SET, cellset.GetName());
  return output;
}

template <typename ValueType, typename Storage, typename Policy>
inline VTKM_CONT bool AddGhostZone::DoMapField(vtkm::cont::DataSet&,
                                               const vtkm::cont::ArrayHandle<ValueType, Storage>&,
                                               const vtkm::filter::FieldMetadata&,
                                               vtkm::filter::PolicyBase<Policy>)
{
  return true;
}
}
}

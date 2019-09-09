//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_GhostCellRemove_hxx
#define vtk_m_filter_GhostCellRemove_hxx
#include <vtkm/filter/GhostCellRemove.h>

#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/CellSetPermutation.h>
#include <vtkm/cont/DynamicCellSet.h>

#include <vtkm/RangeId3.h>
#include <vtkm/filter/ExtractStructured.h>
#include <vtkm/worklet/CellDeepCopy.h>

namespace
{

class RemoveAllGhosts
{
public:
  VTKM_CONT
  RemoveAllGhosts() {}

  VTKM_EXEC bool operator()(const vtkm::UInt8& value) const { return (value == 0); }
};

class RemoveGhostByType
{
public:
  VTKM_CONT
  RemoveGhostByType()
    : RemoveType(0)
  {
  }

  VTKM_CONT
  RemoveGhostByType(const vtkm::UInt8& val)
    : RemoveType(static_cast<vtkm::UInt8>(~val))
  {
  }

  VTKM_EXEC bool operator()(const vtkm::UInt8& value) const
  {
    return value == 0 || (value & RemoveType);
  }

private:
  vtkm::UInt8 RemoveType;
};

template <int DIMS>
VTKM_EXEC_CONT vtkm::Id3 getLogical(const vtkm::Id& index, const vtkm::Id3& cellDims);

template <>
VTKM_EXEC_CONT vtkm::Id3 getLogical<3>(const vtkm::Id& index, const vtkm::Id3& cellDims)
{
  vtkm::Id3 res(0, 0, 0);
  res[0] = index % cellDims[0];
  res[1] = (index / (cellDims[0])) % (cellDims[1]);
  res[2] = index / ((cellDims[0]) * (cellDims[1]));
  return res;
}

template <>
VTKM_EXEC_CONT vtkm::Id3 getLogical<2>(const vtkm::Id& index, const vtkm::Id3& cellDims)
{
  vtkm::Id3 res(0, 0, 0);
  res[0] = index % cellDims[0];
  res[1] = index / cellDims[0];
  return res;
}

template <>
VTKM_EXEC_CONT vtkm::Id3 getLogical<1>(const vtkm::Id& index, const vtkm::Id3&)
{
  vtkm::Id3 res(0, 0, 0);
  res[0] = index;
  return res;
}

template <int DIMS>
class RealMinMax : public vtkm::worklet::WorkletMapField
{
public:
  VTKM_CONT
  RealMinMax(vtkm::Id3 cellDims, bool removeAllGhost, vtkm::UInt8 removeType)
    : CellDims(cellDims)
    , RemoveAllGhost(removeAllGhost)
    , RemoveType(removeType)
  {
  }

  typedef void ControlSignature(FieldIn, AtomicArrayInOut);
  typedef void ExecutionSignature(_1, InputIndex, _2);

  template <typename Atomic>
  VTKM_EXEC void Max(Atomic& atom, const vtkm::Id& val, const vtkm::Id& index) const
  {
    vtkm::Id old = -1;
    do
    {
      old = atom.CompareAndSwap(index, val, old);
    } while (old < val);
  }

  template <typename Atomic>
  VTKM_EXEC void Min(Atomic& atom, const vtkm::Id& val, const vtkm::Id& index) const
  {
    vtkm::Id old = 1000000000;
    do
    {
      old = atom.CompareAndSwap(index, val, old);
    } while (old > val);
  }

  template <typename T, typename AtomicType>
  VTKM_EXEC void operator()(const T& value, const vtkm::Id& index, AtomicType& atom) const
  {
    // we are finding the logical min max of valid cells
    if ((RemoveAllGhost && value != 0) || (!RemoveAllGhost && (value != 0 && value | RemoveType)))
      return;

    vtkm::Id3 logical = getLogical<DIMS>(index, CellDims);

    Min(atom, logical[0], 0);
    Min(atom, logical[1], 1);
    Min(atom, logical[2], 2);

    Max(atom, logical[0], 3);
    Max(atom, logical[1], 4);
    Max(atom, logical[2], 5);
  }

private:
  vtkm::Id3 CellDims;
  bool RemoveAllGhost;
  vtkm::UInt8 RemoveType;
};

template <int DIMS>
VTKM_EXEC_CONT bool checkRange(const vtkm::RangeId3& range, const vtkm::Id3& p);

template <>
VTKM_EXEC_CONT bool checkRange<1>(const vtkm::RangeId3& range, const vtkm::Id3& p)
{
  return p[0] >= range.X.Min && p[0] <= range.X.Max;
}
template <>
VTKM_EXEC_CONT bool checkRange<2>(const vtkm::RangeId3& range, const vtkm::Id3& p)
{
  return p[0] >= range.X.Min && p[0] <= range.X.Max && p[1] >= range.Y.Min && p[1] <= range.Y.Max;
}
template <>
VTKM_EXEC_CONT bool checkRange<3>(const vtkm::RangeId3& range, const vtkm::Id3& p)
{
  return p[0] >= range.X.Min && p[0] <= range.X.Max && p[1] >= range.Y.Min && p[1] <= range.Y.Max &&
    p[2] >= range.Z.Min && p[2] <= range.Z.Max;
}

template <int DIMS>
class Validate : public vtkm::worklet::WorkletMapField
{
public:
  VTKM_CONT
  Validate(const vtkm::Id3& cellDims,
           bool removeAllGhost,
           vtkm::UInt8 removeType,
           const vtkm::RangeId3& range)
    : CellDims(cellDims)
    , RemoveAll(removeAllGhost)
    , RemoveVal(removeType)
    , Range(range)
  {
  }

  typedef void ControlSignature(FieldIn, FieldOut);
  typedef void ExecutionSignature(_1, InputIndex, _2);

  template <typename T>
  VTKM_EXEC void operator()(const T& value, const vtkm::Id& index, vtkm::UInt8& valid) const
  {
    valid = 0;
    if (RemoveAll && value == 0)
      return;
    else if (!RemoveAll && (value == 0 || (value & RemoveVal)))
      return;

    if (checkRange<DIMS>(Range, getLogical<DIMS>(index, CellDims)))
      valid = static_cast<vtkm::UInt8>(1);
  }

private:
  vtkm::Id3 CellDims;
  bool RemoveAll;
  vtkm::UInt8 RemoveVal;
  vtkm::RangeId3 Range;
};

template <int DIMS, typename T, typename StorageType>
bool CanStrip(const vtkm::cont::ArrayHandle<T, StorageType>& ghostField,
              const vtkm::cont::Invoker& invoke,
              bool removeAllGhost,
              vtkm::UInt8 removeType,
              vtkm::RangeId3& range,
              const vtkm::Id3& cellDims,
              vtkm::Id size)
{
  vtkm::cont::ArrayHandle<vtkm::Id> minmax;
  minmax.Allocate(6);
  minmax.GetPortalControl().Set(0, std::numeric_limits<vtkm::Id>::max());
  minmax.GetPortalControl().Set(1, std::numeric_limits<vtkm::Id>::max());
  minmax.GetPortalControl().Set(2, std::numeric_limits<vtkm::Id>::max());
  minmax.GetPortalControl().Set(3, std::numeric_limits<vtkm::Id>::min());
  minmax.GetPortalControl().Set(4, std::numeric_limits<vtkm::Id>::min());
  minmax.GetPortalControl().Set(5, std::numeric_limits<vtkm::Id>::min());

  invoke(RealMinMax<3>(cellDims, removeAllGhost, removeType), ghostField, minmax);

  auto portal = minmax.GetPortalConstControl();
  range = vtkm::RangeId3(
    portal.Get(0), portal.Get(3), portal.Get(1), portal.Get(4), portal.Get(2), portal.Get(5));

  vtkm::cont::ArrayHandle<vtkm::UInt8> validFlags;
  validFlags.Allocate(size);

  invoke(Validate<DIMS>(cellDims, removeAllGhost, removeType, range), ghostField, validFlags);

  vtkm::UInt8 res = vtkm::cont::Algorithm::Reduce(validFlags, vtkm::UInt8(0), vtkm::Maximum());
  return res == 0;
}

template <typename T, typename StorageType>
bool CanDoStructuredStrip(const vtkm::cont::DynamicCellSet& cells,
                          const vtkm::cont::ArrayHandle<T, StorageType>& ghostField,
                          const vtkm::cont::Invoker& invoke,
                          bool removeAllGhost,
                          vtkm::UInt8 removeType,
                          vtkm::RangeId3& range)
{
  bool canDo = false;
  vtkm::Id3 cellDims(1, 1, 1);

  if (cells.IsSameType(vtkm::cont::CellSetStructured<1>()))
  {
    auto cells1D = cells.Cast<vtkm::cont::CellSetStructured<1>>();
    vtkm::Id d = cells1D.GetCellDimensions();
    cellDims[0] = d;
    vtkm::Id sz = d;

    canDo = CanStrip<1>(ghostField, invoke, removeAllGhost, removeType, range, cellDims, sz);
  }
  else if (cells.IsSameType(vtkm::cont::CellSetStructured<2>()))
  {
    auto cells2D = cells.Cast<vtkm::cont::CellSetStructured<2>>();
    vtkm::Id2 d = cells2D.GetCellDimensions();
    cellDims[0] = d[0];
    cellDims[1] = d[1];
    vtkm::Id sz = cellDims[0] * cellDims[1];
    canDo = CanStrip<2>(ghostField, invoke, removeAllGhost, removeType, range, cellDims, sz);
  }
  else if (cells.IsSameType(vtkm::cont::CellSetStructured<3>()))
  {
    auto cells3D = cells.Cast<vtkm::cont::CellSetStructured<3>>();
    cellDims = cells3D.GetCellDimensions();
    vtkm::Id sz = cellDims[0] * cellDims[1] * cellDims[2];
    canDo = CanStrip<3>(ghostField, invoke, removeAllGhost, removeType, range, cellDims, sz);
  }

  return canDo;
}

} // end anon namespace

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline VTKM_CONT GhostCellRemove::GhostCellRemove()
  : vtkm::filter::FilterDataSetWithField<GhostCellRemove>()
  , RemoveAll(false)
  , RemoveField(false)
  , RemoveVals(0)
{
  this->SetActiveField("vtkmGhostCells");
  this->SetFieldsToPass("vtkmGhostCells", vtkm::filter::FieldSelection::MODE_EXCLUDE);
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet GhostCellRemove::DoExecute(
  const vtkm::cont::DataSet& input,
  const vtkm::cont::ArrayHandle<T, StorageType>& field,
  const vtkm::filter::FieldMetadata& fieldMeta,
  vtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  //get the cells and coordinates of the dataset
  const vtkm::cont::DynamicCellSet& cells = input.GetCellSet();
  vtkm::cont::DynamicCellSet cellOut;

  //Preserve structured output where possible.
  if (cells.IsSameType(vtkm::cont::CellSetStructured<1>()) ||
      cells.IsSameType(vtkm::cont::CellSetStructured<2>()) ||
      cells.IsSameType(vtkm::cont::CellSetStructured<3>()))
  {
    vtkm::RangeId3 range;
    if (CanDoStructuredStrip(
          cells, field, this->Invoke, this->GetRemoveAllGhost(), this->GetRemoveType(), range))
    {
      vtkm::filter::ExtractStructured extract;
      extract.SetInvoker(this->Invoke);
      vtkm::RangeId3 erange(
        range.X.Min, range.X.Max + 2, range.Y.Min, range.Y.Max + 2, range.Z.Min, range.Z.Max + 2);
      vtkm::Id3 sample(1, 1, 1);
      extract.SetVOI(erange);
      extract.SetSampleRate(sample);
      if (this->GetRemoveGhostField())
        extract.SetFieldsToPass(this->GetActiveFieldName(),
                                vtkm::filter::FieldSelection::MODE_EXCLUDE);

      auto output = extract.Execute(input);
      return output;
    }
  }

  if (this->GetRemoveAllGhost())
  {
    cellOut = this->Worklet.Run(vtkm::filter::ApplyPolicyCellSet(cells, policy),
                                field,
                                fieldMeta.GetAssociation(),
                                RemoveAllGhosts());
  }
  else if (this->GetRemoveByType())
  {
    cellOut = this->Worklet.Run(vtkm::filter::ApplyPolicyCellSet(cells, policy),
                                field,
                                fieldMeta.GetAssociation(),
                                RemoveGhostByType(this->GetRemoveType()));
  }
  else
  {
    throw vtkm::cont::ErrorFilterExecution("Unsupported ghost cell removal type");
  }

  vtkm::cont::DataSet output;
  output.AddCoordinateSystem(input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()));
  output.SetCellSet(cellOut);

  return output;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT bool GhostCellRemove::DoMapField(
  vtkm::cont::DataSet& result,
  const vtkm::cont::ArrayHandle<T, StorageType>& input,
  const vtkm::filter::FieldMetadata& fieldMeta,
  vtkm::filter::PolicyBase<DerivedPolicy>)
{
  if (fieldMeta.IsPointField())
  {
    //we copy the input handle to the result dataset, reusing the metadata
    result.AddField(fieldMeta.AsField(input));
    return true;
  }
  else if (fieldMeta.IsCellField())
  {
    vtkm::cont::ArrayHandle<T> out = this->Worklet.ProcessCellField(input);
    result.AddField(fieldMeta.AsField(out));
    return true;
  }
  else
  {
    return false;
  }
}
}
}

#endif //vtk_m_filter_GhostCellRemove_hxx

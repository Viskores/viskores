//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/RangeId3.h>
#include <vtkm/cont/UnknownCellSet.h>

#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/filter/MapFieldPermutation.h>
#include <vtkm/filter/entity_extraction/ExtractStructured.h>
#include <vtkm/filter/entity_extraction/GhostCellRemove.h>
#include <vtkm/filter/entity_extraction/worklet/Threshold.h>

namespace
{

template <typename T>
VTKM_EXEC inline bool ShouldRemove(T value, vtkm::UInt8 removeTypes)
{
  return ((value & removeTypes) != 0);
}

class RemoveGhostPredicate
{
public:
  VTKM_CONT RemoveGhostPredicate()
    : RemoveTypes(0xFF)
  {
  }

  VTKM_CONT explicit RemoveGhostPredicate(vtkm::UInt8 val)
    : RemoveTypes(val)
  {
  }

  VTKM_EXEC bool operator()(const vtkm::UInt8& value) const
  {
    return !ShouldRemove(value, this->RemoveTypes);
  }

private:
  vtkm::UInt8 RemoveTypes;
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
  RealMinMax(vtkm::Id3 cellDims, vtkm::UInt8 removeTypes)
    : CellDims(cellDims)
    , RemoveTypes(removeTypes)
  {
  }

  typedef void ControlSignature(FieldIn, AtomicArrayInOut);
  typedef void ExecutionSignature(_1, InputIndex, _2);

  template <typename Atomic>
  VTKM_EXEC void Max(Atomic& atom, const vtkm::Id& val, const vtkm::Id& index) const
  {
    vtkm::Id old = atom.Get(index);
    while (old < val)
    {
      atom.CompareExchange(index, &old, val);
    }
  }

  template <typename Atomic>
  VTKM_EXEC void Min(Atomic& atom, const vtkm::Id& val, const vtkm::Id& index) const
  {
    vtkm::Id old = atom.Get(index);
    while (old > val)
    {
      atom.CompareExchange(index, &old, val);
    }
  }

  template <typename T, typename AtomicType>
  VTKM_EXEC void operator()(const T& value, const vtkm::Id& index, AtomicType& atom) const
  {
    // we are finding the logical min max of valid cells
    if (ShouldRemove(value, this->RemoveTypes))
    {
      return;
    }

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
  vtkm::UInt8 RemoveTypes;
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
  Validate(const vtkm::Id3& cellDims, vtkm::UInt8 removeTypes, const vtkm::RangeId3& range)
    : CellDims(cellDims)
    , RemoveVals(removeTypes)
    , Range(range)
  {
  }

  typedef void ControlSignature(FieldIn, FieldOut);
  typedef void ExecutionSignature(_1, InputIndex, _2);

  template <typename T>
  VTKM_EXEC void operator()(const T& value, const vtkm::Id& index, vtkm::UInt8& invalid) const
  {
    if (ShouldRemove(value, this->RemoveVals) &&
        checkRange<DIMS>(Range, getLogical<DIMS>(index, CellDims)))
    {
      invalid = static_cast<vtkm::UInt8>(1);
    }
    else
    {
      invalid = 0;
    }
  }

private:
  vtkm::Id3 CellDims;
  vtkm::UInt8 RemoveVals;
  vtkm::RangeId3 Range;
};

template <int DIMS, typename T, typename StorageType>
bool CanStrip(const vtkm::cont::ArrayHandle<T, StorageType>& ghostField,
              const vtkm::cont::Invoker& invoke,
              vtkm::UInt8 removeTypes,
              vtkm::RangeId3& range,
              const vtkm::Id3& cellDims)
{
  vtkm::cont::ArrayHandle<vtkm::Id> minmax;
  minmax.Allocate(6);
  minmax.WritePortal().Set(0, std::numeric_limits<vtkm::Id>::max());
  minmax.WritePortal().Set(1, std::numeric_limits<vtkm::Id>::max());
  minmax.WritePortal().Set(2, std::numeric_limits<vtkm::Id>::max());
  minmax.WritePortal().Set(3, std::numeric_limits<vtkm::Id>::min());
  minmax.WritePortal().Set(4, std::numeric_limits<vtkm::Id>::min());
  minmax.WritePortal().Set(5, std::numeric_limits<vtkm::Id>::min());

  invoke(RealMinMax<3>(cellDims, removeTypes), ghostField, minmax);

  auto portal = minmax.ReadPortal();
  range = vtkm::RangeId3(
    portal.Get(0), portal.Get(3), portal.Get(1), portal.Get(4), portal.Get(2), portal.Get(5));

  vtkm::cont::ArrayHandle<vtkm::UInt8> invalidFlags;

  invoke(Validate<DIMS>(cellDims, removeTypes, range), ghostField, invalidFlags);

  vtkm::UInt8 res = vtkm::cont::Algorithm::Reduce(invalidFlags, vtkm::UInt8(0), vtkm::Maximum());
  return res == 0;
}

template <typename T, typename StorageType>
bool CanDoStructuredStrip(const vtkm::cont::UnknownCellSet& cells,
                          const vtkm::cont::ArrayHandle<T, StorageType>& ghostField,
                          const vtkm::cont::Invoker& invoke,
                          vtkm::UInt8 removeTypes,
                          vtkm::RangeId3& range)
{
  bool canDo = false;
  vtkm::Id3 cellDims(1, 1, 1);

  if (cells.CanConvert<vtkm::cont::CellSetStructured<1>>())
  {
    auto cells1D = cells.AsCellSet<vtkm::cont::CellSetStructured<1>>();
    vtkm::Id d = cells1D.GetCellDimensions();
    cellDims[0] = d;
    VTKM_ASSERT(ghostField.GetNumberOfValues() == cellDims[0]);
    canDo = CanStrip<1>(ghostField, invoke, removeTypes, range, cellDims);
  }
  else if (cells.CanConvert<vtkm::cont::CellSetStructured<2>>())
  {
    auto cells2D = cells.AsCellSet<vtkm::cont::CellSetStructured<2>>();
    vtkm::Id2 d = cells2D.GetCellDimensions();
    cellDims[0] = d[0];
    cellDims[1] = d[1];
    VTKM_ASSERT(ghostField.GetNumberOfValues() == (cellDims[0] * cellDims[1]));
    canDo = CanStrip<2>(ghostField, invoke, removeTypes, range, cellDims);
  }
  else if (cells.CanConvert<vtkm::cont::CellSetStructured<3>>())
  {
    auto cells3D = cells.AsCellSet<vtkm::cont::CellSetStructured<3>>();
    cellDims = cells3D.GetCellDimensions();
    VTKM_ASSERT(ghostField.GetNumberOfValues() == (cellDims[0] * cellDims[1] * cellDims[2]));
    canDo = CanStrip<3>(ghostField, invoke, removeTypes, range, cellDims);
  }

  return canDo;
}

bool DoMapField(vtkm::cont::DataSet& result,
                const vtkm::cont::Field& field,
                const vtkm::worklet::Threshold& worklet)
{
  if (field.IsPointField())
  {
    //we copy the input handle to the result dataset, reusing the metadata
    result.AddField(field);
    return true;
  }
  else if (field.IsCellField())
  {
    return vtkm::filter::MapFieldPermutation(field, worklet.GetValidCellIds(), result);
  }
  else if (field.IsWholeDataSetField())
  {
    result.AddField(field);
    return true;
  }
  else
  {
    return false;
  }
}
} // end anon namespace

namespace vtkm
{
namespace filter
{
namespace entity_extraction
{
//-----------------------------------------------------------------------------
VTKM_CONT GhostCellRemove::GhostCellRemove()
{
  this->SetActiveField(vtkm::cont::GetGlobalGhostCellFieldName());
  this->SetFieldsToPass(vtkm::cont::GetGlobalGhostCellFieldName(),
                        vtkm::filter::FieldSelection::Mode::Exclude);
}

//-----------------------------------------------------------------------------
VTKM_CONT vtkm::cont::DataSet GhostCellRemove::DoExecute(const vtkm::cont::DataSet& input)
{
  const vtkm::cont::UnknownCellSet& cells = input.GetCellSet();
  const vtkm::cont::Field& field =
    (this->GetUseGhostCellsAsField() ? input.GetGhostCellField()
                                     : this->GetFieldFromDataSet(input));

  vtkm::cont::ArrayHandle<vtkm::UInt8> fieldArray;
  vtkm::cont::ArrayCopyShallowIfPossible(field.GetData(), fieldArray);

  //Preserve structured output where possible.
  if (cells.CanConvert<vtkm::cont::CellSetStructured<1>>() ||
      cells.CanConvert<vtkm::cont::CellSetStructured<2>>() ||
      cells.CanConvert<vtkm::cont::CellSetStructured<3>>())
  {
    vtkm::RangeId3 range;
    if (CanDoStructuredStrip(cells, fieldArray, this->Invoke, this->GetTypesToRemove(), range))
    {
      vtkm::filter::entity_extraction::ExtractStructured extract;
      extract.SetInvoker(this->Invoke);
      vtkm::RangeId3 erange(
        range.X.Min, range.X.Max + 2, range.Y.Min, range.Y.Max + 2, range.Z.Min, range.Z.Max + 2);
      vtkm::Id3 sample(1, 1, 1);
      extract.SetVOI(erange);
      extract.SetSampleRate(sample);
      if (this->GetRemoveGhostField())
        extract.SetFieldsToPass(this->GetActiveFieldName(),
                                vtkm::filter::FieldSelection::Mode::Exclude);

      auto output = extract.Execute(input);
      return output;
    }
  }

  vtkm::cont::UnknownCellSet cellOut;
  vtkm::worklet::Threshold worklet;

  cellOut = worklet.Run(
    cells, fieldArray, field.GetAssociation(), RemoveGhostPredicate(this->GetTypesToRemove()));

  auto mapper = [&](auto& result, const auto& f) { DoMapField(result, f, worklet); };
  return this->CreateResult(input, cellOut, mapper);
}

}
}
}

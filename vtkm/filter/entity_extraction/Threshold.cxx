//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/filter/MapFieldPermutation.h>
#include <vtkm/filter/entity_extraction/Threshold.h>
#include <vtkm/filter/entity_extraction/worklet/Threshold.h>

namespace
{
class ThresholdRange
{
public:
  VTKM_CONT
  ThresholdRange(const vtkm::Float64& lower, const vtkm::Float64& upper)
    : Lower(lower)
    , Upper(upper)
  {
  }

  template <typename T>
  VTKM_EXEC bool operator()(const T& value) const
  {

    return value >= static_cast<T>(this->Lower) && value <= static_cast<T>(this->Upper);
  }

  //Needed to work with ArrayHandleVirtual
  template <typename PortalType>
  VTKM_EXEC bool operator()(
    const vtkm::internal::ArrayPortalValueReference<PortalType>& value) const
  {
    using T = typename PortalType::ValueType;
    return value.Get() >= static_cast<T>(this->Lower) && value.Get() <= static_cast<T>(this->Upper);
  }

private:
  vtkm::Float64 Lower;
  vtkm::Float64 Upper;
};

bool DoMapField(vtkm::cont::DataSet& result,
                const vtkm::cont::Field& field,
                const vtkm::worklet::Threshold& worklet)
{
  if (field.IsFieldPoint() || field.IsFieldGlobal())
  {
    //we copy the input handle to the result dataset, reusing the metadata
    result.AddField(field);
    return true;
  }
  else if (field.IsFieldCell())
  {
    return vtkm::filter::MapFieldPermutation(field, worklet.GetValidCellIds(), result);
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
vtkm::cont::DataSet Threshold::DoExecute(const vtkm::cont::DataSet& input)
{
  //get the cells and coordinates of the dataset
  const vtkm::cont::UnknownCellSet& cells = input.GetCellSet();
  const auto& field = this->GetFieldFromDataSet(input);

  ThresholdRange predicate(this->GetLowerThreshold(), this->GetUpperThreshold());
  vtkm::worklet::Threshold worklet;
  vtkm::cont::UnknownCellSet cellOut;

  auto resolveArrayType = [&](const auto& concrete) {
    // Note: there are two overloads of .Run, the first one taking an UncertainCellSet, which is
    // the desired entry point in the following call. The other is a function template on the input
    // CellSet. Without the call to .ResetCellSetList to turn an UnknownCellSet to an UncertainCellSet,
    // the compiler will pick the function template (i.e. wrong overload).
    cellOut = worklet.Run(cells.ResetCellSetList<VTKM_DEFAULT_CELL_SET_LIST>(),
                          concrete,
                          field.GetAssociation(),
                          predicate,
                          this->GetAllInRange());
  };

  field.GetData().CastAndCallForTypes<vtkm::TypeListScalarAll, VTKM_DEFAULT_STORAGE_LIST>(
    resolveArrayType);

  auto mapper = [&](auto& result, const auto& f) { DoMapField(result, f, worklet); };
  return this->CreateResult(input, cellOut, input.GetCoordinateSystems(), mapper);
}
} // namespace entity_extraction
} // namespace filter
} // namespace vtkm

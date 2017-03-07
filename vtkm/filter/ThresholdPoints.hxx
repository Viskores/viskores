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

#include <vtkm/cont/DynamicCellSet.h>

#include <vtkm/worklet/DispatcherMapTopology.h>

namespace
{

// Predicate for values less than minimum
class ValuesBelow
{
public:
  VTKM_CONT
  ValuesBelow(const vtkm::Float32& thresholdValue) :
    ThresholdValue(thresholdValue)
  { }

  template<typename T>
  VTKM_EXEC
  bool operator()(const T& value) const
  {
    return value <= static_cast<T>(this->ThresholdValue);
  }

private:
  vtkm::Float32 ThresholdValue;
};

// Predicate for values greater than maximum
class ValuesAbove
{
public:
  VTKM_CONT
  ValuesAbove(const vtkm::Float32& thresholdValue) :
    ThresholdValue(thresholdValue)
  { }

  template<typename T>
  VTKM_EXEC
  bool operator()(const T& value) const
  {
    return value >= static_cast<T>(this->ThresholdValue);
  }

private:
  vtkm::Float32 ThresholdValue;
};

// Predicate for values between minimum and maximum

class ValuesBetween
{
public:
  VTKM_CONT
  ValuesBetween(const vtkm::Float64& lower,
                const vtkm::Float64& upper) :
    Lower(lower),
    Upper(upper)
  { }

  template<typename T>
  VTKM_EXEC
  bool operator()(const T& value) const
  {
    return value >= static_cast<T>(this->Lower) &&
           value <= static_cast<T>(this->Upper);
  }

private:
  vtkm::Float64 Lower;
  vtkm::Float64 Upper;
};
}


namespace vtkm {
namespace filter {

//-----------------------------------------------------------------------------
inline VTKM_CONT
ThresholdPoints::ThresholdPoints():
  vtkm::filter::FilterDataSetWithField<ThresholdPoints>(),
  LowerValue(0),
  UpperValue(0)
{
}

//-----------------------------------------------------------------------------
template<typename T,
         typename StorageType,
         typename DerivedPolicy,
         typename DeviceAdapter>
inline VTKM_CONT
vtkm::filter::ResultDataSet ThresholdPoints::DoExecute(
                                                 const vtkm::cont::DataSet& input,
                                                 const vtkm::cont::ArrayHandle<T, StorageType>& field,
                                                 const vtkm::filter::FieldMetadata& fieldMeta,
                                                 const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
                                                 const DeviceAdapter& device)
{
  //get the cells and coordinates of the dataset
  const vtkm::cont::DynamicCellSet& cells =
                  input.GetCellSet(this->GetActiveCellSetIndex());

  if (fieldMeta.IsPointField() == false)
  {
    //todo: we need to mark this as a failure of input, not a failure of the algorithm
    return vtkm::filter::ResultDataSet();
  }

  ValuesBetween predicate( this->GetLowerThreshold(), this->GetUpperThreshold() );

  // output dataset
  vtkm::cont::DataSet output;
  vtkm::cont::CellSetSingleType<> outCellSet;

  // output data set gets cell set from the worklet
  vtkm::worklet::ThresholdPoints worklet;
  outCellSet = worklet.Run(vtkm::filter::ApplyPolicy(cells, policy),
                           field,
                           predicate,
                           device);
  output.AddCellSet(outCellSet);

  // add input dataset coordinates to the output dataset
  output.AddCoordinateSystem(
          input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()) );

  return vtkm::filter::ResultDataSet(output);
}

//-----------------------------------------------------------------------------
template<typename T,
         typename StorageType,
         typename DerivedPolicy,
         typename DeviceAdapter>
inline VTKM_CONT
bool ThresholdPoints::DoMapField(
                           vtkm::filter::ResultDataSet& result,
                           const vtkm::cont::ArrayHandle<T, StorageType>& input,
                           const vtkm::filter::FieldMetadata& fieldMeta,
                           const vtkm::filter::PolicyBase<DerivedPolicy>&,
                           const DeviceAdapter&)
{
  // point data is copied as is because it was not collapsed
  if(fieldMeta.IsPointField())
  {
    result.GetDataSet().AddField(fieldMeta.AsField(input));
    return true;
  }
  
  // cell data does not apply
  return false;
}

}
}

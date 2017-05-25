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

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline VTKM_CONT ExternalFaces::ExternalFaces()
  : vtkm::filter::FilterDataSet<ExternalFaces>()
  , CompactPoints(false)
{
}

namespace
{

template <typename BasePolicy>
struct CellSetExplicitPolicy : public BasePolicy
{
  using AllCellSetList = vtkm::cont::CellSetListTagExplicitDefault;
};

template <typename DerivedPolicy>
inline vtkm::filter::PolicyBase<CellSetExplicitPolicy<DerivedPolicy>> GetCellSetExplicitPolicy(
  const vtkm::filter::PolicyBase<DerivedPolicy>&)
{
  return vtkm::filter::PolicyBase<CellSetExplicitPolicy<DerivedPolicy>>();
}

} // anonymous namespace

//-----------------------------------------------------------------------------
template <typename DerivedPolicy, typename DeviceAdapter>
inline VTKM_CONT vtkm::filter::ResultDataSet ExternalFaces::DoExecute(
  const vtkm::cont::DataSet& input, const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
  const DeviceAdapter&)
{
  //1. extract the cell set
  const vtkm::cont::DynamicCellSet& cells = input.GetCellSet(this->GetActiveCellSetIndex());

  //2. using the policy convert the dynamic cell set, and run the
  // external faces worklet
  vtkm::cont::CellSetExplicit<> outCellSet(cells.GetName());
  vtkm::worklet::ExternalFaces exfaces;
  exfaces.Run(vtkm::filter::ApplyPolicyUnstructured(cells, policy), outCellSet, DeviceAdapter());

  //3. create the output dataset
  vtkm::cont::DataSet output;
  output.AddCellSet(outCellSet);
  output.AddCoordinateSystem(input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()));

  if (this->CompactPoints)
  {
    this->Compactor.SetCompactPointFields(true);
    return this->Compactor.DoExecute(output, GetCellSetExplicitPolicy(policy), DeviceAdapter());
  }
  else
  {
    return vtkm::filter::ResultDataSet(output);
  }
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
inline VTKM_CONT bool ExternalFaces::DoMapField(
  vtkm::filter::ResultDataSet& result, const vtkm::cont::ArrayHandle<T, StorageType>& input,
  const vtkm::filter::FieldMetadata& fieldMeta,
  const vtkm::filter::PolicyBase<DerivedPolicy>& policy, const DeviceAdapter&)
{
  if (fieldMeta.IsPointField())
  {
    if (this->CompactPoints)
    {
      return this->Compactor.DoMapField(result, input, fieldMeta, policy, DeviceAdapter());
    }
    else
    {
      result.GetDataSet().AddField(fieldMeta.AsField(input));
      return true;
    }
  }

  return false;
}
}
}

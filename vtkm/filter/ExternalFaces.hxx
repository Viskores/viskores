//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
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
  , Worklet()
{
  this->SetPassPolyData(true);
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
inline VTKM_CONT vtkm::cont::DataSet ExternalFaces::DoExecute(
  const vtkm::cont::DataSet& input,
  const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
  const DeviceAdapter&)
{
  //1. extract the cell set
  const vtkm::cont::DynamicCellSet& cells = input.GetCellSet(this->GetActiveCellSetIndex());

  //2. using the policy convert the dynamic cell set, and run the
  // external faces worklet
  vtkm::cont::CellSetExplicit<> outCellSet(cells.GetName());

  if (cells.IsSameType(vtkm::cont::CellSetStructured<3>()))
  {
    this->Worklet.Run(cells.Cast<vtkm::cont::CellSetStructured<3>>(),
                      input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()),
                      outCellSet,
                      DeviceAdapter());
  }
  else
  {
    this->Worklet.Run(
      vtkm::filter::ApplyPolicyUnstructured(cells, policy), outCellSet, DeviceAdapter());
  }

  //3. Check the fields of the dataset to see what kinds of fields are present so
  //   we can free the cell mapping array if it won't be needed.
  const vtkm::Id numFields = input.GetNumberOfFields();
  bool hasCellFields = false;
  for (vtkm::Id fieldIdx = 0; fieldIdx < numFields && !hasCellFields; ++fieldIdx)
  {
    auto f = input.GetField(fieldIdx);
    if (f.GetAssociation() == vtkm::cont::Field::Association::CELL_SET)
    {
      hasCellFields = true;
    }
  }

  if (!hasCellFields)
  {
    this->Worklet.ReleaseCellMapArrays();
  }

  //4. create the output dataset
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
    return output;
  }
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
inline VTKM_CONT bool ExternalFaces::DoMapField(
  vtkm::cont::DataSet& result,
  const vtkm::cont::ArrayHandle<T, StorageType>& input,
  const vtkm::filter::FieldMetadata& fieldMeta,
  const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
  const DeviceAdapter& device)
{
  if (fieldMeta.IsPointField())
  {
    if (this->CompactPoints)
    {
      return this->Compactor.DoMapField(result, input, fieldMeta, policy, DeviceAdapter());
    }
    else
    {
      result.AddField(fieldMeta.AsField(input));
      return true;
    }
  }
  else if (fieldMeta.IsCellField())
  {
    vtkm::cont::ArrayHandle<T> fieldArray;
    fieldArray = this->Worklet.ProcessCellField(input, device);
    result.AddField(fieldMeta.AsField(fieldArray));
    return true;
  }


  return false;
}
}
}

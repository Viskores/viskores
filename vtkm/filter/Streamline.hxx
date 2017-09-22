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

#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/worklet/particleadvection/GridEvaluators.h>
#include <vtkm/worklet/particleadvection/Integrators.h>
#include <vtkm/worklet/particleadvection/Particles.h>

namespace vtkm
{
namespace filter
{

namespace
{

template <typename CellSetList>
bool IsCellSupported(const vtkm::cont::DynamicCellSetBase<CellSetList>& cellset)
{
  //We only support 3D structured for now.
  if (cellset.template IsType<vtkm::cont::CellSetStructured<3>>())
    return true;
  return false;
}
} // anonymous namespace

//-----------------------------------------------------------------------------
inline VTKM_CONT Streamline::Streamline()
  : vtkm::filter::FilterDataSetWithField<Streamline>()
  , Worklet()
{
}

//-----------------------------------------------------------------------------
inline VTKM_CONT void Streamline::SetSeeds(
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>>& seeds)
{
  this->Seeds = seeds;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
inline VTKM_CONT vtkm::filter::Result Streamline::DoExecute(
  const vtkm::cont::DataSet& input,
  const vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, StorageType>& field,
  const vtkm::filter::FieldMetadata& fieldMeta,
  const vtkm::filter::PolicyBase<DerivedPolicy>&,
  const DeviceAdapter& device)
{
  //Check for some basics.
  if (this->Seeds.GetNumberOfValues() == 0)
    return vtkm::filter::Result();

  const vtkm::cont::DynamicCellSet& cells = input.GetCellSet(this->GetActiveCellSetIndex());
  const vtkm::cont::CoordinateSystem& coords =
    input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());

  if (!IsCellSupported(cells))
    return vtkm::filter::Result();

  if (!fieldMeta.IsPointField())
    return vtkm::filter::Result();

  //todo: add check for rectilinear.
  typedef vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>> FieldHandle;
  typedef
    typename FieldHandle::template ExecutionTypes<DeviceAdapter>::PortalConst FieldPortalConstType;
  typedef vtkm::worklet::particleadvection::UniformGridEvaluate<FieldPortalConstType,
                                                                T,
                                                                DeviceAdapter>
    RGEvalType;
  typedef vtkm::worklet::particleadvection::RK4Integrator<RGEvalType, T> RK4RGType;

  //RGEvalType eval(input.GetCoordinateSystem(), input.GetCellSet(0), field);
  RGEvalType eval(coords, cells, field);
  RK4RGType rk4(eval, static_cast<vtkm::FloatDefault>(this->StepSize));

  vtkm::worklet::Streamline streamline;
  vtkm::worklet::StreamlineResult<T> res;
  res = Worklet.Run(rk4, this->Seeds, this->NumberOfSteps, device);

  return vtkm::filter::Result();
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
inline VTKM_CONT bool Streamline::DoMapField(
  vtkm::filter::Result& result,
  const vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, StorageType>& input,
  const vtkm::filter::FieldMetadata& fieldMeta,
  const vtkm::filter::PolicyBase<DerivedPolicy>&,
  const DeviceAdapter& device)
{
  return true;


#if 0
  vtkm::cont::ArrayHandle<T> fieldArray;
  if (fieldMeta.IsPointField())
  {
    fieldArray = this->Worklet.ProcessPointField(input, device);
  }
  else if (fieldMeta.IsCellField())
  {
    fieldArray = this->Worklet.ProcessCellField(input, device);
  }
  else
  {
    return false;
  }

  //use the same meta data as the input so we get the same field name, etc.
  result.GetDataSet().AddField(fieldMeta.AsField(fieldArray));
  return true;
#endif
}
}
} // namespace vtkm::filter

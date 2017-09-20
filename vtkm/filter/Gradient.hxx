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

#include <vtkm/cont/DynamicCellSet.h>
#include <vtkm/worklet/Gradient.h>

namespace
{
//-----------------------------------------------------------------------------
template <typename HandleType>
inline void add_field(vtkm::filter::Result& result,
                      const HandleType& handle,
                      const std::string name)
{
  const vtkm::cont::Field::AssociationEnum assoc = result.GetField().GetAssociation();
  if ((assoc == vtkm::cont::Field::ASSOC_WHOLE_MESH) || (assoc == vtkm::cont::Field::ASSOC_POINTS))
  {
    vtkm::cont::Field field(name, assoc, handle);
    result.GetDataSet().AddField(field);
  }
  else
  {
    vtkm::cont::Field field(name, assoc, result.GetField().GetAssocCellSet(), handle);
    result.GetDataSet().AddField(field);
  }
}

//-----------------------------------------------------------------------------
template <typename T, typename S, typename DeviceAdapter>
inline void transpose_3x3(vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Vec<T, 3>, 3>, S>& field,
                          DeviceAdapter adapter)
{
  vtkm::worklet::gradient::Transpose3x3<T> transpose;
  transpose.Run(field, adapter);
}

//-----------------------------------------------------------------------------
template <typename T, typename S, typename DeviceAdapter>
inline void transpose_3x3(vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, S>&, DeviceAdapter)
{ //This is not a 3x3 matrix so no transpose needed
}

} //namespace

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
Gradient::Gradient()
  : ComputePointGradient(false)
  , ComputeVorticity(false)
  , ComputeQCriterion(false)
  , StoreGradient(true)
  , RowOrdering(true)
  , GradientsName("Gradients")
  , DivergenceName("Divergence")
  , VorticityName("Vorticity")
  , QCriterionName("QCriterion")
{
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
inline vtkm::filter::Result Gradient::DoExecute(
  const vtkm::cont::DataSet& input,
  const vtkm::cont::ArrayHandle<T, StorageType>& inField,
  const vtkm::filter::FieldMetadata& fieldMetadata,
  const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
  const DeviceAdapter& adapter)
{
  if (!fieldMetadata.IsPointField())
  {
    //we currently only support point fields, as we need to write the
    //worklet to efficiently map a cell field to the points of a cell
    //without doing a memory explosion
    return vtkm::filter::Result();
  }

  const vtkm::cont::DynamicCellSet& cells = input.GetCellSet(this->GetActiveCellSetIndex());
  const vtkm::cont::CoordinateSystem& coords =
    input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());

  std::string outputName = this->GetOutputFieldName();
  if (outputName.empty())
  {
    outputName = this->GradientsName;
  }

  //todo: we need to ask the policy what storage type we should be using
  //If the input is implicit, we should know what to fall back to
  vtkm::worklet::GradientOutputFields<T> gradientfields(this->GetComputeGradient(),
                                                        this->GetComputeDivergence(),
                                                        this->GetComputeVorticity(),
                                                        this->GetComputeQCriterion());
  vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>> outArray;
  if (this->ComputePointGradient)
  {
    vtkm::worklet::PointGradient gradient;
    outArray = gradient.Run(vtkm::filter::ApplyPolicy(cells, policy),
                            vtkm::filter::ApplyPolicy(coords, policy),
                            inField,
                            gradientfields,
                            adapter);
  }
  else
  {
    vtkm::worklet::CellGradient gradient;
    outArray = gradient.Run(vtkm::filter::ApplyPolicy(cells, policy),
                            vtkm::filter::ApplyPolicy(coords, policy),
                            inField,
                            gradientfields,
                            adapter);
  }
  if (!this->RowOrdering)
  {
    transpose_3x3(outArray, adapter);
  }

  VTKM_CONSTEXPR bool isVector = std::is_same<typename vtkm::VecTraits<T>::HasMultipleComponents,
                                              vtkm::VecTraitsTagMultipleComponents>::value;

  vtkm::cont::Field::AssociationEnum fieldAssociation(this->ComputePointGradient
                                                        ? vtkm::cont::Field::ASSOC_POINTS
                                                        : vtkm::cont::Field::ASSOC_CELL_SET);
  vtkm::filter::Result result(input, outArray, outputName, fieldAssociation, cells.GetName());

  if (this->GetComputeDivergence() && isVector)
  {
    add_field(result, gradientfields.Divergence, this->GetDivergenceName());
  }
  if (this->GetComputeVorticity() && isVector)
  {
    add_field(result, gradientfields.Vorticity, this->GetVorticityName());
  }
  if (this->GetComputeQCriterion() && isVector)
  {
    add_field(result, gradientfields.QCriterion, this->GetQCriterionName());
  }
  return result;
}
}
} // namespace vtkm::filter

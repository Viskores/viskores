//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_Gradient_hxx
#define vtk_m_filter_Gradient_hxx

#include <vtkm/cont/DynamicCellSet.h>
#include <vtkm/cont/ErrorFilterExecution.h>

#include <vtkm/worklet/Gradient.h>

namespace
{
//-----------------------------------------------------------------------------
template <typename T, typename S>
inline void transpose_3x3(vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Vec<T, 3>, 3>, S>& field)
{
  vtkm::worklet::gradient::Transpose3x3<T> transpose;
  transpose.Run(field);
}

//-----------------------------------------------------------------------------
template <typename T, typename S>
inline void transpose_3x3(vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, S>&)
{ //This is not a 3x3 matrix so no transpose needed
}

} //namespace

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline vtkm::cont::DataSet Gradient::DoExecute(
  const vtkm::cont::DataSet& input,
  const vtkm::cont::ArrayHandle<T, StorageType>& inField,
  const vtkm::filter::FieldMetadata& fieldMetadata,
  const vtkm::filter::PolicyBase<DerivedPolicy>& policy)
{
  if (!fieldMetadata.IsPointField())
  {
    throw vtkm::cont::ErrorFilterExecution("Point field expected.");
  }

  const vtkm::cont::DynamicCellSet& cells = input.GetCellSet();
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
    outArray = gradient.Run(
      vtkm::filter::ApplyPolicyCellSet(cells, policy), coords, inField, gradientfields);
  }
  else
  {
    vtkm::worklet::CellGradient gradient;
    outArray = gradient.Run(
      vtkm::filter::ApplyPolicyCellSet(cells, policy), coords, inField, gradientfields);
  }
  if (!this->RowOrdering)
  {
    transpose_3x3(outArray);
  }

  constexpr bool isVector = std::is_same<typename vtkm::VecTraits<T>::HasMultipleComponents,
                                         vtkm::VecTraitsTagMultipleComponents>::value;

  vtkm::cont::Field::Association fieldAssociation(this->ComputePointGradient
                                                    ? vtkm::cont::Field::Association::POINTS
                                                    : vtkm::cont::Field::Association::CELL_SET);
  vtkm::cont::DataSet result;
  result.CopyStructure(input);
  result.AddField(vtkm::cont::Field{ outputName, fieldAssociation, outArray });

  if (this->GetComputeDivergence() && isVector)
  {
    result.AddField(
      vtkm::cont::Field{ this->GetDivergenceName(), fieldAssociation, gradientfields.Divergence });
  }
  if (this->GetComputeVorticity() && isVector)
  {
    result.AddField(
      vtkm::cont::Field{ this->GetVorticityName(), fieldAssociation, gradientfields.Vorticity });
  }
  if (this->GetComputeQCriterion() && isVector)
  {
    result.AddField(
      vtkm::cont::Field{ this->GetQCriterionName(), fieldAssociation, gradientfields.QCriterion });
  }
  return result;
}
}
} // namespace vtkm::filter

#endif

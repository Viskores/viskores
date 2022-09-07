//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/cont/UnknownCellSet.h>
#include <vtkm/filter/vector_analysis/Gradient.h>
#include <vtkm/filter/vector_analysis/worklet/Gradient.h>

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
namespace vector_analysis
{
//-----------------------------------------------------------------------------
vtkm::cont::DataSet Gradient::DoExecute(const vtkm::cont::DataSet& inputDataSet)
{
  const auto& field = this->GetFieldFromDataSet(inputDataSet);
  if (!field.IsPointField())
  {
    throw vtkm::cont::ErrorFilterExecution("Point field expected.");
  }

  const bool isVector = field.GetData().GetNumberOfComponents() == 3;
  if (GetComputeQCriterion() && !isVector)
  {
    throw vtkm::cont::ErrorFilterExecution("scalar gradients can't generate qcriterion");
  }
  if (GetComputeVorticity() && !isVector)
  {
    throw vtkm::cont::ErrorFilterExecution("scalar gradients can't generate vorticity");
  }

  const vtkm::cont::UnknownCellSet& inputCellSet = inputDataSet.GetCellSet();
  const vtkm::cont::CoordinateSystem& coords =
    inputDataSet.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());

  vtkm::cont::UnknownArrayHandle gradientArray;
  vtkm::cont::UnknownArrayHandle divergenceArray;
  vtkm::cont::UnknownArrayHandle vorticityArray;
  vtkm::cont::UnknownArrayHandle qcriterionArray;

  // TODO: there are a humungous number of (weak) symbols in the .o file. Investigate if
  //  they are all legit.

  auto resolveType = [&](const auto& concrete) {
    // use std::decay to remove const ref from the decltype of concrete.
    using T = typename std::decay_t<decltype(concrete)>::ValueType;
    vtkm::worklet::GradientOutputFields<T> gradientfields(this->GetComputeGradient(),
                                                          this->GetComputeDivergence(),
                                                          this->GetComputeVorticity(),
                                                          this->GetComputeQCriterion());

    vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>> result;
    if (this->ComputePointGradient)
    {
      vtkm::worklet::PointGradient gradient;
      result = gradient.Run(inputCellSet, coords, concrete, gradientfields);
    }
    else
    {
      vtkm::worklet::CellGradient gradient;
      result = gradient.Run(inputCellSet, coords, concrete, gradientfields);
    }
    if (!this->RowOrdering)
    {
      transpose_3x3(result);
    }

    gradientArray = result;
    divergenceArray = gradientfields.Divergence;
    vorticityArray = gradientfields.Vorticity;
    qcriterionArray = gradientfields.QCriterion;
  };

  using SupportedTypes = vtkm::List<vtkm::Float32, vtkm::Float64, vtkm::Vec3f_32, vtkm::Vec3f_64>;
  field.GetData().CastAndCallForTypesWithFloatFallback<SupportedTypes, VTKM_DEFAULT_STORAGE_LIST>(
    resolveType);

  // This copies the CellSet and Fields to be passed from inputDataSet to outputDataSet
  vtkm::cont::DataSet outputDataSet = this->CreateResult(inputDataSet);

  std::string outputName = this->GetOutputFieldName();
  if (outputName.empty())
  {
    outputName = this->GradientsName;
  }

  vtkm::cont::Field::Association fieldAssociation(this->ComputePointGradient
                                                    ? vtkm::cont::Field::Association::Points
                                                    : vtkm::cont::Field::Association::Cells);

  outputDataSet.AddField(vtkm::cont::Field{ outputName, fieldAssociation, gradientArray });

  if (this->GetComputeDivergence() && isVector)
  {
    outputDataSet.AddField(
      vtkm::cont::Field{ this->GetDivergenceName(), fieldAssociation, divergenceArray });
  }
  if (this->GetComputeVorticity() && isVector)
  {
    outputDataSet.AddField(
      vtkm::cont::Field{ this->GetVorticityName(), fieldAssociation, vorticityArray });
  }
  if (this->GetComputeQCriterion() && isVector)
  {
    outputDataSet.AddField(
      vtkm::cont::Field{ this->GetQCriterionName(), fieldAssociation, qcriterionArray });
  }
  return outputDataSet;
}
} // namespace vector_analysis
} // namespace filter
} // namespace vtkm

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//=============================================================================
#ifndef vtk_m_filter_ComputeMoments_hxx
#define vtk_m_filter_ComputeMoments_hxx

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/worklet/moments/ComputeMoments.h>

namespace vtkm
{
namespace filter
{

VTKM_CONT ComputeMoments::ComputeMoments()
{
  this->SetOutputFieldName("moments_");
}

template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet ComputeMoments::DoExecute(
  const vtkm::cont::DataSet& input,
  const vtkm::cont::ArrayHandle<T, StorageType>& field,
  const vtkm::filter::FieldMetadata& fieldMetadata,
  const vtkm::filter::PolicyBase<DerivedPolicy>&)
{
  if (fieldMetadata.GetAssociation() != vtkm::cont::Field::Association::POINTS)
  {
    throw vtkm::cont::ErrorBadValue("Active field for ComputeMoments must be a point field.");
  }

  vtkm::cont::DataSet output;
  output.CopyStructure(input);

  auto worklet = vtkm::worklet::moments::ComputeMoments(this->Spacing, this->Radius);

  worklet.Run(input.GetCellSet(), field, this->Order, output);

  return output;
}
}
}
#endif //vtk_m_filter_ComputeMoments_hxx

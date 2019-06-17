//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//=======================================================================
#ifndef vtk_m_filter_ComputeMoments_hxx
#define vtk_m_filter_ComputeMoments_hxx

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/filter/internal/CreateResult.h>
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
  const vtkm::filter::PolicyBase<DerivedPolicy>& policy)
{
  if (fieldMetadata.GetAssociation() != vtkm::cont::Field::Association::POINTS)
  {
    throw vtkm::cont::ErrorBadValue("Active field for ComputeMoments must be a point field.");
  }

  vtkm::cont::DataSet output = internal::CreateResult(input);
  auto worklet = vtkm::worklet::moments::ComputeMoments(this->Radius);

  // FIXME: 3D with i, j, k
  for (int order = 0; order <= this->Order; ++order)
  {
    for (int p = order; p >= 0; --p)
    {
      vtkm::cont::ArrayHandle<T> moments;

      worklet.Run(
        vtkm::filter::ApplyPolicy(input.GetCellSet(this->GetActiveCellSetIndex()), policy),
        field,
        p,
        Order - p,
        moments);

      std::string fieldName = "index";
      // names for i and j
      for (int j = 0; j < p; ++j)
        fieldName += std::to_string(0);
      for (int j = 0; j < order - p; ++j)
        fieldName += std::to_string(1);
      // TODO: add the same for k

      vtkm::cont::Field momentsField(fieldName, vtkm::cont::Field::Association::POINTS, moments);
      output.AddField(momentsField);
    }
  }

  return output;
}
}
}
#endif //vtk_m_filter_ComputeMoments_hxx

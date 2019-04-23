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
  const vtkm::filter::PolicyBase<DerivedPolicy>&)
{
  if (fieldMetadata.GetAssociation() != vtkm::cont::Field::Association::POINTS)
  {
    throw vtkm::cont::ErrorBadValue("Active field for ComputeMoments must be a point field.");
  }

  vtkm::cont::DataSet output = internal::CreateResult(input);

  for (int i = 0; i <= this->order; ++i)
  {
    for (int p = i; p >= 0; --p)
    {
      vtkm::cont::ArrayHandle<T> moments;

      using DispatcherType =
        vtkm::worklet::DispatcherPointNeighborhood<vtkm::worklet::moments::ComputeMoments>;
      DispatcherType dispatcher(vtkm::worklet::moments::ComputeMoments{ this->radius, p, i - p });
      dispatcher.SetDevice(vtkm::cont::DeviceAdapterTagSerial());
      dispatcher.Invoke(input.GetCellSet(0), field, moments);

      std::string fieldName = "moments_";
      fieldName += std::to_string(p) + std::to_string(i - p);

      vtkm::cont::Field momentsField(fieldName, vtkm::cont::Field::Association::POINTS, moments);
      output.AddField(momentsField);
    }
  }

  return output;
}
}
}
#endif //vtk_m_filter_ComputeMoments_hxx

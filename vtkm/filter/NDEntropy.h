//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_NDEntropy_h
#define vtk_m_filter_NDEntropy_h

#include <vtkm/filter/FilterDataSet.h>

namespace vtkm
{
namespace filter
{
/// \brief Calculate the entropy of input N-Dims fields
///
/// This filter calculate the entropy of input N-Dims fields.
///
class NDEntropy : public vtkm::filter::FilterDataSet<NDEntropy>
{
public:
  VTKM_CONT
  NDEntropy();

  VTKM_CONT
  void AddFieldAndBin(const std::string& fieldName, vtkm::Id numOfBins);

  template <typename Policy>
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& inData,
                                          vtkm::filter::PolicyBase<Policy> policy);

  template <typename T, typename StorageType, typename DerivedPolicy>
  VTKM_CONT bool DoMapField(vtkm::cont::DataSet& result,
                            const vtkm::cont::ArrayHandle<T, StorageType>& input,
                            const vtkm::filter::FieldMetadata& fieldMeta,
                            vtkm::filter::PolicyBase<DerivedPolicy> policy);

private:
  std::vector<vtkm::Id> NumOfBins;
  std::vector<std::string> FieldNames;
};
}
} // namespace vtkm::filter

#include <vtkm/filter/NDEntropy.hxx>

#endif //vtk_m_filter_NDEntropy_h

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_Entropy_h
#define vtk_m_filter_Entropy_h

#include <vtkm/filter/FilterField.h>

namespace vtkm
{
namespace filter
{

/// \brief Construct the entropy histogram of a given Field
///
/// Construct a histogram which is used to compute the entropy with a default of 10 bins
///
class Entropy : public vtkm::filter::FilterField<Entropy>
{
public:
  //currently the Entropy filter only works on scalar data.
  using SupportedTypes = TypeListScalarAll;

  //Construct a histogram which is used to compute the entropy with a default of 10 bins
  VTKM_CONT
  Entropy();

  VTKM_CONT
  void SetNumberOfBins(vtkm::Id count) { this->NumberOfBins = count; }

  template <typename T, typename StorageType, typename DerivedPolicy>
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input,
                                          const vtkm::cont::ArrayHandle<T, StorageType>& field,
                                          const vtkm::filter::FieldMetadata& fieldMeta,
                                          const vtkm::filter::PolicyBase<DerivedPolicy>& policy);

private:
  vtkm::Id NumberOfBins;
};
}
} // namespace vtkm::filter


#include <vtkm/filter/Entropy.hxx>

#endif // vtk_m_filter_Entropy_h

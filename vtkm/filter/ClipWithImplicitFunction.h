//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_ClipWithImplicitFunction_h
#define vtk_m_filter_ClipWithImplicitFunction_h

#include <vtkm/cont/ImplicitFunctionHandle.h>
#include <vtkm/filter/FilterDataSet.h>
#include <vtkm/worklet/Clip.h>

namespace vtkm
{
namespace filter
{

/// \brief Clip a dataset using an implicit function
///
/// Clip a dataset using a given implicit function value, such as vtkm::Sphere
/// or vtkm::Frustum.
/// The resulting geometry will not be water tight.
class ClipWithImplicitFunction : public vtkm::filter::FilterDataSet<ClipWithImplicitFunction>
{
public:
  ClipWithImplicitFunction();

  void SetImplicitFunction(const vtkm::cont::ImplicitFunctionHandle& func)
  {
    this->Function = func;
  }

  void SetInvertClip(bool invert) { this->Invert = invert; }

  const vtkm::cont::ImplicitFunctionHandle& GetImplicitFunction() const { return this->Function; }

  template <typename DerivedPolicy>
  vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input,
                                const vtkm::filter::PolicyBase<DerivedPolicy>& policy);

  //Map a new field onto the resulting dataset after running the filter.
  //This call is only valid after Execute has been called.
  template <typename T, typename StorageType, typename DerivedPolicy>
  bool DoMapField(vtkm::cont::DataSet& result,
                  const vtkm::cont::ArrayHandle<T, StorageType>& input,
                  const vtkm::filter::FieldMetadata& fieldMeta,
                  const vtkm::filter::PolicyBase<DerivedPolicy>& policy);

private:
  vtkm::cont::ImplicitFunctionHandle Function;
  vtkm::worklet::Clip Worklet;
  bool Invert;
};
}
} // namespace vtkm::filter

#include <vtkm/filter/ClipWithImplicitFunction.hxx>

#endif // vtk_m_filter_ClipWithImplicitFunction_h

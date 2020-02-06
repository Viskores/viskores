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

#include <vtkm/filter/vtkm_filter_export.h>

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
class VTKM_ALWAYS_EXPORT ClipWithImplicitFunction
  : public vtkm::filter::FilterDataSet<ClipWithImplicitFunction>
{
public:
  void SetImplicitFunction(const vtkm::cont::ImplicitFunctionHandle& func)
  {
    this->Function = func;
  }

  void SetInvertClip(bool invert) { this->Invert = invert; }

  const vtkm::cont::ImplicitFunctionHandle& GetImplicitFunction() const { return this->Function; }

  template <typename DerivedPolicy>
  vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input,
                                vtkm::filter::PolicyBase<DerivedPolicy> policy);

  VTKM_FILTER_EXPORT VTKM_CONT bool MapFieldOntoOutput(vtkm::cont::DataSet& result,
                                                       const vtkm::cont::Field& field);

  template <typename DerivedPolicy>
  VTKM_CONT bool MapFieldOntoOutput(vtkm::cont::DataSet& result,
                                    const vtkm::cont::Field& field,
                                    vtkm::filter::PolicyBase<DerivedPolicy> policy)
  {
    if (field.IsFieldPoint())
    {
      // DIE, POLICIES, DIE!
      return this->FilterDataSet<ClipWithImplicitFunction>::MapFieldOntoOutput(
        result, field, policy);
    }
    else
    {
      return this->MapFieldOntoOutput(result, field);
    }
  }

  //Map a new field onto the resulting dataset after running the filter.
  //This call is only valid after Execute has been called.
  template <typename T, typename StorageType, typename DerivedPolicy>
  bool DoMapField(vtkm::cont::DataSet& result,
                  const vtkm::cont::ArrayHandle<T, StorageType>& input,
                  const vtkm::filter::FieldMetadata& fieldMeta,
                  vtkm::filter::PolicyBase<DerivedPolicy>)
  {
    // All other conditions should be handled by MapFieldOntoOutput directly.
    VTKM_ASSERT(fieldMeta.IsPointField());

    vtkm::cont::ArrayHandle<T> output;
    output = this->Worklet.ProcessPointField(input);

    //use the same meta data as the input so we get the same field name, etc.
    result.AddField(fieldMeta.AsField(output));
    return true;
  }

private:
  vtkm::cont::ImplicitFunctionHandle Function;
  vtkm::worklet::Clip Worklet;
  bool Invert = false;
};

#ifndef vtkm_filter_ClipWithImplicitFunction_cxx
VTKM_FILTER_EXPORT_EXECUTE_METHOD(ClipWithImplicitFunction);
#endif
}
} // namespace vtkm::filter

#include <vtkm/filter/ClipWithImplicitFunction.hxx>

#endif // vtk_m_filter_ClipWithImplicitFunction_h

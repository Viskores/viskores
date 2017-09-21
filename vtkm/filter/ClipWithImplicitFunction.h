//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2017 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2017 UT-Battelle, LLC.
//  Copyright 2017 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#ifndef vtk_m_filter_ClipWithImplicitFunction_h
#define vtk_m_filter_ClipWithImplicitFunction_h

#include <vtkm/cont/ImplicitFunction.h>
#include <vtkm/filter/FilterDataSet.h>
#include <vtkm/worklet/Clip.h>

#include <memory>

namespace vtkm
{
namespace filter
{

/// \brief Clip a dataset using an implicit function
///
/// Clip a dataset using a given implicit function value, such as vtkm::cont::Sphere
/// or vtkm::cont::Frustum.
/// The resulting geometry will not be water tight.
class ClipWithImplicitFunction : public vtkm::filter::FilterDataSet<ClipWithImplicitFunction>
{
public:
  template <typename ImplicitFunctionType, typename DerivedPolicy>
  void SetImplicitFunction(const std::shared_ptr<ImplicitFunctionType>& func,
                           const vtkm::filter::PolicyBase<DerivedPolicy>& policy);

  template <typename ImplicitFunctionType>
  void SetImplicitFunction(const std::shared_ptr<ImplicitFunctionType>& func)
  {
    this->Function = func;
  }

  std::shared_ptr<vtkm::cont::ImplicitFunction> GetImplicitFunction() const
  {
    return this->Function;
  }

  template <typename DerivedPolicy, typename DeviceAdapter>
  vtkm::filter::Result DoExecute(const vtkm::cont::DataSet& input,
                                 const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
                                 const DeviceAdapter& tag);

  //Map a new field onto the resulting dataset after running the filter.
  //This call is only valid after Execute has been called.
  template <typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
  bool DoMapField(vtkm::filter::Result& result,
                  const vtkm::cont::ArrayHandle<T, StorageType>& input,
                  const vtkm::filter::FieldMetadata& fieldMeta,
                  const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
                  const DeviceAdapter& tag);

private:
  std::shared_ptr<vtkm::cont::ImplicitFunction> Function;
  vtkm::worklet::Clip Worklet;
};
}
} // namespace vtkm::filter

#include <vtkm/filter/ClipWithImplicitFunction.hxx>

#endif // vtk_m_filter_ClipWithImplicitFunction_h

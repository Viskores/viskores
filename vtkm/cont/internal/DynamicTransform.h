//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_internal_DynamicTransform_h
#define vtk_m_cont_internal_DynamicTransform_h

#include <vtkm/cont/CastAndCall.h>
#include <vtkm/internal/IndexTag.h>

#include <utility>

namespace vtkm
{
namespace cont
{
namespace internal
{
/// This functor can be used as the transform in the \c DynamicTransformCont
/// method of \c FunctionInterface. It will allow dynamic objects like
/// \c DynamicArray to be cast to their concrete types for templated operation.
///
struct DynamicTransform
{
  template <typename InputType, typename ContinueFunctor, vtkm::IdComponent Index>
  VTKM_CONT void operator()(const InputType& input,
                            const ContinueFunctor& continueFunc,
                            vtkm::internal::IndexTag<Index>) const
  {
    this->DoTransform(
      input,
      continueFunc,
      typename vtkm::cont::internal::DynamicTransformTraits<InputType>::DynamicTag());
  }

private:
  template <typename InputType, typename ContinueFunctor>
  VTKM_CONT void DoTransform(const InputType& input,
                             const ContinueFunctor& continueFunc,
                             vtkm::cont::internal::DynamicTransformTagStatic) const
  {
    continueFunc(input);
  }

  template <typename InputType, typename ContinueFunctor>
  VTKM_CONT void DoTransform(const InputType& dynamicInput,
                             const ContinueFunctor& continueFunc,
                             vtkm::cont::internal::DynamicTransformTagCastAndCall) const
  {
    CastAndCall(dynamicInput, continueFunc);
  }
};
}
}
} // namespace vtkm::cont::internal

#endif //vtk_m_cont_internal_DynamicTransform_h

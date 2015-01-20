//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014. Los Alamos National Security
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_internal_DynamicTransform_h
#define vtk_m_cont_internal_DynamicTransform_h

#include "vtkm/internal/IndexTag.h"

namespace vtkm {
namespace cont {
namespace internal {

/// Tag used to identify an object that is a dynamic object that contains a
/// CastAndCall method that iterates over all possible dynamic choices to run
/// templated code.
///
struct DynamicTransformTagCastAndCall {  };

/// Tag used to identify an object that is a static object that, when used with
/// a \c DynamicTransform should just pass itself as a concrete object.
///
struct DynamicTransformTagStatic {  };

/// A traits class that identifies whether an object used in a \c
/// DynamicTransform should use a \c CastAndCall functionality or treated as a
/// static object. The default implementation identifies the object as static
/// (as most objects are bound to be). Dynamic objects that implement
/// \c CastAndCall should specialize (or partially specialize) this traits class
/// to identify the object as dynamic. VTK-m classes like \c DynamicArray are
/// already specialized.
///
template<typename T>
struct DynamicTransformTraits {
  /// A type set to either \c DynamicTransformTagStatic or \c
  /// DynamicTransformTagCastAndCall. The default implementation is set to \c
  /// DynamicTransformTagStatic. Dynamic objects that implement \c CastAndCall
  /// should specialize this class redefine it to \c
  /// DynamicTransformTagCastAndCall.
  ///
  typedef vtkm::cont::internal::DynamicTransformTagStatic DynamicTag;
};

/// This functor can be used as the transform in the \c DynamicTransformCont
/// method of \c FunctionInterface. It will allow dynamic objects like
/// \c DynamicArray to be cast to their concrete types for templated operation.
///
struct DynamicTransform
{
  template<typename InputType,
           typename ContinueFunctor,
           vtkm::IdComponent Index>
  VTKM_CONT_EXPORT
  void operator()(const InputType &input,
                  const ContinueFunctor &continueFunc,
                  vtkm::internal::IndexTag<Index>) const
  {
    this->DoTransform(
          input,
          continueFunc,
          typename vtkm::cont::internal::DynamicTransformTraits<InputType>::DynamicTag());
  }

private:
  template<typename InputType, typename ContinueFunctor>
  VTKM_CONT_EXPORT
  void DoTransform(const InputType &input,
                   const ContinueFunctor &continueFunc,
                   vtkm::cont::internal::DynamicTransformTagStatic) const
  {
    continueFunc(input);
  }

  template<typename InputType, typename ContinueFunctor>
  VTKM_CONT_EXPORT
  void DoTransform(const InputType &dynamicInput,
                   const ContinueFunctor &continueFunc,
                   vtkm::cont::internal::DynamicTransformTagCastAndCall) const
  {
    dynamicInput.CastAndCall(continueFunc);
  }
};

}
}
} // namespace vtkm::cont::internal

#endif //vtk_m_cont_internal_DynamicTransform_h

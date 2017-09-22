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

#include "vtkm/internal/IndexTag.h"

namespace vtkm
{
namespace cont
{

template <typename T, typename S>
class ArrayHandle;
template <vtkm::IdComponent>
class CellSetStructured;
template <typename T>
class CellSetSingleType;
template <typename T, typename S, typename U, typename V>
class CellSetExplicit;
template <typename T, typename S>
class CellSetPermutation;

/// A Generic interface to CastAndCall. The default implementation simply calls
/// DynamicObject's CastAndCall, but specializations of this function exist for
/// other classes (e.g. Field, CoordinateSystem, ArrayHandle).
template <typename DynamicObject, typename Functor>
void CastAndCall(const DynamicObject& dynamicObject, const Functor& f)
{
  dynamicObject.CastAndCall(f);
}

/// A specialization of CastAndCall for basic ArrayHandle types,
/// Since the type is already known no deduction is needed.
/// This specialization is used to simplify numerous worklet algorithms
template <typename T, typename U, typename Functor>
void CastAndCall(const vtkm::cont::ArrayHandle<T, U>& handle, const Functor& f)
{
  f(handle);
}

/// A specialization of CastAndCall for basic CellSetStructured types,
/// Since the type is already known no deduction is needed.
/// This specialization is used to simplify numerous worklet algorithms
template <vtkm::IdComponent Dim, typename Functor>
void CastAndCall(const vtkm::cont::CellSetStructured<Dim>& cellset, const Functor& f)
{
  f(cellset);
}

/// A specialization of CastAndCall for basic CellSetSingleType types,
/// Since the type is already known no deduction is needed.
/// This specialization is used to simplify numerous worklet algorithms
template <typename ConnectivityStorageTag, typename Functor>
void CastAndCall(const vtkm::cont::CellSetSingleType<ConnectivityStorageTag>& cellset,
                 const Functor& f)
{
  f(cellset);
}

/// A specialization of CastAndCall for basic CellSetExplicit types,
/// Since the type is already known no deduction is needed.
/// This specialization is used to simplify numerous worklet algorithms
template <typename T, typename S, typename U, typename V, typename Functor>
void CastAndCall(const vtkm::cont::CellSetExplicit<T, S, U, V>& cellset, const Functor& f)
{
  f(cellset);
}

/// A specialization of CastAndCall for basic CellSetPermutation types,
/// Since the type is already known no deduction is needed.
/// This specialization is used to simplify numerous worklet algorithms
template <typename PermutationType, typename CellSetType, typename Functor>
void CastAndCall(const vtkm::cont::CellSetPermutation<PermutationType, CellSetType>& cellset,
                 const Functor& f)
{
  f(cellset);
}

namespace internal
{

/// Tag used to identify an object that is a dynamic object that contains a
/// CastAndCall method that iterates over all possible dynamic choices to run
/// templated code.
///
struct DynamicTransformTagCastAndCall
{
};

/// Tag used to identify an object that is a static object that, when used with
/// a \c DynamicTransform should just pass itself as a concrete object.
///
struct DynamicTransformTagStatic
{
};

/// A traits class that identifies whether an object used in a \c
/// DynamicTransform should use a \c CastAndCall functionality or treated as a
/// static object. The default implementation identifies the object as static
/// (as most objects are bound to be). Dynamic objects that implement
/// \c CastAndCall should specialize (or partially specialize) this traits class
/// to identify the object as dynamic. VTK-m classes like \c DynamicArray are
/// already specialized.
///
template <typename T>
struct DynamicTransformTraits
{
  /// A type set to either \c DynamicTransformTagStatic or \c
  /// DynamicTransformTagCastAndCall. The default implementation is set to \c
  /// DynamicTransformTagStatic. Dynamic objects that implement \c CastAndCall
  /// should specialize this class redefine it to \c
  /// DynamicTransformTagCastAndCall.
  ///
  using DynamicTag = vtkm::cont::internal::DynamicTransformTagStatic;
};

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

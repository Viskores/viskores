//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//
//=============================================================================
#ifndef vtk_m_cont_ArrayHandleImplicit_h
#define vtk_m_cont_ArrayHandleImplicit_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/StorageImplicit.h>

namespace vtkm {
namespace cont {


namespace detail {
/// \brief An array portal that returns the result of a functor
///
/// This array portal is similar to an implicit array i.e an array that is
/// defined functionally rather than actually stored in memory. The array
/// comprises a functor that is called for each index.
///
/// The \c ArrayPortalImplicit is used in an ArrayHandle with an
/// \c StorageImplicit container.
///
template <class ValueType_, class FunctorType_ >
class ArrayPortalImplicit
{
public:
  typedef ValueType_ ValueType;
  typedef ValueType_ IteratorType;
  typedef FunctorType_ FunctorType;

  VTKM_EXEC_CONT_EXPORT
  ArrayPortalImplicit() :
    Functor(),
    NumberOfValues(0) {  }

  VTKM_EXEC_CONT_EXPORT
  ArrayPortalImplicit(FunctorType f, vtkm::Id numValues) :
    Functor(f),
    NumberOfValues(numValues)
  {  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfValues() const { return this->NumberOfValues; }

  VTKM_EXEC_CONT_EXPORT
  ValueType Get(vtkm::Id index) const { return this->Functor(index); }

private:
  FunctorType Functor;
  vtkm::Id NumberOfValues;
};

/// A convenience class that provides a typedef to the appropriate tag for
/// a implicit array container.
template<typename ValueType, typename FunctorType>
struct ArrayHandleImplicitTraits
{
  typedef vtkm::cont::StorageTagImplicit<
      vtkm::cont::detail::ArrayPortalImplicit<ValueType,
                                              FunctorType> > Tag;
};

} // namespace detail


/// \brief An \c ArrayHandle that computes values on the fly.
///
/// \c ArrayHandleImplicit is a specialization of ArrayHandle.
/// It takes a user defined functor which is called with a given index value.
/// The functor returns the result of the functor as the value of this
/// array at that position.
///
template <typename ValueType,
          class FunctorType>
class ArrayHandleImplicit
    : public vtkm::cont::ArrayHandle <
          ValueType,
          typename detail::ArrayHandleImplicitTraits<ValueType,
                                                     FunctorType>::Tag >
{
private:
  typedef typename detail::ArrayHandleImplicitTraits<ValueType,
                                                     FunctorType> ArrayTraits;

  typedef typename ArrayTraits::Tag Tag;

 public:
  typedef vtkm::cont::ArrayHandle<ValueType,Tag> Superclass;

  ArrayHandleImplicit()
    : Superclass(typename Superclass::PortalConstControl(FunctorType(),0)) {  }

  ArrayHandleImplicit(FunctorType functor, vtkm::Id length)
    : Superclass(typename Superclass::PortalConstControl(functor,length))
    {
    }
};

/// make_ArrayHandleImplicit is convenience function to generate an
/// ArrayHandleImplicit.  It takes a functor and the virtual length of the
/// arry.

template <typename T, typename FunctorType>
VTKM_CONT_EXPORT
vtkm::cont::ArrayHandleImplicit<T, FunctorType>
make_ArrayHandleImplicit(FunctorType functor, vtkm::Id length)
{
  return ArrayHandleImplicit<T,FunctorType>(functor,length);
}


}
} // namespace vtkm::cont

#endif //vtk_m_cont_ArrayHandleImplicit_h

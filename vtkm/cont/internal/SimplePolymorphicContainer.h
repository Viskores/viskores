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
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_internal_SimplePolymorphicContainer_h
#define vtk_m_cont_internal_SimplePolymorphicContainer_h

#include <vtkm/Types.h>
#include <memory>

namespace vtkm {
namespace cont {
namespace internal {

/// \brief Base class for SimplePolymorphicContainer
///
struct SimplePolymorphicContainerBase {
  // This must exist so that subclasses are destroyed correctly.
  virtual ~SimplePolymorphicContainerBase() {  }

  virtual std::shared_ptr<SimplePolymorphicContainerBase>
  NewInstance() const = 0;

  virtual const void *GetVoidPointer() const = 0;
};

/// \brief Simple object container that can use C++ run-time type information.
///
/// The SimplePolymorphicContainer is a trivial structure that contains a
/// single object. The intention is to be able to pass around a pointer to the
/// superclass SimplePolymorphicContainerBase to methods that cannot know the
/// full type of the object at run-time. This is roughly equivalent to passing
/// around a void* except that C++ will capture run-time type information that
/// allows for safer dynamic downcasts.
///
template<typename T>
struct SimplePolymorphicContainer : public SimplePolymorphicContainerBase
{
  T Item;

  VTKM_CONT_EXPORT
  SimplePolymorphicContainer() : Item() {  }

  VTKM_CONT_EXPORT
  SimplePolymorphicContainer(const T &src) : Item(src) {  }

  virtual std::shared_ptr<SimplePolymorphicContainerBase> NewInstance() const
  {
    return std::shared_ptr<SimplePolymorphicContainerBase>(
          new SimplePolymorphicContainer<T>());
  }

  virtual const void *GetVoidPointer() const
  {
    return &this->Item;
  }
};

}
}
} // namespace vtkm::cont::internal

#endif //vtk_m_cont_internal_SimplePolymorphicContainer_h

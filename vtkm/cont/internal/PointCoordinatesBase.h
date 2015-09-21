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
#ifndef vtk_m_cont_internal_PointCoordinatesBase_h
#define vtk_m_cont_internal_PointCoordinatesBase_h

#include <vtkm/StaticAssert.h>
#include <vtkm/Types.h>

VTKM_THIRDPARTY_PRE_INCLUDE
#include <boost/type_traits/is_base_of.hpp>
VTKM_THIRDPARTY_POST_INCLUDE

/// Checks that the argument is a proper \c PointCoordinates class. This is a
/// handy concept check for functions and classes to make sure that a template
/// argument is actually point coordinates. (You can get weird errors elsewhere
/// in the code when a mistake is made.)
#define VTKM_IS_POINT_COORDINATES(pctype) \
  VTKM_STATIC_ASSERT_MSG( \
      ::vtkm::cont::internal::IsValidPointCoordinates<pctype>::type::value, \
      "Provided type is not a valid VTK-m PointCoordinates type.")

namespace vtkm {
namespace cont {
namespace internal {

/// \brief Superclass for point coordinates classes.
///
/// \c PointCoordinatesBase is a simple superclass for all point coordinates
/// classes (by convention named \c PointCoordinates___).
///
/// The most important feature of this base class is to provide a common class
/// to perform a compile-time check to make sure a templated class is in fact
/// supposed to be a point coordinate class. (It is assumed that the subclass
/// will implement the expected methods.)
///
/// The second feature of this base class is to provide a type to perform safe
/// up and down casts, although this is easy to get around.
///
class PointCoordinatesBase
{
public:
  // It is important to declare the destructor virtual so that subclasses will
  // be properly destroyed.
  virtual ~PointCoordinatesBase() {  }
};

/// Checks to see if the given type is a valid point coordinates class. This
/// check is compatable with the Boost meta-template programming library (MPL).
/// It contains a typedef named type that is either boost::mpl::true_ or
/// boost::mpl::false_. Both of these have a typedef named value with the
/// respective boolean value.
///
template<typename Type>
struct IsValidPointCoordinates {
  typedef typename boost::is_base_of<
        vtkm::cont::internal::PointCoordinatesBase,Type>::type type;
};

}
}
} // namespace vtkm::cont::internal

#endif //vtk_m_cont_internal_PointCoordinatesBase_h

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
#ifndef vtk_m_internal_ArrayPortalHelpers_h
#define vtk_m_internal_ArrayPortalHelpers_h


#include <vtkm/VecTraits.h>
#include <vtkm/internal/ExportMacros.h>

namespace vtkm
{
namespace internal
{

template <typename PortalType>
struct PortalSupportsGets
{
  template <typename U, typename S = decltype(std::declval<U>().Get(vtkm::Id{}))>
  static std::true_type has(int);
  template <typename U>
  static std::false_type has(...);
  using type = decltype(has<PortalType>(0));
};

template <typename PortalType>
struct PortalSupportsSets
{
  template <typename U,
            typename S = decltype(std::declval<U>().Set(vtkm::Id{},
                                                        std::declval<typename U::ValueType>()))>
  static std::true_type has(int);
  template <typename U>
  static std::false_type has(...);
  using type = decltype(has<PortalType>(0));
};
}
} // namespace vtkm::internal

#endif //vtk_m_internal_ArrayPortalHelpers_h

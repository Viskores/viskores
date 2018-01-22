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
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//
//=============================================================================
#ifndef vtk_m_worklet_colorconversion_Portals_h
#define vtk_m_worklet_colorconversion_Portals_h

#include <vtkm/VectorAnalysis.h>

namespace vtkm
{
namespace worklet
{
namespace colorconversion
{

struct MagnitudePortal
{
  template <typename T, int N>
  VTKM_EXEC auto operator()(const vtkm::Vec<T, N>& values) const
    -> decltype(vtkm::Magnitude(values))
  { //Should we be using RMag?
    return vtkm::Magnitude(values);
  }
};

struct ComponentPortal
{
  vtkm::IdComponent Component;

  ComponentPortal()
    : Component(0)
  {
  }

  ComponentPortal(vtkm::IdComponent comp)
    : Component(comp)
  {
  }

  template <typename T>
  VTKM_EXEC auto operator()(T&& value) const ->
    typename std::remove_reference<decltype(value[vtkm::IdComponent{}])>::type
  {
    return value[this->Component];
  }
};
}
}
}
#endif

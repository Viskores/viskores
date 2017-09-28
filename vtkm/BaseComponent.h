//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
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
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_BaseComponent_h
#define vtk_m_BaseComponent_h

#include <vtkm/Matrix.h>
#include <vtkm/TypeTraits.h>
#include <vtkm/VecTraits.h>

namespace vtkm
{

namespace detail
{

template <typename VecType, typename DimensionalityTag>
struct BaseComponentImpl;

template <typename VecType>
struct BaseComponentImpl<VecType, vtkm::TypeTraitsVectorTag>
{
private:
  using ComponentType = typename vtkm::VecTraits<VecType>::ComponentType;

public:
  using Type =
    typename BaseComponentImpl<ComponentType,
                               typename vtkm::TypeTraits<ComponentType>::DimensionalityTag>::Type;
};

template <typename VecType>
struct BaseComponentImpl<VecType, vtkm::TypeTraitsMatrixTag>
  : BaseComponentImpl<VecType, vtkm::TypeTraitsVectorTag>
{
};

template <typename ScalarType>
struct BaseComponentImpl<ScalarType, vtkm::TypeTraitsScalarTag>
{
  using Type = ScalarType;
};

} // namespace detail

// Finds the base component type of a Vec. If you have a Vec of Vecs, it will
// descend all Vecs until you get to the scalar type.
template <typename VecType>
struct BaseComponent
{
  using Type =
    typename detail::BaseComponentImpl<VecType,
                                       typename vtkm::TypeTraits<VecType>::DimensionalityTag>::Type;
};

} // namespace vtkm

#endif //vtk_m_BaseComponent_h

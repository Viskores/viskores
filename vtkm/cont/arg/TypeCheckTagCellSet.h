//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_arg_TypeCheckTagCellSet_h
#define vtk_m_cont_arg_TypeCheckTagCellSet_h

#include <vtkm/cont/arg/TypeCheck.h>

#include <vtkm/cont/CellSet.h>

namespace vtkm
{
namespace cont
{
namespace arg
{

/// Check for a CellSet-like object.
///
struct TypeCheckTagCellSet
{
};

template <typename CellSetType>
struct TypeCheck<TypeCheckTagCellSet, CellSetType>
{
  static constexpr bool value = vtkm::cont::internal::CellSetCheck<CellSetType>::type::value;
};
}
}
} // namespace vtkm::cont::arg

#endif //vtk_m_cont_arg_TypeCheckTagCellSet_h

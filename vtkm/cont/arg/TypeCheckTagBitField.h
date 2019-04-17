//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_arg_TypeCheckTagBitField_h
#define vtk_m_cont_arg_TypeCheckTagBitField_h

#include <vtkm/cont/arg/TypeCheck.h>

#include <vtkm/cont/BitField.h>

#include <type_traits>

namespace vtkm
{
namespace cont
{
namespace arg
{

struct TypeCheckTagBitField
{
};

template <typename T>
struct TypeCheck<TypeCheckTagBitField, T> : public std::is_base_of<vtkm::cont::BitField, T>
{
};
}
}
} // namespace vtkm::cont::arg

#endif //vtk_m_cont_arg_TypeCheckTagBitField_h

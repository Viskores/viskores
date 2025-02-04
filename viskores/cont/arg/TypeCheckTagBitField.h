//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef viskores_cont_arg_TypeCheckTagBitField_h
#define viskores_cont_arg_TypeCheckTagBitField_h

#include <viskores/cont/arg/TypeCheck.h>

#include <viskores/cont/BitField.h>

#include <type_traits>

namespace viskores
{
namespace cont
{
namespace arg
{

struct TypeCheckTagBitField
{
};

template <typename T>
struct TypeCheck<TypeCheckTagBitField, T> : public std::is_base_of<viskores::cont::BitField, T>
{
};
}
}
} // namespace viskores::cont::arg

#endif //viskores_cont_arg_TypeCheckTagBitField_h

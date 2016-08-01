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

#ifndef vtk_m_internal_IntegerSequence_h
#define vtk_m_internal_IntegerSequence_h

#include <cstdlib>

namespace vtkm {
namespace internal {

/// \brief A container of unsigned integers
///
/// C++11 Doesn't provide an IntegerSequence class and helper constructor
// So we need to roll our own. This class has been tested up to 512 elements.
//
template<std::size_t...> struct IntegerSequence{};

namespace detail {

template<size_t N, size_t... Is> //unroll in blocks of 4
struct MakeSeq : MakeSeq<N-4, N-3, N-2, N-1, N, Is...> {};

template<size_t... Is>
struct MakeSeq<0,1,2,3,Is...> //termination case
{ using type = IntegerSequence<0,1,2,3,Is...>; };

template<size_t Mod, size_t N>
struct PreMakeSeq : MakeSeq<N-3, N-2, N-1, N> {};

template<size_t N> //specialization for value +1 to divisible by 4
struct PreMakeSeq<1,N> : MakeSeq<N> {};

template<size_t N> //specialization for value +2 to divisible by 4
struct PreMakeSeq<2,N> : MakeSeq<N-1,N> {};

template<size_t N> //specialization for value +3 to divisible by 4
struct PreMakeSeq<3,N> : MakeSeq<N-2,N-1,N> {};

template<> //specialization for 4
struct PreMakeSeq<4,3> { using type = IntegerSequence<0,1,2,3>; };

template<> //specialization for 3
struct PreMakeSeq<3,2> { using type = IntegerSequence<0,1,2>; };

template<> //specialization for 2
struct PreMakeSeq<2,1> { using type = IntegerSequence<0,1>; };

template<> //specialization for 1
struct PreMakeSeq<1,0> { using type = IntegerSequence<0>; };

template<> //specialization for 0
struct PreMakeSeq<0,-1UL> { using type = IntegerSequence<>; };

} //namespace detail

/// \brief A helper method to create an Integer sequence of 0...N-1.
template<std::size_t N>
struct MakeIntegerSequence : detail::PreMakeSeq<N%4,N-1> {};

}
}

#endif //vtk_m_internal_IntegerSequence_h

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2017 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2017 UT-Battelle, LLC.
//  Copyright 2017 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef Tuple_h
#define Tuple_h

// tao::tuple is a much more efficient tuple implementation, but it doesn't
// work on MSVC2015. For this compiler, fallback to a simpler implementation.
#if defined(_MSC_VER) && _MSC_VER == 1900
#define TAOCPP_USE_SIMPLE_TUPLE
// There is a bug in apple clang 9.0 that prevents tao-tuple from compiling:
#elif defined(__apple_build_version__) && defined(__clang__) && __clang_major__ == 9 &&            \
  clang_minor == 0
#define TAOCPP_USE_SIMPLE_TUPLE
#endif

#ifdef TAOCPP_USE_SIMPLE_TUPLE
#include "SimpleTuple.h"
#else
#include "tao/tuple/tuple.hpp"
#endif

namespace vtkmstd
{

#ifdef TAOCPP_USE_SIMPLE_TUPLE
using simple_tuple::get;
using simple_tuple::make_tuple;
using simple_tuple::tuple;
using simple_tuple::tuple_element;
using simple_tuple::tuple_size;
#else
using tao::get;
using tao::make_tuple;
using tao::tuple;
using tao::tuple_element;
using tao::tuple_size;
#endif

} // end namespace vtkmstd

#endif // Tuple_h

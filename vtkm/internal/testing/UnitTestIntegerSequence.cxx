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

#include <vtkm/internal/IntegerSequence.h>
#include <vtkm/testing/Testing.h>

#include <vector>

namespace
{
template <std::size_t Len, int... Ts>
void verify_correct_length(vtkm::internal::IntegerSequence<Ts...>)
{
  static_assert(Len == sizeof...(Ts), "Incorrect length");

  //use a runtime time to verify the contents of the integer sequence
  //are 0...N-1
  std::vector<int> container = { Ts... };
  for (std::size_t i = 0; i < Len; ++i)
  {
    VTKM_TEST_ASSERT(container[i] == static_cast<int>(i), "Incorrect value");
  }
}

void IntegerSequenceSizes()
{
  using zero = vtkm::internal::MakeIntegerSequence<0>::type;
  using one = vtkm::internal::MakeIntegerSequence<1>::type;
  using two = vtkm::internal::MakeIntegerSequence<2>::type;
  using four = vtkm::internal::MakeIntegerSequence<4>::type;
  using thirty_two = vtkm::internal::MakeIntegerSequence<32>::type;
  using thirty_three = vtkm::internal::MakeIntegerSequence<33>::type;
  using thirty_four = vtkm::internal::MakeIntegerSequence<34>::type;
  using thirty_five = vtkm::internal::MakeIntegerSequence<35>::type;
  using two_fifty_six = vtkm::internal::MakeIntegerSequence<256>::type;
  using five_twelve = vtkm::internal::MakeIntegerSequence<512>::type;

  verify_correct_length<0>(zero());
  verify_correct_length<1>(one());
  verify_correct_length<2>(two());
  verify_correct_length<4>(four());
  verify_correct_length<32>(thirty_two());
  verify_correct_length<33>(thirty_three());
  verify_correct_length<34>(thirty_four());
  verify_correct_length<35>(thirty_five());
  verify_correct_length<256>(two_fifty_six());
  verify_correct_length<512>(five_twelve());
}
}

int UnitTestIntegerSequence(int, char* [])
{
  return vtkm::testing::Testing::Run(IntegerSequenceSizes);
}

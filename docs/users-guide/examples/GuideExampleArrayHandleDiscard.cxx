//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/ArrayHandleDiscard.h>
#include <viskores/cont/testing/Testing.h>

namespace
{

////
//// BEGIN-EXAMPLE ArrayHandleDiscard
////
template<typename InputArrayType, typename OutputArrayType1, typename OutputArrayType2>
VISKORES_CONT void DoFoo(InputArrayType input,
                     OutputArrayType1 output1,
                     OutputArrayType2 output2);

template<typename InputArrayType>
VISKORES_CONT inline viskores::cont::ArrayHandle<viskores::FloatDefault> DoBar(InputArrayType input)
{
  VISKORES_IS_ARRAY_HANDLE(InputArrayType);

  viskores::cont::ArrayHandle<viskores::FloatDefault> keepOutput;

  viskores::cont::ArrayHandleDiscard<viskores::FloatDefault> discardOutput;

  DoFoo(input, keepOutput, discardOutput);

  return keepOutput;
}
////
//// END-EXAMPLE ArrayHandleDiscard
////

template<typename InputArrayType, typename OutputArrayType1, typename OutputArrayType2>
VISKORES_CONT inline void DoFoo(InputArrayType input,
                            OutputArrayType1 output1,
                            OutputArrayType2 output2)
{
  viskores::cont::Algorithm::Copy(input, output1);
  viskores::cont::Algorithm::Copy(input, output2);
}

void Test()
{
  viskores::cont::ArrayHandleCounting<viskores::FloatDefault> inputArray(0, 10, 10);

  viskores::cont::ArrayHandle<viskores::FloatDefault> outputArray = DoBar(inputArray);

  VISKORES_TEST_ASSERT(outputArray.GetNumberOfValues() == 10, "Wrong size.");
}

} // anonymous namespace

int GuideExampleArrayHandleDiscard(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(Test, argc, argv);
}

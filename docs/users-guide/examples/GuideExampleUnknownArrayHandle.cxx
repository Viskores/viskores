//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

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
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleCast.h>
#include <viskores/cont/ArrayHandleConstant.h>
#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/ArrayHandleGroupVec.h>
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/cont/ArrayHandleMultiplexer.h>
#include <viskores/cont/ArrayRangeCompute.h>
#include <viskores/cont/DeviceAdapter.h>
#include <viskores/cont/Invoker.h>
#include <viskores/cont/UncertainArrayHandle.h>
#include <viskores/cont/UnknownArrayHandle.h>
#include <viskores/cont/internal/StorageError.h>

#include <viskores/worklet/WorkletMapField.h>

#include <viskores/VecTraits.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

constexpr viskores::Id ARRAY_SIZE = 10;

////
//// BEGIN-EXAMPLE CreateUnknownArrayHandle
////
VISKORES_CONT
viskores::cont::UnknownArrayHandle LoadUnknownArray(const void* buffer,
                                                    viskores::Id length,
                                                    std::string type)
{
  viskores::cont::UnknownArrayHandle handle;
  if (type == "float")
  {
    viskores::cont::ArrayHandle<viskores::Float32> concreteArray =
      viskores::cont::make_ArrayHandle(
        reinterpret_cast<const viskores::Float32*>(buffer),
        length,
        viskores::CopyFlag::On);
    handle = concreteArray;
  }
  else if (type == "int")
  {
    viskores::cont::ArrayHandle<viskores::Int32> concreteArray =
      viskores::cont::make_ArrayHandle(reinterpret_cast<const viskores::Int32*>(buffer),
                                       length,
                                       viskores::CopyFlag::On);
    handle = concreteArray;
  }
  return handle;
}
////
//// END-EXAMPLE CreateUnknownArrayHandle
////

void TryLoadUnknownArray()
{
  viskores::Float32 scalarBuffer[ARRAY_SIZE];
  viskores::cont::UnknownArrayHandle handle =
    LoadUnknownArray(scalarBuffer, ARRAY_SIZE, "float");
  VISKORES_TEST_ASSERT((handle.IsValueType<viskores::Float32>()), "Type not right.");
  VISKORES_TEST_ASSERT(!(handle.IsValueType<viskores::Int32>()), "Type not right.");

  viskores::Int32 idBuffer[ARRAY_SIZE];
  handle = LoadUnknownArray(idBuffer, ARRAY_SIZE, "int");
  VISKORES_TEST_ASSERT((handle.IsValueType<viskores::Int32>()), "Type not right.");
  VISKORES_TEST_ASSERT(!(handle.IsValueType<viskores::Float32>()), "Type not right.");
}

void NonTypeUnknownArrayHandleAllocate()
{
  viskores::cont::ArrayHandle<viskores::Id> concreteArray;
  concreteArray.Allocate(ARRAY_SIZE);
  ////
  //// BEGIN-EXAMPLE NonTypeUnknownArrayHandleNewInstance
  //// BEGIN-EXAMPLE UnknownArrayHandleResize
  ////
  viskores::cont::UnknownArrayHandle unknownHandle = // ... some valid array
    //// PAUSE-EXAMPLE
    concreteArray;
  //// RESUME-EXAMPLE

  // Double the size of the array while preserving all the initial values.
  viskores::Id originalArraySize = unknownHandle.GetNumberOfValues();
  unknownHandle.Allocate(originalArraySize * 2, viskores::CopyFlag::On);
  ////
  //// END-EXAMPLE UnknownArrayHandleResize
  ////

  // Create a new array of the same type as the original.
  viskores::cont::UnknownArrayHandle newArray = unknownHandle.NewInstance();

  newArray.Allocate(originalArraySize);
  ////
  //// END-EXAMPLE NonTypeUnknownArrayHandleNewInstance
  ////

  VISKORES_TEST_ASSERT(originalArraySize == ARRAY_SIZE);
  VISKORES_TEST_ASSERT(unknownHandle.GetNumberOfValues() == (2 * ARRAY_SIZE));
  VISKORES_TEST_ASSERT(concreteArray.GetNumberOfValues() == (2 * ARRAY_SIZE));
  VISKORES_TEST_ASSERT(newArray.GetNumberOfValues() == ARRAY_SIZE);
  VISKORES_TEST_ASSERT(newArray.IsType<decltype(concreteArray)>());

  ////
  //// BEGIN-EXAMPLE UnknownArrayHandleBasicInstance
  ////
  viskores::cont::UnknownArrayHandle indexArray = viskores::cont::ArrayHandleIndex();
  // Returns an array of type ArrayHandleBasic<viskores::Id>
  viskores::cont::UnknownArrayHandle basicArray = indexArray.NewInstanceBasic();
  ////
  //// END-EXAMPLE UnknownArrayHandleBasicInstance
  ////

  VISKORES_TEST_ASSERT(
    basicArray.IsType<viskores::cont::ArrayHandleBasic<viskores::Id>>());

  ////
  //// BEGIN-EXAMPLE UnknownArrayHandleFloatInstance
  ////
  viskores::cont::UnknownArrayHandle intArray = viskores::cont::ArrayHandleIndex();
  // Returns an array of type ArrayHandleBasic<viskores::FloatDefault>
  viskores::cont::UnknownArrayHandle floatArray = intArray.NewInstanceFloatBasic();

  viskores::cont::UnknownArrayHandle id3Array =
    viskores::cont::ArrayHandle<viskores::Id3>();
  // Returns an array of type ArrayHandleBasic<viskores::Vec3f>
  viskores::cont::UnknownArrayHandle float3Array = id3Array.NewInstanceFloatBasic();
  ////
  //// END-EXAMPLE UnknownArrayHandleFloatInstance
  ////

  VISKORES_TEST_ASSERT(
    floatArray.IsType<viskores::cont::ArrayHandleBasic<viskores::FloatDefault>>());
  VISKORES_TEST_ASSERT(
    float3Array.IsType<viskores::cont::ArrayHandleBasic<viskores::Vec3f>>());
}

////
//// BEGIN-EXAMPLE UnknownArrayHandleCanConvert
////
VISKORES_CONT viskores::FloatDefault GetMiddleValue(
  const viskores::cont::UnknownArrayHandle& unknownArray)
{
  if (unknownArray
        .CanConvert<viskores::cont::ArrayHandleConstant<viskores::FloatDefault>>())
  {
    // Fast path for known array
    viskores::cont::ArrayHandleConstant<viskores::FloatDefault> constantArray;
    unknownArray.AsArrayHandle(constantArray);
    return constantArray.GetValue();
  }
  else
  {
    // General path
    auto ranges = viskores::cont::ArrayRangeCompute(unknownArray);
    viskores::Range range = ranges.ReadPortal().Get(0);
    return static_cast<viskores::FloatDefault>((range.Min + range.Max) / 2);
  }
}
////
//// END-EXAMPLE UnknownArrayHandleCanConvert
////

////
//// BEGIN-EXAMPLE UnknownArrayHandleDeepCopy
////
VISKORES_CONT viskores::cont::ArrayHandle<viskores::FloatDefault> CopyToDefaultArray(
  const viskores::cont::UnknownArrayHandle& unknownArray)
{
  // Initialize the output UnknownArrayHandle with the array type we want to copy to.
  viskores::cont::UnknownArrayHandle output =
    viskores::cont::ArrayHandle<viskores::FloatDefault>{};
  output.DeepCopyFrom(unknownArray);
  return output.AsArrayHandle<viskores::cont::ArrayHandle<viskores::FloatDefault>>();
}
////
//// END-EXAMPLE UnknownArrayHandleDeepCopy
////

////
//// BEGIN-EXAMPLE ArrayCopyShallow
////
VISKORES_CONT viskores::cont::ArrayHandle<viskores::FloatDefault> GetAsFloatArray(
  const viskores::cont::UnknownArrayHandle& unknownArray)
{
  viskores::cont::ArrayHandle<viskores::FloatDefault> output;
  viskores::cont::ArrayCopyShallowIfPossible(unknownArray, output);
  return output;
}
////
//// END-EXAMPLE ArrayCopyShallow
////

////
//// BEGIN-EXAMPLE UnknownArrayHandleShallowCopy
////
VISKORES_CONT viskores::cont::ArrayHandle<viskores::FloatDefault> GetAsDefaultArray(
  const viskores::cont::UnknownArrayHandle& unknownArray)
{
  // Initialize the output UnknownArrayHandle with the array type we want to copy to.
  viskores::cont::UnknownArrayHandle output =
    viskores::cont::ArrayHandle<viskores::FloatDefault>{};
  output.CopyShallowIfPossible(unknownArray);
  return output.AsArrayHandle<viskores::cont::ArrayHandle<viskores::FloatDefault>>();
}
////
//// END-EXAMPLE UnknownArrayHandleShallowCopy
////

void CastUnknownArrayHandle()
{
  ////
  //// BEGIN-EXAMPLE UnknownArrayHandleAsCastArray
  ////
  viskores::cont::ArrayHandle<viskores::Float32> originalArray;
  viskores::cont::UnknownArrayHandle unknownArray = originalArray;

  viskores::cont::ArrayHandleCast<viskores::Float64, decltype(originalArray)> castArray;
  unknownArray.AsArrayHandle(castArray);
  ////
  //// END-EXAMPLE UnknownArrayHandleAsCastArray
  ////

  ////
  //// BEGIN-EXAMPLE UnknownArrayHandleAsArrayHandle1
  ////
  viskores::cont::ArrayHandle<viskores::Float32> knownArray =
    unknownArray.AsArrayHandle<viskores::cont::ArrayHandle<viskores::Float32>>();
  ////
  //// END-EXAMPLE UnknownArrayHandleAsArrayHandle1
  ////

  ////
  //// BEGIN-EXAMPLE UnknownArrayHandleAsArrayHandle2
  ////
  unknownArray.AsArrayHandle(knownArray);
  ////
  //// END-EXAMPLE UnknownArrayHandleAsArrayHandle2
  ////

  originalArray.Allocate(ARRAY_SIZE);
  SetPortal(originalArray.WritePortal());

  GetMiddleValue(unknownArray);
  CopyToDefaultArray(unknownArray);
  GetAsFloatArray(unknownArray);
  GetAsDefaultArray(unknownArray);
}

////
//// BEGIN-EXAMPLE UsingCastAndCallForTypes
////
struct PrintArrayContentsFunctor
{
  template<typename T, typename S>
  VISKORES_CONT void operator()(const viskores::cont::ArrayHandle<T, S>& array) const
  {
    this->PrintArrayPortal(array.ReadPortal());
  }

private:
  template<typename PortalType>
  VISKORES_CONT void PrintArrayPortal(const PortalType& portal) const
  {
    for (viskores::Id index = 0; index < portal.GetNumberOfValues(); index++)
    {
      // All ArrayPortal objects have ValueType for the type of each value.
      using ValueType = typename PortalType::ValueType;
      using VTraits = viskores::VecTraits<ValueType>;

      ValueType value = portal.Get(index);

      viskores::IdComponent numComponents = VTraits::GetNumberOfComponents(value);
      for (viskores::IdComponent componentIndex = 0; componentIndex < numComponents;
           componentIndex++)
      {
        std::cout << " " << VTraits::GetComponent(value, componentIndex);
      }
      std::cout << std::endl;
    }
  }
};

void PrintArrayContents(const viskores::cont::UnknownArrayHandle& array)
{
  array.CastAndCallForTypes<VISKORES_DEFAULT_TYPE_LIST, VISKORES_DEFAULT_STORAGE_LIST>(
    PrintArrayContentsFunctor{});
}
////
//// END-EXAMPLE UsingCastAndCallForTypes
////

struct MyWorklet : viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldOut);

  template<typename T1, typename T2>
  VISKORES_EXEC void operator()(const T1& in, T2& out) const
  {
    using VTraitsIn = viskores::VecTraits<T1>;
    using VTraitsOut = viskores::VecTraits<T2>;
    const viskores::IdComponent numComponents = VTraitsIn::GetNumberOfComponents(in);
    VISKORES_ASSERT(numComponents == VTraitsOut::GetNumberOfComponents(out));
    for (viskores::IdComponent index = 0; index < numComponents; ++index)
    {
      VTraitsOut::SetComponent(out,
                               index,
                               static_cast<typename VTraitsOut::ComponentType>(
                                 VTraitsIn::GetComponent(in, index)));
    }
  }
};

void TryPrintArrayContents()
{
  viskores::cont::ArrayHandleIndex implicitArray(ARRAY_SIZE);

  viskores::cont::ArrayHandle<viskores::FloatDefault> concreteArray;
  viskores::cont::Algorithm::Copy(implicitArray, concreteArray);

  viskores::cont::UnknownArrayHandle unknownArray = concreteArray;

  PrintArrayContents(unknownArray);

  ////
  //// BEGIN-EXAMPLE UncertainArrayHandle
  ////
  viskores::cont::UncertainArrayHandle<viskores::TypeListScalarAll,
                                       viskores::cont::StorageListBasic>
    uncertainArray(unknownArray);
  uncertainArray.CastAndCall(PrintArrayContentsFunctor{});
  ////
  //// END-EXAMPLE UncertainArrayHandle
  ////

  viskores::cont::ArrayHandle<viskores::FloatDefault> outArray;
  ////
  //// BEGIN-EXAMPLE UnknownArrayResetTypes
  ////
  viskores::cont::Invoker invoke;
  invoke(MyWorklet{},
         unknownArray
           .ResetTypes<viskores::TypeListScalarAll, viskores::cont::StorageListBasic>(),
         outArray);
  ////
  //// END-EXAMPLE UnknownArrayResetTypes
  ////

  ////
  //// BEGIN-EXAMPLE CastAndCallForTypesWithFloatFallback
  ////
  unknownArray.CastAndCallForTypesWithFloatFallback<viskores::TypeListField,
                                                    VISKORES_DEFAULT_STORAGE_LIST>(
    PrintArrayContentsFunctor{});
  ////
  //// END-EXAMPLE CastAndCallForTypesWithFloatFallback
  ////

  ////
  //// BEGIN-EXAMPLE CastAndCallWithFloatFallback
  ////
  uncertainArray.CastAndCallWithFloatFallback(PrintArrayContentsFunctor{});
  ////
  //// END-EXAMPLE CastAndCallWithFloatFallback
  ////
}

void ExtractUnknownComponent()
{
  ////
  //// BEGIN-EXAMPLE UnknownArrayExtractComponent
  ////
  viskores::cont::ArrayHandleBasic<viskores::Vec3f> concreteArray =
    viskores::cont::make_ArrayHandle<viskores::Vec3f>({ { 0, 1, 2 },
                                                        { 3, 4, 5 },
                                                        { 6, 7, 8 },
                                                        { 9, 10, 11 },
                                                        { 12, 13, 14 },
                                                        { 15, 16, 17 } });

  viskores::cont::UnknownArrayHandle unknownArray(concreteArray);

  //// LABEL Call
  auto componentArray = unknownArray.ExtractComponent<viskores::FloatDefault>(0);
  // componentArray contains [ 0, 3, 6, 9, 12, 15 ].
  ////
  //// END-EXAMPLE UnknownArrayExtractComponent
  ////
  VISKORES_TEST_ASSERT(componentArray.GetNumberOfValues() ==
                       concreteArray.GetNumberOfValues());
  {
    auto portal = componentArray.ReadPortal();
    auto expectedPortal = concreteArray.ReadPortal();
    for (viskores::IdComponent i = 0; i < componentArray.GetNumberOfValues(); ++i)
    {
      VISKORES_TEST_ASSERT(test_equal(portal.Get(i), expectedPortal.Get(i)[0]));
    }
  }

  VISKORES_TEST_ASSERT(
    ////
    //// BEGIN-EXAMPLE UnknownArrayBaseComponentType
    ////
    unknownArray.IsBaseComponentType<viskores::FloatDefault>()
    ////
    //// END-EXAMPLE UnknownArrayBaseComponentType
    ////
  );

  auto deepTypeArray = viskores::cont::make_ArrayHandleGroupVec<2>(concreteArray);

  unknownArray = deepTypeArray;
  VISKORES_TEST_ASSERT(unknownArray.GetNumberOfComponentsFlat() == 6);

  viskores::cont::ArrayHandle<viskores::FloatDefault> outputArray;

  viskores::cont::Invoker invoke;

  ////
  //// BEGIN-EXAMPLE UnknownArrayExtractComponentsMultiple
  ////
  std::vector<viskores::cont::ArrayHandle<viskores::FloatDefault>> outputArrays(
    static_cast<std::size_t>(unknownArray.GetNumberOfComponentsFlat()));
  for (viskores::IdComponent componentIndex = 0;
       componentIndex < unknownArray.GetNumberOfComponentsFlat();
       ++componentIndex)
  {
    invoke(MyWorklet{},
           unknownArray.ExtractComponent<viskores::FloatDefault>(componentIndex),
           outputArrays[static_cast<std::size_t>(componentIndex)]);
  }
  ////
  //// END-EXAMPLE UnknownArrayExtractComponentsMultiple
  ////
  for (std::size_t outIndex = 0; outIndex < outputArrays.size(); ++outIndex)
  {
    viskores::IdComponent vecIndex = static_cast<viskores::IdComponent>(outIndex % 3);
    viskores::IdComponent groupIndex = static_cast<viskores::IdComponent>(outIndex / 3);
    auto portal = outputArrays[outIndex].ReadPortal();
    auto expectedPortal = deepTypeArray.ReadPortal();
    VISKORES_TEST_ASSERT(portal.GetNumberOfValues() ==
                         (concreteArray.GetNumberOfValues() / 2));
    for (viskores::IdComponent i = 0; i < portal.GetNumberOfValues(); ++i)
    {
      VISKORES_TEST_ASSERT(
        test_equal(portal.Get(i), expectedPortal.Get(i)[groupIndex][vecIndex]));
    }
  }

  unknownArray = concreteArray;

  viskores::cont::ArrayHandle<viskores::Vec3f> outArray;

  ////
  //// BEGIN-EXAMPLE UnknownArrayExtractArrayWithValueType
  ////
  invoke(
    MyWorklet{}, unknownArray.ExtractArrayWithValueType<viskores::Vec3f>(), outArray);
  ////
  //// END-EXAMPLE UnknownArrayExtractArrayWithValueType
  ////
  VISKORES_TEST_ASSERT(test_equal_ArrayHandles(outArray, concreteArray));

  outArray.ReleaseResources();
  {
    ////
    //// BEGIN-EXAMPLE UnknownArrayAsSOAStride
    ////
    viskores::cont::ArrayHandleSOAStride<viskores::Vec3f> extractedArray;
    unknownArray.AsArrayHandle(extractedArray);
    invoke(MyWorklet{}, extractedArray, outArray);
    ////
    //// END-EXAMPLE UnknownArrayAsSOAStride
    ////
  }
  VISKORES_TEST_ASSERT(test_equal_ArrayHandles(outArray, concreteArray));

  outArray.ReleaseResources();
  {
    ////
    //// BEGIN-EXAMPLE UnknownArrayCastAndCallExtract
    ////
    auto resolveType = [&](auto extractedArray)
    { invoke(MyWorklet{}, extractedArray, outArray); };

    unknownArray
      .CastAndCallForTypes<viskores::TypeListFieldVec3,
                           viskores::List<viskores::cont::StorageTagSOAStride>>(
        resolveType);
    ////
    //// END-EXAMPLE UnknownArrayCastAndCallExtract
    ////
  }
  VISKORES_TEST_ASSERT(test_equal_ArrayHandles(outArray, concreteArray));

  outArray.ReleaseResources();
  ////
  //// BEGIN-EXAMPLE UnknownArrayExtractArrayFromComponents
  ////
  invoke(MyWorklet{},
         unknownArray.ExtractArrayFromComponents<viskores::FloatDefault>(),
         outArray);
  ////
  //// END-EXAMPLE UnknownArrayExtractArrayFromComponents
  ////
  VISKORES_TEST_ASSERT(test_equal_ArrayHandles(outArray, concreteArray));

  ////
  //// BEGIN-EXAMPLE UnknownArrayCallWithExtractedArray
  ////
  unknownArray.CastAndCallWithExtractedArray(PrintArrayContentsFunctor{});
  ////
  //// END-EXAMPLE UnknownArrayCallWithExtractedArray
  ////
}

////
//// BEGIN-EXAMPLE UnknownArrayConstOutput
////
void IndexInitialize(viskores::Id size, const viskores::cont::UnknownArrayHandle& output)
{
  viskores::cont::ArrayHandleIndex input(size);
  output.DeepCopyFrom(input);
}
////
//// END-EXAMPLE UnknownArrayConstOutput
////

////
//// BEGIN-EXAMPLE UseUnknownArrayConstOutput
////
template<typename T>
void Foo(const viskores::cont::ArrayHandle<T>& input,
         viskores::cont::ArrayHandle<T>& output)
{
  IndexInitialize(input.GetNumberOfValues(), output);
  // ...
  ////
  //// END-EXAMPLE UseUnknownArrayConstOutput
  ////

  VISKORES_TEST_ASSERT(output.GetNumberOfValues() == input.GetNumberOfValues());
  auto portal = output.ReadPortal();
  for (viskores::Id index = 0; index < portal.GetNumberOfValues(); ++index)
  {
    VISKORES_TEST_ASSERT(portal.Get(index) == index);
  }
}

void TryConstOutput()
{
  viskores::cont::ArrayHandle<viskores::Id> input =
    viskores::cont::make_ArrayHandle<viskores::Id>({ 3, 6, 1, 4 });
  viskores::cont::ArrayHandle<viskores::Id> output;
  Foo(input, output);
}

void Test()
{
  TryLoadUnknownArray();
  NonTypeUnknownArrayHandleAllocate();
  CastUnknownArrayHandle();
  TryPrintArrayContents();
  ExtractUnknownComponent();
  TryConstOutput();
}

} // anonymous namespace

int GuideExampleUnknownArrayHandle(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(Test, argc, argv);
}

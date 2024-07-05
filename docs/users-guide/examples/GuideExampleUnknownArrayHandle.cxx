//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandleGroupVec.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ArrayHandleMultiplexer.h>
#include <vtkm/cont/ArrayRangeCompute.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/Invoker.h>
#include <vtkm/cont/UncertainArrayHandle.h>
#include <vtkm/cont/UnknownArrayHandle.h>
#include <vtkm/cont/internal/StorageError.h>

#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/VecTraits.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

constexpr vtkm::Id ARRAY_SIZE = 10;

////
//// BEGIN-EXAMPLE CreateUnknownArrayHandle
////
VTKM_CONT
vtkm::cont::UnknownArrayHandle LoadUnknownArray(const void* buffer,
                                                vtkm::Id length,
                                                std::string type)
{
  vtkm::cont::UnknownArrayHandle handle;
  if (type == "float")
  {
    vtkm::cont::ArrayHandle<vtkm::Float32> concreteArray = vtkm::cont::make_ArrayHandle(
      reinterpret_cast<const vtkm::Float32*>(buffer), length, vtkm::CopyFlag::On);
    handle = concreteArray;
  }
  else if (type == "int")
  {
    vtkm::cont::ArrayHandle<vtkm::Int32> concreteArray = vtkm::cont::make_ArrayHandle(
      reinterpret_cast<const vtkm::Int32*>(buffer), length, vtkm::CopyFlag::On);
    handle = concreteArray;
  }
  return handle;
}
////
//// END-EXAMPLE CreateUnknownArrayHandle
////

void TryLoadUnknownArray()
{
  vtkm::Float32 scalarBuffer[ARRAY_SIZE];
  vtkm::cont::UnknownArrayHandle handle =
    LoadUnknownArray(scalarBuffer, ARRAY_SIZE, "float");
  VTKM_TEST_ASSERT((handle.IsValueType<vtkm::Float32>()), "Type not right.");
  VTKM_TEST_ASSERT(!(handle.IsValueType<vtkm::Int32>()), "Type not right.");

  vtkm::Int32 idBuffer[ARRAY_SIZE];
  handle = LoadUnknownArray(idBuffer, ARRAY_SIZE, "int");
  VTKM_TEST_ASSERT((handle.IsValueType<vtkm::Int32>()), "Type not right.");
  VTKM_TEST_ASSERT(!(handle.IsValueType<vtkm::Float32>()), "Type not right.");
}

void NonTypeUnknownArrayHandleAllocate()
{
  vtkm::cont::ArrayHandle<vtkm::Id> concreteArray;
  concreteArray.Allocate(ARRAY_SIZE);
  ////
  //// BEGIN-EXAMPLE NonTypeUnknownArrayHandleNewInstance
  //// BEGIN-EXAMPLE UnknownArrayHandleResize
  ////
  vtkm::cont::UnknownArrayHandle unknownHandle = // ... some valid array
    //// PAUSE-EXAMPLE
    concreteArray;
  //// RESUME-EXAMPLE

  // Double the size of the array while preserving all the initial values.
  vtkm::Id originalArraySize = unknownHandle.GetNumberOfValues();
  unknownHandle.Allocate(originalArraySize * 2, vtkm::CopyFlag::On);
  ////
  //// END-EXAMPLE UnknownArrayHandleResize
  ////

  // Create a new array of the same type as the original.
  vtkm::cont::UnknownArrayHandle newArray = unknownHandle.NewInstance();

  newArray.Allocate(originalArraySize);
  ////
  //// END-EXAMPLE NonTypeUnknownArrayHandleNewInstance
  ////

  VTKM_TEST_ASSERT(originalArraySize == ARRAY_SIZE);
  VTKM_TEST_ASSERT(unknownHandle.GetNumberOfValues() == (2 * ARRAY_SIZE));
  VTKM_TEST_ASSERT(concreteArray.GetNumberOfValues() == (2 * ARRAY_SIZE));
  VTKM_TEST_ASSERT(newArray.GetNumberOfValues() == ARRAY_SIZE);
  VTKM_TEST_ASSERT(newArray.IsType<decltype(concreteArray)>());

  ////
  //// BEGIN-EXAMPLE UnknownArrayHandleBasicInstance
  ////
  vtkm::cont::UnknownArrayHandle indexArray = vtkm::cont::ArrayHandleIndex();
  // Returns an array of type ArrayHandleBasic<vtkm::Id>
  vtkm::cont::UnknownArrayHandle basicArray = indexArray.NewInstanceBasic();
  ////
  //// END-EXAMPLE UnknownArrayHandleBasicInstance
  ////

  VTKM_TEST_ASSERT(basicArray.IsType<vtkm::cont::ArrayHandleBasic<vtkm::Id>>());

  ////
  //// BEGIN-EXAMPLE UnknownArrayHandleFloatInstance
  ////
  vtkm::cont::UnknownArrayHandle intArray = vtkm::cont::ArrayHandleIndex();
  // Returns an array of type ArrayHandleBasic<vtkm::FloatDefault>
  vtkm::cont::UnknownArrayHandle floatArray = intArray.NewInstanceFloatBasic();

  vtkm::cont::UnknownArrayHandle id3Array = vtkm::cont::ArrayHandle<vtkm::Id3>();
  // Returns an array of type ArrayHandleBasic<vtkm::Vec3f>
  vtkm::cont::UnknownArrayHandle float3Array = id3Array.NewInstanceFloatBasic();
  ////
  //// END-EXAMPLE UnknownArrayHandleFloatInstance
  ////

  VTKM_TEST_ASSERT(
    floatArray.IsType<vtkm::cont::ArrayHandleBasic<vtkm::FloatDefault>>());
  VTKM_TEST_ASSERT(float3Array.IsType<vtkm::cont::ArrayHandleBasic<vtkm::Vec3f>>());
}

////
//// BEGIN-EXAMPLE UnknownArrayHandleCanConvert
////
VTKM_CONT vtkm::FloatDefault GetMiddleValue(
  const vtkm::cont::UnknownArrayHandle& unknownArray)
{
  if (unknownArray.CanConvert<vtkm::cont::ArrayHandleConstant<vtkm::FloatDefault>>())
  {
    // Fast path for known array
    vtkm::cont::ArrayHandleConstant<vtkm::FloatDefault> constantArray;
    unknownArray.AsArrayHandle(constantArray);
    return constantArray.GetValue();
  }
  else
  {
    // General path
    auto ranges = vtkm::cont::ArrayRangeCompute(unknownArray);
    vtkm::Range range = ranges.ReadPortal().Get(0);
    return static_cast<vtkm::FloatDefault>((range.Min + range.Max) / 2);
  }
}
////
//// END-EXAMPLE UnknownArrayHandleCanConvert
////

////
//// BEGIN-EXAMPLE UnknownArrayHandleDeepCopy
////
VTKM_CONT vtkm::cont::ArrayHandle<vtkm::FloatDefault> CopyToDefaultArray(
  const vtkm::cont::UnknownArrayHandle& unknownArray)
{
  // Initialize the output UnknownArrayHandle with the array type we want to copy to.
  vtkm::cont::UnknownArrayHandle output = vtkm::cont::ArrayHandle<vtkm::FloatDefault>{};
  output.DeepCopyFrom(unknownArray);
  return output.AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::FloatDefault>>();
}
////
//// END-EXAMPLE UnknownArrayHandleDeepCopy
////

////
//// BEGIN-EXAMPLE UnknownArrayHandleShallowCopy
////
VTKM_CONT vtkm::cont::ArrayHandle<vtkm::FloatDefault> GetAsDefaultArray(
  const vtkm::cont::UnknownArrayHandle& unknownArray)
{
  // Initialize the output UnknownArrayHandle with the array type we want to copy to.
  vtkm::cont::UnknownArrayHandle output = vtkm::cont::ArrayHandle<vtkm::FloatDefault>{};
  output.CopyShallowIfPossible(unknownArray);
  return output.AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::FloatDefault>>();
}
////
//// END-EXAMPLE UnknownArrayHandleShallowCopy
////

void CastUnknownArrayHandle()
{
  ////
  //// BEGIN-EXAMPLE UnknownArrayHandleAsCastArray
  ////
  vtkm::cont::ArrayHandle<vtkm::Float32> originalArray;
  vtkm::cont::UnknownArrayHandle unknownArray = originalArray;

  vtkm::cont::ArrayHandleCast<vtkm::Float64, decltype(originalArray)> castArray;
  unknownArray.AsArrayHandle(castArray);
  ////
  //// END-EXAMPLE UnknownArrayHandleAsCastArray
  ////

  ////
  //// BEGIN-EXAMPLE UnknownArrayHandleAsArrayHandle1
  ////
  vtkm::cont::ArrayHandle<vtkm::Float32> knownArray =
    unknownArray.AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Float32>>();
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
  GetAsDefaultArray(unknownArray);
}

////
//// BEGIN-EXAMPLE UsingCastAndCallForTypes
////
struct PrintArrayContentsFunctor
{
  template<typename T, typename S>
  VTKM_CONT void operator()(const vtkm::cont::ArrayHandle<T, S>& array) const
  {
    this->PrintArrayPortal(array.ReadPortal());
  }

private:
  template<typename PortalType>
  VTKM_CONT void PrintArrayPortal(const PortalType& portal) const
  {
    for (vtkm::Id index = 0; index < portal.GetNumberOfValues(); index++)
    {
      // All ArrayPortal objects have ValueType for the type of each value.
      using ValueType = typename PortalType::ValueType;
      using VTraits = vtkm::VecTraits<ValueType>;

      ValueType value = portal.Get(index);

      vtkm::IdComponent numComponents = VTraits::GetNumberOfComponents(value);
      for (vtkm::IdComponent componentIndex = 0; componentIndex < numComponents;
           componentIndex++)
      {
        std::cout << " " << VTraits::GetComponent(value, componentIndex);
      }
      std::cout << std::endl;
    }
  }
};

void PrintArrayContents(const vtkm::cont::UnknownArrayHandle& array)
{
  array.CastAndCallForTypes<VTKM_DEFAULT_TYPE_LIST, VTKM_DEFAULT_STORAGE_LIST>(
    PrintArrayContentsFunctor{});
}
////
//// END-EXAMPLE UsingCastAndCallForTypes
////

struct MyWorklet : vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldOut);

  template<typename T1, typename T2>
  VTKM_EXEC void operator()(const T1& in, T2& out) const
  {
    using VTraitsIn = vtkm::VecTraits<T1>;
    using VTraitsOut = vtkm::VecTraits<T2>;
    const vtkm::IdComponent numComponents = VTraitsIn::GetNumberOfComponents(in);
    VTKM_ASSERT(numComponents == VTraitsOut::GetNumberOfComponents(out));
    for (vtkm::IdComponent index = 0; index < numComponents; ++index)
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
  vtkm::cont::ArrayHandleIndex implicitArray(ARRAY_SIZE);

  vtkm::cont::ArrayHandle<vtkm::FloatDefault> concreteArray;
  vtkm::cont::Algorithm::Copy(implicitArray, concreteArray);

  vtkm::cont::UnknownArrayHandle unknownArray = concreteArray;

  PrintArrayContents(unknownArray);

  ////
  //// BEGIN-EXAMPLE UncertainArrayHandle
  ////
  vtkm::cont::UncertainArrayHandle<vtkm::TypeListScalarAll, vtkm::cont::StorageListBasic>
    uncertainArray(unknownArray);
  uncertainArray.CastAndCall(PrintArrayContentsFunctor{});
  ////
  //// END-EXAMPLE UncertainArrayHandle
  ////

  vtkm::cont::ArrayHandle<vtkm::FloatDefault> outArray;
  ////
  //// BEGIN-EXAMPLE UnknownArrayResetTypes
  ////
  vtkm::cont::Invoker invoke;
  invoke(
    MyWorklet{},
    unknownArray.ResetTypes<vtkm::TypeListScalarAll, vtkm::cont::StorageListBasic>(),
    outArray);
  ////
  //// END-EXAMPLE UnknownArrayResetTypes
  ////

  ////
  //// BEGIN-EXAMPLE CastAndCallForTypesWithFloatFallback
  ////
  unknownArray.CastAndCallForTypesWithFloatFallback<vtkm::TypeListField,
                                                    VTKM_DEFAULT_STORAGE_LIST>(
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
  vtkm::cont::ArrayHandleBasic<vtkm::Vec3f> concreteArray =
    vtkm::cont::make_ArrayHandle<vtkm::Vec3f>({ { 0, 1, 2 },
                                                { 3, 4, 5 },
                                                { 6, 7, 8 },
                                                { 9, 10, 11 },
                                                { 12, 13, 14 },
                                                { 15, 16, 17 } });

  vtkm::cont::UnknownArrayHandle unknownArray(concreteArray);

  //// LABEL Call
  auto componentArray = unknownArray.ExtractComponent<vtkm::FloatDefault>(0);
  // componentArray contains [ 0, 3, 6, 9, 12, 15 ].
  ////
  //// END-EXAMPLE UnknownArrayExtractComponent
  ////
  VTKM_TEST_ASSERT(componentArray.GetNumberOfValues() ==
                   concreteArray.GetNumberOfValues());
  {
    auto portal = componentArray.ReadPortal();
    auto expectedPortal = concreteArray.ReadPortal();
    for (vtkm::IdComponent i = 0; i < componentArray.GetNumberOfValues(); ++i)
    {
      VTKM_TEST_ASSERT(test_equal(portal.Get(i), expectedPortal.Get(i)[0]));
    }
  }

  VTKM_TEST_ASSERT(
    ////
    //// BEGIN-EXAMPLE UnknownArrayBaseComponentType
    ////
    unknownArray.IsBaseComponentType<vtkm::FloatDefault>()
    ////
    //// END-EXAMPLE UnknownArrayBaseComponentType
    ////
  );

  auto deepTypeArray = vtkm::cont::make_ArrayHandleGroupVec<2>(concreteArray);

  unknownArray = deepTypeArray;
  VTKM_TEST_ASSERT(unknownArray.GetNumberOfComponentsFlat() == 6);

  vtkm::cont::ArrayHandle<vtkm::FloatDefault> outputArray;

  vtkm::cont::Invoker invoke;

  ////
  //// BEGIN-EXAMPLE UnknownArrayExtractComponentsMultiple
  ////
  std::vector<vtkm::cont::ArrayHandle<vtkm::FloatDefault>> outputArrays(
    static_cast<std::size_t>(unknownArray.GetNumberOfComponentsFlat()));
  for (vtkm::IdComponent componentIndex = 0;
       componentIndex < unknownArray.GetNumberOfComponentsFlat();
       ++componentIndex)
  {
    invoke(MyWorklet{},
           unknownArray.ExtractComponent<vtkm::FloatDefault>(componentIndex),
           outputArrays[static_cast<std::size_t>(componentIndex)]);
  }
  ////
  //// END-EXAMPLE UnknownArrayExtractComponentsMultiple
  ////
  for (std::size_t outIndex = 0; outIndex < outputArrays.size(); ++outIndex)
  {
    vtkm::IdComponent vecIndex = static_cast<vtkm::IdComponent>(outIndex % 3);
    vtkm::IdComponent groupIndex = static_cast<vtkm::IdComponent>(outIndex / 3);
    auto portal = outputArrays[outIndex].ReadPortal();
    auto expectedPortal = deepTypeArray.ReadPortal();
    VTKM_TEST_ASSERT(portal.GetNumberOfValues() ==
                     (concreteArray.GetNumberOfValues() / 2));
    for (vtkm::IdComponent i = 0; i < portal.GetNumberOfValues(); ++i)
    {
      VTKM_TEST_ASSERT(
        test_equal(portal.Get(i), expectedPortal.Get(i)[groupIndex][vecIndex]));
    }
  }

  unknownArray = concreteArray;

  vtkm::cont::ArrayHandle<vtkm::Vec3f> outArray;

  ////
  //// BEGIN-EXAMPLE UnknownArrayExtractArrayFromComponents
  ////
  invoke(MyWorklet{},
         unknownArray.ExtractArrayFromComponents<vtkm::FloatDefault>(),
         outArray);
  ////
  //// END-EXAMPLE UnknownArrayExtractArrayFromComponents
  ////
  VTKM_TEST_ASSERT(test_equal_ArrayHandles(outArray, concreteArray));

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
void IndexInitialize(vtkm::Id size, const vtkm::cont::UnknownArrayHandle& output)
{
  vtkm::cont::ArrayHandleIndex input(size);
  output.DeepCopyFrom(input);
}
////
//// END-EXAMPLE UnknownArrayConstOutput
////

////
//// BEGIN-EXAMPLE UseUnknownArrayConstOutput
////
template<typename T>
void Foo(const vtkm::cont::ArrayHandle<T>& input, vtkm::cont::ArrayHandle<T>& output)
{
  IndexInitialize(input.GetNumberOfValues(), output);
  // ...
  ////
  //// END-EXAMPLE UseUnknownArrayConstOutput
  ////

  VTKM_TEST_ASSERT(output.GetNumberOfValues() == input.GetNumberOfValues());
  auto portal = output.ReadPortal();
  for (vtkm::Id index = 0; index < portal.GetNumberOfValues(); ++index)
  {
    VTKM_TEST_ASSERT(portal.Get(index) == index);
  }
}

void TryConstOutput()
{
  vtkm::cont::ArrayHandle<vtkm::Id> input =
    vtkm::cont::make_ArrayHandle<vtkm::Id>({ 3, 6, 1, 4 });
  vtkm::cont::ArrayHandle<vtkm::Id> output;
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
  return vtkm::cont::testing::Testing::Run(Test, argc, argv);
}

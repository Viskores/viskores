//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/CastAndCall.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/Field.h>

#include <vtkm/TypeList.h>
#include <vtkm/TypeTraits.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

// Likely to contain both supported and unsupported types.
using TypesToTry = vtkm::List<vtkm::FloatDefault, vtkm::UInt32, VTKM_UNUSED_INT_TYPE, vtkm::Int8>;

constexpr vtkm::Id DIM_SIZE = 4;
constexpr vtkm::Id ARRAY_SIZE = DIM_SIZE * DIM_SIZE * DIM_SIZE;

template <typename T>
vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>> MakeCoordinates()
{
  vtkm::cont::ArrayHandleUniformPointCoordinates coordArray{ vtkm::Id(DIM_SIZE) };
  VTKM_TEST_ASSERT(coordArray.GetNumberOfValues() == ARRAY_SIZE);
  vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>> castArray;
  vtkm::cont::ArrayCopy(coordArray, castArray);
  return castArray;
}

template <typename T>
vtkm::cont::ArrayHandle<T> MakeField()
{
  vtkm::cont::ArrayHandle<T> fieldArray;
  fieldArray.Allocate(ARRAY_SIZE);
  SetPortal(fieldArray.WritePortal());
  return fieldArray;
}

template <typename T>
vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>> MakeVecField()
{
  return MakeField<vtkm::Vec<T, 3>>();
}

template <typename FieldType>
vtkm::cont::DataSet MakeDataSet()
{
  VTKM_STATIC_ASSERT((std::is_same<typename vtkm::TypeTraits<FieldType>::DimensionalityTag,
                                   vtkm::TypeTraitsScalarTag>::value));

  vtkm::cont::DataSet dataset;

  vtkm::cont::CellSetStructured<3> cellSet;
  cellSet.SetPointDimensions(vtkm::Id3(DIM_SIZE));
  dataset.SetCellSet(cellSet);

  dataset.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coords", MakeCoordinates<FieldType>()));
  dataset.AddPointField("scalars", MakeField<FieldType>());
  dataset.AddPointField("vectors", MakeVecField<FieldType>());

  VTKM_TEST_ASSERT(dataset.GetNumberOfPoints() == ARRAY_SIZE);

  return dataset;
}

struct CheckCoords
{
  template <typename ArrayType>
  void operator()(const ArrayType& array) const
  {
    VTKM_TEST_ASSERT(test_equal_ArrayHandles(array, MakeCoordinates<vtkm::FloatDefault>()));
  }
};

template <typename T>
struct CheckField
{
  template <typename ArrayType>
  void operator()(const ArrayType& array) const
  {
    auto expectedArray = MakeField<T>();
    VTKM_TEST_ASSERT(test_equal_ArrayHandles(array, expectedArray));
  }
};

struct TryType
{
  template <typename FieldType>
  void operator()(FieldType) const
  {
    using VecType = vtkm::Vec<FieldType, 3>;

    std::cout << "Creating data." << std::endl;
    vtkm::cont::DataSet data = MakeDataSet<FieldType>();

    std::cout << "Check original data." << std::endl;
    CheckCoords{}(
      data.GetCoordinateSystem().GetData().AsArrayHandle<vtkm::cont::ArrayHandle<VecType>>());
    CheckField<FieldType>{}(
      data.GetPointField("scalars").GetData().AsArrayHandle<vtkm::cont::ArrayHandle<FieldType>>());
    CheckField<VecType>{}(
      data.GetPointField("vectors").GetData().AsArrayHandle<vtkm::cont::ArrayHandle<VecType>>());

    VTKM_TEST_ASSERT((data.GetCoordinateSystem().IsSupportedType() ==
                      vtkm::ListHas<VTKM_DEFAULT_TYPE_LIST, VecType>::value));
    VTKM_TEST_ASSERT((data.GetField("scalars").IsSupportedType() ==
                      vtkm::ListHas<VTKM_DEFAULT_TYPE_LIST, FieldType>::value));
    VTKM_TEST_ASSERT((data.GetField("vectors").IsSupportedType() ==
                      vtkm::ListHas<VTKM_DEFAULT_TYPE_LIST, VecType>::value));

    std::cout << "Check as float default." << std::endl;
    CheckCoords{}(data.GetCoordinateSystem()
                    .GetDataAsDefaultFloat()
                    .AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Vec3f>>());
    CheckField<FieldType>{}(data.GetPointField("scalars")
                              .GetDataAsDefaultFloat()
                              .AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::FloatDefault>>());
    CheckField<VecType>{}(data.GetPointField("vectors")
                            .GetDataAsDefaultFloat()
                            .AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Vec3f>>());

    std::cout << "Check as expected type." << std::endl;
    vtkm::cont::CastAndCall(data.GetCoordinateSystem().GetDataWithExpectedTypes(), CheckCoords{});
    vtkm::cont::CastAndCall(data.GetPointField("scalars").GetDataWithExpectedTypes(),
                            CheckField<FieldType>{});
    vtkm::cont::CastAndCall(data.GetPointField("vectors").GetDataWithExpectedTypes(),
                            CheckField<VecType>{});

    std::cout << "Convert to expected and check." << std::endl;
    data.ConvertToExpected();
    vtkm::cont::CastAndCall(data.GetCoordinateSystem(), CheckCoords{});
    vtkm::cont::CastAndCall(data.GetPointField("scalars"), CheckField<FieldType>{});
    vtkm::cont::CastAndCall(data.GetPointField("vectors"), CheckField<VecType>{});
  }
};

void Run()
{
  VTKM_TEST_ASSERT(vtkm::ListHas<VTKM_DEFAULT_TYPE_LIST, vtkm::FloatDefault>::value,
                   "This test assumes that VTKM_DEFAULT_TYPE_LIST has vtkm::FloatDefault. "
                   "If there is a reason for this condition, then a special condition needs "
                   "to be added to skip this test.");
  VTKM_TEST_ASSERT(vtkm::ListHas<VTKM_DEFAULT_TYPE_LIST, vtkm::Vec3f>::value,
                   "This test assumes that VTKM_DEFAULT_TYPE_LIST has vtkm::Vec3f. "
                   "If there is a reason for this condition, then a special condition needs "
                   "to be added to skip this test.");

  vtkm::testing::Testing::TryTypes(TryType{}, TypesToTry{});
}

} // anonymous namespace

int UnitTestDataSetConvertToExpected(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(Run, argc, argv);
}

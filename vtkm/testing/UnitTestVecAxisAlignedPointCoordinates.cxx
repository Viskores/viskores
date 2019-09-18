//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/VecAxisAlignedPointCoordinates.h>

#include <vtkm/testing/Testing.h>

namespace
{

using Vec3 = vtkm::Vec3f;

static const Vec3 g_Origin = Vec3(1.0f, 2.0f, 3.0f);
static const Vec3 g_Spacing = Vec3(4.0f, 5.0f, 6.0f);

static const Vec3 g_Coords[8] = { Vec3(1.0f, 2.0f, 3.0f), Vec3(5.0f, 2.0f, 3.0f),
                                  Vec3(5.0f, 7.0f, 3.0f), Vec3(1.0f, 7.0f, 3.0f),
                                  Vec3(1.0f, 2.0f, 9.0f), Vec3(5.0f, 2.0f, 9.0f),
                                  Vec3(5.0f, 7.0f, 9.0f), Vec3(1.0f, 7.0f, 9.0f) };

// You will get a compile fail if this does not pass
void CheckNumericTag(vtkm::TypeTraitsRealTag)
{
  std::cout << "NumericTag pass" << std::endl;
}

// You will get a compile fail if this does not pass
void CheckDimensionalityTag(vtkm::TypeTraitsVectorTag)
{
  std::cout << "VectorTag pass" << std::endl;
}

// You will get a compile fail if this does not pass
void CheckComponentType(Vec3)
{
  std::cout << "ComponentType pass" << std::endl;
}

// You will get a compile fail if this does not pass
void CheckHasMultipleComponents(vtkm::VecTraitsTagMultipleComponents)
{
  std::cout << "MultipleComponents pass" << std::endl;
}

// You will get a compile fail if this does not pass
void CheckVariableSize(vtkm::VecTraitsTagSizeStatic)
{
  std::cout << "StaticSize" << std::endl;
}

template <typename VecCoordsType>
void CheckCoordsValues(const VecCoordsType& coords)
{
  for (vtkm::IdComponent pointIndex = 0; pointIndex < VecCoordsType::NUM_COMPONENTS; pointIndex++)
  {
    VTKM_TEST_ASSERT(test_equal(coords[pointIndex], g_Coords[pointIndex]),
                     "Incorrect point coordinate.");
  }
}

template <vtkm::IdComponent NumDimensions>
void TryVecAxisAlignedPointCoordinates(
  const vtkm::VecAxisAlignedPointCoordinates<NumDimensions>& coords)
{
  using VecCoordsType = vtkm::VecAxisAlignedPointCoordinates<NumDimensions>;
  using TTraits = vtkm::TypeTraits<VecCoordsType>;
  using VTraits = vtkm::VecTraits<VecCoordsType>;

  std::cout << "Check traits tags." << std::endl;
  CheckNumericTag(typename TTraits::NumericTag());
  CheckDimensionalityTag(typename TTraits::DimensionalityTag());
  CheckComponentType(typename VTraits::ComponentType());
  CheckHasMultipleComponents(typename VTraits::HasMultipleComponents());
  CheckVariableSize(typename VTraits::IsSizeStatic());

  std::cout << "Check size." << std::endl;
  VTKM_TEST_ASSERT(coords.GetNumberOfComponents() == VecCoordsType::NUM_COMPONENTS,
                   "Wrong number of components.");
  VTKM_TEST_ASSERT(VTraits::GetNumberOfComponents(coords) == VecCoordsType::NUM_COMPONENTS,
                   "Wrong number of components.");

  std::cout << "Check contents." << std::endl;
  CheckCoordsValues(coords);

  std::cout << "Check CopyInto." << std::endl;
  vtkm::Vec<vtkm::Vec3f, VecCoordsType::NUM_COMPONENTS> copy1;
  coords.CopyInto(copy1);
  CheckCoordsValues(copy1);

  vtkm::Vec<vtkm::Vec3f, VecCoordsType::NUM_COMPONENTS> copy2;
  VTraits::CopyInto(coords, copy2);
  CheckCoordsValues(copy2);

  std::cout << "Check origin and spacing." << std::endl;
  VTKM_TEST_ASSERT(test_equal(coords.GetOrigin(), g_Origin), "Wrong origin.");
  VTKM_TEST_ASSERT(test_equal(coords.GetSpacing(), g_Spacing), "Wrong spacing");
}

void TestVecAxisAlignedPointCoordinates()
{
  std::cout << "***** 1D Coordinates *****************" << std::endl;
  vtkm::VecAxisAlignedPointCoordinates<1> coords1d(g_Origin, g_Spacing);
  VTKM_TEST_ASSERT(coords1d.NUM_COMPONENTS == 2, "Wrong number of components");
  VTKM_TEST_ASSERT(vtkm::VecAxisAlignedPointCoordinates<1>::NUM_COMPONENTS == 2,
                   "Wrong number of components");
  VTKM_TEST_ASSERT(vtkm::VecTraits<vtkm::VecAxisAlignedPointCoordinates<1>>::NUM_COMPONENTS == 2,
                   "Wrong number of components");
  TryVecAxisAlignedPointCoordinates(coords1d);

  std::cout << "***** 2D Coordinates *****************" << std::endl;
  vtkm::VecAxisAlignedPointCoordinates<2> coords2d(g_Origin, g_Spacing);
  VTKM_TEST_ASSERT(coords2d.NUM_COMPONENTS == 4, "Wrong number of components");
  VTKM_TEST_ASSERT(vtkm::VecAxisAlignedPointCoordinates<2>::NUM_COMPONENTS == 4,
                   "Wrong number of components");
  VTKM_TEST_ASSERT(vtkm::VecTraits<vtkm::VecAxisAlignedPointCoordinates<2>>::NUM_COMPONENTS == 4,
                   "Wrong number of components");
  TryVecAxisAlignedPointCoordinates(coords2d);

  std::cout << "***** 3D Coordinates *****************" << std::endl;
  vtkm::VecAxisAlignedPointCoordinates<3> coords3d(g_Origin, g_Spacing);
  VTKM_TEST_ASSERT(coords3d.NUM_COMPONENTS == 8, "Wrong number of components");
  VTKM_TEST_ASSERT(vtkm::VecAxisAlignedPointCoordinates<3>::NUM_COMPONENTS == 8,
                   "Wrong number of components");
  VTKM_TEST_ASSERT(vtkm::VecTraits<vtkm::VecAxisAlignedPointCoordinates<3>>::NUM_COMPONENTS == 8,
                   "Wrong number of components");
  TryVecAxisAlignedPointCoordinates(coords3d);
}

} // anonymous namespace

int UnitTestVecAxisAlignedPointCoordinates(int argc, char* argv[])
{
  return vtkm::testing::Testing::Run(TestVecAxisAlignedPointCoordinates, argc, argv);
}

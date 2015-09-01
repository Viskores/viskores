//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/VecRectilinearPointCoordinates.h>

#include <vtkm/testing/Testing.h>

namespace {

typedef vtkm::Vec<vtkm::FloatDefault,3> Vec3;

static const Vec3 g_Origin = Vec3(1.0f, 2.0f, 3.0f);
static const Vec3 g_Spacing = Vec3(4.0f, 5.0f, 6.0f);

static const Vec3 g_Coords[8] = {
  Vec3(1.0f, 2.0f, 3.0f),
  Vec3(5.0f, 2.0f, 3.0f),
  Vec3(5.0f, 7.0f, 3.0f),
  Vec3(1.0f, 7.0f, 3.0f),
  Vec3(1.0f, 2.0f, 9.0f),
  Vec3(5.0f, 2.0f, 9.0f),
  Vec3(5.0f, 7.0f, 9.0f),
  Vec3(1.0f, 7.0f, 9.0f)
};

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

template<typename VecCoordsType>
void CheckCoordsValues(const VecCoordsType &coords)
{
  for (vtkm::IdComponent pointIndex = 0;
       pointIndex < VecCoordsType::NUM_COMPONENTS;
       pointIndex++)
  {
    VTKM_TEST_ASSERT(test_equal(coords[pointIndex], g_Coords[pointIndex]),
                     "Incorrect point coordinate.");
  }
}

template<vtkm::IdComponent NumDimensions>
void TryVecRectilinearPointCoordinates(
    const vtkm::VecRectilinearPointCoordinates<NumDimensions> &coords)
{
  typedef vtkm::VecRectilinearPointCoordinates<NumDimensions> VecCoordsType;
  typedef vtkm::TypeTraits<VecCoordsType> TTraits;
  typedef vtkm::VecTraits<VecCoordsType> VTraits;

  std::cout << "Check traits tags." << std::endl;
  CheckNumericTag(typename TTraits::NumericTag());
  CheckDimensionalityTag(typename TTraits::DimensionalityTag());
  CheckComponentType(typename VTraits::ComponentType());
  CheckHasMultipleComponents(typename VTraits::HasMultipleComponents());
  CheckVariableSize(typename VTraits::IsSizeStatic());

  std::cout << "Check size." << std::endl;
  VTKM_TEST_ASSERT(
        coords.GetNumberOfComponents() == VecCoordsType::NUM_COMPONENTS,
        "Wrong number of components.");
  VTKM_TEST_ASSERT(
        VTraits::GetNumberOfComponents(coords) == VecCoordsType::NUM_COMPONENTS,
        "Wrong number of components.");

  std::cout << "Check contents." << std::endl;
  CheckCoordsValues(coords);

  std::cout << "Check CopyInto." << std::endl;
  vtkm::Vec<vtkm::Vec<vtkm::FloatDefault,3>,VecCoordsType::NUM_COMPONENTS> copy1;
  coords.CopyInto(copy1);
  CheckCoordsValues(copy1);

  vtkm::Vec<vtkm::Vec<vtkm::FloatDefault,3>,VecCoordsType::NUM_COMPONENTS> copy2;
  VTraits::CopyInto(coords, copy2);
  CheckCoordsValues(copy2);
}

void TestVecRectilinearPointCoordinates()
{
  std::cout << "***** 1D Coordinates *****************" << std::endl;
  vtkm::VecRectilinearPointCoordinates<1> coords1d(g_Origin, g_Spacing);
  VTKM_TEST_ASSERT(coords1d.NUM_COMPONENTS == 2,
                   "Wrong number of components");
  VTKM_TEST_ASSERT(vtkm::VecRectilinearPointCoordinates<1>::NUM_COMPONENTS == 2,
                   "Wrong number of components");
  VTKM_TEST_ASSERT(vtkm::VecTraits<vtkm::VecRectilinearPointCoordinates<1> >::NUM_COMPONENTS == 2,
                   "Wrong number of components");
  TryVecRectilinearPointCoordinates(coords1d);

  std::cout << "***** 2D Coordinates *****************" << std::endl;
  vtkm::VecRectilinearPointCoordinates<2> coords2d(g_Origin, g_Spacing);
  VTKM_TEST_ASSERT(coords2d.NUM_COMPONENTS == 4,
                   "Wrong number of components");
  VTKM_TEST_ASSERT(vtkm::VecRectilinearPointCoordinates<2>::NUM_COMPONENTS == 4,
                   "Wrong number of components");
  VTKM_TEST_ASSERT(vtkm::VecTraits<vtkm::VecRectilinearPointCoordinates<2> >::NUM_COMPONENTS == 4,
                   "Wrong number of components");
  TryVecRectilinearPointCoordinates(coords2d);

  std::cout << "***** 3D Coordinates *****************" << std::endl;
  vtkm::VecRectilinearPointCoordinates<3> coords3d(g_Origin, g_Spacing);
  VTKM_TEST_ASSERT(coords3d.NUM_COMPONENTS == 8,
                   "Wrong number of components");
  VTKM_TEST_ASSERT(vtkm::VecRectilinearPointCoordinates<3>::NUM_COMPONENTS == 8,
                   "Wrong number of components");
  VTKM_TEST_ASSERT(vtkm::VecTraits<vtkm::VecRectilinearPointCoordinates<3> >::NUM_COMPONENTS == 8,
                   "Wrong number of components");
  TryVecRectilinearPointCoordinates(coords3d);
}

} // anonymous namespace

int UnitTestVecRectilinearPointCoordinates(int, char *[])
{
  return vtkm::testing::Testing::Run(TestVecRectilinearPointCoordinates);
}

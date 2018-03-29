//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/internal/ArrayPortalValueReference.h>

#include <vtkm/cont/ArrayHandle.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

template <typename ArrayPortalType>
void SetReference(vtkm::Id index, vtkm::internal::ArrayPortalValueReference<ArrayPortalType> ref)
{
  using ValueType = typename ArrayPortalType::ValueType;
  ref = TestValue(index, ValueType());
}

template <typename ArrayPortalType>
void CheckReference(vtkm::Id index, vtkm::internal::ArrayPortalValueReference<ArrayPortalType> ref)
{
  using ValueType = typename ArrayPortalType::ValueType;
  VTKM_TEST_ASSERT(test_equal(ref, TestValue(index, ValueType())), "Got bad value from reference.");
}

static constexpr vtkm::Id ARRAY_SIZE = 10;

struct DoTestForType
{
  template <typename ValueType>
  VTKM_CONT void operator()(const ValueType&) const
  {
    vtkm::cont::ArrayHandle<ValueType> array;
    array.Allocate(ARRAY_SIZE);

    std::cout << "Set array using reference" << std::endl;
    using PortalType = typename vtkm::cont::ArrayHandle<ValueType>::PortalControl;
    PortalType portal = array.GetPortalControl();
    for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
    {
      SetReference(index, vtkm::internal::ArrayPortalValueReference<PortalType>(portal, index));
    }

    std::cout << "Check values" << std::endl;
    CheckPortal(portal);

    std::cout << "Check references in set array." << std::endl;
    for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
    {
      CheckReference(index, vtkm::internal::ArrayPortalValueReference<PortalType>(portal, index));
    }
  }
};

void DoTest()
{
  vtkm::testing::Testing::TryTypes(DoTestForType());
}

} // anonymous namespace

int UnitTestArrayPortalValueReference(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(DoTest);
}

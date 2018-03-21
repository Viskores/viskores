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

#include <vtkm/cont/testing/Testing.h>
#include <vtkm/filter/FieldSelection.h>

namespace
{
void TestFieldSelection()
{
  {
    // empty field selection,  everything should be false.
    vtkm::filter::FieldSelection selection;
    VTKM_TEST_ASSERT(selection.IsFieldSelected("foo") == false, "field selection failed.");
    VTKM_TEST_ASSERT(selection.IsFieldSelected("bar", vtkm::cont::Field::ASSOC_POINTS) == false,
                     "field selection failed.");
  }

  {
    // field selection with select all,  everything should be true.
    vtkm::filter::FieldSelection selection(vtkm::filter::FieldSelection::MODE_ALL);
    VTKM_TEST_ASSERT(selection.IsFieldSelected("foo") == true, "field selection failed.");
    VTKM_TEST_ASSERT(selection.IsFieldSelected("bar", vtkm::cont::Field::ASSOC_POINTS) == true,
                     "field selection failed.");
  }

  {
    // field selection with select none,  everything should be false no matter what.
    vtkm::filter::FieldSelection selection(vtkm::filter::FieldSelection::MODE_NONE);
    VTKM_TEST_ASSERT(selection.IsFieldSelected("foo") == false, "field selection failed.");
    VTKM_TEST_ASSERT(selection.IsFieldSelected("bar", vtkm::cont::Field::ASSOC_POINTS) == false,
                     "field selection failed.");
  }

  {
    // field selection with specific fields selected.
    vtkm::filter::FieldSelection selection;
    selection.AddField("foo");
    selection.AddField("bar", vtkm::cont::Field::ASSOC_CELL_SET);
    VTKM_TEST_ASSERT(selection.IsFieldSelected("foo") == true, "field selection failed.");
    VTKM_TEST_ASSERT(selection.IsFieldSelected("foo", vtkm::cont::Field::ASSOC_POINTS) == true,
                     "field selection failed.");
    VTKM_TEST_ASSERT(selection.IsFieldSelected("bar", vtkm::cont::Field::ASSOC_POINTS) == false,
                     "field selection failed.");
    VTKM_TEST_ASSERT(selection.IsFieldSelected("bar", vtkm::cont::Field::ASSOC_CELL_SET) == true,
                     "field selection failed.");
    VTKM_TEST_ASSERT(selection.IsFieldSelected("bar") == true, "field selection failed.");
  }

  {
    // field selection with specific fields selected.
    vtkm::filter::FieldSelection selection{ "foo", "bar" };
    VTKM_TEST_ASSERT(selection.IsFieldSelected("foo") == true, "field selection failed.");
    VTKM_TEST_ASSERT(selection.IsFieldSelected("foo", vtkm::cont::Field::ASSOC_POINTS) == true,
                     "field selection failed.");
    VTKM_TEST_ASSERT(selection.IsFieldSelected("bar", vtkm::cont::Field::ASSOC_POINTS) == true,
                     "field selection failed.");
    VTKM_TEST_ASSERT(selection.IsFieldSelected("bar", vtkm::cont::Field::ASSOC_CELL_SET) == true,
                     "field selection failed.");
    VTKM_TEST_ASSERT(selection.IsFieldSelected("bar") == true, "field selection failed.");
  }

  {
    // field selection with specific fields selected.
    using pair_type = std::pair<std::string, vtkm::cont::Field::AssociationEnum>;
    vtkm::filter::FieldSelection selection{ pair_type{ "foo", vtkm::cont::Field::ASSOC_ANY },
                                            pair_type{ "bar", vtkm::cont::Field::ASSOC_CELL_SET } };
    VTKM_TEST_ASSERT(selection.IsFieldSelected("foo") == true, "field selection failed.");
    VTKM_TEST_ASSERT(selection.IsFieldSelected("foo", vtkm::cont::Field::ASSOC_POINTS) == true,
                     "field selection failed.");
    VTKM_TEST_ASSERT(selection.IsFieldSelected("bar", vtkm::cont::Field::ASSOC_POINTS) == false,
                     "field selection failed.");
    VTKM_TEST_ASSERT(selection.IsFieldSelected("bar", vtkm::cont::Field::ASSOC_CELL_SET) == true,
                     "field selection failed.");
    VTKM_TEST_ASSERT(selection.IsFieldSelected("bar") == true, "field selection failed.");
  }
}
}

int UnitTestFieldSelection(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(TestFieldSelection);
}

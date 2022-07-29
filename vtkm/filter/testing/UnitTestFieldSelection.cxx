//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/testing/Testing.h>
#include <vtkm/filter/FieldSelection.h>

namespace
{
void TestFieldSelection()
{
  {
    std::cout << "empty field selection,  everything should be false." << std::endl;
    vtkm::filter::FieldSelection selection;
    VTKM_TEST_ASSERT(selection.IsFieldSelected("foo") == false, "field selection failed.");
    VTKM_TEST_ASSERT(selection.IsFieldSelected("bar", vtkm::cont::Field::Association::Points) ==
                       false,
                     "field selection failed.");
  }

  {
    std::cout << "field selection with select all,  everything should be true." << std::endl;
    vtkm::filter::FieldSelection selection(vtkm::filter::FieldSelection::Mode::All);
    VTKM_TEST_ASSERT(selection.IsFieldSelected("foo") == true, "field selection failed.");
    VTKM_TEST_ASSERT(selection.IsFieldSelected("bar", vtkm::cont::Field::Association::Points) ==
                       true,
                     "field selection failed.");
  }

  {
    std::cout << "field selection with select none,  everything should be false." << std::endl;
    vtkm::filter::FieldSelection selection(vtkm::filter::FieldSelection::Mode::None);
    VTKM_TEST_ASSERT(selection.IsFieldSelected("foo") == false, "field selection failed.");
    VTKM_TEST_ASSERT(selection.IsFieldSelected("bar", vtkm::cont::Field::Association::Points) ==
                       false,
                     "field selection failed.");
  }

  {
    std::cout << "field selection of one field" << std::endl;
    vtkm::filter::FieldSelection selection("foo");
    VTKM_TEST_ASSERT(selection.IsFieldSelected("foo") == true, "field selection failed.");
    VTKM_TEST_ASSERT(selection.IsFieldSelected("foo", vtkm::cont::Field::Association::Points) ==
                       true,
                     "field selection failed.");
    VTKM_TEST_ASSERT(selection.IsFieldSelected("foo", vtkm::cont::Field::Association::Cells) ==
                       true,
                     "field selection failed.");
    VTKM_TEST_ASSERT(selection.IsFieldSelected("bar") == false, "field selection failed.");
  }

  {
    std::cout << "field selection of one field/association" << std::endl;
    vtkm::filter::FieldSelection selection("foo", vtkm::cont::Field::Association::Points);
    VTKM_TEST_ASSERT(selection.IsFieldSelected("foo") == true, "field selection failed.");
    VTKM_TEST_ASSERT(selection.IsFieldSelected("foo", vtkm::cont::Field::Association::Points) ==
                       true,
                     "field selection failed.");
    VTKM_TEST_ASSERT(selection.IsFieldSelected("foo", vtkm::cont::Field::Association::Cells) ==
                       false,
                     "field selection failed.");
    VTKM_TEST_ASSERT(selection.IsFieldSelected("bar") == false, "field selection failed.");
  }

  {
    std::cout << "field selection with specific fields selected (AddField)." << std::endl;
    vtkm::filter::FieldSelection selection;
    selection.AddField("foo");
    selection.AddField("bar", vtkm::cont::Field::Association::Cells);
    VTKM_TEST_ASSERT(selection.IsFieldSelected("foo") == true, "field selection failed.");
    VTKM_TEST_ASSERT(selection.IsFieldSelected("foo", vtkm::cont::Field::Association::Points) ==
                       true,
                     "field selection failed.");
    VTKM_TEST_ASSERT(selection.IsFieldSelected("bar", vtkm::cont::Field::Association::Points) ==
                       false,
                     "field selection failed.");
    VTKM_TEST_ASSERT(selection.IsFieldSelected("bar", vtkm::cont::Field::Association::Cells) ==
                       true,
                     "field selection failed.");
    VTKM_TEST_ASSERT(selection.IsFieldSelected("bar") == true, "field selection failed.");
  }

  {
    std::cout << "field selection with specific fields selected (initializer list)." << std::endl;
    vtkm::filter::FieldSelection selection{ "foo", "bar" };
    VTKM_TEST_ASSERT(selection.IsFieldSelected("foo") == true, "field selection failed.");
    VTKM_TEST_ASSERT(selection.IsFieldSelected("foo", vtkm::cont::Field::Association::Points) ==
                       true,
                     "field selection failed.");
    VTKM_TEST_ASSERT(selection.IsFieldSelected("bar", vtkm::cont::Field::Association::Points) ==
                       true,
                     "field selection failed.");
    VTKM_TEST_ASSERT(selection.IsFieldSelected("bar", vtkm::cont::Field::Association::Cells) ==
                       true,
                     "field selection failed.");
    VTKM_TEST_ASSERT(selection.IsFieldSelected("bar") == true, "field selection failed.");
  }

  {
    std::cout << "field selection with specific fields selected (std::pair initializer list)."
              << std::endl;
    using pair_type = std::pair<std::string, vtkm::cont::Field::Association>;
    vtkm::filter::FieldSelection selection{ pair_type{ "foo", vtkm::cont::Field::Association::Any },
                                            pair_type{ "bar",
                                                       vtkm::cont::Field::Association::Cells } };
    VTKM_TEST_ASSERT(selection.IsFieldSelected("foo") == true, "field selection failed.");
    VTKM_TEST_ASSERT(selection.IsFieldSelected("foo", vtkm::cont::Field::Association::Points) ==
                       true,
                     "field selection failed.");
    VTKM_TEST_ASSERT(selection.IsFieldSelected("bar", vtkm::cont::Field::Association::Points) ==
                       false,
                     "field selection failed.");
    VTKM_TEST_ASSERT(selection.IsFieldSelected("bar", vtkm::cont::Field::Association::Cells) ==
                       true,
                     "field selection failed.");
    VTKM_TEST_ASSERT(selection.IsFieldSelected("bar") == true, "field selection failed.");
  }

  {
    std::cout << "field selection with specific fields selected (vtkm::Pair initializer list)."
              << std::endl;
    using pair_type = vtkm::Pair<std::string, vtkm::cont::Field::Association>;
    vtkm::filter::FieldSelection selection{ pair_type{ "foo", vtkm::cont::Field::Association::Any },
                                            pair_type{ "bar",
                                                       vtkm::cont::Field::Association::Cells } };
    VTKM_TEST_ASSERT(selection.IsFieldSelected("foo") == true, "field selection failed.");
    VTKM_TEST_ASSERT(selection.IsFieldSelected("foo", vtkm::cont::Field::Association::Points) ==
                       true,
                     "field selection failed.");
    VTKM_TEST_ASSERT(selection.IsFieldSelected("bar", vtkm::cont::Field::Association::Points) ==
                       false,
                     "field selection failed.");
    VTKM_TEST_ASSERT(selection.IsFieldSelected("bar", vtkm::cont::Field::Association::Cells) ==
                       true,
                     "field selection failed.");
    VTKM_TEST_ASSERT(selection.IsFieldSelected("bar") == true, "field selection failed.");
  }

  {
    std::cout << "field selection with specific fields excluded." << std::endl;
    using pair_type = std::pair<std::string, vtkm::cont::Field::Association>;
    vtkm::filter::FieldSelection selection(
      { pair_type{ "foo", vtkm::cont::Field::Association::Any },
        pair_type{ "bar", vtkm::cont::Field::Association::Cells } },
      vtkm::filter::FieldSelection::Mode::Exclude);
    VTKM_TEST_ASSERT(selection.IsFieldSelected("foo") == false, "field selection failed.");
    VTKM_TEST_ASSERT(selection.IsFieldSelected("foo", vtkm::cont::Field::Association::Points) ==
                       false,
                     "field selection failed.");
    VTKM_TEST_ASSERT(selection.IsFieldSelected("bar", vtkm::cont::Field::Association::Points) ==
                       true,
                     "field selection failed.");
    VTKM_TEST_ASSERT(selection.IsFieldSelected("bar", vtkm::cont::Field::Association::Cells) ==
                       false,
                     "field selection failed.");
    VTKM_TEST_ASSERT(selection.IsFieldSelected("bar") == false, "field selection failed.");
    VTKM_TEST_ASSERT(selection.IsFieldSelected("baz") == true, "field selection failed.");
  }
}
}

int UnitTestFieldSelection(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestFieldSelection, argc, argv);
}

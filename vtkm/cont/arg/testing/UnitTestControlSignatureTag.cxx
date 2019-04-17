//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

void TestControlSignatures()
{
  VTKM_IS_CONTROL_SIGNATURE_TAG(vtkm::worklet::WorkletMapField::FieldIn);

  VTKM_TEST_ASSERT(vtkm::cont::arg::internal::ControlSignatureTagCheck<
                     vtkm::worklet::WorkletMapField::FieldIn>::Valid,
                   "Bad check for FieldIn");

  VTKM_TEST_ASSERT(vtkm::cont::arg::internal::ControlSignatureTagCheck<
                     vtkm::worklet::WorkletMapField::FieldOut>::Valid,
                   "Bad check for FieldOut");

  VTKM_TEST_ASSERT(
    !vtkm::cont::arg::internal::ControlSignatureTagCheck<vtkm::exec::arg::WorkIndex>::Valid,
    "Bad check for WorkIndex");

  VTKM_TEST_ASSERT(!vtkm::cont::arg::internal::ControlSignatureTagCheck<vtkm::Id>::Valid,
                   "Bad check for vtkm::Id");
}

} // anonymous namespace

int UnitTestControlSignatureTag(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestControlSignatures, argc, argv);
}

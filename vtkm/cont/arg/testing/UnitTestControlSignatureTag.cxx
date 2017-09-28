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

#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

void TestControlSignatures()
{
  VTKM_IS_CONTROL_SIGNATURE_TAG(vtkm::worklet::WorkletMapField::FieldIn<vtkm::Float32>);

  VTKM_TEST_ASSERT(vtkm::cont::arg::internal::ControlSignatureTagCheck<
                     vtkm::worklet::WorkletMapField::FieldIn<vtkm::Id>>::Valid,
                   "Bad check for FieldIn");

  VTKM_TEST_ASSERT(vtkm::cont::arg::internal::ControlSignatureTagCheck<
                     vtkm::worklet::WorkletMapField::FieldOut<vtkm::Id>>::Valid,
                   "Bad check for FieldOut");

  VTKM_TEST_ASSERT(
    !vtkm::cont::arg::internal::ControlSignatureTagCheck<vtkm::exec::arg::WorkIndex>::Valid,
    "Bad check for WorkIndex");

  VTKM_TEST_ASSERT(!vtkm::cont::arg::internal::ControlSignatureTagCheck<vtkm::Id>::Valid,
                   "Bad check for vtkm::Id");
}

} // anonymous namespace

int UnitTestControlSignatureTag(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(TestControlSignatures);
}

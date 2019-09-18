//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/RuntimeDeviceInformation.h>

#include <vtkm/cont/cuda/DeviceAdapterCuda.h>
#include <vtkm/cont/openmp/DeviceAdapterOpenMP.h>
#include <vtkm/cont/serial/DeviceAdapterSerial.h>
#include <vtkm/cont/tbb/DeviceAdapterTBB.h>

#include <vtkm/cont/testing/Testing.h>

#include <cctype> //for tolower

namespace
{

template <typename Tag>
void TestName(const std::string& name, Tag tag, vtkm::cont::DeviceAdapterId id)
{
  vtkm::cont::RuntimeDeviceInformation info;

  VTKM_TEST_ASSERT(id.GetName() == name, "Id::GetName() failed.");
  VTKM_TEST_ASSERT(tag.GetName() == name, "Tag::GetName() failed.");
  VTKM_TEST_ASSERT(vtkm::cont::make_DeviceAdapterId(id.GetValue()) == id,
                   "make_DeviceAdapterId(int8) failed");

  VTKM_TEST_ASSERT(info.GetName(id) == name, "RDeviceInfo::GetName(Id) failed.");
  VTKM_TEST_ASSERT(info.GetName(tag) == name, "RDeviceInfo::GetName(Tag) failed.");
  VTKM_TEST_ASSERT(info.GetId(name) == id, "RDeviceInfo::GetId(name) failed.");

  //check going from name to device id
  auto lowerCaseFunc = [](char c) {
    return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  };

  auto upperCaseFunc = [](char c) {
    return static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  };

  if (id.IsValueValid())
  { //only test make_DeviceAdapterId with valid device ids
    VTKM_TEST_ASSERT(
      vtkm::cont::make_DeviceAdapterId(name) == id, "make_DeviceAdapterId(", name, ") failed");

    std::string casedName = name;
    std::transform(casedName.begin(), casedName.end(), casedName.begin(), lowerCaseFunc);
    VTKM_TEST_ASSERT(
      vtkm::cont::make_DeviceAdapterId(casedName) == id, "make_DeviceAdapterId(", name, ") failed");

    std::transform(casedName.begin(), casedName.end(), casedName.begin(), upperCaseFunc);
    VTKM_TEST_ASSERT(
      vtkm::cont::make_DeviceAdapterId(casedName) == id, "make_DeviceAdapterId(", name, ") failed");
  }
}

void TestNames()
{
  vtkm::cont::DeviceAdapterTagUndefined undefinedTag;
  vtkm::cont::DeviceAdapterTagSerial serialTag;
  vtkm::cont::DeviceAdapterTagTBB tbbTag;
  vtkm::cont::DeviceAdapterTagOpenMP openmpTag;
  vtkm::cont::DeviceAdapterTagCuda cudaTag;

  TestName("Undefined", undefinedTag, undefinedTag);
  TestName("Serial", serialTag, serialTag);
  TestName("TBB", tbbTag, tbbTag);
  TestName("OpenMP", openmpTag, openmpTag);
  TestName("Cuda", cudaTag, cudaTag);
}

} // end anon namespace

int UnitTestRuntimeDeviceNames(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestNames, argc, argv);
}

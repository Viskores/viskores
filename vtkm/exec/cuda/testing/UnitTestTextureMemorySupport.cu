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
#include <vtkm/exec/cuda/internal/ArrayPortalFromThrust.h>

namespace
{

struct customType
{
};

void TestScalarTextureLoad()
{
  using namespace vtkm::exec::cuda::internal;
  using f = load_through_texture<vtkm::Float32>;
  using i = load_through_texture<vtkm::Int32>;
  using ui = load_through_texture<vtkm::UInt8>;

  using ct = load_through_texture<customType>;

  VTKM_TEST_ASSERT(f::WillUseTexture == 1, "Float32 can be loaded through texture memory");
  VTKM_TEST_ASSERT(i::WillUseTexture == 1, "Int32 can be loaded through texture memory");
  VTKM_TEST_ASSERT(ui::WillUseTexture == 1, "Unsigned Int8 can be loaded through texture memory");
  VTKM_TEST_ASSERT(ct::WillUseTexture == 0, "Custom Types can't be loaded through texture memory");
}

void TestVecTextureLoad()
{
  using namespace vtkm::exec::cuda::internal;
  using ui32_3 = load_through_texture<vtkm::Vec<vtkm::UInt32, 3>>;
  using f32_3 = load_through_texture<vtkm::Vec<vtkm::Float32, 3>>;
  using ui8_3 = load_through_texture<vtkm::Vec<vtkm::UInt8, 3>>;
  using f64_3 = load_through_texture<vtkm::Vec<vtkm::Float64, 3>>;

  using ui32_4 = load_through_texture<vtkm::Vec<vtkm::UInt32, 4>>;
  using f32_4 = load_through_texture<vtkm::Vec<vtkm::Float32, 4>>;
  using ui8_4 = load_through_texture<vtkm::Vec<vtkm::UInt8, 4>>;
  using f64_4 = load_through_texture<vtkm::Vec<vtkm::Float64, 4>>;

  using ct_3 = load_through_texture<vtkm::Vec<customType, 3>>;
  using ct_4 = load_through_texture<vtkm::Vec<customType, 4>>;

  VTKM_TEST_ASSERT(ui32_3::WillUseTexture == 1, "Can be loaded through texture loads");
  VTKM_TEST_ASSERT(f32_3::WillUseTexture == 1, "Can be loaded through texture loads");
  VTKM_TEST_ASSERT(ui8_3::WillUseTexture == 1, "Can be loaded through texture loads");
  VTKM_TEST_ASSERT(f64_3::WillUseTexture == 1, "Can be loaded through texture loads");

  VTKM_TEST_ASSERT(ui32_4::WillUseTexture == 1, "Can be loaded through texture loads");
  VTKM_TEST_ASSERT(f32_4::WillUseTexture == 1, "Can be loaded through texture loads");
  VTKM_TEST_ASSERT(ui8_4::WillUseTexture == 1, "Can be loaded through texture loads");
  VTKM_TEST_ASSERT(f64_4::WillUseTexture == 1, "Can be loaded through texture loads");

  VTKM_TEST_ASSERT(ct_4::WillUseTexture == 0, "Can't be loaded through texture loads");
  VTKM_TEST_ASSERT(ct_4::WillUseTexture == 0, "Can't be loaded through texture loads");
}

} // namespace

void TestTextureMemorySupport()
{
  TestScalarTextureLoad();
  TestVecTextureLoad();
}

int UnitTestTextureMemorySupport(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestTextureMemorySupport, argc, argv);
}

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

#include <vtkm/cont/testing/Testing.h>
#include <vtkm/exec/cuda/internal/ArrayPortalFromThrust.h>

namespace {

struct customType { };

void TestScalarTextureLoad()
{
  using namespace  vtkm::exec::cuda::internal;
  typedef load_through_texture< vtkm::Float32 > f;
  typedef load_through_texture< vtkm::Int32 > i;
  typedef load_through_texture< vtkm::UInt8 > ui;

  typedef load_through_texture< customType > ct;

  VTKM_TEST_ASSERT( f::WillUseTexture == 1, "Float32 can be loaded through texture memory" );
  VTKM_TEST_ASSERT( i::WillUseTexture == 1, "Int32 can be loaded through texture memory" );
  VTKM_TEST_ASSERT( ui::WillUseTexture == 1, "Unsigned Int8 can be loaded through texture memory" );
  VTKM_TEST_ASSERT( ct::WillUseTexture == 0, "Custom Types can't be loaded through texture memory" );

}

void TestVecTextureLoad()
{
  using namespace
  vtkm::exec::cuda::internal;
  typedef load_through_texture< vtkm::Vec<vtkm::UInt32,3> > ui32_3;
  typedef load_through_texture< vtkm::Vec<vtkm::Float32,3> > f32_3;
  typedef load_through_texture< vtkm::Vec<vtkm::UInt8,3>  > ui8_3;
  typedef load_through_texture< vtkm::Vec<vtkm::Float64,3> > f64_3;

  typedef load_through_texture< vtkm::Vec<vtkm::UInt32,4> > ui32_4;
  typedef load_through_texture< vtkm::Vec<vtkm::Float32,4> > f32_4;
  typedef load_through_texture< vtkm::Vec<vtkm::UInt8,4>  > ui8_4;
  typedef load_through_texture< vtkm::Vec<vtkm::Float64,4> > f64_4;

  typedef load_through_texture< vtkm::Vec<customType, 3> > ct_3;
  typedef load_through_texture< vtkm::Vec<customType, 4> > ct_4;


  VTKM_TEST_ASSERT( ui32_3::WillUseTexture == 1, "Can be loaded through texture loads");
  VTKM_TEST_ASSERT( f32_3::WillUseTexture == 1, "Can be loaded through texture loads");
  VTKM_TEST_ASSERT( ui8_3::WillUseTexture == 1, "Can be loaded through texture loads");
  VTKM_TEST_ASSERT( f64_3::WillUseTexture == 1, "Can be loaded through texture loads");

  VTKM_TEST_ASSERT( ui32_4::WillUseTexture == 1, "Can be loaded through texture loads");
  VTKM_TEST_ASSERT( f32_4::WillUseTexture == 1, "Can be loaded through texture loads");
  VTKM_TEST_ASSERT( ui8_4::WillUseTexture == 1, "Can be loaded through texture loads");
  VTKM_TEST_ASSERT( f64_4::WillUseTexture == 1, "Can be loaded through texture loads");

  VTKM_TEST_ASSERT( ct_4::WillUseTexture == 0, "Can't be loaded through texture loads");
  VTKM_TEST_ASSERT( ct_4::WillUseTexture == 0, "Can't be loaded through texture loads");
}


} // namespace

void TestTextureMemorySupport()
{
  TestScalarTextureLoad();
  TestVecTextureLoad();
}

int UnitTestTextureMemorySupport(int, char *[])
{
  return vtkm::cont::testing::Testing::Run( TestTextureMemorySupport );
}

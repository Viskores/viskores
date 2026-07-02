//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/cont/ErrorBadValue.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/interop/anari/ANARIActor.h>
#include <viskores/interop/anari/ANARIMapperGlyphs.h>
#include <viskores/interop/anari/ANARIMapperPoints.h>
#include <viskores/interop/anari/ANARIMapperTriangles.h>
#include <viskores/interop/anari/ANARIMapperVolume.h>
#include <viskores/interop/anari/ANARIScene.h>
#include <viskores/source/Tangle.h>

#include <type_traits>
#include <utility>

#include "ANARITestCommon.h"

namespace
{

template <typename MapperType>
constexpr bool IsMoveOnlyMapper = std::is_move_constructible<MapperType>::value &&
  !std::is_copy_constructible<MapperType>::value && !std::is_copy_assignable<MapperType>::value;

static_assert(IsMoveOnlyMapper<viskores::interop::anari::ANARIMapper>);
static_assert(IsMoveOnlyMapper<viskores::interop::anari::ANARIMapperGlyphs>);
static_assert(IsMoveOnlyMapper<viskores::interop::anari::ANARIMapperPoints>);
static_assert(IsMoveOnlyMapper<viskores::interop::anari::ANARIMapperTriangles>);
static_assert(IsMoveOnlyMapper<viskores::interop::anari::ANARIMapperVolume>);

void TestActorValueSemantics()
{
  viskores::interop::anari::ANARIActor original;
  auto copy = original;

  copy.SetPrimaryFieldIndex(1);

  VISKORES_TEST_ASSERT(original.GetPrimaryFieldIndex() == 0,
                       "Changing an actor copy changed the original actor.");
  VISKORES_TEST_ASSERT(copy.GetPrimaryFieldIndex() == 1,
                       "The copied actor did not retain its own primary field.");
}

void TestSceneMapperOwnership()
{
  auto loadedDevice = loadANARIDevice();
  auto device = loadedDevice.GetDevice();
  viskores::interop::anari::ANARIScene scene(device);

  auto& volume =
    scene.AddMapper(viskores::interop::anari::ANARIMapperVolume(device, {}, "owned-volume"));
  auto* volumeAddress = &volume;

  scene.AddMapper(viskores::interop::anari::ANARIMapperGlyphs(device, {}, "owned-glyphs"));

  VISKORES_TEST_ASSERT(scene.GetNumberOfMappers() == 2, "The scene did not retain both mappers.");
  VISKORES_TEST_ASSERT(&scene.GetMapper("owned-volume") == volumeAddress,
                       "Adding another mapper invalidated a stable mapper reference.");
}

void TestActorReplacementAndMapperDestruction()
{
  auto loadedDevice = loadANARIDevice();
  auto device = loadedDevice.GetDevice();

  viskores::source::Tangle source;
  source.SetPointDimensions({ 4, 4, 4 });
  auto dataSet = source.Execute();
  viskores::interop::anari::ANARIActor actor(
    dataSet.GetCellSet(), dataSet.GetCoordinateSystem(), dataSet.GetField("tangle"));

  viskores::interop::anari::ANARIScene scene(device);
  viskores::interop::anari::ANARIMapperVolume mapper(device, actor, "replace-volume");
  auto originalVolume = mapper.GetANARIVolume();
  auto& volume = scene.AddMapper(std::move(mapper));
  VISKORES_TEST_ASSERT(volume.GetANARIVolume() == originalVolume,
                       "Moving the mapper into the scene replaced its ANARI volume.");
  VISKORES_TEST_ASSERT(scene.GetANARIWorld() != nullptr, "The scene did not create a world.");
  VISKORES_TEST_ASSERT(volume.GetANARIVolume() != nullptr, "The mapper did not create a volume.");

  actor.SetPrimaryFieldIndex(1);
  VISKORES_TEST_ASSERT(volume.GetActor().GetPrimaryFieldIndex() == 0,
                       "Mutating the source actor changed a materialized mapper.");

  auto& replacement = scene.AddMapper(
    viskores::interop::anari::ANARIMapperVolume(device, volume.GetActor(), "replace-volume"));
  VISKORES_TEST_ASSERT(scene.GetNumberOfMappers() == 1,
                       "Replacing a mapper changed the mapper count.");
  VISKORES_TEST_ASSERT(replacement.GetANARIVolume() != nullptr,
                       "The replacement mapper did not create a volume.");

  for (viskores::IdComponent updateIndex = 0; updateIndex < 3; ++updateIndex)
  {
    viskores::interop::anari::ANARIActor replacementActor(
      dataSet.GetCellSet(), dataSet.GetCoordinateSystem(), dataSet.GetField("tangle"));
    replacement.SetActor(replacementActor);
    VISKORES_TEST_ASSERT(replacement.GetANARIVolume() != nullptr,
                         "Updating the replacement actor invalidated the volume.");
  }

  scene.RemoveMapper("replace-volume");
  VISKORES_TEST_ASSERT(scene.GetNumberOfMappers() == 0, "The scene did not destroy the mapper.");
}

void TestDeviceMismatchRejected()
{
  auto sceneLoadedDevice = loadANARIDevice();
  auto mapperLoadedDevice = loadANARIDevice();
  auto sceneDevice = sceneLoadedDevice.GetDevice();
  auto mapperDevice = mapperLoadedDevice.GetDevice();
  VISKORES_TEST_ASSERT(sceneDevice != mapperDevice,
                       "The test requires two distinct ANARI devices.");

  viskores::interop::anari::ANARIScene scene(sceneDevice);
  bool mismatchRejected = false;
  try
  {
    scene.AddMapper(
      viskores::interop::anari::ANARIMapperPoints(mapperDevice, {}, "foreign-device"));
  }
  catch (const viskores::cont::ErrorBadValue&)
  {
    mismatchRejected = true;
  }

  VISKORES_TEST_ASSERT(mismatchRejected, "The scene accepted a mapper from another device.");
  VISKORES_TEST_ASSERT(scene.GetNumberOfMappers() == 0,
                       "A rejected mapper was added to the scene.");
}

void RunTests()
{
  TestActorValueSemantics();
  TestSceneMapperOwnership();
  TestActorReplacementAndMapperDestruction();
  TestDeviceMismatchRejected();
}

} // namespace

int UnitTestANARIOwnership(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(RunTests, argc, argv);
}

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_interop_anari_testing_ANARITestCommon_h
#define vtk_m_interop_anari_testing_ANARITestCommon_h

// vtk-m
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/interop/anari/ANARIMapper.h>
#include <vtkm/rendering/testing/Testing.h>
#include <vtkm/testing/Testing.h>

namespace
{

static void StatusFunc(const void* userData,
                       ANARIDevice /*device*/,
                       ANARIObject source,
                       ANARIDataType /*sourceType*/,
                       ANARIStatusSeverity severity,
                       ANARIStatusCode /*code*/,
                       const char* message)
{
  bool verbose = *(bool*)userData;
  if (!verbose)
    return;

  if (severity == ANARI_SEVERITY_FATAL_ERROR)
  {
    fprintf(stderr, "[FATAL][%p] %s\n", source, message);
  }
  else if (severity == ANARI_SEVERITY_ERROR)
  {
    fprintf(stderr, "[ERROR][%p] %s\n", source, message);
  }
  else if (severity == ANARI_SEVERITY_WARNING)
  {
    fprintf(stderr, "[WARN ][%p] %s\n", source, message);
  }
  else if (severity == ANARI_SEVERITY_PERFORMANCE_WARNING)
  {
    fprintf(stderr, "[PERF ][%p] %s\n", source, message);
  }
  else if (severity == ANARI_SEVERITY_INFO)
  {
    fprintf(stderr, "[INFO ][%p] %s\n", source, message);
  }
  else if (severity == ANARI_SEVERITY_DEBUG)
  {
    fprintf(stderr, "[DEBUG][%p] %s\n", source, message);
  }
}

static void setColorMap(anari_cpp::Device d, vtkm::interop::anari::ANARIMapper& mapper)
{
  auto colorArray = anari_cpp::newArray1D(d, ANARI_FLOAT32_VEC3, 3);
  auto* colors = anari_cpp::map<vtkm::Vec3f_32>(d, colorArray);
  colors[0] = vtkm::Vec3f_32(0.f, 0.f, 1.f);
  colors[1] = vtkm::Vec3f_32(0.f, 1.f, 0.f);
  colors[2] = vtkm::Vec3f_32(1.f, 0.f, 0.f);
  anari_cpp::unmap(d, colorArray);

  auto opacityArray = anari_cpp::newArray1D(d, ANARI_FLOAT32, 2);
  auto* opacities = anari_cpp::map<float>(d, opacityArray);
  opacities[0] = 0.f;
  opacities[1] = 1.f;
  anari_cpp::unmap(d, opacityArray);

  mapper.SetANARIColorMap(colorArray, opacityArray, true);
  mapper.SetANARIColorMapValueRange(vtkm::Vec2f_32(0.f, 10.f));
  mapper.SetANARIColorMapOpacityScale(0.5f);
}

static anari_cpp::Device loadANARIDevice()
{
  vtkm::testing::FloatingPointExceptionTrapDisable();
  auto* libraryName = std::getenv("VTKM_TEST_ANARI_LIBRARY");
  static bool verbose = std::getenv("VTKM_TEST_ANARI_VERBOSE") != nullptr;
  auto lib = anari_cpp::loadLibrary(libraryName ? libraryName : "helide", StatusFunc, &verbose);
  auto d = anari_cpp::newDevice(lib, "default");
  anari_cpp::unloadLibrary(lib);
  return d;
}

static void renderTestANARIImage(anari_cpp::Device d,
                                 anari_cpp::World w,
                                 vtkm::Vec3f_32 cam_pos,
                                 vtkm::Vec3f_32 cam_dir,
                                 vtkm::Vec3f_32 cam_up,
                                 const std::string& imgName,
                                 vtkm::Vec2ui_32 imgSize = vtkm::Vec2ui_32(1024, 768))
{
  auto renderer = anari_cpp::newObject<anari_cpp::Renderer>(d, "default");
  anari_cpp::setParameter(d, renderer, "background", vtkm::Vec4f_32(0.3f, 0.3f, 0.3f, 1.f));
  anari_cpp::setParameter(d, renderer, "pixelSamples", 64);
  anari_cpp::commitParameters(d, renderer);

  auto camera = anari_cpp::newObject<anari_cpp::Camera>(d, "perspective");
  anari_cpp::setParameter(d, camera, "aspect", imgSize[0] / float(imgSize[1]));
  anari_cpp::setParameter(d, camera, "position", cam_pos);
  anari_cpp::setParameter(d, camera, "direction", cam_dir);
  anari_cpp::setParameter(d, camera, "up", cam_up);
  anari_cpp::commitParameters(d, camera);

  auto frame = anari_cpp::newObject<anari_cpp::Frame>(d);
  anari_cpp::setParameter(d, frame, "size", imgSize);
  anari_cpp::setParameter(d, frame, "channel.color", ANARI_FLOAT32_VEC4);
  anari_cpp::setParameter(d, frame, "world", w);
  anari_cpp::setParameter(d, frame, "camera", camera);
  anari_cpp::setParameter(d, frame, "renderer", renderer);
  anari_cpp::commitParameters(d, frame);

  anari_cpp::release(d, camera);
  anari_cpp::release(d, renderer);

  anari_cpp::render(d, frame);
  anari_cpp::wait(d, frame);

  const auto fb = anari_cpp::map<vtkm::Vec4f_32>(d, frame, "channel.color");

  vtkm::cont::DataSetBuilderUniform builder;
  vtkm::cont::DataSet image = builder.Create(vtkm::Id2(fb.width, fb.height));

  // NOTE: We are only copying the pixel data into a VTK-m array for the
  //       purpose of using VTK-m's image comparison test code. Applications
  //       would not normally do this and instead just use the pixel data
  //       directly, such as displaying it in an interactive window.

  vtkm::cont::ArrayHandle<vtkm::Vec4f_32> colorArray =
    vtkm::cont::make_ArrayHandle(fb.data, fb.width * fb.height, vtkm::CopyFlag::On);

  anari_cpp::unmap(d, frame, "channel.color");
  anari_cpp::release(d, frame);

  image.AddPointField("color", colorArray);

  VTKM_TEST_ASSERT(test_equal_images(image, imgName));
}

} // namespace

#endif

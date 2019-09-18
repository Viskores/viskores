//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_testing_TestingFancyArrayHandles_h
#define vtk_m_cont_testing_TestingFancyArrayHandles_h

#include <vtkm/VecTraits.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCartesianProduct.h>
#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/ArrayHandleCompositeVector.h>
#include <vtkm/cont/ArrayHandleConcatenate.h>
#include <vtkm/cont/ArrayHandleCounting.h>

#include <vtkm/interop/TransferToOpenGL.h>

#include <vtkm/cont/testing/Testing.h>

#include <vector>

namespace vtkm
{
namespace interop
{
namespace testing
{

namespace
{
template <typename T>
vtkm::cont::ArrayHandle<T> makeArray(vtkm::Id length, T)
{
  vtkm::cont::ArrayHandle<T> data;
  data.Allocate(length);

  auto portal = data.GetPortalControl();
  for (vtkm::Id i = 0; i != data.GetNumberOfValues(); ++i)
  {
    portal.Set(i, TestValue(i, T()));
  }
  return data;
}

//bring the data back from openGL and into a std vector. Will bind the
//passed in handle to the default buffer type for the type T
template <typename T>
std::vector<T> CopyGLBuffer(GLuint& handle, T t)
{
  //get the type we used for this buffer.
  GLenum type = vtkm::interop::internal::BufferTypePicker(t);

  //bind the buffer to the guessed buffer type, this way
  //we can call CopyGLBuffer no matter what it the active buffer
  glBindBuffer(type, handle);

  //get the size of the buffer
  int bytesInBuffer = 0;
  glGetBufferParameteriv(type, GL_BUFFER_SIZE, &bytesInBuffer);
  const std::size_t size = (static_cast<std::size_t>(bytesInBuffer) / sizeof(T));

  //get the buffer contents and place it into a vector
  std::vector<T> data;
  data.resize(size);
  glGetBufferSubData(type, 0, bytesInBuffer, &data[0]);

  return data;
}

template <typename T, typename U>
void validate(vtkm::cont::ArrayHandle<T, U> handle, vtkm::interop::BufferState& state)
{
  GLboolean is_buffer;
  is_buffer = glIsBuffer(*state.GetHandle());
  VTKM_TEST_ASSERT(is_buffer == GL_TRUE, "OpenGL buffer not filled");
  std::vector<T> returnedValues = CopyGLBuffer(*state.GetHandle(), T());

  vtkm::Int64 retSize = static_cast<vtkm::Int64>(returnedValues.size());

  //since BufferState allows for re-use of a GL buffer that is slightly
  //larger than the current array size, we should only check that the
  //buffer is not smaller than the array.
  //This GL buffer size is done to improve performance when transferring
  //arrays to GL whose size changes on a per frame basis
  VTKM_TEST_ASSERT(retSize >= handle.GetNumberOfValues(), "OpenGL buffer not large enough size");

  //validate that retsize matches the bufferstate capacity which returns
  //the amount of total GL buffer space, not the size we are using
  const vtkm::Int64 capacity = (state.GetCapacity() / static_cast<vtkm::Int64>(sizeof(T)));
  VTKM_TEST_ASSERT(retSize == capacity, "OpenGL buffer size doesn't match BufferState");

  //validate that the capacity and the SMPTransferResource have the same size
  vtkm::interop::internal::SMPTransferResource* resource =
    dynamic_cast<vtkm::interop::internal::SMPTransferResource*>(state.GetResource());

  VTKM_TEST_ASSERT(resource->Size == capacity,
                   "buffer state internal resource doesn't match BufferState capacity");

  auto portal = handle.GetPortalConstControl();
  auto iter = returnedValues.cbegin();
  for (vtkm::Id i = 0; i != handle.GetNumberOfValues(); ++i, ++iter)
  {
    VTKM_TEST_ASSERT(portal.Get(i) == *iter, "incorrect value returned from OpenGL buffer");
  }
}

void test_ArrayHandleCartesianProduct()
{
  vtkm::cont::ArrayHandle<vtkm::Float32> x = makeArray(10, vtkm::Float32());
  vtkm::cont::ArrayHandle<vtkm::Float32> y = makeArray(10, vtkm::Float32());
  vtkm::cont::ArrayHandle<vtkm::Float32> z = makeArray(10, vtkm::Float32());

  auto cartesian = vtkm::cont::make_ArrayHandleCartesianProduct(x, y, z);

  vtkm::interop::BufferState state;
  vtkm::interop::TransferToOpenGL(cartesian, state);
  validate(cartesian, state);
  vtkm::interop::TransferToOpenGL(cartesian, state); //make sure we can do multiple trasfers
  validate(cartesian, state);

  //resize up
  x = makeArray(100, vtkm::Float32());
  y = makeArray(100, vtkm::Float32());
  z = makeArray(100, vtkm::Float32());
  cartesian = vtkm::cont::make_ArrayHandleCartesianProduct(x, y, z);
  vtkm::interop::TransferToOpenGL(cartesian, state);
  validate(cartesian, state);

  //resize down but instead capacity threshold
  x = makeArray(99, vtkm::Float32());
  y = makeArray(99, vtkm::Float32());
  z = makeArray(99, vtkm::Float32());
  cartesian = vtkm::cont::make_ArrayHandleCartesianProduct(x, y, z);
  vtkm::interop::TransferToOpenGL(cartesian, state);
  validate(cartesian, state);

  //resize down
  x = makeArray(10, vtkm::Float32());
  y = makeArray(10, vtkm::Float32());
  z = makeArray(10, vtkm::Float32());
  cartesian = vtkm::cont::make_ArrayHandleCartesianProduct(x, y, z);
  vtkm::interop::TransferToOpenGL(cartesian, state);
  validate(cartesian, state);
}

void test_ArrayHandleCast()
{
  vtkm::cont::ArrayHandle<vtkm::Vec3f_64> handle = makeArray(100000, vtkm::Vec3f_64());
  auto castArray = vtkm::cont::make_ArrayHandleCast(handle, vtkm::Vec3f_32());

  vtkm::interop::BufferState state;
  vtkm::interop::TransferToOpenGL(castArray, state);
  validate(castArray, state);
  vtkm::interop::TransferToOpenGL(castArray, state); //make sure we can do multiple trasfers
  validate(castArray, state);

  //resize down
  handle = makeArray(1000, vtkm::Vec3f_64());
  castArray = vtkm::cont::make_ArrayHandleCast(handle, vtkm::Vec3f_32());
  vtkm::interop::TransferToOpenGL(castArray, state);
  validate(castArray, state);
}

void test_ArrayHandleCounting()
{
  auto counting1 = vtkm::cont::make_ArrayHandleCounting(vtkm::Id(0), vtkm::Id(1), vtkm::Id(10000));
  auto counting2 = vtkm::cont::make_ArrayHandleCounting(vtkm::Id(0), vtkm::Id(4), vtkm::Id(10000));
  auto counting3 = vtkm::cont::make_ArrayHandleCounting(vtkm::Id(0), vtkm::Id(0), vtkm::Id(10000));

  //use the same state with different counting handles
  vtkm::interop::BufferState state;
  vtkm::interop::TransferToOpenGL(counting1, state);
  validate(counting1, state);
  vtkm::interop::TransferToOpenGL(counting2, state);
  validate(counting2, state);
  vtkm::interop::TransferToOpenGL(counting3, state);
  validate(counting3, state);
}

void test_ArrayHandleConcatenate()
{
  vtkm::cont::ArrayHandle<vtkm::Float32> a = makeArray(5000, vtkm::Float32());
  vtkm::cont::ArrayHandle<vtkm::Float32> b = makeArray(25000, vtkm::Float32());

  auto concatenate = vtkm::cont::make_ArrayHandleConcatenate(a, b);

  vtkm::interop::BufferState state;
  vtkm::interop::TransferToOpenGL(concatenate, state);
  validate(concatenate, state);
  vtkm::interop::TransferToOpenGL(concatenate, state); //make sure we can do multiple trasfers
  validate(concatenate, state);

  //resize down
  b = makeArray(1000, vtkm::Float32());
  concatenate = vtkm::cont::make_ArrayHandleConcatenate(a, b);
  vtkm::interop::TransferToOpenGL(concatenate, state);
  validate(concatenate, state);
}

void test_ArrayHandleCompositeVector()
{
  vtkm::cont::ArrayHandle<vtkm::Float32> x = makeArray(10000, vtkm::Float32());
  vtkm::cont::ArrayHandle<vtkm::Float32> y = makeArray(10000, vtkm::Float32());
  vtkm::cont::ArrayHandle<vtkm::Float32> z = makeArray(10000, vtkm::Float32());

  auto composite = vtkm::cont::make_ArrayHandleCompositeVector(x, 0, y, 0, z, 0);

  vtkm::interop::BufferState state;
  vtkm::interop::TransferToOpenGL(composite, state);
  validate(composite, state);
}
}

/// This class has a single static member, Run, that tests that all Fancy Array
/// Handles work with vtkm::interop::TransferToOpenGL
///
struct TestingTransferFancyHandles
{
public:
  /// Run a suite of tests to check to see if a vtkm::interop::TransferToOpenGL
  /// properly supports all the fancy array handles that vtkm supports. Returns an
  /// error code that can be returned from the main function of a test.
  ///
  struct TestAll
  {
    void operator()() const
    {
      std::cout << "Doing FancyArrayHandle TransferToOpenGL Tests" << std::endl;

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleCartesianProduct" << std::endl;
      test_ArrayHandleCartesianProduct();

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleCast" << std::endl;
      test_ArrayHandleCast();

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleCounting" << std::endl;
      test_ArrayHandleCounting();

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleConcatenate" << std::endl;
      test_ArrayHandleConcatenate();

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleConcatenate" << std::endl;
      test_ArrayHandleCompositeVector();
    }
  };

  static int Run(int argc, char* argv[])
  {
    return vtkm::cont::testing::Testing::Run(TestAll(), argc, argv);
  }
};
}
}
} // namespace vtkm::cont::testing

#endif //vtk_m_cont_testing_TestingFancyArrayHandles_h

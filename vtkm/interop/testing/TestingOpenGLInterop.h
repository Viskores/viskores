//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_interop_testing_TestingOpenGLInterop_h
#define vtk_m_interop_testing_TestingOpenGLInterop_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/Magnitude.h>

#include <vtkm/interop/TransferToOpenGL.h>

#include <vtkm/cont/testing/Testing.h>
// #include <vtkm/cont/testing/TestingGridGenerator.h>

#include <algorithm>
#include <iterator>
#include <random>
#include <vector>

namespace vtkm
{
namespace interop
{
namespace testing
{

/// This class has a single static member, Run, that tests the templated
/// DeviceAdapter for support for opengl interop.
///
template <class DeviceAdapterTag, class StorageTag = VTKM_DEFAULT_STORAGE_TAG>
struct TestingOpenGLInterop
{
private:
  //fill the array with a collection of values and return it wrapped in
  //an vtkm array handle
  template <typename T>
  static vtkm::cont::ArrayHandle<T, StorageTag> FillArray(std::vector<T>& data, std::size_t length)
  {
    using iterator = typename std::vector<T>::iterator;
    //make sure the data array is exactly the right length
    data.clear();
    data.resize(length);
    vtkm::Id pos = 0;
    for (iterator i = data.begin(); i != data.end(); ++i, ++pos)
    {
      *i = TestValue(pos, T());
    }

    std::random_device rng;
    std::mt19937 urng(rng());
    std::shuffle(data.begin(), data.end(), urng);
    return vtkm::cont::make_ArrayHandle(data);
  }

  //Transfer the data in a vtkm ArrayHandle to open gl while making sure
  //we don't throw any errors
  template <typename ArrayHandleType>
  static void SafelyTransferArray(ArrayHandleType array, GLuint& handle)
  {
    try
    {
      vtkm::interop::BufferState state(handle);
      vtkm::interop::TransferToOpenGL(array, state, DeviceAdapterTag());
    }
    catch (vtkm::cont::ErrorBadAllocation& error)
    {
      std::cout << error.GetMessage() << std::endl;
      VTKM_TEST_ASSERT(true == false,
                       "Got an unexpected Out Of Memory error transferring to openGL");
    }
    catch (vtkm::cont::ErrorBadValue& bvError)
    {
      std::cout << bvError.GetMessage() << std::endl;
      VTKM_TEST_ASSERT(true == false, "Got an unexpected Bad Value error transferring to openGL");
    }

    // Test device adapter deduction:
    try
    {
      vtkm::interop::BufferState state(handle);
      vtkm::interop::TransferToOpenGL(array, state);
    }
    catch (vtkm::cont::ErrorBadAllocation& error)
    {
      std::cout << error.GetMessage() << std::endl;
      VTKM_TEST_ASSERT(true == false,
                       "Got an unexpected Out Of Memory error transferring to openGL");
    }
    catch (vtkm::cont::ErrorBadValue& bvError)
    {
      std::cout << bvError.GetMessage() << std::endl;
      VTKM_TEST_ASSERT(true == false, "Got an unexpected Bad Value error transferring to openGL");
    }
  }

  template <typename ArrayHandleType>
  static void SafelyTransferArray(ArrayHandleType array, GLuint& handle, GLenum type)
  {
    try
    {
      vtkm::interop::BufferState state(handle, type);
      vtkm::interop::TransferToOpenGL(array, state, DeviceAdapterTag());
    }
    catch (vtkm::cont::ErrorBadAllocation& error)
    {
      std::cout << error.GetMessage() << std::endl;
      VTKM_TEST_ASSERT(true == false,
                       "Got an unexpected Out Of Memory error transferring to openGL");
    }
    catch (vtkm::cont::ErrorBadValue& bvError)
    {
      std::cout << bvError.GetMessage() << std::endl;
      VTKM_TEST_ASSERT(true == false, "Got an unexpected Bad Value error transferring to openGL");
    }

    // Test device adapter deduction
    try
    {
      vtkm::interop::BufferState state(handle, type);
      vtkm::interop::TransferToOpenGL(array, state);
    }
    catch (vtkm::cont::ErrorBadAllocation& error)
    {
      std::cout << error.GetMessage() << std::endl;
      VTKM_TEST_ASSERT(true == false,
                       "Got an unexpected Out Of Memory error transferring to openGL");
    }
    catch (vtkm::cont::ErrorBadValue& bvError)
    {
      std::cout << bvError.GetMessage() << std::endl;
      VTKM_TEST_ASSERT(true == false, "Got an unexpected Bad Value error transferring to openGL");
    }
  }

  //bring the data back from openGL and into a std vector. Will bind the
  //passed in handle to the default buffer type for the type T
  template <typename T>
  static std::vector<T> CopyGLBuffer(GLuint& handle, T t)
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

  struct TransferFunctor
  {
    template <typename T>
    void operator()(const T t) const
    {
      const std::size_t Size = 10;
      GLuint GLHandle;
      //verify that T is able to be transfer to openGL.
      //than pull down the results from the array buffer and verify
      //that they match the handles contents
      std::vector<T> tempData;
      vtkm::cont::ArrayHandle<T, StorageTag> temp = FillArray(tempData, Size);

      //verify that the signature that doesn't have type works
      SafelyTransferArray(temp, GLHandle);

      GLboolean is_buffer;
      is_buffer = glIsBuffer(GLHandle);
      VTKM_TEST_ASSERT(is_buffer == GL_TRUE, "OpenGL buffer not filled");

      std::vector<T> returnedValues = CopyGLBuffer(GLHandle, t);

      //verify the results match what is in the array handle
      temp.SyncControlArray();
      T* expectedValues = temp.GetStorage().GetArray();

      for (std::size_t i = 0; i < Size; ++i)
      {
        VTKM_TEST_ASSERT(test_equal(*(expectedValues + i), returnedValues[i]),
                         "Array Handle failed to transfer properly");
      }

      temp.ReleaseResources();
      temp = FillArray(tempData, Size * 2);
      GLenum type = vtkm::interop::internal::BufferTypePicker(t);
      SafelyTransferArray(temp, GLHandle, type);
      is_buffer = glIsBuffer(GLHandle);
      VTKM_TEST_ASSERT(is_buffer == GL_TRUE, "OpenGL buffer not filled");
      returnedValues = CopyGLBuffer(GLHandle, t);
      //verify the results match what is in the array handle
      temp.SyncControlArray();
      expectedValues = temp.GetStorage().GetArray();

      for (std::size_t i = 0; i < Size * 2; ++i)
      {
        VTKM_TEST_ASSERT(test_equal(*(expectedValues + i), returnedValues[i]),
                         "Array Handle failed to transfer properly");
      }

      //verify this work for a constant value array handle
      T constantValue = TestValue(2, T()); //verified by die roll
      vtkm::cont::ArrayHandleConstant<T> constant(constantValue, static_cast<vtkm::Id>(Size));
      SafelyTransferArray(constant, GLHandle);
      is_buffer = glIsBuffer(GLHandle);
      VTKM_TEST_ASSERT(is_buffer == GL_TRUE, "OpenGL buffer not filled");
      returnedValues = CopyGLBuffer(GLHandle, constantValue);
      for (std::size_t i = 0; i < Size; ++i)
      {
        VTKM_TEST_ASSERT(test_equal(returnedValues[i], constantValue),
                         "Constant value array failed to transfer properly");
      }
    }
  };

  // struct TransferGridFunctor
  // {
  //   GLuint CoordGLHandle;
  //   GLuint MagnitudeGLHandle;

  //   template <typename GridType>
  //   void operator()(const GridType)
  //   {
  //   //verify we are able to be transfer both coordinates and indices to openGL.
  //   //than pull down the results from the array buffer and verify
  //   //that they match the handles contents
  //   vtkm::cont::testing::TestGrid<GridType,
  //                                StorageTag,
  //                                DeviceAdapterTag> grid(64);

  //   vtkm::cont::ArrayHandle<vtkm::FloatDefault,
  //                          StorageTag,
  //                          DeviceAdapterTag> magnitudeHandle;

  //   vtkm::cont::DispatcherMapField< vtkm::worklet::Magnitude,
  //                                  DeviceAdapterTag> dispatcher;
  //   dispatcher.Invoke(grid->GetPointCoordinates(), magnitudeHandle);

  //   //transfer to openGL 3 handles and catch any errors
  //   //
  //   SafelyTransferArray(grid->GetPointCoordinates(),this->CoordGLHandle);
  //   SafelyTransferArray(magnitudeHandle,this->MagnitudeGLHandle);

  //   //verify all 3 handles are actually handles
  //   bool  is_buffer = glIsBuffer(this->CoordGLHandle);
  //   VTKM_TEST_ASSERT(is_buffer==GL_TRUE,
  //                   "Coordinates OpenGL buffer not filled");

  //   is_buffer = glIsBuffer(this->MagnitudeGLHandle);
  //   VTKM_TEST_ASSERT(is_buffer==GL_TRUE,
  //                   "Magnitude OpenGL buffer not filled");

  //   //now that everything is openGL we have one task left.
  //   //transfer everything back to the host and compare it to the
  //   //computed values.
  //   std::vector<vtkm::Vec<vtkm::FloatDefault,3>> GLReturnedCoords = CopyGLBuffer(
  //                                       this->CoordGLHandle, vtkm::Vec<vtkm::FloatDefault,3>());
  //   std::vector<vtkm::FloatDefault> GLReturneMags = CopyGLBuffer(
  //                                       this->MagnitudeGLHandle,vtkm::FloatDefault());

  //   for (vtkm::Id pointIndex = 0;
  //        pointIndex < grid->GetNumberOfPoints();
  //        pointIndex++)
  //     {
  //     vtkm::Vec<vtkm::FloatDefault,3> pointCoordinateExpected = grid.GetPointCoordinates(
  //                                                                   pointIndex);
  //     vtkm::Vec<vtkm::FloatDefault,3> pointCoordinatesReturned =  GLReturnedCoords[pointIndex];
  //     VTKM_TEST_ASSERT(test_equal(pointCoordinateExpected,
  //                                pointCoordinatesReturned),
  //                     "Got bad coordinate from OpenGL buffer.");

  //     vtkm::FloatDefault magnitudeValue = GLReturneMags[pointIndex];
  //     vtkm::FloatDefault magnitudeExpected =
  //         sqrt(vtkm::Dot(pointCoordinateExpected, pointCoordinateExpected));
  //     VTKM_TEST_ASSERT(test_equal(magnitudeValue, magnitudeExpected),
  //                     "Got bad magnitude from OpenGL buffer.");
  //     }
  //   }
  // };

public:
  VTKM_CONT static int Run(int argc, char* argv[])
  {
    std::cout << "TestingOpenGLInterop Run() " << std::endl;

    //verify that we can transfer basic arrays and constant value arrays to opengl
    vtkm::testing::Testing::TryTypes(TransferFunctor(), argc, argv);

    //verify that openGL interop works with all grid types in that we can
    //transfer coordinates / verts and properties to openGL
    // vtkm::cont::testing::GridTesting::TryAllGridTypes(
    //                              TransferGridFunctor(),
    //                              vtkm::testing::Testing::CellCheckAlwaysTrue(),
    //                              StorageTag(),
    //                              DeviceAdapterTag() );

    return 0;
  }
};
}
}
}

#endif //vtk_m_interop_testing_TestingOpenGLInterop_h

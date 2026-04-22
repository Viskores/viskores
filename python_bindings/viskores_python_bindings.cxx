//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include <Python.h>
#include <structmember.h>

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/arrayobject.h>

#include <viskores/Types.h>
#include <viskores/Range.h>
#include <viskores/cont/ArrayHandleBasic.h>
#include <viskores/cont/CellSetStructured.h>
#include <viskores/cont/ColorTable.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/Initialize.h>
#include <viskores/cont/PartitionedDataSet.h>
#include <viskores/filter/MapFieldPermutation.h>
#include <viskores/filter/contour/Contour.h>
#include <viskores/filter/field_conversion/CellAverage.h>
#include <viskores/filter/field_conversion/PointAverage.h>
#include <viskores/filter/scalar_topology/ContourTreeUniformAugmented.h>
#include <viskores/filter/scalar_topology/ContourTreeUniformDistributed.h>
#include <viskores/filter/scalar_topology/DistributedBranchDecompositionFilter.h>
#include <viskores/filter/scalar_topology/ExtractTopVolumeContoursFilter.h>
#include <viskores/filter/scalar_topology/SelectTopVolumeBranchesFilter.h>
#include <viskores/filter/vector_analysis/Gradient.h>
#include <viskores/filter/vector_analysis/VectorMagnitude.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/ProcessContourTree.h>
#include <viskores/rendering/Actor.h>
#include <viskores/rendering/Camera.h>
#include <viskores/rendering/CanvasRayTracer.h>
#include <viskores/rendering/MapperRayTracer.h>
#include <viskores/rendering/MapperVolume.h>
#include <viskores/rendering/MapperWireframer.h>
#include <viskores/rendering/Scene.h>
#include <viskores/rendering/View3D.h>
#include <viskores/source/Tangle.h>

#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace
{

using Association = viskores::cont::Field::Association;

constexpr int NumPyFloatType =
  std::is_same_v<viskores::FloatDefault, viskores::Float64> ? NPY_DOUBLE : NPY_FLOAT;

struct PyDataSetObject
{
  PyObject_HEAD std::shared_ptr<viskores::cont::DataSet>* DataSet;
};

struct PyPartitionedDataSetObject
{
  PyObject_HEAD std::shared_ptr<viskores::cont::PartitionedDataSet>* DataSet;
};

PyTypeObject PyDataSetType = {
  PyVarObject_HEAD_INIT(nullptr, 0)
};

PyTypeObject PyPartitionedDataSetType = {
  PyVarObject_HEAD_INIT(nullptr, 0)
};

void SetPythonError(const std::exception& error)
{
  PyErr_SetString(PyExc_RuntimeError, error.what());
}

template <typename ComponentType, int NumPyType>
PyObject* CopyUnknownArrayToNumPy(const viskores::cont::UnknownArrayHandle& array)
{
  const viskores::Id numberOfValues = array.GetNumberOfValues();
  const viskores::IdComponent numberOfComponents = array.GetNumberOfComponentsFlat();
  if (numberOfComponents <= 0)
  {
    throw std::runtime_error("Cannot export arrays with variable component counts to NumPy.");
  }

  if (numberOfComponents == 1)
  {
    npy_intp dims[1] = { static_cast<npy_intp>(numberOfValues) };
    PyObject* output = PyArray_SimpleNew(1, dims, NumPyType);
    if (output == nullptr)
    {
      return nullptr;
    }
    auto component = array.ExtractComponent<ComponentType>(0);
    auto portal = component.ReadPortal();
    auto* outputData =
      static_cast<ComponentType*>(PyArray_DATA(reinterpret_cast<PyArrayObject*>(output)));
    for (viskores::Id index = 0; index < numberOfValues; ++index)
    {
      outputData[index] = portal.Get(index);
    }
    return output;
  }

  npy_intp dims[2] = { static_cast<npy_intp>(numberOfValues),
                       static_cast<npy_intp>(numberOfComponents) };
  PyObject* output = PyArray_SimpleNew(2, dims, NumPyType);
  if (output == nullptr)
  {
    return nullptr;
  }
  auto* outputData =
    static_cast<ComponentType*>(PyArray_DATA(reinterpret_cast<PyArrayObject*>(output)));
  for (viskores::IdComponent componentIndex = 0; componentIndex < numberOfComponents;
       ++componentIndex)
  {
    auto component = array.ExtractComponent<ComponentType>(componentIndex);
    auto portal = component.ReadPortal();
    for (viskores::Id tupleIndex = 0; tupleIndex < numberOfValues; ++tupleIndex)
    {
      outputData[(tupleIndex * numberOfComponents) + componentIndex] = portal.Get(tupleIndex);
    }
  }
  return output;
}

PyObject* CreateId3ArrayObject(const viskores::cont::ArrayHandle<viskores::Id3>& array)
{
  viskores::cont::UnknownArrayHandle unknown(array);
  return CopyUnknownArrayToNumPy<viskores::Id, NPY_INT64>(unknown);
}

Association ParseAssociation(PyObject* object, Association defaultValue = Association::Any)
{
  if ((object == nullptr) || (object == Py_None))
  {
    return defaultValue;
  }
  if (PyLong_Check(object))
  {
    return static_cast<Association>(PyLong_AsLong(object));
  }
  if (PyUnicode_Check(object))
  {
    const char* text = PyUnicode_AsUTF8(object);
    if (text == nullptr)
    {
      throw std::runtime_error("Association must be a UTF-8 string.");
    }
    std::string value(text);
    if ((value == "any") || (value == "ANY"))
    {
      return Association::Any;
    }
    if ((value == "whole_dataset") || (value == "whole-dataset") || (value == "whole"))
    {
      return Association::WholeDataSet;
    }
    if ((value == "points") || (value == "point"))
    {
      return Association::Points;
    }
    if ((value == "cells") || (value == "cell"))
    {
      return Association::Cells;
    }
    if ((value == "partitions") || (value == "partition"))
    {
      return Association::Partitions;
    }
    if (value == "global")
    {
      return Association::Global;
    }
  }
  throw std::runtime_error("Association must be an int enum value or one of: any, points, cells.");
}

viskores::Id3 ParseDimensions(PyObject* object)
{
  PyObject* sequence = PySequence_Fast(object, "dimensions must be a sequence");
  if (sequence == nullptr)
  {
    throw std::runtime_error("dimensions must be a sequence of 1, 2, or 3 integers.");
  }

  const Py_ssize_t size = PySequence_Fast_GET_SIZE(sequence);
  if ((size < 1) || (size > 3))
  {
    Py_DECREF(sequence);
    throw std::runtime_error("dimensions must contain 1, 2, or 3 integers.");
  }

  viskores::Id3 dims(1, 1, 1);
  PyObject** items = PySequence_Fast_ITEMS(sequence);
  for (Py_ssize_t index = 0; index < size; ++index)
  {
    dims[static_cast<viskores::IdComponent>(index)] = static_cast<viskores::Id>(PyLong_AsLongLong(items[index]));
    if (PyErr_Occurred())
    {
      Py_DECREF(sequence);
      throw std::runtime_error("dimensions must contain integer values.");
    }
  }
  Py_DECREF(sequence);
  return dims;
}

viskores::Vec3f ParseVec3(PyObject* object, const viskores::Vec3f& defaultValue)
{
  if ((object == nullptr) || (object == Py_None))
  {
    return defaultValue;
  }

  PyObject* sequence = PySequence_Fast(object, "expected a sequence");
  if (sequence == nullptr)
  {
    throw std::runtime_error("Expected a sequence of 1, 2, or 3 floats.");
  }

  const Py_ssize_t size = PySequence_Fast_GET_SIZE(sequence);
  if ((size < 1) || (size > 3))
  {
    Py_DECREF(sequence);
    throw std::runtime_error("Expected a sequence of 1, 2, or 3 floats.");
  }

  viskores::Vec3f value = defaultValue;
  PyObject** items = PySequence_Fast_ITEMS(sequence);
  for (Py_ssize_t index = 0; index < size; ++index)
  {
    value[static_cast<viskores::IdComponent>(index)] =
      static_cast<viskores::FloatDefault>(PyFloat_AsDouble(items[index]));
    if (PyErr_Occurred())
    {
      Py_DECREF(sequence);
      throw std::runtime_error("Vector entries must be numeric.");
    }
  }
  Py_DECREF(sequence);
  return value;
}

viskores::rendering::Color ParseColor(
  PyObject* object,
  const viskores::rendering::Color& defaultValue = viskores::rendering::Color(0, 0, 0, 1))
{
  if ((object == nullptr) || (object == Py_None))
  {
    return defaultValue;
  }

  PyObject* sequence = PySequence_Fast(object, "Expected a sequence of 3 or 4 floats.");
  if (sequence == nullptr)
  {
    throw std::runtime_error("Expected a sequence of 3 or 4 floats.");
  }

  const Py_ssize_t size = PySequence_Fast_GET_SIZE(sequence);
  if ((size != 3) && (size != 4))
  {
    Py_DECREF(sequence);
    throw std::runtime_error("Expected a sequence of 3 or 4 floats.");
  }

  float values[4] = { defaultValue.Components[0],
                      defaultValue.Components[1],
                      defaultValue.Components[2],
                      defaultValue.Components[3] };
  PyObject** items = PySequence_Fast_ITEMS(sequence);
  for (Py_ssize_t index = 0; index < size; ++index)
  {
    values[index] = static_cast<float>(PyFloat_AsDouble(items[index]));
    if (PyErr_Occurred())
    {
      Py_DECREF(sequence);
      throw std::runtime_error("Color entries must be numeric.");
    }
  }
  Py_DECREF(sequence);
  return viskores::rendering::Color(values[0], values[1], values[2], values[3]);
}

viskores::Range ParseRange(PyObject* object, const viskores::Range& defaultValue = viskores::Range())
{
  if ((object == nullptr) || (object == Py_None))
  {
    return defaultValue;
  }

  PyObject* sequence = PySequence_Fast(object, "Expected a sequence of 2 floats.");
  if (sequence == nullptr)
  {
    throw std::runtime_error("Expected a sequence of 2 floats.");
  }

  if (PySequence_Fast_GET_SIZE(sequence) != 2)
  {
    Py_DECREF(sequence);
    throw std::runtime_error("Expected a sequence of 2 floats.");
  }

  PyObject** items = PySequence_Fast_ITEMS(sequence);
  const auto minValue = PyFloat_AsDouble(items[0]);
  const auto maxValue = PyFloat_AsDouble(items[1]);
  Py_DECREF(sequence);
  if (PyErr_Occurred())
  {
    throw std::runtime_error("Range entries must be numeric.");
  }
  return viskores::Range(minValue, maxValue);
}

viskores::Id2 ParseSize2D(PyObject* object, const viskores::Id2& defaultValue = viskores::Id2(1024, 1024))
{
  if ((object == nullptr) || (object == Py_None))
  {
    return defaultValue;
  }

  PyObject* sequence = PySequence_Fast(object, "Expected a sequence of 2 integers.");
  if (sequence == nullptr)
  {
    throw std::runtime_error("Expected a sequence of 2 integers.");
  }

  if (PySequence_Fast_GET_SIZE(sequence) != 2)
  {
    Py_DECREF(sequence);
    throw std::runtime_error("Expected a sequence of 2 integers.");
  }

  viskores::Id2 dims;
  PyObject** items = PySequence_Fast_ITEMS(sequence);
  for (Py_ssize_t index = 0; index < 2; ++index)
  {
    dims[static_cast<viskores::IdComponent>(index)] =
      static_cast<viskores::Id>(PyLong_AsLongLong(items[index]));
    if (PyErr_Occurred())
    {
      Py_DECREF(sequence);
      throw std::runtime_error("Expected a sequence of 2 integers.");
    }
  }
  Py_DECREF(sequence);
  return dims;
}

template <typename VecType>
viskores::cont::UnknownArrayHandle BuildVectorArray(const viskores::FloatDefault* rawData,
                                                    npy_intp numberOfTuples,
                                                    npy_intp numberOfComponents)
{
  std::vector<VecType> values(static_cast<std::size_t>(numberOfTuples));
  for (npy_intp tupleIndex = 0; tupleIndex < numberOfTuples; ++tupleIndex)
  {
    for (npy_intp componentIndex = 0; componentIndex < numberOfComponents; ++componentIndex)
    {
      values[static_cast<std::size_t>(tupleIndex)][static_cast<viskores::IdComponent>(componentIndex)] =
        rawData[(tupleIndex * numberOfComponents) + componentIndex];
    }
  }
  return viskores::cont::make_ArrayHandle(values, viskores::CopyFlag::On);
}

viskores::cont::UnknownArrayHandle NumPyArrayToUnknownArray(PyObject* object)
{
  Py_buffer view;
  if (PyObject_GetBuffer(
        object, &view, PyBUF_FORMAT | PyBUF_ND | PyBUF_STRIDES | PyBUF_C_CONTIGUOUS) != 0)
  {
    throw std::runtime_error("Expected a NumPy-compatible floating-point array.");
  }

  const bool isFloat32 =
    (view.format != nullptr) &&
    ((std::string(view.format) == "f") || (std::string(view.format) == "=f"));
  const bool isFloat64 =
    (view.format != nullptr) &&
    ((std::string(view.format) == "d") || (std::string(view.format) == "=d"));
  if (!isFloat32 && !isFloat64)
  {
    PyBuffer_Release(&view);
    throw std::runtime_error("Only float32 and float64 arrays are currently supported.");
  }

  const auto loadValue = [&](Py_ssize_t linearIndex) -> viskores::FloatDefault {
    if (isFloat32)
    {
      return static_cast<viskores::FloatDefault>(static_cast<const float*>(view.buf)[linearIndex]);
    }
    return static_cast<viskores::FloatDefault>(static_cast<const double*>(view.buf)[linearIndex]);
  };

  viskores::cont::UnknownArrayHandle result;
  if (view.ndim == 1)
  {
    std::vector<viskores::FloatDefault> values(static_cast<std::size_t>(view.shape[0]));
    for (Py_ssize_t index = 0; index < view.shape[0]; ++index)
    {
      values[static_cast<std::size_t>(index)] = loadValue(index);
    }
    result = viskores::cont::make_ArrayHandle(values, viskores::CopyFlag::On);
  }
  else if (view.ndim == 2)
  {
    const npy_intp numberOfTuples = view.shape[0];
    const npy_intp numberOfComponents = view.shape[1];
    std::vector<viskores::FloatDefault> flatValues(
      static_cast<std::size_t>(numberOfTuples * numberOfComponents));
    for (npy_intp index = 0; index < (numberOfTuples * numberOfComponents); ++index)
    {
      flatValues[static_cast<std::size_t>(index)] = loadValue(index);
    }
    switch (numberOfComponents)
    {
      case 2:
        result = BuildVectorArray<viskores::Vec2f>(
          flatValues.data(), numberOfTuples, numberOfComponents);
        break;
      case 3:
        result = BuildVectorArray<viskores::Vec3f>(
          flatValues.data(), numberOfTuples, numberOfComponents);
        break;
      case 4:
        result = BuildVectorArray<viskores::Vec4f>(
          flatValues.data(), numberOfTuples, numberOfComponents);
        break;
      default:
        PyBuffer_Release(&view);
        throw std::runtime_error("Only 1D arrays or Nx{2,3,4} arrays are currently supported.");
    }
  }
  else
  {
    PyBuffer_Release(&view);
    throw std::runtime_error("Only 1D arrays or 2D arrays can be added as fields.");
  }

  PyBuffer_Release(&view);
  return result;
}

PyObject* UnknownArrayToNumPyArray(const viskores::cont::UnknownArrayHandle& defaultArray)
{
  if (defaultArray.IsBaseComponentType<viskores::Float64>())
  {
    return CopyUnknownArrayToNumPy<viskores::Float64, NPY_DOUBLE>(defaultArray);
  }
  if (defaultArray.IsBaseComponentType<viskores::Float32>())
  {
    return CopyUnknownArrayToNumPy<viskores::Float32, NPY_FLOAT>(defaultArray);
  }
  if (defaultArray.IsBaseComponentType<viskores::Id>())
  {
    return CopyUnknownArrayToNumPy<viskores::Id, NPY_INT64>(defaultArray);
  }
  if (defaultArray.IsBaseComponentType<viskores::Int32>())
  {
    return CopyUnknownArrayToNumPy<viskores::Int32, NPY_INT32>(defaultArray);
  }
  if (defaultArray.IsBaseComponentType<viskores::UInt32>())
  {
    return CopyUnknownArrayToNumPy<viskores::UInt32, NPY_UINT32>(defaultArray);
  }
  if (defaultArray.IsBaseComponentType<viskores::UInt64>())
  {
    return CopyUnknownArrayToNumPy<viskores::UInt64, NPY_UINT64>(defaultArray);
  }
  throw std::runtime_error("Unsupported field component type for NumPy export.");
}

PyObject* FieldToNumPyArray(const viskores::cont::Field& field)
{
  return UnknownArrayToNumPyArray(field.GetData());
}

PyObject* IdArrayToNumPy(const viskores::cont::ArrayHandle<viskores::Id>& array)
{
  viskores::cont::UnknownArrayHandle unknown(array);
  return UnknownArrayToNumPyArray(unknown);
}

PyObject* FloatArrayToNumPy(const viskores::cont::ArrayHandle<viskores::FloatDefault>& array)
{
  viskores::cont::UnknownArrayHandle unknown(array);
  return UnknownArrayToNumPyArray(unknown);
}

PyObject* EdgePairArrayToNumPy(
  const viskores::worklet::contourtree_augmented::EdgePairArray& saddlePeak)
{
  const auto portal = saddlePeak.ReadPortal();
  npy_intp dims[2] = { static_cast<npy_intp>(saddlePeak.GetNumberOfValues()), 2 };
  PyObject* output = PyArray_SimpleNew(2, dims, NPY_INT64);
  if (output == nullptr)
  {
    return nullptr;
  }
  auto* values = static_cast<viskores::Id*>(PyArray_DATA(reinterpret_cast<PyArrayObject*>(output)));
  for (viskores::Id index = 0; index < saddlePeak.GetNumberOfValues(); ++index)
  {
    const auto edge = portal.Get(index);
    values[(index * 2) + 0] = edge.first;
    values[(index * 2) + 1] = edge.second;
  }
  return output;
}

PyObject* VectorToNumPy(const std::vector<viskores::FloatDefault>& values)
{
  npy_intp dims[1] = { static_cast<npy_intp>(values.size()) };
  PyObject* output = PyArray_SimpleNew(1, dims, NumPyFloatType);
  if (output == nullptr)
  {
    return nullptr;
  }
  auto* outputData =
    static_cast<viskores::FloatDefault*>(PyArray_DATA(reinterpret_cast<PyArrayObject*>(output)));
  for (std::size_t index = 0; index < values.size(); ++index)
  {
    outputData[index] = values[index];
  }
  return output;
}

viskores::Id3 ComputeNumberOfBlocksPerAxis(viskores::Id3 globalSize, viskores::Id numberOfBlocks)
{
  viskores::Id powerOfTwoPortion = 1;
  while (numberOfBlocks % 2 == 0)
  {
    powerOfTwoPortion *= 2;
    numberOfBlocks /= 2;
  }

  auto findSplitAxis = [](viskores::Id3 size) {
    viskores::IdComponent splitAxis = 0;
    for (viskores::IdComponent d = 1; d < 3; ++d)
    {
      if (size[d] > size[splitAxis])
      {
        splitAxis = d;
      }
    }
    return splitAxis;
  };

  viskores::Id3 blocksPerAxis{ 1, 1, 1 };
  if (numberOfBlocks > 1)
  {
    viskores::IdComponent splitAxis = findSplitAxis(globalSize);
    blocksPerAxis[splitAxis] = numberOfBlocks;
    globalSize[splitAxis] /= numberOfBlocks;
  }

  while (powerOfTwoPortion > 1)
  {
    viskores::IdComponent splitAxis = findSplitAxis(globalSize);
    blocksPerAxis[splitAxis] *= 2;
    globalSize[splitAxis] /= 2;
    powerOfTwoPortion /= 2;
  }

  return blocksPerAxis;
}

std::tuple<viskores::Id3, viskores::Id3, viskores::Id3> ComputeBlockExtents(
  viskores::Id3 globalSize,
  viskores::Id3 blocksPerAxis,
  viskores::Id blockNo)
{
  viskores::Id3 blockIndex, blockOrigin, blockSize;
  for (viskores::IdComponent d = 0; d < 3; ++d)
  {
    blockIndex[d] = blockNo % blocksPerAxis[d];
    blockNo /= blocksPerAxis[d];

    float dx = float(globalSize[d] - 1) / float(blocksPerAxis[d]);
    blockOrigin[d] = viskores::Id(blockIndex[d] * dx);
    viskores::Id maxIdx = blockIndex[d] < blocksPerAxis[d] - 1
      ? viskores::Id((blockIndex[d] + 1) * dx)
      : globalSize[d] - 1;
    blockSize[d] = maxIdx - blockOrigin[d] + 1;
  }
  return std::make_tuple(blockIndex, blockOrigin, blockSize);
}

viskores::cont::DataSet CreateSubDataSet(const viskores::cont::DataSet& ds,
                                         viskores::Id3 blockOrigin,
                                         viskores::Id3 blockSize,
                                         const std::string& fieldName)
{
  viskores::Id3 globalSize;
  ds.GetCellSet().CastAndCallForTypes<VISKORES_DEFAULT_CELL_SET_LIST_STRUCTURED>(
    viskores::worklet::contourtree_augmented::GetPointDimensions(), globalSize);
  const viskores::Id nOutValues = blockSize[0] * blockSize[1] * blockSize[2];

  const auto inDataArrayHandle = ds.GetPointField(fieldName).GetData();
  viskores::cont::ArrayHandle<viskores::Id> copyIdsArray;
  copyIdsArray.Allocate(nOutValues);
  auto copyIdsPortal = copyIdsArray.WritePortal();

  viskores::Id3 outArrIdx;
  for (outArrIdx[2] = 0; outArrIdx[2] < blockSize[2]; ++outArrIdx[2])
    for (outArrIdx[1] = 0; outArrIdx[1] < blockSize[1]; ++outArrIdx[1])
      for (outArrIdx[0] = 0; outArrIdx[0] < blockSize[0]; ++outArrIdx[0])
      {
        viskores::Id3 inArrIdx = outArrIdx + blockOrigin;
        viskores::Id inIdx =
          (inArrIdx[2] * globalSize[1] + inArrIdx[1]) * globalSize[0] + inArrIdx[0];
        viskores::Id outIdx =
          (outArrIdx[2] * blockSize[1] + outArrIdx[1]) * blockSize[0] + outArrIdx[0];
        copyIdsPortal.Set(outIdx, inIdx);
      }

  viskores::cont::Field permutedField;
  bool success =
    viskores::filter::MapFieldPermutation(ds.GetPointField(fieldName), copyIdsArray, permutedField);
  if (!success)
  {
    throw std::runtime_error("Field copy failed while creating a sub-dataset.");
  }

  viskores::cont::DataSetBuilderUniform builder;
  if (globalSize[2] <= 1)
  {
    viskores::Id2 dimensions{ blockSize[0], blockSize[1] };
    auto dataSet = builder.Create(dimensions);
    viskores::cont::CellSetStructured<2> cellSet;
    cellSet.SetPointDimensions(dimensions);
    cellSet.SetGlobalPointDimensions(viskores::Id2{ globalSize[0], globalSize[1] });
    cellSet.SetGlobalPointIndexStart(viskores::Id2{ blockOrigin[0], blockOrigin[1] });
    dataSet.SetCellSet(cellSet);
    dataSet.AddField(permutedField);
    return dataSet;
  }

  auto dataSet = builder.Create(blockSize);
  viskores::cont::CellSetStructured<3> cellSet;
  cellSet.SetPointDimensions(blockSize);
  cellSet.SetGlobalPointDimensions(globalSize);
  cellSet.SetGlobalPointIndexStart(blockOrigin);
  dataSet.SetCellSet(cellSet);
  dataSet.AddField(permutedField);
  return dataSet;
}

viskores::cont::ArrayHandle<viskores::Id3> ParseId3Array(PyObject* object)
{
  Py_buffer view;
  if (PyObject_GetBuffer(
        object, &view, PyBUF_FORMAT | PyBUF_ND | PyBUF_STRIDES | PyBUF_C_CONTIGUOUS) != 0)
  {
    throw std::runtime_error("Expected a C-contiguous integer array with shape (N, 3).");
  }

  const bool isInt64 =
    (view.format != nullptr) &&
    ((std::string(view.format) == "q") || (std::string(view.format) == "=q") ||
     (std::string(view.format) == "l") || (std::string(view.format) == "=l"));
  const bool isInt32 =
    (view.format != nullptr) &&
    ((std::string(view.format) == "i") || (std::string(view.format) == "=i"));
  if ((!isInt64 && !isInt32) || (view.ndim != 2) || (view.shape[1] != 3))
  {
    PyBuffer_Release(&view);
    throw std::runtime_error("Expected a C-contiguous integer array with shape (N, 3).");
  }

  std::vector<viskores::Id3> values(static_cast<std::size_t>(view.shape[0]));
  for (Py_ssize_t row = 0; row < view.shape[0]; ++row)
  {
    for (Py_ssize_t col = 0; col < 3; ++col)
    {
      const Py_ssize_t linearIndex = (row * 3) + col;
      values[static_cast<std::size_t>(row)][static_cast<viskores::IdComponent>(col)] =
        isInt64 ? static_cast<viskores::Id>(static_cast<const long long*>(view.buf)[linearIndex])
                : static_cast<viskores::Id>(static_cast<const int*>(view.buf)[linearIndex]);
    }
  }
  PyBuffer_Release(&view);
  return viskores::cont::make_ArrayHandle(values, viskores::CopyFlag::On);
}

PyDataSetObject* RequireDataSet(PyObject* object)
{
  if (!PyObject_TypeCheck(object, &PyDataSetType))
  {
    PyErr_SetString(PyExc_TypeError, "Expected a viskores.DataSet instance.");
    return nullptr;
  }
  return reinterpret_cast<PyDataSetObject*>(object);
}

PyPartitionedDataSetObject* RequirePartitionedDataSet(PyObject* object)
{
  if (!PyObject_TypeCheck(object, &PyPartitionedDataSetType))
  {
    PyErr_SetString(PyExc_TypeError, "Expected a viskores.PartitionedDataSet instance.");
    return nullptr;
  }
  return reinterpret_cast<PyPartitionedDataSetObject*>(object);
}

PyObject* NewPyDataSet(const viskores::cont::DataSet& dataSet)
{
  PyDataSetObject* object = PyObject_New(PyDataSetObject, &PyDataSetType);
  if (object == nullptr)
  {
    return nullptr;
  }
  object->DataSet = new std::shared_ptr<viskores::cont::DataSet>(
    std::make_shared<viskores::cont::DataSet>(dataSet));
  return reinterpret_cast<PyObject*>(object);
}

PyObject* NewPyPartitionedDataSet(const viskores::cont::PartitionedDataSet& dataSet)
{
  PyPartitionedDataSetObject* object =
    PyObject_New(PyPartitionedDataSetObject, &PyPartitionedDataSetType);
  if (object == nullptr)
  {
    return nullptr;
  }
  object->DataSet = new std::shared_ptr<viskores::cont::PartitionedDataSet>(
    std::make_shared<viskores::cont::PartitionedDataSet>(dataSet));
  return reinterpret_cast<PyObject*>(object);
}

void PyDataSetDealloc(PyDataSetObject* self)
{
  delete self->DataSet;
  Py_TYPE(self)->tp_free(reinterpret_cast<PyObject*>(self));
}

void PyPartitionedDataSetDealloc(PyPartitionedDataSetObject* self)
{
  delete self->DataSet;
  Py_TYPE(self)->tp_free(reinterpret_cast<PyObject*>(self));
}

PyObject* PyDataSetNew(PyTypeObject* type, PyObject*, PyObject*)
{
  PyDataSetObject* self = reinterpret_cast<PyDataSetObject*>(type->tp_alloc(type, 0));
  if (self != nullptr)
  {
    self->DataSet = new std::shared_ptr<viskores::cont::DataSet>(
      std::make_shared<viskores::cont::DataSet>());
  }
  return reinterpret_cast<PyObject*>(self);
}

PyObject* PyPartitionedDataSetNew(PyTypeObject* type, PyObject*, PyObject*)
{
  PyPartitionedDataSetObject* self =
    reinterpret_cast<PyPartitionedDataSetObject*>(type->tp_alloc(type, 0));
  if (self != nullptr)
  {
    self->DataSet = new std::shared_ptr<viskores::cont::PartitionedDataSet>(
      std::make_shared<viskores::cont::PartitionedDataSet>());
  }
  return reinterpret_cast<PyObject*>(self);
}

PyObject* PyDataSetRepr(PyDataSetObject* self)
{
  std::ostringstream stream;
  stream << "viskores.DataSet(points=" << (*self->DataSet)->GetNumberOfPoints()
         << ", cells=" << (*self->DataSet)->GetNumberOfCells()
         << ", fields=" << (*self->DataSet)->GetNumberOfFields() << ")";
  return PyUnicode_FromString(stream.str().c_str());
}

PyObject* PyPartitionedDataSetRepr(PyPartitionedDataSetObject* self)
{
  std::ostringstream stream;
  stream << "viskores.PartitionedDataSet(partitions="
         << (*self->DataSet)->GetNumberOfPartitions() << ")";
  return PyUnicode_FromString(stream.str().c_str());
}

PyObject* PyDataSetAddPointField(PyDataSetObject* self, PyObject* args)
{
  const char* name = nullptr;
  PyObject* values = nullptr;
  if (!PyArg_ParseTuple(args, "sO:add_point_field", &name, &values))
  {
    return nullptr;
  }

  try
  {
    (*self->DataSet)->AddPointField(name, NumPyArrayToUnknownArray(values));
    Py_RETURN_NONE;
  }
  catch (const std::exception& error)
  {
    SetPythonError(error);
    return nullptr;
  }
}

PyObject* PyDataSetAddCellField(PyDataSetObject* self, PyObject* args)
{
  const char* name = nullptr;
  PyObject* values = nullptr;
  if (!PyArg_ParseTuple(args, "sO:add_cell_field", &name, &values))
  {
    return nullptr;
  }

  try
  {
    (*self->DataSet)->AddCellField(name, NumPyArrayToUnknownArray(values));
    Py_RETURN_NONE;
  }
  catch (const std::exception& error)
  {
    SetPythonError(error);
    return nullptr;
  }
}

PyObject* PyDataSetGetField(PyDataSetObject* self, PyObject* args, PyObject* kwargs)
{
  static char* keywords[] = { const_cast<char*>("name"), const_cast<char*>("association"), nullptr };
  const char* name = nullptr;
  PyObject* associationObject = Py_None;
  if (!PyArg_ParseTupleAndKeywords(
        args, kwargs, "s|O:get_field", keywords, &name, &associationObject))
  {
    return nullptr;
  }

  try
  {
    Association association = ParseAssociation(associationObject, Association::Any);
    const auto& field = (*self->DataSet)->GetField(name, association);
    return FieldToNumPyArray(field);
  }
  catch (const std::exception& error)
  {
    SetPythonError(error);
    return nullptr;
  }
}

PyObject* PyDataSetHasField(PyDataSetObject* self, PyObject* args, PyObject* kwargs)
{
  static char* keywords[] = { const_cast<char*>("name"), const_cast<char*>("association"), nullptr };
  const char* name = nullptr;
  PyObject* associationObject = Py_None;
  if (!PyArg_ParseTupleAndKeywords(
        args, kwargs, "s|O:has_field", keywords, &name, &associationObject))
  {
    return nullptr;
  }

  try
  {
    Association association = ParseAssociation(associationObject, Association::Any);
    if ((*self->DataSet)->HasField(name, association))
    {
      Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
  }
  catch (const std::exception& error)
  {
    SetPythonError(error);
    return nullptr;
  }
}

PyObject* PyDataSetFieldNames(PyDataSetObject* self, PyObject*)
{
  const viskores::IdComponent numberOfFields = (*self->DataSet)->GetNumberOfFields();
  PyObject* list = PyList_New(numberOfFields);
  if (list == nullptr)
  {
    return nullptr;
  }

  for (viskores::IdComponent index = 0; index < numberOfFields; ++index)
  {
    const auto& name = (*self->DataSet)->GetField(index).GetName();
    PyObject* value = PyUnicode_FromString(name.c_str());
    if (value == nullptr)
    {
      Py_DECREF(list);
      return nullptr;
    }
    PyList_SET_ITEM(list, index, value);
  }
  return list;
}

PyObject* PyDataSetGetNumberOfPoints(PyDataSetObject* self, void*)
{
  return PyLong_FromLongLong((*self->DataSet)->GetNumberOfPoints());
}

PyObject* PyDataSetGetNumberOfCells(PyDataSetObject* self, void*)
{
  return PyLong_FromLongLong((*self->DataSet)->GetNumberOfCells());
}

PyObject* PyPartitionedDataSetAppendPartition(PyPartitionedDataSetObject* self, PyObject* args)
{
  PyObject* datasetObject = nullptr;
  if (!PyArg_ParseTuple(args, "O:append_partition", &datasetObject))
  {
    return nullptr;
  }
  PyDataSetObject* dataset = RequireDataSet(datasetObject);
  if (dataset == nullptr)
  {
    return nullptr;
  }
  (*self->DataSet)->AppendPartition(*(*dataset->DataSet));
  Py_RETURN_NONE;
}

PyObject* PyPartitionedDataSetGetPartition(PyPartitionedDataSetObject* self, PyObject* args)
{
  long long index = 0;
  if (!PyArg_ParseTuple(args, "L:get_partition", &index))
  {
    return nullptr;
  }
  try
  {
    return NewPyDataSet((*self->DataSet)->GetPartition(static_cast<viskores::Id>(index)));
  }
  catch (const std::exception& error)
  {
    SetPythonError(error);
    return nullptr;
  }
}

PyObject* PyPartitionedDataSetGetField(PyPartitionedDataSetObject* self,
                                       PyObject* args,
                                       PyObject* kwargs)
{
  static char* keywords[] = { const_cast<char*>("name"), const_cast<char*>("association"), nullptr };
  const char* name = nullptr;
  PyObject* associationObject = Py_None;
  if (!PyArg_ParseTupleAndKeywords(
        args, kwargs, "s|O:get_field", keywords, &name, &associationObject))
  {
    return nullptr;
  }
  try
  {
    Association association = ParseAssociation(associationObject, Association::Any);
    const auto& field = (*self->DataSet)->GetField(name, association);
    return FieldToNumPyArray(field);
  }
  catch (const std::exception& error)
  {
    SetPythonError(error);
    return nullptr;
  }
}

PyObject* PyPartitionedDataSetGetNumberOfPartitions(PyPartitionedDataSetObject* self, void*)
{
  return PyLong_FromLongLong((*self->DataSet)->GetNumberOfPartitions());
}

PyMethodDef PyDataSetMethods[] = {
  { "add_point_field", reinterpret_cast<PyCFunction>(PyDataSetAddPointField), METH_VARARGS, nullptr },
  { "add_cell_field", reinterpret_cast<PyCFunction>(PyDataSetAddCellField), METH_VARARGS, nullptr },
  { "get_field",
    reinterpret_cast<PyCFunction>(PyDataSetGetField),
    METH_VARARGS | METH_KEYWORDS,
    nullptr },
  { "has_field",
    reinterpret_cast<PyCFunction>(PyDataSetHasField),
    METH_VARARGS | METH_KEYWORDS,
    nullptr },
  { "field_names", reinterpret_cast<PyCFunction>(PyDataSetFieldNames), METH_NOARGS, nullptr },
  { nullptr, nullptr, 0, nullptr }
};

PyMethodDef PyPartitionedDataSetMethods[] = {
  { "append_partition",
    reinterpret_cast<PyCFunction>(PyPartitionedDataSetAppendPartition),
    METH_VARARGS,
    nullptr },
  { "get_partition",
    reinterpret_cast<PyCFunction>(PyPartitionedDataSetGetPartition),
    METH_VARARGS,
    nullptr },
  { "get_field",
    reinterpret_cast<PyCFunction>(PyPartitionedDataSetGetField),
    METH_VARARGS | METH_KEYWORDS,
    nullptr },
  { nullptr, nullptr, 0, nullptr }
};

PyGetSetDef PyDataSetGetSet[] = {
  { const_cast<char*>("number_of_points"),
    reinterpret_cast<getter>(PyDataSetGetNumberOfPoints),
    nullptr,
    nullptr,
    nullptr },
  { const_cast<char*>("number_of_cells"),
    reinterpret_cast<getter>(PyDataSetGetNumberOfCells),
    nullptr,
    nullptr,
    nullptr },
  { nullptr, nullptr, nullptr, nullptr, nullptr }
};

PyGetSetDef PyPartitionedDataSetGetSet[] = {
  { const_cast<char*>("number_of_partitions"),
    reinterpret_cast<getter>(PyPartitionedDataSetGetNumberOfPartitions),
    nullptr,
    nullptr,
    nullptr },
  { nullptr, nullptr, nullptr, nullptr, nullptr }
};

template <typename FilterType, typename ConfigureFunctor>
PyObject* ExecuteFieldFilter(PyObject* datasetObject,
                             const char* fieldName,
                             const char* outputFieldName,
                             ConfigureFunctor&& configure)
{
  PyDataSetObject* dataSet = RequireDataSet(datasetObject);
  if (dataSet == nullptr)
  {
    return nullptr;
  }

  try
  {
    FilterType filter;
    filter.SetActiveField(fieldName, Association::Any);
    if ((outputFieldName != nullptr) && (outputFieldName[0] != '\0'))
    {
      filter.SetOutputFieldName(outputFieldName);
    }
    configure(filter);
    auto result = filter.Execute(*(*dataSet->DataSet));
    return NewPyDataSet(result);
  }
  catch (const std::exception& error)
  {
    SetPythonError(error);
    return nullptr;
  }
}

PyObject* PyCreateUniformDataSet(PyObject*, PyObject* args, PyObject* kwargs)
{
  static char* keywords[] = {
    const_cast<char*>("dimensions"),
    const_cast<char*>("origin"),
    const_cast<char*>("spacing"),
    const_cast<char*>("coord_name"),
    nullptr
  };
  PyObject* dimensionsObject = nullptr;
  PyObject* originObject = Py_None;
  PyObject* spacingObject = Py_None;
  const char* coordinateName = "coords";
  if (!PyArg_ParseTupleAndKeywords(args,
                                   kwargs,
                                   "O|OOs:create_uniform_dataset",
                                   keywords,
                                   &dimensionsObject,
                                   &originObject,
                                   &spacingObject,
                                   &coordinateName))
  {
    return nullptr;
  }

  try
  {
    const auto dimensions = ParseDimensions(dimensionsObject);
    const auto origin = ParseVec3(originObject, viskores::Vec3f(0.0f, 0.0f, 0.0f));
    const auto spacing = ParseVec3(spacingObject, viskores::Vec3f(1.0f, 1.0f, 1.0f));
    auto dataSet = viskores::cont::DataSetBuilderUniform::Create(dimensions, origin, spacing, coordinateName);
    return NewPyDataSet(dataSet);
  }
  catch (const std::exception& error)
  {
    SetPythonError(error);
    return nullptr;
  }
}

PyObject* PyCellAverage(PyObject*, PyObject* args, PyObject* kwargs)
{
  static char* keywords[] = {
    const_cast<char*>("dataset"),
    const_cast<char*>("field_name"),
    const_cast<char*>("output_field_name"),
    nullptr
  };
  PyObject* datasetObject = nullptr;
  const char* fieldName = nullptr;
  const char* outputFieldName = nullptr;
  if (!PyArg_ParseTupleAndKeywords(
        args, kwargs, "Os|z:cell_average", keywords, &datasetObject, &fieldName, &outputFieldName))
  {
    return nullptr;
  }

  return ExecuteFieldFilter<viskores::filter::field_conversion::CellAverage>(
    datasetObject, fieldName, outputFieldName, [](auto&) {});
}

PyObject* PyPointAverage(PyObject*, PyObject* args, PyObject* kwargs)
{
  static char* keywords[] = {
    const_cast<char*>("dataset"),
    const_cast<char*>("field_name"),
    const_cast<char*>("output_field_name"),
    nullptr
  };
  PyObject* datasetObject = nullptr;
  const char* fieldName = nullptr;
  const char* outputFieldName = nullptr;
  if (!PyArg_ParseTupleAndKeywords(
        args, kwargs, "Os|z:point_average", keywords, &datasetObject, &fieldName, &outputFieldName))
  {
    return nullptr;
  }

  return ExecuteFieldFilter<viskores::filter::field_conversion::PointAverage>(
    datasetObject, fieldName, outputFieldName, [](auto&) {});
}

PyObject* PyVectorMagnitude(PyObject*, PyObject* args, PyObject* kwargs)
{
  static char* keywords[] = {
    const_cast<char*>("dataset"),
    const_cast<char*>("field_name"),
    const_cast<char*>("output_field_name"),
    nullptr
  };
  PyObject* datasetObject = nullptr;
  const char* fieldName = nullptr;
  const char* outputFieldName = nullptr;
  if (!PyArg_ParseTupleAndKeywords(args,
                                   kwargs,
                                   "Os|z:vector_magnitude",
                                   keywords,
                                   &datasetObject,
                                   &fieldName,
                                   &outputFieldName))
  {
    return nullptr;
  }

  return ExecuteFieldFilter<viskores::filter::vector_analysis::VectorMagnitude>(
    datasetObject, fieldName, outputFieldName, [](auto&) {});
}

PyObject* PyGradient(PyObject*, PyObject* args, PyObject* kwargs)
{
  static char* keywords[] = {
    const_cast<char*>("dataset"),
    const_cast<char*>("field_name"),
    const_cast<char*>("output_field_name"),
    const_cast<char*>("compute_point_gradient"),
    nullptr
  };
  PyObject* datasetObject = nullptr;
  const char* fieldName = nullptr;
  const char* outputFieldName = nullptr;
  int computePointGradient = 0;
  if (!PyArg_ParseTupleAndKeywords(args,
                                   kwargs,
                                   "Os|zp:gradient",
                                   keywords,
                                   &datasetObject,
                                   &fieldName,
                                   &outputFieldName,
                                   &computePointGradient))
  {
    return nullptr;
  }

  return ExecuteFieldFilter<viskores::filter::vector_analysis::Gradient>(
    datasetObject, fieldName, outputFieldName, [computePointGradient](auto& filter) {
      filter.SetComputePointGradient(computePointGradient != 0);
    });
}

std::vector<viskores::Float64> ParseIsoValues(PyObject* object)
{
  PyObject* sequence = PySequence_Fast(object, "iso_values must be a sequence of floats");
  if (sequence == nullptr)
  {
    throw std::runtime_error("iso_values must be a sequence of floats.");
  }

  const Py_ssize_t size = PySequence_Fast_GET_SIZE(sequence);
  if (size < 1)
  {
    Py_DECREF(sequence);
    throw std::runtime_error("iso_values must contain at least one entry.");
  }

  std::vector<viskores::Float64> values(static_cast<std::size_t>(size));
  PyObject** items = PySequence_Fast_ITEMS(sequence);
  for (Py_ssize_t index = 0; index < size; ++index)
  {
    values[static_cast<std::size_t>(index)] = PyFloat_AsDouble(items[index]);
    if (PyErr_Occurred())
    {
      Py_DECREF(sequence);
      throw std::runtime_error("iso_values must contain numeric entries.");
    }
  }
  Py_DECREF(sequence);
  return values;
}

PyObject* PyContour(PyObject*, PyObject* args, PyObject* kwargs)
{
  static char* keywords[] = {
    const_cast<char*>("dataset"),
    const_cast<char*>("field_name"),
    const_cast<char*>("iso_values"),
    const_cast<char*>("generate_normals"),
    nullptr
  };
  PyObject* datasetObject = nullptr;
  const char* fieldName = nullptr;
  PyObject* isoValuesObject = nullptr;
  int generateNormals = 0;
  if (!PyArg_ParseTupleAndKeywords(args,
                                   kwargs,
                                   "OsO|p:contour",
                                   keywords,
                                   &datasetObject,
                                   &fieldName,
                                   &isoValuesObject,
                                   &generateNormals))
  {
    return nullptr;
  }

  auto isoValues = ParseIsoValues(isoValuesObject);
  return ExecuteFieldFilter<viskores::filter::contour::Contour>(
    datasetObject, fieldName, nullptr, [&isoValues, generateNormals](auto& filter) {
      filter.SetIsoValues(isoValues);
      filter.SetGenerateNormals(generateNormals != 0);
    });
}

PyObject* PyCreateTangleDataSet(PyObject*, PyObject* args, PyObject* kwargs)
{
  static char* keywords[] = { const_cast<char*>("point_dimensions"), nullptr };
  PyObject* dimensionsObject = Py_None;
  if (!PyArg_ParseTupleAndKeywords(
        args, kwargs, "|O:create_tangle_dataset", keywords, &dimensionsObject))
  {
    return nullptr;
  }

  try
  {
    viskores::source::Tangle tangle;
    if (dimensionsObject != Py_None)
    {
      tangle.SetPointDimensions(ParseDimensions(dimensionsObject));
    }
    return NewPyDataSet(tangle.Execute());
  }
  catch (const std::exception& error)
  {
    SetPythonError(error);
    return nullptr;
  }
}

PyObject* PyPartitionUniformDataSet(PyObject*, PyObject* args, PyObject* kwargs)
{
  static char* keywords[] = {
    const_cast<char*>("dataset"),
    const_cast<char*>("field_name"),
    const_cast<char*>("num_blocks"),
    nullptr
  };
  PyObject* datasetObject = nullptr;
  const char* fieldName = nullptr;
  long long numBlocks = 1;
  if (!PyArg_ParseTupleAndKeywords(args,
                                   kwargs,
                                   "OsL:partition_uniform_dataset",
                                   keywords,
                                   &datasetObject,
                                   &fieldName,
                                   &numBlocks))
  {
    return nullptr;
  }

  PyDataSetObject* dataset = RequireDataSet(datasetObject);
  if (dataset == nullptr)
  {
    return nullptr;
  }

  try
  {
    viskores::Id3 globalSize;
    (*dataset->DataSet)->GetCellSet().CastAndCallForTypes<VISKORES_DEFAULT_CELL_SET_LIST_STRUCTURED>(
      viskores::worklet::contourtree_augmented::GetPointDimensions(), globalSize);

    const viskores::Id3 blocksPerDim =
      ComputeNumberOfBlocksPerAxis(globalSize, static_cast<viskores::Id>(numBlocks));
    viskores::cont::PartitionedDataSet partitions;
    viskores::cont::ArrayHandle<viskores::Id3> localBlockIndices;
    localBlockIndices.Allocate(static_cast<viskores::Id>(numBlocks));
    auto portal = localBlockIndices.WritePortal();

    for (viskores::Id blockNo = 0; blockNo < static_cast<viskores::Id>(numBlocks); ++blockNo)
    {
      auto [blockIndex, blockOrigin, blockSize] =
        ComputeBlockExtents(globalSize, blocksPerDim, blockNo);
      partitions.AppendPartition(
        CreateSubDataSet(*(*dataset->DataSet), blockOrigin, blockSize, fieldName));
      portal.Set(blockNo, blockIndex);
    }

    PyObject* result = PyTuple_New(3);
    PyTuple_SET_ITEM(result, 0, NewPyPartitionedDataSet(partitions));
    PyTuple_SET_ITEM(result,
                     1,
                     Py_BuildValue("(LLL)",
                                   static_cast<long long>(blocksPerDim[0]),
                                   static_cast<long long>(blocksPerDim[1]),
                                   static_cast<long long>(blocksPerDim[2])));
    PyTuple_SET_ITEM(result, 2, CreateId3ArrayObject(localBlockIndices));
    return result;
  }
  catch (const std::exception& error)
  {
    SetPythonError(error);
    return nullptr;
  }
}

PyObject* PyContourTreeAugmented(PyObject*, PyObject* args, PyObject* kwargs)
{
  static char* keywords[] = {
    const_cast<char*>("dataset"),
    const_cast<char*>("field_name"),
    const_cast<char*>("use_marching_cubes"),
    const_cast<char*>("compute_regular_structure"),
    const_cast<char*>("compute_branch_decomposition"),
    const_cast<char*>("num_levels"),
    const_cast<char*>("contour_type"),
    const_cast<char*>("eps"),
    const_cast<char*>("num_components"),
    const_cast<char*>("contour_select_method"),
    const_cast<char*>("use_persistence_sorter"),
    nullptr
  };
  PyObject* datasetObject = nullptr;
  const char* fieldName = nullptr;
  int useMarchingCubes = 0;
  unsigned int computeRegularStructure = 1;
  int computeBranchDecomposition = 0;
  long long numLevels = 0;
  int contourType = 0;
  double eps = 1e-5;
  long long numComponents = 0;
  int contourSelectMethod = 0;
  int usePersistenceSorter = 1;
  if (!PyArg_ParseTupleAndKeywords(args,
                                   kwargs,
                                   "Os|pIpLidLip:contour_tree_augmented",
                                   keywords,
                                   &datasetObject,
                                   &fieldName,
                                   &useMarchingCubes,
                                   &computeRegularStructure,
                                   &computeBranchDecomposition,
                                   &numLevels,
                                   &contourType,
                                   &eps,
                                   &numComponents,
                                   &contourSelectMethod,
                                   &usePersistenceSorter))
  {
    return nullptr;
  }

  PyDataSetObject* dataset = RequireDataSet(datasetObject);
  if (dataset == nullptr)
  {
    return nullptr;
  }

  try
  {
    viskores::filter::scalar_topology::ContourTreeAugmented filter(
      useMarchingCubes != 0, computeRegularStructure);
    filter.SetActiveField(fieldName);
    auto result = filter.Execute(*(*dataset->DataSet));

    PyObject* output = PyDict_New();
    PyDict_SetItemString(output, "result", NewPyDataSet(result));
    viskores::worklet::contourtree_augmented::EdgePairArray saddlePeak;
    viskores::worklet::contourtree_augmented::ProcessContourTree::CollectSortedSuperarcs(
      filter.GetContourTree(), filter.GetSortOrder(), saddlePeak);
    PyDict_SetItemString(output, "sorted_superarcs", EdgePairArrayToNumPy(saddlePeak));
    PyDict_SetItemString(output, "sort_order", IdArrayToNumPy(filter.GetSortOrder()));
    PyDict_SetItemString(
      output, "num_iterations", PyLong_FromLongLong(filter.GetNumIterations()));

    if (computeBranchDecomposition != 0)
    {
      using IdArrayType = viskores::worklet::contourtree_augmented::IdArrayType;
      IdArrayType superarcIntrinsicWeight;
      IdArrayType superarcDependentWeight;
      IdArrayType supernodeTransferWeight;
      IdArrayType hyperarcDependentWeight;
      viskores::worklet::contourtree_augmented::ProcessContourTree::ComputeVolumeWeightsSerial(
        filter.GetContourTree(),
        filter.GetNumIterations(),
        superarcIntrinsicWeight,
        superarcDependentWeight,
        supernodeTransferWeight,
        hyperarcDependentWeight);

      IdArrayType whichBranch;
      IdArrayType branchMinimum;
      IdArrayType branchMaximum;
      IdArrayType branchSaddle;
      IdArrayType branchParent;
      viskores::worklet::contourtree_augmented::ProcessContourTree::
        ComputeVolumeBranchDecompositionSerial(filter.GetContourTree(),
                                               superarcDependentWeight,
                                               superarcIntrinsicWeight,
                                               whichBranch,
                                               branchMinimum,
                                               branchMaximum,
                                               branchSaddle,
                                               branchParent);

      PyDict_SetItemString(output, "which_branch", IdArrayToNumPy(whichBranch));
      PyDict_SetItemString(output, "branch_minimum", IdArrayToNumPy(branchMinimum));
      PyDict_SetItemString(output, "branch_maximum", IdArrayToNumPy(branchMaximum));
      PyDict_SetItemString(output, "branch_saddle", IdArrayToNumPy(branchSaddle));
      PyDict_SetItemString(output, "branch_parent", IdArrayToNumPy(branchParent));
      PyDict_SetItemString(
        output, "superarc_intrinsic_weight", IdArrayToNumPy(superarcIntrinsicWeight));
      PyDict_SetItemString(
        output, "superarc_dependent_weight", IdArrayToNumPy(superarcDependentWeight));

      if (numLevels > 0)
      {
        auto dataField =
          (*dataset->DataSet)
            ->GetField(fieldName)
            .GetDataAsDefaultFloat()
            .AsArrayHandle<viskores::cont::ArrayHandle<viskores::FloatDefault>>();
        auto* root =
          viskores::worklet::contourtree_augmented::ProcessContourTree::ComputeBranchDecomposition<
            viskores::FloatDefault>(filter.GetContourTree().Superparents,
                                    filter.GetContourTree().Supernodes,
                                    whichBranch,
                                    branchMinimum,
                                    branchMaximum,
                                    branchSaddle,
                                    branchParent,
                                    filter.GetSortOrder(),
                                    dataField,
                                    false);
        const viskores::Id components =
          (numComponents > 0) ? static_cast<viskores::Id>(numComponents)
                              : static_cast<viskores::Id>(numLevels + 1);
        root->SimplifyToSize(components, usePersistenceSorter != 0);
        std::vector<viskores::FloatDefault> isoValues;
        if (contourSelectMethod == 1)
        {
          viskores::worklet::contourtree_augmented::process_contourtree_inc::
            PiecewiseLinearFunction<viskores::FloatDefault>
              plf;
          root->AccumulateIntervals(
            contourType, static_cast<viskores::FloatDefault>(eps), plf);
          isoValues = plf.nLargest(static_cast<unsigned int>(numLevels));
        }
        else
        {
          root->GetRelevantValues(
            contourType, static_cast<viskores::FloatDefault>(eps), isoValues);
        }
        delete root;
        std::sort(isoValues.begin(), isoValues.end());
        isoValues.erase(std::unique(isoValues.begin(), isoValues.end()), isoValues.end());
        PyDict_SetItemString(output, "isovalues", VectorToNumPy(isoValues));
      }
    }

    return output;
  }
  catch (const std::exception& error)
  {
    SetPythonError(error);
    return nullptr;
  }
}

PyObject* PyContourTreeDistributed(PyObject*, PyObject* args, PyObject* kwargs)
{
  static char* keywords[] = {
    const_cast<char*>("partitioned_dataset"),
    const_cast<char*>("field_name"),
    const_cast<char*>("blocks_per_dimension"),
    const_cast<char*>("local_block_indices"),
    const_cast<char*>("use_boundary_extrema_only"),
    const_cast<char*>("use_marching_cubes"),
    const_cast<char*>("augment_hierarchical_tree"),
    const_cast<char*>("presimplify_threshold"),
    const_cast<char*>("save_dot_files"),
    nullptr
  };
  PyObject* partitionedObject = nullptr;
  const char* fieldName = nullptr;
  PyObject* blocksPerDimensionObject = nullptr;
  PyObject* localBlockIndicesObject = nullptr;
  int useBoundaryExtremaOnly = 1;
  int useMarchingCubes = 0;
  int augmentHierarchicalTree = 0;
  long long presimplifyThreshold = 0;
  int saveDotFiles = 0;
  if (!PyArg_ParseTupleAndKeywords(args,
                                   kwargs,
                                   "OsOO|pppLp:contour_tree_distributed",
                                   keywords,
                                   &partitionedObject,
                                   &fieldName,
                                   &blocksPerDimensionObject,
                                   &localBlockIndicesObject,
                                   &useBoundaryExtremaOnly,
                                   &useMarchingCubes,
                                   &augmentHierarchicalTree,
                                   &presimplifyThreshold,
                                   &saveDotFiles))
  {
    return nullptr;
  }

  PyPartitionedDataSetObject* partitioned = RequirePartitionedDataSet(partitionedObject);
  if (partitioned == nullptr)
  {
    return nullptr;
  }

  try
  {
    auto blocksPerDimension = ParseDimensions(blocksPerDimensionObject);
    auto localBlockIndices = ParseId3Array(localBlockIndicesObject);
    viskores::filter::scalar_topology::ContourTreeUniformDistributed filter;
    filter.SetActiveField(fieldName);
    filter.SetUseBoundaryExtremaOnly(useBoundaryExtremaOnly != 0);
    filter.SetUseMarchingCubes(useMarchingCubes != 0);
    filter.SetAugmentHierarchicalTree(augmentHierarchicalTree != 0);
    filter.SetPresimplifyThreshold(static_cast<viskores::Id>(presimplifyThreshold));
    filter.SetSaveDotFiles(saveDotFiles != 0);
    filter.SetBlockIndices(blocksPerDimension, localBlockIndices);
    return NewPyPartitionedDataSet(filter.Execute(*(*partitioned->DataSet)));
  }
  catch (const std::exception& error)
  {
    SetPythonError(error);
    return nullptr;
  }
}

PyObject* PyDistributedBranchDecomposition(PyObject*, PyObject* args)
{
  PyObject* partitionedObject = nullptr;
  if (!PyArg_ParseTuple(args, "O:distributed_branch_decomposition", &partitionedObject))
  {
    return nullptr;
  }
  PyPartitionedDataSetObject* partitioned = RequirePartitionedDataSet(partitionedObject);
  if (partitioned == nullptr)
  {
    return nullptr;
  }
  try
  {
    viskores::filter::scalar_topology::DistributedBranchDecompositionFilter filter;
    return NewPyPartitionedDataSet(filter.Execute(*(*partitioned->DataSet)));
  }
  catch (const std::exception& error)
  {
    SetPythonError(error);
    return nullptr;
  }
}

PyObject* PySelectTopVolumeBranches(PyObject*, PyObject* args, PyObject* kwargs)
{
  static char* keywords[] = {
    const_cast<char*>("partitioned_dataset"),
    const_cast<char*>("num_saved_branches"),
    const_cast<char*>("presimplify_threshold"),
    nullptr
  };
  PyObject* partitionedObject = nullptr;
  long long numSavedBranches = 1;
  long long presimplifyThreshold = 0;
  if (!PyArg_ParseTupleAndKeywords(args,
                                   kwargs,
                                   "OL|L:select_top_volume_branches",
                                   keywords,
                                   &partitionedObject,
                                   &numSavedBranches,
                                   &presimplifyThreshold))
  {
    return nullptr;
  }
  PyPartitionedDataSetObject* partitioned = RequirePartitionedDataSet(partitionedObject);
  if (partitioned == nullptr)
  {
    return nullptr;
  }
  try
  {
    viskores::filter::scalar_topology::SelectTopVolumeBranchesFilter filter;
    filter.SetSavedBranches(static_cast<viskores::Id>(numSavedBranches));
    filter.SetPresimplifyThreshold(static_cast<viskores::Id>(presimplifyThreshold));
    return NewPyPartitionedDataSet(filter.Execute(*(*partitioned->DataSet)));
  }
  catch (const std::exception& error)
  {
    SetPythonError(error);
    return nullptr;
  }
}

PyObject* PyExtractTopVolumeContours(PyObject*, PyObject* args, PyObject* kwargs)
{
  static char* keywords[] = {
    const_cast<char*>("partitioned_dataset"),
    const_cast<char*>("marching_cubes"),
    const_cast<char*>("shift_isovalue_by_epsilon"),
    nullptr
  };
  PyObject* partitionedObject = nullptr;
  int marchingCubes = 0;
  int shiftIsovalueByEpsilon = 0;
  if (!PyArg_ParseTupleAndKeywords(args,
                                   kwargs,
                                   "O|pp:extract_top_volume_contours",
                                   keywords,
                                   &partitionedObject,
                                   &marchingCubes,
                                   &shiftIsovalueByEpsilon))
  {
    return nullptr;
  }
  PyPartitionedDataSetObject* partitioned = RequirePartitionedDataSet(partitionedObject);
  if (partitioned == nullptr)
  {
    return nullptr;
  }
  try
  {
    viskores::filter::scalar_topology::ExtractTopVolumeContoursFilter filter;
    filter.SetMarchingCubes(marchingCubes != 0);
    filter.SetShiftIsovalueByEpsilon(shiftIsovalueByEpsilon != 0);
    return NewPyPartitionedDataSet(filter.Execute(*(*partitioned->DataSet)));
  }
  catch (const std::exception& error)
  {
    SetPythonError(error);
    return nullptr;
  }
}

PyObject* PyFieldRange(PyObject*, PyObject* args, PyObject* kwargs)
{
  static char* keywords[] = {
    const_cast<char*>("dataset"),
    const_cast<char*>("field_name"),
    nullptr
  };
  PyObject* datasetObject = nullptr;
  const char* fieldName = nullptr;
  if (!PyArg_ParseTupleAndKeywords(
        args, kwargs, "Os:field_range", keywords, &datasetObject, &fieldName))
  {
    return nullptr;
  }

  PyDataSetObject* dataset = RequireDataSet(datasetObject);
  if (dataset == nullptr)
  {
    return nullptr;
  }

  try
  {
    const auto range = (*dataset->DataSet)->GetField(fieldName).GetRange().ReadPortal().Get(0);
    return Py_BuildValue("(dd)", range.Min, range.Max);
  }
  catch (const std::exception& error)
  {
    SetPythonError(error);
    return nullptr;
  }
}

PyObject* PyRenderDataSet(PyObject*, PyObject* args, PyObject* kwargs)
{
  static char* keywords[] = {
    const_cast<char*>("dataset"),
    const_cast<char*>("field_name"),
    const_cast<char*>("output_path"),
    const_cast<char*>("mapper"),
    const_cast<char*>("image_size"),
    const_cast<char*>("camera_look_at"),
    const_cast<char*>("camera_view_up"),
    const_cast<char*>("camera_position"),
    const_cast<char*>("camera_clipping_range"),
    const_cast<char*>("camera_field_of_view"),
    const_cast<char*>("background"),
    const_cast<char*>("color_table"),
    const_cast<char*>("scalar_range"),
    nullptr
  };
  PyObject* datasetObject = nullptr;
  const char* fieldName = nullptr;
  const char* outputPath = nullptr;
  const char* mapperName = "volume";
  PyObject* imageSizeObject = Py_None;
  PyObject* lookAtObject = Py_None;
  PyObject* viewUpObject = Py_None;
  PyObject* positionObject = Py_None;
  PyObject* clippingRangeObject = Py_None;
  PyObject* fieldOfViewObject = Py_None;
  PyObject* backgroundObject = Py_None;
  const char* colorTableName = "inferno";
  PyObject* scalarRangeObject = Py_None;
  if (!PyArg_ParseTupleAndKeywords(args,
                                   kwargs,
                                   "Oss|sOOOOOOOsO:render_dataset",
                                   keywords,
                                   &datasetObject,
                                   &fieldName,
                                   &outputPath,
                                   &mapperName,
                                   &imageSizeObject,
                                   &lookAtObject,
                                   &viewUpObject,
                                   &positionObject,
                                   &clippingRangeObject,
                                   &fieldOfViewObject,
                                   &backgroundObject,
                                   &colorTableName,
                                   &scalarRangeObject))
  {
    return nullptr;
  }

  PyDataSetObject* dataset = RequireDataSet(datasetObject);
  if (dataset == nullptr)
  {
    return nullptr;
  }

  try
  {
    const auto imageSize = ParseSize2D(imageSizeObject);
    viskores::rendering::Camera camera;
    camera.SetLookAt(ParseVec3(lookAtObject, viskores::make_Vec(0.5f, 0.5f, 0.5f)));
    camera.SetViewUp(ParseVec3(viewUpObject, viskores::make_Vec(0.f, 1.f, 0.f)));
    camera.SetPosition(ParseVec3(positionObject, viskores::make_Vec(1.5f, 1.5f, 1.5f)));
    const auto clippingRange = ParseRange(clippingRangeObject, viskores::Range(1.0, 10.0));
    camera.SetClippingRange(static_cast<viskores::Float32>(clippingRange.Min),
                            static_cast<viskores::Float32>(clippingRange.Max));
    if ((fieldOfViewObject != nullptr) && (fieldOfViewObject != Py_None))
    {
      camera.SetFieldOfView(static_cast<viskores::Float32>(PyFloat_AsDouble(fieldOfViewObject)));
      if (PyErr_Occurred())
      {
        throw std::runtime_error("camera_field_of_view must be numeric.");
      }
    }
    else
    {
      camera.SetFieldOfView(60.f);
    }

    const auto background =
      ParseColor(backgroundObject, viskores::rendering::Color(0.2f, 0.2f, 0.2f, 1.0f));
    viskores::cont::ColorTable colorTable(colorTableName);
    viskores::rendering::Actor actor(*(*dataset->DataSet), fieldName, colorTable);
    if ((scalarRangeObject != nullptr) && (scalarRangeObject != Py_None))
    {
      actor.SetScalarRange(ParseRange(scalarRangeObject));
    }

    viskores::rendering::Scene scene;
    scene.AddActor(actor);
    viskores::rendering::CanvasRayTracer canvas(imageSize[0], imageSize[1]);

    std::string mapper(mapperName);
    if ((mapper == "volume") || (mapper == "MapperVolume"))
    {
      viskores::rendering::View3D view(scene, viskores::rendering::MapperVolume(), canvas, camera, background);
      view.Paint();
      view.SaveAs(outputPath);
    }
    else if ((mapper == "wireframe") || (mapper == "wireframer") ||
             (mapper == "MapperWireframer"))
    {
      viskores::rendering::View3D view(
        scene, viskores::rendering::MapperWireframer(), canvas, camera, background);
      view.Paint();
      view.SaveAs(outputPath);
    }
    else if ((mapper == "raytracer") || (mapper == "MapperRayTracer"))
    {
      viskores::rendering::View3D view(
        scene, viskores::rendering::MapperRayTracer(), canvas, camera, background);
      view.Paint();
      view.SaveAs(outputPath);
    }
    else
    {
      throw std::runtime_error("mapper must be one of: volume, wireframe, raytracer.");
    }

    Py_RETURN_NONE;
  }
  catch (const std::exception& error)
  {
    SetPythonError(error);
    return nullptr;
  }
}

PyMethodDef ModuleMethods[] = {
  { "create_tangle_dataset",
    reinterpret_cast<PyCFunction>(PyCreateTangleDataSet),
    METH_VARARGS | METH_KEYWORDS,
    nullptr },
  { "create_uniform_dataset",
    reinterpret_cast<PyCFunction>(PyCreateUniformDataSet),
    METH_VARARGS | METH_KEYWORDS,
    nullptr },
  { "partition_uniform_dataset",
    reinterpret_cast<PyCFunction>(PyPartitionUniformDataSet),
    METH_VARARGS | METH_KEYWORDS,
    nullptr },
  { "cell_average",
    reinterpret_cast<PyCFunction>(PyCellAverage),
    METH_VARARGS | METH_KEYWORDS,
    nullptr },
  { "point_average",
    reinterpret_cast<PyCFunction>(PyPointAverage),
    METH_VARARGS | METH_KEYWORDS,
    nullptr },
  { "vector_magnitude",
    reinterpret_cast<PyCFunction>(PyVectorMagnitude),
    METH_VARARGS | METH_KEYWORDS,
    nullptr },
  { "gradient",
    reinterpret_cast<PyCFunction>(PyGradient),
    METH_VARARGS | METH_KEYWORDS,
    nullptr },
  { "contour",
    reinterpret_cast<PyCFunction>(PyContour),
    METH_VARARGS | METH_KEYWORDS,
    nullptr },
  { "contour_tree_augmented",
    reinterpret_cast<PyCFunction>(PyContourTreeAugmented),
    METH_VARARGS | METH_KEYWORDS,
    nullptr },
  { "contour_tree_distributed",
    reinterpret_cast<PyCFunction>(PyContourTreeDistributed),
    METH_VARARGS | METH_KEYWORDS,
    nullptr },
  { "distributed_branch_decomposition",
    reinterpret_cast<PyCFunction>(PyDistributedBranchDecomposition),
    METH_VARARGS,
    nullptr },
  { "select_top_volume_branches",
    reinterpret_cast<PyCFunction>(PySelectTopVolumeBranches),
    METH_VARARGS | METH_KEYWORDS,
    nullptr },
  { "extract_top_volume_contours",
    reinterpret_cast<PyCFunction>(PyExtractTopVolumeContours),
    METH_VARARGS | METH_KEYWORDS,
    nullptr },
  { "field_range",
    reinterpret_cast<PyCFunction>(PyFieldRange),
    METH_VARARGS | METH_KEYWORDS,
    nullptr },
  { "render_dataset",
    reinterpret_cast<PyCFunction>(PyRenderDataSet),
    METH_VARARGS | METH_KEYWORDS,
    nullptr },
  { nullptr, nullptr, 0, nullptr }
};

PyModuleDef ModuleDef = {
  PyModuleDef_HEAD_INIT,
  "_viskores",
  nullptr,
  -1,
  ModuleMethods,
  nullptr,
  nullptr,
  nullptr,
  nullptr
};

} // namespace

PyMODINIT_FUNC PyInit__viskores()
{
  import_array();

  PyDataSetType.tp_name = "viskores.DataSet";
  PyDataSetType.tp_basicsize = sizeof(PyDataSetObject);
  PyDataSetType.tp_dealloc = reinterpret_cast<destructor>(PyDataSetDealloc);
  PyDataSetType.tp_flags = Py_TPFLAGS_DEFAULT;
  PyDataSetType.tp_new = PyDataSetNew;
  PyDataSetType.tp_repr = reinterpret_cast<reprfunc>(PyDataSetRepr);
  PyDataSetType.tp_methods = PyDataSetMethods;
  PyDataSetType.tp_getset = PyDataSetGetSet;

  if (PyType_Ready(&PyDataSetType) < 0)
  {
    return nullptr;
  }

  PyPartitionedDataSetType.tp_name = "viskores.PartitionedDataSet";
  PyPartitionedDataSetType.tp_basicsize = sizeof(PyPartitionedDataSetObject);
  PyPartitionedDataSetType.tp_dealloc = reinterpret_cast<destructor>(PyPartitionedDataSetDealloc);
  PyPartitionedDataSetType.tp_flags = Py_TPFLAGS_DEFAULT;
  PyPartitionedDataSetType.tp_new = PyPartitionedDataSetNew;
  PyPartitionedDataSetType.tp_repr = reinterpret_cast<reprfunc>(PyPartitionedDataSetRepr);
  PyPartitionedDataSetType.tp_methods = PyPartitionedDataSetMethods;
  PyPartitionedDataSetType.tp_getset = PyPartitionedDataSetGetSet;

  if (PyType_Ready(&PyPartitionedDataSetType) < 0)
  {
    return nullptr;
  }

  PyObject* module = PyModule_Create(&ModuleDef);
  if (module == nullptr)
  {
    return nullptr;
  }

  viskores::cont::Initialize();

  Py_INCREF(&PyDataSetType);
  if (PyModule_AddObject(module, "DataSet", reinterpret_cast<PyObject*>(&PyDataSetType)) < 0)
  {
    Py_DECREF(&PyDataSetType);
    Py_DECREF(module);
    return nullptr;
  }

  Py_INCREF(&PyPartitionedDataSetType);
  if (PyModule_AddObject(
        module, "PartitionedDataSet", reinterpret_cast<PyObject*>(&PyPartitionedDataSetType)) < 0)
  {
    Py_DECREF(&PyPartitionedDataSetType);
    Py_DECREF(module);
    return nullptr;
  }

  PyModule_AddIntConstant(module, "ASSOCIATION_ANY", static_cast<int>(Association::Any));
  PyModule_AddIntConstant(module, "ASSOCIATION_WHOLE_DATASET", static_cast<int>(Association::WholeDataSet));
  PyModule_AddIntConstant(module, "ASSOCIATION_POINTS", static_cast<int>(Association::Points));
  PyModule_AddIntConstant(module, "ASSOCIATION_CELLS", static_cast<int>(Association::Cells));
  PyModule_AddIntConstant(module, "ASSOCIATION_PARTITIONS", static_cast<int>(Association::Partitions));
  PyModule_AddIntConstant(module, "ASSOCIATION_GLOBAL", static_cast<int>(Association::Global));

  return module;
}

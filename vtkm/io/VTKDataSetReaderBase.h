//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_io_VTKDataSetReaderBase_h
#define vtk_m_io_VTKDataSetReaderBase_h

#include <vtkm/Types.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/io/ErrorIO.h>
#include <vtkm/io/vtkm_io_export.h>

#include <vtkm/io/internal/Endian.h>
#include <vtkm/io/internal/VTKDataSetStructures.h>
#include <vtkm/io/internal/VTKDataSetTypes.h>

#include <fstream>

namespace vtkm
{
namespace io
{

namespace internal
{

struct VTKDataSetFile
{
  std::string FileName;
  vtkm::Id2 Version;
  std::string Title;
  bool IsBinary;
  vtkm::io::internal::DataSetStructure Structure;
  std::ifstream Stream;
};

inline void parseAssert(bool condition)
{
  if (!condition)
  {
    throw vtkm::io::ErrorIO("Parse Error");
  }
}

template <typename T>
struct StreamIOType
{
  using Type = T;
};
template <>
struct StreamIOType<vtkm::Int8>
{
  using Type = vtkm::Int16;
};
template <>
struct StreamIOType<vtkm::UInt8>
{
  using Type = vtkm::UInt16;
};

inline vtkm::cont::DynamicCellSet CreateCellSetStructured(const vtkm::Id3& dim)
{
  if (dim[0] > 1 && dim[1] > 1 && dim[2] > 1)
  {
    vtkm::cont::CellSetStructured<3> cs;
    cs.SetPointDimensions(vtkm::make_Vec(dim[0], dim[1], dim[2]));
    return cs;
  }
  else if (dim[0] > 1 && dim[1] > 1 && dim[2] <= 1)
  {
    vtkm::cont::CellSetStructured<2> cs;
    cs.SetPointDimensions(vtkm::make_Vec(dim[0], dim[1]));
    return cs;
  }
  else if (dim[0] > 1 && dim[1] <= 1 && dim[2] <= 1)
  {
    vtkm::cont::CellSetStructured<1> cs;
    cs.SetPointDimensions(dim[0]);
    return cs;
  }

  std::stringstream ss;
  ss << "Unsupported dimensions: (" << dim[0] << ", " << dim[1] << ", " << dim[2]
     << "), 2D structured datasets should be on X-Y plane and "
     << "1D structured datasets should be along X axis";
  throw vtkm::io::ErrorIO(ss.str());
}

} // namespace internal

class VTKM_IO_EXPORT VTKDataSetReaderBase
{
protected:
  std::unique_ptr<internal::VTKDataSetFile> DataFile;
  vtkm::cont::DataSet DataSet;

private:
  bool Loaded;
  vtkm::cont::ArrayHandle<vtkm::Id> CellsPermutation;

  friend class VTKDataSetReader;

public:
  explicit VTKM_CONT VTKDataSetReaderBase(const char* fileName);

  explicit VTKM_CONT VTKDataSetReaderBase(const std::string& fileName);

  virtual VTKM_CONT ~VTKDataSetReaderBase();

  VTKDataSetReaderBase(const VTKDataSetReaderBase&) = delete;
  void operator=(const VTKDataSetReaderBase&) = delete;

  const VTKM_CONT vtkm::cont::DataSet& ReadDataSet();

  const vtkm::cont::DataSet& GetDataSet() const { return this->DataSet; }

  virtual VTKM_CONT void PrintSummary(std::ostream& out) const;

protected:
  VTKM_CONT void ReadPoints();

  VTKM_CONT void ReadCells(vtkm::cont::ArrayHandle<vtkm::Id>& connectivity,
                           vtkm::cont::ArrayHandle<vtkm::IdComponent>& numIndices);

  VTKM_CONT void ReadShapes(vtkm::cont::ArrayHandle<vtkm::UInt8>& shapes);

  VTKM_CONT void ReadAttributes();

  void SetCellsPermutation(const vtkm::cont::ArrayHandle<vtkm::Id>& permutation)
  {
    this->CellsPermutation = permutation;
  }

  VTKM_CONT vtkm::cont::ArrayHandle<vtkm::Id> GetCellsPermutation() const
  {
    return this->CellsPermutation;
  }

  VTKM_CONT void TransferDataFile(VTKDataSetReaderBase& reader)
  {
    reader.DataFile.swap(this->DataFile);
    this->DataFile.reset(nullptr);
  }

  VTKM_CONT virtual void CloseFile();

  VTKM_CONT virtual void Read() = 0;

private:
  VTKM_CONT void OpenFile();
  VTKM_CONT void ReadHeader();
  VTKM_CONT void AddField(const std::string& name,
                          vtkm::cont::Field::Association association,
                          vtkm::cont::VariantArrayHandle& data);
  VTKM_CONT void ReadScalars(vtkm::cont::Field::Association association, std::size_t numElements);
  VTKM_CONT void ReadColorScalars(vtkm::cont::Field::Association association,
                                  std::size_t numElements);
  VTKM_CONT void ReadLookupTable();
  VTKM_CONT void ReadTextureCoordinates(vtkm::cont::Field::Association association,
                                        std::size_t numElements);
  VTKM_CONT void ReadVectors(vtkm::cont::Field::Association association, std::size_t numElements);
  VTKM_CONT void ReadTensors(vtkm::cont::Field::Association association, std::size_t numElements);
  VTKM_CONT void ReadFields(vtkm::cont::Field::Association association,
                            std::size_t expectedNumElements);

protected:
  VTKM_CONT void ReadGlobalFields(std::vector<vtkm::Float32>* visitBounds = nullptr);

private:
  class SkipArrayVariant;
  class ReadArrayVariant;

  //Make the Array parsing methods protected so that derived classes
  //can call the methods.
protected:
  VTKM_CONT void DoSkipArrayVariant(std::string dataType,
                                    std::size_t numElements,
                                    vtkm::IdComponent numComponents);
  VTKM_CONT vtkm::cont::VariantArrayHandle DoReadArrayVariant(
    vtkm::cont::Field::Association association,
    std::string dataType,
    std::size_t numElements,
    vtkm::IdComponent numComponents);

  template <typename T>
  VTKM_CONT void ReadArray(std::vector<T>& buffer)
  {
    using ComponentType = typename vtkm::VecTraits<T>::ComponentType;
    constexpr vtkm::IdComponent numComponents = vtkm::VecTraits<T>::NUM_COMPONENTS;

    std::size_t numElements = buffer.size();
    if (this->DataFile->IsBinary)
    {
      this->DataFile->Stream.read(reinterpret_cast<char*>(&buffer[0]),
                                  static_cast<std::streamsize>(numElements * sizeof(T)));
      if (vtkm::io::internal::IsLittleEndian())
      {
        vtkm::io::internal::FlipEndianness(buffer);
      }
    }
    else
    {
      for (std::size_t i = 0; i < numElements; ++i)
      {
        for (vtkm::IdComponent j = 0; j < numComponents; ++j)
        {
          typename internal::StreamIOType<ComponentType>::Type val;
          this->DataFile->Stream >> val;
          vtkm::VecTraits<T>::SetComponent(buffer[i], j, static_cast<ComponentType>(val));
        }
      }
    }
    this->DataFile->Stream >> std::ws;
    this->SkipArrayMetaData(numComponents);
  }

  template <vtkm::IdComponent NumComponents>
  VTKM_CONT void ReadArray(
    std::vector<vtkm::Vec<vtkm::io::internal::DummyBitType, NumComponents>>& buffer)
  {
    VTKM_LOG_S(vtkm::cont::LogLevel::Warn,
               "Support for data type 'bit' is not implemented. Skipping.");
    this->SkipArray(buffer.size(), vtkm::Vec<vtkm::io::internal::DummyBitType, NumComponents>());
    buffer.clear();
  }

  VTKM_CONT void ReadArray(std::vector<vtkm::io::internal::DummyBitType>& buffer);

  template <typename T>
  void SkipArray(std::size_t numElements, T)
  {
    using ComponentType = typename vtkm::VecTraits<T>::ComponentType;
    constexpr vtkm::IdComponent numComponents = vtkm::VecTraits<T>::NUM_COMPONENTS;

    if (this->DataFile->IsBinary)
    {
      this->DataFile->Stream.seekg(static_cast<std::streamoff>(numElements * sizeof(T)),
                                   std::ios_base::cur);
    }
    else
    {
      for (std::size_t i = 0; i < numElements; ++i)
      {
        for (vtkm::IdComponent j = 0; j < numComponents; ++j)
        {
          typename internal::StreamIOType<ComponentType>::Type val;
          this->DataFile->Stream >> val;
        }
      }
    }
    this->DataFile->Stream >> std::ws;
    this->SkipArrayMetaData(numComponents);
  }

  template <vtkm::IdComponent NumComponents>
  void SkipArray(std::size_t numElements,
                 vtkm::Vec<vtkm::io::internal::DummyBitType, NumComponents>)
  {
    this->SkipArray(numElements * static_cast<std::size_t>(NumComponents),
                    vtkm::io::internal::DummyBitType(),
                    NumComponents);
  }

  VTKM_CONT void SkipArray(std::size_t numElements,
                           vtkm::io::internal::DummyBitType,
                           vtkm::IdComponent numComponents = 1);

  VTKM_CONT void SkipArrayMetaData(vtkm::IdComponent numComponents);
};
}
} // vtkm::io

#endif // vtk_m_io_VTKDataSetReaderBase_h

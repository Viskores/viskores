//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_io_reader_VTKDataSetReaderBase_h
#define vtk_m_io_reader_VTKDataSetReaderBase_h

#include <vtkm/io/internal/Endian.h>
#include <vtkm/io/internal/VTKDataSetCells.h>
#include <vtkm/io/internal/VTKDataSetStructures.h>
#include <vtkm/io/internal/VTKDataSetTypes.h>

#include <vtkm/Types.h>
#include <vtkm/VecTraits.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DynamicArrayHandle.h>
#include <vtkm/internal/ExportMacros.h>
#include <vtkm/io/ErrorIO.h>

#include <algorithm>
#include <fstream>
#include <string>
#include <vector>

namespace vtkm
{
namespace io
{
namespace reader
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

inline void PrintVTKDataFileSummary(const VTKDataSetFile& df, std::ostream& out)
{
  out << "\tFile: " << df.FileName << std::endl;
  out << "\tVersion: " << df.Version[0] << "." << df.Version[0] << std::endl;
  out << "\tTitle: " << df.Title << std::endl;
  out << "\tFormat: " << (df.IsBinary ? "BINARY" : "ASCII") << std::endl;
  out << "\tDataSet type: " << vtkm::io::internal::DataSetStructureString(df.Structure)
      << std::endl;
}

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

// Since Fields and DataSets store data in the default DynamicArrayHandle, convert
// the data to the closest type supported by default. The following will
// need to be updated if DynamicArrayHandle or TypeListTagCommon changes.
template <typename T>
struct ClosestCommonType
{
  using Type = T;
};
template <>
struct ClosestCommonType<vtkm::Int8>
{
  using Type = vtkm::Int32;
};
template <>
struct ClosestCommonType<vtkm::UInt8>
{
  using Type = vtkm::Int32;
};
template <>
struct ClosestCommonType<vtkm::Int16>
{
  using Type = vtkm::Int32;
};
template <>
struct ClosestCommonType<vtkm::UInt16>
{
  using Type = vtkm::Int32;
};
template <>
struct ClosestCommonType<vtkm::UInt32>
{
  using Type = vtkm::Int64;
};
template <>
struct ClosestCommonType<vtkm::UInt64>
{
  using Type = vtkm::Int64;
};

template <typename T>
struct ClosestFloat
{
  using Type = T;
};
template <>
struct ClosestFloat<vtkm::Int8>
{
  using Type = vtkm::Float32;
};
template <>
struct ClosestFloat<vtkm::UInt8>
{
  using Type = vtkm::Float32;
};
template <>
struct ClosestFloat<vtkm::Int16>
{
  using Type = vtkm::Float32;
};
template <>
struct ClosestFloat<vtkm::UInt16>
{
  using Type = vtkm::Float32;
};
template <>
struct ClosestFloat<vtkm::Int32>
{
  using Type = vtkm::Float64;
};
template <>
struct ClosestFloat<vtkm::UInt32>
{
  using Type = vtkm::Float64;
};
template <>
struct ClosestFloat<vtkm::Int64>
{
  using Type = vtkm::Float64;
};
template <>
struct ClosestFloat<vtkm::UInt64>
{
  using Type = vtkm::Float64;
};

template <typename T>
vtkm::cont::DynamicArrayHandle CreateDynamicArrayHandle(const std::vector<T>& vec)
{
  switch (vtkm::VecTraits<T>::NUM_COMPONENTS)
  {
    case 1:
    {
      using CommonType = typename ClosestCommonType<T>::Type;
      constexpr bool not_same = !std::is_same<T, CommonType>::value;
      if (not_same)
      {
        std::cerr << "Type " << vtkm::io::internal::DataTypeName<T>::Name()
                  << " is currently unsupported. Converting to "
                  << vtkm::io::internal::DataTypeName<CommonType>::Name() << "." << std::endl;
      }

      vtkm::cont::ArrayHandle<CommonType> output;
      output.Allocate(static_cast<vtkm::Id>(vec.size()));
      for (vtkm::Id i = 0; i < output.GetNumberOfValues(); ++i)
      {
        output.GetPortalControl().Set(i, static_cast<CommonType>(vec[static_cast<std::size_t>(i)]));
      }

      return vtkm::cont::DynamicArrayHandle(output);
    }
    case 2:
    case 3:
    {
      using InComponentType = typename vtkm::VecTraits<T>::ComponentType;
      using OutComponentType = typename ClosestFloat<InComponentType>::Type;
      using CommonType = vtkm::Vec<OutComponentType, 3>;
      constexpr bool not_same = !std::is_same<T, CommonType>::value;
      if (not_same)
      {
        std::cerr << "Type " << vtkm::io::internal::DataTypeName<InComponentType>::Name() << "["
                  << vtkm::VecTraits<T>::NUM_COMPONENTS << "] "
                  << "is currently unsupported. Converting to "
                  << vtkm::io::internal::DataTypeName<OutComponentType>::Name() << "[3]."
                  << std::endl;
      }

      vtkm::cont::ArrayHandle<CommonType> output;
      output.Allocate(static_cast<vtkm::Id>(vec.size()));
      for (vtkm::Id i = 0; i < output.GetNumberOfValues(); ++i)
      {
        CommonType outval = CommonType();
        for (vtkm::IdComponent j = 0; j < vtkm::VecTraits<T>::NUM_COMPONENTS; ++j)
        {
          outval[j] = static_cast<OutComponentType>(
            vtkm::VecTraits<T>::GetComponent(vec[static_cast<std::size_t>(i)], j));
        }
        output.GetPortalControl().Set(i, outval);
      }

      return vtkm::cont::DynamicArrayHandle(output);
    }
    default:
    {
      std::cerr << "Only 1, 2, or 3 components supported. Skipping." << std::endl;
      return vtkm::cont::DynamicArrayHandle(vtkm::cont::ArrayHandle<vtkm::Float32>());
    }
  }
}

inline vtkm::cont::DynamicCellSet CreateCellSetStructured(const vtkm::Id3& dim)
{
  if (dim[0] > 1 && dim[1] > 1 && dim[2] > 1)
  {
    vtkm::cont::CellSetStructured<3> cs("cells");
    cs.SetPointDimensions(vtkm::make_Vec(dim[0], dim[1], dim[2]));
    return cs;
  }
  else if (dim[0] > 1 && dim[1] > 1 && dim[2] <= 1)
  {
    vtkm::cont::CellSetStructured<2> cs("cells");
    cs.SetPointDimensions(vtkm::make_Vec(dim[0], dim[1]));
    return cs;
  }
  else if (dim[0] > 1 && dim[1] <= 1 && dim[2] <= 1)
  {
    vtkm::cont::CellSetStructured<1> cs("cells");
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

VTKM_SILENCE_WEAK_VTABLE_WARNING_START

class VTKDataSetReaderBase
{
public:
  explicit VTKDataSetReaderBase(const char* fileName)
    : DataFile(new internal::VTKDataSetFile)
    , DataSet()
    , Loaded(false)
  {
    this->DataFile->FileName = fileName;
  }

  virtual ~VTKDataSetReaderBase() {}

  const vtkm::cont::DataSet& ReadDataSet()
  {
    if (!this->Loaded)
    {
      try
      {
        this->OpenFile();
        this->ReadHeader();
        this->Read();
        this->CloseFile();
        this->Loaded = true;
      }
      catch (std::ifstream::failure& e)
      {
        std::string message("IO Error: ");
        throw vtkm::io::ErrorIO(message + e.what());
      }
    }

    return this->DataSet;
  }

  const vtkm::cont::DataSet& GetDataSet() const { return this->DataSet; }

  virtual void PrintSummary(std::ostream& out) const
  {
    out << "VTKDataSetReader" << std::endl;
    PrintVTKDataFileSummary(*this->DataFile.get(), out);
    this->DataSet.PrintSummary(out);
  }

protected:
  void ReadPoints()
  {
    std::string dataType;
    std::size_t numPoints;
    this->DataFile->Stream >> numPoints >> dataType >> std::ws;

    vtkm::cont::DynamicArrayHandle points;
    this->DoReadDynamicArray(dataType, numPoints, 3, points);

    this->DataSet.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", points));
  }

  void ReadCells(vtkm::cont::ArrayHandle<vtkm::Id>& connectivity,
                 vtkm::cont::ArrayHandle<vtkm::IdComponent>& numIndices)
  {
    vtkm::Id numCells, numInts;
    this->DataFile->Stream >> numCells >> numInts >> std::ws;

    connectivity.Allocate(numInts - numCells);
    numIndices.Allocate(numCells);

    std::vector<vtkm::Int32> buffer(static_cast<std::size_t>(numInts));
    this->ReadArray(buffer);

    vtkm::Int32* buffp = &buffer[0];
    vtkm::cont::ArrayHandle<vtkm::Id>::PortalControl connectivityPortal =
      connectivity.GetPortalControl();
    vtkm::cont::ArrayHandle<vtkm::IdComponent>::PortalControl numIndicesPortal =
      numIndices.GetPortalControl();
    for (vtkm::Id i = 0, connInd = 0; i < numCells; ++i)
    {
      vtkm::IdComponent numInds = static_cast<vtkm::IdComponent>(*buffp++);
      numIndicesPortal.Set(i, numInds);
      for (vtkm::IdComponent j = 0; j < numInds; ++j, ++connInd)
      {
        connectivityPortal.Set(connInd, static_cast<vtkm::Id>(*buffp++));
      }
    }
  }

  void ReadShapes(vtkm::cont::ArrayHandle<vtkm::UInt8>& shapes)
  {
    std::string tag;
    vtkm::Id numCells;
    this->DataFile->Stream >> tag >> numCells >> std::ws;
    internal::parseAssert(tag == "CELL_TYPES");

    shapes.Allocate(numCells);
    std::vector<vtkm::Int32> buffer(static_cast<std::size_t>(numCells));
    this->ReadArray(buffer);

    vtkm::Int32* buffp = &buffer[0];
    vtkm::cont::ArrayHandle<vtkm::UInt8>::PortalControl shapesPortal = shapes.GetPortalControl();
    for (vtkm::Id i = 0; i < numCells; ++i)
    {
      shapesPortal.Set(i, static_cast<vtkm::UInt8>(*buffp++));
    }
  }

  void ReadAttributes()
  {
    if (this->DataFile->Stream.eof())
    {
      return;
    }

    vtkm::cont::Field::Association association = vtkm::cont::Field::Association::ANY;
    std::size_t size;

    std::string tag;
    this->DataFile->Stream >> tag;
    while (!this->DataFile->Stream.eof())
    {
      if (tag == "POINT_DATA")
      {
        association = vtkm::cont::Field::Association::POINTS;
      }
      else if (tag == "CELL_DATA")
      {
        association = vtkm::cont::Field::Association::CELL_SET;
      }
      else
      {
        internal::parseAssert(false);
      }

      this->DataFile->Stream >> size;
      while (!this->DataFile->Stream.eof())
      {
        std::string name;
        vtkm::cont::ArrayHandle<vtkm::Float32> empty;
        vtkm::cont::DynamicArrayHandle data(empty);

        this->DataFile->Stream >> tag;
        if (tag == "SCALARS")
        {
          this->ReadScalars(size, name, data);
        }
        else if (tag == "COLOR_SCALARS")
        {
          this->ReadColorScalars(size, name);
        }
        else if (tag == "LOOKUP_TABLE")
        {
          this->ReadLookupTable(name);
        }
        else if (tag == "VECTORS" || tag == "NORMALS")
        {
          this->ReadVectors(size, name, data);
        }
        else if (tag == "TEXTURE_COORDINATES")
        {
          this->ReadTextureCoordinates(size, name, data);
        }
        else if (tag == "TENSORS")
        {
          this->ReadTensors(size, name, data);
        }
        else if (tag == "FIELD")
        {
          this->ReadFields(name);
        }
        else
        {
          break;
        }

        if (data.GetNumberOfValues() > 0)
        {
          switch (association)
          {
            case vtkm::cont::Field::Association::POINTS:
              this->DataSet.AddField(vtkm::cont::Field(name, association, data));
              break;
            case vtkm::cont::Field::Association::CELL_SET:
              vtkm::cont::CastAndCall(data, PermuteCellData(this->CellsPermutation, data));
              this->DataSet.AddField(vtkm::cont::Field(name, association, "cells", data));
              break;
            default:
              break;
          }
        }
      }
    }
  }

  void SetCellsPermutation(const vtkm::cont::ArrayHandle<vtkm::Id>& permutation)
  {
    this->CellsPermutation = permutation;
  }

  void TransferDataFile(VTKDataSetReaderBase& reader)
  {
    reader.DataFile.swap(this->DataFile);
    this->DataFile.reset(nullptr);
  }

  virtual void CloseFile() { this->DataFile->Stream.close(); }

private:
  void OpenFile()
  {
    this->DataFile->Stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    this->DataFile->Stream.open(this->DataFile->FileName.c_str(),
                                std::ios_base::in | std::ios_base::binary);
  }

  void ReadHeader()
  {
    char vstring[] = "# vtk DataFile Version";
    const std::size_t vlen = sizeof(vstring);

    // Read version line
    char vbuf[vlen];
    this->DataFile->Stream.read(vbuf, vlen - 1);
    vbuf[vlen - 1] = '\0';
    if (std::string(vbuf) != std::string(vstring))
    {
      throw vtkm::io::ErrorIO("Incorrect file format.");
    }

    char dot;
    this->DataFile->Stream >> this->DataFile->Version[0] >> dot >> this->DataFile->Version[1];
    // skip rest of the line
    std::string skip;
    std::getline(this->DataFile->Stream, skip);

    // Read title line
    std::getline(this->DataFile->Stream, this->DataFile->Title);

    // Read format line
    this->DataFile->IsBinary = false;
    std::string format;
    this->DataFile->Stream >> format >> std::ws;
    if (format == "BINARY")
    {
      this->DataFile->IsBinary = true;
    }
    else if (format != "ASCII")
    {
      throw vtkm::io::ErrorIO("Unsupported Format.");
    }

    // Read structure line
    std::string tag, structStr;
    this->DataFile->Stream >> tag >> structStr >> std::ws;
    internal::parseAssert(tag == "DATASET");

    this->DataFile->Structure = vtkm::io::internal::DataSetStructureId(structStr);
    if (this->DataFile->Structure == vtkm::io::internal::DATASET_UNKNOWN)
    {
      throw vtkm::io::ErrorIO("Unsupported DataSet type.");
    }
  }

  virtual void Read() = 0;

  void ReadScalars(std::size_t numElements,
                   std::string& dataName,
                   vtkm::cont::DynamicArrayHandle& data)
  {
    std::string dataType, lookupTableName;
    vtkm::IdComponent numComponents = 1;
    this->DataFile->Stream >> dataName >> dataType;
    std::string tag;
    this->DataFile->Stream >> tag;
    if (tag != "LOOKUP_TABLE")
    {
      try
      {
        numComponents = std::stoi(tag);
      }
      catch (std::invalid_argument&)
      {
        internal::parseAssert(false);
      }
      this->DataFile->Stream >> tag;
    }

    internal::parseAssert(tag == "LOOKUP_TABLE");
    this->DataFile->Stream >> lookupTableName >> std::ws;

    this->DoReadDynamicArray(dataType, numElements, numComponents, data);
  }

  void ReadColorScalars(std::size_t numElements, std::string& dataName)
  {
    std::cerr << "Support for COLOR_SCALARS is not implemented. Skipping." << std::endl;

    std::size_t numValues;
    this->DataFile->Stream >> dataName >> numValues >> std::ws;
    this->SkipArray(numElements * numValues, vtkm::io::internal::ColorChannel8());
  }

  void ReadLookupTable(std::string& dataName)
  {
    std::cerr << "Support for LOOKUP_TABLE is not implemented. Skipping." << std::endl;

    std::size_t numEntries;
    this->DataFile->Stream >> dataName >> numEntries >> std::ws;
    this->SkipArray(numEntries, vtkm::Vec<vtkm::io::internal::ColorChannel8, 4>());
  }

  void ReadTextureCoordinates(std::size_t numElements,
                              std::string& dataName,
                              vtkm::cont::DynamicArrayHandle& data)
  {
    vtkm::IdComponent numComponents;
    std::string dataType;
    this->DataFile->Stream >> dataName >> numComponents >> dataType >> std::ws;

    this->DoReadDynamicArray(dataType, numElements, numComponents, data);
  }

  void ReadVectors(std::size_t numElements,
                   std::string& dataName,
                   vtkm::cont::DynamicArrayHandle& data)
  {
    std::string dataType;
    this->DataFile->Stream >> dataName >> dataType >> std::ws;

    this->DoReadDynamicArray(dataType, numElements, 3, data);
  }

  void ReadTensors(std::size_t numElements,
                   std::string& dataName,
                   vtkm::cont::DynamicArrayHandle& data)
  {
    std::string dataType;
    this->DataFile->Stream >> dataName >> dataType >> std::ws;

    this->DoReadDynamicArray(dataType, numElements, 9, data);
  }

protected:
  //ReadFields needs to be protected so that derived readers can skip
  //VisIt header fields
  void ReadFields(std::string& dataName, std::vector<vtkm::Float32>* visitBounds = nullptr)
  {
    std::cerr << "Support for FIELD is not implemented. Skipping." << std::endl;

    vtkm::Id numArrays;
    this->DataFile->Stream >> dataName >> numArrays >> std::ws;
    for (vtkm::Id i = 0; i < numArrays; ++i)
    {
      std::size_t numTuples;
      vtkm::IdComponent numComponents;
      std::string arrayName, dataType;
      this->DataFile->Stream >> arrayName >> numComponents >> numTuples >> dataType >> std::ws;
      if (arrayName == "avtOriginalBounds" && visitBounds)
      {
        visitBounds->resize(6);
        internal::parseAssert(numComponents == 1 && numTuples == 6);
        // parse the bounds and fill the bounds vector
        this->ReadArray(*visitBounds);
      }
      else
      {
        this->DoSkipDynamicArray(dataType, numTuples, numComponents);
      }
    }
  }

private:
  class SkipDynamicArray
  {
  public:
    SkipDynamicArray(VTKDataSetReaderBase* reader, std::size_t numElements)
      : Reader(reader)
      , NumElements(numElements)
    {
    }

    template <typename T>
    void operator()(T) const
    {
      this->Reader->SkipArray(this->NumElements, T());
    }

    template <typename T>
    void operator()(vtkm::IdComponent numComponents, T) const
    {
      this->Reader->SkipArray(this->NumElements * static_cast<std::size_t>(numComponents), T());
    }

  protected:
    VTKDataSetReaderBase* Reader;
    std::size_t NumElements;
  };

  class ReadDynamicArray : public SkipDynamicArray
  {
  public:
    ReadDynamicArray(VTKDataSetReaderBase* reader,
                     std::size_t numElements,
                     vtkm::cont::DynamicArrayHandle& data)
      : SkipDynamicArray(reader, numElements)
      , Data(&data)
    {
    }

    template <typename T>
    void operator()(T) const
    {
      std::vector<T> buffer(this->NumElements);
      this->Reader->ReadArray(buffer);
      *this->Data = internal::CreateDynamicArrayHandle(buffer);
    }

    template <typename T>
    void operator()(vtkm::IdComponent numComponents, T) const
    {
      std::cerr << "Support for " << numComponents << " components not implemented. Skipping."
                << std::endl;
      SkipDynamicArray::operator()(numComponents, T());
    }

  private:
    vtkm::cont::DynamicArrayHandle* Data;
  };

  //Make the Array parsing methods protected so that derived classes
  //can call the methods.
protected:
  void DoSkipDynamicArray(std::string dataType,
                          std::size_t numElements,
                          vtkm::IdComponent numComponents)
  {
    // string is unsupported for SkipDynamicArray, so it requires some
    // special handling
    if (dataType == "string")
    {
      const vtkm::Id stringCount = numComponents * static_cast<vtkm::Id>(numElements);
      for (vtkm::Id i = 0; i < stringCount; ++i)
      {
        std::string trash;
        this->DataFile->Stream >> trash;
      }
    }
    else
    {
      vtkm::io::internal::DataType typeId = vtkm::io::internal::DataTypeId(dataType);
      vtkm::io::internal::SelectTypeAndCall(
        typeId, numComponents, SkipDynamicArray(this, numElements));
    }
  }

  void DoReadDynamicArray(std::string dataType,
                          std::size_t numElements,
                          vtkm::IdComponent numComponents,
                          vtkm::cont::DynamicArrayHandle& data)
  {
    vtkm::io::internal::DataType typeId = vtkm::io::internal::DataTypeId(dataType);
    vtkm::io::internal::SelectTypeAndCall(
      typeId, numComponents, ReadDynamicArray(this, numElements, data));
  }

  template <typename T>
  void ReadArray(std::vector<T>& buffer)
  {
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
      using ComponentType = typename vtkm::VecTraits<T>::ComponentType;
      const vtkm::IdComponent numComponents = vtkm::VecTraits<T>::NUM_COMPONENTS;

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
  }

  template <vtkm::IdComponent NumComponents>
  void ReadArray(std::vector<vtkm::Vec<vtkm::io::internal::DummyBitType, NumComponents>>& buffer)
  {
    std::cerr << "Support for data type 'bit' is not implemented. Skipping." << std::endl;
    this->SkipArray(buffer.size(), vtkm::Vec<vtkm::io::internal::DummyBitType, NumComponents>());
    buffer.clear();
  }

  void ReadArray(std::vector<vtkm::io::internal::DummyBitType>& buffer)
  {
    std::cerr << "Support for data type 'bit' is not implemented. Skipping." << std::endl;
    this->SkipArray(buffer.size(), vtkm::io::internal::DummyBitType());
    buffer.clear();
  }

  template <typename T>
  void SkipArray(std::size_t numElements, T)
  {
    if (this->DataFile->IsBinary)
    {
      this->DataFile->Stream.seekg(static_cast<std::streamoff>(numElements * sizeof(T)),
                                   std::ios_base::cur);
    }
    else
    {
      using ComponentType = typename vtkm::VecTraits<T>::ComponentType;
      const vtkm::IdComponent numComponents = vtkm::VecTraits<T>::NUM_COMPONENTS;

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
  }

  template <vtkm::IdComponent NumComponents>
  void SkipArray(std::size_t numElements,
                 vtkm::Vec<vtkm::io::internal::DummyBitType, NumComponents>)
  {
    this->SkipArray(numElements * static_cast<std::size_t>(NumComponents),
                    vtkm::io::internal::DummyBitType());
  }

  void SkipArray(std::size_t numElements, vtkm::io::internal::DummyBitType)
  {
    if (this->DataFile->IsBinary)
    {
      numElements = (numElements + 7) / 8;
      this->DataFile->Stream.seekg(static_cast<std::streamoff>(numElements), std::ios_base::cur);
    }
    else
    {
      for (std::size_t i = 0; i < numElements; ++i)
      {
        vtkm::UInt16 val;
        this->DataFile->Stream >> val;
      }
    }
    this->DataFile->Stream >> std::ws;
  }

private:
  class PermuteCellData
  {
  public:
    PermuteCellData(const vtkm::cont::ArrayHandle<vtkm::Id>& permutation,
                    vtkm::cont::DynamicArrayHandle& data)
      : Permutation(permutation)
      , Data(&data)
    {
    }

    template <typename T>
    void operator()(const vtkm::cont::ArrayHandle<T>& handle) const
    {
      if (this->Permutation.GetNumberOfValues() < 1)
        return;
      vtkm::cont::ArrayHandle<T> out;
      out.Allocate(this->Permutation.GetNumberOfValues());

      vtkm::cont::ArrayHandle<vtkm::Id>::PortalConstControl permutationPortal =
        this->Permutation.GetPortalConstControl();
      typename vtkm::cont::ArrayHandle<T>::PortalConstControl inPortal =
        handle.GetPortalConstControl();
      typename vtkm::cont::ArrayHandle<T>::PortalControl outPortal = out.GetPortalControl();
      for (vtkm::Id i = 0; i < out.GetNumberOfValues(); ++i)
      {
        outPortal.Set(i, inPortal.Get(permutationPortal.Get(i)));
      }
      *this->Data = vtkm::cont::DynamicArrayHandle(out);
    }

  private:
    const vtkm::cont::ArrayHandle<vtkm::Id> Permutation;
    vtkm::cont::DynamicArrayHandle* Data;
  };

protected:
  std::unique_ptr<internal::VTKDataSetFile> DataFile;
  vtkm::cont::DataSet DataSet;

private:
  bool Loaded;
  vtkm::cont::ArrayHandle<vtkm::Id> CellsPermutation;

  friend class VTKDataSetReader;
};

VTKM_SILENCE_WEAK_VTABLE_WARNING_END
}
}
} // vtkm::io::reader

VTKM_BASIC_TYPE_VECTOR(vtkm::io::internal::ColorChannel8)
VTKM_BASIC_TYPE_VECTOR(vtkm::io::internal::DummyBitType)

#endif // vtk_m_io_reader_VTKDataSetReaderBase_h

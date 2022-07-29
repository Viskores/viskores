//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/Field.h>

#include <vtkm/cont/Invoker.h>
#include <vtkm/cont/Logging.h>

#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/TypeList.h>

#include <vtkm/cont/ArrayRangeCompute.h>

namespace vtkm
{
namespace cont
{

/// constructors for points / whole mesh
VTKM_CONT
Field::Field(std::string name, Association association, const vtkm::cont::UnknownArrayHandle& data)
  : Name(name)
  , FieldAssociation(association)
  , Data(data)
  , Range()
  , ModifiedFlag(true)
{
}

VTKM_CONT
Field::Field(const vtkm::cont::Field& src)
  : Name(src.Name)
  , FieldAssociation(src.FieldAssociation)
  , Data(src.Data)
  , Range(src.Range)
  , ModifiedFlag(src.ModifiedFlag)
{
}

VTKM_CONT
Field::Field(vtkm::cont::Field&& src) noexcept
  : Name(std::move(src.Name))
  , FieldAssociation(std::move(src.FieldAssociation))
  , Data(std::move(src.Data))
  , Range(std::move(src.Range))
  , ModifiedFlag(std::move(src.ModifiedFlag))
{
}

VTKM_CONT
Field& Field::operator=(const vtkm::cont::Field& src)
{
  this->Name = src.Name;
  this->FieldAssociation = src.FieldAssociation;
  this->Data = src.Data;
  this->Range = src.Range;
  this->ModifiedFlag = src.ModifiedFlag;
  return *this;
}

VTKM_CONT
Field& Field::operator=(vtkm::cont::Field&& src) noexcept
{
  this->Name = std::move(src.Name);
  this->FieldAssociation = std::move(src.FieldAssociation);
  this->Data = std::move(src.Data);
  this->Range = std::move(src.Range);
  this->ModifiedFlag = std::move(src.ModifiedFlag);
  return *this;
}


VTKM_CONT
void Field::PrintSummary(std::ostream& out) const
{
  out << "   " << this->Name;
  out << " assoc= ";
  switch (this->GetAssociation())
  {
    case Association::Any:
      out << "Any ";
      break;
    case Association::WholeMesh:
      out << "Mesh ";
      break;
    case Association::Points:
      out << "Points ";
      break;
    case Association::Cells:
      out << "Cells ";
      break;
  }
  this->Data.PrintSummary(out);
}

VTKM_CONT
Field::~Field() {}


VTKM_CONT
const vtkm::cont::UnknownArrayHandle& Field::GetData() const
{
  return this->Data;
}

VTKM_CONT
vtkm::cont::UnknownArrayHandle& Field::GetData()
{
  this->ModifiedFlag = true;
  return this->Data;
}

VTKM_CONT const vtkm::cont::ArrayHandle<vtkm::Range>& Field::GetRange() const
{
  VTKM_LOG_SCOPE(vtkm::cont::LogLevel::Perf, "Field::GetRange");

  if (this->ModifiedFlag)
  {
    this->Range = vtkm::cont::ArrayRangeCompute(this->Data);
    this->ModifiedFlag = false;
  }

  return this->Range;
}

VTKM_CONT void Field::GetRange(vtkm::Range* range) const
{
  this->GetRange();
  const vtkm::Id length = this->Range.GetNumberOfValues();
  auto portal = this->Range.ReadPortal();
  for (vtkm::Id i = 0; i < length; ++i)
  {
    range[i] = portal.Get(i);
  }
}

VTKM_CONT void Field::SetData(const vtkm::cont::UnknownArrayHandle& newdata)
{
  this->Data = newdata;
  this->ModifiedFlag = true;
}

namespace
{

struct CheckArrayType
{
  template <typename T, typename S>
  void operator()(vtkm::List<T, S>, const vtkm::cont::UnknownArrayHandle& data, bool& found) const
  {
    if (data.CanConvert<vtkm::cont::ArrayHandle<T, S>>())
    {
      found = true;
    }
  }
};

} // anonymous namespace

bool Field::IsSupportedType() const
{
  bool found = false;
  vtkm::ListForEach(
    CheckArrayType{},
    vtkm::cont::internal::ListAllArrayTypes<VTKM_DEFAULT_TYPE_LIST, VTKM_DEFAULT_STORAGE_LIST>{},
    this->Data,
    found);
  return found;
}

namespace
{

struct CheckStorageType
{
  template <typename S>
  void operator()(S, const vtkm::cont::UnknownArrayHandle& data, bool& found) const
  {
    if (data.IsStorageType<S>())
    {
      found = true;
    }
  }
};

// This worklet is used in lieu of ArrayCopy because the use of ArrayHandleRecombineVec
// can throw off the casting in implementations of ArrayCopy.
struct CopyWorklet : vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);

  template <typename InType, typename OutType>
  VTKM_EXEC_CONT void operator()(const InType& in, OutType& out) const
  {
    VTKM_ASSERT(in.GetNumberOfComponents() == out.GetNumberOfComponents());
    for (vtkm::IdComponent cIndex = 0; cIndex < in.GetNumberOfComponents(); ++cIndex)
    {
      out[cIndex] = static_cast<vtkm::FloatDefault>(in[cIndex]);
    }
  }
};

struct CopyToFloatArray
{
  template <typename ArrayType>
  void operator()(const ArrayType& inArray, vtkm::cont::UnknownArrayHandle& outArray) const
  {
    vtkm::cont::Invoker invoke;
    invoke(CopyWorklet{}, inArray, outArray.ExtractArrayFromComponents<vtkm::FloatDefault>());
  }
};

} // anonymous namespace

vtkm::cont::UnknownArrayHandle Field::GetDataAsDefaultFloat() const
{
  if (this->Data.IsBaseComponentType<vtkm::FloatDefault>())
  {
    bool supportedStorage = false;
    vtkm::ListForEach(
      CheckStorageType{}, VTKM_DEFAULT_STORAGE_LIST{}, this->Data, supportedStorage);
    if (supportedStorage)
    {
      // Array is already float default and supported storage. No better conversion can be done.
      return this->Data;
    }
  }

  VTKM_LOG_SCOPE(vtkm::cont::LogLevel::Info,
                 "Converting field '%s' to default floating point.",
                 this->GetName().c_str());
  vtkm::cont::UnknownArrayHandle outArray = this->Data.NewInstanceFloatBasic();
  outArray.Allocate(this->Data.GetNumberOfValues());
  this->Data.CastAndCallWithExtractedArray(CopyToFloatArray{}, outArray);
  return outArray;
}

vtkm::cont::UnknownArrayHandle Field::GetDataWithExpectedTypes() const
{
  if (this->IsSupportedType())
  {
    return this->Data;
  }
  else
  {
    return this->GetDataAsDefaultFloat();
  }
}

void Field::ConvertToExpected()
{
  this->SetData(this->GetDataWithExpectedTypes());
}

}
} // namespace vtkm::cont

namespace mangled_diy_namespace
{

void Serialization<vtkm::cont::Field>::save(BinaryBuffer& bb, const vtkm::cont::Field& field)
{
  vtkmdiy::save(bb, field.GetName());
  vtkmdiy::save(bb, static_cast<int>(field.GetAssociation()));
  vtkmdiy::save(bb, field.GetData());
}

void Serialization<vtkm::cont::Field>::load(BinaryBuffer& bb, vtkm::cont::Field& field)
{
  std::string name;
  vtkmdiy::load(bb, name);
  int assocVal = 0;
  vtkmdiy::load(bb, assocVal);

  auto assoc = static_cast<vtkm::cont::Field::Association>(assocVal);
  vtkm::cont::UnknownArrayHandle data;
  vtkmdiy::load(bb, data);
  field = vtkm::cont::Field(name, assoc, data);
}


} // namespace diy

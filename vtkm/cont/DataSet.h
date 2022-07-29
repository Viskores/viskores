//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_DataSet_h
#define vtk_m_cont_DataSet_h

#include <vtkm/cont/vtkm_cont_export.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/ErrorBadValue.h>
#include <vtkm/cont/Field.h>
#include <vtkm/cont/UnknownArrayHandle.h>
#include <vtkm/cont/UnknownCellSet.h>

namespace vtkm
{
namespace cont
{

class VTKM_CONT_EXPORT DataSet
{
public:
  VTKM_CONT void Clear();

  /// Get the number of cells contained in this DataSet
  VTKM_CONT vtkm::Id GetNumberOfCells() const;

  /// Get the number of points contained in this DataSet
  ///
  /// Note: All coordinate systems for a DataSet are expected
  /// to have the same number of points.
  VTKM_CONT vtkm::Id GetNumberOfPoints() const;

  VTKM_CONT void AddField(const Field& field);

  VTKM_CONT
  const vtkm::cont::Field& GetField(vtkm::Id index) const;

  VTKM_CONT
  vtkm::cont::Field& GetField(vtkm::Id index);

  VTKM_CONT
  bool HasField(const std::string& name,
                vtkm::cont::Field::Association assoc = vtkm::cont::Field::Association::Any) const
  {
    return (this->GetFieldIndex(name, assoc) != -1);
  }

  VTKM_CONT
  bool HasCellField(const std::string& name) const
  {
    return (this->GetFieldIndex(name, vtkm::cont::Field::Association::Cells) != -1);
  }

  VTKM_CONT
  bool HasPointField(const std::string& name) const
  {
    return (this->GetFieldIndex(name, vtkm::cont::Field::Association::Points) != -1);
  }


  /// Returns the field that matches the provided name and association
  /// Will return -1 if no match is found
  VTKM_CONT
  vtkm::Id GetFieldIndex(
    const std::string& name,
    vtkm::cont::Field::Association assoc = vtkm::cont::Field::Association::Any) const;

  /// Returns the field that matches the provided name and association
  /// Will throw an exception if no match is found
  //@{
  VTKM_CONT
  const vtkm::cont::Field& GetField(
    const std::string& name,
    vtkm::cont::Field::Association assoc = vtkm::cont::Field::Association::Any) const;

  VTKM_CONT
  vtkm::cont::Field& GetField(
    const std::string& name,
    vtkm::cont::Field::Association assoc = vtkm::cont::Field::Association::Any);
  //@}

  /// Returns the first cell field that matches the provided name.
  /// Will throw an exception if no match is found
  //@{
  VTKM_CONT
  const vtkm::cont::Field& GetCellField(const std::string& name) const
  {
    return this->GetField(name, vtkm::cont::Field::Association::Cells);
  }

  VTKM_CONT
  vtkm::cont::Field& GetCellField(const std::string& name)
  {
    return this->GetField(name, vtkm::cont::Field::Association::Cells);
  }
  //@}

  /// Returns the first point field that matches the provided name.
  /// Will throw an exception if no match is found
  //@{
  VTKM_CONT
  const vtkm::cont::Field& GetPointField(const std::string& name) const
  {
    return this->GetField(name, vtkm::cont::Field::Association::Points);
  }

  VTKM_CONT
  vtkm::cont::Field& GetPointField(const std::string& name)
  {
    return this->GetField(name, vtkm::cont::Field::Association::Points);
  }
  //@}

  VTKM_CONT
  void AddPointField(const std::string& fieldName, const vtkm::cont::UnknownArrayHandle& field)
  {
    this->AddField(make_FieldPoint(fieldName, field));
  }

  template <typename T, typename Storage>
  VTKM_CONT void AddPointField(const std::string& fieldName,
                               const vtkm::cont::ArrayHandle<T, Storage>& field)
  {
    this->AddField(make_FieldPoint(fieldName, field));
  }

  template <typename T>
  VTKM_CONT void AddPointField(const std::string& fieldName, const std::vector<T>& field)
  {
    this->AddField(
      make_Field(fieldName, vtkm::cont::Field::Association::Points, field, vtkm::CopyFlag::On));
  }

  template <typename T>
  VTKM_CONT void AddPointField(const std::string& fieldName, const T* field, const vtkm::Id& n)
  {
    this->AddField(
      make_Field(fieldName, vtkm::cont::Field::Association::Points, field, n, vtkm::CopyFlag::On));
  }

  //Cell centered field
  VTKM_CONT
  void AddCellField(const std::string& fieldName, const vtkm::cont::UnknownArrayHandle& field)
  {
    this->AddField(make_FieldCell(fieldName, field));
  }

  template <typename T, typename Storage>
  VTKM_CONT void AddCellField(const std::string& fieldName,
                              const vtkm::cont::ArrayHandle<T, Storage>& field)
  {
    this->AddField(make_FieldCell(fieldName, field));
  }

  template <typename T>
  VTKM_CONT void AddCellField(const std::string& fieldName, const std::vector<T>& field)
  {
    this->AddField(
      make_Field(fieldName, vtkm::cont::Field::Association::Cells, field, vtkm::CopyFlag::On));
  }

  template <typename T>
  VTKM_CONT void AddCellField(const std::string& fieldName, const T* field, const vtkm::Id& n)
  {
    this->AddField(
      make_Field(fieldName, vtkm::cont::Field::Association::Cells, field, n, vtkm::CopyFlag::On));
  }


  VTKM_CONT
  void AddCoordinateSystem(const vtkm::cont::CoordinateSystem& cs)
  {
    this->CoordSystems.push_back(cs);
  }

  VTKM_CONT
  bool HasCoordinateSystem(const std::string& name) const
  {
    return this->GetCoordinateSystemIndex(name) >= 0;
  }

  VTKM_CONT
  const vtkm::cont::CoordinateSystem& GetCoordinateSystem(vtkm::Id index = 0) const;

  VTKM_CONT
  vtkm::cont::CoordinateSystem& GetCoordinateSystem(vtkm::Id index = 0);

  /// Returns the index for the first CoordinateSystem whose
  /// name matches the provided string.
  /// Will return -1 if no match is found
  VTKM_CONT
  vtkm::Id GetCoordinateSystemIndex(const std::string& name) const;

  /// Returns the first CoordinateSystem that matches the provided name.
  /// Will throw an exception if no match is found
  //@{
  VTKM_CONT
  const vtkm::cont::CoordinateSystem& GetCoordinateSystem(const std::string& name) const;

  VTKM_CONT
  vtkm::cont::CoordinateSystem& GetCoordinateSystem(const std::string& name);
  //@}

  /// Returns an `std::vector` of `CoordinateSystem`s held in this `DataSet`.
  ///
  VTKM_CONT
  std::vector<vtkm::cont::CoordinateSystem> GetCoordinateSystems() const
  {
    return this->CoordSystems;
  }

  template <typename CellSetType>
  VTKM_CONT void SetCellSet(const CellSetType& cellSet)
  {
    VTKM_IS_KNOWN_OR_UNKNOWN_CELL_SET(CellSetType);
    this->CellSet = vtkm::cont::UnknownCellSet(cellSet);
  }

  VTKM_CONT
  const vtkm::cont::UnknownCellSet& GetCellSet() const { return this->CellSet; }

  VTKM_CONT
  vtkm::cont::UnknownCellSet& GetCellSet() { return this->CellSet; }

  VTKM_CONT
  vtkm::IdComponent GetNumberOfFields() const
  {
    return static_cast<vtkm::IdComponent>(this->Fields.size());
  }

  VTKM_CONT
  vtkm::IdComponent GetNumberOfCoordinateSystems() const
  {
    return static_cast<vtkm::IdComponent>(this->CoordSystems.size());
  }

  /// Copies the structure i.e. coordinates systems and cellset from the source
  /// dataset. The fields are left unchanged.
  VTKM_CONT
  void CopyStructure(const vtkm::cont::DataSet& source);

  /// \brief Convert the structures in this data set to expected types.
  ///
  /// A `DataSet` object can contain data structures of unknown types. Using the data
  /// requires casting these data structures to concrete types. It is only possible to
  /// check a finite number of data structures.
  ///
  /// The types checked by default are listed in `vtkm/cont/DefaultTypes.h`, which can
  /// be configured at compile time. If a `DataSet` contains data not listed there, then
  /// it is likely going to cause problems pulling the data back out. To get around this
  /// problem, you can call this method to convert the data to a form that is likely to
  /// be recognized. This conversion is likely but not guaranteed because not all types
  /// are convertable to something recognizable.
  ///
  VTKM_CONT void ConvertToExpected();

  VTKM_CONT
  void PrintSummary(std::ostream& out) const;

private:
  struct FieldCompare
  {
    using Key = std::pair<std::string, vtkm::cont::Field::Association>;

    template <typename T>
    bool operator()(const T& a, const T& b) const
    {
      if (a.first == b.first)
        return a.second < b.second && a.second != vtkm::cont::Field::Association::Any &&
          b.second != vtkm::cont::Field::Association::Any;

      return a.first < b.first;
    }
  };

  std::vector<vtkm::cont::CoordinateSystem> CoordSystems;
  std::map<FieldCompare::Key, vtkm::cont::Field, FieldCompare> Fields;
  vtkm::cont::UnknownCellSet CellSet;
};

} // namespace cont
} // namespace vtkm

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace vtkm
{
namespace cont
{

template <typename FieldTypeList = VTKM_DEFAULT_TYPE_LIST,
          typename CellSetTypesList = VTKM_DEFAULT_CELL_SET_LIST>
struct SerializableDataSet
{
  SerializableDataSet() = default;

  explicit SerializableDataSet(const vtkm::cont::DataSet& dataset)
    : DataSet(dataset)
  {
  }

  vtkm::cont::DataSet DataSet;
};
}
} // vtkm::cont

namespace mangled_diy_namespace
{

template <typename FieldTypeList, typename CellSetTypesList>
struct Serialization<vtkm::cont::SerializableDataSet<FieldTypeList, CellSetTypesList>>
{
private:
  using Type = vtkm::cont::SerializableDataSet<FieldTypeList, CellSetTypesList>;

public:
  static VTKM_CONT void save(BinaryBuffer& bb, const Type& serializable)
  {
    const auto& dataset = serializable.DataSet;

    vtkm::IdComponent numberOfCoordinateSystems = dataset.GetNumberOfCoordinateSystems();
    vtkmdiy::save(bb, numberOfCoordinateSystems);
    for (vtkm::IdComponent i = 0; i < numberOfCoordinateSystems; ++i)
    {
      vtkmdiy::save(bb, dataset.GetCoordinateSystem(i));
    }

    vtkmdiy::save(bb, dataset.GetCellSet().ResetCellSetList(CellSetTypesList{}));

    vtkm::IdComponent numberOfFields = dataset.GetNumberOfFields();
    vtkmdiy::save(bb, numberOfFields);
    for (vtkm::IdComponent i = 0; i < numberOfFields; ++i)
    {
      vtkmdiy::save(bb, dataset.GetField(i));
    }
  }

  static VTKM_CONT void load(BinaryBuffer& bb, Type& serializable)
  {
    auto& dataset = serializable.DataSet;
    dataset = {}; // clear

    vtkm::IdComponent numberOfCoordinateSystems = 0;
    vtkmdiy::load(bb, numberOfCoordinateSystems);
    for (vtkm::IdComponent i = 0; i < numberOfCoordinateSystems; ++i)
    {
      vtkm::cont::CoordinateSystem coords;
      vtkmdiy::load(bb, coords);
      dataset.AddCoordinateSystem(coords);
    }

    vtkm::cont::UncertainCellSet<CellSetTypesList> cells;
    vtkmdiy::load(bb, cells);
    dataset.SetCellSet(cells);

    vtkm::IdComponent numberOfFields = 0;
    vtkmdiy::load(bb, numberOfFields);
    for (vtkm::IdComponent i = 0; i < numberOfFields; ++i)
    {
      vtkm::cont::Field field;
      vtkmdiy::load(bb, field);
      dataset.AddField(field);
    }
  }
};

} // diy
/// @endcond SERIALIZATION

#endif //vtk_m_cont_DataSet_h

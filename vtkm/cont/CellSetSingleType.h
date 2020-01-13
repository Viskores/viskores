//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_CellSetSingleType_h
#define vtk_m_cont_CellSetSingleType_h

#include <vtkm/CellShape.h>
#include <vtkm/CellTraits.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/CellSet.h>
#include <vtkm/cont/CellSetExplicit.h>

#include <map>
#include <utility>

namespace vtkm
{
namespace cont
{

//Only works with fixed sized cell sets

template <typename ConnectivityStorageTag = VTKM_DEFAULT_CONNECTIVITY_STORAGE_TAG>
class VTKM_ALWAYS_EXPORT CellSetSingleType
  : public vtkm::cont::CellSetExplicit<
      typename vtkm::cont::ArrayHandleConstant<vtkm::UInt8>::StorageTag, //ShapesStorageTag
      ConnectivityStorageTag,
      typename vtkm::cont::ArrayHandleCounting<vtkm::Id>::StorageTag //OffsetsStorageTag
      >
{
  using Thisclass = vtkm::cont::CellSetSingleType<ConnectivityStorageTag>;
  using Superclass =
    vtkm::cont::CellSetExplicit<typename vtkm::cont::ArrayHandleConstant<vtkm::UInt8>::StorageTag,
                                ConnectivityStorageTag,
                                typename vtkm::cont::ArrayHandleCounting<vtkm::Id>::StorageTag>;

public:
  VTKM_CONT
  CellSetSingleType()
    : Superclass()
    , ExpectedNumberOfCellsAdded(-1)
    , CellShapeAsId(CellShapeTagEmpty::Id)
    , NumberOfPointsPerCell(0)
  {
  }

  VTKM_CONT
  CellSetSingleType(const Thisclass& src)
    : Superclass(src)
    , ExpectedNumberOfCellsAdded(-1)
    , CellShapeAsId(src.CellShapeAsId)
    , NumberOfPointsPerCell(src.NumberOfPointsPerCell)
  {
  }

  VTKM_CONT
  CellSetSingleType(Thisclass&& src) noexcept : Superclass(std::forward<Superclass>(src)),
                                                ExpectedNumberOfCellsAdded(-1),
                                                CellShapeAsId(src.CellShapeAsId),
                                                NumberOfPointsPerCell(src.NumberOfPointsPerCell)
  {
  }


  VTKM_CONT
  Thisclass& operator=(const Thisclass& src)
  {
    this->Superclass::operator=(src);
    this->CellShapeAsId = src.CellShapeAsId;
    this->NumberOfPointsPerCell = src.NumberOfPointsPerCell;
    return *this;
  }

  VTKM_CONT
  Thisclass& operator=(Thisclass&& src) noexcept
  {
    this->Superclass::operator=(std::forward<Superclass>(src));
    this->CellShapeAsId = src.CellShapeAsId;
    this->NumberOfPointsPerCell = src.NumberOfPointsPerCell;
    return *this;
  }

  virtual ~CellSetSingleType() override {}

  /// First method to add cells -- one at a time.
  VTKM_CONT
  void PrepareToAddCells(vtkm::Id numCells, vtkm::Id connectivityMaxLen)
  {
    this->CellShapeAsId = vtkm::CELL_SHAPE_EMPTY;

    this->Data->CellPointIds.Connectivity.Allocate(connectivityMaxLen);

    this->Data->NumberOfCellsAdded = 0;
    this->Data->ConnectivityAdded = 0;
    this->ExpectedNumberOfCellsAdded = numCells;
  }

  /// Second method to add cells -- one at a time.
  template <typename IdVecType>
  VTKM_CONT void AddCell(vtkm::UInt8 shapeId, vtkm::IdComponent numVertices, const IdVecType& ids)
  {
    using Traits = vtkm::VecTraits<IdVecType>;
    VTKM_STATIC_ASSERT_MSG((std::is_same<typename Traits::ComponentType, vtkm::Id>::value),
                           "CellSetSingleType::AddCell requires vtkm::Id for indices.");

    if (Traits::GetNumberOfComponents(ids) < numVertices)
    {
      throw vtkm::cont::ErrorBadValue("Not enough indices given to CellSetSingleType::AddCell.");
    }

    if (this->Data->ConnectivityAdded + numVertices >
        this->Data->CellPointIds.Connectivity.GetNumberOfValues())
    {
      throw vtkm::cont::ErrorBadValue(
        "Connectivity increased past estimated maximum connectivity.");
    }

    if (this->CellShapeAsId == vtkm::CELL_SHAPE_EMPTY)
    {
      if (shapeId == vtkm::CELL_SHAPE_EMPTY)
      {
        throw vtkm::cont::ErrorBadValue("Cannot create cells of type empty.");
      }
      this->CellShapeAsId = shapeId;
      this->CheckNumberOfPointsPerCell(numVertices);
      this->NumberOfPointsPerCell = numVertices;
    }
    else
    {
      if (shapeId != this->GetCellShape(0))
      {
        throw vtkm::cont::ErrorBadValue("Cannot have differing shapes in CellSetSingleType.");
      }
      if (numVertices != this->NumberOfPointsPerCell)
      {
        throw vtkm::cont::ErrorBadValue(
          "Inconsistent number of points in cells for CellSetSingleType.");
      }
    }
    auto conn = this->Data->CellPointIds.Connectivity.GetPortalControl();
    for (vtkm::IdComponent iVert = 0; iVert < numVertices; ++iVert)
    {
      conn.Set(this->Data->ConnectivityAdded + iVert, Traits::GetComponent(ids, iVert));
    }
    this->Data->NumberOfCellsAdded++;
    this->Data->ConnectivityAdded += numVertices;
  }

  /// Third and final method to add cells -- one at a time.
  VTKM_CONT
  void CompleteAddingCells(vtkm::Id numPoints)
  {
    this->Data->NumberOfPoints = numPoints;
    this->CellPointIds.Connectivity.Shrink(this->ConnectivityAdded);

    vtkm::Id numCells = this->NumberOfCellsAdded;

    this->CellPointIds.Shapes =
      vtkm::cont::make_ArrayHandleConstant(this->GetCellShape(0), numCells);
    this->CellPointIds.IndexOffsets = vtkm::cont::make_ArrayHandleCounting(
      vtkm::Id(0), static_cast<vtkm::Id>(this->NumberOfPointsPerCell), numCells);

    this->CellPointIds.ElementsValid = true;

    if (this->ExpectedNumberOfCellsAdded != this->GetNumberOfCells())
    {
      throw vtkm::cont::ErrorBadValue("Did not add the expected number of cells.");
    }

    this->NumberOfCellsAdded = -1;
    this->ConnectivityAdded = -1;
    this->ExpectedNumberOfCellsAdded = -1;
  }

  //This is the way you can fill the memory from another system without copying
  VTKM_CONT
  void Fill(vtkm::Id numPoints,
            vtkm::UInt8 shapeId,
            vtkm::IdComponent numberOfPointsPerCell,
            const vtkm::cont::ArrayHandle<vtkm::Id, ConnectivityStorageTag>& connectivity)
  {
    this->Data->NumberOfPoints = numPoints;
    this->CellShapeAsId = shapeId;
    this->CheckNumberOfPointsPerCell(numberOfPointsPerCell);

    const vtkm::Id numCells = connectivity.GetNumberOfValues() / numberOfPointsPerCell;
    VTKM_ASSERT((connectivity.GetNumberOfValues() % numberOfPointsPerCell) == 0);

    this->Data->CellPointIds.Shapes = vtkm::cont::make_ArrayHandleConstant(shapeId, numCells);

    this->Data->CellPointIds.Offsets = vtkm::cont::make_ArrayHandleCounting(
      vtkm::Id(0), static_cast<vtkm::Id>(numberOfPointsPerCell), numCells + 1);

    this->Data->CellPointIds.Connectivity = connectivity;

    this->Data->CellPointIds.ElementsValid = true;

    this->ResetConnectivity(TopologyElementTagPoint{}, TopologyElementTagCell{});
  }

  VTKM_CONT
  vtkm::Id GetCellShapeAsId() const { return this->CellShapeAsId; }

  VTKM_CONT
  vtkm::UInt8 GetCellShape(vtkm::Id vtkmNotUsed(cellIndex)) const override
  {
    return static_cast<vtkm::UInt8>(this->CellShapeAsId);
  }

  VTKM_CONT
  std::shared_ptr<CellSet> NewInstance() const override
  {
    return std::make_shared<CellSetSingleType>();
  }

  VTKM_CONT
  void DeepCopy(const CellSet* src) override
  {
    const auto* other = dynamic_cast<const CellSetSingleType*>(src);
    if (!other)
    {
      throw vtkm::cont::ErrorBadType("CellSetSingleType::DeepCopy types don't match");
    }

    this->Superclass::DeepCopy(other);
    this->CellShapeAsId = other->CellShapeAsId;
    this->NumberOfPointsPerCell = other->NumberOfPointsPerCell;
  }

  virtual void PrintSummary(std::ostream& out) const override
  {
    out << "   CellSetSingleType: Type=" << this->CellShapeAsId << std::endl;
    out << "   CellPointIds:" << std::endl;
    this->Data->CellPointIds.PrintSummary(out);
    out << "   PointCellIds:" << std::endl;
    this->Data->PointCellIds.PrintSummary(out);
  }

private:
  template <typename CellShapeTag>
  void CheckNumberOfPointsPerCell(CellShapeTag,
                                  vtkm::CellTraitsTagSizeFixed,
                                  vtkm::IdComponent numVertices) const
  {
    if (numVertices != vtkm::CellTraits<CellShapeTag>::NUM_POINTS)
    {
      throw vtkm::cont::ErrorBadValue("Passed invalid number of points for cell shape.");
    }
  }

  template <typename CellShapeTag>
  void CheckNumberOfPointsPerCell(CellShapeTag,
                                  vtkm::CellTraitsTagSizeVariable,
                                  vtkm::IdComponent vtkmNotUsed(numVertices)) const
  {
    // Technically, a shape with a variable number of points probably has a
    // minimum number of points, but we are not being sophisticated enough to
    // check that. Instead, just pass the check by returning without error.
  }

  void CheckNumberOfPointsPerCell(vtkm::IdComponent numVertices) const
  {
    switch (this->CellShapeAsId)
    {
      vtkmGenericCellShapeMacro(this->CheckNumberOfPointsPerCell(
        CellShapeTag(), vtkm::CellTraits<CellShapeTag>::IsSizeFixed(), numVertices));
      default:
        throw vtkm::cont::ErrorBadValue("CellSetSingleType unable to determine the cell type");
    }
  }

  vtkm::Id ExpectedNumberOfCellsAdded;
  vtkm::Id CellShapeAsId;
  vtkm::IdComponent NumberOfPointsPerCell;
};
}
} // namespace vtkm::cont

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace vtkm
{
namespace cont
{

template <typename ConnectivityST>
struct SerializableTypeString<vtkm::cont::CellSetSingleType<ConnectivityST>>
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name = "CS_Single<" +
      SerializableTypeString<vtkm::cont::ArrayHandle<vtkm::Id, ConnectivityST>>::Get() + "_ST>";

    return name;
  }
};
}
} // vtkm::cont

namespace mangled_diy_namespace
{

template <typename ConnectivityST>
struct Serialization<vtkm::cont::CellSetSingleType<ConnectivityST>>
{
private:
  using Type = vtkm::cont::CellSetSingleType<ConnectivityST>;

public:
  static VTKM_CONT void save(BinaryBuffer& bb, const Type& cs)
  {
    vtkmdiy::save(bb, cs.GetNumberOfPoints());
    vtkmdiy::save(bb, cs.GetCellShape(0));
    vtkmdiy::save(bb, cs.GetNumberOfPointsInCell(0));
    vtkmdiy::save(
      bb, cs.GetConnectivityArray(vtkm::TopologyElementTagCell{}, vtkm::TopologyElementTagPoint{}));
  }

  static VTKM_CONT void load(BinaryBuffer& bb, Type& cs)
  {
    vtkm::Id numberOfPoints = 0;
    vtkmdiy::load(bb, numberOfPoints);
    vtkm::UInt8 shape;
    vtkmdiy::load(bb, shape);
    vtkm::IdComponent count;
    vtkmdiy::load(bb, count);
    vtkm::cont::ArrayHandle<vtkm::Id, ConnectivityST> connectivity;
    vtkmdiy::load(bb, connectivity);

    cs = Type{};
    cs.Fill(numberOfPoints, shape, count, connectivity);
  }
};

} // diy
/// @endcond SERIALIZATION

#endif //vtk_m_cont_CellSetSingleType_h

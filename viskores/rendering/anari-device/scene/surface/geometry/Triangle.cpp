//=============================================================================
//
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//
//=============================================================================

#include "Triangle.h"
// Viskores
#include <viskores/CellShape.h>
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandleRuntimeVec.h>
#include <viskores/cont/CellSetSingleType.h>
#include <viskores/cont/UnknownArrayHandle.h>
// std
#include <array>
#include <numeric>

namespace viskores_device
{

Triangle::Triangle(ViskoresDeviceGlobalState* s)
  : Geometry(s)
  , m_index(this)
  , m_vertexColor(this)
{
  this->m_vertexAttributes.setAttributes(this,
                                         { "position",
                                           "normal",
                                           "tangent",
                                           "color",
                                           "attribute0",
                                           "attribute1",
                                           "attribute2",
                                           "attribute3" });
  this->m_vertexAttributes.setAnariAssociation("vertex");
  this->m_vertexAttributes.setViskoresAssociation(viskores::cont::Field::Association::Points);

  this->m_faceVaryingAttributes.setAttributes(
    this, { "normal", "tangent", "color", "attribute0", "attribute1", "attribute2", "attribute3" });
  this->m_faceVaryingAttributes.setAnariAssociation("faceVarying");
  this->m_faceVaryingAttributes.setViskoresAssociation(viskores::cont::Field::Association::Cells);
}

void Triangle::commitParameters()
{
  this->Geometry::commitParameters();

  // Stashing these in a ChangeObserverPtr means that commit will be
  // called again if the array contents change.
  this->m_index = getParamObject<Array1D>("primitive.index");
  this->m_vertexAttributes.commitParameters();
  this->m_faceVaryingAttributes.commitParameters();

  box1 range = { 0, 1 };
  this->getParam("valueRange", ANARI_FLOAT32_BOX1, &range);
  this->m_colorTable.RescaleToRange({ range.lower, range.upper });
}

void Triangle::finalize()
{
  this->Geometry::finalize();

  helium::ChangeObserverPtr<Array1D>& positionArray = this->m_vertexAttributes.getParam("position");
  if (!positionArray)
  {
    reportMessage(ANARI_SEVERITY_WARNING,
                  "'triangle' geometry missing 'vertex.position' parameter");
    return;
  }

  if (!this->m_index)
  {
    reportMessage(ANARI_SEVERITY_INFO, "generating 'triangle' index array");

    Array1DMemoryDescriptor md;
    md.appMemory = nullptr;
    md.deleter = nullptr;
    md.deleterPtr = nullptr;
    md.elementType = ANARI_UINT32_VEC3;
    md.numItems = positionArray->totalSize() / 3;

    this->m_index = new Array1D(this->deviceState(), md);
    this->m_index->refDec(helium::RefType::PUBLIC); // no public references

    auto* begin = (uint32_t*)this->m_index->map();
    auto* end = begin + this->m_index->totalSize() * 3;
    std::iota(begin, end, 0);
  }

  // Reset data
  this->m_dataSet = viskores::cont::DataSet{};
  this->m_mapper = std::make_shared<viskores::rendering::MapperRayTracer>();

  // Get the connection array.
  // Note that ANARI provides the connection array as a series of triples
  // whereas Viskores wants a flat array of indices. The easist way to do the
  // conversion (while sharing pointers) is to use ArrayHandleRuntimeVec.
  viskores::cont::ArrayHandleRuntimeVec<viskores::Id> connectionArray(3);
  viskores::cont::ArrayCopyShallowIfPossible(this->m_index->dataAsViskoresArray(), connectionArray);

  viskores::cont::CellSetSingleType<> cellSet;
  cellSet.Fill(static_cast<viskores::Id>(positionArray->size()),
               viskores::CELL_SHAPE_TRIANGLE,
               3,
               connectionArray.GetComponentsArray());
  this->m_dataSet.SetCellSet(cellSet);

  this->m_vertexAttributes.setFields(this->m_dataSet);
  this->m_faceVaryingAttributes.setFields(this->m_dataSet);

  // We have already checked that the position array exists.
  this->m_dataSet.AddCoordinateSystem("position");
}

bool Triangle::isValid() const
{
  return this->m_vertexAttributes.getParam("position");
}

} // namespace viskores_device

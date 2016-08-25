//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_ArrayHandle_Interpreter_h
#define vtk_m_cont_ArrayHandle_Interpreter_h

#include <vtkm/cont/ArrayHandle.h>

namespace vtkm{
namespace cont{

template< typename T >
class ArrayHandleInterpreter : public vtkm::cont::ArrayHandle< T >
{
private:
  vtkm::Id dimX, dimY, dimZ;
  bool     valid2D, valid3D;

public:
  VTKM_ARRAY_HANDLE_SUBCLASS( ArrayHandleInterpreter, 
                              (ArrayHandleInterpreter<T>), 
                              (vtkm::cont::ArrayHandle<T>) );
  
  void PrintInfo()
  {
    std::cout << "Dimensions: (" << dimX << ", " << dimY << ", " << dimZ << ")" << std::endl;
    if (valid2D)
      std::cout << "valid2D. " << std::endl;
    if( valid3D )
      std::cout << "valid3D. " << std::endl;
  }

  void CopyInterpreterInfo( const ArrayHandleInterpreter& src )
  {
    this->dimX    = src.GetDimX();
    this->dimY    = src.GetDimY();
    this->dimZ    = src.GetDimZ();
    this->valid2D = src.IsValid2D();
    this->valid3D = src.IsValid3D();
  }

  bool InterpretAs2D( vtkm::Id dim_x, vtkm::Id dim_y )
  {
    this->dimX    = dim_x;
    this->dimY    = dim_y;
    this->dimZ    = 1;
    this->valid2D = this->valid3D = false;
    if( this->GetNumberOfValues() == dimX * dimY )
      valid2D = valid3D = true;

    return valid2D;
  } 

  bool InterpretAs3D( vtkm::Id dim_x, vtkm::Id dim_y, vtkm::Id dim_z )
  {
    this->dimX    = dim_x;
    this->dimY    = dim_y;
    this->dimZ    = dim_z;
    this->valid2D = this->valid3D = false;
    if( this->GetNumberOfValues() == dimX * dimY * dimZ )
      valid3D = true;

    return valid3D;
  } 

  bool IsValid2D() const    { return this->valid2D; }
  bool IsValid3D() const    { return this->valid3D; }

  T Get2D( vtkm::Id x, vtkm::Id y ) const
  {
    if( this->valid2D )
    {
      if( x < dimX && y < dimY && x > -1 && y > -1 )
        return this->Get3D( x, y, 0 );
      else
        throw vtkm::cont::ErrorControlBadValue(
              "ArrayHandleInterpreter::Not valid 2D indices to read.");
    }
    else
      throw vtkm::cont::ErrorControlInternal(
            "ArrayHandleInterpreter::Not ready to interpret as 2D matrix yet.");
  }

  T Get3D( vtkm::Id x, vtkm::Id y, vtkm::Id z ) const
  {
    if( this->valid3D )
    {
      if( x < dimX && y < dimY && z < dimZ &&
          x > -1   && y > -1   && z > -1 )
      {
        vtkm::Id idx = z*(dimX*dimY) + y*dimX + x;
        return this->GetPortalConstControl().Get( idx );
      }
      else
        throw vtkm::cont::ErrorControlBadValue(
              "ArrayHandleInterpreter::Not valid 3D indices to read.");
    }
    else
      throw vtkm::cont::ErrorControlInternal(
            "ArrayHandleInterpreter::Not ready to interpret as 3D volume yet.");
  }

  void Set2D( vtkm::Id x, vtkm::Id y, T val )
  {
    if( this->valid2D )
    {
      if( x < dimX && y < dimY && x > -1 && y > -1 )
        this->Set3D( x, y, 0, val );
      else
        throw vtkm::cont::ErrorControlBadValue(
              "ArrayHandleInterpreter::Not valid 2D indices to read.");
    }
    else
      throw vtkm::cont::ErrorControlInternal(
            "ArrayHandleInterpreter::Not ready to interpret as 2D matrix yet.");
  }

  void Set3D( vtkm::Id x, vtkm::Id y, vtkm::Id z, T val )
  {
    if( this->valid3D )
    {
      if( x < dimX && y < dimY && z < dimZ &&
          x > -1   && y > -1   && z > -1     )
      {
        vtkm::Id idx = z*(dimX*dimY) + y*dimX + x;
        this->GetPortalControl().Set( idx, val );
      }
      else
        throw vtkm::cont::ErrorControlBadValue(
              "ArrayHandleInterpreter::Not valid 3D indices to read.");
    }
    else
      throw vtkm::cont::ErrorControlInternal(
            "ArrayHandleInterpreter::Not ready to interpret as 3D volume yet.");
  }

  vtkm::Id GetDimX() const
  {
    if( valid3D || valid2D )
    {
      if( dimX > 0 && dimY > 0 && dimZ > 0 )
        return dimX; 
      else
        throw vtkm::cont::ErrorControlInternal(
              "ArrayHandleInterpreter::Not ready to interpret yet.");
    }
      else
        throw vtkm::cont::ErrorControlInternal(
              "ArrayHandleInterpreter::Not ready to interpret yet.");
  }

  vtkm::Id GetDimY() const
  {
    if( valid3D || valid2D )
    {
      if( dimX > 0 && dimY > 0 && dimZ > 0 )
        return dimY; 
      else
        throw vtkm::cont::ErrorControlInternal(
              "ArrayHandleInterpreter::Not ready to interpret yet.");
    }
      else
        throw vtkm::cont::ErrorControlInternal(
              "ArrayHandleInterpreter::Not ready to interpret yet.");
  }

  vtkm::Id GetDimZ() const
  {
    if( valid3D || valid2D )
    {
      if( dimX > 0 && dimY > 0 && dimZ > 0 )
        return dimZ; 
      else
        throw vtkm::cont::ErrorControlInternal(
              "ArrayHandleInterpreter::Not ready to interpret yet.");
    }
      else
        throw vtkm::cont::ErrorControlInternal(
              "ArrayHandleInterpreter::Not ready to interpret yet.");
  }
  
};

}
}





#endif

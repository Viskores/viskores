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

public:
  VTKM_ARRAY_HANDLE_SUBCLASS( ArrayHandleInterpreter, 
                              (ArrayHandleInterpreter<T>), 
                              (vtkm::cont::ArrayHandle<T>) );
  
  void PrintInfo()
  {
    std::cout << "Dimensions: (" << dimX << ", " << dimY << ", " << dimZ << ")" << std::endl;
  }

  void CopyDimInfo( const ArrayHandleInterpreter& src )
  {
    this->dimX    = src.GetDimX();
    this->dimY    = src.GetDimY();
    this->dimZ    = src.GetDimZ();
  }

  bool InterpretAs2D( vtkm::Id dim_x, vtkm::Id dim_y )
  {
    if( this->GetNumberOfValues() == dim_x * dim_y )
    {
      this->dimX    = dim_x;
      this->dimY    = dim_y;
      this->dimZ    = 1;
      return true;
    }
    else
    {
      throw vtkm::cont::ErrorControlBadValue(
            "ArrayHandleInterpreter::Not valid 2D dimensions to interpret.");
      return false;
    }
  } 

  bool InterpretAs3D( vtkm::Id dim_x, vtkm::Id dim_y, vtkm::Id dim_z )
  {
    if( this->GetNumberOfValues() == dim_x * dim_y * dim_z )
    {
      this->dimX    = dim_x;
      this->dimY    = dim_y;
      this->dimZ    = dim_z;
      return true;
    }
    else
    {
      throw vtkm::cont::ErrorControlBadValue(
            "ArrayHandleInterpreter::Not valid 3D dimensions to interpret.");
      return false;
    }
  } 

  T Get2D( vtkm::Id x, vtkm::Id y ) const
  {
    if( x < dimX && y < dimY && x > -1 && y > -1 )
      return this->Get3D( x, y, 0 );
    else
    {
      std::cerr << "    trying to read: " << x << ", " << y << std::endl;
      std::cerr << "    dimX and dimY are: " << dimX << ", " << dimY << std::endl;
      throw vtkm::cont::ErrorControlBadValue(
            "ArrayHandleInterpreter::Not valid 2D indices to read.");
    }
  }

  T Get3D( vtkm::Id x, vtkm::Id y, vtkm::Id z ) const
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

  void Set2D( vtkm::Id x, vtkm::Id y, T val )
  {
    if( x < dimX && y < dimY && x > -1 && y > -1 )
      this->Set3D( x, y, 0, val );
    else
    {
      std::cerr << "    trying to write: " << x << ", " << y << std::endl;
      throw vtkm::cont::ErrorControlBadValue(
            "ArrayHandleInterpreter::Not valid 2D indices to write.");
    }
  }

  void Set3D( vtkm::Id x, vtkm::Id y, vtkm::Id z, T val )
  {
    if( x < dimX && y < dimY && z < dimZ &&
        x > -1   && y > -1   && z > -1     )
    {
      vtkm::Id idx = z*(dimX*dimY) + y*dimX + x;
      this->GetPortalControl().Set( idx, val );
    }
    else
      throw vtkm::cont::ErrorControlBadValue(
            "ArrayHandleInterpreter::Not valid 3D indices to write.");
  }

  vtkm::Id GetDimX() const  { return dimX; }
  vtkm::Id GetDimY() const  { return dimY; }
  vtkm::Id GetDimZ() const  { return dimZ; }
};

}
}





#endif

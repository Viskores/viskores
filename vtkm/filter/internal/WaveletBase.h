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

#ifndef vtk_m_filter_internal_waveletbase_h
#define vtk_m_filter_internal_waveletbase_h


#include <vtkm/filter/internal/WaveletFilter.h>

#include <vtkm/Math.h>

namespace vtkm {
namespace filter {

namespace internal {

enum DWTMode {    // boundary extension modes
  INVALID = -1,
  ZPD, 
  SYMH, 
  SYMW,
  ASYMH, ASYMW, SP0, SP1, PPD, PER
};

// Functionalities are similar to MatWaveBase in VAPoR.
class WaveletBase
{
public:

  // Constructor
  WaveletBase( const std::string &w_name )
  {
    this->filter = NULL;
    this->wmode = PER;
    this->wname = w_name;
    if( wname.compare("CDF9/7") == 0 )
    {
      this->wmode = SYMW;   // Default extension mode, see MatWaveBase.cpp
      this->filter = new vtkm::filter::internal::WaveletFilter( wname );
    }
  }

  // Destructor
  virtual ~WaveletBase()
  {
    if( filter )  
      delete filter;
    filter = NULL;
  }

  // Get the wavelet filter
  const WaveletFilter* GetWaveletFilter() 
  { 
    if( this->filter == NULL )
    {
      // throw an error
    }
    return filter; 
  }

  // Returns length of approximation coefficients from a decompostition pass.
  vtkm::Id GetApproxLength( vtkm::Id sigInLen )
  {
    vtkm::Id filterLen = this->filter->GetFilterLength();

    if (this->wmode == PER) 
      return static_cast<vtkm::Id>(vtkm::Ceil( (static_cast<vtkm::Float64>(sigInLen)) / 2.0 ));
    else if (this->filter->isSymmetric()) 
    {
      if ( (this->wmode == SYMW && (filterLen % 2 != 0)) ||
           (this->wmode == SYMH && (filterLen % 2 == 0)) )  
      {
        if (sigInLen % 2 != 0)
          return((sigInLen+1) / 2);
        else 
          return((sigInLen) / 2);
      }
    }

    return static_cast<vtkm::Id>( vtkm::Floor(
           static_cast<vtkm::Float64>(sigInLen + filterLen - 1) / 2.0 ) );
  }

  // Returns length of detail coefficients from a decompostition pass
  vtkm::Id GetDetailLength( vtkm::Id sigInLen )
  {
    vtkm::Id filterLen = this->filter->GetFilterLength();

    if (this->wmode == PER) 
      return static_cast<vtkm::Id>(vtkm::Ceil( (static_cast<vtkm::Float64>(sigInLen)) / 2.0 ));
    else if (this->filter->isSymmetric()) 
    {
      if ( (this->wmode == SYMW && (filterLen % 2 != 0)) ||
           (this->wmode == SYMH && (filterLen % 2 == 0)) )  
      {
        if (sigInLen % 2 != 0)
          return((sigInLen-1) / 2);
        else 
          return((sigInLen) / 2);
      }
    }

    return static_cast<vtkm::Id>( vtkm::Floor(
           static_cast<vtkm::Float64>(sigInLen + filterLen - 1) / 2.0 ) );
  }

  // Returns length of coefficients generated in a decompostition pass
  vtkm::Id GetCoeffLength( vtkm::Id sigInLen )
  {
    return( GetApproxLength( sigInLen ) + GetDetailLength( sigInLen ) );
  }
  vtkm::Id GetCoeffLength2( vtkm::Id sigInX, vtkm::Id sigInY )
  {
    return( GetCoeffLength( sigInX) * GetCoeffLength( sigInY ) );
  }
  vtkm::Id GetCoeffLength3( vtkm::Id sigInX, vtkm::Id sigInY, vtkm::Id sigInZ)
  {
    return( GetCoeffLength( sigInX) * GetCoeffLength( sigInY ) * GetCoeffLength( sigInZ ) );
  }

  // Returns maximum wavelet decompostion level
  vtkm::Id GetWaveletMaxLevel( vtkm::Id sigInLen )
  {
    if( ! this->filter )
      return 0;
    else {
      vtkm::Id filterLen = this->filter->GetFilterLength(); 
      vtkm::Id level;
      this->WaveLengthValidate( sigInLen, filterLen, level );
      return level;
    }
  }

protected:
  vtkm::filter::internal::DWTMode           wmode;
  WaveletFilter*                            filter;
  std::string                               wname;

  void WaveLengthValidate( vtkm::Id sigInLen, vtkm::Id filterLength, vtkm::Id &level)
  {
    // *lev = (int) (log((double) sigInLen / (double) (waveLength)) / log(2.0)) + 1;
    if( sigInLen < filterLength )
      level = 0;
    else
      level = static_cast<vtkm::Id>( vtkm::Floor( 
                  vtkm::Log2( static_cast<vtkm::Float64>(sigInLen) / 
                              static_cast<vtkm::Float64>(filterLength) ) + 1.0 ) );
  }
};    // Finish class WaveletBase.


}     // Finish namespace internal

}     // Finish namespace filter
}     // Finish namespace vtkm

#endif 

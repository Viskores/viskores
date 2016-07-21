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

#include <vtkm/filter/WaveletCompressor.h>

namespace vtkm {
namespace filter {


// Multi-level 1D wavelet decomposition
template< typename SignalArrayType, typename CoeffArrayType>
vtkm::Id 
WaveletCompressor::WaveDecompose( const SignalArrayType   &sigIn,   // Input
                                  vtkm::Id                nLevels,  // n levels of DWT
                                  CoeffArrayType          &coeffOut,
                                  vtkm::Id*               L )       // bookkeeping array;
                                                                    // len(L) = nLevels+2
{

  return 0;
}
                      


}     // Finish namespace filter
}     // Finish namespace vtkm


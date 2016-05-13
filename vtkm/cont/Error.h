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
#ifndef vtk_m_cont_Error_h
#define vtk_m_cont_Error_h

// Note that this class and (most likely) all of its subclasses are not
// templated.  If there is any reason to create a VTKm control library,
// this class and its subclasses should probably go there.

#include <string>

namespace vtkm {
namespace cont {

/// The superclass of all exceptions thrown by any VTKm function or method.
///
class Error
{
public:
//See note about GetMessage macro below.
#ifndef GetMessage
  const std::string &GetMessage() const { return this->Message; }
#endif

//GetMessage is a macro defined by <windows.h> to redirrect to
//GetMessageA or W depending on if you are using ansi or unicode.
//To get around this we make our own A/W variants on windows.
#ifdef _WIN32
  const std::string &GetMessageA() const { return this->Message; }
  const std::string &GetMessageW() const { return this->Message; }
#endif

protected:
  Error() { }
  Error(const std::string message) : Message(message) { }

  void SetMessage(const std::string &message)
  {
    this->Message = message;
  }

private:
  std::string Message;
};

}
} // namespace vtkm::cont

#endif //vtk_m_cont_Error_h

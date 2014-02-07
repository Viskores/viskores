### Design Decisions ###

A quick list of where the primary classes that will go into VTKM.

High level classes:
+ vtkm::vtkAllocators [ dax ]
+ vtkm::vtkMappedDataSets [ dax ]
+ vtkm::Filters [ eavl ]
  + pistons halo finder
+ vtkm::Mutators [ eavl ] + additive fields
+ vtkm::OpenGLInterop [ dax + piston ]


Mid level:
+ vtkm::ArrayHandle [dax]
  + dynamic info from eavl
+ vtkm::CellSet [eavl]
  + Includes the Explicit and Implicit versions
+ vtkm::DataSet [eavl]
  + Holds a Coordinate field
  + Holds a collection of CellSets
  + Holds a collection of array handles as fields

Low level:
+ vtkm::DeviceAdapter [dax]
+ vtkm::DeviceAdapterGeneral [dax]
+ vtkm::TopologyMap [eavl]
+ vtkm::WorkletConcept [ eavl + dax ]


Code Layout:
vtkm/
  cont/
    datamodel/
    filters/
  exec/
    worklets/

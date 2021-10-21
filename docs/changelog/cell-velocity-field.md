# Adding ability to use cell-centered velocity fields for particle advection

Vector fields for particle advection are not always nodal,; e.g., AMR-Wind uses
zonal vector fields to store velocity information. Previously, VTK-m filters
only supported particle advection in nodal vector fields. With this change, VTK-m
will support zonal vector fields. Users do not need to worry about changing the
way they specify inputs to the flow visualization filters. However, if users use
the particle advection worklets, they'll need to specify the associativity for
their vector fields.

```
vtkm::cont::Field field = dataset.GetField("velocity");
vtkm::cont::Field::Association assoc = field.GetAssociation();

using FieldArray = vtkm::cont::ArrayHandle<vtkm::Vec3f>;
using FieldType  = vtkm::worklet::particleadvection::VelocityField<FieldType>;

FieldArray data;
field.GetData().AsArrayHandle<FieldArray>(data);

// Use this field to pass to the GridEvaluators
FieldType velocities(data, assoc);
``` 

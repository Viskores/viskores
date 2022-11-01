# Expose the Variant helper class

For several versions, VTK-m has had a `Variant` templated class. This acts
like a templated union where the object will store one of a list of types
specified as the template arguments. (There are actually 2 versions for the
control and execution environments, respectively.)

Because this is a complex class that required several iterations to work
through performance and compiler issues, `Variant` was placed in the
`internal` namespace to avoid complications with backward compatibility.
However, the class has been stable for a while, so let us expose this
helpful tool for wider use.

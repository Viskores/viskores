# Coordiante systems are stored as Fields

Previously, `DataSet` managed `CoordinateSystem`s separately from `Field`s.
However, a `CoordinateSystem` is really just a `Field` with some special
attributes. Thus, coordiante systems are now just listed along with the
rest of the fields, and the coordinate systems are simply strings that
point back to the appropriate field. (This was actually the original
concept for `DataSet`, but the coordinate systems were separated from
fields for some now obsolete reasons.)

This change should not be very noticible, but there are a few consequences
that should be noted.

1. The `GetCoordinateSystem` methods no longer return a reference to a
   `CoordinateSystem` object. This is because the `CoordinateSystem` object
   is made on the fly from the field.
2. When mapping fields in filters, the coordinate systems get mapped as
   part of this process. This has allowed us to remove some of the special
   cases needed to set the coordinate system in the output.
3. If a filter is generating a coordinate system in a special way
   (different than mapping other point fields), then it can use the special
   `CreateResultCoordianteSystem` method to attach this custom coordinate
   system to the output.
4. The `DataSet::GetCoordianteSystems()` method to get a `vector<>` of all
   coordiante systems is removed. `DataSet` no longer internally has this
   structure. Although it could be built, the only reason for its existance
   was to support passing coordinate systems in filters. Now that this is
   done autmoatically, the method is no longer needed.


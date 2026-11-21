# Motion target updater `80118BBC`

`src/semantic/track_vehicle_ground.c` replaces the generated draft for the
208-word game function at `80118BBC`. The retained IDA shape was checked
instruction-by-instruction against the retail MIPS body. In particular, the
function has one signed index argument and returns the non-positive actor count,
zero when no actor is near, or one after emitting the first nearby interaction.

The function rebuilds two endpoints around one 56-byte motion record, samples
their terrain heights, updates the attached object's height and orientation,
rebuilds its rotation matrix, and scans the player actors by Manhattan distance.
For the first actor inside 800 units it transforms a rear offset and conditionally
calls `sub_80100398` (trackVehicleSurfaceSet) with the signed object angle.

`tools/audit_motion_target_update.py` differentially executes retail and candidate
MIPS over ten fixtures. They cover both non-positive count exits, no match, first
and later matches, the 800-unit boundary, the record gate, both record indices,
negative coordinates/heights and both absolute-value arms. All 208 retail words
execute, and calls, transformed endpoint/emit memory, return, RAM and ABI agree.
The compiled body is 728 bytes and fits the 832-byte retail slot.

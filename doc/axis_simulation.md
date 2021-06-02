# AccessTSN Industrial Use Case Demo - RTDriveControl: Documentation of Axis Simulation
The purpose of the *demo_tsndrive* application is to simulate the behavior of one or multiple axes of a CNC machine. This mainly means to calculate current position values from the last position values and the set point values. The functions which implement this functionality are bundled in the *axis_sim.h* and axis_sim.c* files.

## Program structure and assumptions
The simulation of the behavior of a axis is calculated using a PT2 filter. To get a better result, multiple iterations are computed for each set-point value. The number of the so called fine iterations is configurable. It is assumed that the axis have a limited length and velocity and that the absolute value of the minimum and maximum position and velocity values are identical. Also it is assumed that the axis are independent from each other.

### Definition and data containers
Some definitions and data structures are used to enable adaptions of values to the execution environment and to organize values.

#### Definitions
Some variables greatly influence the behavior of a simulated axis. These can be adapted to simulate a different axis behavior or to represent a different axis setup. For the AccessTSN Industrial Use Case Demo the given values should fine. If a change is necessary the values can be easily change by precompiler definitions. The following variables are available:
- PT2-Variables: K-factor, Time and damping factors
- Maximum positions and velocities of four axes; changes here must also reflected in the CNC component
- Number of iteration between two set point values

#### Axis structure (axis_t)
This structure represents a single axis with it's current state. Aside from an axis identifier, it stores the maximum and current position values as well as the maximum, current, last and set point velocity values. Also it stores the enable and fault switches.

### Functions

#### Initialization (*axis_sim.c/axs_init*)
This function initializes a single simulated axis. It sets the current state of the axis to zero, sets the start position to the supplied value and sets the maximum and minium values. 

#### Initialization of requested axis (*axis_sim.c/axs_initreq*)
This initializes the requested number of axes starting from the specified starting axis. This function wraps the *axs_init* function. It is used to prepare the axes which should be simulated by this instance.

#### Calculate new position value once (*axs_sim.c/axs_clcpstn*)
This is the main calculation function to determine a new position of an axis. After first checking if the axis is enabled, the new velocity value is calculated using a PT2 filter. Then it is checked if the new velocity value is in the allowed range and it is bounded if necessary. Then the new position value is calculated. A check it the new position values is in the allowed range including bounding if necessary takes place next. At the end the new values are stored as the current values.

#### Calculate new position value multiple times (*axs_sim.c/axs_fineclcpstn*)
This function divides the given time interval in the requested amount of iterations and calculates a new position value with the requested number of iterations using *axs_clpstn*.

#### Enable an axis (*axs_sim.c/axs_enbl*)
This function first clears faults on the given axis (using *axs_clrflt*) and then sets the enable value to true.

#### Disable an axis (*axs_sim.c/axs_dsbl*)
Through setting the enable and fault value of the specified axis to false and setting the set point of the velocity value to zero this function disables an axis. It also calculates one new position update with a time step of one second (with the set point value of zero) and then sets the current velocity value also to zero. This assumes, that the axis stops within one second.

#### Clear faults on an axis (*axs_sim.c/axs_clrflt*)
It the specified axis is disabled, this functions clears the fault value of the axis.

#### Update velocity set points of all simulated axes (*axs_sim.c/axes_updt_setvel*)
The velocity set point values given in the controlinfo structure are set to the corresponding axis for all axes.

#### Update enable set point value for all simulated axes (*axs_sim.c/axes_updt_enbl*)
If the enable set point value in the given controlinfo structure is greater than zero the corresponding axis is enabled. The axis is disabled otherwise. If the value is smaller the zero the startup function (*axs_ststrtup*) for this axis is called. 

#### Set startup position of axis (*axs_sim.c/axs_ststrtup*)
This function is used to set the starting position of an axis. In case the simulation and the control are not started with the same state of the axis position this function can be used to set the position of the axis to the value given by the control and therefore synchronizing the two. Before setting the given position value to the axis the function checks if the value is within the allowed range.
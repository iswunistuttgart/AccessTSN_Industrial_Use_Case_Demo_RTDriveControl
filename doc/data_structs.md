# AccessTSN Industrial Use Case Demo - RTDriveControl: Documentation of Data Structures
For the real-time drive control applications in the AccessTSn Industrial Use Case Demo some common structures are used to hold values. Theses structures are used as arguments between functions and could be extended in the future is necessary. Teh structures are defined in the file _data_structs.h_.

## Enumerations
### Axis Identification (axsID_t)
This enumeration is used to identify which axis the information corresponds to.

## Structures
### Axis Information (axsnfo_t)
This structure hold the information for a single axis. Aside from the axis identification if holds a control value, a position set point, the current position ans a control switch. The control switch can be used for the enable or a fault signal. 
This structure is used to pass the information of a axis between the control and the communication functions as well as between the axis simulation and communication functions.

### Control Information (cntrlnfo_t)
The control information structure is a collection of the control information from the CNC main control of the demo machine. It hold the information of the four axes as well as the status/command for the spindle brake, the machine status and the emergency stop. The structure is used to pass this information between the control and the communication functions as well as between the axis simulation and communication functions.
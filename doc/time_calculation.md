# AccessTSN Industrial Use Case Demo - RTDriveControl: Documentation of the calculation of timing variables
During the execution of the applications of the AccessTSN Industrial Use Case Demo various timing variables are necessary and need t be calculated. The files *time_calc.h* and *time_calc.c* bundle the implemented functionality.

## Program structure and assumptions
The files provide functions to do calculation with the timespec format as well as conversion function to the OPC UA time format. The functions should be usage without a lot of other dependencies. Some functions use other functions of this files heavily. 

### Definitions
The *time_calc.h* defines two constants which are necessary for the conversion of the used time formats. One is the amount of nano seconds with in a second and the other one is the difference between the base times of the unix time format and the OPC UA time format.

### Functions
#### Convert Timespec to OPC UA time (*time_calc.c/cnvrt_tmspc2uatm*)
The OPC UA time format uses a signed 64 Bit integer to store the number of 100 nanosecond intervals since January 1, 1601 (UTC). The function converts the timespec to nanoseconds, adds the epoch difference and divides everything by one hundred to get the number of 100 nanoseconds.

#### Convert OPC UA time to timespec (*time_calc.c/cnvrt_uatm2tmsp*)
For the conversion from OPC UA time format to the timespec format, the function multiplies the UA time format by one hundred to get nanoseconds and then subtracts the epoch difference. Then the nanosecond value is split in complete seconds and the remaining nanoseconds for the timespec format.

#### Increase timestamp by an interval (*time_calc.c/inc_tm*)
This function increases a timestamp in timespec format by a nanosecond interval. If the nanosecond value of the timespec get larger than a second by the added interval, the function also increases the second value accordingly.

#### Decrease timestamp by an interval (*time_calc.c/dec_tm*)
This function decreases a timestamp in timespec format by a nanosecond interval. First it checks if the interval is larger than one second. It increases the nanosecond value and reduces the second value of the timespec value until the nanosecond value is larger then the interval. Then the interval is subtracted from the nanosecond value.

#### Convert timespec to 64 Bit integer (*time_calc.c/cnvrt_tmspc2int64*)
This function converts a timestamp in the timespec format to a 64 Bit integer representing nanoseconds. This is used in some other functions to do calculation easier.

#### Convert 64 Bit integer to timespec (*time_calc.c/cnvrt_int642tmspc*)
This function converts a 64 Bit integer representing nanoseconds to a timestamp in the timespec format. This is used in some other functions after calculations based on nanoseconds. Conversion  is done through a modulo and a division operation.

#### Convert double to timespec (*time_calc.c/dbl2tmspc*)
This converts a double value representing seconds to the timespec format. The conversion is done using a cast and basic subtraction and multiplication.

#### Compare two timespec values (*time_calc.c/cmptmpc_Ab4rB*)
In time based execution is is necessary to compare two timestamps (e.g. current time and planned time). This function does a comparison of two timestamps in the timespec format. It first checks if the second value is of the two timestamp is identical. If they are identical the function returns if the nanosecond value of the first timestamp is lower than the second. If the second value is not identical it returns if the second value of the first timestamp is lower than the second.

#### Copy timespec (*time_calc.c/tmspc_cp*)
This function copies the values of one timestamp in the timespec format to the other.

#### Add two timespec variables (*time_calc.c/tmspc_add*)
This function adds the seconds value of the two supplied timespec variables as well as the nanosecond values. It increases the seconds value if the nanoseconds value exceeds one second.

#### Subtract two timespec variables (*time_calc.c/tmspc_sub*)
This function subtracts the seconds value of the two supplied timespec variables from each other as well as the nanosecond values. If the nanoseconds value of the second (B) variable is larger than of the first (A) the function decreases the seconds value of the first variable by one and increases the nanoseconds value accordingly before subtracting the values of the second variable from the first.

#### Calculate Epoch start time (*time_calc.c/clc_est*)
The Epoch start time is the moment in time when the first period of the network cycle starts in which the application is active. It is calculated using the specified base time. First the function checks if the specified base time would be in the future after adding the time interval of one period to the current time. In that case the base time is directly used as the epoch start time and the values are copied. The time interval is added during the check to account for the time the application would need to calculate the first set of values to communicate in the period.

If the base time is in the past the epoch start time is calculated by adding a time interval of multiple periods to the base time. The duration of the time interval is determined by the difference between the base time and the current time divided (integer division) by the length of a period and again multiplying the resulting amount og periods with the length of a period. Again one additional period is added to account for the calculation time needed by the application. For easier multiplication and division the timespec variables are converted to 64 bit integers and back using the functions described above.

#### Calculate next TxTime (*time_calc.c/clc_txtm*)
The TxTime is the the moment in time a packet should leave the network interface towards the network. This is used on socket with the *TxTime* options enabled. This timestamp needs to be communicated to the network stack using the *msg_hdr* and the *sendmsg* function to use the feature. The value of the TxTime variable is calculated using the epoch start time (the start of a cycle period) and the offset of the transmission from the start of a cycle period. Also the duration the stack and hardware need to forward the packet to the network needs to be considered.

The function first adds the epoch start time to an empty TxTime variable. Then the TxTime is increased by the specified offset form the start of the cycle period. Then it is decreased by the duration the stacks and hardware need for forwarding.

#### Calculate next wake-up time for a sending thread (*time_calc.c/clc_sndwkuptm*)
The wake-up time is the moment in time until a thread (or application) can sleep or when it needs to become active again to be able to complete the necessary calculations before the next packet is to be received or sent. For a sending thread this is based on the TxTime of the next packet as well as the calculation duration and the maximum wake-up jitter of the system.

This function first add the next TxTime to an empty send wake-up variable and decreases it by the duration the application (or thread) needs for its calculations between it's wake-up and the next packet being ready to send. The function also decreases the send wake-up variable by the maximum value of the wake-up jitter. The wake-up jitter being the duration between the time of the planned and actual wake-up of the application (or thread).

#### Calculate next wake-up time for a receiving thread (*time_calc.c/clc_rcvwkuptm*)
The wake-up time is the moment in time until a thread (or application) can sleep or when it needs to become active again to be able to complete the necessary calculations before the next packet is to be received or sent. For a receiving thread this is based on the epoch start time and the offset of the end of the incoming transmission from the start of a cycle period. Also the calculation duration and the maximum wake-up jitter of the system need to be considered.

This function first add the epoch start time of the next cycle period to an empty receive wake-up variable and increases it by the transmission offset to the start of the cycle period and the duration the hardware and stack need to forward a received packet to the application. The maximum value of the wake-up jitter is added. Then the function decreases the receive wake-up variable by the duration the application (or threads) needs for calculations until it is ready to receive a packet after it's wake-up.


# AccessTSN Industrial Use Case Demo - RTDriveControl: Documentation of the Drive-Component "demo_tsndrive"
## Execution and Command line arguments

### Introduction
The Drive-Component "demo_tsndrive" represents the drives of the basic milling machine in the AccessTSN Industrial Use Case Demo. It is a part of the real-time control loop and one of the components in the **Real-time drive control** communication relationship. The application simulates the behavior of one or multiple drives. This means it calculates and returns current position values from the received set-point velocities. The calculation mimics a PT-2 behavior. 
For more flexibility in the setup of the use case, the *demo_tsndrive* can simulate a variable number of axes specifiable through command line arguments. This way one instance of the application can be executed on a simple hardware platform simulating all axes resulting in a simpler setup or multiple instances can be executed on multiple hardware platforms each simulating a single axis for a more complex setup with more active endpoints on the network.

### Requirements for execution
The *demo_tsndrive* application utilizes some low level kernel functions. On most system not every user is allowed to use theses. Therefore the application needs to be executed by a user with the following privileges:
- Opening a "AF_PACKET"-(RAW)Socket
- Change the scheduling policy and priority for the spawned threads
- Send packets with the *SO_TXTime* socket option
For testing purposes the application is mostly executed as *root* with the *sudo* command.

Furthermore the application uses some functionality of the Linux network stack, which is not available or configured by default on every system. The system on which the application is executed should support:
- Opening and using a socket with the *SO_TXTIME* socket option. For this to function correctly, the *ETF* scheduling qdisc should be configured in the traffic control subsystem. Explanations on how to configure the ETF qdisc can be found in the [TSN Documentation Project for Linux](https://tsn.readthedocs.io/qdiscs.html#configuring-the-etf-qdisc). An example script on how to do this is included in the AccessTSN industrial Use Case Demo repositories.

### Command line arguments
To change the behavior of the application various arguments can be specified through the command line. some of the arguments have default values, theses are therefore optional. A help is printed when starting the application with the *-h* cli switch.
Note: In case of multiple instances of the application running, the sending offset is to be set identical for each instance. The application determines the specific offset for each axis using the sending offset and the send window duration for each axis.

|Switch| Description | Default-Value |
|--------------------|--------------|---------------|
|-t [value] | Specifies update-period in microseconds.| 1 millisecond|
|-b [value]  | Specifies the basetime (the start of the cycle). This is a Unix-Timestamp. ||
|-o [nanosec] | Specifies the sending offset, time between start of cycle and sending slot in nano seconds. ||
|-r [nanosec] | Specifies the receiving offset, time between start of cycle and end of receive slot in nano seconds.||
|-w [nanosec]  | Specifies the receive window duration, timeinterval in which a packet is expected in nano seconds.||
|-s [nanosec]  | Specifies the send window duration, timeinterval between 2 axis-packets in nano seconds.||
|-i                  | Name of the Networkinterface to use.||
|-p                   | PublisherID e.g. TalkerID,| 0xAC00|
|-y                  | Priority of sending socket (can be 1-7) |6|
|-n [value <5]       | Number of simulated axes. |4|
|-a [index <4]       | Index of the first simulated axis. x = 0, y = 1, z = 2, spindle = 3 |0|
|-h                  | Prints help message and exits||

An execution command for the suggested schedule of the AccessTSN Industrial Use Case demo could look like (change network interface to used system):

```Shell
sudo ./demo_tsndrive -i eth0 -t 1 -b 0 -o 300000 -r 0 -w 100000 -s 50000
```


## Program structure
During execution the tsndrive spawns a single realtime thread. In its loop, this thread first tries to receive one control packet, then calculates updates for the current position values and sends the new values out over the network using *SO_TXTIME* sockets. Then the thread sleeps until the next packet with control information is expected. There is some time configurable time buffer in place. This way resources can be saved and the application does not block with busy waiting. Through the specified timing parameters synchronization with the communication cycle is possible. This however depends heavily on the realtime execution properties of the operating system with the scheduling scheme its using and the system clock in conjunction with *clock_nanosleep()*. Currently only a single control packet is handled within one cycle.

### Definition and data containers
The *demo_tsndrive* application uses some definitions and data structures to enable adaptions of values to the execution environment and to organize values.

#### Timing definitions
Some timing values are essential to the successful execution of the applications. These are not constant across every hardware and implementation but should be fixed on each machine. Therefore these values are adaptable through precompiler defines. Theses values are (all values in nano seconds):
- Sending stack duration; which is the time interval the sending stack needs to send a packet from the socket to the hardware.
- Receiving stack duration; which is the time interval the sending stack needs to receive a packet from the hardware and make is available at the socket.
- Application send wake up; which is the time interval the application needs between it's wake up and it having a packet ready to send.
- Application receive wake up; which is the time interval the application needs between it's wake up and it being ready to receive a packet.
- Maximum wake up jitter; which is the largest time interval between a planned wake up and the actual wake up of the application

**These values must be tuned for each hardware platform the application is executed on to get best performance!**

#### Configuration options structure (cnfg_optns_t)
This structure hold the configuration options which are most set through the command-line interface (see [Command Line Arguments](#command-line-arguments)). Additionally the multicast MAC addresses which are used in the AccessTSN industrial USe Case Demo are stored in this structure. 

#### TSN drive structure (tsndrive_t)
The TSNdrive structure stores the information on a single instance of a TSNdrive. It was designed that way to have an option to be able to support multiple instances in the future. The stored information is:
- configuration options (see above)
- send and receive sockets
- packet storage; a preallocated memory pool to store packets
- simulated axes; the information on the axes simulated be the application
- handle of the real-time thread and it's attributes


### Main program path (*demo_tsnsender.c/main*)
Upon start of the tsndrive it executes following functions in the given order:
1. Set standard values:  
   A *drivesim* struct is created. This holds the necessary information for the demo-tsndrive instance. 
1. Read and process command line arguments (*demo_tsndrive.c/evalCLI*):  
   The command line is parsed and the *cnfg_optns* struct is filled with the specified values.
1. Initialization (*demo_tsndrive.c/init*):  
   1. Standard values like the multicast MAC addresses are initialized.
   1. Open send and receive sockets (*demo_tsndrive.c/opntxsckt*; *packet_handler.h/opnrxsckt*)
   1. Init packet storage (*packet_handler.c/initpktstrg): To not allocate memory during the the realtime threads, a packet storage  to hold send and receive packets is created and the necessary memory allocated.
   1. Create correct number of axis, allocate necessary memory and initialize the created axes (*axis_sim.c/axes_initreq*).
   1. Lock memory pages.
   1. Setup real-time thread including setting scheduling policy and priority.
1. Register signal handlers:  
   *SIGTERM * and *SIGINT* handlers are registered. Both will set a *run* variable to zero and *SIGINT* will terminate the execution on the second try.
1. Create real-time thread
1. Wait until stop/termination:  
   Sleep in while-loop until *run* variable is set to zero. Sleep duration is set to one second.
1. Cleanup (*demo_tsndrive.c/cleanup*):  
   1. Cancel thread
   2. Close sockets
   4. Destroy packet storage: Clear and free memory (*packet_handler.c/destroypktstrg*)
   5. Free memory of standard values like MAC addresses.
   6. Free memory of created axes.
1. Exit

### Real-Time Thread (*demo_tsndrive.c/rt_thrd*)
The real-time thread operates the execution loop. It tries to receive packets, calculates position value updates, created new packets, sends the new packets at the correct time and sleeps till the next iteration. To do that, the following steps in the given order are necessary:
1. Thread initialization:  
   * Get and prepare the sending MAC-Addresses (one for each axis) from Network Interface (*packet_handler.h/fillethaddr*)
   * Get current (system) time and calculate point in time for first execution as well as first TxTime. The calculation is based on the the base time of the cycle, and timing values concerning the duration/latency of application wake-up and execution. (*time_calc.c*)
1. Sleep till first execution.
1. Execution loop (infinite):  
   1. Receive packet (*demo_tsndrive.c/rcv_cntrlmsg*).
   1. Update enable values for each axis (*axis_sim.c/axes_updt_enbl*).
   1. For each axis:  
      1. Calculate new values (*axis_sim.c/axs_fineclcpstn*).
      1. Create new packet, insert and send new axis values (*demo_tsndrive.c/snd_axsmsg*).
   1. Update velocity values for each axis (*axis_sim.c/axes_updt_setvel*).
   1. Increase time values (next execution an TxTime) by one cycle.
   1. Sleep till next execution using *clock_nanosleep*.

### Receive Control Information Function (*demo_tsndrive.c/rcv_cntrlmsg*)
This function handles the receiving of packets with control messages. It checks for packets, receives them, extracts the received information and formats it into a control information struct *cntrlnfo*. The function tries to receive and handles only a single packet from the receive MAC-address in one cycle. The function returns a *0* for a successful execution, a *1* in case of an error or a *-1* if no packet was available for receiving. The function performs the following steps in the given order:
1. Calculate various timeout for receiving a packet.
1. Check if packet is ready to be received using *poll* on the RX sockets with a timeout.
   It no packet is ready after timeout, return with a return code of *-1*.
1. Get memory for packet from preallocated pool (packet storage) (*packet_handler.c/getfreepkt*) and receive packet into that memory (*packet_handler.c/rcvpkt*). 
1. Check destination MAC-Address of the packet and compare it to the specified receiving MAC-Addresses (*packet_handler.c/chckethhdr*).
1. Parse the packet content (*packet_handler.c/prspkt*) and check it's headers (packet_handler.c/chckpkthdrs*).
1. Parse dataset message out of received packet (*packet_handler.c/prsdtstmsg*).
1. Extract control information out of the dataset message (*packet_handler.c/prscntrlmsg*) and write the information to a control information struct.
1. Return used packet back to memory pool (*packet_handler.c/retusedpkt*)

### Send Axis Information Function (*demo_tsndrive.c/snd_axsmsg*)
This function handles the creation and sending of packets with axis messages. It creates packets, fills them with the updated axis information (e.g. current position values) and sends the packet at the correct time. The function only handles a single axis during each execution. The function returns a *0* for a successful execution or a *1* in case of an error. The function performs the following steps in the given order:
1. Calculate TxTime for specified axis.
1. Get memory for packet from preallocated pool (packet storage) (*packet_handler.c/getfreepkt*) and fill packet headers (*packet_handler.c/setpkt*).
1. Fill packet with axis information (*packet_handler.c/fillaxspkt*).
1. Send packet with TxTime and increase count for sent packets: (*packet_handler.c/sendpkt*).
1. Return used packet back to memory pool (*packet_handler.c/retusedpkt*).
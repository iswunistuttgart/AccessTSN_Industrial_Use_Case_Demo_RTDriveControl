# AccessTSN Industrial Use Case Demo - RTDriveControl: Documentation of the Sender-Component "demo_tsnsender"
## Execution and Command line arguments

### Introduction
The Sender-Component "demo_tsnsender" represents the networking portion of the control of the basic milling machine in the AccessTSN Industrial Use Case Demo. It is a part of the real-time control loop and one of the components in the **Real-time drive control** communication relationship. The application interfaces with the Hardware abstraction layer (HAL) of the computer numerical control (CNC) application (In the use case Machinekit and LinuxCNC are used. See [https://github.com/iswunistuttgart/AccessTSN_Industrial_Use_Case_Demo_CNC].) through a shared memory. The application takes set-point values from the shared memory and send them out over the TSN network. It receives current (control) values over the TSN network and writes them to the shared memory. In combination with the "demo_tsndrive" in complete the control loop ove the network.

### Requirements for execution
The *demo_tsnsender* application utilizes some low level kernel functions. On most system not every user is allowed to use theses. Therefore the application needs to be executed by a user with the following privileges:
- Opening a "AF_PACKET"-(RAW)Socket
- Opening/Creating the required shared memories and semaphores
- Change the scheduling policy and priority for the spawned threads
- Send packets with the *SO_TXTime* socket option
For testing purposes the application is mostly executed as *root* with the *sudo* command.

Futhermore the application uses some functionality of the Linux network stack, which is not available or configured by default on every system. The system on which the application is executed should support:
- Opening and using a socket with the *SO_TXTIME* socket option. For this to function correctly, the *ETF* scheduling qdisc should be configured in the traffic control subsystem. Explanations on how to configure the ETF qdisc can be found in the [TSN Documentation Project for Linux](https://tsn.readthedocs.io/qdiscs.html#configuring-the-etf-qdisc). An example script on how to do this is included in the AccessTSN industrial Use Case Demo repositories.

### Command line arguments
To change the behavior of the application various arguments can be specified through the command line. some of the arguments have default values, theses are therefore optional. A help is printed when starting the application with the *-h* cli switch.

|Switch| Description | Default-Value |
|--------------------|--------------|---------------|
|-t [value] | Specifies update-period in microseconds.| 1 millisecond|
|-b [value]  | Specifies the basetime (the start of the cycle). This is a Unix-Timestamp. ||
|-o [nanosec] | Specifies the sending offset, time between start of cycle and sending slot in nano seconds. ||
|-r [nanosec] | Specifies the receiving offset, time between start of cycle and end of receive slot in nano seconds.||
|-w [nanosec]  | Specifies the receive window duration, timeinterval in which a packet is expected in nano seconds.||
|-i                  | Name of the Networkinterface to use.||
|-p                   | PublisherID e.g. TalkerID,| 0xAC00|
|-y                  | Priority of sending socket (can be 1-7) |6|
|-h                  | Prints help message and exits||

An execution command for the suggested schedule of the AccessTSN Industrial Use Case demo could look like (change network interface to used system):

```Shell
sudo ./demo_tsnsender -i eth0 -t 1 -b 0 -o 0 -r 300000 -w 200000
```


## Program structure
During execution the tsnsender spawns two independent realtime threads: one for sending and one for receiving. Since the tsnsender only forwards information from a shared memory to the network and vice versa and has no influence when new values (in the shared memory or on the network) are calculated both threads can be independently. Through the specified timing parameters synchronization is possible. This however depends heavily on the realtime execution properties of the operating system with the scheduling scheme its using and the system clock in conjunction with *clock_nanosleep()*.

### Definition and data containers
The *demo_tsnsender* application uses some definitions and data structures to enable adaptions of values to the execution environment and to organize values.

#### Timing definitions
Some timing values are essential to the successful execution of the applications. These are not constant across every hardware and implementation but should be fixed on each machine. Therefore these values are adaptable through pre compiler defines. Theses values are (all values in nano seconds):
- Sending stack duration; which is the time interval the sending stack needs to send a packet from the socket to the hardware.
- Receiving stack duration; which is the time interval the sending stack needs to receive a packet from the hardware and make is available at the socket.
- Application send wake up; which is the time interval the application needs between it's wake up and it having a packet ready to send.
- Application receive wake up; which is the time interval the application needs between it's wake up and it being ready to receive a packet.
- Maximum wake up jitter; which is the largest time interval between a planned wake up and the actual wake up of the application

**These values must be tuned for each hardware platform the application is executed on to get best performance!**

#### Configuration options structure (cnfg_optns_t)
This structure hold the configuration options which are most set through the command-line interface (see [Command Line Arguments](#command-line-arguments)). Additionally the multicast MAC addresses which are used in the AccessTSN industrial USe Case Demo are stored in this structure. 

#### TSN sender structure (tsnsender_t)
The TSNsender structure stores the information on a single instance of a TSNsender. It was designed that way to have an option to be able to support multiple instances in the future. The stored information is:
- configuration options (see above)
- send and receive sockets
- handles ot the shared memories and semaphores which are used to exchange data with the CNC control component
- packet storage; a preallocated memory pool to store packets
- handles of the real-time threads (RX and TX) and their attributes



### Main program path (*demo_tsnsender.c/main*)
Upon start of the tsnsender it executes following functions in the given order:
1. Set standard values:  
   A *tsnsender* struct is created. This holds the necessary information for the demo-tsnsender instance. Then the multicast MAC addresses are initialized.
1. Read and process command line arguments (*demo_tsnsender.c/evalCLI*):  
   The command line is parsed and the *cnfg_optns* struct is filled with the specified values.
1. Initialization (*demo_tsnsender.c/init*):  
   1. Open send and receive sockets (*demo_tsnsender.c/opntxsckt*; *packet_handler.h/opnrxsckt*)
   1. Open shared memories and necessary semaphores to lock shared memories in case of writing. Shared memories will be created if necessary. (*axisshm_handler.h/opnShM_[...]*)
   1. Init packet storage (*packet_handler.c/initpktstrg): To not allocate memory during the the realtime threads, a packet storage  to hold send and receive packets is created and the necessary memory allocated.
   1. Lock memory pages.
   1. Setup send and receive thread including setting scheduling policy and priority.
1. Register signal handlers:  
   *SIGTERM * and *SIGINT* handlers are registered. Both will set a *run* variable to zero and *SIGINT* will terminate the execution on the second try.
1. Create send and receive thread.
1. Wait until stop/termination:  
   Sleep in while-loop until *run* variable is set to zero. Sleep duration is set to one second.
1. Cleanup (*demo_tsnsender.c/cleanup*):  
   1. Cancel threads
   2. Close sockets
   3. Close shared memories and semaphores. If this is last instance accessing the resources, they are deleted (*packet_handler.c/close[...]ShM*)
   4. Destroy packet storage: Clear and free memory (*packet_handler.c/destroypktstrg*)
   5. Free memory of standard values like MAC addresses.
1. Exit

### Send Thread (*demo_tsnsender.c/rt_thrd*)
The send thread operates the sending loop. It takes information from the shared memory, created a packet, inserts the information, sends the packet at the correct time and sleeps till the next iteration. To do that, the following steps in the given order are necessary:
1. Thread initialization:  
   * Get and prepare the sending MAC-Address from Network Interface (*packet_handler.h/fillethaddr*)
   * Get current (system) time and calculate point in time for first execution as well as first TxTime. The calculation is based on the the base time of the cycle, and timing values concerning the duration/latency of application wake-up and execution. (*time_calc.c*)
1. Sleep till first execution.
1. Execution loop (infinite):  
   1. Read TX values from shared memory (*axisshm_handler.c/rd_shm2cntrlinfo*)
   1. Get memory for packet from preallocated pool (packet storage) (*packet_handler.c/getfreepkt*) and fill packet headers (*packet_handler.c/setpkt*). 
   1. Fill packet with TX values from shared memory. (*packet_handler.c/fillcntrlpkt*)
   1. Send packet with TxTime and increase count for sent packets: (*packet_handler.c/sendpkt*)
   1. Return used packet back to memory pool (*packet_handler.c/retusedpkt*)
   1. Increase time value by one cycle.
   1. Calculate point in time for next execution and next TxTime.
   1. Sleep till next execution using *clock_nanosleep*.

### Receive Thread (*demo_tsnsender.c/rx_thrd*)
The receive thread operates the receiving loop. It checks for packets, receives them, extracts the received information and writes the information to the shared memory. The thread tries to receive as many packets as specified receive MAC-addresses in one cycle. To do that, the following steps in the given order are necessary:
1. Thread initialization:  
   * Calculate various timeout for receiving a packet.
   * Get current (system) time and calculate point in time for first execution. The calculation is based on the the base time of the cycle, and timing values concerning the duration/latency of application wake-up and execution. (*time_calc.c*)
1. Execution loop (infinite):  
   1. Check how many packets were received in current cycle.  
      If all packets for this cycle were receive (or the respective timeout expired):  
      1. Increase time value by one cycle.
      1. Calculate point in time for next execution.
      1. Sleep till next execution using *clock_nanosleep*.
   1. Check if packet is ready to be received using *poll* on the RX sockets with a timeout.
      It no packet is ready after timeout, skip to next iteration of loop.
   1. Get memory for packet from preallocated pool (packet storage) (*packet_handler.c/getfreepkt*) and receive packet into that memory (*packet_handler.c/rcvpkt*). 
   1. Check destination MAC-Address of the packet and compare it to the specified receiving MAC-Addresses (*packet_handler.c/chckethhdr*).
   1. Parse the packet content (*packet_handler.c/prspkt*) and check it's headers (packet_handler.c/chckpkthdrs*).
   1. Parse dataset messages out of received packet (*packet_handler.c/prsdtstmsg*).
   1. Extract axis information out of the dataset messages (*packet_handler.c/prsaxsmsg*) and write the information to the shared memory (*axisshm_handler.c/wrt_axsinfo2shm*).
   1. Return used packet back to memory pool (*packet_handler.c/retusedpkt*)
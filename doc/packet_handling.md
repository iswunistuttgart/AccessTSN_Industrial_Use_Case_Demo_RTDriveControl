# AccessTSN Industrial Use Case Demo - RTDriveControl: Documentation of the Packet Handling
The purpose of the applications is to communicate information between the CNC component of the AccessTSN industrial Use Case Demo and the (simulated) axes of a CNC machine over a TSN enabled ethernet network. The information which is to be transmitted is packeted into Ethernet frames and send out on the network. Also received Ethernet frames are parsed and the relevant information is extracted. The *packet_handler.h* and packet_handler.c* files implement the required functionality to communicate over the network. 

## Program structure and assumptions
The files define data containers to efficiently handle Ethernet frames containing layered protocols. The data structures are modelled after the socket buffers used in the Linux Networking Stack. Also a package storage is implemented which takes care of the memory management for packets. This helps with real-time execution as no memory allocation for packets is necessary from within the application after the initialization.
The used frame formatting is explained in [a separate document](communication_frameformat.md).

### Definition and data containers
The  applications use some definitions and data structures to enable adaptions of values to the execution environment and to organize values. The definitions and data containers which are concerned with the network operations are defined in *packet_handler.h*

The data containers which represent the Ethernet frame and the included OPC UA message must not include any padding for better memory alignment. All data fields of these containers need to follow directly its predecessor. Otherwise the ethernet frames are not formed correctly or the parsing fails.

#### Definitions of the values for OPC UA data fields
Some of the data fields in the Ethernet frames and the included OPC UA message have fixed values which define the structure of the data fields in the frame. Since theses values directly influence the implementation theses are hardcoded at the related places in the source code. 

Other data fields have constant values which is used for identification and management of the messages. These values can be different in each setup. For easier adaptation of these variables they are available through precompiler defines. Available variables are (provided values in brackets):
- Writer Group ID (0x1000); Identifier for the OPC UA PubSub writerGroup; a group of multiple Writers
- Group Version (0x26DEFA00); defined as seconds since January 1st, 2000; creation time here: Sept 1, 2020
- Writer IDs: Identifier for the OPC UA PubSub DataSetWriter, an identifier for the writer of a published data set; here writerID are defined for each dataset published by the four axis and the control

#### Definitions of values for the Ethernet frame fields
The communication scheme used by the applications is a publish/subscribe scheme. Information is not addressed to a certain physical endpoint but to a endpoint independent address. endpoint interested in the information then can subscribe and listen to these independent addresses. With Ethernet frames Multicast addresses are used for these addresses. Following Multicast addresses can be adapted through precompiler definitions:
- Destination address for frames with control information
- Destination addresses for frames with axis information for each axis
Additionally the value of the Ethertype field is defined by a precompiler definition. The here used value *0xB62C* specifies a UADP over layer 2 message.

#### Definitions for memory management
The maximum size (ion Bytes) of a packet is also specified by a precompiler definition. It is used to calculate the amount of memory which should be allocated for the packet storage during the initialization phase of the applications. This value can be optimized for a better memory footprint of the applications.

#### Ethernet header struct (eth_hdr_t*)
The Ethernet header struct represents the header of an Ethernet frame with the destination and source address as well as the Ethertype field.

#### NetworkMessage Header struct (*ntwrkmsg_hdr_t*)
This struct contains data field for the version and extended flags as well as for the publisher ID. It represents the NetworkMessage header of a UADP message.

#### GroupHeader struct (*grp_hdr_t*)
Containing data fields for the group flags, the writer group ID, the group version and the message sequence number, this struct represents the GroupHeader of a UADP Networkmessage.

#### Payload Header struct (*pyld_hdr_t*)
The payload header struct represents the DataSet payload header of a UADP message. It consists of two fields, one for the message count and one for an array of DataSetWriterIDs. In the implementation the field for the writerIDs is used as a starting memory address for the array and to define the variable type since the array size is not constant and unknown at compile time.

#### Extended Network Message Header struct (*extntwrkmsg_hdr_t*)
This struct represents the Extended NetworkMessage header of a UADP Networkmessage. In the used configuration it contains a single data field for the timestamp.

#### Size Array struct (*szrry_t*)
The size array struct represents the beginning of the payload in an UADP NetworkMessage. At the beginning of the payload an array with the sizes of each DataSetMessage in the payload is placed. In case of only a single contained DataSetMessage the size array is omitted. Because ot that, this implementation uses the size array struct as the a starting memory address for the array and to define the variable type. The actual omittance of the data field is directly decided in the parsing and filling functions based on the value of the message count data field.

#### Struct for DataSet Message for Control information (*dtstmsg_cntrl_t*)
This custom data struct defines the data fields and the structure of a DataSet Message which contains the information from the CNC control to the drives. It starts with the necessary DataSet Message header followed by a data field for the number of following data fields. Then the four velocity set points for the axes and spindle followed by the four enable variables are placed. Afterwards follows the variables for the spindle brake, the machine status and the status of the emergency stop.

#### Struct for DataSet Message for axis information (*dtstmsg_axs_t*)
The second custom data struct defines the data fields and the structure of a DataSet Message which contains the information from a single drive to the CNC control. It starts with the necessary DataSet Message header followed by a data field for the number of following data fields. Then the variable for the current position is placed before the variable representing a fault.

#### DataSet Message union (*dtstmsg_t*)
For implementation purposes a union is used to be able to address both custom DataSet Messages (control and axis information) arbitrarily. 

#### Network packet struct (*rt_pkt_t*)
This structure is designed to hold a Ethernet frame and make the various data fields easily accessible without to much memory usage and copying. The design is modeled after the socket buffers of the Linux Network stack. A common data member (buffer) holds the entire memory of a frame. The other data members of the struct contain pointers to the memory locations in the common data member of the various data fields of the frame represented by the above structs. Additionally a data member (length) contains the number of valid bytes in the buffer.

#### Message Type Enumeration
An enumeration defines the possible message types. Currently only the two types: Control anx Axis are available.

### Packet storage definitions
A small memory management is implemented to handle hte necessary memory fór multiple packets. It is called the packet storage.

#### Packet storage element struct (*pktstrelmt_t*)
This struct represents a single entry in the packet storage. It consists of two data fields: One for the packet and an indicator if the packet is in use or not.

#### Packet storage struct (*pktstore_t*)
The struct representing the packet storage contains two data fields: One pointer to a packet store element and the size value of the packet store (in packets).

### Functions

#### Initialize packet headers (*packet_handler.c/initpkthdrs*)
This function sets the constants in the network message and group headers. While the flag values are hardcoded since they influence the implementation, for the identification values the values defined by precompiler definition are used.

#### Set packet (*packet_handler.c/setpkt*)
This functions prepares the supplied network packets struct (*rt_pkt_t*) and prepared it for usage. It sets the included memory buffer to zero and sets the field pointers to the fitting memory locations of the memory buffer according to the message type. The correct values for the field pointers are calculated based on the definitions of the structs above. The data set message header is set with the values corresponding to the required message type. Finally the function calls the *initpkthdrs* function to set the other headers of the message. Currently this functions only supports a single type of DataSetMessage per packet.

#### Packet creation (*packet_handler.c/createpkt*)
The memory required for the packet structure as well as for the packet buffer isa allocated by this function. It is used by the packet storage in initialization phase.

#### Packet destruction (*packet_handler.c/destroypkt*)
The pointers of the supplied packet struct are NUlled, the memory of the packet buffer and of the packet struct itself if freed. 

#### Filling a control message packet with information (*packet_handler.c/fillcntrlpkt*)
The function writes the control information to a prepared packet. For that it converts the the values to network byte order after converting double to integers values using the included *dbl2nint64* function. It also adds the current time to the extended network message header in UA time format. Currently this function only support a single control message per packet.

#### Filling a axis message packet with information (*packet_handler.c/fillaxspkt*)
The function writes the axis information to a prepared packet. For that it converts the the values to network byte order after converting double to integers values using the included *dbl2nint64* function. Depending on the axis it choses the correct WriterID.  It also adds the current time to the extended network message header in UA time format. Currently this function only support a single control message per packet.

#### Convert double to integer (nano-value) (*packet_handler.c/dbl2nint64*)
To have a common encoding and understanding of double values on the network this function converts double values to 64 Bit integers. Assuming all values are within a fitting range the doubles are simply multiplied by 10⁹.

#### Convert interger to double (nano-value) *packet_handler.c/nint642dbl*)
To have a common encoding and understanding of double values on the network this function converts 64 Bit integers to double values. Assuming all values are within a fitting range the integers are simply multiplied by 10⁻⁹.

#### Fill Ethernet address (*packet_handler.c/fillethaddr*)
this function fills the socket address structure (*sockaddr_ll*) which is necessary for sending a packet with the required values. Based on the name of the network interface it determines the interface index. Then it sets the required values for a *AF_PACKET* address.

#### Send a packet with TxTime (*packet_handler.c/sendpkt*)
To send a packet at a specific time, the TxTime socket option is used. To use this option a *msg_hdr* struct is necessary to configure the option. This function first sets the values of a *msg_hdr* including the memory address of the buffer filled with the packet. Then it configures the usage of the TxTime option and writes the TxTime timestamp to the *msg_hdr* struct. Finally it uses the *sendmsg* of the Linux Network stack to send the packet.

#### Open a receive socket (*packet_handler.c/opnrxsckt*)
This function opens a raw (*SOCK_RAW*) *AF_PACKET* socket with the defined Ethertype. It allows the socket to be reused and gets the interface index. Then it configures the interface to receive multicast packets and adds the specified multicast addressed to the receive list.

#### Receive a packet (*packet_handler.c/rcvpkt*)
This function gets a packet from the socket. It first prepares a *msg_iov* struct with the buffer of the supplied packet struct and the receives a packet from the socket using the Linux network stacks *recvmsg* function. Before returning the functions checks if the packet was truncated. Only packets which are not truncated are forwarded.

#### Check Ethernet header (*packet_handler.c/chckethhdr*)
To make sure a packet is supposed to used by the application, this function is used to check the Ethernet header of the received packet. It checks the Ethertype and compares the packet's destination MAC address to the applications list of specified receive addresses. 

#### Check Message Header (*packet_handler.c/chckmsghdr*)
To check if a packet can be parsed by the application this function checks the packet's message data (the received *msg_hdr* struct) for the correct packet type, Ethertype and MAC address.

#### Parse a packet (*packet_handler.c/prspkt*)
This function parses a received Ethernet packet. In this context this means it sets the field pointers of supplied packet struct to the correct memory locations in the packet buffer based on the packet structure. the function calculated te memory locations with the help of the data field definitions described above. It determines the number of included messages in the packet and the type of the OPC UA Pub/Sub DatasetMessage. The function takes padding of an axis message into account. Current the function only supports a single type of DatasetMessages within a packet.

#### Check UDAP Message headers (*packet_handler.c/chckpkthdrs*)
To ensure correct parsing of a OPC UA pub/sub message, this function checks the flags of the network message header. The function also checks the WriterGroupID with its group version to make sure that the information is meant for the application.

#### Parse OPC UA Pub/Sub DataSetMessage (*packet_handler.c/prsdtstmsg*)
This function extracts the DataSetMessages from a packet. Currently is only supports a single type of DataSetMessages per packet.

#### Parse Axis information from DataSetMessage (*packet_handler.c/prsaxsmsg*)
This function reads the information of an axis from a DatasetMessage of the *axis* type. It writes the information to an *axsnfo_t* struct. First it checks the header of the DataSetMessage for the correct values. Then it read and writes the axis' fault and current position value. It does the necessary conversion to host byte order and the custom conversion of integer to double values. For the later conversion it uses the *nint642dbl* function.

#### Parse Control information from DataSetMessage (*packet_handler.c/prscntrlmsg*)
This function reads the control information from a DatasetMessage of the *control* type. It writes the information to an *cntrlnfo_t* struct. First it checks the header of the DataSetMessage for the correct values. Then it read and writes the enable and set point velocity values of all the axes as well as the status values of the machine. It does the necessary conversion to host byte order and the custom conversion of integer to double values. For the later conversion it uses the *nint642dbl* function.

### Packet storage functions
The packet storage is a simple memory manager for packets. The storage is implemented as a continuous memory region. The access to the packet store elements is done through array indices. 

#### Initialization of the packet storage (*packet_handler.c/initpktstrg*)
To reserve enough memory this function first allocates memory for the specified number of packet store elements. Then it creates one packet for each allocated packet store element using the *createpkt* function. 

#### Destruction of the packet storage (*packet_handler.c/destroypktstrg*)
To cleanup this function first destroys every packet in the packet storage using the *destroypkt* function. Then it frees the memory previously allocated for the packet store elements and NULLs the packet stores structure fields.

#### Get an unused packet from the packet store (*packet_handler.c/getfreepkt*)
This function supplies an unused packet from the packet storage to the application. It searches the packet store for the first unused packet, sets the supplied packet pointer to the packet and sets the corresponding packet store element to used. 

#### Return a used packet to the packet store (*packet_handler.c/retusedpkt*)
This function returns a used packet from the application to the packet storage. It searches the packet store for the packet store element which contains the supplied packet. Then it resets the usage indicator and NULLs the application's packet pointer. 
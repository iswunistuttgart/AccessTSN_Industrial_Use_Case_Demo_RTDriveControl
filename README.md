# AccessTSN Industrial Use Case Demo - RTDriveControl
This is a demonstration to give an example on how to use Linux with the current support for Time Sensitive Networking (TSN) as an industrial endpoint. For this purpose the components of this demonstration form a converged network use case in an industrial setting. 

The Use Case Demonstration is aimed at the AccessTSN Endpoint Image and is tested with but should also work independently from the image.

This work is a result of the AccessTSN-Project. More information on the project as well as contact information are on the project's webpage: https://accesstsn.com

## Structure of the Industrial Use Case Demo Git-projects

The AccessTSN Industrial Use Case Demo consists of multiple Git-projects, each holding the code to one component of the demonstration. This is the repository for the RT drive communication. It includes a simple simulation of a drive using a PT2 behavior to calculate new position values from velocity setpoint values. The two applications which can be build from the code is a TSNsender and a TSNdrive. 
The TSNsender reads and writes data from/to a shared memory and send/received these value over the network using the TSN features of Linux. The shared memory acts as an interface to Machinekit-Instance running a 3-Axis CNC.
The TSNdrive receives the setpoint values from the TSNsender over the network, then calculates updated position values and returns them back to the TSNsender using the TSN features of Linux. For the Industrial Use Case Demo the applications should be run on different devices. Both applications are based on the *Simple Realtime Example* in the Linux Foundation Wiki.

The main repository for AccessTSN Industrial Use Case Demo can be found on Github: https://github.com/iswunistuttgart/AccessTSN_Industrial_Use_Case_Demo

## Building the application suing the provided build script
For automation purposes a build script _build.sh_ is provided. It fill first check for dependencies and then build the specified application(s).
The script can be called with: ```./buils.sh b```
This will build both applications, the TSNsender and the TSNdrive. To build only the TSNsender change the commandline option to ```s```. For the TSNdrive change it to ```d```. If no commandline option was supplied the script build both applications.

## Manual building the applications

The applications depend on the submodule ["AccessTSN Industrial Use Case Demo Common"](https://github.com/iswunistuttgart/AccessTSN_Industrial_Use_Case_Demo_Common) which contains the definitions of a shared memory for communication between the components of the Industrial use case demo. Before building the applications this submodule needs to be cloned and initialized: 

```shell
git submodule update --init --recursive
```

The applications then can be build using the provided makefile. The following makefile targets are available to choose which applications to build.

Both applications: ```make all```   
Only the TSNsender application: ```make demo_tsnsender```   
Only the TSNdrive application:   ``` make demo_tsndrive```

## Running the applications
Both applications run on the command-line and do not need a graphical user interface (GUI). For information on the command-line arguments and the execution requirements see the the documentation files ([TSNsender](doc/tsnsender.md); [TSNdrive](doc/tsndrive.md)). 

## Documentation
The documentation of the applications including the command-line arguments, the program structure and descriptions of the functions can be found in the _doc_ subdirectory. A description of the communication setup including the frame format can be found there as well.




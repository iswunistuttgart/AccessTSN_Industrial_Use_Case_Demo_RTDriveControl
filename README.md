# AccessTSN Industrial Use Case Demo - RTDriveControl
This is a demonstration to give an example on how to use Linux with the current support for Time Sensitive Networking (TSN) as an industrial endpoint. For this purpose the components of this demonstration form a converged network use case in an industrial setting. 

The Use Case Demonstration is aimed at the AccessTSN Endpoint Image and is tested with but should also work independently from the image.

This work is a result of the AccessTSN-Project. More information on the project as well as contact information are on the project's webpage: https://accesstsn.com

## Structure of the Industrial Use Case Demo Git-projects

The AccessTSN Industrial Use Case Demo consists of multiple Git-projects, each holding the code to one component of the demonstration. This is the repository for the RT drive communication. It includes a simple simulation of a drive using a PT2 behavior to calculate new position values from velocity setpoint values. The two applications which can be build from the code is a TSNsender and a TSNdrive. 
The TSNsender reads and writes data from/to a shared memory and send/received these value over the network using the TSN features of Linux. The shared memory acts as an interface to Machinekit-Instance running a 3-Axis CNC.
The TSNdrive receives the setpoint values from the TSNsender over the network, then calculates updated position values and returns them back to the TSNsender using the TSn features of Linux. For the Industrial Use Case Demo the applications should be run on different devices. Both applications are based on the *Simple Realtime Example* in the Linux Foundation Wiki.

The main repository for AccessTSN Industrial Use Case Demo can be found on Github: https://github.com/iswunistuttgart/AccessTSN_Industrial_Use_Case_Demo




# AccessTSN Industrial Use Case Demo - RTDriveControl: Documentation of the Shared Memory Handling
The purpose of the *demo_tsnsender* application is to communicate information between the CNC component of the AccessTSN industrial Use Case Demo and the (simulated) axes of a CNC machine. The interface to the CNC component is realized using shared memory. The *axisshm_handler.h* and axisshm_handler.c* files bundle the functionality to handle this shared memory interface. 

## Program structure and assumptions
The functions open and close the required shared memories and read or write the necessary information and values from them. The structure and references of the shared memory is included from the *common* project of the Industrial Use Case Demo as a submodule.

### Functions

#### Open Shared Memory for Control Information (*axisshm_handler.c/opnShM_cntrlnfo*)
This functions opens the shared memory *MK_MAINOUTKEY* which contains the control information (mainly set point values)origination from the CNC component. The function first tries to open an existing shared memory read only. It this fails it retries to create the shared memory with read/write permission. Then the shared memory is truncated and memory mapped. Then a semaphore to control write access to the shared memory is opened and if necessary created. If the shared memory was created while opening it, all values are initialized with zero and the write access is dropped afterwards. 

#### Open Shared Memory for additional Control Information (*axisshm_handler.c/opnShM_addcntrlnfo*)
This functions opens the shared memory *MK_ADDAOUTKEY* which contains additional control information origination from the CNC component. The function first tries to open an existing shared memory read only. It this fails it retries to create the shared memory with read/write permission. Then the shared memory is truncated and memory mapped. Then a semaphore to control write access to the shared memory is opened and if necessary created. If the shared memory was created while opening it, all values are initialized with zero and the write access is dropped afterwards.

#### Open Shared Memory for Axis Information (*axisshm_handler.c/opnShM_axsnfo*)
This functions opens the shared memory *MK_MAININKEY* which contains the current values origination from the axes. The function tries to open an existing shared memory read/write and creates it if necessary. Then the shared memory is truncated and memory mapped. Then a semaphore to control write access to the shared memory is created and opened. All values in the shared memory are initialized with zero.

#### Write axis information to shared memory (*axisshm_handler.c/wrt_axsinfo2shm*)
The information of a single axis is written to the corresponding shared memory by this function. After waiting for write access through the correct semaphore the current position value and the current fault value are written from the supplied axis information to the shared memory variables of the axis specified by the axisID in the axis information. After write access is returned through the semaphore.

#### Read axis control information from shared memory (*axisshm_handler.c/rd_shm2axscntrlinfo*)
The current position values are read from the *Maininput* shared memory to a control information struct. After waiting until a possible concurrent write access to the shared memory is finished, the position values are read from the shared memory.

#### Read control information from shared memory (*axisshm_handler.c/rd_shm2cntrlinfo*)
This functions fills a control information struct which the corresponding information from the *Mainout* shared memory. First the function waits until a possible concurrent write access to the shared memory is finished, then the values are read and the struct is filled.

#### Read additional control information from shared memory (*axisshm_handler.c/rd_shm2addcntrlinfo*)
This function reads the position set point values from the *Addoutput* shared memory are writes them to the supplied control information struct. First the function waits until a possible concurrent write access to the shared memory is finished, then the position set points are read and written to the struct.

#### Close control information shared memory (*axisshm_handler.c/clscntrlShM*)
This function closes the *Mainout* shared memory. It first unmaps the memory, clears the pointer and closes the corresponding semaphore. The shared memory is not unlinked, because the application only reads from the *Mainout* shared memory. By design policy the application which writes to the shared memory needs to unlink it after closing.

#### Close additional control information shared memory (*axisshm_handler.c/clsaddcntrlShM*)
This function closes the *Addout* shared memory. It first unmaps the memory, clears the pointer and closes the corresponding semaphore. The shared memory is not unlinked, because the application only reads from the *Addout* shared memory. By design policy the application which writes to the shared memory needs to unlink it after closing.

#### Close axes information shared memory (*axisshm_handler.c/clsaxsShM*)
This function closes the *Mainin* shared memory. It first unmaps the memory, clears the pointer and closes the corresponding semaphore. Afterwards the shared memory is unlinked, because the application writes to the *Mainin* shared memory. By design policy the application which writes to the shared memory needs to unlink it after closing.



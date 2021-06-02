#!/bin/bash
echo "Building of RTControl Component of the AccessTSN Industrial Use Case Demo"
BUILDCOMPONENT=$1
if [ -z "$BUILDCOMPONENT" ]
   then
      echo "No application to build was specified; Will build both applications."
      echo "Usage: specify 's' to build TSNsender-application; or"
      echo "       specify 'd' to build TSNdrive-application; or"
      echo "       specify 'b' to build both applications."
      BUILDCOMPONENT="b"
fi
echo "Checking for prerequisites"

if ! command -v gcc &> /dev/null
   then
       echo "Error: gcc not found! Please install gcc."
       exit 1
   else
       echo "gcc found"
fi

if ! command -v make &> /dev/null
   then
       echo "Error: make not found! Please install make."
       exit 1
   else
       echo "make found"
fi

if ! command -v dpkg &> /dev/null
   then
       echo "Error: dpkg not found! Please install dpkg."
       exit 1
   else
       echo "dpkg found"
fi

if ! command -v grep &> /dev/null
   then
       echo "Error: grep not found! Please install grep."
       exit 1
   else
       echo "grep found"
fi

dpkg -s build-essential &> /dev/null
rtn=$?
if [ $rtn -eq 0 ]
   then
       echo "build-essential installed"
   else
       echo "Error: build-essential not installed! Please install build-essential."
       exit 1
fi

if ! grep SO_TXTIME /usr/include/asm-generic/socket.h &> /dev/null
   then
      if  ! grep SCM_TXTIME /usr/include/asm-generic/socket.h &> /dev/null
         then
            echo "Error: Socket Option 'SO_TXTIME' or 'SCM_TXTIME' not found in socket.h. Please update headers and kernel to newer version with activated TXTIME option."
            exit 1
         else
            echo "Socket option SCM_TXTIME found."
      fi
   else 
      echo "Socket option SO_TXTIME found."
fi

echo "Cloning and updating submodules"
git submodule init
git submodule update

echo "Building specified applications using make"
make clean
case $BUILDCOMPONENT in
   b|B) echo "Building both applications"
        make all
        ;;
   s|S) echo "Building demo-TSNsender"
        make demo_tsnsender
        ;;
   d|D) echo "Building demo-TSNdrive"
        make demo_tsndrive
        ;;
   *)   echo "Error: specified application not available."
        echo "Specified was" echo $BUILDCOMPONENT
        exit 1
        ;;
esac

echo "Finished building application(s)."
echo "On how to executed the application please see the documentation in the 'doc' subdirectory."
echo "Or the CLI help using 'demo_tsnsender -h' or 'demo_tsndrive -h'."
exit 0



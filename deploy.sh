#!/bin/bash

# ssh -i ~/SailfishOS/vmshare/ssh/private_keys/sdk root@sailfish

set -xe
PATH=$PATH:~/SailfishOS/bin/

export OS_VERSION=5.0.0.62
export ARCHITECTURE=${ARCHITECTURE:-aarch64}
export DEV_DEVICE=Xperia

sfdk config "no-fix-version"
sfdk config "target=SailfishOS-${OS_VERSION}-${ARCHITECTURE}"
# sudo modprobe -r kvm_intel
sfdk build --enable-debug
# sfdk build-shell


sfdk config "device=${DEV_DEVICE}"
if [ $? -ne 0 ] ; then
  echo
  echo "Available devices:"
  sfdk device list 2> /dev/null
  exit 1;
fi

sfdk deploy --sdk --debug

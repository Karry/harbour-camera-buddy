#!/bin/bash

# ssh -i ~/SailfishOS/vmshare/ssh/private_keys/sdk root@sailfish

set -xe
PATH=$PATH:~/SailfishOS/bin/

export OS_VERSION=5.0.0.62

for ARCHITECTURE in aarch64 i486 armv7hl ; do
  sfdk config "no-fix-version"
  sfdk config "target=SailfishOS-${OS_VERSION}-${ARCHITECTURE}"
  sfdk build --enable-debug
done

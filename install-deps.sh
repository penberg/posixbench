#!/bin/bash

. /etc/os-release

if [[ "$ID" = "fedora" || "$ID" = "rhel" ]]; then
  dnf install make cmake gcc-c++ zlib-devel python3 hwloc-devel
elif [ "$ID" = "ubuntu" ]; then
  apt install make cmake gcc zlib1g-dev python3 libhwloc-dev
else
  echo "$0: $ID is not a supported OS."
fi

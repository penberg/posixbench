#!/bin/bash

model=$(awk -F: '/^model name/ {print $2; exit}' </proc/cpuinfo)

echo $model

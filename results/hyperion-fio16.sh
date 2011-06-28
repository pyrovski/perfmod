#!/bin/bash

for i in `seq 12 15` ; do echo 1600000 > /sys/devices/system/cpu/cpu$i/cpufreq/scaling_setspeed; done
for i in `seq 4 7` ; do echo 1600000 > /sys/devices/system/cpu/cpu$i/cpufreq/scaling_setspeed; done
for i in `seq 0 3` ; do echo 2667000 > /sys/devices/system/cpu/cpu$i/cpufreq/scaling_setspeed; done
for i in `seq 8 11` ; do echo 2667000 > /sys/devices/system/cpu/cpu$i/cpufreq/scaling_setspeed; done
echo 1600000 | tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_min_freq
echo 2667000 | tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_max_freq
mpirun -n 16 -bind-to-core ../test -1

#!/bin/bash
echo 32012491 | sudo -S echo
while :
do
    now=$(date +"%T")
    echo "========================================================================================================="
    echo "Current time : $now" 
    echo "---------------------------------------------------------------------------------------------------------"
    sudo tail -9 /var/log/kern.log
    echo "---------------------------------------------------------------------------------------------------------"
    cat /proc/meminfo | grep Mem
    echo "---------------------------------------------------------------------------------------------------------"
    #tail -10 /var/log/kern.log 
    cat /proc/kv_proc 
    echo "---------------------------------------------------------------------------------------------------------"
    sleep 1
done

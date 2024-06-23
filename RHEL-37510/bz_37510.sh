#!/bin/bash
set -e
set -x

# counter=0
# while true; do printf "ABCDEFGHIJKLMNOPQRSTUVWXYZ%05iabcdefghijklmnopqrstuvwxyz%07i" $((counter/1024)) $((counter/8)); counter=$((counter+1)); done | dd of=bz_37510.img bs=1M count=16 iflag=fullblock

modprobe brd rd_size=34816 rd_nr=5
vgcreate -s 512K vg /dev/ram*
lvcreate -aey -L 16M -n lv vg
dd if=bz_37510.img of=/dev/vg/lv
touch check_file
# while [ -e "check_file" ]; do cmp -n 16777216 bz_37510.img /dev/vg/lv; done &
./compare /dev/vg/lv 16777216 &
checkpid=$!

echo 'Convert linear -> raid1 (takeover)' | tee /dev/kmsg
lvconvert -y --type raid6 --stripes 3 --stripesize 64K --regionsize 128K vg/lv
echo 'Convert linear -> raid1 (takeover) Finished' | tee /dev/kmsg
while [ `dmsetup status vg/lv | tee /dev/stderr | awk '{print $8}'` != "idle" ]; do sleep .1 ; done
jobs -pr | grep -q $checkpid

echo 'Convert raid1 -> raid5_ls (takeover)' | tee /dev/kmsg
lvconvert -y --type raid6 --stripes 3 --stripesize 64K --regionsize 128K vg/lv
echo 'Convert raid1 -> raid5_ls (takeover) Finished' | tee /dev/kmsg
while [ `dmsetup status vg/lv | tee /dev/stderr | awk '{print $8}'` != "idle" ]; do sleep .1 ; done
jobs -pr | grep -q $checkpid

echo 'Convert raid5_ls adding stripes (reshape)' | tee /dev/kmsg
lvconvert -y --type raid6 --stripes 3 --stripesize 64K --regionsize 128K vg/lv
echo 'Convert raid5_ls adding stripes (reshape) Finished' | tee /dev/kmsg
while [ `dmsetup status vg/lv | tee /dev/stderr | awk '{print $8}'` != "idle" ]; do sleep .1 ; done
jobs -pr | grep -q $checkpid

echo 'Convert raid5_ls -> raid6_ls_6 (takeover)' | tee /dev/kmsg
lvconvert -y --type raid6 --stripes 3 --stripesize 64K --regionsize 128K vg/lv
echo 'Convert raid5_ls -> raid6_ls_6 (takeover) Finished' | tee /dev/kmsg
while [ `dmsetup status vg/lv | tee /dev/stderr | awk '{print $8}'` != "idle" ]; do sleep .1 ; done
jobs -pr | grep -q $checkpid

echo 'Convert raid6_ls_6 -> raid6(_zr) (reshape)' | tee /dev/kmsg
lvconvert -y --type raid6 --stripes 3 --stripesize 64K --regionsize 128K vg/lv
echo 'Convert raid6_ls_6 -> raid6(_zr) (reshape) Finished' | tee /dev/kmsg
while [ `dmsetup status vg/lv | tee /dev/stderr | awk '{print $8}'` != "idle" ]; do sleep .1 ; done
jobs -pr | grep -q $checkpid

echo 'Remove reshape space' | tee /dev/kmsg
lvconvert -y --type raid6 --stripes 3 --stripesize 64K --regionsize 128K vg/lv
echo 'Remove reshape space Finished' | tee /dev/kmsg
while [ `dmsetup status vg/lv | tee /dev/stderr | awk '{print $8}'` != "idle" ]; do sleep .1 ; done

rm check_file
wait $checkpid

vgremove -ff vg
pvremove /dev/ram*
rmmod brd

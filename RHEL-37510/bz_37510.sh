#!/bin/bash
set -e
set -x

# counter=0
# while true; do printf "ABCDEFGHIJKLMNOPQRSTUVWXYZ123456abcdefghijklmnopqrstuvwxyz%06i" $counter; counter=$((counter+1)); done | dd of=bz_37510.img bs=1M count=16 iflag=fullblock

modprobe brd rd_size=34816 rd_nr=5
vgcreate -s 512K vg /dev/ram*
lvcreate -aey -L 16M -n lv vg
dd if=bz_37510.img of=/dev/vg/lv
touch check_file
while [ -e "check_file" ]; do cmp -n 16777216 bz_37510.img /dev/vg/lv; done &
checkpid=$!

echo 'Convert linear -> raid1 (takeover)'
lvconvert -y --type raid6 --stripes 3 --stripesize 64K --regionsize 128K vg/lv
while [ `dmsetup status vg/lv | tee /dev/stderr | awk '{print $8}'` != "idle" ]; do sleep .1 ; done

echo 'Convert raid1 -> raid5_ls (takeover)'
lvconvert -y --type raid6 --stripes 3 --stripesize 64K --regionsize 128K vg/lv
while [ `dmsetup status vg/lv | tee /dev/stderr | awk '{print $8}'` != "idle" ]; do sleep .1 ; done

echo 'Convert raid5_ls adding stripes (reshape)'
lvconvert -y --type raid6 --stripes 3 --stripesize 64K --regionsize 128K vg/lv
while [ `dmsetup status vg/lv | tee /dev/stderr | awk '{print $8}'` != "idle" ]; do sleep .1 ; done

echo 'Convert raid5_ls -> raid6_ls_6 (takeover)'
lvconvert -y --type raid6 --stripes 3 --stripesize 64K --regionsize 128K vg/lv
while [ `dmsetup status vg/lv | tee /dev/stderr | awk '{print $8}'` != "idle" ]; do sleep .1 ; done

echo 'Convert raid6_ls_6 -> raid6(_zr) (reshape)'
lvconvert -y --type raid6 --stripes 3 --stripesize 64K --regionsize 128K vg/lv
while [ `dmsetup status vg/lv | tee /dev/stderr | awk '{print $8}'` != "idle" ]; do sleep .1 ; done

echo 'Remove reshape space'
lvconvert -y --type raid6 --stripes 3 --stripesize 64K --regionsize 128K vg/lv

rm check_file
wait $checkpid

vgremove -ff vg
pvremove /dev/ram*
rmmod brd

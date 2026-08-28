# test_iio 原始实验记录

> 本文件只保存板端真实终端输出，按实验时间顺序整理。
> 不在这里做结论分析；整理后的实验结论见 `test_iio_report.md`。
> 对原始串口中明显的粘连/控制字符只做换行和排版整理，不改变实验内容。

```text
EXT4-fs (mmcblk0p2): re-mounted. Opts: data=ordered
hwclock: can't open '/dev/misc/rtc': No such file or directory
Sat Aug 22 07:48:48 UTC 2026
hwclock: can't open '/dev/misc/rtc': No such file or directory
Starting internet superserver: inetd.
INIT: Entering runlevel: 5
Configuring network interfaces... IPv6: ADDRCONF(NETDEV_UP): eth0: link is not ready
udhcpc (v1.24.1) started
Sending discover...
xilinx_axienet 41000000.ethernet eth0: Link is Down
Sending discover...
Sending discover...
No lease, forking to background
done.
Starting system message bus: dbus.
Starting Dropbear SSH server: dropbear.
hwclock: can't open '/dev/misc/rtc': No such file or directory
/etc/rc5.d/S20iiod: line 25: log_daemon_msg: command not found
Starting syslogd/klogd: done
Starting tcf-agent: OK

Last login: Sat Aug 22 07:48:58 UTC 2026 on tty1

root@LIULIU-ZYNQ1:~# ls
root@LIULIU-ZYNQ1:~# ls
root@LIULIU-ZYNQ1:~# cd ..
root@LIULIU-ZYNQ1:/home# ls
root
root@LIULIU-ZYNQ1:/home# cd root
root@LIULIU-ZYNQ1:~# ls

root@LIULIU-ZYNQ1:~# df -h
Filesystem                Size      Used Available Use% Mounted on
/dev/root                14.1G     55.8M     13.3G   0% /
devtmpfs                240.8M      4.0K    240.7M   0% /dev
tmpfs                   249.3M     84.0K    249.2M   0% /run
tmpfs                   249.3M     44.0K    249.2M   0% /var/volatile
/dev/mmcblk0p1           98.4M      6.3M     92.2M   6% /run/media/mmcblk0p1

root@LIULIU-ZYNQ1:~# mount
/dev/root on / type ext4 (rw,relatime,data=ordered)
devtmpfs on /dev type devtmpfs (rw,relatime,size=246528k,nr_inodes=61632,mode=755)
proc on /proc type proc (rw,relatime)
sysfs on /sys type sysfs (rw,relatime)
configfs on /sys/kernel/config type configfs (rw,relatime)
tmpfs on /run type tmpfs (rw,nosuid,nodev,mode=755)
tmpfs on /var/volatile type tmpfs (rw,relatime)
/dev/mmcblk0p1 on /run/media/mmcblk0p1 type vfat (rw,relatime,gid=6,fmask=0007,dmask=0007,allow_utime=0020,codepage=437,iocharset=iso8859-1,shortname=mixed,errors=remount-ro)
devpts on /dev/pts type devpts (rw,relatime,gid=5,mode=620,ptmxmode=000)

root@LIULIU-ZYNQ1:~# ls

root@LIULIU-ZYNQ1:~# find / -name test_iio.ko 2>/dev/null
/root/linux_demo/iio/test_iio.ko

root@LIULIU-ZYNQ1:~# cd /root/linux_demo/iio
root@LIULIU-ZYNQ1:/root/linux_demo/iio# ls
test_iio.ko

root@LIULIU-ZYNQ1:/root/linux_demo/iio# insmod test_iio.ko
test_iio: loading out-of-tree module taints kernel.
test_iio test_iio: probing generic test IIO device
test_iio test_iio: registered IIO device "test-iio-adc"

root@LIULIU-ZYNQ1:/root/linux_demo/iio# dmesg | tail -30
sit: IPv6, IPv4 and MPLS over IPv4 tunneling driver
NET: Registered protocol family 17
can: controller area network core (rev 20170425 abi 9)
NET: Registered protocol family 29
can: raw protocol (rev 20170425)
can: broadcast manager protocol (rev 20170425 t)
can: netlink gateway (rev 20170425) max_hops=1
Registering SWP/SWPB emulation handler
mmc0: new high speed SDHC card at address 0001
mmcblk0: mmc0:0001 SD 14.6 GiB
 mmcblk0: p1 p2
hctosys: unable to open rtc device (rtc0)
of_cfs_init
of_cfs_init: OK
ALSA device list:
  No soundcards found.
EXT4-fs (mmcblk0p2): couldn't mount as ext3 due to feature incompatibilities
EXT4-fs (mmcblk0p2): mounted filesystem with ordered data mode. Opts: (null)
VFS: Mounted root (ext4 filesystem) on device 179:2.
devtmpfs: mounted
Freeing unused kernel memory: 1024K
udevd[689]: starting version 3.2.2
udevd[690]: starting eudev-3.2.2
FAT-fs (mmcblk0p1): Volume was not properly unmounted. Some data may be corrupt. Please run fsck.
EXT4-fs (mmcblk0p2): re-mounted. Opts: data=ordered
IPv6: ADDRCONF(NETDEV_UP): eth0: link is not ready
xilinx_axienet 41000000.ethernet eth0: Link is Down
test_iio: loading out-of-tree module taints kernel.
test_iio test_iio: probing generic test IIO device
test_iio test_iio: registered IIO device "test-iio-adc"

root@LIULIU-ZYNQ1:/root/linux_demo/iio# ls /sys/bus/iio/devices/
iio:device0  iio:device1

root@LIULIU-ZYNQ1:/root/linux_demo/iio# find /sys/bus/iio/devices -maxdepth 2 -type f | sort

root@LIULIU-ZYNQ1:/root/linux_demo/iio# cat /sys/bus/iio/devices/iio:device*/name
xadc
test-iio-adc

root@LIULIU-ZYNQ1:/root/linux_demo/iio# ls -l /sys/bus/iio/devices/iio:device1
lrwxrwxrwx    1 root     root             0 Aug 22 07:53 /sys/bus/iio/devices/iio:device1 -> ../../../devices/iio:device1

root@LIULIU-ZYNQ1:/root/linux_demo/iio# ls -la /sys/bus/iio/devices/iio:device1/
total 0
drwxr-xr-x    3 root     root             0 Aug 22 07:52 .
drwxr-xr-x   10 root     root             0 Jan  1  1970 ..
-r--r--r--    1 root     root          4096 Aug 22 07:55 dev
-rw-r--r--    1 root     root          4096 Aug 22 07:55 in_voltage0_raw
-rw-r--r--    1 root     root          4096 Aug 22 07:55 in_voltage1_raw
-rw-r--r--    1 root     root          4096 Aug 22 07:55 in_voltage_sampling_frequency
-rw-r--r--    1 root     root          4096 Aug 22 07:55 in_voltage_scale
-r--r--r--    1 root     root          4096 Aug 22 07:53 name
drwxr-xr-x    2 root     root             0 Aug 22 07:55 power
lrwxrwxrwx    1 root     root             0 Aug 22 07:55 subsystem -> ../../bus/iio
-rw-r--r--    1 root     root          4096 Aug 22 07:52 uevent

root@LIULIU-ZYNQ1:/root/linux_demo/iio# cat /sys/bus/iio/devices/iio:device1/name
test-iio-adc

root@LIULIU-ZYNQ1:/root/linux_demo/iio# cat /sys/bus/iio/devices/iio:device1/in_voltage0_raw
1

root@LIULIU-ZYNQ1:/root/linux_demo/iio# cat /sys/bus/iio/devices/iio:device1/in_voltage1_raw
-1

root@LIULIU-ZYNQ1:/root/linux_demo/iio# cat /sys/bus/iio/devices/iio:device1/in_voltage0_raw
2

root@LIULIU-ZYNQ1:/root/linux_demo/iio# cat /sys/bus/iio/devices/iio:device1/in_voltage0_raw
3

root@LIULIU-ZYNQ1:/root/linux_demo/iio# cat /sys/bus/iio/devices/iio:device1/in_voltage0_raw
4

root@LIULIU-ZYNQ1:/root/linux_demo/iio# ls /sys/bus/iio/devices/iio:device1/
dev                            in_voltage1_raw                in_voltage_scale               power                          uevent
in_voltage0_raw                in_voltage_sampling_frequency  name                           subsystem

root@LIULIU-ZYNQ1:/root/linux_demo/iio# cat /sys/bus/iio/devices/iio:device1/in_voltage0_raw
5

root@LIULIU-ZYNQ1:/root/linux_demo/iio# cat /sys/bus/iio/devices/iio:device1/in_voltage1_raw
-5

root@LIULIU-ZYNQ1:/root/linux_demo/iio# which iio_info
/usr/bin/iio_info

root@LIULIU-ZYNQ1:/root/linux_demo/iio# iio_info -u local:
Library version: 0.18 (git tag: v0.18)
Compiled with backends: local xml ip usb
IIO context created with local backend.
Backend version: 0.18 (git tag: v0.18)
Backend description string: Linux LIULIU-ZYNQ1 4.14.0-xilinx-v2018.3 #1 SMP PREEMPT Mon Mar 30 12:56:22 UTC 2026 armv7l
IIO context has 1 attributes:
        local,kernel: 4.14.0-xilinx-v2018.3
IIO context has 2 devices:
        iio:device0: xadc
                9 channels found:
                        voltage5: vccoddr (input)
                        2 channel-specific attributes found:
                                attr  0: raw value: 2017
                                attr  1: scale value: 0.732421875
                        voltage0: vccint (input)
                        2 channel-specific attributes found:
                                attr  0: raw value: 1345
                                attr  1: scale value: 0.732421875
                        voltage4: vccpaux (input)
                        2 channel-specific attributes found:
                                attr  0: raw value: 2439
                                attr  1: scale value: 0.732421875
                        temp0:  (input)
                        3 channel-specific attributes found:
                                attr  0: offset value: -2219
                                attr  1: raw value: 2669
                                attr  2: scale value: 123.040771484
                        voltage7: vrefn (input)
                        2 channel-specific attributes found:
                                attr  0: raw value: -13
                                attr  1: scale value: 0.732421875
                        voltage1: vccaux (input)
                        2 channel-specific attributes found:
                                attr  0: raw value: 2441
                                attr  1: scale value: 0.732421875
                        voltage2: vccbram (input)
                        2 channel-specific attributes found:
                                attr  0: raw value: 1347
                                attr  1: scale value: 0.732421875
                        voltage3: vccpint (input)
                        2 channel-specific attributes found:
                                attr  0: raw value: 1345
                                attr  1: scale value: 0.732421875
                        voltage6: vrefp (input)
                        2 channel-specific attributes found:
                                attr  0: raw value: 1689
                                attr  1: scale value: 0.732421875
                1 device-specific attributes found:
                                attr  0: sampling_frequency value: 961538
        iio:device1: test-iio-adc
                2 channels found:
                        voltage0:  (input)
                        3 channel-specific attributes found:
                                attr  0: raw value: 6
                                attr  1: sampling_frequency value: 1000
                                attr  2: scale value: 0.001000
                        voltage1:  (input)
                        3 channel-specific attributes found:
                                attr  0: raw value: -6
                                attr  1: sampling_frequency value: 1000
                                attr  2: scale value: 0.001000

root@LIULIU-ZYNQ1:/root/linux_demo/iio# ls /usr/bin/iio*
/usr/bin/iio_adi_xflow_check  /usr/bin/iio_genxml           /usr/bin/iio_readdev          /usr/bin/iio_writedev
/usr/bin/iio_attr             /usr/bin/iio_info             /usr/bin/iio_reg

root@LIULIU-ZYNQ1:/root/linux_demo/iio# iio_attr --help
Usage:
        iio_attr [OPTION]...    -d [device] [attr] [value]
                                -c [device] [channel] [attr] [value]
                                -B [device] [attr] [value]
                                -D [device] [attr] [value]
                                -C [attr]
Options:
        -h, --help           : Show this help and quit.
        -I, --ignore-case    : Ignore case distinctions.
        -q, --quiet          : Return result only.
        -a, --auto           : Use the first context found.
Optional qualifiers:
        -u, --uri            : Use the context at the provided URI.
        -i, --input-channel  : Filter Input Channels only.
        -o, --output-channel : Filter Output Channels only.
Attribute types:
        -s, --scan-channel   : Filter Scan Channels only.
        -d, --device-attr    : Read/Write device attributes
        -c, --channel-attr   : Read/Write channel attributes.
        -C, --context-attr   : Read IIO context attributes.
        -B, --buffer-attr    : Read/Write buffer attributes.
        -D, --debug-attr     : Read/Write debug attributes.

root@LIULIU-ZYNQ1:/root/linux_demo/iio# iio_attr -c test-iio-adc voltage0 raw
dev 'test-iio-adc', channel 'voltage0' (input), attr 'raw', value '7'

root@LIULIU-ZYNQ1:/root/linux_demo/iio# iio_attr -c test-iio-adc voltage1 raw
dev 'test-iio-adc', channel 'voltage1' (input), attr 'raw', value '-7'

root@LIULIU-ZYNQ1:/root/linux_demo/iio# iio_attr -c test-iio-adc voltage0 sampling_frequency 2000
dev 'test-iio-adc', channel 'voltage0' (input), attr 'sampling_frequency', value '1000'
wrote 5 bytes to sampling_frequency
dev 'test-iio-adc', channel 'voltage0' (input), attr 'sampling_frequency', value '2000'

root@LIULIU-ZYNQ1:/root/linux_demo/iio# iio_attr -c test-iio-adc voltage0 sampling_frequency
dev 'test-iio-adc', channel 'voltage0' (input), attr 'sampling_frequency', value '2000'

root@LIULIU-ZYNQ1:/root/linux_demo/iio# iio_attr -c test-iio-adc voltage1 sampling_frequency
dev 'test-iio-adc', channel 'voltage1' (input), attr 'sampling_frequency', value '2000'

root@LIULIU-ZYNQ1:/root/linux_demo/iio# cat /sys/bus/iio/devices/iio:device1/in_voltage_sampling_frequency
2000

root@LIULIU-ZYNQ1:/root/linux_demo/iio# iio_attr -c test-iio-adc voltage0 scale
dev 'test-iio-adc', channel 'voltage0' (input), attr 'scale', value '0.001000'

root@LIULIU-ZYNQ1:/root/linux_demo/iio# iio_attr -c test-iio-adc voltage0 sampling_frequency 0
dev 'test-iio-adc', channel 'voltage0' (input), attr 'sampling_frequency', value '2000'
error Invalid argument (-22) while writing 'sampling_frequency' with '0'
dev 'test-iio-adc', channel 'voltage0' (input), attr 'sampling_frequency', value '2000'

random: crng init done

root@LIULIU-ZYNQ1:/root/linux_demo/iio# c
-sh: c: command not found

root@LIULIU-ZYNQ1:/root/linux_demo/iio# rmmod test_iio
test_iio test_iio: removing test IIO device

root@LIULIU-ZYNQ1:/root/linux_demo/iio# dmesg | tail -10
udevd[690]: starting eudev-3.2.2
FAT-fs (mmcblk0p1): Volume was not properly unmounted. Some data may be corrupt. Please run fsck.
EXT4-fs (mmcblk0p2): re-mounted. Opts: data=ordered
IPv6: ADDRCONF(NETDEV_UP): eth0: link is not ready
xilinx_axienet 41000000.ethernet eth0: Link is Down
test_iio: loading out-of-tree module taints kernel.
test_iio test_iio: probing generic test IIO device
test_iio test_iio: registered IIO device "test-iio-adc"
random: crng init done
test_iio test_iio: removing test IIO device

root@LIULIU-ZYNQ1:/root/linux_demo/iio# ls /sys/bus/iio/devices/
iio:device0

root@LIULIU-ZYNQ1:/root/linux_demo/iio# cat /sys/bus/iio/devices/iio:device*/name
xadc

root@LIULIU-ZYNQ1:/root/linux_demo/iio# ls /sys/bus/iio/devices/
iio:device0

root@LIULIU-ZYNQ1:/root/linux_demo/iio# cat /sys/bus/iio/devices/iio:device*/name
xadc

root@LIULIU-ZYNQ1:/root/linux_demo/iio# cd /root/linux_demo/iio

root@LIULIU-ZYNQ1:/root/linux_demo/iio# insmod test_iio.ko
test_iio test_iio: probing generic test IIO device
test_iio test_iio: registered IIO device "test-iio-adc"

root@LIULIU-ZYNQ1:/root/linux_demo/iio# dmesg | tail -10
EXT4-fs (mmcblk0p2): re-mounted. Opts: data=ordered
IPv6: ADDRCONF(NETDEV_UP): eth0: link is not ready
xilinx_axienet 41000000.ethernet eth0: Link is Down
test_iio: loading out-of-tree module taints kernel.
test_iio test_iio: probing generic test IIO device
test_iio test_iio: registered IIO device "test-iio-adc"
random: crng init done
test_iio test_iio: removing test IIO device
test_iio test_iio: probing generic test IIO device
test_iio test_iio: registered IIO device "test-iio-adc"

root@LIULIU-ZYNQ1:/root/linux_demo/iio# ls /sys/bus/iio/devices/
iio:device0  iio:device1

root@LIULIU-ZYNQ1:/root/linux_demo/iio# cat /sys/bus/iio/devices/iio:device*/name
xadc
test-iio-adc

root@LIULIU-ZYNQ1:/root/linux_demo/iio# iio_attr -c test-iio-adc voltage0 sampling_frequency
dev 'test-iio-adc', channel 'voltage0' (input), attr 'sampling_frequency', value '1000'

root@LIULIU-ZYNQ1:/root/linux_demo/iio# iio_attr -c test-iio-adc voltage0 raw
dev 'test-iio-adc', channel 'voltage0' (input), attr 'raw', value '1'

root@LIULIU-ZYNQ1:/root/linux_demo/iio# iio_attr -c test-iio-adc voltage1 raw
dev 'test-iio-adc', channel 'voltage1' (input), attr 'raw', value '-1'

root@LIULIU-ZYNQ1:/root/linux_demo/iio#
```

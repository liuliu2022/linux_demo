# test_iio 实验报告

## 1. 实验目的

本实验用于验证自定义 `test_iio.ko` 外部内核模块是否能够在 Zynq + PetaLinux 2018.3 平台上：

1. 正常加载到目标 Linux 内核；
2. 成功执行驱动 `probe()`；
3. 注册为标准 Linux IIO 设备；
4. 在 sysfs 中生成预期的 IIO 属性；
5. 通过 `read_raw()` 路径读取测试数据；
6. 验证测试通道数据变化是否符合驱动设计。

当前阶段只验证 **IIO direct read / sysfs 属性访问**，暂未验证 IIO buffer、trigger、DMA 和 libiio buffer/refill。

---

## 2. 实验环境

- 目标平台：Zynq / ARMv7
- PetaLinux：2018.3
- Kernel：`4.14.0-xilinx-v2018.3`
- 模块：`test_iio.ko`
- 模块存放位置：`/root/linux_demo/iio/test_iio.ko`
- rootfs：SD 卡第二分区，ext4
- boot 分区：SD 卡第一分区，vfat

此前在 PC 端已经通过外部模块方式交叉编译并验证：

```text
ELF 32-bit LSB relocatable, ARM, EABI5
```

模块 `vermagic`：

```text
4.14.0-xilinx-v2018.3 SMP preempt mod_unload modversions ARMv7 p2v8
```

---

## 3. rootfs 与模块位置确认

### 3.1 文件系统挂载现象

执行：

```bash
df -h
```

板端输出：

```text
Filesystem                Size      Used Available Use% Mounted on
/dev/root                14.1G     55.8M     13.3G   0% /
devtmpfs                240.8M      4.0K    240.7M   0% /dev
tmpfs                   249.3M     84.0K    249.2M   0% /run
tmpfs                   249.3M     44.0K    249.2M   0% /var/volatile
/dev/mmcblk0p1           98.4M      6.3M     92.2M   6% /run/media/mmcblk0p1
```

执行：

```bash
mount
```

关键输出：

```text
/dev/root on / type ext4 (rw,relatime,data=ordered)
/dev/mmcblk0p1 on /run/media/mmcblk0p1 type vfat (...)
```

### 结论

- SD 卡第二分区作为根文件系统 `/` 使用，容量约 14.1 GiB；
- SD 卡第一分区是约 100 MiB 的 FAT boot 分区；
- 驱动实验文件放在 rootfs 中比放在 boot 分区更合理；
- 当前实验目录采用：

```text
/root/linux_demo/iio/
```

---

### 3.2 查找模块

执行：

```bash
find / -name test_iio.ko 2>/dev/null
```

输出：

```text
/root/linux_demo/iio/test_iio.ko
```

进入目录：

```bash
cd /root/linux_demo/iio
ls
```

输出：

```text
test_iio.ko
```

### 结论

模块已经成功通过 SD 卡复制到目标板 rootfs，板端可以直接访问该 `.ko` 文件。

---

## 4. 加载 test_iio 内核模块

执行：

```bash
insmod test_iio.ko
```

输出：

```text
test_iio: loading out-of-tree module taints kernel.
test_iio test_iio: probing generic test IIO device
test_iio test_iio: registered IIO device "test-iio-adc"
```

### 现象分析

第一行：

```text
test_iio: loading out-of-tree module taints kernel.
```

表示当前模块是通过内核源码树之外的方式构建并加载的外部模块，因此 Linux 将内核状态标记为 `tainted`。这不是模块加载失败，也不是驱动错误。

后两行说明：

```text
probe() 已经执行
        ↓
IIO device 注册流程已经执行
        ↓
设备名 test-iio-adc 注册成功
```

### 结论

`test_iio.ko` 成功加载，驱动 `probe()` 成功执行，并成功完成 IIO 设备注册。

---

## 5. dmesg 中的驱动日志

执行：

```bash
dmesg | tail -30
```

与本实验相关的关键输出：

```text
test_iio: loading out-of-tree module taints kernel.
test_iio test_iio: probing generic test IIO device
test_iio test_iio: registered IIO device "test-iio-adc"
```

系统中同时出现：

```text
FAT-fs (mmcblk0p1): Volume was not properly unmounted. Some data may be corrupt. Please run fsck.
```

该信息与 `test_iio` 驱动无关，表示 SD 卡 FAT boot 分区此前没有被完全正常卸载。以后在 PC 侧写入 SD 卡后，应执行：

```bash
sync
```

并安全卸载设备后再拔卡。

### 结论

内核日志中没有发现 `test_iio` 的注册错误，驱动初始化路径正常。

---

## 6. IIO 设备枚举

执行：

```bash
ls /sys/bus/iio/devices/
```

输出：

```text
iio:device0  iio:device1
```

读取设备名称：

```bash
cat /sys/bus/iio/devices/iio:device*/name
```

输出：

```text
xadc
test-iio-adc
```

因此可确定：

```text
iio:device0  -> Zynq 原有 XADC
iio:device1  -> 自定义 test_iio 驱动
```

### 结论

自定义驱动已经真正进入 Linux IIO 子系统，并获得标准 IIO 设备编号 `iio:device1`。

这说明当前链路已经走通：

```text
test_iio.ko
   ↓
probe()
   ↓
iio_device_register()
   ↓
Linux IIO core
   ↓
/sys/bus/iio/devices/iio:device1
```

---

## 7. IIO sysfs 节点检查

首先查看设备符号链接：

```bash
ls -l /sys/bus/iio/devices/iio:device1
```

输出：

```text
lrwxrwxrwx    1 root     root             0 Aug 22 07:53 /sys/bus/iio/devices/iio:device1 -> ../../../devices/iio:device1
```

这也解释了此前执行：

```bash
find /sys/bus/iio/devices -maxdepth 2 -type f | sort
```

没有输出的原因：`/sys/bus/iio/devices/iio:device1` 本身是一个符号链接，而 `find` 默认不会跟随该符号链接继续搜索。

查看设备目录：

```bash
ls -la /sys/bus/iio/devices/iio:device1/
```

输出：

```text
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
```

再次简化查看：

```bash
ls /sys/bus/iio/devices/iio:device1/
```

输出：

```text
dev                            in_voltage1_raw                in_voltage_scale               power                          uevent
in_voltage0_raw                in_voltage_sampling_frequency  name                           subsystem
```

### 结论

当前驱动已成功通过 IIO core 创建以下标准属性：

```text
name
in_voltage0_raw
in_voltage1_raw
in_voltage_scale
in_voltage_sampling_frequency
```

说明驱动中的 channel 描述、`iio_info` 和 sysfs ABI 已经开始正常工作。

当前还没有看到 `buffer/`、`scan_elements/` 等目录，符合当前 V1 版本尚未实现 buffered IIO 的设计目标。

---

## 8. 读取设备名称

执行：

```bash
cat /sys/bus/iio/devices/iio:device1/name
```

输出：

```text
test-iio-adc
```

### 结论

IIO device 的名称与驱动中注册的：

```text
test-iio-adc
```

一致，说明设备身份信息注册正确。

---

## 9. 读取 voltage0 / voltage1 原始数据

### 9.1 第一次读取

执行：

```bash
cat /sys/bus/iio/devices/iio:device1/in_voltage0_raw
```

输出：

```text
1
```

执行：

```bash
cat /sys/bus/iio/devices/iio:device1/in_voltage1_raw
```

输出：

```text
-1
```

---

### 9.2 连续读取 voltage0

继续执行：

```bash
cat /sys/bus/iio/devices/iio:device1/in_voltage0_raw
```

输出：

```text
2
```

再次执行：

```bash
cat /sys/bus/iio/devices/iio:device1/in_voltage0_raw
```

输出：

```text
3
```

再次执行：

```bash
cat /sys/bus/iio/devices/iio:device1/in_voltage0_raw
```

输出：

```text
4
```

随后再次读取：

```bash
cat /sys/bus/iio/devices/iio:device1/in_voltage0_raw
```

输出：

```text
5
```

再读取 voltage1：

```bash
cat /sys/bus/iio/devices/iio:device1/in_voltage1_raw
```

输出：

```text
-5
```

### 现象总结

实测数据序列表现为：

```text
voltage0:  1, 2, 3, 4, 5, ...
voltage1: -1,             -5, ...
```

当前测试驱动设计中：

```text
CH0 = counter
CH1 = -counter
```

因此实验现象与设计完全一致。

### 结论

`in_voltage0_raw` / `in_voltage1_raw` 可以通过标准 IIO sysfs ABI 正常触发驱动数据读取路径。

这说明以下链路已经验证：

```text
用户态 cat
   ↓
IIO sysfs ABI
   ↓
IIO core
   ↓
test_iio_read_raw()
   ↓
test_iio_hw_read_channel()
   ↓
返回测试数据
```

同时，正负通道关系也证明了：

- channel 0 与 channel 1 没有混淆；
- 两个通道均可独立访问；
- 驱动内部测试 counter 状态能够持续更新；
- `read_raw()` 的基本功能正确。

---

## 10. 当前实验阶段总览

本次实验已经验证：

```text
PC 端源码
    ↓
外部内核模块交叉编译
    ↓
test_iio.ko
    ↓
SD 卡复制到 rootfs
    ↓
/root/linux_demo/iio/test_iio.ko
    ↓
insmod
    ↓
probe()
    ↓
IIO device register
    ↓
iio:device1
    ↓
sysfs IIO attributes
    ↓
read_raw()
    ↓
测试数据 1,2,3... / -1,-5...
```

目前可以认为：

> **test_iio V1 direct-read 模型已经在真实 Zynq + PetaLinux 2018.3 板卡上验证成功。**

---

## 11. 当前阶段结论

### 已完成

- [x] 外部 ARM 内核模块交叉编译
- [x] `.ko` 架构检查
- [x] kernel vermagic 检查
- [x] SD 卡 rootfs 部署
- [x] `insmod test_iio.ko`
- [x] `probe()` 正常执行
- [x] 标准 IIO device 注册
- [x] `iio:device1` 创建
- [x] `name` 属性创建
- [x] `in_voltage0_raw` 创建并可读取
- [x] `in_voltage1_raw` 创建并可读取
- [x] `in_voltage_scale` 创建
- [x] `in_voltage_sampling_frequency` 创建
- [x] 测试 counter 数据变化符合预期

### 尚未验证

- [ ] `in_voltage_scale` 实际读取值
- [ ] `in_voltage_sampling_frequency` 实际读写
- [ ] 模块卸载 `rmmod`
- [ ] libiio `iio_info`
- [ ] libiio direct channel access
- [ ] IIO buffer
- [ ] `scan_elements`
- [ ] trigger
- [ ] `/dev/iio:deviceX` buffered read
- [ ] libiio buffer/refill
- [ ] DMA producer

---

## 12. 下一阶段建议

下一阶段继续保持“小步验证”的方式，不直接进入 DMA。

建议顺序：

```text
V1 direct read                ← 当前已经完成
    ↓
验证 scale / sampling_frequency
    ↓
验证 rmmod / 再次 insmod
    ↓
使用 iio_info 验证 libiio 枚举
    ↓
V2 IIO triggered buffer
    ↓
scan_elements
    ↓
/dev/iio:deviceX
    ↓
libiio buffer/refill
    ↓
真实 IRQ / trigger
    ↓
DMA
```

这样每一层出问题时，都可以明确知道问题属于哪一个接口层。

---

## 13. 本阶段最重要的认识

这次实验已经不仅证明“一个 `.ko` 能加载”，而是验证了 Linux IIO 的一个完整最小闭环：

```text
自定义驱动
   ↓
Linux IIO 子系统
   ↓
标准 sysfs ABI
   ↓
用户空间
```

这意味着后续真正接入 ADC / FPGA 时，可以尽量保持上层 IIO ABI 不变，只把当前虚拟的：

```text
test_iio_hw_read_channel()
```

逐步替换成真实硬件访问，例如：

```text
MMIO
SPI
IRQ
DMA
```

因此当前 `test_iio` 已经具备作为后续真实 IIO ADC 驱动模板的基础价值。

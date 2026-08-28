# test_iio 实验报告

## 1. 实验目的

本报告用于长期记录 `test_iio.ko` 在 Zynq + PetaLinux 2018.3 平台上的验证过程，方便后续在没有实验条件时回看代码、现象和结论。

当前已经验证：

1. 外部 ARM 内核模块交叉编译；
2. 模块加载与 `probe()`；
3. IIO device 注册；
4. sysfs direct-read；
5. libiio 0.18 local backend 枚举；
6. libiio channel 属性读取；
7. libiio 属性写入；
8. 非法参数错误传播；
9. `rmmod` / 再次 `insmod`；
10. 驱动私有状态重新初始化。

当前尚未进入 IIO buffer、trigger、DMA 和 libiio buffer/refill。

---

## 2. 实验环境

- 目标平台：Zynq / ARMv7
- PetaLinux：2018.3
- Kernel：`4.14.0-xilinx-v2018.3`
- libiio：`0.18`
- 模块：`test_iio.ko`
- 模块位置：`/root/linux_demo/iio/test_iio.ko`
- rootfs：SD 卡第二分区，ext4
- boot 分区：SD 卡第一分区，vfat

PC 端已经验证模块架构：

```text
ELF 32-bit LSB relocatable, ARM, EABI5
```

模块 `vermagic`：

```text
4.14.0-xilinx-v2018.3 SMP preempt mod_unload modversions ARMv7 p2v8
```

---

## 3. rootfs 与模块位置确认

执行：

```bash
df -h
```

关键输出：

```text
Filesystem                Size      Used Available Use% Mounted on
/dev/root                14.1G     55.8M     13.3G   0% /
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

查找模块：

```bash
find / -name test_iio.ko 2>/dev/null
```

输出：

```text
/root/linux_demo/iio/test_iio.ko
```

### 结论

SD 卡第二分区作为 Linux 根文件系统使用，因此驱动实验文件放在 `/root/linux_demo/` 下比放在容量较小的 boot 分区更合理。

---

## 4. 加载 test_iio 内核模块

执行：

```bash
cd /root/linux_demo/iio
insmod test_iio.ko
```

输出：

```text
test_iio: loading out-of-tree module taints kernel.
test_iio test_iio: probing generic test IIO device
test_iio test_iio: registered IIO device "test-iio-adc"
```

### 现象分析

`loading out-of-tree module taints kernel` 只表示该模块是在内核源码树之外构建并加载的，因此内核被标记为 tainted，并不是加载失败。

真正关键的是：

```text
probing generic test IIO device
registered IIO device "test-iio-adc"
```

说明：

```text
insmod
  ↓
module init
  ↓
platform driver/device 匹配
  ↓
probe()
  ↓
iio_device_register()
  ↓
IIO device 注册成功
```

### 结论

`test_iio.ko` 成功加载，`probe()` 正常执行，IIO 设备注册成功。

---

## 5. IIO 设备枚举

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

因此：

```text
iio:device0 -> Zynq 原有 XADC
iio:device1 -> 自定义 test_iio 驱动
```

### 结论

自定义驱动已经真正进入 Linux IIO 子系统，并获得标准 IIO 设备编号 `iio:device1`。

---

## 6. sysfs 属性检查

执行：

```bash
ls -la /sys/bus/iio/devices/iio:device1/
```

关键输出：

```text
in_voltage0_raw
in_voltage1_raw
in_voltage_sampling_frequency
in_voltage_scale
name
```

设备链接：

```bash
ls -l /sys/bus/iio/devices/iio:device1
```

输出：

```text
/sys/bus/iio/devices/iio:device1 -> ../../../devices/iio:device1
```

这也解释了此前：

```bash
find /sys/bus/iio/devices -maxdepth 2 -type f | sort
```

没有输出的原因：`iio:device1` 本身是符号链接，而 `find` 默认不会跟随它继续遍历。

### 结论

当前驱动已通过 IIO core 创建出预期的标准 sysfs ABI：

```text
name
in_voltage0_raw
in_voltage1_raw
in_voltage_scale
in_voltage_sampling_frequency
```

当前没有 `buffer/`、`scan_elements/`，符合 V1 尚未实现 buffered IIO 的设计。

---

## 7. sysfs direct-read 验证

读取：

```bash
cat /sys/bus/iio/devices/iio:device1/in_voltage0_raw
cat /sys/bus/iio/devices/iio:device1/in_voltage1_raw
```

实验过程中观察到：

```text
voltage0: 1, 2, 3, 4, 5, ...
voltage1: -1,             -5, ...
```

驱动测试模型为：

```text
CH0 = counter
CH1 = -counter
```

### 结论

实验结果与驱动设计一致，说明：

- channel 0 与 channel 1 映射正确；
- `read_raw()` 正常工作；
- 驱动内部 counter 状态持续更新；
- 用户态通过标准 IIO sysfs ABI 能进入自定义驱动读取路径。

读取路径：

```text
cat
 ↓
IIO sysfs ABI
 ↓
IIO core
 ↓
test_iio_read_raw()
 ↓
test_iio_hw_read_channel()
 ↓
测试数据
```

---

## 8. libiio 环境确认

执行：

```bash
which iio_info
```

输出：

```text
/usr/bin/iio_info
```

执行：

```bash
iio_info -u local:
```

开头输出：

```text
Library version: 0.18 (git tag: v0.18)
Compiled with backends: local xml ip usb
IIO context created with local backend.
Backend version: 0.18 (git tag: v0.18)
Backend description string: Linux LIULIU-ZYNQ1 4.14.0-xilinx-v2018.3 ... armv7l
IIO context has 2 devices:
```

板端工具：

```text
iio_adi_xflow_check
iio_attr
iio_genxml
iio_info
iio_readdev
iio_reg
iio_writedev
```

### 结论

libiio 0.18 已正确安装，local backend 可以正常创建 IIO context。

---

## 9. libiio 枚举自定义 IIO 设备

`iio_info -u local:` 中与 `test_iio` 相关的输出：

```text
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
```

此前 sysfs 已经读到：

```text
voltage0 = 5
voltage1 = -5
```

随后 `iio_info` 读到：

```text
voltage0 = 6
voltage1 = -6
```

### 现象分析

说明 libiio 并不是读取自己的缓存，而是通过 local backend 再次访问 Linux IIO ABI，再次进入驱动的 `read_raw()` 路径。

```text
iio_info
   ↓
libiio
   ↓
local backend
   ↓
Linux IIO ABI
   ↓
IIO core
   ↓
test_iio_read_raw()
   ↓
6 / -6
```

### 结论

libiio 不需要了解 `test_iio.c` 的内部实现，只要驱动遵守 Linux IIO ABI，就可以自动发现设备、channel 和属性。

---

## 10. libiio direct channel 属性读取

执行：

```bash
iio_attr -c test-iio-adc voltage0 raw
```

输出：

```text
dev 'test-iio-adc', channel 'voltage0' (input), attr 'raw', value '7'
```

执行：

```bash
iio_attr -c test-iio-adc voltage1 raw
```

输出：

```text
dev 'test-iio-adc', channel 'voltage1' (input), attr 'raw', value '-7'
```

### 结论

`iio_attr` 能够通过 libiio 正常访问自定义 channel 的 `raw` 属性，并实际触发驱动读取逻辑。

两种用户态入口最终汇聚到同一个驱动：

```text
cat sysfs ─────┐
               ├─> Linux IIO ABI -> IIO core -> test_iio
libiio/iio_attr┘
```

---

## 11. libiio 读取 scale

执行：

```bash
iio_attr -c test-iio-adc voltage0 scale
```

输出：

```text
dev 'test-iio-adc', channel 'voltage0' (input), attr 'scale', value '0.001000'
```

### 结论

驱动暴露的 `scale` 属性能够被 libiio 正确解析，数值为 `0.001000`。

---

## 12. libiio 修改 sampling_frequency

执行：

```bash
iio_attr -c test-iio-adc voltage0 sampling_frequency 2000
```

输出：

```text
dev 'test-iio-adc', channel 'voltage0' (input), attr 'sampling_frequency', value '1000'
wrote 5 bytes to sampling_frequency
dev 'test-iio-adc', channel 'voltage0' (input), attr 'sampling_frequency', value '2000'
```

随后执行：

```bash
iio_attr -c test-iio-adc voltage0 sampling_frequency
iio_attr -c test-iio-adc voltage1 sampling_frequency
```

输出均为：

```text
2000
```

再通过 sysfs 交叉验证：

```bash
cat /sys/bus/iio/devices/iio:device1/in_voltage_sampling_frequency
```

输出：

```text
2000
```

### 结论

- libiio 能够通过标准 IIO ABI 写入可写属性；
- `sampling_frequency` 是 shared-by-type 属性；
- voltage0 修改后 voltage1 看到同一个值；
- sysfs 与 libiio 读回一致。

写入路径：

```text
iio_attr
   ↓
libiio
   ↓
local backend
   ↓
Linux IIO ABI
   ↓
IIO core
   ↓
test_iio_write_raw()
   ↓
test_iio_hw_set_sampling_frequency()
   ↓
驱动内部状态 = 2000
```

---

## 13. 非法 sampling_frequency 参数验证

执行：

```bash
iio_attr -c test-iio-adc voltage0 sampling_frequency 0
```

输出：

```text
dev 'test-iio-adc', channel 'voltage0' (input), attr 'sampling_frequency', value '2000'
error Invalid argument (-22) while writing 'sampling_frequency' with '0'
dev 'test-iio-adc', channel 'voltage0' (input), attr 'sampling_frequency', value '2000'
```

其中：

```text
-22 = EINVAL = Invalid argument
```

### 结论

驱动正确拒绝非法采样率 `0`，并且失败后原状态保持为 `2000`。

错误传播链路已经验证：

```text
非法用户输入 0
   ↓
libiio
   ↓
IIO core
   ↓
test_iio_write_raw()
   ↓
参数检查失败
   ↓
-EINVAL
   ↓
libiio 显示 Invalid argument (-22)
```

---

## 14. 模块卸载验证

在上一阶段结束时：

```text
sampling_frequency = 2000
counter ≈ 7
```

执行：

```bash
rmmod test_iio
```

输出：

```text
test_iio test_iio: removing test IIO device
```

查看设备：

```bash
ls /sys/bus/iio/devices/
```

输出：

```text
iio:device0
```

读取名称：

```bash
cat /sys/bus/iio/devices/iio:device*/name
```

输出：

```text
xadc
```

### 结论

`test-iio-adc` 对应的 `iio:device1` 已经消失，说明 remove 路径和 IIO device 注销正常。

```text
rmmod test_iio
      ↓
remove()
      ↓
IIO device unregister
      ↓
iio:device1 消失
```

---

## 15. 再次加载与状态重新初始化

再次执行：

```bash
insmod /root/linux_demo/iio/test_iio.ko
```

输出：

```text
test_iio test_iio: probing generic test IIO device
test_iio test_iio: registered IIO device "test-iio-adc"
```

设备重新出现：

```text
iio:device0  iio:device1
```

名称重新恢复：

```text
xadc
test-iio-adc
```

### 15.1 sampling_frequency 复位

执行：

```bash
iio_attr -c test-iio-adc voltage0 sampling_frequency
```

输出：

```text
dev 'test-iio-adc', channel 'voltage0' (input), attr 'sampling_frequency', value '1000'
```

即：

```text
卸载前：2000
重新加载后：1000
```

### 15.2 counter 复位

执行：

```bash
iio_attr -c test-iio-adc voltage0 raw
iio_attr -c test-iio-adc voltage1 raw
```

输出：

```text
dev 'test-iio-adc', channel 'voltage0' (input), attr 'raw', value '1'
dev 'test-iio-adc', channel 'voltage1' (input), attr 'raw', value '-1'
```

即：

```text
上一轮：7 / -7
重新加载后：1 / -1
```

### 结论

重新 `insmod` 后：

- 新的 `probe()` 正常执行；
- IIO device 重新注册；
- `sampling_frequency` 恢复默认值 `1000`；
- counter 从 `1/-1` 重新开始；
- 没有观察到上一轮驱动实例状态残留。

完整生命周期：

```text
insmod
  ↓
probe()
  ↓
IIO device 注册
  ↓
运行并修改状态
  │ sampling_frequency = 2000
  │ counter = 7
  ↓
rmmod
  ↓
remove()
  ↓
IIO device 注销
  ↓
iio:device1 消失
  ↓
再次 insmod
  ↓
新的 probe()
  ↓
状态重新初始化
  │ sampling_frequency = 1000
  │ counter = 1 / -1
```

---

## 16. 当前阶段完整数据路径

### 读取方向

```text
用户程序 / iio_attr / iio_info
            ↓
          libiio
            ↓
        local backend
            ↓
       Linux IIO ABI
            ↓
          IIO core
            ↓
   test_iio_read_raw()
            ↓
 test_iio_hw_read_channel()
            ↓
        测试数据
```

### 写入方向

```text
iio_attr sampling_frequency=2000
            ↓
          libiio
            ↓
        local backend
            ↓
       Linux IIO ABI
            ↓
          IIO core
            ↓
   test_iio_write_raw()
            ↓
 test_iio_hw_set_sampling_frequency()
            ↓
   驱动内部状态更新
```

### 生命周期方向

```text
insmod
  ↓
probe
  ↓
register
  ↓
rmmod
  ↓
remove
  ↓
unregister
  ↓
再次 insmod
  ↓
重新初始化
```

---

## 17. V1 / V1.1 阶段完成项

- [x] 外部 ARM 内核模块交叉编译
- [x] `.ko` 架构检查
- [x] kernel vermagic 检查
- [x] SD 卡 rootfs 部署
- [x] `insmod test_iio.ko`
- [x] `probe()` 正常执行
- [x] 标准 IIO device 注册
- [x] `iio:device1` 创建
- [x] sysfs direct-read
- [x] `in_voltage_scale` 读取
- [x] `in_voltage_sampling_frequency` 读取
- [x] libiio 0.18 local context 创建
- [x] `iio_info` 枚举自定义设备
- [x] libiio 枚举两个 voltage channel
- [x] `iio_attr` 读取 raw
- [x] `iio_attr` 读取 scale
- [x] `iio_attr` 写 sampling_frequency：`1000 -> 2000`
- [x] shared-by-type 属性验证
- [x] sysfs / libiio 读回一致
- [x] 非法 sampling_frequency=0 被拒绝
- [x] `-EINVAL` 正确传播到 libiio
- [x] `rmmod test_iio`
- [x] remove 路径执行
- [x] `iio:device1` 卸载后消失
- [x] 再次 `insmod`
- [x] `sampling_frequency` 恢复 1000
- [x] counter 恢复 1/-1
- [x] 未观察到跨模块生命周期的状态残留

---

## 18. 阶段性里程碑

### V1：IIO direct-read 验证成功

```text
test_iio.ko
  ↓
IIO core
  ↓
sysfs ABI
  ↓
cat raw
```

### V1.1：libiio direct attribute 读写验证成功

```text
test_iio.ko
  ↓
Linux IIO core
  ↓
标准 IIO ABI
  ↓
libiio local backend
  ↓
iio_info / iio_attr
```

### V1.2：模块生命周期验证成功

```text
load
 ↓
register
 ↓
run
 ↓
unload
 ↓
unregister
 ↓
reload
 ↓
state re-init
```

当前可以明确认为：

> **test_iio V1/V1.1/V1.2 已经在真实 Zynq + PetaLinux 2018.3 板卡上完成验证。**

---

## 19. 尚未验证

- [ ] IIO buffer
- [ ] `scan_elements`
- [ ] trigger
- [ ] `/dev/iio:deviceX` buffered read
- [ ] `iio_readdev`
- [ ] libiio buffer/refill
- [ ] DMA producer

---

## 20. 下一阶段

下一阶段进入：

```text
V2: IIO buffered capture
```

推荐继续保持小步验证：

```text
scan_elements
   ↓
buffer
   ↓
/dev/iio:deviceX
   ↓
iio_readdev
   ↓
libiio buffer/refill
   ↓
trigger / IRQ
   ↓
DMA
```

---

## 21. 本阶段最重要的认识

当前已经验证了 Linux IIO 与 libiio 的职责边界：

```text
硬件/测试数据源
      ↓
test_iio kernel driver
      ↓
Linux IIO subsystem
      ↓
标准 IIO ABI
      ↓
libiio
      ↓
用户应用
```

libiio 并不认识 `test_iio.c`，也不会直接调用驱动私有函数。它只依赖 Linux IIO 暴露的标准 ABI。

因此以后把当前虚拟测试数据源替换成真实 ADC / FPGA 时，可以尽量保持上层接口不变，只逐步替换驱动内部硬件访问层：

```text
test_iio_hw_read_channel()
        ↓
MMIO / SPI / IRQ / DMA
```

当前 `test_iio` 已经具备作为后续真实 IIO ADC 驱动模板继续演进的基础价值。

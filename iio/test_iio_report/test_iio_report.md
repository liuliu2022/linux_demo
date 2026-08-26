# test_iio 实验报告

## 1. 实验目的

本实验用于验证自定义 `test_iio.ko` 外部内核模块在 Zynq + PetaLinux 2018.3 平台上的最小 IIO 闭环，并进一步验证 libiio 是否能够通过标准 Linux IIO ABI 正确发现、读取和写入该驱动暴露的属性。

当前阶段覆盖：

1. 外部内核模块加载；
2. `probe()` 与 IIO device 注册；
3. sysfs direct-read；
4. libiio local backend 枚举；
5. libiio channel 属性读取；
6. libiio 属性写入；
7. 非法参数拒绝。

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

PC 端已经验证模块为 ARM 架构：

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

```text
test_iio: loading out-of-tree module taints kernel.
```

只表示当前模块属于 out-of-tree module，内核被标记为 `tainted`，不是加载错误。

下面两行更关键：

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

这也解释了之前执行：

```bash
find /sys/bus/iio/devices -maxdepth 2 -type f | sort
```

没有输出的原因：`iio:device1` 是符号链接，而 `find` 默认不会跟随它继续遍历。

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

读取设备名称：

```bash
cat /sys/bus/iio/devices/iio:device1/name
```

输出：

```text
test-iio-adc
```

连续读取原始数据：

```bash
cat /sys/bus/iio/devices/iio:device1/in_voltage0_raw
cat /sys/bus/iio/devices/iio:device1/in_voltage1_raw
```

实验过程中得到：

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
- 用户态通过标准 IIO sysfs ABI 能够进入自定义驱动的数据读取路径。

读取路径已经验证为：

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
返回测试数据
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

板端安装的 libiio 工具：

```bash
ls /usr/bin/iio*
```

输出：

```text
/usr/bin/iio_adi_xflow_check
/usr/bin/iio_attr
/usr/bin/iio_genxml
/usr/bin/iio_info
/usr/bin/iio_readdev
/usr/bin/iio_reg
/usr/bin/iio_writedev
```

### 结论

libiio 0.18 已经正确安装，并且 local backend 可以正常创建 IIO context。

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

值得注意的是，在此之前通过 sysfs 已经读到：

```text
voltage0 = 5
voltage1 = -5
```

而执行 `iio_info` 后观察到：

```text
voltage0 = 6
voltage1 = -6
```

### 现象分析

这说明 libiio 并不是读取自己缓存的数据，而是通过 local backend 再次访问 Linux IIO ABI，从而再次进入驱动的 `read_raw()` 路径。

数据路径可以理解为：

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

libiio 不需要了解 `test_iio.c` 的内部实现，只要驱动遵守 Linux IIO ABI，就可以自动发现：

- `test-iio-adc`
- `voltage0`
- `voltage1`
- `raw`
- `scale`
- `sampling_frequency`

这验证了 Linux IIO 驱动与 libiio 之间的标准接口边界。

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

再次看到数据从 `6/-6` 继续变为 `7/-7`。

### 结论

`iio_attr` 能够通过 libiio 正常访问自定义 channel 的 `raw` 属性，并实际触发驱动读取逻辑。

当前已经验证两种用户态入口最终汇聚到同一个驱动：

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

驱动暴露的 `scale` 属性能够被 libiio 正确解析，数值为：

```text
0.001000
```

---

## 12. libiio 修改 sampling_frequency

### 12.1 正常写入

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

这个输出非常明确地表现出：

```text
旧值 1000
   ↓
libiio 写入 2000
   ↓
重新读回 2000
```

随后执行：

```bash
iio_attr -c test-iio-adc voltage0 sampling_frequency
```

输出：

```text
dev 'test-iio-adc', channel 'voltage0' (input), attr 'sampling_frequency', value '2000'
```

读取另一个 channel：

```bash
iio_attr -c test-iio-adc voltage1 sampling_frequency
```

输出：

```text
dev 'test-iio-adc', channel 'voltage1' (input), attr 'sampling_frequency', value '2000'
```

再通过 sysfs 交叉验证：

```bash
cat /sys/bus/iio/devices/iio:device1/in_voltage_sampling_frequency
```

输出：

```text
2000
```

### 现象分析

`voltage0` 写入后，`voltage1` 同样看到 `2000`，而 sysfs 中只有一个共享节点：

```text
in_voltage_sampling_frequency
```

这与驱动将 `sampling_frequency` 设计为 shared-by-type 属性的行为一致。

写入路径已经验证为：

```text
iio_attr
   ↓
libiio
   ↓
local backend
   ↓
IIO sysfs ABI
   ↓
IIO core
   ↓
test_iio_write_raw()
   ↓
test_iio_hw_set_sampling_frequency()
   ↓
驱动内部状态 = 2000
```

### 结论

libiio 不仅能够读取自定义驱动属性，还能够通过标准 IIO ABI 成功修改可写属性。

这证明用户态到驱动的反向控制路径已经走通。

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

可以看到：

```text
写入前：2000
   ↓
尝试写 0
   ↓
驱动拒绝，返回 -EINVAL
   ↓
写入后仍然为 2000
```

### 结论

驱动不仅支持正常参数写入，也正确拒绝非法采样率 `0`，而且失败后原有状态保持不变。

这验证了参数检查和错误码向用户态传播的完整链路：

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

## 14. 当前阶段完整数据路径

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

因此当前已经形成一个真正的用户态/内核态双向闭环。

---

## 15. 当前阶段结论

### 已完成

- [x] 外部 ARM 内核模块交叉编译
- [x] `.ko` 架构检查
- [x] kernel vermagic 检查
- [x] SD 卡 rootfs 部署
- [x] `insmod test_iio.ko`
- [x] `probe()` 正常执行
- [x] 标准 IIO device 注册
- [x] `iio:device1` 创建
- [x] `name` 属性创建与读取
- [x] `in_voltage0_raw` / `in_voltage1_raw` sysfs direct-read
- [x] `in_voltage_scale` 创建并通过 libiio 读取
- [x] `in_voltage_sampling_frequency` 创建
- [x] libiio 0.18 local context 创建
- [x] `iio_info` 枚举自定义设备
- [x] libiio 枚举两个 voltage channel
- [x] `iio_attr` 读取 raw
- [x] `iio_attr` 读取 scale
- [x] `iio_attr` 读取 sampling_frequency
- [x] `iio_attr` 写 sampling_frequency：`1000 -> 2000`
- [x] shared-by-type 属性在两个 channel 上保持一致
- [x] sysfs 与 libiio 读回值一致
- [x] 非法 sampling_frequency=0 被拒绝
- [x] 驱动返回 `-EINVAL` 并传播到 libiio
- [x] 非法写入失败后原状态保持为 2000

### 尚未验证

- [ ] 模块卸载 `rmmod`
- [ ] 再次 `insmod` 后状态初始化
- [ ] IIO buffer
- [ ] `scan_elements`
- [ ] trigger
- [ ] `/dev/iio:deviceX` buffered read
- [ ] `iio_readdev`
- [ ] libiio buffer/refill
- [ ] DMA producer

---

## 16. 阶段性里程碑

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

当前已经可以明确认为：

> **test_iio V1/V1.1 已经在真实 Zynq + PetaLinux 2018.3 板卡上完成验证。驱动不仅能够通过 sysfs 工作，而且已经能够被 libiio 0.18 正确枚举、读取、写入和错误处理。**

---

## 17. 下一阶段建议

下一阶段仍保持小步验证，不直接跳到 DMA：

```text
V1 / V1.1 direct mode          <- 已完成
       ↓
rmmod / 再次 insmod
       ↓
V2 IIO buffer
       ↓
scan_elements
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

这样每一层都能单独留下现象和结论，便于后续复盘与定位问题。

---

## 18. 本阶段最重要的认识

本阶段已经验证了 Linux IIO 与 libiio 的职责边界：

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

这使当前 `test_iio` 不只是一次性测试代码，而是具备继续演进为真实 IIO ADC 驱动模板的基础价值。

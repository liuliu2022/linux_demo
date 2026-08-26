# test_iio 卸载与重新加载实验报告

## 1. 实验目的

在已经完成 `test_iio` direct-read 与 libiio direct attribute 读写验证之后，本实验进一步确认：

1. `rmmod test_iio` 是否能够正确执行驱动移除流程；
2. 自定义 IIO 设备是否会从 Linux IIO 子系统中消失；
3. 再次 `insmod test_iio.ko` 后设备是否能够重新注册；
4. 驱动私有状态是否重新初始化，而不是保留上一次运行状态。

本实验开始前，上一阶段已经将：

```text
sampling_frequency = 2000
counter ≈ 7
```

因此非常适合用来判断重新加载后状态是否复位。

---

## 2. 卸载模块

执行：

```bash
rmmod test_iio
```

输出：

```text
test_iio test_iio: removing test IIO device
```

随后查看内核日志：

```bash
dmesg | tail -10
```

关键输出：

```text
test_iio test_iio: probing generic test IIO device
test_iio test_iio: registered IIO device "test-iio-adc"
random: crng init done
test_iio test_iio: removing test IIO device
```

### 结论

`rmmod` 成功进入驱动移除路径，并执行了 `test_iio` 的 remove 流程。

---

## 3. 检查 IIO 设备是否消失

卸载后执行：

```bash
ls /sys/bus/iio/devices/
```

输出：

```text
iio:device0
```

读取当前 IIO 设备名称：

```bash
cat /sys/bus/iio/devices/iio:device*/name
```

输出：

```text
xadc
```

卸载前系统中为：

```text
iio:device0 -> xadc
iio:device1 -> test-iio-adc
```

卸载后变为：

```text
iio:device0 -> xadc
```

### 结论

`test-iio-adc` 对应的 `iio:device1` 已经从 IIO 子系统中消失。

这说明模块卸载不仅执行了驱动 remove 回调，同时 IIO device 以及相应的 sysfs ABI 也被正确注销。

当前卸载链路可以表示为：

```text
rmmod test_iio
      ↓
module exit
      ↓
platform device / driver 解除
      ↓
remove()
      ↓
IIO device unregister
      ↓
iio:device1 消失
```

---

## 4. 重新加载模块

再次执行：

```bash
cd /root/linux_demo/iio
insmod test_iio.ko
```

输出：

```text
test_iio test_iio: probing generic test IIO device
test_iio test_iio: registered IIO device "test-iio-adc"
```

查看日志：

```bash
dmesg | tail -10
```

关键输出：

```text
test_iio test_iio: removing test IIO device
test_iio test_iio: probing generic test IIO device
test_iio test_iio: registered IIO device "test-iio-adc"
```

### 结论

重新 `insmod` 后，驱动再次进入 `probe()`，并成功重新注册 IIO device。

---

## 5. 检查 IIO 设备是否重新出现

执行：

```bash
ls /sys/bus/iio/devices/
```

输出：

```text
iio:device0  iio:device1
```

读取名称：

```bash
cat /sys/bus/iio/devices/iio:device*/name
```

输出：

```text
xadc
test-iio-adc
```

### 结论

重新加载后，`test-iio-adc` 再次作为 `iio:device1` 出现在 Linux IIO 子系统中。

设备的完整生命周期已经得到验证：

```text
注册
 ↓
iio:device1 出现
 ↓
rmmod
 ↓
iio:device1 消失
 ↓
insmod
 ↓
iio:device1 再次出现
```

---

## 6. sampling_frequency 状态复位验证

在模块卸载之前，上一阶段已经通过 libiio 将采样频率从默认值：

```text
1000
```

修改为：

```text
2000
```

重新加载后执行：

```bash
iio_attr -c test-iio-adc voltage0 sampling_frequency
```

输出：

```text
dev 'test-iio-adc', channel 'voltage0' (input), attr 'sampling_frequency', value '1000'
```

### 现象分析

状态发生：

```text
卸载前：2000
   ↓
rmmod
   ↓
原驱动实例销毁
   ↓
insmod
   ↓
新的驱动实例初始化
   ↓
重新变为默认值 1000
```

### 结论

`sampling_frequency` 没有跨模块生命周期残留，重新 `probe()` 后正确恢复为驱动默认值 `1000`。

这证明驱动私有状态在新实例创建时被重新初始化。

---

## 7. counter 状态复位验证

重新加载前，上一轮 direct-read / libiio 实验中的 counter 已经运行到：

```text
7 / -7
```

重新加载后执行：

```bash
iio_attr -c test-iio-adc voltage0 raw
```

输出：

```text
dev 'test-iio-adc', channel 'voltage0' (input), attr 'raw', value '1'
```

再执行：

```bash
iio_attr -c test-iio-adc voltage1 raw
```

输出：

```text
dev 'test-iio-adc', channel 'voltage1' (input), attr 'raw', value '-1'
```

### 现象分析

数据变化为：

```text
卸载前：7 / -7
   ↓
rmmod
   ↓
驱动实例释放
   ↓
insmod
   ↓
counter 重新初始化
   ↓
1 / -1
```

### 结论

测试 counter 同样没有残留上一次模块运行状态。

重新加载模块后，数据生成器从初始状态重新开始运行。

---

## 8. 本实验最终结论

本实验已经完整验证 `test_iio` 的模块生命周期：

```text
insmod
  ↓
probe()
  ↓
IIO device 注册
  ↓
运行过程中修改状态
  │   sampling_frequency = 2000
  │   counter = 7
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
IIO device 重新注册
  ↓
状态重新初始化
  │   sampling_frequency = 1000
  │   counter = 1 / -1
```

因此可以确认：

- [x] `rmmod test_iio` 正常工作；
- [x] remove 路径正常执行；
- [x] IIO device 能够正确注销；
- [x] `iio:device1` 在卸载后消失；
- [x] 再次 `insmod` 能重新注册设备；
- [x] `sampling_frequency` 从运行时值 `2000` 恢复默认值 `1000`；
- [x] counter 从上一轮 `7/-7` 恢复到 `1/-1`；
- [x] 驱动实例之间没有观察到状态残留。

---

## 9. V1 阶段收口

经过目前全部实验，`test_iio` V1 已经验证：

```text
外部模块交叉编译
       ↓
insmod / probe
       ↓
Linux IIO device 注册
       ↓
sysfs direct-read
       ↓
libiio 枚举
       ↓
libiio 属性读取
       ↓
libiio 属性写入
       ↓
非法参数错误传播
       ↓
rmmod / remove
       ↓
重新加载与状态初始化
```

至此可以将：

> **V1 / V1.1：IIO direct mode + libiio direct attribute + 模块生命周期**

视为完成验证。

下一阶段可以正式进入：

```text
V2: IIO buffered capture
```

重点将从单次 `read_raw()` 属性访问转向：

```text
scan_elements
buffer
/dev/iio:deviceX
triggered buffer
libiio buffer/refill
```

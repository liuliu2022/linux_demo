# test_iio V2 - software buffered capture

Status: **code written, not yet validated on the Zynq board**.

This version keeps V1 direct-mode support and adds the first buffered IIO path without IRQ or DMA.

Data path:

```text
hrtimer
  -> workqueue
  -> build one scan [CH0, CH1]
  -> iio_push_to_buffers()
  -> Linux IIO kfifo buffer
  -> /dev/iio:deviceX
  -> read / libiio
```

Files:

- `test_iio_core.c`: device registration, channels, direct attributes, virtual ADC state.
- `test_iio_buffer.c`: software producer, kfifo attachment, buffer enable/disable lifecycle.
- `test_iio.h`: shared state and declarations.
- `Makefile`: builds the multi-object `test_iio.ko` module.

V2 intentionally permits only one buffered scan mask: both voltage channels enabled together. This keeps the first buffered experiment deterministic:

```text
scan N = [ CH0=N ][ CH1=-N ]
```

Build after loading the PetaLinux environment:

```bash
cd ~/linux_demo
source kernel_env.sh
cd iio/v2_buffer
make check
```

Do not treat V2 as experimentally verified until it has been compiled against the PetaLinux 2018.3 kernel and tested on the board. A V2 report should be created only after those experiments are completed.

# Simple Operating System Simulator

Mo phong he dieu hanh cho bai tap lon CO2018, gom:

- MLQ scheduler co weighted slot va FIFO trong tung hang doi.
- Unified system-call dispatcher; kernel lookup PCB bang PID.
- User/kernel memory protection.
- Paging 64-bit nam cap cap dong voi low/high canonical-address validation.
- Paging statistics va 22 testcase co semantic assertions.

## Quick start

```bash
make clean
make all
make test
```

Xem [SUBMISSION_GUIDE.md](SUBMISSION_GUIDE.md) de biet chi tiet test, output
mong doi va cach tao archive nop bai.

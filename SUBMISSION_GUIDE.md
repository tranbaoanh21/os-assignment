# OS Simulator - Huong dan kiem tra va nop bai

## Trang thai

| Thanh phan | Trang thai | Bang chung |
|---|---|---|
| Build | PASS | `make clean && make all` |
| MLQ scheduler | PASS | FIFO trong tung muc uu tien, weighted slot, queue lock |
| PID/system-call boundary | PASS | Memory wrappers chi truyen PID va scalar arguments vao syscall 17 |
| User/kernel protection | PASS | `os_mem_protection` tu choi raw physical read/write |
| Cross-PID protection | PASS | PID 2 bi tu choi khi truy cap frame dang thuoc PID 1 |
| Data-region reuse | PASS | `os_mem_reuse` cap lai vung vua free |
| 64-bit multi-level paging | PASS | Sparse table cap dong; low/high canonical; tu choi non-canonical |
| Paging statistics | PASS | Output co `walks`, `accesses`, `bytes` |
| User/kernel copy | PASS | `os_mem_roundtrip` doc lai dung byte 65 |
| Semantic assertions | PASS | `tests/assert_outputs.sh` kiem tra cac bat bien quan trong |
| Synchronization | PASS | Scheduler queue va physical-frame free list duoc bao ve boi mutex |

`PASS` o day co nghia la da build va vuot qua bo test trong repository. Ket qua
khong thay the viec demo va giai thich thiet ke voi giang vien.

## Build va chay test

```bash
make clean
make all
make test
```

`make test` chay 22 cau hinh trong `run.sh`, ghi output vao `output/`, sau do
chay semantic assertions. Lenh se tra ve loi neu mot bat bien quan trong khong dung.

Ba test quan trong:

```bash
./os os_mm64_canonical
./os os_mem_protection
./os os_mem_roundtrip
```

Ket qua mong doi:

```text
MM64 rejected non-canonical address 0x100000000000000
MM64 pid=1 va=0x200000 indexes=[0,0,0,1,0]
MM64 pid=1 va=0xff00000000000000 indexes=[256,0,0,0,0]
Kernel denied PID 1 physical read at 0x0
Kernel denied PID 1 physical write at 0x0
MEM read PID 1 region 3 offset 0 value 65
```

Thong ke paging co dang:

```text
MM64 pid=1 va=0x0 indexes=[0,0,0,0,0] walks=7 accesses=35 bytes=20480
```

Moi page-table walk hop le truy cap nam cap, vi vay `accesses = 5 * walks`.
Nhanh dau tien dung 20480 bytes; mapping branch moi lam `bytes` tang theo tung
bang 4096 bytes.

## Kiem tra truoc khi nop

- [ ] Dien thong tin lop, giang vien va day du thanh vien trong bao cao.
- [ ] `make clean && make all` thanh cong.
- [ ] `make test` thanh cong.
- [ ] Kiem tra output cua `os_mm64_canonical`, `os_mem_protection` va `os_mem_roundtrip`.
- [ ] Kiem tra dong `All semantic output assertions passed.`.
- [ ] Doc va cap nhat `assignment_os_report.tex` theo ket qua demo cuoi.
- [ ] Khong nop `.git/`, `obj/`, binary `os`, `sched`, `mem`.

## Tao archive

De bai yeu cau ten file `assignment_STUDENTID.zip`:

```bash
make clean
cd ..
zip -r assignment_2410155.zip ossim_caitoa \
  -x "ossim_caitoa/.git/*" \
     "ossim_caitoa/obj/*" \
     "ossim_caitoa/os" \
     "ossim_caitoa/sched" \
     "ossim_caitoa/mem"
```

## Ghi chu thiet ke

- PCB khong duoc truyen qua syscall. Kernel dung `find_proc_by_pid()` de tim
  process trong cac scheduler list duoi `queue_lock`.
- Moi process so huu `proc->mm`; kernel khong thay doi mot con tro `krnl->mm`
  dung chung khi context switch.
- Raw physical I/O chi duoc phep neu frame dang duoc map trong page table cua
  PID goi syscall.
- Dia chi 57-bit phai canonical truoc khi duoc tach thanh chi so nam cap.
- Cac bang con MM64 duoc cap dong va giai phong de quy khi process ket thuc.
- Swap-in va swap-out dung operation rieng; page replacement end-to-end van can demo
  them neu giang vien hoi phan optional.

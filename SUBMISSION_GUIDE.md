# OS Simulator - Huong dan kiem tra va nop bai

## Trang thai

| Thanh phan | Trang thai | Bang chung |
|---|---|---|
| Build | PASS | `make clean && make all` |
| MLQ scheduler | PASS | FIFO trong tung muc uu tien, weighted slot, queue lock |
| PID/system-call boundary | PASS | Memory wrappers chi truyen PID va scalar arguments vao syscall 17 |
| User/kernel protection | PASS | `os_mem_protection` tu choi raw physical read/write |
| 64-bit multi-level paging | PASS | Walk `PGD -> P4D -> PUD -> PMD -> PT`, co canonical check |
| Paging statistics | PASS | Output co `walks`, `accesses`, `bytes` |
| Synchronization | PASS | Scheduler queue va physical-frame free list duoc bao ve boi mutex |

`PASS` o day co nghia la da build va vuot qua bo test trong repository. Ket qua
khong thay the viec demo va giai thich thiet ke voi giang vien.

## Build va chay test

```bash
make clean
make all
make test
```

`make test` chay tat ca cau hinh trong `run.sh` va ghi output vao `output/`.

Hai test bao ve quan trong:

```bash
./os os_mm64_canonical
./os os_mem_protection
```

Ket qua mong doi:

```text
MM64 rejected non-canonical address 0x100000000000000
Kernel denied PID 1 physical read at 0x0
Kernel denied PID 1 physical write at 0x0
```

Thong ke paging co dang:

```text
MM64 pid=1 va=0x0 indexes=[0,0,0,0,0] walks=7 accesses=35 bytes=20480
```

Moi page-table walk hop le truy cap nam cap, vi vay
`accesses = 5 * walks`. Kich thuoc nam bang la
`5 * 512 * sizeof(uint64_t) = 20480` bytes cho moi process.

## Kiem tra truoc khi nop

- [ ] Dien thong tin lop, giang vien va day du thanh vien trong bao cao.
- [ ] `make clean && make all` thanh cong.
- [ ] `make test` thanh cong.
- [ ] Kiem tra output cua `os_mm64_canonical` va `os_mem_protection`.
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

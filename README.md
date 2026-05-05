# Hướng dẫn chi tiết từng phần trong đặc tả
## Tổng quan yêu cầu bài tập lớn
Bài tập này yêu cầu bạn mô phỏng lại 3 thành phần cốt lõi của một hệ điều hành (OS) đơn giản bằng cách lập trình, bao gồm:

- **Scheduler (Bộ lập lịch)**: Quản lý và điều phối các tiến trình (process) hoặc luồng (thread) được thực thi trên CPU.

- **Synchronization (Đồng bộ hóa)**: Giải quyết các vấn đề tranh chấp tài nguyên (race condition) khi nhiều tiến trình/luồng truy cập vào vùng dữ liệu dùng chung.

- **Memory Allocation (Cấp phát bộ nhớ)**: Mô phỏng cơ chế ánh xạ và chuyển đổi từ Bộ nhớ ảo (Virtual Memory) sang Bộ nhớ vật lý (Physical Memory).

Ngoài ra, bạn sẽ thiết kế và triển khai các **System Call (Lời gọi hệ thống)** để các chương trình người dùng có thể tương tác với các thành phần của OS này.

## Các khái niệm lý thuyết cốt lõi cần nắm
- **Process / Thread Scheduling**: Lập lịch CPU là quá trình quyết định tiến trình nào sẽ được đưa vào CPU xử lý tại một thời điểm để tối ưu hóa hiệu suất (ví dụ: giảm thời gian chờ, tăng thông lượng).

- **Synchronization Primitives**: Các cơ chế như Mutex, Semaphore hoặc Lock dùng để đảm bảo Mutual Exclusion (Loại trừ tương hỗ), tránh tình trạng dữ liệu bị lỗi khi nhiều tiến trình cùng ghi/đọc.

- **Virtual to Physical Address Translation**: Cơ chế ánh xạ các địa chỉ bộ nhớ ảo mà tiến trình nhìn thấy thành địa chỉ vật lý thực tế trên RAM thông qua các bảng trang (Page Table).

- **System Call**: Giao diện lập trình (API) cho phép ứng dụng ở chế độ người dùng (User Mode) yêu cầu các dịch vụ đặc quyền từ kernel (Kernel Mode).

# 1 Introduction
## 1.1 An overview
### 1. Tổng quan kiến trúc hệ điều hành (OS)
Đề bài giới thiệu kiến trúc tổng thể của hệ điều hành mà bạn cần mô phỏng, bao gồm 2 thành phần chính để quản lý tài nguyên:

- **Scheduler & Dispatcher (Bộ lập lịch và điều phối)**: Quyết định tiến trình nào được cấp phát CPU để chạy.

- **Virtual Memory (Bộ nhớ ảo)**: Cách ly không gian bộ nhớ của từng tiến trình. Nó ánh xạ địa chỉ ảo của tiến trình sang địa chỉ vật lý của RAM, giúp các tiến trình chạy độc lập mà không biết đến sự tồn tại của nhau.

### 2. Lý thuyết cốt lõi cần nắm
Dual-mode operation (Hoạt động đa chế độ):

- **User Mode (Chế độ người dùng)**: Chế độ dành cho các ứng dụng thông thường. Các ứng dụng ở chế độ này không được phép truy cập trực tiếp vào phần cứng hoặc bộ nhớ của OS.

- **Kernel Mode (Chế độ nhân)**: Chế độ đặc quyền dành cho hệ điều hành. Các thao tác quản lý phần cứng, bộ nhớ hoặc lập lịch đều diễn ra ở đây.

- **Mode Bit**: Cờ phần cứng (bit trạng thái) để phân biệt hệ thống đang chạy ở User Mode hay Kernel Mode.

### 3. Những việc cần làm
Dựa trên yêu cầu của phần này, bạn cần:

- Hiểu rõ vai trò của 2 mô-đun trọng tâm: Scheduler/Dispatcher và Virtual Memory Engine.

- Nắm vững cơ chế chuyển đổi giữa User Mode và Kernel Mode để đảm bảo tính an toàn cho hệ thống.

## 1.2 Source code
### A. Các tệp tiêu đề (.h)
#### Hệ thống cốt lõi:

- `timer.h`: Định nghĩa bộ định thời (timer) cho hệ thống.

- `cpu.h`: Định nghĩa các hàm cho CPU ảo.

- `common.h`: Chứa các cấu trúc (struct) và hàm dùng chung.

- `syscall.h`: Định nghĩa các lời gọi hệ thống (system call).

#### Quản lý tiến trình & Lập lịch:

- `queue.h`: Cấu trúc hàng đợi (queue) để lưu trữ khối điều khiển tiến trình (PCB - Process Control Block).

- `sched.h`: Các hàm phục vụ cho bộ lập lịch (scheduler).

#### Quản lý bộ nhớ:

- `os-mm.h`, `mm.h`, `mm64.h:` Định nghĩa cấu trúc và dữ liệu cho quản lý bộ nhớ sử dụng phân trang (Paging-based Memory Management).

### B. Các tệp mã nguồn (.c)
 #### Khởi chạy & Điều khiển:

- `os.c`: Chứa hàm main để khởi động toàn bộ hệ điều hành.

- `timer.c`, `cpu.c`: Cài đặt bộ định thời và CPU ảo.

#### Tiến trình & Lập lịch:

- `queue.c`: Cài đặt các thao tác trên hàng đợi.

- `sched.c`: Cài đặt bộ lập lịch (scheduler).

#### Bộ nhớ:

- `mm.c`, `mm64.c`, `mm-vm.c`, `mm-memphy.c`: Cài đặt chi tiết về quản lý bộ nhớ phân trang.

#### Hệ thống & Thư viện:

- `syscall.c`, `syscall.tbl`, `syscalltbl.sh`, `sys_xxx.c`: Cài đặt các lời gọi hệ thống.

- `libmem.c`, `libstd.c`: Các hàm thư viện tiêu chuẩn.

### C. Cấu trúc khác
- `Makefile`: Tệp dùng để biên dịch (build) dự án.

- `input` / `output`: Chứa các tệp dữ liệu đầu vào để kiểm tra và kết quả mẫu.

### 2. Lý thuyết cốt lõi cần nắm
- **PCB (Process Control Block)**: Cấu trúc dữ liệu lưu trữ thông tin của một tiến trình (trạng thái, bộ đếm chương trình, định danh tiến trình).

- **Quản lý bộ nhớ phân trang (Paging)**: Cơ chế chia bộ nhớ thành các khối cố định (page/frame) để ánh xạ bộ nhớ ảo sang vật lý.

Dựa vào mô-đun đã cũ (có chú thích `obsoleted`), bạn sẽ tập trung chính vào việc lập trình và kiểm thử các tệp liên quan đến **Scheduling** (sched.c, `queue.c`) và **Memory Paging** (`mm.c`, `mm-vm.c`, v.v.).

## 1.3 Processes
### 1. Cấu trúc Quản lý Tiến trình (PCB - Process Control Block)
Trong hệ điều hành, PCB là cấu trúc dữ liệu cốt lõi để lưu trữ toàn bộ trạng thái và thông tin của một tiến trình. Dưới đây là các thành phần chính trong struct `pcb_t`:

- `pid`: Định danh duy nhất cho từng tiến trình (Process ID).

- `priority`: Độ ưu tiên của tiến trình. **Giá trị càng nhỏ, độ ưu tiên càng cao.** Giá trị này thường cố định trong suốt vòng đời tiến trình.

- `path`: Đường dẫn tới tệp chương trình của tiến trình.
- `code`: Trỏ tới vùng nhớ chứa mã nguồn (Text segment) của tiến trình.
- `regs`: Mảng lưu trữ trạng thái của 10 thanh ghi (từ 0 đến 9).
- `pc`: Program Counter – lưu địa chỉ của câu lệnh tiếp theo cần thực thi.
- `bp`: Break pointer – dùng để quản lý vùng nhớ Heap (vùng nhớ cấp phát động).
- `prio`: Độ ưu tiên thực thi động (được sử dụng nếu hệ thống hỗ trợ thuật toán MLQ - Multi-Level Queue).
- `page_table`: (Bị vô hiệu hóa trong bài tập này) Quản lý ánh xạ địa chỉ ảo sang vật lý.
### 2. Tập lệnh của Tiến trình
Tiến trình được mô phỏng dưới dạng một danh sách các câu lệnh chạy tuần tự. Dưới đây là ý nghĩa chi tiết của các lệnh:
#### A. Lệnh Tính toán
- `CALC`: Yêu cầu CPU thực hiện tính toán mà không cần tham số.
#### B. Lệnh Quản lý Bộ nhớ (Memory & Kernel Allocations)

- `alloc [size] [reg]`: Cấp phát size byte từ bộ nhớ RAM. Địa chỉ của byte đầu tiên được lưu vào thanh ghi reg.
- `kmalloc [size] [reg]`: Cấp phát size byte từ bộ nhớ kernel (yêu cầu vùng nhớ vật lý phải liên tục).
- `kmem_cache_create [size] [align] [cache_pool_id]`: Tạo một vùng nhớ đệm (cache pool) trong không gian Kernel.
- `kmem_cache_alloc [reg] [cache_pool_id]`: Cấp phát một khối nhớ từ vùng đệm (cache pool) đã tạo trước đó.

**Lý thuyết Bộ nhớ đệm Kernel (Kernel Memory Cache - Slab)**: Cơ chế này giúp hệ điều hành quản lý các đối tượng thường xuyên được tạo và hủy. Thay vì cấp phát mới (tốn thời gian), hệ thống sẽ chuẩn bị sẵn các khối bộ nhớ (memslot) để tái sử dụng ngay lập tức.
#### C. Lệnh Hủy cấp phát
- `free [reg]`: Giải phóng vùng nhớ mà địa chỉ của nó đang được lưu trong thanh ghi reg.
#### D. Lệnh Truy cập Bộ nhớ (User-space & Kernel-space)

- `read [source] [offset] [destination]`: Đọc 1 byte từ địa chỉ `source + offset` và lưu vào thanh ghi destination. 
    - **Lưu ý**: Chỉ được phép truy cập User-space.
- `write [data] [destination] [offset]`: Ghi dữ liệu data vào địa chỉ $(destination + offset)$ trong bộ nhớ. 
    - **Lưu ý**: Chỉ được phép truy cập User-space.
- `copy_from_user [source] [destination]`: Chuyển dữ liệu trực tiếp từ User-space (source) sang Kernel-space (destination).
- `copy_to_user [source] [destination] [offset]`: Chuyển dữ liệu từ Kernel-space (source) sang User-space (destination tại vị trí offset).
### 3. Những việc cần làm
Khi triển khai phần này, bạn cần:
- Hiểu rõ cấu trúc và cách lưu trữ trạng thái của tiến trình (thanh ghi, PC, PID, độ ưu tiên).
- Xây dựng các hàm xử lý các lệnh trên, đặc biệt là việc kiểm tra giới hạn truy cập (phân tách giữa User-space và Kernel-space) để đảm bảo an toàn cho hệ điều hành.

## 1.4 How to Create a Process?
Mỗi tiến trình được tạo ra từ một tệp chương trình lưu trên đĩa. Tệp này chứa các thông tin cơ bản và danh sách các lệnh mà tiến trình sẽ thực hiện.
### A. Cấu trúc tệp chương trình (Input file)

    [priority] [N = số lượng câu lệnh]
    [instruction 0]
    [instruction 1]
    ...
    [instruction N-1]
- `priority`: Độ ưu tiên mặc định của tiến trình (giá trị càng nhỏ, độ ưu tiên càng cao).

- `N`: Tổng số câu lệnh mà tiến trình sẽ thực thi.

- Các câu lệnh phía sau: Là danh sách các thao tác (như `CALC`, `ALLOC`, `WRITE`,...) đã được liệt kê ở phần trước.

### B. Cơ chế ưu tiên kép (Dual Priority Mechanism)
Hệ thống sử dụng cơ chế ưu tiên kép:

1. Độ ưu tiên mặc định được định nghĩa trong tệp chương trình.

2. Khi tiến trình được nạp vào hệ thống mô phỏng, giá trị này có thể bị ghi đè bởi giá trị ưu tiên trực tiếp từ tệp cấu hình môi trường.

## 1.5 How to Run the Simulation
Để hệ thống chạy mô phỏng, bạn cần tạo một tệp cấu hình môi trường (Configuration File) nằm trong thư mục `input/`. Tệp này mô tả phần cứng và các tiến trình sẽ chạy.

### A. Cấu trúc tệp cấu hình môi trường

    [time slice] [N = Số lượng CPU] [M = Số lượng tiến trình]
    [time 0] [path 0] [priority 0]
    [time 1] [path 1] [priority 1]
    ...
    [time M-1] [path M-1] [priority M-1]

- `time slice`: Khoảng thời gian (tính bằng giây) mà một tiến trình được phép chạy trên CPU trước khi bị ngắt (Time Quantum).

- `N`: Số lượng CPU ảo có sẵn trong hệ thống.

- `M`: Tổng số tiến trình sẽ được khởi chạy trong suốt quá trình mô phỏng.

- Các dòng tiếp theo: Định nghĩa thời điểm khởi chạy (time), đường dẫn tới tệp chương trình (path), và độ ưu tiên thực tế (priority sẽ ghi đè độ ưu tiên mặc định).

### B. Những việc cần làm
Khi triển khai phần này, bạn cần:

- **Biên dịch mã nguồn**: Sử dụng lệnh `make all` trong terminal.

- **Thực thi mô phỏng**: Chạy lệnh `./os [configure_file]` với `configure_file` là tệp chứa thông tin cấu hình (ví dụ: `input/sample.txt`).

## 1.6 How to Write the Kernel Interface
### 1.6.1 Kiến trúc Kernel và Quản lý Bộ nhớ
Cấu trúc `krnl_t` là thành phần trung tâm của hệ điều hành, đóng vai trò quản lý tài nguyên (tiến trình và bộ nhớ).

#### Cấu trúc Kernel (struct `krnl_t`):

    // From include/common.h
    /* Kernel structure */
    struct krnl_t
    {
        struct queue_t *ready_queue;
        struct queue_t *running_list;
    #ifdef MLQ_SCHED
        struct queue_t *mlq_ready_queue;
    #endif
    #ifdef MM_PAGING
        struct mm_struct *mm;
        struct memphy_struct *mram;
        struct memphy_struct **mswp;
        struct memphy_struct *active_mswp;
        uint32_t active_mswp_id;
    #endif
    };

- **Hàng đợi tiến trình**: `ready_queue`, `running_list`, và `mlq_ready_queue` (dùng cho thuật toán lập lịch nhiều hàng đợi - MLQ).

- **Thành phần bộ nhớ**: `mm_struct`, `mram` (bộ nhớ vật lý RAM), `mswp` (bộ nhớ swap), và con trỏ tới bộ nhớ swap đang hoạt động.
### 1.6.2 Lời gọi hệ thống (System Call) và Luồng xử lý
#### Khái niệm:
- System Call là giao diện lập trình cho phép các ứng dụng (User Mode) yêu cầu các dịch vụ từ Kernel.

- Các ứng dụng không gọi trực tiếp mà thông qua các hàm wrapper trong thư viện `libstd`. Các hàm này sẽ sao chép tham số vào thanh ghi và gọi lệnh ngắt hoặc cổng để chuyển sang Kernel Mode.

#### Các System Call chính trong bài:
- `listsyscall (SYSCALL 0)`: Hiển thị danh sách các system call đang được hỗ trợ trong hệ thống.

- `memmap (SYSCALL 17)`: Hỗ trợ các thao tác quản lý bộ nhớ như:

    - `SYSMEM_MAP_OP` (Áp dụng hàm `vmap_pgd_memset()`)

    - `SYSMEM_INC_OP` (Mở rộng giới hạn VMA)

    - `SYSMEM_SWP_OP` (Hoán đổi trang bộ nhớ)

    - `SYSMEM_IO_READ` / `SYSMEM_IO_WRITE` (Truy xuất I/O)

### 1.6.3 Hướng dẫn thêm một System Call mới
Bạn có thể thêm một system call mới thông qua 3 bước đơn giản:

1. **Bước 1**: Tạo tệp xử lý (`src/sys_xxxhandler.c`)

Tạo hàm xử lý system call trong thư mục `src`:

    // From src/sys_xxxhandler.c
    #include "common.h"
    #include "syscall.h"
    #include "stdio.h"
    int __sys_xxxhandler(struct pcb_t *caller, struct sc_regs* reg) {
        /* TODO: implement syscall job */
        printf("The first system call parameter %d\n", regs->a1);
        return 0;
    };

2. **Bước 2**: Khai báo trong `Makefile` và Bảng System Call

- Thêm vào `Makefile`: 

```makefile
# From Makefile
SYSCALL_OBJ += $(addprefix $(OBJ)/, sys_xxxhandler.o)
```

- Thêm vào `src/syscall.tbl`: 

```
# From src/syscall.tbl
440     xxx     sys_xxxhandler
```

3. **Bước 3**: Cài đặt và Kiểm tra

- Biên dịch lại hệ thống: `make all`

- Tạo chương trình gọi system call (ví dụ `sc` với lệnh `syscall 440 1`) và tệp cấu hình. Chạy mô phỏng bằng lệnh: `./os os_syscall`

#### Kiểm tra kết quả:
Phần này hướng dẫn bạn tạo một chương trình kiểm tra (test program) và tệp cấu hình để khởi động hệ điều hành, nhằm xác thực System Call vừa thêm vào có hoạt động chính xác hay không.

- **Bước 1**: Tạo chương trình kiểm tra `sc`

    - Tạo tệp thực thi `sc` chứa câu lệnh gọi system call (với ID là 440 và tham số là 1):

    ```Plaintext
    20 1
    syscall 440 1
    ```
    - **Ý nghĩa**: Chương trình sẽ nạp và thực thi câu lệnh system call có mã 440 với tham số đầu vào là 1.

- **Bước 2**: Tạo tệp cấu hình môi trường `os_syscall`

    - Tạo tệp cấu hình (ví dụ đặt tên là `os_syscall` trong thư mục `input/`) với nội dung sau:

    ```Plaintext
    2 1 1
    2048 16777216 0 0 0
    9 sc 15
    ```
    - Giải thích các thông số:

        - **Dòng 1**: 2 giây cho time_slice, 1 CPU, và 1 tiến trình được thực thi.

        - **Dòng 2**: Các tham số cấu hình bộ nhớ (kích thước bộ nhớ, địa chỉ cơ sở, v.v.).

        - **Dòng 3**: Khởi chạy tiến trình sc vào thời điểm 9 giây với độ ưu tiên là 15.

3. **Bước 3**: Khởi động hệ điều hành

- Biên dịch lại mã nguồn và chạy lệnh trên terminal:

```Bash
make all
./os os_syscall
```
4. **Bước 4**: Kiểm tra kết quả
- Đọc các dòng cuối cùng của đầu ra (output messages). Nếu cấu hình và lập trình đúng, bạn sẽ nhận được thông báo:

```Plaintext
The first system call parameter 1
CPU 0: Processed
CPU 0 stopped
1 has finished
Congratulations! You have successfully added a system call to the Simple Operating System!
```
### 1.6.4 Question trong bài
1.  What are the advantages and disadvantages of using the unified system call interface for manipulating different system components, i.e. read/write/free for files, memory and I/O devices. In your analysis,
consider how this abstraction influences operating system design, performance trade-offs, error handling
complexity, and the balance between portability and efficiency.
- **Ưu điểm:**

    - **Tính trừu tượng cao và nhất quán**: Lập trình viên không cần biết chi tiết phần cứng bên dưới đang là tệp, bộ nhớ hay thiết bị I/O.

    - **Tính di động (Portability)**: Dễ dàng chuyển mã nguồn giữa các hệ thống.

- **Nhược điểm:**

    - **Hiệu suất (Performance)**: Cần thông qua nhiều tầng trung gian, không tối ưu cho từng loại phần cứng đặc thù.

    - **Độ phức tạp trong xử lý lỗi**: Việc dùng chung giao diện khiến việc phân loại và xử lý lỗi trở nên phức tạp hơn.

2. When a system call executes too long time, how does the Operating system detect and handle
the case?
- **Phát hiện**: Hệ điều hành sử dụng cơ chế Timer Interrupt (Ngắt định thời). Khi tiến trình ở chế độ Kernel, bộ định thời vẫn hoạt động và phát ra tín hiệu ngắt nếu thời gian chạy vượt quá `time_slice` (thời lượng phân bổ).

- **Xử lý**:

    - Kernel lưu lại ngữ cảnh của tiến trình (Registers, PC).

    - Thu hồi quyền điều khiển CPU và chuyển trạng thái của tiến trình từ Running sang Ready.

    - Bộ lập lịch (Scheduler) sẽ chọn một tiến trình khác để chạy.

# xv6-labs-2020 实验
本项目是MIT 6.S081 (Operating System Engineering) 2020 秋季课程实验的实现。涉及页表、中断处理、文件系统、锁等知识，在项目中完成了懒加载、写时复制、用户级多线程、并发锁设计、符号链接功能实现等任务。
## 实验完成情况
通过了10个实验的测试：
- [x] Lab: Xv6 and Unix utilities
- [x] Lab: System calls
- [x] Lab: Page tables
- [x] Lab: Traps
- [x] Lab: Copy-on-Write fork
- [x] Lab: Multithreading
- [x] Lab: Network driver
- [x] Lab: Locks
- [x] Lab: File system
- [x] Lab: Mmap
## 分支说明
由于实验要求在不同的分支上进行开发，你可以通过切换分支查看每个实验的具体实现：
- `git checkout util` : 查看uitls实验，用于熟悉操作系统
- `git checkout syscall` : 查看系统调用实验，包含两个实验：实现一个追踪系统调用的trace系统调用、实现一个用于获取空闲内存量的sysinfo函数
- `git checkout pgtbl` : 查看页表实验，包含：页表打印、内核的进程页表实现
- `git checkout traps` : 查看中断处理实验，包含：回溯实验、时钟实验
- `git checkout lazy` : 查看懒加载实现
- `git checkout cow` : 查看写时复制实现
- `git checkout thread` : 查看多进程实现，包含用户级线程上下文切换实现、用户级多线程实现、屏障实现
- `git checkout Locks` : 查看锁实现，包含修改kmem锁的粒度以减少锁争用、采用哈希桶锁优化buffer cache锁提高性能
- `git checkout fs` : 查看文件系统实验，包含修改文件Inode结构使文件系统支持大文件存储、实现符号链接
- `git checkout mmap` : 查看内存映射文件实现
## 🛠 运行环境
- **OS**: Ubuntu 20.04 / 22.04 
- **Compiler**: riscv64-unknown-elf-gcc
- **Emulator**: QEMU 5.0+

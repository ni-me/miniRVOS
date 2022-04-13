# miniRVOS
miniRVOS是一个基于课程[《循序渐进，学习开发一个 RISC-V 上的操作系统》](https://www.bilibili.com/video/BV1Q5411w7z5)实现的一个简易操作系统。

## 功能

- [x] 串口输入输出
- [x] 物理内存管理：内存分配和回收
- [x] 多线程支持：基于优先级和时间片进行调度
- [x] 中断支持：外部中断、软中断和时间中断
- [x] 任务同步：自旋锁
- [x] 软件定时器
- [x] 简单的系统调用

## 环境和构建

### 环境
运行环境为``Ubuntu 20.04``，详细的系统版本信息如下：

```bash

$ lsb_release -a
No LSB modules are available.
Distributor ID: Ubuntu
Description:    Ubuntu 20.04.4 LTS
Release:        20.04
Codename:       focal

$ uname -r
5.10.16.3-microsoft-standard-WSL2

```

在``Ubuntu 20.04``环境下，请首先运行下列代码安装工具链（tool chain）：

```bash
$ sudo apt update
$ sudo apt install build-essential gcc make perl dkms git gcc-riscv64-unknown-elf gdb-multiarch qemu-system-misc

```

### 构建

完成环境搭建后，进入``os``目录，运行``make``即可进行构建。下列为构建命令，具体请参考项目中的``Makefile``文件：

- ``make``：编译构建
- ``make run``：启动 ``qemu`` 并运行
- ``make debug``：启动 ``GDB`` 进行调试
- ``make code``：反汇编查看二进制代码
- ``make clean``：清理生成的文件


## 参考
- [https://gitee.com/unicornx/riscv-operating-system-mooc](https://gitee.com/unicornx/riscv-operating-system-mooc)

- [https://github.com/cccriscv/mini-riscv-os](https://github.com/cccriscv/mini-riscv-os)

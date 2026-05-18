# x64dbg-linux

x64dbg 的 Linux 移植版本，基于 ElfBug 调试引擎和 Qt5 GUI 框架。

## 概述

这是一个功能完整的 Linux 调试器，复用 x64dbg 的跨平台组件：
- **ElfBug**: Linux ptrace 调试引擎
- **widgets**: 跨平台 GUI 组件库
- **debugger**: 基础 GUI 框架参考实现

## 目录结构

```
src/cross/x64dbg-linux/
├── cmake.toml              # cmkr 构建配置
├── CMakeLists.txt          # CMake 入口（由 cmkr 生成）
├── Dockerfile              # Docker 构建环境
├── build-appimage.sh       # AppImage 打包脚本
├── main/                   # 程序入口
│   └── main.cpp
├── core/                   # 调试核心
│   ├── DbgAdapter.cpp/h    # ElfBug 适配层
│   └── LinuxArchitecture.h
├── gui/                    # 图形界面
│   ├── MainWindow.cpp/h    # 主窗口
│   └── CPUStack.cpp/h      # 堆栈视图
├── bridge/                 # 桥接层（预留）
├── commands/               # 命令系统（预留）
├── plugins/                # 插件系统（预留）
└── resources/              # 资源文件（预留）
```

## 构建

### 方法 1: Docker 构建（推荐）

```bash
cd src/cross/x64dbg-linux

# 构建 Docker 镜像
docker build -t x64dbg-linux-builder .

# 运行容器并编译
docker run --rm \
    -v $(pwd)/../..:/build \
    -w /build \
    x64dbg-linux-builder \
    bash -c "cd src/cross/x64dbg-linux && cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build --target x64dbg-linux"

# 输出二进制
./build/x64dbg-linux
```

### 方法 2: 本地构建

**依赖:**
- GCC 13+ 或 Clang
- CMake 3.19+
- Ninja
- Qt5 (Widgets, Svg, WebSockets)
- libelf-dev, libdw-dev, libunwind-dev

```bash
cd src/cross/x64dbg-linux

# 配置
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# 编译
cmake --build build --target x64dbg-linux

# 运行
./build/x64dbg-linux
```

## AppImage 打包

使用 `build-appimage.sh` 脚本打包成 AppImage 分发格式：

```bash
cd src/cross/x64dbg-linux

# 方法 1: 在 Docker 中打包（推荐）
docker run --rm \
    -v $(pwd)/../..:/build \
    -w /build \
    x64dbg-linux-builder \
    bash -c "cd src/cross/x64dbg-linux && ./build-appimage.sh"

# 方法 2: 本地打包（需要 linuxdeploy）
./build-appimage.sh
```

打包完成后，会在 `build/` 目录下生成 `x64dbg-linux-x86_64.AppImage`。

### build-appimage.sh 使用说明

```bash
./build-appimage.sh [选项]

环境变量:
  BUILD_DIR       构建目录 (默认: ./build)
  OUTPUT          输出文件名 (默认: ./build/x64dbg-linux-x86_64.AppImage)

示例:
  # 默认打包
  ./build-appimage.sh

  # 指定输出路径
  OUTPUT=./dist/x64dbg.AppImage ./build-appimage.sh
```

## 开发工作流

### 启动开发容器

```bash
docker run -it --rm \
    -v $(pwd)/../..:/build \
    -w /build \
    x64dbg-linux-builder \
    bash

# 在容器内
cd src/cross/x64dbg-linux
cmake -B build -G Ninja
cmake --build build --target x64dbg-linux
```

### 增量编译

```bash
# 修改代码后，只编译变更部分
cmake --build build --target x64dbg-linux
```

### 清理构建

```bash
rm -rf build/
```

## 技术细节

### 构建系统

- **cmkr**: 基于 TOML 的 CMake 生成器
- **cmake.toml**: 主构建配置文件
- **CMakeLists.txt**: 由 cmkr 自动生成，不要手动编辑

### 调试引擎

ElfBug 提供以下功能：
- 进程创建和附加（ptrace）
- 软件断点（INT3）
- 内存读写（/proc/pid/mem）
- 寄存器访问（PTRACE_GETREGS/SETREGS）
- 线程管理
- 信号处理

### GUI 组件

复用 widgets 库的组件：
- Disassembly（Zydis 反汇编）
- HexDump（十六进制查看）
- RegistersView（寄存器显示）
- 暗黑主题支持

## 与 Windows 版本的差异

| 功能 | Windows (x64dbg) | Linux (x64dbg-linux) |
|------|------------------|----------------------|
| 调试引擎 | TitanEngine/GleeBug | ElfBug |
| 进程创建 | CreateProcess | fork + execve |
| 断点 | WriteProcessMemory | ptrace POKEDATA |
| 内存读取 | ReadProcessMemory | /proc/pid/mem |
| 寄存器 | GetThreadContext | ptrace GETREGS |
| 模块枚举 | PSAPI | /proc/pid/maps |
| 符号解析 | dbghelp | libdw/libbfd |

## 许可证

与 x64dbg 主项目相同（GPLv3）。

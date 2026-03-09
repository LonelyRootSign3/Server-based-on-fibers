# C++ 高性能协程网络框架 (Server-based-on-Fibers)

![C++](https://img.shields.io/badge/C++-17-blue.svg)
![Build](https://img.shields.io/badge/build-CMake-brightgreen.svg)
![Platform](https://img.shields.io/badge/platform-Linux-lightgrey.svg)

本项目是一个基于 **C++17** 编写的高性能服务器网络框架。核心采用 **“协程 (Fiber) +  epoll + 调度器”** 的异步非阻塞并发模型。通过底层的 **System Call Hook** 技术，开发者可以使用同步的思维编写代码，框架在底层自动进行异步的协程切换，极大地降低了高并发网络编程的门槛。

框架包含完善的基础组件、网络通信组件以及 HTTP 协议栈（包含 HTTP 服务端、路由分发与客户端连接池），可用于快速构建高性能的微服务、网关或 Web 后端。

---

## ✨ 核心特性 (Features)

- **日志系统 (Log)**：参考log4j,支持多目的地（Console、File）、多级别配置，支持基于 `yaml` 文件的热加载与动态修改。
- **配置系统 (Config)**：基于 `yaml-cpp`，采用约定优于配置的理念，支持系统级别的参数绑定与回调监听。
- **协程模块 (Fiber)**：基于 `ucontext_t` 实现的非对称纯用户态协程，支持在代码执行过程中随时 Yield 让出执行权。
- **协程调度器 (Scheduler)**：N:M 协程调度模型，将 M 个协程动态分配给 N 个线程执行。
- **IO 协程调度器 (IOManager)**：基于 `epoll` 实现，集成定时器 (`Timer`) 功能，将网络 IO 事件与协程调度完美结合。
- **系统 Hook (Hook)**：接管底层的 `read`, `write`, `socket`, `sleep` 等系统调用。当网络 IO 阻塞时，自动挂起当前协程并切换执行其他协程，数据就绪时自动唤醒，**实现“同步编码，异步执行”**。
- **网络基础设施 (Net)**：封装了 IPv4/IPv6 地址解析 (`Address`)、`Socket`、面向字节流的 `ByteArray` 与基础的 `TcpServer`。
- **HTTP 协议栈 (Http)**：
  - 基于高性能 C 库 `picohttpparser` 封装的 HTTP 报文解析器。
  - 支持 `Servlet` 规范的路由分发器，支持精准匹配、模糊匹配与 Lambda 匿名函数处理。
  - 极度优化的 `HttpConnection` 客户端，支持流式大文件下载。
  - 高并发安全的 **HTTP 客户端连接池**，支持复用保活与后台自动清理过期连接。
  - 基于 **Ragel 状态机** 的极致 URI 解析模块。

---

## 🛠️ 环境与依赖 (Dependencies)

- **操作系统**：Linux (推荐 Ubuntu 20.04+ 或 CentOS 8+)
- **编译器**：GCC 9.0+ 或 Clang 10.0+ (需完全支持 C++17)
- **构建工具**：CMake >= 3.14
- **第三方库**：
  - `yaml-cpp`：用于解析 YAML 配置文件。
  - `pthread`, `dl`：系统线程与动态链接库。
  - `ragel`：用于编译 `Uri.rl` 状态机生成高性能 C++ 代码。

**依赖安装 (Ubuntu 为例)：**
```bash
sudo apt-get update
sudo apt-get install build-essential cmake
sudo apt-get install libyaml-cpp-dev
sudo apt-get install ragel

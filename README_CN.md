# EyeBreak

**轻量级 Windows 护眼提醒工具，驻留系统托盘，每 20 分钟弹出通知提醒你远眺休息。**

[![平台](https://img.shields.io/badge/平台-Windows_7%2B-blue)](https://www.microsoft.com/windows)
[![语言](https://img.shields.io/badge/语言-C_Win32_API-purple)](https://docs.microsoft.com/cpp/)
[![协议](https://img.shields.io/badge/协议-MIT-green)](LICENSE)
[![体积](https://img.shields.io/badge/体积-144KB-orange)](#)

[**English →**](README.md)

---

## 概览

| | |
|---|---|
| **体积** | **~144 KB** — 单个可执行文件，无需安装，无外部依赖 |
| **内存占用** | 运行时约 ~2 MB RAM |
| **编程语言** | C (Win32 API) |
| **支持系统** | Windows 7 SP1+ (64 位) |

双击运行。右键托盘图标进行操作。

---

## 为什么需要

### 20-20-20 护眼法则

美国眼科学会（AAO）推荐的 **20-20-20 护眼法则**：

> 每 **20 分钟**，向 **6 米（20 英尺）外**注视 **20 秒**。

### 医学研究

| 研究 | 结论 | 来源 |
|------|------|------|
| Blehm 等 (2005) | 长时间近距离用眼使眨眼频率从约 15 次/分钟降至约 5 次/分钟；定时休息可使症状减轻 50–65%。 | [*Optometry and Vision Science*](https://doi.org/10.1097/01.OPX.0000168706.96613.C2) |
| Sheppard & Wolffsohn (2018) | 纳入 1,454 名参与者的荟萃分析证实：20-20 法则显著降低视疲劳和干眼症状。 | [*BMJ Open*](https://doi.org/10.1136/bmjopen-2017-020189) |
| Jeon 等 (2021) | **不完全眨眼率**与睑板腺缺失呈强正相关（r=0.811），与干眼症状严重程度中度相关（r=0.596）。不完全闭眼是蒸发性干眼的核心驱动因素。 | [*International Ophthalmology* / PMC7993415](https://doi.org/10.1007/s10792-020-03600-w) |
| Kim 等 (2021) | **眨眼练习**（每 20 分钟 1 次，**每次 10 秒含用力紧闭眼睑**）：不完全眨眼比例从 54% 降至 34%（p<0.001），泪膜破裂时间从 6.5s 延长至 8.1s（n=41）。 | [*Cont Lens Anterior Eye*](https://doi.org/10.1016/j.clae.2020.04.014) |

> **完整建议：** 每 20 分钟，向 6 米外注视 **20 秒**，然后**用力紧闭双眼并保持挤压状态 10 秒**（Kim 等，《Cont Lens & Anterior Eye》，2021）——"挤压"步骤确保上下眼睑完全闭合，促进泪膜重新分布和睑板腺脂质排出。

EyeBreak 只做一件事：到点提醒你抬头。

---

## 功能特性

- **20 分钟定时** — 到时弹出气泡通知
- **锁屏感知** — 锁屏/解锁自动重置计时器（WTS API）
- **实时倒计时** — 鼠标悬停托盘图标显示 MM:SS
- **开机自启** — 可选，通过注册表 HKCU\Run 管理
- **中英文界面** — 右键菜单切换，重启后保留语言偏好
- **单实例运行** — Mutex 防重复启动
- **无痕卸载** — 删除 exe 即可

---

## 使用方法

### 快速上手

1. 下载 `dist/EyeBreak.exe`
2. 双击运行
3. 图标出现在系统托盘中
4. 右键打开菜单：

```
┌──────────────────────┐
│ 发送测试通知          │
│ 停止提醒              │
│ ✓ 开机自启            │
│ ────────────────── │
│ Switch to English     │   ← 切换语言（重启后保留）
│ ────────────────── │
│ 退出                  │
└──────────────────────┘
```

默认界面语言为中文。语言偏好通过注册表持久保存，重启后自动恢复。

右键点击托盘图标打开菜单。

---

## 技术方案

| 项目 | 说明 |
|------|------|
| **编程语言** | C99 (MSVC) |
| **API** | 纯 Win32 —— 无框架，除 MSVCRT 外无其他 CRT 依赖 |
| **编译器** | `cl.exe` (Visual Studio 2022) 或 Makefile |
| **链接方式** | 静态链接 `/MT` —— 无需 VC++ 运行时库 |
| **架构** | 消息驱动隐藏窗口 + 托盘图标 |
| **锁屏检测** | WTS (`WTSRegisterSessionNotification`) —— 事件驱动，零轮询开销 |
| **开机自启** | 注册表 `HKCU\Software\Microsoft\Windows\CurrentVersion\Run` |
| **单实例** | 命名内核 Mutex |
| **日志系统** | 编译时消除（Release 版本宏展开为空操作） |

### 依赖库

所有链接的库均为 **Windows 系统自带 DLL** —— 零第三方依赖：

| 库文件 | 用途 |
|--------|------|
| `user32.dll` | 窗口创建、消息循环、菜单 |
| `shell32.dll` | 托盘图标 (`Shell_NotifyIconW`) |
| `advapi32.dll` | 注册表操作、Mutex |
| `ole32.dll` | Shell API 内部所需的 OLE 组件 |
| `wtsapi32.dll` | WTS 会话变更通知 |

无需 .NET 运行时、Python、Electron 或 Node.js。一个纯原生二进制程序。

---

## 编译

### 环境要求

- **Windows 7 SP1+** (64 位)
- **Visual Studio 2022**（Build Tools 版本也可）

### 方式一：build.bat（推荐）

```
build.bat
```

输出至 `dist/EyeBreak.exe`。若 VS 安装在非默认位置需修改 `vcvars64.bat` 路径。

### 方式二：Makefile

```bash
make          # Release 编译
make debug    # Debug 编译（启用 EYEBREAK_DEBUG 日志）
make clean    # 清理编译产物
```

### 方式三：手动命令行

```bat
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
rc.exe /I include /Fo build\resources.res res\resources.rc
cl.exe /MT /O2 /W3 /utf-8 /I include /Fe:dist\EyeBreak.exe /Fobuild\ src\eye_break.c build\resources.res ^
    /link /SUBSYSTEM:WINDOWS user32.lib shell32.lib advapi32.lib ole32.lib wtsapi32.lib
```

---

## 项目结构

```
EyeBreak/
├── src/
│   └── eye_break.c       # 全部源码（~767 行）
├── include/
│   └── resources.h       # 资源 ID 定义
├── res/
│   ├── resources.rc      # 资源脚本（绑定托盘图标）
│   └── tray_icon.ico     # 托盘图标（编译进 exe）
├── build/                # 中间编译产物
├── dist/                 # 编译输出目录
│   └── EyeBreak.exe      # 最终可执行文件 (~144 KB)
├── log/                  # 调试日志
├── Makefile              # 跨平台构建脚本
├── build.bat             # Windows 编译脚本（推荐）
├── .gitignore            # Git 忽略规则
├── LICENSE               # MIT 协议
├── README.md             # 英文文档
└── README_CN.md          # 本文档（中文）
```

更换托盘图标步骤：替换 `res/tray_icon.ico` → 重新编译。

---

## 设计说明

### 隐藏窗口

控制台程序无法接收 Windows 消息（托盘回调、会话通知、定时器）。隐藏窗口仅作为消息泵存在，从不显示。

### 会话检测（WTS）

使用 `WTSRegisterSessionNotification`（`WM_WTSSESSION_CHANGE`）检测锁屏/解锁——操作系统原生事件驱动方案，零轮询开销。

### 安全软件拦截处理

安全软件（联想电脑管家、360、火绒等）常异步删除注册表启动项。EyeBreak 采用"乐观写入 + 延迟验证"策略：写入后等待 2 秒再回读，若值被删除则弹窗引导用户处理。

### 日志编译时消除

```c
#ifdef EYEBREAK_DEBUG
#define LOG(fmt, ...) WriteLog(fmt, ##__VA_ARGS__)
#else
#define LOG(fmt, ...) ((void)0)  // Release 下编译器优化后完全移除
#endif
```

Release 版本不包含任何日志代码，零运行时开销。

---

## 卸载方法

**完全不留痕——无安装程序，注册表仅存语言偏好（`HKCU\Software\EyeBreak`），不在程序目录外产生任何文件：**

1. 右键托盘图标 → **退出**
2. 删除 `EyeBreak.exe`
3. 完成。

如果之前开启了开机自启，退出前在菜单中关闭自启即可自动清除注册表项。即使未清除，残留的注册表值指向不存在的路径，Windows 会静默忽略，不会产生任何影响。

---

## 已知限制

- 气泡通知的实际显示时长由 Windows 控制，`uTimeout` 仅作参考
- 将 exe 移动到其他路径后，自启动注册表条目失效（重新开启一次即可）
- 未对多显示器场景做特殊处理
- 以标准用户权限运行——无需管理员权限

---

## 规划方向

- [ ] 可配置提醒间隔（不仅限于 20 分钟）
- [ ] 自定义提醒文案
- [ ] 免打扰时段（如 23:00–08:00 不提醒）
- [ ] 最长暂停时限（超时后强制提醒）
- [ ] 自定义提示音
- [ ] "关于"对话框

---

## 开源协议

[MIT](LICENSE)

# WealthPilot 部署指南

## 系统要求

### 硬件要求

- CPU: Intel Core i5 或更高
- 内存: 4GB RAM 或更高
- 磁盘: 500MB 可用空间
- 显示器: 1280x720 或更高分辨率

### 软件要求

- 操作系统: Windows 10/11 (64位)
- Qt: 6.10.2
- MinGW: 13.1.0 (64位)
- CMake: 3.16+

## 开发环境搭建

### 1. 安装 Qt

下载并安装 Qt 6.10.2：

- 访问 https://www.qt.io/download
- 选择 Qt 6.10.2 for Windows (MinGW 64-bit)
- 安装路径建议: `C:\Qt\6.10.2\mingw_64`

### 2. 安装 MinGW

Qt 安装包已包含 MinGW，路径为：

```
C:\Qt\Tools\mingw1310_64\bin
```

### 3. 安装 CMake

Qt 安装包已包含 CMake，路径为：

```
C:\Qt\Tools\CMake_64\bin
```

### 4. 配置环境变量

将以下路径添加到 PATH：

```
C:\Qt\6.10.2\mingw_64\bin
C:\Qt\Tools\mingw1310_64\bin
C:\Qt\Tools\CMake_64\bin
```

## 编译项目

### 1. 克隆代码

```bash
git clone https://github.com/your-repo/wealth-pilot.git
cd wealth-pilot
```

### 2. 创建构建目录

```bash
mkdir cmake-build-debug
cd cmake-build-debug
```

### 3. 配置项目

```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug
```

### 4. 编译项目

```bash
cmake --build . --target WealthPilot
```

### 5. 运行项目

使用启动脚本：

```bash
start.bat
```

或手动设置 PATH：

```bash
set PATH=C:\Qt\6.10.2\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;%PATH%
.\WealthPilot.exe
```

## 发布部署

### 1. 编译 Release 版本

```bash
mkdir cmake-build-release
cd cmake-build-release
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --target WealthPilot
```

### 2. 使用 windeployqt

```bash
windeployqt WealthPilot.exe
```

这将自动复制所有必需的 Qt DLL 到程序目录。

### 3. 手动复制依赖

如果 windeployqt 未复制所有依赖，手动添加：

```
Qt6Core.dll
Qt6Gui.dll
Qt6Widgets.dll
Qt6Network.dll
Qt6Sql.dll
Qt6Charts.dll
Qt6Concurrent.dll
Qt6Svg.dll
Qt6Multimedia.dll
Qt6OpenGL.dll
Qt6OpenGLWidgets.dll
Qt6Quick.dll
Qt6Qml.dll
Qt6QuickWidgets.dll
Qt6StateMachine.dll
Qt6TextToSpeech.dll
Qt6WebSockets.dll
Qt6Core5Compat.dll
```

以及 MinGW 运行时：

```
libgcc_s_seh-1.dll
libstdc++-6.dll
libwinpthread-1.dll
```

### 4. 创建安装包

使用 Inno Setup 或 NSIS 创建安装程序。

**Inno Setup 示例脚本:**

```ini
[Setup]
AppName=WealthPilot
AppVersion=1.0.0
DefaultDirName={pf}\WealthPilot
DefaultGroupName=WealthPilot
OutputDir=installer
OutputBaseFilename=WealthPilotSetup

[Files]
Source: "cmake-build-release\*"; DestDir: "{app}"; Flags: recurse

[Icons]
Name: "{group}\WealthPilot"; Filename: "{app}\WealthPilot.exe"
Name: "{commondesktop}\WealthPilot"; Filename: "{app}\WealthPilot.exe"

[Run]
Filename: "{app}\WealthPilot.exe"; Description: "启动 WealthPilot"; Flags: postinstall nowait skipifsilent
```

## 配置文件部署

### 1. 数据源配置

复制 `config/dataSources.json` 到安装目录的 `config/` 子目录。

### 2. CTP 配置

复制 `config/ctp_config.json` 并填写用户账号信息。

**注意:** CTP 账号需要从 SimNow 注册获取：
https://www.simnow.com.cn/

### 3. AI 配置

复制 `config/ai_config.json` 并填写 API Key。

**智谱AI API Key 获取:**
https://open.bigmodel.cn/

## 日志管理

### 日志位置

日志文件存储在 `logs/` 目录：

```
logs/wealthpilot-YYYY-MM-DD.log
```

### 日志清理

程序自动清理超过 7 天的日志文件。

### 日志级别

可在 `EnvironmentConfig` 中设置日志级别：

- debug
- info
- warning
- error

## 数据库管理

### 数据库位置

数据库文件存储在 `data/` 目录：

```
data/wealthpilot.db
```

### 数据库备份

定期备份 `data/wealthpilot.db` 文件。

## 常见问题

### Q: 程序启动失败，提示找不到 DLL

**解决方案:**

1. 确保 PATH 包含 Qt 和 MinGW 的 bin 目录
2. 使用 `windeployqt` 复制依赖
3. 使用 `start.bat` 启动脚本

### Q: CTP 连接失败

**解决方案:**

1. 检查 `config/ctp_config.json` 配置
2. 确保 SimNow 账号正确
3. 检查网络连接

### Q: AI 功能不可用

**解决方案:**

1. 检查 `config/ai_config.json` 配置
2. 确保 API Key 正确
3. 检查 API 配额

### Q: 数据源无法获取数据

**解决方案:**

1. 检查 `config/dataSources.json` 配置
2. 检查网络连接
3. 检查 API 是否可用

## 更新升级

### 手动更新

1. 下载新版本安装包
2. 运行安装程序覆盖旧版本
3. 保留 `config/` 和 `data/` 目录

### 自动更新

未来版本将支持自动更新功能。

---

*部署指南最后更新: 2026-05-15*
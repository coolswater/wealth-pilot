# WealthPilot 领航资产管理AI助手- 项目说明文档

## 1. 项目
WealthPilot 是一个基于 Qt 框架开发的金融信息展示与分析软件，专为 PC 平台设计。该软件提供股票、期货等金融产品的实时数据展示、自选股管理、市场全景等功能，旨在为用户提供全面的金融市场信息。

## 2. 技术栈与环境

- **开发语言**: C++ 17
- **GUI 框架**: Qt 6.x (兼容 Qt 5.x)
- **构建系统**: CMake 3.16+
- **国际化**: Qt Linguist Tools
- **操作系统**: 跨平台支持 (当前主要针对 Windows，其次 macOS)

## 3. 项目结构
```
wealth-pilot/
├── CMakeLists.txt                          # 根 CMake
├── docs/                                   # 项目文档
├── external/                               # 第三方依赖（vcpkg 管理）
├── resources/                              # 静态资源
│   └── fonts/                              # 字体                            
│   ├── i18n/                               # 国际化翻译文
│   ├── icons/                              # icons文件
│   ├── images/                             # 图片
│   ├── style/                              # 样式表
│   ├── resources.qrc                       # 静态资源配置
├── scripts/                                # 构建脚本、打包脚本
├── src/                                    # 源码根目录
│   ├── controllers/                        # 控制器
│   ├── core/                               # 核心工具
│   ├── models/                             # 数据模型
│   ├── network/                            # 网络模块
│   ├── services/                           # 公共服务（业务无关但被业务模块调用）
│   ├── utils/                              # 通用工具类
│   ├── views/                              # 视图类
│   │   ├── cryptoCurrency/                 # 数字货币
│   │   ├── dashboard/                      # 主仪表盘
│   │   └── forex/                          # 外汇模块
│   │   ├── fund/                           # 基金模块
│   │   ├── futures/                        # 期货模块
│   │   ├── hkstock/                        # 港股模块
│   │   ├── mainWindow/                     # 主窗口
│   │   ├── news/                           # 资讯模块
│   │   ├── portfolio/                      # 投资组合
│   │   ├── settings/                       # 系统设置
│   │   ├── signalCenter/                   # 信号中心
│   │   ├── stock/                          # 股票模块
│   │   ├── user/                           # 用户模块
│   │   ├── usStock/                        # 美股模块
│   │   ├── warning/                        # 预警模块
│   │   ├── widgets/                        # 控件模块
│── tests/                                  # 单元测试
└── .gitignore
└── CMakeLists.txt
└── main.cpp
└── README.md
```

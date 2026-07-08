# 智能全自动和面机系统 —— 全国大学生嵌入式程序设计大赛参赛文档

---

## 2.1 整体介绍

### 系统整体框图

![系统整体框图]

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           智能全自动和面机系统                                 │
└─────────────────────────────────────────────────────────────────────────────┘
                                      │
        ┌─────────────┬───────────────┼───────────────┬──────────────┐
        ▼             ▼               ▼               ▼              ▼
  ┌──────────┐ ┌──────────┐ ┌──────────────┐ ┌──────────┐ ┌──────────────┐
  │ 微信小程序 │ │ 触控屏UI  │ │  小智AI服务端 │ │ HTTP网页  │ │  ESP32-S3    │
  │ (用户入口) │ │ (本地交互) │ │  (智能中枢)   │ │ (远程控制) │ │  (主控+执行)  │
  └─────┬─────┘ └─────┬─────┘ └──────┬───────┘ └─────┬────┘ └──────┬───────┘
        │             │               │               │              │
        │    ┌────────┴────────┐      │               │    ┌────────┴────────┐
        │    │  ctl_mutex      │      │               │    │  硬件执行层      │
        │    │  三端互斥控制    │      │               │    │                  │
        │    └────────┬────────┘      │               │    │ ┌─ 水泵 DRV8870  │
        │             │               │               │    │ │  GPIO12(PWM)  │
        │             └───────┬───────┘               │    │ │  GPIO13(LOW)  │
        │                     │                        │    │ ├─ 研磨继电器    │
        │              ┌──────┴──────┐                 │    │ │  GPIO46       │
        │              │  function.c │                 │    │ ├─ 面粉ESC      │
        │              │  核心工作流  │                 │    │ │  GPIO15       │
        │              │  weight_work│                 │    │ ├─ 搅拌ESC      │
        │              │  fwork/fstop│                 │    │ │  GPIO9        │
        │              └─────────────┘                 │    │ ├─ 旋转舵机      │
        │                                              │    │ │  GPIO8        │
        │                                              │    │ ├─ 击打舵机      │
        │                                              │    │ │  GPIO3        │
        │                                              │    │ ├─ 步进电机      │
        │                                              │    │ │  GPIO4/5/6   │
        │                                              │    │ ├─ 面粉磁铁      │
        │                                              │    │ │  GPIO18       │
        │                                              │    │ ├─ 流量计 YF-S201│
        │                                              │    │ │  GPIO37       │
        │                                              │    │ └─ 麦克风+喇叭   │
        │                                              │    │   I2S 14/16/17/7│
        └──────────────────────────────────────────────┘    └────────────────┘
```

**> 建议拍摄：ESP32-S3 开发板 + DRV8870 驱动模块 + 电机 + 触摸屏 + 麦克风/喇叭 的实物连接全景照片**

### 文字说明

本系统以 **ESP32-S3** 为主控芯片，构建了一套智能全自动和面机。系统采用四层架构：

**1. 用户交互层**：提供三种控制入口——微信小程序（语音指令经小智AI解析后下发）、2.4寸触控屏（LVGL/EEZ Studio 图形界面）、网页控制面板（HTTP REST API），通过统一的互斥锁机制保证同一时刻仅一端控制。

**2. 智能服务层**：部署在PC/云端的**小智AI服务端**（xiaozhi-server），集成 ASR 语音识别、LLM 大语言模型（DeepSeek）、TTS 语音合成及营养推荐引擎。用户通过微信小程序描述当日饮食和面食需求，服务端解析营养数据并计算面团配比（杂粮粉/普通面粉/水的精确克重），生成结构化参数下发给和面机执行。

**3. 主控决策层**：ESP32-S3 运行 FreeRTOS 多任务系统，核心工作流引擎 `function.c` 根据接收到的参数，按配方比例驱动各执行机构完成面粉加入→加水→研磨→加料→搅拌的全自动流程。

**4. 硬件执行层**：水泵采用 DRV8870 电机驱动模块（5kHz PWM，GPIO12/13），研磨与搅拌采用无刷电调（ESC 50Hz PWM），加料机构由双舵机协同完成，配以 YF-S201 霍尔流量计实现闭环水量控制。

---

## 2.3 软件系统介绍

### 2.3.1 软件整体介绍

系统软件分为**设备端固件**、**智能服务端**和**微信小程序**三个部分，通过 WiFi 网络互联。

**设备端固件**（ESP32-S3, C/C++）：
- 基于 ESP-IDF v6.0 和 FreeRTOS 实时操作系统
- 图形界面采用 LVGL v9 + EEZ Studio 可视化设计
- 音频管线采用 I2S 全双工 + Opus 编解码 + ESP-SR 唤醒词引擎
- WebSocket 长连接维持与智能服务端的实时通信
- 三端控制互斥锁（ctl_mutex）防止多源并发冲突

**智能服务端**（Python, 部署于PC/云端）：
- WebSocket 服务器接收设备音频流，转发至 ASR 引擎识别
- 调用 DeepSeek/ChatGLM 等大语言模型理解用户意图
- TTS 引擎将回复文本合成为语音返回设备播放
- MCP（Model Context Protocol）工具调用机制实现设备控制指令下发
- HTTP API 为微信小程序提供营养推荐服务（`nutrition-service`）
- SQLite 数据库存储用户饮食记录与推荐历史

**微信小程序**（uni-app/Vue）：
- 聊天式交互界面，接收用户饮食描述
- 调用服务端营养接口获取解析结果与配比推荐
- 确认后通过服务端中继下发和面任务至设备端
- 历史记录查询与推荐快照展示

**> 建议拍摄：微信小程序聊天界面截图 + 服务端终端运行截图 + ESP32 串口日志截图**

### 2.3.2 软件各模块介绍

---

#### 模块一：核心工作流引擎（function.c）

该模块是和面机自动化的核心，定义了从配方计算到多阶段执行的全部逻辑。

**主函数调用链：**

```
app_main()
  └─ init_all()               // 初始化全部外设与任务
       ├─ ledc_pwm_set_state(0)   // 水泵 DRV8870 PWM 初始化
       ├─ servo_init(8)           // 旋转舵机
       ├─ servo2_init(3)          // 击打舵机
       ├─ stepper_init()          // 步进电机
       ├─ esc_init(&esc1, 15)     // 面粉电调
       ├─ esc_init(&esc2, 9)      // 搅拌电调
       ├─ wifi_init_softap()      // WiFi AP (SSID: dough_mixer)
       ├─ tcp_server_task()       // TCP 指令服务器 (:8080)
       ├─ http_server_start()     // HTTP REST API (:80)
       ├─ xz_init()               // 小智语音控制
       ├─ bsp_lcd_init()          // ST7789V LCD
       ├─ lvgl_ui_init()          // LVGL 图形框架
       └─ ui_init()               // EEZ Studio UI 加载
  └─ while(1) { ui_tick() }       // 主循环驱动 UI (10ms)
```

**`weight_work(weight)` 流程图：**

![weight_work 流程图]

```
weight_work(300g)
  │
  ├─ flour  = (int)((300 × k_flour  × 1000) / 2.8324)   // 面粉量
  ├─ water  = (int)((300 × k_water  × 1000) / 28.9)     // 水量
  ├─ grain  = (int)((300 × k_grain  × 1000 × 60) / 4.43) // 杂粮量
  ├─ yeast  = (int)((300 × k_yeast) / 0.3)               // 酵母循环次数
  ├─ salt   = (int)((300 × k_salt)  / 0.3)               // 盐循环次数
  │
  └─ fwork(flour, water, 500, grain, 400000)
       │
       ├─ Task1: 面粉加入
       │   └─ esc1 40%油门 + GPIO18(磁铁阀) ON → delay → OFF
       │
       ├─ Task2: 水泵加水
       │   └─ ledc_pwm_set_state(1) → 等待流量计脉冲达标 → ledc_pwm_set_state(0)
       │
       ├─ Task3: 研磨 + 加料
       │   ├─ 酵母循环 ×yeast: servo(145°) → servo2(65°) → 击打 → 归位
       │   ├─ 盐循环 ×salt:   servo(55°)  → servo2(60°) → 击打 → 归位
       │   └─ 研磨电机分时段间歇运转 (60s ON / 30s OFF 循环)
       │
       └─ Task4: 搅拌
           └─ esc2 40%油门 → delay(400000ms) → 熄火
```

**关键输入变量：**

| 变量 | 类型 | 来源 | 含义 |
|------|------|------|------|
| `weight` | uint32_t | 微信小程序/触控屏/HTTP | 面团总重量(g) |
| `k_flour` | float | 配方常量 (120/267.5) | 面粉比例 |
| `k_water` | float | 配方常量 (65/267.5) | 水比例 |
| `k_grain` | float | 配方常量 (80/267.5) | 杂粮比例 |
| `k_yeast` | float | 配方常量 (2/267.5) | 酵母比例 |
| `k_salt` | float | 配方常量 (0.5/267.5) | 盐比例 |

**关键输出变量：**

| 变量 | 类型 | 含义 |
|------|------|------|
| `flour` | int | 面粉电机运行时长(ms) |
| `water` | int | 水泵运行时长(ms) |
| `grain` | int | 研磨电机运行时长(ms) |
| `yeast` | int | 酵母加料循环次数 |
| `salt` | int | 盐加料循环次数 |

**> 建议拍摄：fwork 函数代码截图 + 一次完整工作流程的串口日志截图**

---

#### 模块二：DRV8870 水泵 PWM 驱动（ledc_pwm.c）

该模块将原继电器开/关控制升级为 DRV8870 电机驱动模块的 PWM 调速控制。

**`ledc_pwm_set_state(state)` 流程图：**

```
ledc_pwm_set_state(state)
  │
  ├─ [首次调用] init()
  │    ├─ ledc_timer_config(Timer0, 5kHz, 10bit)
  │    ├─ ledc_channel_config(CH3→GPIO12)   // 水泵 PWM
  │    └─ ledc_channel_config(CH4→GPIO13)   // 辅助 LOW
  │
  ├─ state == 0: duty = 0      (关闭)
  └─ state == 1: duty = 511    (50%占空比)
       │
       ├─ ledc_set_duty(CH3, duty)
       └─ ledc_update_duty(CH3)
```

**LEDC 资源分配：**

| Timer | 频率 | Channel | GPIO | 设备 |
|-------|------|---------|------|------|
| 0 | 5kHz | CH3 | 12 | 水泵 DRV8870 IN1 |
| 0 | 5kHz | CH4 | 13 | 水泵 DRV8870 IN2 |
| 1 | 1kHz | CH1 | 4 | 步进电机 STP |
| 2 | 50Hz | CH0 | 8 | 旋转舵机 |
| 2 | 50Hz | CH2 | 3 | 击打舵机 |
| 3 | 50Hz | CH6 | 15 | 面粉 ESC |
| 3 | 50Hz | CH7 | 9 | 搅拌 ESC |

**> 建议拍摄：示波器捕获 GPIO12 的 50%占空比 PWM 波形截图**

---

#### 模块三：三端互斥控制（ctl_mutex.c）

由于系统支持触控屏、HTTP 网页、小智AI三方控制，必须防止多源同时操作导致硬件冲突。

**`ctl_try_acquire(src)` 流程图：**

```
ctl_try_acquire(CTL_TOUCH)
  │
  ├─ xSemaphoreTake(mutex, 0)     // 非阻塞获取
  │    └─ [失败] return false      // 已被占用
  │
  ├─ [s_owner != NONE && s_owner != src]
  │    └─ 释放锁, return false     // 其他源占用中
  │
  ├─ s_owner = CTL_TOUCH          // 登记占用者
  └─ xSemaphoreGive(mutex), return true
```

**各控制源的互斥接入点：**

```
触控屏: action_star_mixer() → ctl_try_acquire(CTL_TOUCH)
HTTP:   api_start_post_handler() → ctl_try_acquire(CTL_HTTP)
小智AI: handle_mcp_command() → ctl_try_acquire(CTL_XIAOZHI)
```

**> 建议拍摄：三端同时操作的测试截图，展示互斥响应**

---

#### 模块四：小智AI语音控制（xiaozhi 组件）

**架构说明**：用户通过**微信小程序**以文字或语音方式与小智 AI 服务端交互。服务端内部调用 DeepSeek 大语言模型解析用户意图（如“启动和面，300克饺子皮”），通过 MCP（Model Context Protocol）工具调用机制将结构化指令下发给 ESP32 设备端。设备端执行完成后，通过 WebSocket 回传状态，服务端将结果转换为自然语言通过 TTS 合成语音播报，或在微信小程序中展示。

**通信流程：**

```
用户打开微信小程序 "晚餐杂粮助手"
  │
  ├─ 文字输入："中午吃了牛肉面，晚上想包300g饺子"
  │    └─ POST /api/v1/nutrition/intake/parse
  │         └─ 服务端解析饮食条目 → 返回确认清单
  │
  ├─ 用户确认后
  │    └─ POST /api/v1/nutrition/recommendations/coarse-grain
  │         └─ 营养推荐引擎计算配比 → 返回精确克重
  │
  └─ 用户点击"开始和面"
       └─ 服务端通过 WebSocket 下发 MCP 指令至 ESP32
            └─ {"type":"mcp","tool":"start_mixer","args":{"weight":300}}
                 └─ ESP32 执行 fwork(...) 全自动流程
```

**MCP 指令映射表：**

| 用户语音/小程序指令 | MCP tool | 设备端函数 |
|------|------|------|
| "开始和面 300克" | `start_mixer {weight:300}` | `weight_work(300)` |
| "停止" | `stop_mixer` | `fstop()` |
| "推出面团" | `push_out` | `push_and_out(1)` |
| "退回" | `push_back` | `push_and_out(0)` |

**服务端处理流程：**

```
WebSocket 接收用户消息
  ↓
ASR 语音识别 / 小程序文本直传
  ↓
DeepSeek LLM 意图识别 + 营养推荐引擎
  ↓
生成结构化 JSON 参数
  ↓
MCP 工具调用 → 下发至 ESP32
  ↓
ESP32 执行 → WebSocket 回传状态
  ↓
TTS 合成语音 → 返回微信小程序/设备端播报
```

**营养推荐输出示例：**

```json
{
  "dough_total_weight_g": 300,
  "flour_weight_g": 228,
  "water_weight_g": 72,
  "coarse_grain_ratio": 0.35,
  "coarse_grain_weight_g": 80,
  "reason": "今日精制主食摄入偏多，建议提高杂粮比例至35%。"
}
```

**音频 I2S 引脚配置（用于设备端语音反馈）：**

| GPIO | 功能 | 连接 |
|------|------|------|
| 14 | I2S BCLK（共享） | INMP441 SCK + MAX98357A BCLK |
| 16 | I2S WS（共享） | INMP441 WS + MAX98357A LRC |
| 17 | I2S SD（输入） | INMP441 数据 → ESP32 |
| 7 | I2S DIN（输出） | ESP32 → MAX98357A |

**> 建议拍摄：微信小程序操作界面截图 + 小智服务端终端日志 + 设备端收到 MCP 指令后开始工作的视频截图**

---

#### 模块五：流量计闭环水量控制

将传统的定时开/关水泵升级为基于 YF-S201 霍尔流量计的脉冲计数闭环控制，消除电压波动和水压变化对加水精度的影响。

**控制流程对比：**

```
改前（开环定时）：
  开水泵 → vTaskDelay(N ms) → 关水泵
  精度: ±20%（受电压/水压影响极大）

改后（闭环计数）：
  开水泵 → 中断计数脉冲 → count ≥ target → 关水泵
  精度: ±2%（450脉冲/L，1脉冲≈2.22mL）
```

**流量计参数：**

| 参数 | 值 |
|------|-----|
| 型号 | YF-S201 |
| 频率公式 | F(Hz) = 7.5 × Q(L/min) |
| 脉冲/升 | 450 |
| 单脉冲水量 | 2.22mL |
| GPIO | 37（中断输入） |

**> 建议拍摄：流量计模块特写 + 串口脉冲计数日志截图**

---

### 系统性能参数

| 指标 | 数值 |
|------|------|
| 主控芯片 | ESP32-S3 (Xtensa LX7, 240MHz) |
| Flash | 16MB (DIO模式) |
| 固件大小 | 2.7MB |
| 内存占用 | 512KB SRAM (LVGL + WiFi + 音频管线) |
| 操作系统 | FreeRTOS (多任务) |
| 显示屏 | ST7789V 2.4寸 SPI TFT (240×320) |
| 触摸芯片 | FT6336U (I2C) |
| 无线通信 | WiFi AP (802.11n) + TCP/HTTP/WebSocket |
| LEDC资源 | 8通道/4定时器全满 |
| 水泵频率 | 5kHz PWM (DRV8870) |
| 舵机频率 | 50Hz PWM |
| ESC频率 | 50Hz PWM |
| 音频采样率 | 16kHz (I2S 全双工) |
| 音频编码 | Opus |
| 唤醒词 | 你好小智 (ESP-SR MultiNet) |
| 服务器模型 | DeepSeek / ChatGLM |
| 功耗 | 约 1.5W (待机) ~ 5W (全功率工作) |

---

### 软件技术栈

| 层次 | 技术 |
|------|------|
| 设备固件 | C/C++, ESP-IDF v6.0, FreeRTOS, LVGL v9, EEZ Studio |
| 音频管线 | I2S 全双工, Opus, ESP-SR (MultiNet唤醒词) |
| 设备通信 | WebSocket (WSS), JSON/MCP 协议 |
| 智能服务端 | Python, ASR, DeepSeek LLM, TTS, SQLite |
| 微信小程序 | uni-app, Vue.js, TypeScript |
| 网页控制 | HTML/JS (嵌入固件), HTTP REST API |
| 构建系统 | CMake + Ninja + ESP-IDF 组件管理器 |

---

### 创新点总结

1. **三端统一互斥控制**：触控屏、网页、AI语音三种交互方式共享同一硬件资源，通过自研 ctl_mutex 组件实现非阻塞互斥调度，避免多源并发冲突。

2. **AI驱动配方计算**：基于大语言模型和营养推荐引擎，根据用户当日饮食自动计算面团配比，将健康饮食理念嵌入厨房自动化。

3. **闭环水量控制**：用霍尔流量计替代定时盲开，将加水精度从 ±20% 提升至 ±2%。

4. **DRV8870 PWM 调速**：使用 MCPWM/LEDC 硬件 PWM 替代继电器，支持电机调速，减少电气噪声和机械冲击。

5. **模块化组件设计**：15 个独立 ESP-IDF 组件，功能解耦，可复用、可扩展。

6. **云端智能 + 边缘执行**：AI 推理部署在服务端（可本地/云端），设备端专注实时控制，架构清晰，维护成本低。

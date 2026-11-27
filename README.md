# ZHIoT_Mario_Game — STM32L431 超级马里奥使用文档

基于 **STM32L431VC**（PCBA-20019 主板 + 320×240 TFT LCD）移植并深度优化的横版马里奥游戏，支持 **60 FPS 级 DMA 双缓冲刷屏**、**中文开场语音（“嘿，我是马里奥”）与音效**、以及 **电脑键盘零延迟控制 + PC 实时大屏投屏**。

---

## 1. 功能特性

- **双关卡完整体验**：
  - `1-1` 白天关卡 & `1-2` 夜晚关卡，含问号砖顶金币、碎砖块、管道、悬崖陷阱、栗子怪踩踏与终点旗杆通关。
  - **可调跳跃高度**：短按 `KEY3` / `W` / `空格` 小跳，长按高跳。
- **Wi-Fi 2~4 人同图联机（板载 ESP8285 / SoftAP 热点直连）**：
  - 全开发板烧录**同一份通用固件**，无需路由器：房主按 `KEY1` 自建 Wi-Fi 热点 **`MARIO-MP`**（密码 `12345678`，主机 `192.168.4.1`，UDP 端口 `9000`），其余 1~3 台开发板按 `KEY2` 自动连入同一房间。
  - **4 色区分角色**：`1P` 红色经典马里奥（Host）、`2P` 绿色路易吉、`3P` 金色马里奥、`4P` 青色马里奥；同屏显示队友实时跑跳位置，顶碎砖块/吃金币/击杀怪物/摸旗过关状态通过 UDP 事件同步。
  - 支持 PC 客户端脚本 [pc_mp_client.py](pc_mp_client.py) 接入 `MARIO-MP` 作为 `2P~4P` 游玩，即使手边只有一块开发板也能直接体验双人/多人联机。
- **高帧率 DMA 显示优化（~58 FPS）**：
  - `SPI1`（`40 MHz`）配合 `DMA1_Channel3`（`16-bit` 突发传输）与双行缓冲 `strip_buf[2][320×16]` 流水线渲染，并通过 64-bit 条带哈希跳过静态不变行。
  - 顶部 HUD 实时显示：`关卡-小关`、`分数`、`金币`、`生命`、`实时 FPS`、`音效状态 (SND/MUT)`。
- **双通道语音与音效**：
  - **LD3320 MP3 语音/音效**（`SPI3`）：开机进入游戏自动播放中文语音 **“嘿，我是马里奥”**，并支持跳跃、吃金币/顶问号、踩怪/碎砖、过关、阵亡 5 种 MPEG-1（44.1 kHz / 48 kbps）音效。
  - **板载 `PE8` 蜂鸣器联动**：同时提供短促清脆的按键/事件提示音（在 `LD3320` 在线时最长 `18 ms`，不抢声）；未连接扩展板喇叭也能听到清晰音效反馈。
- **PC 键盘控制 + 实时 2× 同步投屏**：
  - 通过单根 **ST-LINK** USB 线即可使用 [pc_keyboard_ctrl.py](pc_keyboard_ctrl.py) 弹出 `640×480` 实时画面窗口，并用电脑键盘顺滑操控（实测 SWD 后台读写延迟 `< 7 ms/帧`，不暂停 MCU 运行）。

---

## 2. 硬件连接与注意事项

| 模块 / 接口 | 位置与引脚 | 说明 |
| :--- | :--- | :--- |
| **ST-LINK 调试与投屏线** | 主板上方 `STLINK DEBUG` Type-C 口 | 用于编译烧录、PC 键盘控制与实时投屏（仅需插这一根 USB 线） |
| **主板电源** | 12V DC 电源口（或 USB 供电） | 连接扩展板与喇叭外放时建议打开实验箱 12V 主电源 |
| **板载实体按键** | 主板左下角 `KEY1` ~ `KEY4` (`PD5..PD2`) | 随时可与 PC 键盘同时使用 |
| **生命指示灯** | `LED1` ~ `LED3` (`PA4..PA6`) | 亮灯数量表示当前剩余生命（1~3 条命）；`LED4` 吃金币/踩怪闪烁 |
| **板载蜂鸣器** | `BEEP` (`PE8`) | 无需接线即可发声 |
| **面板外放喇叭（可选）** | `20020（智慧家居）JP2` ↔ `20024（传感器）JP8` | 需用 **34P 牛角扣排线**连接，才可由面板 `P4 喇叭` 播放 `"嘿，我是马里奥"` 与 MP3 音效 |

> **安全提示**：主板下方 `J1 (DEBUG)` Type-C 口的 `VBUS` 与 `12V→5V` 电源直接连通且无防倒灌隔离，日常游玩、烧录与 PC 投屏 **只需保留上方 `STLINK DEBUG` 的一根数据线**，无需再往 `J1` 插第二根 USB 线，以免触发电脑 USB 口干扰或倒灌。

---

## 3. 操作说明

### 方式一：PC 键盘控制 + 实时投屏（推荐）

确保开发板已连接 **ST-LINK** 线，在电脑桌面终端运行：

```bash
python3 /home/luo/Desktop/ZHIoT_Mario_Game/pc_keyboard_ctrl.py
```

运行后会打开 **`Super Mario STM32 - PC Mirror + Controller`**（`640×480` 2 倍像素画面），点击聚焦该窗口即可游玩：

| 按键 | 功能 |
| :--- | :--- |
| `A` / `←` / `1` | 向左移动 |
| `D` / `→` / `2` | 向右移动 |
| `W` / `空格` / `↑` / `回车` / `3` | 开始游戏 / 跳跃（长按跳得更高）/ 过关或失败后重开 |
| `M` / `4` | 切换静音 (`SND` / `MUT`) |
| `Esc` / `Q` | 退出 PC 投屏控制窗口 |

可选启动模式：
- `python3 pc_keyboard_ctrl.py --cli`：纯终端键盘模式（无 GUI 弹窗，按 `S` 可急停）。
- `python3 pc_keyboard_ctrl.py /dev/ttyUSB0`：显式指定 `CH340 UART2 (115200)` 串口作为键盘输入通道。

> **注意**：`st-flash` 或 `OpenOCD` 同一时间只能有一个进程占用 ST-LINK。需要重新烧录固件前，请先按 `Esc`/`Q` 关闭 `pc_keyboard_ctrl.py`。

### 方式二：板载实体按键控制

#### 标题画面选择模式：
| 开发板按键 | 对应键盘 | 功能 |
| :--- | :--- | :--- |
| `KEY3` | `3` / `W` / `空格` | 单人游戏 (`SOLO`) |
| `KEY1` | `1` / `A` / `←` | 房主建房 (`HOST`，开启 `MARIO-MP` 热点并进入大厅) |
| `KEY2` | `2` / `D` / `→` | 加入房间 (`JOIN`，自动搜索并连接 `MARIO-MP`) |
| `KEY4` | `4` / `M` | 切换静音 (`SND` / `MUT`) |

#### 联机大厅 (`WIFI LOBBY`) & 游戏中按键：
| 开发板按键 | 对应功能 |
| :--- | :--- |
| `KEY1` | 向左移动 |
| `KEY2` | 向右移动 |
| `KEY3` | 大厅中房主按此**开始游戏**；游戏中**跳跃**；阵亡或通关后**重开** |
| `KEY4` | 大厅中按此**取消返回标题**；游戏中切换静音 (`SND` / `MUT`) |

### 方式三：Wi-Fi 2~4 人多人联机步骤

1. **开发板 ↔ 开发板联机（推荐 2~4 块 ZHIoT 实验板）**：
   - 所有开发板烧录本工程同一份固件，保证核心板上的 **ESP8285 Wi-Fi 模块** 就绪（`USART1: PA9/PA10 @ 115200`）。
   - **1P 房主**：在标题画面按 **`KEY1`**，屏幕进入 `WIFI LOBBY [MARIO-MP]`，当进度变为 **`HOST READY`** 即成功开启热点 `MARIO-MP`。
   - **2P~4P 玩家**：在其余开发板标题画面按 **`KEY2`**，等待 `JOIN MARIO-MP...` → **`CONNECTED!`**，大厅将依次点亮 `1P:OK 2P:OK 3P:OK 4P:OK`。
   - **发车**：1P 房主按 **`KEY3`**，全部玩家同步进入 `1-1` 开始合作闯关。
2. **开发板 ↔ PC 客户端联机（仅 1 块开发板即可测双人）**：
   - 开发板在标题画面按 **`KEY1`**（或电脑控制端按 `1`）开出 `MARIO-MP` 热点。
   - 电脑 Wi-Fi 连接 **`MARIO-MP`**（密码 `12345678`），运行：
     ```bash
     python3 /home/luo/Desktop/ZHIoT_Mario_Game/pc_mp_client.py
     ```
   - 房主大厅出现 `2P:OK` 后按 **`KEY3`** 开始，PC 窗口即可用 `WASD`/方向键+空格操控 `2P` 绿色路易吉与 `1P` 同屏游玩。

---

## 4. 编译、烧录与原厂固件恢复

在 `/home/luo/Desktop/ZHIoT_Mario_Game` 目录下执行：

```bash
# 清理并全量编译（生成 build/ZHIoT_Mario_Game.elf / .hex / .bin）
make clean && make -j$(nproc)

# 通过 ST-LINK 烧录并自动复位运行
make flash
```

如需恢复实验箱原始出厂演示固件（已提前完整备份为 `256 KB` 二进制文件）：

```bash
st-flash --reset write /home/luo/Desktop/ZHIoT_Sensor_Diffuse_Reflectance/backup_smart_home_factory.bin 0x08000000
```

---

## 5. 目录结构

```text
ZHIoT_Mario_Game/
├── README.md                 # 本使用文档
├── Makefile                  # ARM GCC 编译与 st-flash 烧录配置
├── compile_commands.json     # clangd 代码索引
├── pc_keyboard_ctrl.py       # PC 键盘控制 + 640x480 实时投屏脚本（支持单人/联机投屏，零第三方依赖）
├── pc_mp_client.py           # PC Wi-Fi 联机客户端（连接 MARIO-MP UDP:9000 作为 2P~4P 游玩）
├── Inc/
│   ├── mario_game.h          # 游戏主接口
│   ├── mario_sprites.h       # RGB565 调色板（含 1P~4P 配色）与 16x16 像素点阵
│   ├── mario_audio.h         # 内嵌 MPEG-1 MP3 语音与音效数组
│   ├── ld3320_audio.h        # LD3320 SPI3 音频驱动接口
│   └── wifi_net.h            # ESP8285 Wi-Fi 2~4 人 SoftAP/STA UDP 协议接口
├── Src/
│   ├── main.c                # 时钟（80 MHz）、UART2、GPIO、LCD 初始化与主循环
│   ├── mario_game.c          # 游戏物理、关卡地图、双缓冲 DMA 调度、联机事件同步与状态镜像
│   ├── wifi_net.c            # 非阻塞 ESP8285 AT 状态机与 UDP 房间/帧同步驱动
│   ├── ld3320_audio.c        # LD3320 24 MHz PLL、FIFO 水位与非阻塞 MP3 喂流驱动
│   ├── stm32l4xx_it.c        # DMA1_Channel3 与 USART1_IRQHandler 中断处理
│   ├── gpio.c / spi.c / usart.c
└── Hardware/LCD/
    ├── lcd.h / lcd.c         # ST7789/ILI9341 SPI1 + 16-bit DMA 异步刷屏驱动
```

---

## 6. 常见问题排查

1. **运行 `pc_keyboard_ctrl.py` 提示 `OpenOCD failed to start` 或找不到 ST-LINK**：
   - 检查是否已有其它 `openocd` 进程占用了调试器（可执行 `pkill openocd` 后重试），以及 USB 线是否插在主板上方 `STLINK DEBUG` 口。
2. **板载蜂鸣器有声音，但面板 `P4 喇叭` 不响**：
   - 确认实验箱 `12V` 主电源已打开，且 `20020（智慧家居）JP2` 与 `20024（传感器）JP8` 之间的 **34P 牛角扣排线** 已插牢；若同时插着 `20021（智慧交通）`，注意其 `JP9` 与 `JP8` 在 `20024` 板上并联 `SPOP/SPON`，可尝试单独连接 `20020` 排查。
3. **不想听到任何声音**：
   - 游戏中直接按键盘 **`M`** 或主板 **`KEY4`**，右上角 HUD 从 `SND` 变为 `MUT` 即可完全静音。

# Cus_Flash — STM32 片上 Flash 存储库

面向 STM32F1 / F4 全系列的轻量级 Flash 读写库，提供三层抽象：**底层硬件驱动 → Page/Sector 操作 → Manager 记录管理**。支持 RTOS多线程环境、PVD 欠压检测、以及掉电安全的记录提交机制。

------

## 它适合谁？

- **学生课程相关设计**：快速进行不掉电参数存储，并且存储可视化，易于上层逻辑编写。
- **设备特征参数存储**：校准值、序列号、传感器偏移、PID 系数等掉电不丢失的关键参数，Manager 模式一行 Append 搞定。
- **嵌入式配置管理**：Wi-Fi 凭证、设备别名、运行模式等多条配置记录，按 Desc 描述符增删改查，支持多实例分区。
- **掉电安全的运行时数据**：配合 validFlag 后写策略 + PVD 欠压检测，适合需要在写入中途容忍断电的场景。
- **Bootloader 参数传递**：App 与 Bootloader 之间通过约定 Flash 区域交换固件版本、启动标志等。

---

## 最佳工程实践建议

由于 Flash 按扇区/页整块擦除，如果将 Manager 区域直接放在 APP 固件末尾，固件每次增长都可能改变配置区起始地址，烧录时极易误擦配置数据。因此强烈建议采用 **Bootloader + Manager + APP** 的三区布局，将配置区域物理隔离。

**推荐内存布局：**

```
┌────────────── 0x0800 0000 ──────────────┐
│  Bootloader    ( 8 KB )                 │
├────────────── 0x0800 2000 ──────────────┤
│  Manager Region ( 8 KB, 4 pages )       │  ← 库独占，不参与 APP 编译
├────────────── 0x0800 4000 ──────────────┤
│  APP           (剩余空间)               │
└────────────── Flash End ────────────────┘
```

> 以 STM32F103C8T6（64 KB，每页 2 KB）为例：Bootloader 占 0x08000000--0x08001FFF（4 页），Manager 区域占 0x08002000--0x08003FFF（4 页），APP 从 0x08004000 开始。即使 APP 膨胀到 40+ KB，Manager 区域的物理地址也不会变化。

**实施要点：**
- **Scatter 文件 / 链接脚本**：APP 的链接地址设为 Manager 区域之后（如 0x08004000），Manager 区域不参与编译链接，由库在运行时通过 `Cus_FlashMgr_Init` 接管
- **烧录配置**：Keil 中将 APP 的烧录范围限定在 APP 区域，避免每次下载擦除 Manager 区域
- **多 Target 支持**（参考本模板）：为不同 MCU 创建独立的 sct 文件和 Target，一键切换

**垃圾回收策略：**
Manager 采用 Append-Only 写入，逻辑删除仅标记 validFlag 而不释放物理空间。当区域写满时：
1. 调用 `Cus_FlashMgr_DumpAll` 导出全部有效记录到 RAM
2. 调用 `Cus_FlashMgr_EraseRegion` 擦除整区
3. 逐条 `Cus_FlashMgr_Append` 回写

此过程需保证不掉电（或配合 PVD 在回写完成前阻止系统复位）。

------

## 目录

- [1. 架构总览](#1-架构总览)
- [2. 快速开始](#2-快速开始)
  - [2.1 配置头文件](#21-配置头文件)
  - [2.2 裸写模式（不使用 Manager）](#22-裸写模式不使用-manager)
  - [2.3 Manager 模式（推荐）](#23-manager-模式推荐)
- [3. 掉电保护设计](#3-掉电保护设计)
  - [3.1 validFlag 后写策略](#31-validflag-后写策略)
  - [3.2 PVD 辅助](#32-pvd-辅助)
- [4. on-flash 数据布局](#4-on-flash-数据布局)
  - [4.1 记录格式](#41-记录格式)
  - [4.2 区域内存布局](#42-区域内存布局)
- [5. API 参考](#5-api-参考)
  - [5.1 底层 API](#51-底层-api)
  - [5.2 Page/Sector API](#52-pagesector-api)
  - [5.3 Manager API](#53-manager-api)
- [6. FreeRTOS 集成](#6-freertos-集成)
- [7. Hook 回调](#7-hook-回调)
- [8. 移植指南](#8-移植指南)
- [9. 限制与注意事项](#9-限制与注意事项)
- [10. 错误码](#10-错误码)

---

## 1. 架构总览

```
┌─────────────────────────────────────────────┐
│  Manager 层  (CUS_FLASH_USE_MANAGER)        │
│  Init / Append / Delete / Query / Dump      │
│  基于 desc_t 头部 + RAM 索引表              │
├─────────────────────────────────────────────┤
│  Page / Sector 层                           │
│  ErasePage / WritePage / ReadPage   (F1)    │
│  EraseSector / WriteSector / ReadSector(F4) │
├─────────────────────────────────────────────┤
│  底层驱动                                    │
│  Unlock / Lock / WriteBuffer / VerifyBuffer │
│  WaitBusy / CalibrateLatency / PVD          │
└─────────────────────────────────────────────┘
```

| 层级 | 职责 | 典型场景 |
|------|------|---------|
| 底层 | 硬件抽象：解锁序列、半字编程、状态轮询、校验 | 所有场景的基础 |
| Page/Sector | 按物理单元（页/扇区）擦写读，处理非对齐尾部 | 需要精确控制物理地址时 |
| Manager | 记录级 CRUD、自动空间分配、逻辑删除 | 配置/日志等键值存储 |

---

## 2. 快速开始

### 2.1 配置头文件

在 `Cus_Flash.h` 中修改以下宏：

```c
// 芯片系列型号（二选一，另一个设为 0）
#define DEVICE_STM32F1xx   (1)
#define DEVICE_STM32F4xx   (0)

// F1: 每页字节数（必须按实际 Flash 规格修改）
#define CUS_FLASH_BYTE_PER_PAGE  (2048UL)

// F4: Flash 总容量（必须按实际规格修改）
#define DEVICE_FLASH_TOTAL_SIZE  (256UL * 1024UL)

// 库时基基准，可由外界提供（如 HAL_GetTick / xTaskGetTickCount）。
// 若留空不定义，则需在初始化时调用 Cus_Flash_SYS_TickInit()，库内部使用 DWT 计时。
// #define CUS_FLASH_GET_TICK()  HAL_GetTick()

// Manager 开关（如需使用库提供的简易管理器功能，请置1）
// Ps: 置1过后，底层写入原语除WriteBuffer()外，均被屏蔽，只允许使用库提供的追加API.
#define CUS_FLASH_USE_MANAGER    (1)

// Manager 参数
#define FLASH_MGR_MAX_RECORDS    (64)   // RAM 索引表最大条目数
#define FLASH_MGR_MAX_INSTANCES  (4)    // 最大同时活跃的 Manager 实例数

// RTOS 开关 (是否启用多线程支持)
#define CUS_FLASH_USE_SYS        (1)    
```

### 2.2 裸写模式（不使用 Manager）

适用于需要完全控制物理地址的场景：

```c
#include "Cus_Flash.h"

// 校准 Flash 等待周期 + 初始化时基（初始化时调用一次）
Cus_Flash_CalibrateLatency();
Cus_Flash_SYS_TickInit();        // 若未自定义 CUS_FLASH_GET_TICK，必须调用

uint8_t buf[256] = { /* ... */ };
uint32_t page_addr = 0x0800FC00;  // 最后一页起始地址

Cus_Flash_PageReq_t req = {
    .pBuffer          = buf,
    .bufSize          = sizeof(buf),
    .Offset           = 0,
    .PageStartAddress = page_addr,
};

Cus_Flash_ErasePage(page_addr);
Cus_Flash_WritePage(&req);
Cus_Flash_ReadPage(&req);         // 回读到 buf
```

### 2.3 Manager 模式（推荐）

由 Manager 自动管理空间分配和记录索引：

```c
#include "Cus_Flash.h"

FlashMgr_Instance_t cfg_mgr;

void storage_init(void)
{
    Cus_Flash_CalibrateLatency();
    Cus_Flash_SYS_TickInit();

    // 初始化 Manager 实例，划定 Manager 管理的 Flash 区域
    Cus_FlashMgr_Init(&cfg_mgr, 0x0800FC00, 0x08010000);

    // （可选）PVD 掉电检测
    Cus_Flash_PVDConfig(CUS_FLASH_PVD_LEVEL4);
}

// 写入
void save_config(const char *key, uint8_t *data, uint32_t len)
{
    Cus_FlashMgr_Req_t req = {
        .DataType  = 1,
        .DataBuff  = data,
        .DataSize  = len,
    };
    strncpy(req.DataDesc, key, sizeof(req.DataDesc));
    Cus_FlashMgr_Append(&cfg_mgr, &req);
}

// 读取（取最新的一条匹配记录）
Cus_Flash_desc_t desc;
if (Cus_FlashMgr_GetRecordByDesc(&cfg_mgr, "wifi_cfg", &desc) == CUS_FLASH_OK) {
    // desc.dataStartAddr 指向用户数据
    // desc.dataSize        为用户数据大小
}

// 删除.(删除最新一条记录)
Cus_FlashMgr_DeleteByDesc(&cfg_mgr, "wifi_cfg");
```

---

## 3. 掉电保护设计

### 3.1 validFlag 后写策略

利用 NOR Flash **只能将 bit 从 1 编程为 0** 的物理特性，设计三态标记：

| validFlag 值 | 状态 | 说明 |
|-------------|------|------|
| `0xFFFF` | 空闲 / 未提交 | Flash 擦除后的默认态。不参与扫描匹配。 |
| `0xF0F0` | 已提交（有效） | 记录完整写入，可以安全读取。 |
| `0xF0E0` | 已删除 | 在 `0xF0F0` 基础上将 bit4 从 1→0，不可逆。 |

**状态迁移路径（全部合法，仅涉及 1→0）：**

```
0xFFFF ──(编程)──▶ 0xF0F0 ──(编程bit4)──▶ 0xF0E0
 擦除态            已提交              已删除
```

**写入顺序（Append）：**

```
1. WriteBuffer(addr,      &desc, 30)    // 写 header 前 30 字节（Magic/Size/DataAddr/Type/Desc）
                                         // validFlag 位置保持 0xFFFF（未提交）
2. WriteBuffer(addr + 32, userData, n)   // 写用户数据
3. WriteBuffer(addr + 30, &commit, 2)   // 写 validFlag = 0xF0F0（提交）
```

**安全性分析：**

- 步骤 1 或 2 期间掉电 → validFlag 仍为 `0xFFFF` → 扫描忽略 → **安全**
- 步骤 3 是 2 字节单次半字写入（原子），要么完成要么未完成 → **安全**
- 全部完成后掉电 → 记录完整且已提交 → **安全**

### 3.2 PVD 辅助

库内置了 STM32 的 PVD（Programmable Voltage Detector）支持，作为掉电保护的辅助手段：

```c
// 初始化
Cus_Flash_PVDConfig(CUS_FLASH_PVD_LEVEL4);  // 约 2.9V 阈值

// 在 PVD IRQ Handler 中调用
void PVD_IRQHandler(void)
{
    Cus_Flash_PVDSet();  // 置位 gs_lowVoltage 标记
    // ... 清理外设、关闭正在进行的 Flash 操作 ...
}
```

Manager 的写入流程可在关键步骤前检查 `gs_lowVoltage`（通过自行扩展），在电压过低时提前中止写入。

---

## 4. on-flash 数据布局

### 4.1 记录格式

每一条 Manager 记录在 Flash 上的存储结构：

```
┌───────────────────────────────────────────────┐
│  desc_t 头部 (32 bytes)                       │
│  ┌──────────┬──────────┬────────────────────┐ │
│  │ Magic    │ Size     │ dataStartAddr      │ │
│  │ (4B)     │ (4B)     │ (4B)               │ │
│  ├──────────┼──────────┼──────────┬─────────┤ │
│  │ Type     │validFlag │ Desc[16]           │ │
│  │ (2B)     │ (2B)     │                    │ │
│  └──────────┴──────────┴────────────────────┘ │
├───────────────────────────────────────────────┤
│  用户数据 (Size 字节)                          │
│  ┌──────────────────────────────────────────┐ │
│  │ ...                                      │ │
│  └──────────────────────────────────────────┘ │
├───────────────────────────────────────────────┤
│  下一记录（addr 按 4 字节对齐）                 │
└───────────────────────────────────────────────┘
```

| 字段 | 偏移 | 大小 | 说明 |
|------|------|------|------|
| Magic | 0 | 4 | `0xBEEFFEEB`，识别有效头部 |
| Size | 4 | 4 | 后续用户数据字节数 |
| dataStartAddr | 8 | 4 | 用户数据的绝对 Flash 地址 |
| Type | 12 | 2 | 应用自定义数据类型标签 |
| validFlag | 14 | 2 | `0xFFFF`=未提交, `0xF0F0`=有效, `0xF0E0`=已删除 |
| Desc | 16 | 16 | 人可读描述字符串（null 填充） |

### 4.2 区域内存布局

```
[start_addr]
  ├─ Record 0  ──┤  (32 + Size0) bytes
  ├─ Record 1  ──┤  (32 + Size1) bytes
  ├─ Record 2  ──┤  (32 + Size2) bytes
  ├─ ...         ──┤
  ├─ Free Space  ──┤  全 0xFF
[end_addr]
```

Init 时从头到尾扫描：遇 Magic 匹配 + validFlag 有效则加入 RAM 索引表；遇连续 `0xFFFFFFFF` 区域则标记为最低空闲地址。

---

## 5. API 参考

### 5.1 底层 API

| 函数 | 返回值 | 说明 |
|:-----|--------|------|
| `Cus_Flash_CalibrateLatency()` | `bool` | 根据 SystemCoreClock 自动配置 Flash 等待周期。初始化时必须调用。 |
| `Cus_Flash_Unlock()` | — | Flash 解锁（KEYR 序列），已解锁时直接返回 |
| `Cus_Flash_Lock()` | — | Flash 上锁，等待 BSY 清零后置位 LOCK |
| `Cus_Flash_WriteBuffer(addr, pData, size)` | `Cus_Flash_State_t` | 半字写入 + 自动回读校验。自动处理奇数长度 |
| `Cus_Flash_VerifyBuffer(addr, pData, size)` | `bool` | 独立校验：4 字节对齐时走字比对，否则逐字节比对 |
| `Cus_Flash_IsValid(addr, size)` | `bool` | 检查指定区域是否全 `0xFF`（已擦除、可写入） |
| `Cus_Flash_PVDConfig(level)` | `Cus_Flash_State_t` | 配置 PVD 检测电平并使能 NVIC |
| `Cus_Flash_PVDSet()` | — | 置位 `gs_lowVoltage`（在 PVD ISR 中调用） |
| `Cus_Flash_PVDClr()` | — | 清除 `gs_lowVoltage` |
| `Cus_Flash_SYS_TickInit()` | — | 初始化 DWT 周期计数器（默认时基，未自定义 `CUS_FLASH_GET_TICK` 时必须调用） |
| `Cus_Flash_SYS_GetTick()` | `uint32_t` | 默认实现的 DWT 毫秒 tick，由 `CUS_FLASH_GET_TICK` 宏内部调用 |

### 5.2 Page/Sector API

**F1（页擦除模式）：**

| 函数 | 说明 |
|------|------|
| `Cus_Flash_ErasePage(PageAddress)` | 擦除单页，超时 3s |
| `Cus_Flash_ErasePages(StartAddress, Count)` | 批量擦除，返回成功数 |
| `Cus_Flash_WritePage(pPage)` | 写一页，含擦除前自动检查 |
| `Cus_Flash_ReadPage(pPage)` | 读一页到用户缓冲区 |
| `Cus_Flash_GetPageAddress(index)` | 页号 → 起始地址 |
| `Cus_Flash_GetPageStart(Address)` | 任意地址 → 所在页起始地址 |
| `Cus_Flash_GetPageIndex(Address)` | 任意地址 → 所在页号 |
| `Cus_Flash_GetTotalPages()` | 返回 Flash 总页数 |
| `Cus_Flash_GetRemainPages(PageAddress)` | 返回剩余页数 |

**F4（扇区擦除模式）：**

| 函数 | 说明 |
|------|------|
| `Cus_Flash_EraseSector(pSector)` | 擦除单扇区 |
| `Cus_Flash_EraseSectors(pSector, num)` | 批量擦除，返回成功数 |
| `Cus_Flash_WriteSector(pRequest)` | 写扇区，含非对齐尾部处理 |
| `Cus_Flash_ReadSector(pReq)` | 读扇区到用户缓冲区 |
| `Cus_Flash_EnableART()` | 使能 ART 加速器（F4） |
| `Cus_Flash_GetSectorbyAddr(Addr)` | 地址 → 扇区描述符 |
| `Cus_Flash_GetSectorbyIndex(Index)` | 索引 → 扇区描述符 |
| `Cus_Flash_GetTotalSectors()` | 返回扇区总数 |
| `Cus_Flash_GetRemainSectors(Index)` | 返回剩余扇区数 |
| `Cus_Flash_GetSectorSize(Index)` | 返回指定扇区大小 |

### 5.3 Manager API

| 函数 | 返回值 | 说明 |
|------|--------|------|
| `Cus_FlashMgr_Init(instance, start, end)` | `Cus_Flash_State_t` | 初始化实例，扫描 Flash 区域重建 RAM 索引 |
| `Cus_FlashMgr_Append(instance, pReq)` | `Cus_Flash_State_t` | 追加一条记录（自动定位空闲区） |
| `Cus_FlashMgr_GetRecordByDesc(instance, desc, pOut)` | `Cus_Flash_State_t` | 按描述符查询，返回最新匹配 |
| `Cus_FlashMgr_DeleteByIndex(instance, index)` | `Cus_Flash_State_t` | 按索引逻辑删除（validFlag → 0xF0E0） |
| `Cus_FlashMgr_DeleteByDesc(instance, desc)` | `Cus_Flash_State_t` | 按描述符逻辑删除（最新匹配） |
| `Cus_FlashMgr_DeleteAllByDesc(instance, desc, pCnt)` | `Cus_Flash_State_t` | 批量逻辑删除所有匹配记录 |
| `Cus_FlashMgr_EraseRegion(instance)` | `Cus_Flash_State_t` | 物理擦除整个 Manager 区域（清空所有数据） |
| `Cus_FlashMgr_GetRecordCount(instance, pOut)` | `Cus_Flash_State_t` | 获取当前有效记录数 |
| `Cus_FlashMgr_GetFreeSpace(instance, pOut)` | `Cus_Flash_State_t` | 获取剩余空闲字节数 |
| `Cus_FlashMgr_DumpByDesc(instance, desc, cb)` | `Cus_Flash_State_t` | 遍历打印所有匹配记录（调试用） |
| `Cus_FlashMgr_DumpAll(instance, cb)` | `Cus_Flash_State_t` | 打印该Manager实例所有记录（调试用） |
| `Cus_FlashMgr_DeInit(instance)` | `Cus_Flash_State_t` | 反初始化，从全局活跃列表移除 |

---

## 6. RTOS 集成

启用 `CUS_FLASH_USE_SYS` 后，库通过 `Cus_Flash_RTOS_Port.h` 接入 RTOS（库Port模板中默认使用FreeRTOS进行移植，若使用其余RTOS，请自行替换其头文件以及对应的临界区定义）。

```c
// Cus_Flash_RTOS_Port.h 示例 (FreeRTOS)
#define CUS_FLASH_SYS_ON

#include "FreeRTOS.h"
#include "task.h"

#define CUS_FLASH_ENTER_CRITICAL()   taskENTER_CRITICAL()
#define CUS_FLASH_EXIT_CRITICAL()    taskEXIT_CRITICAL()

// 用户需实现互斥锁（弱定义默认返回 true）
bool Cus_Flash_SYS_Lock(void);
void Cus_Flash_SYS_Unlock(void);
```

**互斥锁实现示例：**

```c
static SemaphoreHandle_t flash_mutex;

bool Cus_Flash_SYS_Lock(void)
{
    return (xSemaphoreTake(flash_mutex, pdMS_TO_TICKS(5000)) == pdTRUE);
}

void Cus_Flash_SYS_Unlock(void)
{
    xSemaphoreGive(flash_mutex);
}
```

裸机使用时关闭 `CUS_FLASH_USE_SYS`，临界区宏退化为 `__nop()`，互斥锁直接返回 `true`。

---

## 7. Hook 回调

所有 Hook 均为 `__weak`，用户按需重写：

| Hook | 触发条件 |
|------|---------|
| `Cus_FLASH_UnlockFailed_Hook()` | 解锁超时或 KEYR 回读不匹配 |
| `Cus_FLASH_LockFailed_Hook()` | 上锁后 LOCK 位未置位 |
| `Cus_FLASH_EraseFailed_Hook()` | 擦除操作超时 |
| `Cus_FLASH_VerifyBufferFailed_Hook(addr, pData, size)` | 写入后校验不匹配 |
| `Cus_FLASH_PageWriteFailed_Hook(pPage)` | F1 页写入失败 |
| `Cus_FLASH_WriteSectorFailed_Hook(pSector)` | F4 扇区写入失败 |
| `Cus_FLASH_EraseSectorFailed_Hook(pSector)` | F4 扇区擦除失败 |
| `Cus_FLASH_ARTEnableFailed_Hook(ACR)` | F4 ART 加速器使能失败 |
| `Cus_FLASH_MGRBufOVFL_Hook(totalRecords)` | Manager 扫描时记录数超出 FLASH_MGR_MAX_RECORDS |

---

## 8. 移植指南

**新 MCU 型号：**
1. 在 `Cus_Flash.h` 中添加 `DEVICE_STM32xxx` 宏
2. 定义 `CUS_FLASH_BYTE_PER_PAGE`（F1 类）或 `DEVICE_FLASH_TOTAL_SIZE`（F4 类）
3. 补充 `SYSTEMCLOCK_xxxMhz` 时钟档位
4. 在 `Cus_Flash_CalibrateLatency()` 中添加对应时钟段的 LATENCY 配置

**新 RTOS：**
1. 修改 `Cus_Flash_RTOS_Port.h` 中的 `CUS_FLASH_ENTER_CRITICAL/EXIT_CRITICAL` 宏
2. 实现 `Cus_Flash_SYS_Lock/Unlock`（信号量或互斥锁）

---

## 9. 限制与注意事项

- **F1 写入前需擦除整页**（2048 字节），不支持页内局部覆盖。Manager 模式通过 Append 追加新记录、Delete 逻辑删除来规避此限制
- **建议 Manager 区域独占若干完整页/扇区**，与程序固件区物理隔离
- **逻辑删除不回收 Flash 空间**，需定期调用 `Cus_FlashMgr_EraseRegion` 或外部触发 GC
- **validFlag 利用 NOR Flash 的 1→0 不可逆特性**，删除后无法恢复
- **Init 扫描是 O(n)**，大区域可能耗时较长。可通过减少 `FLASH_MGR_MAX_RECORDS` 限制索引规模
- **PVD 中断优先级应设最高**（当前代码设为 0），确保掉电时最先响应

---

## 10. 错误码

| 错误码 | 含义 |
|--------|------|
| `CUS_FLASH_OK` | 操作成功 |
| `CUS_FLASH_TIMEOUT` | Flash 操作超时（BSY 未清零） |
| `CUS_FLASH_BUSY` | Flash 控制器忙 |
| `CUS_FLASH_ERROR` | 通用错误 |
| `CUS_FLASH_PARAMETER` | 参数非法（空指针、地址越界等） |
| `CUS_FLASH_PROGRAM_WRPRTERR` | 写保护错误 |
| `CUS_FLASH_PROGRAM_PGERR` | 编程错误 |
| `CUS_FLASH_ALIGNED_ERR` | 地址或长度未 4 字节对齐 |
| `CUS_FLASH_NOT_ERASED` | 目标区域未擦除（非全 0xFF） |
| `CUS_FLASH_VERIFY_ERR` | 写入后校验失败 |
| `CUS_FLASH_OVFLW_ERR` | RAM 索引表溢出 |
| `CUS_FLASH_NOSPACE_ERR` | Flash 空间不足 |
| `CUS_FLASH_NOT_FOUND` | 未找到匹配记录 |
| `CUS_FLASH_OVERLAP_ERR` | Manager 实例区域重叠 |
| `CUS_FLASH_LOW_VOLTAGE_ERR` | 低电压（保留，待扩展） |
| `CUS_FLASH_MUTEX_ERR` | 互斥锁获取失败 |

# sample_models 测试用例包

工程内置 18 套典型武器模型预检测试靶标（已包含头文件、导入库与动态库），目录结构如下：
`sample_models/<型号>/{include,lib,models}`

## 18 款测试靶标问题对照

| 问题类别 | 型号 | 缺陷特征与暴露方式 |
| :--- | :--- | :--- |
| **Debug 缺失** | `ReleaseOnly_A` / `ReleaseOnly_B` | 包内仅有 Release `.lib/.dll`，静态扫描提示缺少 Debug 库（`D=FAIL`） |
| **头文件重名冲突** | `HeaderClash_A` / `HeaderClash_B` | 同名 `WeaponModelParams` 结构体但字段顺序不同，成对勾选静态扫描报冲突（`FAIL`） |
| **多线程不安全** | `ThreadUnsafe_A` / `ThreadUnsafe_B` | 内部使用全局静态单例，多线程并发 Step 存在数据竞争 |
| **不支持多实例** | `SingleInstance_A` / `SingleInstance_B` | 仅支持 1 个槽位，第 2 次 Create 发生越界损坏 |
| **进程间干扰** | `SharedKernel_A` / `SharedKernel_B` | 同名 FileMapping 共享全局环境，同进程并发互相踩踏内存 |
| **CRT 运行时冲突** | `CrtMd_A` (/MD) / `CrtMt_B` (/MT) | PE 检查识别不同 CRT 运行时及堆隔离风险 |
| **内存泄漏** | `MemLeak_A` | `Model_Init` 泄漏 64KB，`Model_Step` 每步泄漏 1KB，性能压测可捕获内存持续上升 |
| **硬件异常崩溃 (SEH)** | `SehFault_A` / `SehFault_B` | `SehFault_A`（Step 空指针访问越界）、`SehFault_B`（Init 整数除零），SEH 隔离防护可精准捕获 |
| **基准合规对照** | `TcPass_A` / `TcPass_B` | 规范标准实现（支持多实例、线程互斥安全、/MD 编译、Debug/Release 齐全） |
| **类型隔离对照** | `TcPass_C` | 独立类型名与命名空间（`namespace TcPass_C`），可与 `TcPass_A` 无冲突混用 |

## 统一 API 规范（多数型号）

```c
void* Model_Create(void);
int   Model_Init(void* handle, const WeaponModelParams* params);
int   Model_Step(void* handle, WeaponModelOutput* output);
void  Model_Destroy(void* handle);
```

各型号内附带 `WeaponObject` 类，供 `MoCreate / MoInit / MoStep / MoDestroy` 适配器集成。

## TcPass_C 差异

- 参数/输出类型：`TcPass_C_ModelParams` / `TcPass_C_ModelOutput`
- 对象封装类：`TcPass_C::WeaponObject`
- Mo* 适配代码：`using MoModelType = TcPass_C::WeaponObject;`

# sample_models 测试用例包

生成路径：`dist/sample_models/<型号>/{include,lib,models}`

| 问题 | 型号对 | 如何暴露 |
|------|--------|----------|
| Debug 编译失败、Release 成功 | ReleaseOnly_A / ReleaseOnly_B | 包内仅有 Release `.lib/.dll`，无 Debug |
| 头文件结构体重名、无命名空间 | HeaderClash_A / HeaderClash_B | 同名 `WeaponModelParams`/`VendorConfig`，B 字段顺序不同 |
| 未做线程安全 | ThreadUnsafe_A / ThreadUnsafe_B | 全局单例状态，多线程 Step 竞态 |
| 不支持多实例 / 越界 | SingleInstance_A / SingleInstance_B | 仅 1 个槽位，第 2 次 Create 越界 |
| 共享全局/单例环境或写坏公共数据 | SharedKernel_A / SharedKernel_B | 同名 FileMapping + 越界写 |
| CRT 不匹配 / 堆隔离 | CrtMd_A (/MD) / CrtMt_B (/MT) | PE 可见不同 CRT；堆分配在 DLL 内 |
| 无上述问题（对照） | TcPass_A / TcPass_B | 多实例 + 互斥 + /MD + 完整 Debug/Release |
| 无上述问题 + 类型隔离（可与 A 同选） | TcPass_C | 独立类型名 + `namespace TcPass_C`，不与 A 结构体重名 |

## 统一 API（多数型号）

```c
void* Model_Create(void);
int   Model_Init(void* handle, const WeaponModelParams* params);
int   Model_Step(void* handle, WeaponModelOutput* output);
void  Model_Destroy(void* handle);
```

`WeaponObject` 供 MoCreate/MoInit/MoStep/MoDestroy 集成。

## TcPass_C 差异

- 参数/输出：`TcPass_C_ModelParams` / `TcPass_C_ModelOutput`
- 对象类：`TcPass_C::WeaponObject`
- Mo*：`using MoModelType = TcPass_C::WeaponObject;`

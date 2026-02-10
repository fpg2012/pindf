# PDF 文档修改（modif）功能实现提案

## 当前状态分析

### 现有实现
1. **`pdf/doc.c` 中的 `pindf_doc_save_modif` 函数**：仅为空壳实现，仅检查 `modif` 是否存在并输出警告
2. **`pindf_modif` 数据结构** (`modif.h`)：已实现基本结构，支持修改的间接对象链表管理
3. **`pindf_xref` 数据结构** (`xref.h`)：交叉引用表结构，支持多个 section
4. **序列化基础** (`core/serialize.c`)：已有 `pindf_pdf_obj_serialize` 函数，可将对象序列化为 PDF 格式

### 缺失功能
- xref 表的序列化输出功能
- trailer 字典的构建与更新逻辑
- 增量更新的完整 PDF 附加流程
- 修改应用的具体实现

## 核心目标

实现 PDF 增量更新功能：将修改以增量方式追加到原 PDF 文件末尾，生成符合 PDF 1.7 标准的增量更新段。

## 现有数据结构

### 1. `pindf_modif` (修改记录)
```c
typedef struct {
    pindf_ind_obj_node *modif_log;  // 按 obj_num 排序的链表
    int count;                      // 修改条目数
} pindf_modif;
```

**已有操作**：
- `pindf_modif_init()` / `pindf_modif_destroy()`
- `pindf_modif_new()`
- `pindf_modif_addentry()` - 添加间接对象到修改记录

### 2. `pindf_xref` (交叉引用表)
```c
typedef struct pindf_xref {
    pindf_xref_entry *entries;      // 扁平化条目数组
    pindf_xref_section *first_section; // section 链表
    size_t n_sections;              // section 数量
    size_t size;                    // 总对象数
} pindf_xref;
```

**已有操作**：
- `pindf_xref_init()` / `pindf_xref_alloc_section()`
- `pindf_xref_section_setentry()` / `pindf_xref_section_getentry()`

### 3. `pindf_pdf_ind_obj` (间接对象)
```c
typedef struct {
    int obj_num;                    // 对象编号
    int generation_num;             // 生成号
    pindf_pdf_obj *obj;             // 对象内容
    size_t start_pos;               // 起始位置
} pindf_pdf_ind_obj;
```

## 实现步骤

### 阶段一：基础功能扩展

#### 1.1 添加 xref 序列化函数
在 `xref.h`/`xref.c` 中添加：
```c
// 序列化整个 xref 表到文件
size_t pindf_xref_serialize(pindf_xref *xref, FILE *fp);

// 序列化单个 xref section
size_t pindf_xref_section_serialize(pindf_xref_section *section, FILE *fp);

// 创建用于增量更新的 xref 表
pindf_xref *pindf_create_incremental_xref(pindf_doc *doc, pindf_modif *modif);
```

#### 1.2 添加 trailer 处理函数
在 `doc.h`/`doc.c` 中添加：
```c
// 获取原始 trailer 字典
pindf_pdf_obj *pindf_doc_get_trailer(pindf_doc *doc);

// 构建更新后的 trailer 字典
pindf_pdf_obj *pindf_build_updated_trailer(pindf_doc *doc,
                                         pindf_xref *new_xref,
                                         size_t prev_xref_offset);
```

#### 1.3 添加对象序列化到文件的封装函数
在 `serialize.h`/`serialize.c` 中添加：
```c
// 将间接对象序列化到文件流
size_t pindf_ind_obj_serialize_to_file(pindf_pdf_ind_obj *ind_obj, FILE *fp);
```

### 阶段二：核心实现

#### 2.1 完善 `pindf_doc_save_modif` 函数
```c
void pindf_doc_save_modif(pindf_doc *doc, FILE *fp)
{
    if (doc->modif == NULL) {
        PINDF_WARN("no modification");
        return;
    }

    // 1. 记录新段起始位置
    long start_offset = ftell(fp);

    // 2. 序列化所有修改的间接对象
    pindf_ind_obj_node *node = doc->modif->modif_log->next;
    pindf_xref *new_xref = pindf_create_incremental_xref(doc, doc->modif);

    while (node) {
        size_t obj_offset = ftell(fp) - start_offset;
        pindf_ind_obj_serialize_to_file(node->ind_obj, fp);

        // 添加到新 xref 表
        pindf_xref_entry *entry = pindf_xref_getentry(new_xref,
            &(pindf_pdf_ref){node->obj_num, 0});
        if (entry) {
            entry->type = PINDF_XREF_ENTRY_N;
            entry->fields[0] = obj_offset;
            entry->fields[1] = 0;  // 生成号
        }

        node = node->next;
    }

    // 3. 写入新的 xref 表
    size_t xref_offset = ftell(fp) - start_offset;
    pindf_xref_serialize(new_xref, fp);

    // 4. 写入更新后的 trailer
    pindf_pdf_obj *new_trailer = pindf_build_updated_trailer(
        doc, new_xref, doc->xref_offset);
    // 需要实现 trailer 序列化函数
    pindf_trailer_serialize(new_trailer, fp);

    // 5. 写入 startxref
    fprintf(fp, "startxref\r\n%ld\r\n%%%%EOF\r\n", xref_offset);

    // 6. 清理
    pindf_xref_destroy(new_xref);
    pindf_pdf_obj_destroy(new_trailer);
}
```

#### 2.2 实现 trailer 构建逻辑
```c
pindf_pdf_obj *pindf_build_updated_trailer(pindf_doc *doc,
                                         pindf_xref *new_xref,
                                         size_t prev_xref_offset)
{
    // 获取原始 trailer 或创建新 trailer
    pindf_pdf_obj *trailer = pindf_doc_get_trailer(doc);
    if (!trailer) {
        trailer = pindf_pdf_obj_new(PINDF_PDF_DICT);
    }

    // 更新必要字段
    // /Size: 原始最大对象数 + 新增对象数
    // /Prev: 指向原始 xref 表
    // 保留 /Root, /Info, /ID 等关键字段

    return trailer;
}
```

### 阶段三：扩展功能

#### 3.1 支持对象删除
扩展 `pindf_modif` 结构以支持删除标记：
```c
enum pindf_modif_type {
    PINDF_MODIF_ADD_OR_UPDATE,
    PINDF_MODIF_DELETE
};

typedef struct pindf_ind_obj_node {
    uint obj_num;
    pindf_pdf_ind_obj *ind_obj;
    enum pindf_modif_type type;  // 新增：修改类型
    struct pindf_ind_obj_node *next;
} pindf_ind_obj_node;
```

#### 3.2 支持多次增量更新
处理 `/Prev` 链：
- 每次增量更新需指向上一次的 xref 表
- 需要维护更新链的完整性

#### 3.3 流对象优化
对于大型流对象，实现流式序列化以避免内存缓冲。

## 增量更新规范要点

### 文件结构
```
[原始 PDF 内容]
[新增/修改的间接对象 1]
[新增/修改的间接对象 2]
...
xref
0 1
0000000000 65535 f
[新 xref 条目]
trailer
<<
  /Size (原始总数 + 新增数)
  /Prev (原始 xref 偏移量)
  /Root (保持原样)
  /Info (保持原样)
  /ID (保持原样)
>>
startxref
[新 xref 表偏移量]
%%EOF
```

### 对象编号规则
1. **修改对象**：使用原始 `obj_num`
2. **新增对象**：分配新编号（原始最大编号 + 1）
3. **删除对象**：在 xref 表中标记为自由条目（F entry）

### xref 表规则
1. 仅包含修改/新增对象的条目
2. 条目偏移量相对于新段的起始位置
3. 自由条目（删除对象）标记为 `65535 f`

### trailer 更新规则
| 字段 | 更新规则 |
|------|----------|
| `/Size` | 原始最大对象数 + 新增对象数 |
| `/Prev` | 原始 xref 表偏移量 |
| `/Root` | 保持不变（指向原始 Catalog） |
| `/Info` | 保持不变 |
| `/ID` | 保持不变 |

## 边界情况处理

### 1. 空修改
- 当前实现：输出警告并返回
- 建议：可以添加详细日志说明

### 2. 多次增量更新
- 需要正确处理 `/Prev` 链式引用
- 建议：跟踪最新更新链

### 3. 对象删除
- 当前 `modif` 结构不支持
- 需要扩展支持删除标记

### 4. 大文件处理
- 流对象可能很大
- 建议：实现流式序列化，避免内存缓冲

### 5. 错误处理
- 文件写入失败
- 内存分配失败
- 序列化错误

## 建议的优先级

### 高优先级（第一阶段）
1. 实现 xref 序列化函数
2. 实现 trailer 处理逻辑
3. 完善 `pindf_doc_save_modif` 基础功能
4. 基础错误处理

### 中优先级（第二阶段）
1. 支持对象删除功能
2. 优化流对象处理
3. 完善错误处理和日志

### 低优先级（第三阶段）
1. 支持多次增量更新链
2. 性能优化（缓冲、批处理）
3. 高级功能（压缩、加密）

## 测试策略

### 单元测试
1. `pindf_xref_serialize` 函数测试
2. `pindf_build_updated_trailer` 函数测试
3. 单个间接对象序列化测试

### 集成测试
1. 简单文档增量更新测试
2. 包含修改、新增、删除的完整测试
3. 多次增量更新测试

### 回归测试
1. 确保原始 PDF 解析不受影响
2. 向后兼容性测试

## 性能考虑

### 内存使用
- 避免在序列化过程中缓冲整个对象
- 流式处理大对象

### 文件 I/O
- 使用缓冲写入
- 减少文件定位操作

### 复杂度
- 增量更新应保持 O(n) 复杂度，n 为修改对象数

## 结论

`pindf_doc_save_modif` 功能的完整实现需要约 300-500 行新增代码，主要集中在：
1. xref 序列化（50-100 行）
2. trailer 处理（100-150 行）
3. 核心增量更新逻辑（150-200 行）
4. 辅助函数和错误处理（50 行）

实现后，PDF 编辑器将支持符合标准的增量更新功能，能够在保持原始文件大部分内容不变的情况下应用修改。

---
*提案版本：1.0*
*更新日期：2026-02-09*
*基于代码分析：doc.c, modif.h, xref.h, serialize.c*
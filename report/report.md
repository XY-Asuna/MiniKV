# 实验报告
实验所有代码 数据 等文件详细可见[github repo](https://github.com/XY-Asuna/MiniKV)（ https://github.com/XY-Asuna/MiniKV ）
## 数据结构设计
![数据结构设计图](数据结构设计图.png)
图1 数据结构设计图

**说明**
1. 使用数组模拟内存`Memory` 每个元素代表一个`Block` 存储 `onwer`和`used`信息
2. 每个`Request`的`head`与`tail`维护一个`BlockNode`链表 其`index`与`Memory`数组一一对应
3. 使用数组`Buckets`作为哈希数组
4. 每个哈希值对应一个`Request`链表 存储哈希冲突的各个`Request`信息
5. `Request`链表节点存储`Request`的信息 其中`head` 和 `tail` 分别指向该token在内存中的首尾位置
## 算法设计
### 构造函数
1. 申请并初始化`Memory`数组空间
2. 申请并初始化`Buckets`数组空间 每个元素初始化一个`Request`链表
3. 若为`Paged Strategy` 则另外申请`to_used_index`供后续`allocate()`和`append()`使用
### 析构函数
1. 释放`Memory`数组空间
2. 释放每个`Request`对应的`BlockNode`链表空间 然后释放该`Request`链表空间
3. 释放`Buckets`数组空间
4. 若为`Paged Strategy` 则释放`to_used_index`空间
### allocate
#### 算法逻辑
1. 判断`tokenCount`合法性以及调用`lookup`函数判断该`token`是否已经存在
2. 
   1. **Contiguous Strategy**：遍历`Memory`数组 寻找连续的空闲块并记录即将存放的内存块的首尾位置
   2. **Paged Strategy**：遍历`Memory`数组 寻找空闲块并用`to_used_index`记录即将存放的位置
3. 计算哈希值 将`Request`插入到对应的哈希链表中
4. 更新`Memory`数组中对应的`Block`的`owner`和`used`信息 同时使用尾插法创建`BlockNode`链表
#### 伪代码实现
```cpp
```
#### 复杂度分析
1. 调用`lookup`函数的时间复杂度为 $O(1)$
2. 遍历`Memory`数组的时间复杂度为 $O(n)$
3. 计算哈希值的时间复杂度为 $O(1)$
4. 更新`Memory`数组和创建`BlockNode`链表的时间复杂度为 $O(n)$

可以看出`allocate`函数的时间复杂度为 $O(n)$
### append
#### 算法逻辑
1. 判断`tokenCount`合法性以及调用`lookup`函数判断该`token`是否已经存在
2. 判断原有`token`所占最后一块`Block`剩余空间是否足够 若足够则直接在原有`Block`上追加 否则进入后续步骤
3. 
   1. **Contiguous Strategy**：遍历`Memory`数组 寻找连续的空闲块并记录即将存放的内存块的首尾位置
   2. **Paged Strategy**：遍历`Memory`数组 寻找空闲块并用`to_used_index`记录即将存放的位置
4. 将原有`token`所占最后一块`Block`填满
5. 计算哈希值 寻找到对应的`Request`
6. 更新`Memory`数组中对应的`Block`的`owner`和`used`信息 同时在尾部插入对应节点以更新`BlockNode`链表
#### 伪代码实现
```cpp
```
#### 复杂度分析
1. 调用`lookup`函数的时间复杂度为 $O(1)$
2. 判断原有`token`所占最后一块`Block`剩余空间的时间复杂度为 $O(1)$
3. 遍历`Memory`数组的时间复杂度为 $O(n)$
4. 填满原有`token`所占最后一块`Block`的时间复杂度为 $O(1)$
5. 计算哈希值的时间复杂度为 $O(1)$
6. 更新`Memory`数组和`BlockNode`链表的时间复杂度为 $O(n)$

可以看出`append`函数的时间复杂度为 $O(n)$
### lookup
#### 算法逻辑
##### 哈希函数：fnv1a
作为一种实现相对简单 结果稳定且性能较好的哈希函数。对比fnv1通过改变异或和乘法的顺序来提高雪崩效应 保证了哈希值的均匀随机性。

### free
### displayMemory
### displayRequest
## 实验评估与算法分析
### 实验评估
#### 不同工作负载下两个Strategy 的表现
#### Block Size 对系统的影响 
### 算法分析
#### 时间复杂度分析
#### Fragmentation 指标

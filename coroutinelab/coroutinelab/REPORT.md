# Coroutine Lab 实验报告

> 侯林之 2023010779 计33

## **Task 1: 协程库的编写**

### 添加代码

1. `coroutine.h`

   ```c++
   void serial_execute_all(){
       ...
       bool all_finished = false; // 标志检查所有协程是否完成
       while (!all_finished){
           all_finished = true; // 假设所有协程都已完成
           for (int i = 0; i < coroutines.size(); i++){
               all_finished = false; // 如果有任何协程未完成，则将标志设置为false
               context_id = i; // 设置当前上下文ID
               if (!coroutines[i]->finished){
                   coroutines[i]->resume(); // 如果协程未完成，则恢复执行
               }
           }
       }
       ...
   }
   ```

2. `context.h`

   ```c++
   virtual void resume(){
   // 调用 coroutine_switch
   // 在汇编中保存 callee-saved 寄存器，设置协程函数栈帧，然后将 rip 恢复到协程 yield 之后所需要执行的指令地址。
   coroutine_switch(caller_registers, callee_registers); // 切换到协程上下文
   }
   ```

3. `common.h`

   ```c++
   void yield(){
     if (!g_pool->is_parallel){
       ...
       // 调用 coroutine_switch 切换到 coroutine_pool 上下文
       coroutine_switch(context->callee_registers, context->caller_registers); // 切换到 coroutine_pool 上下文
     }
   }
   ```

4. `context.S`

   ```asm
   coroutine_switch:
       # TODO: Task 1
       # 保存 callee-saved 寄存器到 %rdi 指向的上下文
       movq %rsp, 64(%rdi)
       movq %rbx, 72(%rdi)
       movq %rbp, 80(%rdi)
       movq %r12, 88(%rdi)
       movq %r13, 96(%rdi)
       movq %r14, 104(%rdi)
       movq %r15, 112(%rdi)
       # 保存的上下文中 rip 指向 ret 指令的地址（.coroutine_ret）
       leaq .coroutine_ret(%rip), %rax
       movq %rax, 120(%rdi)
   
       # 从 %rsi 指向的上下文恢复 callee-saved 寄存器
       movq 64(%rsi), %rsp
       movq 72(%rsi), %rbx
       movq 80(%rsi), %rbp
       movq 88(%rsi), %r12
       movq 96(%rsi), %r13
       movq 104(%rsi), %r14
       movq 112(%rsi), %r15
       # 最后 jmpq 到上下文保存的 rip
       movq 120(%rsi), %rax
       jmpq *%rax
   ```

### 分析代码

1. `basic_context`

   ```c++
   uint64_t rsp = (uint64_t)&stack[stack_size - 1];
   rsp = rsp - (rsp & 0xF);
   
   void coroutine_main(struct basic_context * context);
   
   callee_registers[(int)Registers::RSP] = rsp;
   // 协程入口是 coroutine_entry
   callee_registers[(int)Registers::RIP] = (uint64_t)coroutine_entry;
   // 设置 r12 寄存器为 coroutine_main 的地址
   callee_registers[(int)Registers::R12] = (uint64_t)coroutine_main;
   // 设置 r13 寄存器，用于 coroutine_main 的参数
   callee_registers[(int)Registers::R13] = (uint64_t)this;
   ```

   在 `basic_context` 结构体的构造函数中，协程的栈空间被分配了指定的大小，并通过计算对齐到 16 字节边界的栈顶地址，以确保满足硬件平台的 ABI 规范，因为不对齐的栈可能导致某些硬件平台在执行过程中的未定义行为。随后，`callee_registers` 数组中的关键寄存器被初始化，其中 `RSP` 寄存器被设置为对齐后的栈顶地址，用作协程的运行栈，`RIP` 寄存器被设置为 `coroutine_entry`，即协程的入口函数地址。而 `R12` 和 `R13` 分别被初始化为 `coroutine_main` 的地址和 `this` 指针，从而将协程的运行逻辑与其上下文实例绑定。

2. `coroutine_main`

   ```c++
   void coroutine_main(struct basic_context *context){
     context->run();
     context->finished = true;
     coroutine_switch(context->callee_registers, context->caller_registers);
   
     // unreachable
     assert(false);
   }
   ```

   `coroutine_main` 函数是协程的主要执行逻辑，其中 `context->run()` 调用了协程上下文对象的 `run` 方法，执行协程的主要任务。任务完成后，`finished` 标志被设置为 `true`，表示协程已经执行完成。接着，通过 `coroutine_switch` 将控制权切换回调用者。如果协程未正确切换回调用者，上下文将触发 `assert(false)`，以确保协程的逻辑不会意外地继续运行到无效的状态。

### 额外要求

1. 绘制出在协程切换时，栈的变化过程；

   1. **初次切换到目标协程**
      在初次切换到目标协程时，目标协程尚未执行过代码。此时，`coroutine_entry` 设置了目标协程的入口函数为 `coroutine_main`，并将 `basic_context` 的指针作为参数传递。目标协程的栈帧仅包含初始配置的寄存器状态。

      | 协程 A 的栈 | 协程 B 的栈                |
      | ----------- | -------------------------- |
      | `resume`    |                            |
      | 其他指令    |                            |
      | `%rsp`      |                            |
      | `switch`    | **未初始化，等待首次切换** |

     2. **目标协程运行中**
        当目标协程正在运行时，协程 A 的栈帧被保存，协程 B 的栈帧被恢复并用于当前执行。此时，协程 B 的栈帧包含当前执行状态和寄存器状态。

      | 协程 A 的栈          | 协程 B 的栈 |
      | -------------------- | ----------- |
      | **暂停状态，保存中** | `yield`     |
      | 其他指令             | 其他指令    |
      |                      | `%rsp`      |
      | `switch`             | `switch`    |

     3. **目标协程调用 `yield`**
        当目标协程调用 `yield` 时，协程 B 的栈帧被保存，协程 A 的栈帧被恢复。此时，协程 A 的栈帧包含当前执行状态和寄存器状态。

      | 协程 A 的栈 | 协程 B 的栈          |
      | ----------- | -------------------- |
      | `resume`    | **暂停状态，保存中** |
      | 其他指令    | 其他指令             |
      | `%rsp`      |                      |
      | `switch`    | `switch`             |

     4. **目标协程运行结束**
        当目标协程执行完毕或调用 `yield` 返回调用方后，控制权返回到调用协程。此时，目标协程的栈帧被销毁，控制权回到调用方。

      | 协程 A 的栈 | 协程 B 的栈  |
      | ----------- | ------------ |
      | `resume`    | **栈被销毁** |
      | 其他指令    |              |
      | `%rsp`      |              |
      | `switch`    |              |

2. 并结合源代码，解释协程是如何开始执行的，包括 `coroutine_entry` 和 `coroutine_main` 函数以及初始的协程状态；

   协程的执行从 `coroutine_entry` 开始，该函数是协程的初始入口点，由 `basic_context` 构造函数初始化。构造协程时，分配了栈空间并对齐，使栈指针（`RSP`）满足 x86-64 的 ABI 要求，同时通过 `callee_registers` 设置寄存器状态：`RSP` 指向栈顶，`RIP` 设置为 `coroutine_entry` 地址。首次切换到协程时，它会从 `coroutine_entry` 开始执行。

   `coroutine_entry` 是汇编函数，跳转到 `coroutine_main`，通过 `callee_registers` 中的 `R12` 和 `R13` 分别传递 `coroutine_main` 的地址和 `this` 指针，从而初始化协程的上下文。在 `coroutine_main` 中，调用协程的 `run` 方法，最终利用模板展开调用用户定义的协程函数。协程函数运行时，栈帧与寄存器上下文完全独立。

   当协程函数结束或调用 `yield` 时，控制权返回到 `coroutine_main`，随后调用 `coroutine_switch` 保存当前状态并切换回调用方。初始状态通过 `callee_registers` 精确设置，使协程能够从 `coroutine_entry` 开始，并跳转到用户逻辑执行。


3. 目前的协程切换代码只考虑了通用寄存器，设计一下，如果要考虑浮点和向量寄存器，要怎么处理。

   可以扩展 `basic_context` 中的 `caller_registers` 和 `callee_registers` 数据结构，或为这些寄存器单独添加新的数组成员。在 `coroutine_switch` 的汇编实现中，需要额外加入保存和恢复浮点与向量寄存器的指令，例如使用 `movaps`、`vmovaps` 等指令，以确保在协程切换时完整保存和恢复寄存器状态。

## **Task 2: 实现 sleep 函数**

### 添加代码

1. `common.h`

   ```c++
       // 从 g_pool 中获取当前协程状态
       auto context = g_pool->coroutines[g_pool->context_id];
   
       // 获取当前时间，更新 ready_func
       // ready_func：检查当前时间，如果已经超时，则返回 true
       context->ready = false; // 设置协程为不可用状态
       auto cur = get_time(); // 获取当前时间
       context->ready_func = [cur, ms]() {
         return std::chrono::duration_cast<std::chrono::milliseconds>(get_time() - cur).count() >= ms;
       };
   
       // 调用 coroutine_switch 切换到 coroutine_pool 上下文
       coroutine_switch(context->callee_registers, context->caller_registers);
   ```

2. `coroutine_pool.h`

   ```c++
   if (!coroutines[i]->finished)
   {
     all_finished = false;
     if (!coroutines[i]->ready)// 当前协程的ready属性为false
     {
       if (coroutines[i]->ready_func()) // 如果ready_func返回true
         coroutines[i]->ready = true; // 将当前协程的ready属性设置为true
       else
         continue; // 否则跳过当前协程
     }
     coroutines[i]->resume();
   }
   ```

### 额外要求

1. 按照时间线，绘制出 `sleep_sort` 中不同协程的运行情况；

   以 1, 3, 4, 5, 2 为例，按照时间顺序：

   **协程1：** print(1)，**协程2：** print(3)，**协程3：** print(4)，**协程4：** print(5)，**协程5：** print(2)

   | 协程 | 协程12345    | 协程1    | 协程5    | 协程2    | 协程3    | 协程4    |
   | ---- | ------------ | -------- | -------- | -------- | -------- | -------- |
   | 状态 | 几乎同时开始 | 结束     | 结束     | 结束     | 结束     | 结束     |
   | 结果 | ---          | print(1) | print(2) | print(3) | print(4) | print(5) |

2. 目前的协程库实现方式是轮询 `ready_func` 是否等于 `true`，设计一下，能否有更加高效的方法。

   引入事件驱动机制，通过让协程监听特定的事件来判断是否就绪。当协程调用 `sleep` 函数时，它会监听一个超时事件，记录当前时间加上等待时间作为触发点。一旦事件触发，协程将被唤醒并标记为 ready。这种方法避免了频繁的轮询，通过事件的自然触发减少无效的 CPU 消耗，同时提高了调度器的效率，使协程运行更加流畅。

## **Task 3: 利用协程优化二分查找**

### 添加代码

1. `binary_search.cpp`

   ```c++
   // 使用 __builtin_prefetch 预取容易产生缓存缺失的内存
   __builtin_prefetch(&table[probe]);
   // 并调用 yield
   yield();
   ```

### 额外要求

1. 汇报性能的提升效果。

   通过修改命令行参数多次运行，比较不同参数配置下的结果分析出程序性能的变化情况。

   1. `-l` (log2_size) 对性能的影响

   | `-l` | Size       | Naive Search (ns per search) | Naive Search (ns per access) | Coroutine Search (ns per search) | Coroutine Search (ns per access) |
   | ---- | ---------- | ---------------------------- | ---------------------------- | -------------------------------- | -------------------------------- |
   | 4    | 16         | 5.24                         | 1.31                         | 292.71                           | 73.18                            |
   | 8    | 256        | 8.54                         | 1.07                         | 375.58                           | 46.95                            |
   | 16   | 65536      | 33.39                        | 2.09                         | 676.02                           | 42.25                            |
   | 32   | 4294967296 | 5398.74                      | 168.71                       | 1593.81                          | 49.81                            |

    - 随着 `-l` 增加，数据数组的大小呈指数级增长。
    - Naive 方法的搜索时间随着数据大小的增加显著增加。
    - Coroutine 方法的搜索时间也随着数据大小的增加而增加，但增幅较小。
    - 对于较大的数据大小（如 `-l = 32`），Coroutine 方法的性能明显优于 Naive 方法。

   2. `-m` (loops) 对性能的影响

   | `-m`    | Size       | Naive Search (ns per search) | Naive Search (ns per access) | Coroutine Search (ns per search) | Coroutine Search (ns per access) |
   | ------- | ---------- | ---------------------------- | ---------------------------- | -------------------------------- | -------------------------------- |
   | 8000    | 4294967296 | 2341.64                      | 73.18                        | 1427.36                          | 44.61                            |
   | 40000   | 4294967296 | 2303.16                      | 71.97                        | 1443.73                          | 45.12                            |
   | 200000  | 4294967296 | 2455.18                      | 76.72                        | 1415.22                          | 44.23                            |
   | 1000000 | 4294967296 | 2376.10                      | 74.25                        | 1431.10                          | 44.72                            |

    - 随着 `-m` 增加，搜索操作的次数增加。
    - Naive 方法的每次搜索时间和每次访问时间变化不大。
    - Coroutine 方法的每次搜索时间和每次访问时间也变化不大，但始终优于 Naive 方法。

   3. `-b` (batch size) 对性能的影响

   | `-b` | Size       | Naive Search (ns per search) | Naive Search (ns per access) | Coroutine Search (ns per search) | Coroutine Search (ns per access) |
   | ---- | ---------- | ---------------------------- | ---------------------------- | -------------------------------- | -------------------------------- |
   | 2    | 4294967296 | 2392.82                      | 74.78                        | 2236.14                          | 69.88                            |
   | 4    | 4294967296 | 2387.66                      | 74.61                        | 1478.81                          | 46.21                            |
   | 8    | 4294967296 | 2377.82                      | 74.31                        | 1401.96                          | 43.81                            |
   | 16   | 4294967296 | 5097.13                      | 159.29                       | 1561.69                          | 48.80                            |

    - Batch size 对 Coroutine 方法的性能有显著影响。
    - 较小的 batch size（如 2）导致较高的每次搜索时间。
    - 较大的 batch size（如 8 和 16）显著提高了 Coroutine 方法的性能。
    - Naive 方法的性能基本不受 batch size 影响。

     性能提升效果

    - **数据大小 (`-l`) 增加时：** Coroutine 方法在处理大数据集时表现出更好的性能扩展性，相比 Naive 方法，Coroutine 方法的性能提升显著。
    - **搜索操作次数 (`-m`) 增加时：** 无论是 Naive 方法还是 Coroutine 方法，性能变化不大，但 Coroutine 方法始终优于 Naive 方法。
    - **Batch size (`-b`) 增加时：** Coroutine 方法的性能随着 batch size 的增加而显著提升，尤其是在较大的数据集上。

    通过使用适当的 batch size，Coroutine 方法在处理大规模数据集和大量搜索操作时，能够显著提升性能。相比之下，Naive 方法在数据规模增大时性能下降明显。

## 总结和感想

通过这次实验，我深刻感受到协程实现的核心难点在于理解它的运行机制和上下文切换的原理。在实际编写代码的过程中，我起初对协程的运行逻辑以及上下文保存和恢复的细节感到困惑，但通过反复阅读代码和注释，再结合调试和测试一步步完善，我对协程有了更全面的认识。

实验的关键在于第一个子任务的实现，PPT中对需要完成的函数功能和方法提供了清晰的指导。我按照提示逐步完成相关代码，发现协程虽然概念看似抽象，但在逐行实现的过程中，它的核心原理其实并不复杂，只要理清了栈空间的切换，整个实现就变得豁然开朗。

这次实验让我第一次接触到C++和汇编代码的结合，说实话，刚开始还是挺有压力的，尤其是面对汇编代码的语法和它直接操作计算机底层的特性，让我一度摸不着头脑。不过，随着实验的深入，我也慢慢意识到，汇编语言虽然看起来晦涩，但它能够操作底层，完成许多高级语言无法实现的功能。通过这次实验，我不仅了解了协程的底层实现，也对汇编语言的实际应用有了更直观的感受。

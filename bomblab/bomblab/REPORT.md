

# Bomblab

首先在终端运行以下指令，将有关内容存储起来：

```bash
objdump -d ./bomb > objdump.txt
xxd ./bomb > xxd.txt
strings ./bomb > strings.txt
```

## phase_1

在 `objdump.txt` 中发现如下字段，从 `phase_1` 的反汇编代码中，可以推断出如何找到与之对应的字符串。对 `phase_1` 函数的分析如下：

```assembly
0000000000001607 <phase_1>:
    1607:	f3 0f 1e fa          	endbr64 
    160b:	48 83 ec 08          	sub    $0x8,%rsp 
    160f:	48 8d 35 c2 1c 00 00 	lea    0x1cc2(%rip),%rsi        # 32d8 <_IO_stdin_used+0x2d8>
    1616:	e8 8e 05 00 00       	call   1ba9 <strings_not_equal>
    # 调用 `strings_not_equal` 函数，比较输入和指定字符串。比较的结果返回在 `%eax` 中。
    161b:	85 c0                	test   %eax,%eax 
    161d:	75 05                	jne    1624 <phase_1+0x1d> 
    # 如果返回值不为0 (代表字符串不相等)，跳转到 `1624` 调用 `explode_bomb`。
    161f:	48 83 c4 08          	add    $0x8,%rsp
    1623:	c3                   	ret
    1624:	e8 a3 06 00 00       	call   1ccc <explode_bomb>
    # 如果字符串不匹配，引爆炸弹。
    1629:	eb f4                	jmp    161f <phase_1+0x18>
```

从汇编代码中，我们看到：

```assembly
160f:	lea    0x1cc2(%rip),%rsi        # 32d8 <_IO_stdin_used+0x2d8>  
```

这里只显示了通过 `RIP` 基址加 `0x1cc2` 偏移得到的最终字符串目标地址，`0x32d8` 是字符串在程序中的偏移位置。接着，从 `xxd.txt` 中找到地址为 `32d8` 的部分，并找到相应的字符串：

```
000032b0: 536f 2079 6f75 2067 6f74 2074 6861 7420  So you got that 
000032c0: 6f6e 652e 2020 5472 7920 7468 6973 206f  one.  Try this o
000032d0: 6e65 2e00 0000 0000 5765 2068 6176 6520  ne......We have 
000032e0: 746f 2073 7461 6e64 2077 6974 6820 6f75  to stand with ou
000032f0: 7220 4e6f 7274 6820 4b6f 7265 616e 2061  r North Korean a
00003300: 6c6c 6965 732e 0000 576f 7721 2059 6f75  llies...Wow! You
00003310: 2776 6520 6465 6675 7365 6420 7468 6520  've defused the 
00003320: 7365 6372 6574 2073 7461 6765 2100 0000  secret stage!...
```

在`strings.txt`中可以找到相应语句：

```
So you got that one.  Try this one.
We have to stand with our North Korean allies.
Wow! You've defused the secret stage!
```

于是可以推断 `phase_1`  的密码是 `We have to stand with our North Korean allies.` ，运行程序验证猜想，发现可以成功。

同时，在 `gdb` 中使用 `disassemble` 命令可以查看以查看 `phase_1` 的汇编代码：

```gdb
(gdb) disassemble phase_1  
Dump of assembler code for function phase_1:
   0x0000555555555607 <+0>:     endbr64
   0x000055555555560b <+4>:     sub    $0x8,%rsp
   0x000055555555560f <+8>:     lea    0x1cc2(%rip),%rsi        # 0x5555555572d8
   0x0000555555555616 <+15>:    call   0x555555555ba9 <strings_not_equal>
   0x000055555555561b <+20>:    test   %eax,%eax
   0x000055555555561d <+22>:    jne    0x555555555624 <phase_1+29>
   0x000055555555561f <+24>:    add    $0x8,%rsp
   0x0000555555555623 <+28>:    ret
   0x0000555555555624 <+29>:    call   0x555555555ccc <explode_bomb>
   0x0000555555555629 <+34>:    jmp    0x55555555561f <phase_1+24>
End of assembler dump.
```

`strings_not_equal` 用于比较两个字符串。如果返回值为0，则表示输入字符串与地址 `0x5555555572d8` 的字符串相等。要找到正确的输入，可以查看 `0x5555555572d8` 处的字符串。在 GDB 中，检查该地址上的内容：

```gdb
(gdb) x/s 0x5555555572d8  
0x5555555572d8: "We have to stand with our North Korean allies."
```

同样找到 `phase_1` 的正确字符串。

## phase_2

从 `phase_2` 的反汇编代码中，可以推断出如何找到与之对应的字符串。对 `phase_2` 函数的分析如下：

1. **读取六个数字**：
   - `1648: call 1cf8 <read_six_numbers>`：调用从输入中读取六个数字的函数。
2. **检查第一个数字**：
   - `164d: cmpl $0x1,(%rsp)`：检查第一个读到的数字是否为1。
   - 如果不是，则调用 `explode_bomb` (`165d: call 1ccc <explode_bomb>`)，表明第一个数字必须是1。
3. **迭代检查**：
   - 从 `1653: mov %rsp,%rbx` 到 `166b: je 167d <phase_2+0x52>`：用以遍历检查输入的数字。
   - `166d: mov (%rbx),%eax`：将当前数字加载到 `%eax`。
   - `166f: add %eax,%eax`：将当前数字加倍。
   - `1671: cmp %eax,0x4(%rbx)`：将加倍的数值与下一个数字进行比较。
   - `1674: je ...`：如果它们相等，就向前继续，否则引爆炸弹 (`1676: call 1ccc <explode_bomb>`)。
4. **栈保护与返回**：
   - 最后检查栈保护机制以防止溢出攻击。
   - `1694: call 1260 <__stack_chk_fail@plt>` 如果栈保护失效，程序将会失败。

从上述分析可以看出，`phase_2` 期望输入的六个数字满足如下条件：

- 第一个数字为1。

- 每个后续数字是前一个数字的两倍。

从而知道 `phase_2` 的密码是 `1 2 4 8 16 32` ，经检验后成功。

## phase_3

从 `phase_3` 的汇编代码中可以观察到这个阶段与输入值涉及多条逻辑分支和条件检查。

### 读取输入

```assembly
169d:	48 83 ec 28          	sub    $0x28,%rsp
16a1:	64 48 8b 04 25 28 00 	mov    %fs:0x28,%rax
16a8:	00 00 
16aa:	48 89 44 24 18       	mov    %rax,0x18(%rsp)
16af:	31 c0                	xor    %eax,%eax
16b1:	48 8d 4c 24 0f       	lea    0xf(%rsp),%rcx
16b6:	48 8d 54 24 10       	lea    0x10(%rsp),%rdx
16bb:	4c 8d 44 24 14       	lea    0x14(%rsp),%r8
16c0:	48 8d 35 c1 19 00 00 	lea    0x19c1(%rip),%rsi        # 3088 <_IO_stdin_used+0x88>
16c7:	e8 44 fc ff ff       	call   1310 <__isoc99_sscanf@plt>
```

- 以上部分通常是设置栈和准备使用 `sscanf` 来读取用户输入。
- `3088` 是格式化字符串的地址，用于 `sscanf`。可能用于读取两个值。

通过在 `xxd.txt` 中找到 `3088` 处的字符串，解析可以知道：程序期望的输入是一个整数、一个字符和另一个整数（`%d %c %d`）。

```
00003070: 6f72 6b21 2020 4f6e 2074 6f20 7468 6520  ork!  On to the 
00003080: 6e65 7874 2e2e 2e00 2564 2025 6320 2564  next....%d %c %d
00003090: 0057 656c 6c2e 2e2e 004f 4b2e 203a 2d29  .Well....OK. :-)
```

通过 `gdb` 也可知道密码格式：

```
(gdb) x/s 0x3088
0x3088: "%d %c %d"
```

### 间接跳转

```assembly
16d1:	83 7c 24 10 07       	cmpl   $0x7,0x10(%rsp)
16d6:	0f 87 00 01 00 00    	ja     17dc <phase_3+0x143>
16dc:	8b 44 24 10          	mov    0x10(%rsp),%eax
16e0:	48 8d 15 f9 1d 00 00 	lea    0x1df9(%rip),%rdx        # 34e0 <_IO_stdin_used+0x4e0>
16e7:	48 63 04 82          	movslq (%rdx,%rax,4),%rax
16eb:	48 01 d0             	add    %rdx,%rax
16ee:	3e ff e0             	notrack jmp *%rax
```

- `cmpl $0x7,0x10(%rsp)`：检查第一个整数是否大于7。如果大于7，程序会调用 `explode_bomb`。
- `mov 0x10(%rsp),%eax`：将第一个整数加载到 `eax`。
- `lea 0x1df9(%rip),%rdx`：加载跳转表的基地址到 `rdx`。
- `movslq (%rdx,%rax,4),%rax`：从跳转表中获取偏移量。
- `add %rdx,%rax`：计算目标地址。
- `notrack jmp *%rax`：跳转到目标地址。

### 分支分析

#### 分支 0

```assembly
16f8:	b8 78 00 00 00       	mov    $0x78,%eax
16fd:	81 7c 24 14 a2 00 00 	cmpl   $0xa2,0x14(%rsp)
1704:	00 
1705:	0f 84 db 00 00 00    	je     17e6 <phase_3+0x14d>
170b:	e8 bc 05 00 00       	call   1ccc <explode_bomb>
1710:	b8 78 00 00 00       	mov    $0x78,%eax
1715:	e9 cc 00 00 00       	jmp    17e6 <phase_3+0x14d>
```

- 将 `0x78` (即 ASCII 字符 `'x'`) 移动到 `eax`。
- 比较第三个输入整数是否为 `0xa2` (162)。
- 如果相等，跳转到 `17e6`，否则调用 `explode_bomb`。
- 最后，`eax` 保持为 `0x78`。

#### 分支 1

```assembly
171a:	b8 6b 00 00 00       	mov    $0x6b,%eax
171f:	83 7c 24 14 61       	cmpl   $0x61,0x14(%rsp)
1724:	0f 84 bc 00 00 00    	je     17e6 <phase_3+0x14d>
172a:	e8 9d 05 00 00       	call   1ccc <explode_bomb>
172f:	b8 6b 00 00 00       	mov    $0x6b,%eax
1734:	e9 ad 00 00 00       	jmp    17e6 <phase_3+0x14d>
```

- 将 `0x6b` (即 ASCII 字符 `'k'`) 移动到 `eax`。
- 比较第三个输入整数是否为 `0x61` (97)。
- 如果相等，跳转到 `17e6`，否则调用 `explode_bomb`。
- 最后，`eax` 保持为 `0x6b`。

同理可得，所有可能的密码为：

```
0 x 162
1 k 97
2 g 62
3 q 140
4 l 583
5 b 790
6 j 126
7 g 755
```

## phase_4

函数 `phase_4` 从地址 `0x1843` 开始。

**读取输入**：

```assembly
184b: 64 48 8b 04 25 28 00  mov    %fs:0x28,%rax  
1852: 00 00   
1854: 48 89 44 24 08        mov    %rax,0x8(%rsp)  
1859: 31 c0                 xor    %eax,%eax  
185b: 48 8d 4c 24 04        lea    0x4(%rsp),%rcx  
1860: 48 89 e2              mov    %rsp,%rdx  
1863:	48 8d 35 74 18 00 00 	lea    0x1874(%rip),%rsi        # 30de <_IO_stdin_used+0xde> 
186a: e8 a1 fa ff ff        call   1310 <__isoc99_sscanf@plt>  
```

- 调用 `sscanf` 读取输入，期望两个整数作为响应，然后将其放入栈中。
- `30de` 是格式化字符串的地址，用于 `sscanf`。可能用于读取两个值。

通过在 `xxd.txt` 中找到 `30de` 处的字符串，解析可以知道：程序期望的输入是一个整数和另一个整数（`%d %d`）。

```
000030c0: 6f6d 6220 6861 7320 626c 6f77 6e20 7570  omb has blown up
000030d0: 2e00 2564 2025 6420 2564 2025 6420 2564  ..%d %d %d %d %d
000030e0: 2025 6400 4572 726f 723a 2050 7265 6d61   %d.Error: Prema
```

通过 `gdb` 也可知道密码格式：

```
(gdb) x/s 0x30de
0x30de: "%d %d"
```

**输入验证**：

```assembly
186f: 83 f8 02              cmp    $0x2,%eax  
1872: 75 06                 jne    187a <phase_4+0x37>  
1874: 83 3c 24 0e           cmpl   $0xe,(%rsp)  
1878: 76 05                 jbe    187f <phase_4+0x3c>  
187a: e8 4d 04 00 00        call   1ccc <explode_bomb>  
```

- 确保接收到两个有效输入，并且第一个输入不超过 14 (`0x0e`)。如果不满足，则调用 `explode_bomb`。

**函数调用和条件**：

```
187f: ba 0e 00 00 00        mov    $0xe,%edx  
1884: be 00 00 00 00        mov    $0x0,%esi  
1889: 8b 3c 24              mov    (%rsp),%edi  
188c: e8 7c ff ff ff        call   180d <func4>  
1891: 83 f8 07              cmp    $0x7,%eax  
1894: 75 07                 jne    189d <phase_4+0x5a>  
```

- 初始化了一些寄存器值：`%edx = 0xe`（14），`%esi = 0`。

  调用 `func4` 并传入 `%edi`、`%esi` 和 `%edx` 来执行递归运算。

- 检查结果是否等于 `7`。如果不等，则调用 `explode_bomb`。

**最终检查和返回**：

```
1896: 83 7c 24 04 07        cmpl   $0x7,0x4(%rsp)  
189b: 74 05                 je     18a2 <phase_4+0x5f>  
189d: e8 2a 04 00 00        call   1ccc <explode_bomb>  
```

- 最后，验证第二个输入（在 `0x4(%rsp)`）是否为 `7`。否则调用 `explode_bomb`。

### `func4`

`func4` 是一个递归函数，它使用二分搜索算法来处理输入。从功能和汇编代码的分析来看，`func4` 的原型大致为 `int func4(int target, int low, int high)`。

1. **计算中间值**:
   - 从 `low` 和 `high` 计算 `mid`，实际表示为 `mid = low + (high - low) / 2` 。
   - 通过这一系列汇编指令计算得出中间值 `%ebx`。
2. **比较 `mid` 和 `target`**:
   - **`cmp %edi, %ebx`**：比较 `mid` 与 `target`。
   - 如果 `mid > target`，执行 `func4(target, low, mid - 1)`。
   - 如果 `mid < target`，执行 `func4(target, mid + 1, high)`。
   - 如果 `mid == target`，返回 `mid`。
3. **递归调用**:
   - 如果 `mid` 与 `target` 不相同，递归调用自身，调整 `low` 或 `high`。
4. **返回**:
   - 调整后的 `mid` 值通过递归汇加返回。


由此可近似还原出原 `C语言` 代码：

```c
int func4(int edi, int esi, int edx) {
    // 计算中间值
    int eax = edx - esi;
    int ebx = (eax >> 31) + eax; // 处理符号位并对结果进行调整
    ebx = (ebx >> 1) + esi;      // 取平均值并加上第二个参数

    if (ebx > edi) {
        // 递归调用，edx = ebx - 1
        return func4(edi, esi, ebx - 1) + ebx;
    } else if (ebx < edi) {
        // 递归调用，esi = ebx + 1
        return func4(edi, ebx + 1, edx) + ebx;
    } else {
        // 终止递归，返回当前的 ebx
        return ebx;
    }
}
```

### 密码分析

- 从 `phase_4` 的逻辑来看，程序期待用户输入两个整数，分别是存储在栈中的两个数值。
- 首先，第一个输入的数字必须小于或等于 14 (`0xe`)。
- 然后，调用 `func4` 递归函数，传递参数 `%edi`、`%esi` 和 `%edx`，并最终希望 `func4` 返回值等于 7。

**密码组合**：

- 第一个输入必须是某个使得 `func4` 最终返回值为 7 的数字，经检验，只有 7 满足要求。
- 第二个输入必须等于 7。

由此可得 `phase_4` 的密码为 `7 7` 。

## phase_5

**函数入口**：

- `push %rbx` 保存寄存器。
- `mov %rdi, %rbx` 把传入的参数（字符串指针）保存到 `%rbx`。

**字符串长度检查**：

- `call 0x1b88 <string_length>` 调用了一个函数来计算输入字符串的长度，并把结果存入 `%eax`。
- `cmp $0x6, %eax` 比较字符串长度是否为6。如果长度不是6，程序跳转到 `0x18fa`，调用 `explode_bomb()` 来引爆炸弹。

**循环分析**：

- 这里的循环通过 `lea 0x6(%rbx), %rdi` 和 `cmp %rdi, %rax` 来确保循环6次（字符串长度为6）。
- 在每次循环中，`movzbl (%rax), %edx` 将字符串中的一个字符（通过 `%rax` 指向的地址）加载到 `%edx`，然后通过 `and $0xf, %edx` 取低4位。`and $0xf` 是将 `%edx` 限制在 0 到 15 的范围内。

**数组查找**：

- `lea 0x1c1f(%rip), %rsi` 将一个数组地址加载到 `%rsi`，此数组存储一些映射值。

  通过查询 `xxd.txt` 可以得到这个储存映射值的数组：

  ```
    00003500: 0200 0000 0a00 0000 0600 0000 0100 0000  ................
    00003510: 0c00 0000 1000 0000 0900 0000 0300 0000  ................
    00003520: 0400 0000 0700 0000 0e00 0000 0500 0000  ................
    00003530: 0b00 0000 0800 0000 0f00 0000 0d00 0000  ................
  ```

    由以上结果可知，查找表如下所示

    | key  | value | key  | value | key  | value | key  | value |
    | :--: | :---: | :--: | :---: | :--: | :---: | :--: | ----- |
    |  0   | 0x02  |  4   | 0x0c  |  8   | 0x04  |  12  | 0x0b  |
    |  1   | 0x0a  |  5   | 0x10  |  9   | 0x07  |  13  | 0x08  |
    |  2   | 0x06  |  6   | 0x09  |  10  | 0x0e  |  14  | 0x0f  |
    |  3   | 0x01  |  7   | 0x03  |  11  | 0x05  |  15  | 0x0d  |

- `add (%rsi,%rdx,4), %ecx` 使用 `%edx` 作为索引，在数组中查找，并将结果加到 `%ecx` 中。

**循环结束检查**：

- 如果循环结束后，`%ecx` 累加的结果不等于53（十进制），则跳转到 `0x1901`，调用 `explode_bomb()` 引爆炸弹。

**由以上分析即可得到可用的密码。** 即一个 6 位字符串，每个字符取低 4 位作为索引在表中的值之和为53。例如：
$$
\begin{aligned}
53 & = 3 + 10 + 6 + 8 + 11 + 15 \\
   & = 3 + a + 6 + 8 + b + f
\end{aligned}
$$
由键值表可得 `3a68bf` 的索引为 `712dce` ，一个可行的字符串为 `gabmln` 。像这样的字符串还有很多。

## phase_6

分析 `phase_6` 函数的汇编代码从而推断密码。

### 数据输入和初始化

```assembly
1928:    49 89 e5              mov    %rsp,%r13
192b:    4c 89 ee              mov    %r13,%rsi
192e:    e8 c5 03 00 00        call   1cf8 <read_six_numbers>
1933:    41 be 01 00 00 00     mov    $0x1,%r14d
```

在这里，程序通过调用`read_six_numbers`函数读取了六个用户输入的数字，这些数字存储在栈中。随后，`r14d`被初始化为 1。`r14d`是用于跟踪循环次数或数字的标记。

```assembly
1946:    41 83 fe 05           cmp    $0x5,%r14d
194a:    0f 8e e2 00 00 00     jle    1a32 <phase_6+0x12a>
1950:    eb 27                 jmp    1979 <phase_6+0x71>
```

程序通过 `cmp $0x5,%r14d` 判断 `r14d` 是否超过 5，以此控制循环次数。每次循环中，程序处理一个用户输入数字并进行后续的比较和操作。

### 循环完成数字合法性检查

在循环中，主要完成数字的范围检查和数字的唯一性检查。

#### 数字范围检查

```assembly
193c:	e9 d4 00 00 00       	jmp    1a15 <phase_6+0x10d>
1941:	e8 86 03 00 00       	call   1ccc <explode_bomb>
1946:	41 83 fe 05          	cmp    $0x5,%r14d
194a:	0f 8e e2 00 00 00    	jle    1a32 <phase_6+0x12a>
```

跳转到 `1a15`，开始主循环。循环体从 `1a15` 到 `1a32`。

```assembly
1a15:	4c 89 ed             	mov    %r13,%rbp
1a18:	41 8b 45 00          	mov    0x0(%r13),%eax
1a1c:	83 e8 01             	sub    $0x1,%eax
1a1f:	83 f8 05             	cmp    $0x5,%eax
1a22:	0f 87 19 ff ff ff    	ja     1941 <phase_6+0x39>
1a28:	41 83 fe 05          	cmp    $0x5,%r14d
1a2c:	0f 8f 3d ff ff ff    	jg     196f <phase_6+0x67>
1a32:	4c 89 f3             	mov    %r14,%rbx
```

将 `r13` 复制到 `rbp`。`r13` 和 `rbp` 都指向当前正在处理的数字。从 `r13` 指向的内存中读取一个数字到 `eax`，将其减 1，然后检查是否大于 5。如果大于 5，则跳转到 `1941`，调用 `explode_bomb` 函数，炸弹爆炸。这意味着每个输入的数字必须小于等于 6。

#### 数字唯一性检查

```assembly
195f:	41 8b 04 9c          	mov    (%r12,%rbx,4),%eax
1963:	39 45 00             	cmp    %eax,0x0(%rbp)
1966:	75 ea                	jne    1952 <phase_6+0x4a>
1968:	e8 5f 03 00 00       	call   1ccc <explode_bomb>
196d:	eb e3                	jmp    1952 <phase_6+0x4a>
```

这是一个嵌套循环，用于检查输入的数字是否唯一。 将 `(r12 + rbx * 4)` 的值（即第 `rbx` 个输入数字）加载到 `eax`。再将 `eax` 与 `(rbp)` 的值（即第 `r14` 个输入数字）比较，如果两个数字相等，并且 `rbx` 不等于 `r14`（意味着找到了重复的数字），则跳转到 `1968`，调用 `explode_bomb`。

### 链表分析

```assembly
197e:    8b 0c b4              mov    (%rsp,%rsi,4),%ecx
1981:    b8 01 00 00 00        mov    $0x1,%eax
1986:    48 8d 15 83 38 00 00  lea    0x3883(%rip),%rdx        # 5210 <node1>
198d:    83 f9 01              cmp    $0x1,%ecx
```

在这一段，程序将输入的数字和一个链表中的数据进行比较。`lea 0x3883(%rip),%rdx` 加载链表的地址，`cmp $0x1,%ecx` 比较当前的数字，如果不满足条件则跳转，最终对链表进行排序。这个 `node1` 则能够提示我们这似乎是个节点，使用十六进制形式多查看一些内容。

```
(gdb) x/32x 0x5210
0x5210 <node1>: 0x0000018f      0x00000001      0x00005220      0x00000000
0x5220 <node2>: 0x000002fb      0x00000002      0x00005230      0x00000000
0x5230 <node3>: 0x0000020a      0x00000003      0x00005240      0x00000000
0x5240 <node4>: 0x0000026f      0x00000004      0x00005250      0x00000000
0x5250 <node5>: 0x000002f7      0x00000005      0x00005110      0x00000000
0x5260 <host_table>:    0x00003148      0x00000000      0x00003162      0x00000000
0x5270 <host_table+16>: 0x0000317c      0x00000000      0x00003195      0x00000000
0x5280 <host_table+32>: 0x00000000      0x00000000      0x00000000      0x00000000
```

通过 `gdb` 检查出现的地址 `5210` 。可以看出，一共有六个节点，这些节点中的第一个数值是节点的值，第二个是节点的索引，第三个是指向下一个节点的指针。由 `node5` 储存的下一个节点的地址可找到 `node6` 。

```
(gdb) x/8x 0x5110
0x5110 <node6>: 0x0000006f      0x00000006      0x00000000      0x00000000
0x5120 <bomb_id>:       0x0000004d      0x00000000      0x00000000      0x00000000
```

- `node1` 地址`0x5210`: 数据`0x18f`, 编号`1`, 下一个`0x5220`
- `node2` 地址`0x5220`: 数据`0x2fb`, 编号`2`, 下一个`0x5230`
- `node3` 地址`0x5230`: 数据`0x20a`, 编号`3`, 下一个`0x5240`
- `node4` 地址`0x5240`: 数据`0x26f`, 编号`4`, 下一个`0x5250`
- `node5` 地址`0x5250`: 数据`0x2f7`, 编号`5`, 下一个`0x5110` 
- `node6` 地址`0x5110`: 数据`0x06f`, 编号`6`, 为最后一个节点。

可发现从大到小节点分别为 `2 5 4 3 1 6`。可推测答案为 `2 5 4 3 1 6` 或 `6 1 3 4 5 2`。经检验发现 `2 5 4 3 1 6` 可以成功。接着检查汇编代码，检验其依据。

### 排序逻辑

```assembly
19f3:	48 8b 5b 08          	mov    0x8(%rbx),%rbx
19f7:	83 ed 01             	sub    $0x1,%ebp
19fa:	74 3e                	je     1a3a <phase_6+0x132>
19fc:	48 8b 43 08          	mov    0x8(%rbx),%rax
1a00:	8b 00                	mov    (%rax),%eax
1a02:	39 03                	cmp    %eax,(%rbx)
1a04:	7d ed                	jge    19f3 <phase_6+0xeb>
1a06:	e8 c1 02 00 00       	call   1ccc <explode_bomb>
1a0b:	eb e6                	jmp    19f3 <phase_6+0xeb>
```

- **`19f3: mov 0x8(%rbx),%rbx`**：从当前链表节点的指针地址（由寄存器 `%rbx` 指向的）中获取下一个节点，并移动 `%rbx` 指向这个下一个节点。

- **`19f7: sub $0x1,%ebp`**：减少计数器 `%ebp` 的值，该计数器用来记录需要检查的剩余节点数。如果 `%ebp` 减少到 0，跳转到完成链表检查的逻辑位置，这意味着链表已经完全遍历。

- **`19fc: mov 0x8(%rbx),%rax`**：获取下一个节点的地址并放入 `%rax`。
- **`1a00: mov (%rax),%eax`**：从下一个节点的地址中获取存储的数据（假设存储在偏移 0 处）并放入 `%eax`。
- **`1a02: cmp %eax,(%rbx)`**：将下一个节点的数据与当前节点的数据进行比较。
- **`1a04: jge 19f3 <phase_6+0xeb>`**：如果当前节点的数据大于等于下一个节点的数据，则继续循环，指向下一个链接节点。
- **`1a06: call 1ccc <explode_bomb>`**：如果比较结果不满足且次序错误（当前节点小于下一个节点的值），调用 `explode_bomb` 函数，这是一个错误处理机制，表示输入不满足此阶段的安全条件。
- **`1a0b: jmp 19f3 <phase_6+0xeb>`**：在数据比较无误时，跳回进行下一次链表节点的检查。

由此可知，`phase_6` 的密码为`2 5 4 3 1 6` 。

## secret_phase

在 `objdump.txt` 中，`phase_6` 之后还有 `func7` 和 `secret_phase` 函数，可知最后还有隐藏密码。

### 触发条件

通过全局搜索 `secret_phase` 可以发现在 `phase_defused` 函数中调用 `secret_phase` 。

```assembly
0000000000001e7f <phase_defused>:
    1e7f:	f3 0f 1e fa          	endbr64
    1e83:	48 83 ec 78          	sub    $0x78,%rsp
    1e87:	64 48 8b 04 25 28 00 	mov    %fs:0x28,%rax
    1e8e:	00 00 
    1e90:	48 89 44 24 68       	mov    %rax,0x68(%rsp)
    1e95:	31 c0                	xor    %eax,%eax
    1e97:	83 3d 52 38 00 00 06 	cmpl   $0x6,0x3852(%rip)        # 56f0 <num_input_strings>
    1e9e:	74 15                	je     1eb5 <phase_defused+0x36>
    1ea0:	48 8b 44 24 68       	mov    0x68(%rsp),%rax
    1ea5:	64 48 2b 04 25 28 00 	sub    %fs:0x28,%rax
    1eac:	00 00 
    1eae:	75 73                	jne    1f23 <phase_defused+0xa4>
    1eb0:	48 83 c4 78          	add    $0x78,%rsp
    1eb4:	c3                   	ret
    1eb5:	48 8d 4c 24 0c       	lea    0xc(%rsp),%rcx
    1eba:	48 8d 54 24 08       	lea    0x8(%rsp),%rdx
    1ebf:	4c 8d 44 24 10       	lea    0x10(%rsp),%r8
    1ec4:	48 8d 35 6d 12 00 00 	lea    0x126d(%rip),%rsi        # 3138 <_IO_stdin_used+0x138>
    1ecb:	48 8d 3d 1e 39 00 00 	lea    0x391e(%rip),%rdi        # 57f0 <input_strings+0xf0>
    1ed2:	e8 39 f4 ff ff       	call   1310 <__isoc99_sscanf@plt>
    1ed7:	83 f8 03             	cmp    $0x3,%eax
    1eda:	74 0e                	je     1eea <phase_defused+0x6b>
    1edc:	48 8d 3d e5 14 00 00 	lea    0x14e5(%rip),%rdi        # 33c8 <_IO_stdin_used+0x3c8>
    1ee3:	e8 48 f3 ff ff       	call   1230 <puts@plt>
    1ee8:	eb b6                	jmp    1ea0 <phase_defused+0x21>
    1eea:	48 8d 7c 24 10       	lea    0x10(%rsp),%rdi
    1eef:	48 8d 35 4b 12 00 00 	lea    0x124b(%rip),%rsi        # 3141 <_IO_stdin_used+0x141>
    1ef6:	e8 ae fc ff ff       	call   1ba9 <strings_not_equal>
    1efb:	85 c0                	test   %eax,%eax
    1efd:	75 dd                	jne    1edc <phase_defused+0x5d>
    1eff:	48 8d 3d 62 14 00 00 	lea    0x1462(%rip),%rdi        # 3368 <_IO_stdin_used+0x368>
    1f06:	e8 25 f3 ff ff       	call   1230 <puts@plt>
    1f0b:	48 8d 3d 7e 14 00 00 	lea    0x147e(%rip),%rdi        # 3390 <_IO_stdin_used+0x390>
    1f12:	e8 19 f3 ff ff       	call   1230 <puts@plt>
    1f17:	b8 00 00 00 00       	mov    $0x0,%eax
    1f1c:	e8 7c fb ff ff       	call   1a9d <secret_phase>
    1f21:	eb b9                	jmp    1edc <phase_defused+0x5d>
    1f23:	e8 38 f3 ff ff       	call   1260 <__stack_chk_fail@plt>
```

对代码中出现的地址使用 `gdb` 查看。

```
(gdb) x/s 0x56f0
0x56f0 <num_input_strings>:     ""
(gdb) x/s 0x3138
0x3138: "%d %d %s"
(gdb) x/s 0x57f0
0x57f0 <input_strings+240>:     ""
(gdb) x/s 0x33c8
0x33c8: "Congratulations! You've defused the bomb!"
(gdb) x/s 0x3141
0x3141: "DrEvil"
(gdb) x/s 0x3368
0x3368: "Curses, you've found the secret phase!"
(gdb) x/s 0x3390
0x3390: "But finding it and solving it are quite different..."
```

注释处的 `num_input_string` 提示我们当输入字符串数为 6 时，通过一系列判断是否调用 `secret_phase`。即在 `phase_6` 调用结束后，才能执行 `1eb5` 后的若干条指令，否则直接停止并 `return` 。注释 `3138` 提示我们输入格式是 两个数字后接一个字符串。在数值 `3141` 后的行调用 `strings_not_equal` 函数，因此可知所求字符串即为 `DrEvil` 。

```assembly
1ed2:	e8 39 f4 ff ff       	call   1310 <__isoc99_sscanf@plt>
1ed7:	83 f8 03             	cmp    $0x3,%eax
1eda:	74 0e                	je     1eea <phase_defused+0x6b>
```

注意到调用 `sscanf` 函数之后检测了是否读取三个参数，而全局搜索发现之前在 `phase_3` 和 `phase_4` 调用 `sscanf` ，结合所需输入的`"%d %d %s"` ，可以推断在 `phase_4` 输入后需要加上 `DrEvil` 可触发隐藏。检测后成立。

### 函数实现

接下来分析密码，回到 `secret_phase` 函数。

#### `func7`

这个函数`fun7`是一个递归函数，用于遍历二叉树，并根据传入的目标值与树节点的值进行比较。当目标值小于节点值时，递归进入左子树，大于时进入右子树。函数通过累加递归层数的方式返回一个整数结果，左子树的返回值按照`2 * rax + 1`的公式计算，右子树返回值加倍。如果遍历到空节点或没有匹配到目标值，则返回-1。转化为 `c++` 代码如下：

```cpp
int fun7(Node *node, int target)
{
    if (node == nullptr) { return -1; }

    if (node->value == target) { return 0; }
    else if (target < node->value) { return 2 * fun7(node->left, target); }
    else { return 2 * fun7(node->right, target) + 1; }
}
```

#### `secret_phase`

##### 输入与范围检查

`secret_phase`函数通过读取用户输入，将输入解析为整数，并使用该整数递归遍历一棵二叉树。如果满足特定条件，则解除“炸弹”，否则触发爆炸。函数首先调用`read_line`函数，读取用户输入的字符串，返回值保存在`%rax`，传递给`%rdi`。然后通过调用 `call strtol` 将输入字符串解析为整数，结果保存在`%eax`，也存入`%ebx`供后续使用。接着检查输入的范围：

```asm
1abb:  83 e8 01            sub    $0x1,%eax
1abe:  3d e8 03 00 00      cmp    $0x3e8,%eax
1ac3:  77 26               ja     1aeb <secret_phase+0x4e>
```
- `sub $0x1, %eax`：将解析出的整数减1。
- `cmp $0x3e8, %eax`：判断输入的值是否大于1000（即原始输入大于1001）。

##### 调用`fun7`遍历二叉树：
```asm
1ac5:  89 de               mov    %ebx,%esi
1ac7:  48 8d 3d 62 36 00 00 lea    0x3662(%rip),%rdi        # 5130 <n1>
1ace:  e8 89 ff ff ff      call   1a5c <fun7>
```
将解析出的整数值存入`%esi`，作为目标值传递给`fun7`。然后加载二叉树的根节点`n1`的地址，传递给`%rdi`。调用`fun7`递归遍历二叉树，返回一个结果到`%eax`。确定 `func7`的返回值为 7 后不触发炸弹爆炸。

通过 `gdb` 可从注释的地址 `0x5130` 找到二叉树的节点，并根据所找到的左右子节点的地址找到所有的节点。发现节点从地址 `0x5010` 开始。

```
(gdb) x/128w 0x5010
0x5010 <n45>:   0x00000028      0x00000000      0x00000000      0x00000000
0x5020 <n45+16>:        0x00000000      0x00000000      0x00000000      0x00000000
0x5030 <n41>:   0x00000001      0x00000000      0x00000000      0x00000000
0x5040 <n41+16>:        0x00000000      0x00000000      0x00000000      0x00000000
0x5050 <n47>:   0x00000063      0x00000000      0x00000000      0x00000000
0x5060 <n47+16>:        0x00000000      0x00000000      0x00000000      0x00000000
0x5070 <n44>:   0x00000023      0x00000000      0x00000000      0x00000000
0x5080 <n44+16>:        0x00000000      0x00000000      0x00000000      0x00000000
0x5090 <n42>:   0x00000007      0x00000000      0x00000000      0x00000000
0x50a0 <n42+16>:        0x00000000      0x00000000      0x00000000      0x00000000
0x50b0 <n43>:   0x00000014      0x00000000      0x00000000      0x00000000
0x50c0 <n43+16>:        0x00000000      0x00000000      0x00000000      0x00000000
0x50d0 <n46>:   0x0000002f      0x00000000      0x00000000      0x00000000
0x50e0 <n46+16>:        0x00000000      0x00000000      0x00000000      0x00000000
0x50f0 <n48>:   0x000003e9      0x00000000      0x00000000      0x00000000
0x5100 <n48+16>:        0x00000000      0x00000000      0x00000000      0x00000000
0x5110 <node6>: 0x0000006f      0x00000006      0x00000000      0x00000000
0x5120 <bomb_id>:       0x0000004d      0x00000000      0x00000000      0x00000000
0x5130 <n1>:    0x00000024      0x00000000      0x00005150      0x00000000
0x5140 <n1+16>: 0x00005170      0x00000000      0x00000000      0x00000000
0x5150 <n21>:   0x00000008      0x00000000      0x000051d0      0x00000000
0x5160 <n21+16>:        0x00005190      0x00000000      0x00000000      0x00000000
0x5170 <n22>:   0x00000032      0x00000000      0x000051b0      0x00000000
0x5180 <n22+16>:        0x000051f0      0x00000000      0x00000000      0x00000000
0x5190 <n32>:   0x00000016      0x00000000      0x000050b0      0x00000000
0x51a0 <n32+16>:        0x00005070      0x00000000      0x00000000      0x00000000
0x51b0 <n33>:   0x0000002d      0x00000000      0x00005010      0x00000000
0x51c0 <n33+16>:        0x000050d0      0x00000000      0x00000000      0x00000000
0x51d0 <n31>:   0x00000006      0x00000000      0x00005030      0x00000000
0x51e0 <n31+16>:        0x00005090      0x00000000      0x00000000      0x00000000
0x51f0 <n34>:   0x0000006b      0x00000000      0x00005050      0x00000000
0x5200 <n34+16>:        0x000050f0      0x00000000      0x00000000      0x00000000
```

构建二叉树如下：

```markdown
        36  
       /  \
      8    50  
     / \   / \
    6  22 45 107  
   / \    /   / \
  1   7  40  99  1001  
       \    \
       20    47
```

#### 推断密码

`secret_phase`函数首先读取用户输入，将其转换为整数后，判断其是否在合法范围内。然后，函数将该值作为参数传递给`fun7`递归遍历二叉树。如果`fun7`返回的结果为7，则输出成功信息并解除炸弹；否则，调用`explode_bomb`函数，触发爆炸。

因此，输入的字符串应使 `func7` 的结果为 7 。遍历 `0 - 1001` 范围内的所有整数，发现只有 1001 符合要求，检验发现可以解开密码。

至此解开所有密码

## 最终密码

`input.txt`：

```
We have to stand with our North Korean allies.
1 2 4 8 16 32
0 x 162
7 7 DrEvil
gablmn
2 5 4 3 1 6
1001

```

输出结果：

```bash
2023010779@ics24:~/bomblab$ ./bomb input.txt 
Welcome to my fiendish little bomb. You have 6 phases with
which to blow yourself up. Have a nice day!
Phase 1 defused. How about the next one?
That's number 2.  Keep going!
Halfway there!
So you got that one.  Try this one.
Good work!  On to the next...
Curses, you've found the secret phase!
But finding it and solving it are quite different...
Wow! You've defused the secret stage!
Congratulations! You've defused the bomb!
```


# 本次编译矩阵的汇编证据

摘录来自本轮二进制的 objdump -d -C；完整文件保留在 results/raw/phase2-toolchain-matrix。

## gpp-4a228b36 / O2 / perf_interval

```text
000000000001f2d0 <std::_Function_handler<void (), lab::perf_interval(lab::Config const&)::{lambda()#1}>::_M_invoke(std::_Any_data const&)>:
   1f2ed:	48 8b 07             	mov    (%rdi),%rax
   1f2f0:	48 85 c9             	test   %rcx,%rcx
   1f2f3:	74 27                	je     1f31c <std::_Function_handler<void (), lab::perf_interval(lab::Config const&)::{lambda()#1}>::_M_invoke(std::_Any_data const&)+0x4c>
   1f2f5:	48 be 2d 7f 95 4c 2d 	movabs $0x5851f42d4c957f2d,%rsi
   1f2fc:	f4 51 58
   1f2ff:	31 d2                	xor    %edx,%edx
   1f301:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)
   1f308:	48 0f af c6          	imul   %rsi,%rax
   1f30c:	48 83 c2 01          	add    $0x1,%rdx
   1f310:	48 83 c0 01          	add    $0x1,%rax
   1f314:	48 39 d1             	cmp    %rdx,%rcx
   1f317:	75 ef                	jne    1f308 <std::_Function_handler<void (), lab::perf_interval(lab::Config const&)::{lambda()#1}>::_M_invoke(std::_Any_data const&)+0x38>
   1f319:	48 89 07             	mov    %rax,(%rdi)
   1f31c:	c3                   	ret
   1f31d:	0f 1f 00             	nopl   (%rax)
   1f320:	48 8b 12             	mov    (%rdx),%rdx
   1f323:	81 3a 65 6d 70 74    	cmpl   $0x74706d65,(%rdx)
   1f329:	75 b6                	jne    1f2e1 <std::_Function_handler<void (), lab::perf_interval(lab::Config const&)::{lambda()#1}>::_M_invoke(std::_Any_data const&)+0x11>
```

## clangpp-92810474 / O2 / perf_interval

```text
0000000000014ed0 <std::_Function_handler<void (), lab::perf_interval(lab::Config const&)::$_0>::_M_invoke(std::_Any_data const&)>:
   14f0f:	72 2c                	jb     14f3d <std::_Function_handler<void (), lab::perf_interval(lab::Config const&)::$_0>::_M_invoke(std::_Any_data const&)+0x6d>
   14f11:	48 83 e6 f8          	and    $0xfffffffffffffff8,%rsi
   14f15:	48 bf 21 3d 41 38 5f 	movabs $0xb59dda5f38413d21,%rdi
   14f1c:	da 9d b5
   14f1f:	49 b8 d8 f4 6e da 78 	movabs $0x9a8b7f78da6ef4d8,%r8
   14f26:	7f 8b 9a
   14f29:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)
   14f30:	48 0f af c7          	imul   %rdi,%rax
   14f34:	4c 01 c0             	add    %r8,%rax
   14f37:	48 83 c6 f8          	add    $0xfffffffffffffff8,%rsi
   14f3b:	75 f3                	jne    14f30 <std::_Function_handler<void (), lab::perf_interval(lab::Config const&)::$_0>::_M_invoke(std::_Any_data const&)+0x60>
   14f3d:	48 85 d2             	test   %rdx,%rdx
   14f40:	74 1a                	je     14f5c <std::_Function_handler<void (), lab::perf_interval(lab::Config const&)::$_0>::_M_invoke(std::_Any_data const&)+0x8c>
   14f42:	48 be 2d 7f 95 4c 2d 	movabs $0x5851f42d4c957f2d,%rsi
   14f49:	f4 51 58
   14f4c:	0f 1f 40 00          	nopl   0x0(%rax)
   14f50:	48 0f af c6          	imul   %rsi,%rax
   14f54:	48 ff c0             	inc    %rax
```

## gpp-4a228b36 / O3 / cache_patterns

```text
0000000000043620 <std::_Function_handler<void (), lab::cache_patterns(lab::Config const&)::{lambda()#1}>::_M_invoke(std::_Any_data const&)>:
   4364e:	66 0f ef c0          	pxor   %xmm0,%xmm0
   43652:	48 d1 e9             	shr    $1,%rcx
   43655:	48 c1 e1 04          	shl    $0x4,%rcx
   43659:	48 01 d1             	add    %rdx,%rcx
   4365c:	0f 1f 40 00          	nopl   0x0(%rax)
   43660:	f3 0f 6f 10          	movdqu (%rax),%xmm2
   43664:	48 83 c0 10          	add    $0x10,%rax
   43668:	66 0f d4 c2          	paddq  %xmm2,%xmm0
   4366c:	48 39 c1             	cmp    %rax,%rcx
   4366f:	75 ef                	jne    43660 <std::_Function_handler<void (), lab::cache_patterns(lab::Config const&)::{lambda()#1}>::_M_invoke(std::_Any_data const&)+0x40>
   43671:	66 0f 6f c8          	movdqa %xmm0,%xmm1
   43675:	66 0f 73 d9 08       	psrldq $0x8,%xmm1
   4367a:	66 0f d4 c1          	paddq  %xmm1,%xmm0
   4367e:	66 48 0f 7e c0       	movq   %xmm0,%rax
   43683:	40 f6 c6 01          	test   $0x1,%sil
   43687:	74 0b                	je     43694 <std::_Function_handler<void (), lab::cache_patterns(lab::Config const&)::{lambda()#1}>::_M_invoke(std::_Any_data const&)+0x74>
   43689:	48 83 e6 fe          	and    $0xfffffffffffffffe,%rsi
   4368d:	48 8d 14 f2          	lea    (%rdx,%rsi,8),%rdx
```

## gpp-4a228b36 / O3-native / cache_patterns

```text
00000000000448b0 <std::_Function_handler<void (), lab::cache_patterns(lab::Config const&)::{lambda()#1}>::_M_invoke(std::_Any_data const&)>:
   448e5:	48 89 f1             	mov    %rsi,%rcx
   448e8:	c5 f9 ef c0          	vpxor  %xmm0,%xmm0,%xmm0
   448ec:	48 c1 e9 02          	shr    $0x2,%rcx
   448f0:	48 c1 e1 05          	shl    $0x5,%rcx
   448f4:	48 01 d1             	add    %rdx,%rcx
   448f7:	66 0f 1f 84 00 00 00 	nopw   0x0(%rax,%rax,1)
   448fe:	00 00
   44900:	c5 fd d4 00          	vpaddq (%rax),%ymm0,%ymm0
   44904:	48 83 c0 20          	add    $0x20,%rax
   44908:	48 39 c8             	cmp    %rcx,%rax
   4490b:	75 f3                	jne    44900 <std::_Function_handler<void (), lab::cache_patterns(lab::Config const&)::{lambda()#1}>::_M_invoke(std::_Any_data const&)+0x50>
   4490d:	c5 f9 6f c8          	vmovdqa %xmm0,%xmm1
   44911:	c4 e3 7d 39 c0 01    	vextracti128 $0x1,%ymm0,%xmm0
   44917:	c5 f1 d4 c0          	vpaddq %xmm0,%xmm1,%xmm0
   4491b:	c5 f1 73 d8 08       	vpsrldq $0x8,%xmm0,%xmm1
   44920:	c5 f9 d4 c1          	vpaddq %xmm1,%xmm0,%xmm0
   44924:	c4 e1 f9 7e c0       	vmovq  %xmm0,%rax
   44929:	40 f6 c6 03          	test   $0x3,%sil
```

## clangpp-92810474 / O2 / cache_patterns

```text
0000000000017180 <std::_Function_handler<void (), lab::cache_patterns(lab::Config const&)::$_0>::_M_invoke(std::_Any_data const&)>:
   171b6:	49 83 e1 fc          	and    $0xfffffffffffffffc,%r9
   171ba:	4a 8d 0c ca          	lea    (%rdx,%r9,8),%rcx
   171be:	66 0f ef c0          	pxor   %xmm0,%xmm0
   171c2:	45 31 c0             	xor    %r8d,%r8d
   171c5:	66 0f ef c9          	pxor   %xmm1,%xmm1
   171c9:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)
   171d0:	f3 42 0f 6f 14 c2    	movdqu (%rdx,%r8,8),%xmm2
   171d6:	66 0f d4 c2          	paddq  %xmm2,%xmm0
   171da:	f3 42 0f 6f 54 c2 10 	movdqu 0x10(%rdx,%r8,8),%xmm2
   171e1:	66 0f d4 ca          	paddq  %xmm2,%xmm1
   171e5:	49 83 c0 04          	add    $0x4,%r8
   171e9:	4d 39 c1             	cmp    %r8,%r9
   171ec:	75 e2                	jne    171d0 <std::_Function_handler<void (), lab::cache_patterns(lab::Config const&)::$_0>::_M_invoke(std::_Any_data const&)+0x50>
   171ee:	66 0f d4 c8          	paddq  %xmm0,%xmm1
   171f2:	66 0f 70 c1 ee       	pshufd $0xee,%xmm1,%xmm0
   171f7:	66 0f d4 c1          	paddq  %xmm1,%xmm0
   171fb:	66 49 0f 7e c0       	movq   %xmm0,%r8
   17200:	4c 39 ce             	cmp    %r9,%rsi
```

## 解释

GCC O2 的 PMU workload 每次循环执行一条递推乘法；Clang O2 将 8 步 unsigned 递推合并成一次乘法和一次加法，循环计数每次减 8。两者保持模 2^64 语义，但机器指令数不同，因此 cycles/source-step 不能解释成同一条乘法的指令延迟。常数已验证为 a^8 mod 2^64 和 sum(a^i, i=0..7) mod 2^64。

streaming 的 SIMD 指令提供向量化证据；native 指令宽度与目标 ISA 相关。吞吐差异还受频率、内存层级及循环代码影响，不能单靠运行时间证明某一种编译变换。

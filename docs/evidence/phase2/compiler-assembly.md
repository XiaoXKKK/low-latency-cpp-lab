# Compiler assembly evidence

从实际GCC构建产物的objdump输出提取。完整文件保留在原始campaign目录，以下只选streaming lambda的invoke入口。

- O0入口仍调用function manager/invoke层，O3入口已直接包含遍历循环，说明这些调用层被内联。
- O3使用XMM `paddq`，循环每次前进16字节；O3-native使用YMM `vpaddq`，每次前进32字节。可观察到SIMD宽度变化。
- 这里的每次2/4个uint64元素来自向量化，不应仅据此声称进行了独立的loop-unroll优化；尾部有标量展开处理。
- O3-LTO的该循环与O3类似；LTO不保证这个kernel进一步加速。
- runtime、代码布局、频率与调用开销也改变；PMU权限拒绝，不能给出实测IPC。

## O0

Source: `results/raw/phase2-complete/compilers/gpp-4a228b36-O0-assembly.txt`

```asm
0000000000030e5e <std::_Function_handler<void (), lab::cache_patterns(lab::Config const&)::{lambda()#1}>::_M_invoke(std::_Any_data const&)>:
   30e5e:	f3 0f 1e fa          	endbr64
   30e62:	55                   	push   %rbp
   30e63:	48 89 e5             	mov    %rsp,%rbp
   30e66:	48 83 ec 10          	sub    $0x10,%rsp
   30e6a:	48 89 7d f8          	mov    %rdi,-0x8(%rbp)
   30e6e:	48 8b 45 f8          	mov    -0x8(%rbp),%rax
   30e72:	48 89 c7             	mov    %rax,%rdi
   30e75:	e8 8a 03 00 00       	call   31204 <std::_Function_base::_Base_manager<lab::cache_patterns(lab::Config const&)::{lambda()#1}>::_M_get_pointer(std::_Any_data const&)>
   30e7a:	48 89 c7             	mov    %rax,%rdi
   30e7d:	e8 ac 03 00 00       	call   3122e <std::enable_if<is_invocable_r_v<void, lab::cache_patterns(lab::Config const&)::{lambda()#1}&>, void>::type std::__invoke_r<void, lab::cache_patterns(lab::Config const&)::{lambda()#1}&>(lab::cache_patterns(lab::Config const&)::{lambda()#1}&)>
   30e82:	90                   	nop
   30e83:	c9                   	leave
   30e84:	c3                   	ret
```

## O3

Source: `results/raw/phase2-complete/compilers/gpp-4a228b36-O3-assembly.txt`

```asm
00000000000422e0 <std::_Function_handler<void (), lab::cache_patterns(lab::Config const&)::{lambda()#1}>::_M_invoke(std::_Any_data const&)>:
   422e0:	f3 0f 1e fa          	endbr64
   422e4:	48 8b 07             	mov    (%rdi),%rax
   422e7:	48 8b 10             	mov    (%rax),%rdx
   422ea:	48 8b 48 08          	mov    0x8(%rax),%rcx
   422ee:	48 39 ca             	cmp    %rcx,%rdx
   422f1:	74 6d                	je     42360 <std::_Function_handler<void (), lab::cache_patterns(lab::Config const&)::{lambda()#1}>::_M_invoke(std::_Any_data const&)+0x80>
   422f3:	48 29 d1             	sub    %rdx,%rcx
   422f6:	48 89 d0             	mov    %rdx,%rax
   422f9:	48 8d 71 f8          	lea    -0x8(%rcx),%rsi
   422fd:	48 c1 ee 03          	shr    $0x3,%rsi
   42301:	48 83 c6 01          	add    $0x1,%rsi
   42305:	48 83 f9 08          	cmp    $0x8,%rcx
   42309:	74 5f                	je     4236a <std::_Function_handler<void (), lab::cache_patterns(lab::Config const&)::{lambda()#1}>::_M_invoke(std::_Any_data const&)+0x8a>
   4230b:	48 89 f1             	mov    %rsi,%rcx
   4230e:	66 0f ef c0          	pxor   %xmm0,%xmm0
   42312:	48 d1 e9             	shr    $1,%rcx
   42315:	48 c1 e1 04          	shl    $0x4,%rcx
   42319:	48 01 d1             	add    %rdx,%rcx
   4231c:	0f 1f 40 00          	nopl   0x0(%rax)
   42320:	f3 0f 6f 10          	movdqu (%rax),%xmm2
   42324:	48 83 c0 10          	add    $0x10,%rax
   42328:	66 0f d4 c2          	paddq  %xmm2,%xmm0
   4232c:	48 39 c1             	cmp    %rax,%rcx
   4232f:	75 ef                	jne    42320 <std::_Function_handler<void (), lab::cache_patterns(lab::Config const&)::{lambda()#1}>::_M_invoke(std::_Any_data const&)+0x40>
   42331:	66 0f 6f c8          	movdqa %xmm0,%xmm1
   42335:	66 0f 73 d9 08       	psrldq $0x8,%xmm1
   4233a:	66 0f d4 c1          	paddq  %xmm1,%xmm0
   4233e:	66 48 0f 7e c0       	movq   %xmm0,%rax
   42343:	40 f6 c6 01          	test   $0x1,%sil
   42347:	74 0b                	je     42354 <std::_Function_handler<void (), lab::cache_patterns(lab::Config const&)::{lambda()#1}>::_M_invoke(std::_Any_data const&)+0x74>
   42349:	48 83 e6 fe          	and    $0xfffffffffffffffe,%rsi
   4234d:	48 8d 14 f2          	lea    (%rdx,%rsi,8),%rdx
   42351:	48 03 02             	add    (%rdx),%rax
   42354:	48 8b 57 08          	mov    0x8(%rdi),%rdx
   42358:	48 89 02             	mov    %rax,(%rdx)
   4235b:	c3                   	ret
   4235c:	0f 1f 40 00          	nopl   0x0(%rax)
   42360:	48 8b 57 08          	mov    0x8(%rdi),%rdx
   42364:	31 c0                	xor    %eax,%eax
   42366:	48 89 02             	mov    %rax,(%rdx)
   42369:	c3                   	ret
   4236a:	31 c0                	xor    %eax,%eax
   4236c:	eb e3                	jmp    42351 <std::_Function_handler<void (), lab::cache_patterns(lab::Config const&)::{lambda()#1}>::_M_invoke(std::_Any_data const&)+0x71>
   4236e:	66 90                	xchg   %ax,%ax
```

## O3-native

Source: `results/raw/phase2-complete/compilers/gpp-4a228b36-O3-native-assembly.txt`

```asm
0000000000043390 <std::_Function_handler<void (), lab::cache_patterns(lab::Config const&)::{lambda()#1}>::_M_invoke(std::_Any_data const&)>:
   43390:	f3 0f 1e fa          	endbr64
   43394:	48 8b 07             	mov    (%rdi),%rax
   43397:	48 8b 10             	mov    (%rax),%rdx
   4339a:	4c 8b 40 08          	mov    0x8(%rax),%r8
   4339e:	4c 39 c2             	cmp    %r8,%rdx
   433a1:	0f 84 a9 00 00 00    	je     43450 <std::_Function_handler<void (), lab::cache_patterns(lab::Config const&)::{lambda()#1}>::_M_invoke(std::_Any_data const&)+0xc0>
   433a7:	49 8d 48 f8          	lea    -0x8(%r8),%rcx
   433ab:	48 89 d0             	mov    %rdx,%rax
   433ae:	48 29 d1             	sub    %rdx,%rcx
   433b1:	48 89 ce             	mov    %rcx,%rsi
   433b4:	48 c1 ee 03          	shr    $0x3,%rsi
   433b8:	48 ff c6             	inc    %rsi
   433bb:	48 83 f9 10          	cmp    $0x10,%rcx
   433bf:	0f 86 95 00 00 00    	jbe    4345a <std::_Function_handler<void (), lab::cache_patterns(lab::Config const&)::{lambda()#1}>::_M_invoke(std::_Any_data const&)+0xca>
   433c5:	48 89 f1             	mov    %rsi,%rcx
   433c8:	c5 f9 ef c0          	vpxor  %xmm0,%xmm0,%xmm0
   433cc:	48 c1 e9 02          	shr    $0x2,%rcx
   433d0:	48 c1 e1 05          	shl    $0x5,%rcx
   433d4:	48 01 d1             	add    %rdx,%rcx
   433d7:	66 0f 1f 84 00 00 00 	nopw   0x0(%rax,%rax,1)
   433de:	00 00
   433e0:	c5 fd d4 00          	vpaddq (%rax),%ymm0,%ymm0
   433e4:	48 83 c0 20          	add    $0x20,%rax
   433e8:	48 39 c8             	cmp    %rcx,%rax
   433eb:	75 f3                	jne    433e0 <std::_Function_handler<void (), lab::cache_patterns(lab::Config const&)::{lambda()#1}>::_M_invoke(std::_Any_data const&)+0x50>
   433ed:	c5 f9 6f c8          	vmovdqa %xmm0,%xmm1
   433f1:	c4 e3 7d 39 c0 01    	vextracti128 $0x1,%ymm0,%xmm0
   433f7:	c5 f1 d4 c0          	vpaddq %xmm0,%xmm1,%xmm0
   433fb:	c5 f1 73 d8 08       	vpsrldq $0x8,%xmm0,%xmm1
   43400:	c5 f9 d4 c1          	vpaddq %xmm1,%xmm0,%xmm0
   43404:	c4 e1 f9 7e c0       	vmovq  %xmm0,%rax
   43409:	40 f6 c6 03          	test   $0x3,%sil
   4340d:	74 31                	je     43440 <std::_Function_handler<void (), lab::cache_patterns(lab::Config const&)::{lambda()#1}>::_M_invoke(std::_Any_data const&)+0xb0>
   4340f:	48 83 e6 fc          	and    $0xfffffffffffffffc,%rsi
   43413:	48 8d 14 f2          	lea    (%rdx,%rsi,8),%rdx
   43417:	c5 f8 77             	vzeroupper
   4341a:	48 8d 4a 08          	lea    0x8(%rdx),%rcx
   4341e:	48 03 02             	add    (%rdx),%rax
   43421:	49 39 c8             	cmp    %rcx,%r8
   43424:	74 11                	je     43437 <std::_Function_handler<void (), lab::cache_patterns(lab::Config const&)::{lambda()#1}>::_M_invoke(std::_Any_data const&)+0xa7>
   43426:	48 8d 4a 10          	lea    0x10(%rdx),%rcx
   4342a:	48 03 42 08          	add    0x8(%rdx),%rax
   4342e:	49 39 c8             	cmp    %rcx,%r8
   43431:	74 04                	je     43437 <std::_Function_handler<void (), lab::cache_patterns(lab::Config const&)::{lambda()#1}>::_M_invoke(std::_Any_data const&)+0xa7>
   43433:	48 03 42 10          	add    0x10(%rdx),%rax
   43437:	48 8b 57 08          	mov    0x8(%rdi),%rdx
   4343b:	48 89 02             	mov    %rax,(%rdx)
   4343e:	c3                   	ret
   4343f:	90                   	nop
   43440:	c5 f8 77             	vzeroupper
   43443:	48 8b 57 08          	mov    0x8(%rdi),%rdx
   43447:	48 89 02             	mov    %rax,(%rdx)
   4344a:	c3                   	ret
   4344b:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)
   43450:	48 8b 57 08          	mov    0x8(%rdi),%rdx
   43454:	31 c0                	xor    %eax,%eax
   43456:	48 89 02             	mov    %rax,(%rdx)
   43459:	c3                   	ret
   4345a:	31 c0                	xor    %eax,%eax
   4345c:	eb bc                	jmp    4341a <std::_Function_handler<void (), lab::cache_patterns(lab::Config const&)::{lambda()#1}>::_M_invoke(std::_Any_data const&)+0x8a>
   4345e:	66 90                	xchg   %ax,%ax
```

## O3-lto

Source: `results/raw/phase2-complete/compilers/gpp-4a228b36-O3-lto-assembly.txt`

```asm
000000000001ab40 <std::_Function_handler<void (), lab::cache_patterns(lab::Config const&)::{lambda()#1}>::_M_invoke(std::_Any_data const&)>:
   1ab40:	f3 0f 1e fa          	endbr64
   1ab44:	48 8b 07             	mov    (%rdi),%rax
   1ab47:	48 8b 10             	mov    (%rax),%rdx
   1ab4a:	48 8b 48 08          	mov    0x8(%rax),%rcx
   1ab4e:	48 39 ca             	cmp    %rcx,%rdx
   1ab51:	74 6d                	je     1abc0 <std::_Function_handler<void (), lab::cache_patterns(lab::Config const&)::{lambda()#1}>::_M_invoke(std::_Any_data const&)+0x80>
   1ab53:	48 29 d1             	sub    %rdx,%rcx
   1ab56:	48 89 d0             	mov    %rdx,%rax
   1ab59:	48 8d 71 f8          	lea    -0x8(%rcx),%rsi
   1ab5d:	48 c1 ee 03          	shr    $0x3,%rsi
   1ab61:	48 83 c6 01          	add    $0x1,%rsi
   1ab65:	48 83 f9 08          	cmp    $0x8,%rcx
   1ab69:	74 5f                	je     1abca <std::_Function_handler<void (), lab::cache_patterns(lab::Config const&)::{lambda()#1}>::_M_invoke(std::_Any_data const&)+0x8a>
   1ab6b:	48 89 f1             	mov    %rsi,%rcx
   1ab6e:	66 0f ef c0          	pxor   %xmm0,%xmm0
   1ab72:	48 d1 e9             	shr    $1,%rcx
   1ab75:	48 c1 e1 04          	shl    $0x4,%rcx
   1ab79:	48 01 d1             	add    %rdx,%rcx
   1ab7c:	0f 1f 40 00          	nopl   0x0(%rax)
   1ab80:	f3 0f 6f 10          	movdqu (%rax),%xmm2
   1ab84:	48 83 c0 10          	add    $0x10,%rax
   1ab88:	66 0f d4 c2          	paddq  %xmm2,%xmm0
   1ab8c:	48 39 c1             	cmp    %rax,%rcx
   1ab8f:	75 ef                	jne    1ab80 <std::_Function_handler<void (), lab::cache_patterns(lab::Config const&)::{lambda()#1}>::_M_invoke(std::_Any_data const&)+0x40>
   1ab91:	66 0f 6f c8          	movdqa %xmm0,%xmm1
   1ab95:	66 0f 73 d9 08       	psrldq $0x8,%xmm1
   1ab9a:	66 0f d4 c1          	paddq  %xmm1,%xmm0
   1ab9e:	66 48 0f 7e c0       	movq   %xmm0,%rax
   1aba3:	40 f6 c6 01          	test   $0x1,%sil
   1aba7:	74 0b                	je     1abb4 <std::_Function_handler<void (), lab::cache_patterns(lab::Config const&)::{lambda()#1}>::_M_invoke(std::_Any_data const&)+0x74>
   1aba9:	48 83 e6 fe          	and    $0xfffffffffffffffe,%rsi
   1abad:	48 8d 14 f2          	lea    (%rdx,%rsi,8),%rdx
   1abb1:	48 03 02             	add    (%rdx),%rax
   1abb4:	48 8b 57 08          	mov    0x8(%rdi),%rdx
   1abb8:	48 89 02             	mov    %rax,(%rdx)
   1abbb:	c3                   	ret
   1abbc:	0f 1f 40 00          	nopl   0x0(%rax)
   1abc0:	48 8b 57 08          	mov    0x8(%rdi),%rdx
   1abc4:	31 c0                	xor    %eax,%eax
   1abc6:	48 89 02             	mov    %rax,(%rdx)
   1abc9:	c3                   	ret
   1abca:	31 c0                	xor    %eax,%eax
   1abcc:	eb e3                	jmp    1abb1 <std::_Function_handler<void (), lab::cache_patterns(lab::Config const&)::{lambda()#1}>::_M_invoke(std::_Any_data const&)+0x71>
   1abce:	66 90                	xchg   %ax,%ax
```

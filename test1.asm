
ramfs/test1:     file format elf64-x86-64


Disassembly of section .text:

0000000004000000 <signal_handler>:
 4000000:	48 b8 b0 25 00 04 00 	movabs rax,0x40025b0
 4000007:	00 00 00 
 400000a:	55                   	push   rbp
 400000b:	48 89 e5             	mov    rbp,rsp
 400000e:	41 55                	push   r13
 4000010:	41 89 fd             	mov    r13d,edi
 4000013:	41 54                	push   r12
 4000015:	ff d0                	call   rax
 4000017:	41 89 c4             	mov    r12d,eax
 400001a:	48 b8 78 25 00 04 00 	movabs rax,0x4002578
 4000021:	00 00 00 
 4000024:	ff d0                	call   rax
 4000026:	44 89 e9             	mov    ecx,r13d
 4000029:	44 89 e2             	mov    edx,r12d
 400002c:	41 5c                	pop    r12
 400002e:	48 bf 10 34 00 04 00 	movabs rdi,0x4003410
 4000035:	00 00 00 
 4000038:	89 c6                	mov    esi,eax
 400003a:	41 5d                	pop    r13
 400003c:	31 c0                	xor    eax,eax
 400003e:	49 b8 00 1f 00 04 00 	movabs r8,0x4001f00
 4000045:	00 00 00 
 4000048:	5d                   	pop    rbp
 4000049:	41 ff e0             	jmp    r8
 400004c:	0f 1f 40 00          	nop    DWORD PTR [rax+0x0]

0000000004000050 <sa_sigaction>:
 4000050:	44 8b 46 08          	mov    r8d,DWORD PTR [rsi+0x8]
 4000054:	48 89 d1             	mov    rcx,rdx
 4000057:	31 c0                	xor    eax,eax
 4000059:	48 89 f2             	mov    rdx,rsi
 400005c:	49 b9 00 1f 00 04 00 	movabs r9,0x4001f00
 4000063:	00 00 00 
 4000066:	89 fe                	mov    esi,edi
 4000068:	48 bf c8 34 00 04 00 	movabs rdi,0x40034c8
 400006f:	00 00 00 
 4000072:	41 ff e1             	jmp    r9
 4000075:	66 66 2e 0f 1f 84 00 	data16 cs nop WORD PTR [rax+rax*1+0x0]
 400007c:	00 00 00 00 

0000000004000080 <test>:
 4000080:	55                   	push   rbp
 4000081:	66 0f ef c0          	pxor   xmm0,xmm0
 4000085:	31 d2                	xor    edx,edx
 4000087:	48 b8 00 00 00 04 00 	movabs rax,0x4000000
 400008e:	00 00 00 
 4000091:	48 89 e5             	mov    rbp,rsp
 4000094:	41 55                	push   r13
 4000096:	49 bd 80 1c 00 04 00 	movabs r13,0x4001c80
 400009d:	00 00 00 
 40000a0:	41 54                	push   r12
 40000a2:	48 8d 75 c0          	lea    rsi,[rbp-0x40]
 40000a6:	41 89 fc             	mov    r12d,edi
 40000a9:	bf 0b 00 00 00       	mov    edi,0xb
 40000ae:	53                   	push   rbx
 40000af:	48 bb f0 25 00 04 00 	movabs rbx,0x40025f0
 40000b6:	00 00 00 
 40000b9:	48 83 ec 28          	sub    rsp,0x28
 40000bd:	48 c7 45 d8 00 00 00 	mov    QWORD PTR [rbp-0x28],0x0
 40000c4:	00 
 40000c5:	48 89 45 c0          	mov    QWORD PTR [rbp-0x40],rax
 40000c9:	0f 11 45 c8          	movups XMMWORD PTR [rbp-0x38],xmm0
 40000cd:	ff d3                	call   rbx
 40000cf:	85 c0                	test   eax,eax
 40000d1:	75 61                	jne    4000134 <test+0xb4>
 40000d3:	48 b8 50 00 00 04 00 	movabs rax,0x4000050
 40000da:	00 00 00 
 40000dd:	31 d2                	xor    edx,edx
 40000df:	48 8d 75 c0          	lea    rsi,[rbp-0x40]
 40000e3:	48 c7 45 c0 00 00 00 	mov    QWORD PTR [rbp-0x40],0x0
 40000ea:	00 
 40000eb:	c7 45 d0 40 00 00 00 	mov    DWORD PTR [rbp-0x30],0x40
 40000f2:	bf 1b 00 00 00       	mov    edi,0x1b
 40000f7:	48 89 45 d8          	mov    QWORD PTR [rbp-0x28],rax
 40000fb:	ff d3                	call   rbx
 40000fd:	85 c0                	test   eax,eax
 40000ff:	75 6c                	jne    400016d <test+0xed>
 4000101:	31 d2                	xor    edx,edx
 4000103:	48 8d 75 c0          	lea    rsi,[rbp-0x40]
 4000107:	bf 1c 00 00 00       	mov    edi,0x1c
 400010c:	ff d3                	call   rbx
 400010e:	85 c0                	test   eax,eax
 4000110:	75 48                	jne    400015a <test+0xda>
 4000112:	31 d2                	xor    edx,edx
 4000114:	48 8d 75 c0          	lea    rsi,[rbp-0x40]
 4000118:	bf 16 00 00 00       	mov    edi,0x16
 400011d:	ff d3                	call   rbx
 400011f:	85 c0                	test   eax,eax
 4000121:	75 24                	jne    4000147 <test+0xc7>
 4000123:	48 b8 50 25 00 04 00 	movabs rax,0x4002550
 400012a:	00 00 00 
 400012d:	44 89 e7             	mov    edi,r12d
 4000130:	ff d0                	call   rax
 4000132:	eb fe                	jmp    4000132 <test+0xb2>
 4000134:	48 bf 2e 34 00 04 00 	movabs rdi,0x400342e
 400013b:	00 00 00 
 400013e:	89 c6                	mov    esi,eax
 4000140:	31 c0                	xor    eax,eax
 4000142:	41 ff d5             	call   r13
 4000145:	eb 8c                	jmp    40000d3 <test+0x53>
 4000147:	48 bf 2e 34 00 04 00 	movabs rdi,0x400342e
 400014e:	00 00 00 
 4000151:	89 c6                	mov    esi,eax
 4000153:	31 c0                	xor    eax,eax
 4000155:	41 ff d5             	call   r13
 4000158:	eb c9                	jmp    4000123 <test+0xa3>
 400015a:	48 bf 2e 34 00 04 00 	movabs rdi,0x400342e
 4000161:	00 00 00 
 4000164:	89 c6                	mov    esi,eax
 4000166:	31 c0                	xor    eax,eax
 4000168:	41 ff d5             	call   r13
 400016b:	eb a5                	jmp    4000112 <test+0x92>
 400016d:	48 bf 2e 34 00 04 00 	movabs rdi,0x400342e
 4000174:	00 00 00 
 4000177:	89 c6                	mov    esi,eax
 4000179:	31 c0                	xor    eax,eax
 400017b:	41 ff d5             	call   r13
 400017e:	eb 81                	jmp    4000101 <test+0x81>

0000000004000180 <start>:
 4000180:	e8 3b 25 00 00       	call   40026c0 <main>
 4000185:	48 89 c7             	mov    rdi,rax
 4000188:	e8 e3 23 00 00       	call   4002570 <sys_exit>
 400018d:	eb fe                	jmp    400018d <start+0xd>
 400018f:	90                   	nop

0000000004000190 <_out_buffer>:
 4000190:	48 39 ca             	cmp    rdx,rcx
 4000193:	73 04                	jae    4000199 <_out_buffer+0x9>
 4000195:	40 88 3c 16          	mov    BYTE PTR [rsi+rdx*1],dil
 4000199:	c3                   	ret
 400019a:	66 0f 1f 44 00 00    	nop    WORD PTR [rax+rax*1+0x0]

00000000040001a0 <_out_null>:
 40001a0:	c3                   	ret
 40001a1:	66 66 2e 0f 1f 84 00 	data16 cs nop WORD PTR [rax+rax*1+0x0]
 40001a8:	00 00 00 00 
 40001ac:	0f 1f 40 00          	nop    DWORD PTR [rax+0x0]

00000000040001b0 <_out_rev>:
 40001b0:	55                   	push   rbp
 40001b1:	48 89 e5             	mov    rbp,rsp
 40001b4:	41 57                	push   r15
 40001b6:	41 56                	push   r14
 40001b8:	49 89 ce             	mov    r14,rcx
 40001bb:	41 55                	push   r13
 40001bd:	49 89 f5             	mov    r13,rsi
 40001c0:	41 54                	push   r12
 40001c2:	49 89 d4             	mov    r12,rdx
 40001c5:	53                   	push   rbx
 40001c6:	48 89 fb             	mov    rbx,rdi
 40001c9:	48 83 ec 28          	sub    rsp,0x28
 40001cd:	48 89 55 b8          	mov    QWORD PTR [rbp-0x48],rdx
 40001d1:	4c 89 45 c8          	mov    QWORD PTR [rbp-0x38],r8
 40001d5:	4c 89 4d c0          	mov    QWORD PTR [rbp-0x40],r9
 40001d9:	f6 45 18 03          	test   BYTE PTR [rbp+0x18],0x3
 40001dd:	75 3a                	jne    4000219 <_out_rev+0x69>
 40001df:	48 89 d0             	mov    rax,rdx
 40001e2:	8b 55 10             	mov    edx,DWORD PTR [rbp+0x10]
 40001e5:	49 89 c7             	mov    r15,rax
 40001e8:	49 89 d4             	mov    r12,rdx
 40001eb:	4d 29 cc             	sub    r12,r9
 40001ee:	49 01 c4             	add    r12,rax
 40001f1:	49 39 d1             	cmp    r9,rdx
 40001f4:	0f 83 a6 00 00 00    	jae    40002a0 <_out_rev+0xf0>
 40001fa:	66 0f 1f 44 00 00    	nop    WORD PTR [rax+rax*1+0x0]
 4000200:	4c 89 fa             	mov    rdx,r15
 4000203:	49 83 c7 01          	add    r15,0x1
 4000207:	4c 89 f1             	mov    rcx,r14
 400020a:	4c 89 ee             	mov    rsi,r13
 400020d:	bf 20 00 00 00       	mov    edi,0x20
 4000212:	ff d3                	call   rbx
 4000214:	4d 39 fc             	cmp    r12,r15
 4000217:	75 e7                	jne    4000200 <_out_rev+0x50>
 4000219:	4c 8b 7d c0          	mov    r15,QWORD PTR [rbp-0x40]
 400021d:	4b 8d 04 27          	lea    rax,[r15+r12*1]
 4000221:	48 89 45 c0          	mov    QWORD PTR [rbp-0x40],rax
 4000225:	4d 85 ff             	test   r15,r15
 4000228:	74 2a                	je     4000254 <_out_rev+0xa4>
 400022a:	66 0f 1f 44 00 00    	nop    WORD PTR [rax+rax*1+0x0]
 4000230:	48 8b 55 c0          	mov    rdx,QWORD PTR [rbp-0x40]
 4000234:	48 8b 45 c8          	mov    rax,QWORD PTR [rbp-0x38]
 4000238:	4c 89 f1             	mov    rcx,r14
 400023b:	4c 89 ee             	mov    rsi,r13
 400023e:	49 89 d4             	mov    r12,rdx
 4000241:	4c 29 fa             	sub    rdx,r15
 4000244:	49 83 ef 01          	sub    r15,0x1
 4000248:	42 0f be 3c 38       	movsx  edi,BYTE PTR [rax+r15*1]
 400024d:	ff d3                	call   rbx
 400024f:	4d 85 ff             	test   r15,r15
 4000252:	75 dc                	jne    4000230 <_out_rev+0x80>
 4000254:	f6 45 18 02          	test   BYTE PTR [rbp+0x18],0x2
 4000258:	74 34                	je     400028e <_out_rev+0xde>
 400025a:	8b 45 10             	mov    eax,DWORD PTR [rbp+0x10]
 400025d:	4d 89 e7             	mov    r15,r12
 4000260:	4c 2b 7d b8          	sub    r15,QWORD PTR [rbp-0x48]
 4000264:	48 89 45 c8          	mov    QWORD PTR [rbp-0x38],rax
 4000268:	49 39 c7             	cmp    r15,rax
 400026b:	73 21                	jae    400028e <_out_rev+0xde>
 400026d:	0f 1f 00             	nop    DWORD PTR [rax]
 4000270:	4c 89 e2             	mov    rdx,r12
 4000273:	4c 89 f1             	mov    rcx,r14
 4000276:	4c 89 ee             	mov    rsi,r13
 4000279:	bf 20 00 00 00       	mov    edi,0x20
 400027e:	ff d3                	call   rbx
 4000280:	49 83 c4 01          	add    r12,0x1
 4000284:	49 83 c7 01          	add    r15,0x1
 4000288:	4c 3b 7d c8          	cmp    r15,QWORD PTR [rbp-0x38]
 400028c:	72 e2                	jb     4000270 <_out_rev+0xc0>
 400028e:	48 83 c4 28          	add    rsp,0x28
 4000292:	4c 89 e0             	mov    rax,r12
 4000295:	5b                   	pop    rbx
 4000296:	41 5c                	pop    r12
 4000298:	41 5d                	pop    r13
 400029a:	41 5e                	pop    r14
 400029c:	41 5f                	pop    r15
 400029e:	5d                   	pop    rbp
 400029f:	c3                   	ret
 40002a0:	49 89 c4             	mov    r12,rax
 40002a3:	e9 71 ff ff ff       	jmp    4000219 <_out_rev+0x69>
 40002a8:	0f 1f 84 00 00 00 00 	nop    DWORD PTR [rax+rax*1+0x0]
 40002af:	00 

00000000040002b0 <_ntoa_long>:
 40002b0:	55                   	push   rbp
 40002b1:	4c 8d 15 f8 ff ff ff 	lea    r10,[rip+0xfffffffffffffff8]        # 40002b0 <_ntoa_long>
 40002b8:	49 bb 58 3d 00 00 00 	movabs r11,0x3d58
 40002bf:	00 00 00 
 40002c2:	4d 01 da             	add    r10,r11
 40002c5:	49 89 f3             	mov    r11,rsi
 40002c8:	48 89 e5             	mov    rbp,rsp
 40002cb:	41 57                	push   r15
 40002cd:	41 56                	push   r14
 40002cf:	41 55                	push   r13
 40002d1:	49 89 cd             	mov    r13,rcx
 40002d4:	41 54                	push   r12
 40002d6:	49 89 d4             	mov    r12,rdx
 40002d9:	53                   	push   rbx
 40002da:	48 83 ec 48          	sub    rsp,0x48
 40002de:	8b 45 28             	mov    eax,DWORD PTR [rbp+0x28]
 40002e1:	48 8b 75 10          	mov    rsi,QWORD PTR [rbp+0x10]
 40002e5:	48 89 7d a8          	mov    QWORD PTR [rbp-0x58],rdi
 40002e9:	44 89 4d a0          	mov    DWORD PTR [rbp-0x60],r9d
 40002ed:	89 c3                	mov    ebx,eax
 40002ef:	81 e3 00 04 00 00    	and    ebx,0x400
 40002f5:	89 5d 9c             	mov    DWORD PTR [rbp-0x64],ebx
 40002f8:	4d 85 c0             	test   r8,r8
 40002fb:	0f 85 e7 01 00 00    	jne    40004e8 <_ntoa_long+0x238>
 4000301:	41 89 c6             	mov    r14d,eax
 4000304:	41 83 e6 ef          	and    r14d,0xffffffef
 4000308:	85 db                	test   ebx,ebx
 400030a:	74 5c                	je     4000368 <_ntoa_long+0xb8>
 400030c:	45 31 c9             	xor    r9d,r9d
 400030f:	83 e0 02             	and    eax,0x2
 4000312:	89 45 a4             	mov    DWORD PTR [rbp-0x5c],eax
 4000315:	0f 84 ac 00 00 00    	je     40003c7 <_ntoa_long+0x117>
 400031b:	80 7d a0 00          	cmp    BYTE PTR [rbp-0x60],0x0
 400031f:	0f 84 9a 01 00 00    	je     40004bf <_ntoa_long+0x20f>
 4000325:	42 c6 44 0d b0 2d    	mov    BYTE PTR [rbp+r9*1-0x50],0x2d
 400032b:	49 83 c1 01          	add    r9,0x1
 400032f:	8b 45 20             	mov    eax,DWORD PTR [rbp+0x20]
 4000332:	41 56                	push   r14
 4000334:	4c 8d 45 b0          	lea    r8,[rbp-0x50]
 4000338:	4c 89 e9             	mov    rcx,r13
 400033b:	48 8b 7d a8          	mov    rdi,QWORD PTR [rbp-0x58]
 400033f:	4c 89 e2             	mov    rdx,r12
 4000342:	4c 89 de             	mov    rsi,r11
 4000345:	50                   	push   rax
 4000346:	48 b8 a8 c1 ff ff ff 	movabs rax,0xffffffffffffc1a8
 400034d:	ff ff ff 
 4000350:	4c 01 d0             	add    rax,r10
 4000353:	ff d0                	call   rax
 4000355:	48 8d 65 d8          	lea    rsp,[rbp-0x28]
 4000359:	5b                   	pop    rbx
 400035a:	41 5c                	pop    r12
 400035c:	41 5d                	pop    r13
 400035e:	41 5e                	pop    r14
 4000360:	41 5f                	pop    r15
 4000362:	5d                   	pop    rbp
 4000363:	c3                   	ret
 4000364:	0f 1f 40 00          	nop    DWORD PTR [rax+0x0]
 4000368:	c7 45 a4 00 00 00 00 	mov    DWORD PTR [rbp-0x5c],0x0
 400036f:	44 89 f0             	mov    eax,r14d
 4000372:	48 8d 7d af          	lea    rdi,[rbp-0x51]
 4000376:	83 e0 20             	and    eax,0x20
 4000379:	83 f8 01             	cmp    eax,0x1
 400037c:	19 db                	sbb    ebx,ebx
 400037e:	45 31 c9             	xor    r9d,r9d
 4000381:	83 e3 20             	and    ebx,0x20
 4000384:	83 c3 37             	add    ebx,0x37
 4000387:	eb 0a                	jmp    4000393 <_ntoa_long+0xe3>
 4000389:	0f 1f 80 00 00 00 00 	nop    DWORD PTR [rax+0x0]
 4000390:	49 89 c0             	mov    r8,rax
 4000393:	31 d2                	xor    edx,edx
 4000395:	4c 89 c0             	mov    rax,r8
 4000398:	48 f7 f6             	div    rsi
 400039b:	48 83 fa 09          	cmp    rdx,0x9
 400039f:	44 8d 7a 30          	lea    r15d,[rdx+0x30]
 40003a3:	8d 0c 13             	lea    ecx,[rbx+rdx*1]
 40003a6:	41 0f 46 cf          	cmovbe ecx,r15d
 40003aa:	49 83 c1 01          	add    r9,0x1
 40003ae:	42 88 0c 0f          	mov    BYTE PTR [rdi+r9*1],cl
 40003b2:	49 39 f0             	cmp    r8,rsi
 40003b5:	72 06                	jb     40003bd <_ntoa_long+0x10d>
 40003b7:	49 83 f9 1f          	cmp    r9,0x1f
 40003bb:	76 d3                	jbe    4000390 <_ntoa_long+0xe0>
 40003bd:	41 f6 c6 02          	test   r14b,0x2
 40003c1:	0f 85 a5 00 00 00    	jne    400046c <_ntoa_long+0x1bc>
 40003c7:	8b 45 18             	mov    eax,DWORD PTR [rbp+0x18]
 40003ca:	44 89 f1             	mov    ecx,r14d
 40003cd:	44 8b 45 20          	mov    r8d,DWORD PTR [rbp+0x20]
 40003d1:	83 e1 01             	and    ecx,0x1
 40003d4:	49 83 f9 1f          	cmp    r9,0x1f
 40003d8:	40 0f 96 c7          	setbe  dil
 40003dc:	49 39 c1             	cmp    r9,rax
 40003df:	0f 92 c2             	setb   dl
 40003e2:	21 fa                	and    edx,edi
 40003e4:	45 85 c0             	test   r8d,r8d
 40003e7:	74 27                	je     4000410 <_ntoa_long+0x160>
 40003e9:	85 c9                	test   ecx,ecx
 40003eb:	0f 84 57 01 00 00    	je     4000548 <_ntoa_long+0x298>
 40003f1:	80 7d a0 00          	cmp    BYTE PTR [rbp-0x60],0x0
 40003f5:	75 06                	jne    40003fd <_ntoa_long+0x14d>
 40003f7:	41 f6 c6 0c          	test   r14b,0xc
 40003fb:	74 04                	je     4000401 <_ntoa_long+0x151>
 40003fd:	83 6d 20 01          	sub    DWORD PTR [rbp+0x20],0x1
 4000401:	84 d2                	test   dl,dl
 4000403:	75 0f                	jne    4000414 <_ntoa_long+0x164>
 4000405:	eb 39                	jmp    4000440 <_ntoa_long+0x190>
 4000407:	66 0f 1f 84 00 00 00 	nop    WORD PTR [rax+rax*1+0x0]
 400040e:	00 00 
 4000410:	84 d2                	test   dl,dl
 4000412:	74 20                	je     4000434 <_ntoa_long+0x184>
 4000414:	48 8d 7d af          	lea    rdi,[rbp-0x51]
 4000418:	eb 0c                	jmp    4000426 <_ntoa_long+0x176>
 400041a:	66 0f 1f 44 00 00    	nop    WORD PTR [rax+rax*1+0x0]
 4000420:	49 83 f9 1f          	cmp    r9,0x1f
 4000424:	77 0e                	ja     4000434 <_ntoa_long+0x184>
 4000426:	49 83 c1 01          	add    r9,0x1
 400042a:	42 c6 04 0f 30       	mov    BYTE PTR [rdi+r9*1],0x30
 400042f:	49 39 c1             	cmp    r9,rax
 4000432:	72 ec                	jb     4000420 <_ntoa_long+0x170>
 4000434:	85 c9                	test   ecx,ecx
 4000436:	74 34                	je     400046c <_ntoa_long+0x1bc>
 4000438:	49 83 f9 1f          	cmp    r9,0x1f
 400043c:	40 0f 96 c7          	setbe  dil
 4000440:	8b 45 20             	mov    eax,DWORD PTR [rbp+0x20]
 4000443:	49 39 c1             	cmp    r9,rax
 4000446:	73 24                	jae    400046c <_ntoa_long+0x1bc>
 4000448:	40 84 ff             	test   dil,dil
 400044b:	74 1f                	je     400046c <_ntoa_long+0x1bc>
 400044d:	48 8d 7d af          	lea    rdi,[rbp-0x51]
 4000451:	eb 0b                	jmp    400045e <_ntoa_long+0x1ae>
 4000453:	0f 1f 44 00 00       	nop    DWORD PTR [rax+rax*1+0x0]
 4000458:	49 83 f9 1f          	cmp    r9,0x1f
 400045c:	77 0e                	ja     400046c <_ntoa_long+0x1bc>
 400045e:	49 83 c1 01          	add    r9,0x1
 4000462:	42 c6 04 0f 30       	mov    BYTE PTR [rdi+r9*1],0x30
 4000467:	49 39 c1             	cmp    r9,rax
 400046a:	72 ec                	jb     4000458 <_ntoa_long+0x1a8>
 400046c:	8b 55 a4             	mov    edx,DWORD PTR [rbp-0x5c]
 400046f:	85 d2                	test   edx,edx
 4000471:	74 38                	je     40004ab <_ntoa_long+0x1fb>
 4000473:	8b 45 9c             	mov    eax,DWORD PTR [rbp-0x64]
 4000476:	85 c0                	test   eax,eax
 4000478:	0f 85 92 00 00 00    	jne    4000510 <_ntoa_long+0x260>
 400047e:	4d 85 c9             	test   r9,r9
 4000481:	75 7d                	jne    4000500 <_ntoa_long+0x250>
 4000483:	48 83 fe 10          	cmp    rsi,0x10
 4000487:	0f 84 32 01 00 00    	je     40005bf <_ntoa_long+0x30f>
 400048d:	48 83 fe 02          	cmp    rsi,0x2
 4000491:	0f 85 2f 01 00 00    	jne    40005c6 <_ntoa_long+0x316>
 4000497:	c6 45 b0 62          	mov    BYTE PTR [rbp-0x50],0x62
 400049b:	41 b9 01 00 00 00    	mov    r9d,0x1
 40004a1:	42 c6 44 0d b0 30    	mov    BYTE PTR [rbp+r9*1-0x50],0x30
 40004a7:	49 83 c1 01          	add    r9,0x1
 40004ab:	49 83 f9 20          	cmp    r9,0x20
 40004af:	0f 84 7a fe ff ff    	je     400032f <_ntoa_long+0x7f>
 40004b5:	80 7d a0 00          	cmp    BYTE PTR [rbp-0x60],0x0
 40004b9:	0f 85 66 fe ff ff    	jne    4000325 <_ntoa_long+0x75>
 40004bf:	41 f6 c6 04          	test   r14b,0x4
 40004c3:	0f 85 8f 00 00 00    	jne    4000558 <_ntoa_long+0x2a8>
 40004c9:	41 f6 c6 08          	test   r14b,0x8
 40004cd:	0f 84 5c fe ff ff    	je     400032f <_ntoa_long+0x7f>
 40004d3:	42 c6 44 0d b0 20    	mov    BYTE PTR [rbp+r9*1-0x50],0x20
 40004d9:	49 83 c1 01          	add    r9,0x1
 40004dd:	e9 4d fe ff ff       	jmp    400032f <_ntoa_long+0x7f>
 40004e2:	66 0f 1f 44 00 00    	nop    WORD PTR [rax+rax*1+0x0]
 40004e8:	89 c3                	mov    ebx,eax
 40004ea:	41 89 c6             	mov    r14d,eax
 40004ed:	83 e3 10             	and    ebx,0x10
 40004f0:	89 5d a4             	mov    DWORD PTR [rbp-0x5c],ebx
 40004f3:	e9 77 fe ff ff       	jmp    400036f <_ntoa_long+0xbf>
 40004f8:	0f 1f 84 00 00 00 00 	nop    DWORD PTR [rax+rax*1+0x0]
 40004ff:	00 
 4000500:	8b 45 18             	mov    eax,DWORD PTR [rbp+0x18]
 4000503:	49 39 c1             	cmp    r9,rax
 4000506:	74 68                	je     4000570 <_ntoa_long+0x2c0>
 4000508:	8b 45 20             	mov    eax,DWORD PTR [rbp+0x20]
 400050b:	49 39 c1             	cmp    r9,rax
 400050e:	74 60                	je     4000570 <_ntoa_long+0x2c0>
 4000510:	49 83 f9 1f          	cmp    r9,0x1f
 4000514:	0f 96 c0             	setbe  al
 4000517:	89 c2                	mov    edx,eax
 4000519:	48 83 fe 10          	cmp    rsi,0x10
 400051d:	74 69                	je     4000588 <_ntoa_long+0x2d8>
 400051f:	83 fe 02             	cmp    esi,0x2
 4000522:	75 0e                	jne    4000532 <_ntoa_long+0x282>
 4000524:	84 c0                	test   al,al
 4000526:	74 0a                	je     4000532 <_ntoa_long+0x282>
 4000528:	42 c6 44 0d b0 62    	mov    BYTE PTR [rbp+r9*1-0x50],0x62
 400052e:	49 83 c1 01          	add    r9,0x1
 4000532:	49 83 f9 20          	cmp    r9,0x20
 4000536:	0f 84 f3 fd ff ff    	je     400032f <_ntoa_long+0x7f>
 400053c:	e9 60 ff ff ff       	jmp    40004a1 <_ntoa_long+0x1f1>
 4000541:	0f 1f 80 00 00 00 00 	nop    DWORD PTR [rax+0x0]
 4000548:	84 d2                	test   dl,dl
 400054a:	0f 85 c4 fe ff ff    	jne    4000414 <_ntoa_long+0x164>
 4000550:	e9 17 ff ff ff       	jmp    400046c <_ntoa_long+0x1bc>
 4000555:	0f 1f 00             	nop    DWORD PTR [rax]
 4000558:	42 c6 44 0d b0 2b    	mov    BYTE PTR [rbp+r9*1-0x50],0x2b
 400055e:	49 83 c1 01          	add    r9,0x1
 4000562:	e9 c8 fd ff ff       	jmp    400032f <_ntoa_long+0x7f>
 4000567:	66 0f 1f 84 00 00 00 	nop    WORD PTR [rax+rax*1+0x0]
 400056e:	00 00 
 4000570:	4c 89 c9             	mov    rcx,r9
 4000573:	48 83 e9 01          	sub    rcx,0x1
 4000577:	0f 95 c2             	setne  dl
 400057a:	83 fe 10             	cmp    esi,0x10
 400057d:	0f 94 c0             	sete   al
 4000580:	20 c2                	and    dl,al
 4000582:	74 24                	je     40005a8 <_ntoa_long+0x2f8>
 4000584:	49 83 e9 02          	sub    r9,0x2
 4000588:	44 89 f0             	mov    eax,r14d
 400058b:	83 e0 20             	and    eax,0x20
 400058e:	75 08                	jne    4000598 <_ntoa_long+0x2e8>
 4000590:	84 d2                	test   dl,dl
 4000592:	75 1c                	jne    40005b0 <_ntoa_long+0x300>
 4000594:	85 c0                	test   eax,eax
 4000596:	74 9a                	je     4000532 <_ntoa_long+0x282>
 4000598:	84 d2                	test   dl,dl
 400059a:	74 96                	je     4000532 <_ntoa_long+0x282>
 400059c:	42 c6 44 0d b0 58    	mov    BYTE PTR [rbp+r9*1-0x50],0x58
 40005a2:	49 83 c1 01          	add    r9,0x1
 40005a6:	eb 8a                	jmp    4000532 <_ntoa_long+0x282>
 40005a8:	49 89 c9             	mov    r9,rcx
 40005ab:	e9 60 ff ff ff       	jmp    4000510 <_ntoa_long+0x260>
 40005b0:	42 c6 44 0d b0 78    	mov    BYTE PTR [rbp+r9*1-0x50],0x78
 40005b6:	49 83 c1 01          	add    r9,0x1
 40005ba:	e9 73 ff ff ff       	jmp    4000532 <_ntoa_long+0x282>
 40005bf:	ba 01 00 00 00       	mov    edx,0x1
 40005c4:	eb c2                	jmp    4000588 <_ntoa_long+0x2d8>
 40005c6:	c6 45 b0 30          	mov    BYTE PTR [rbp-0x50],0x30
 40005ca:	41 b9 01 00 00 00    	mov    r9d,0x1
 40005d0:	e9 46 fd ff ff       	jmp    400031b <_ntoa_long+0x6b>
 40005d5:	66 66 2e 0f 1f 84 00 	data16 cs nop WORD PTR [rax+rax*1+0x0]
 40005dc:	00 00 00 00 

00000000040005e0 <_etoa>:
 40005e0:	49 bb 28 3a 00 00 00 	movabs r11,0x3a28
 40005e7:	00 00 00 
 40005ea:	55                   	push   rbp
 40005eb:	48 89 e5             	mov    rbp,rsp
 40005ee:	41 57                	push   r15
 40005f0:	41 56                	push   r14
 40005f2:	49 89 d6             	mov    r14,rdx
 40005f5:	41 55                	push   r13
 40005f7:	49 89 fd             	mov    r13,rdi
 40005fa:	41 54                	push   r12
 40005fc:	45 89 cc             	mov    r12d,r9d
 40005ff:	53                   	push   rbx
 4000600:	48 8d 1d d9 ff ff ff 	lea    rbx,[rip+0xffffffffffffffd9]        # 40005e0 <_etoa>
 4000607:	4c 01 db             	add    rbx,r11
 400060a:	48 83 ec 38          	sub    rsp,0x38
 400060e:	66 0f 2e c0          	ucomisd xmm0,xmm0
 4000612:	44 8b 7d 10          	mov    r15d,DWORD PTR [rbp+0x10]
 4000616:	0f 8a 74 02 00 00    	jp     4000890 <_etoa+0x2b0>
 400061c:	48 b8 e8 f4 ff ff ff 	movabs rax,0xfffffffffffff4e8
 4000623:	ff ff ff 
 4000626:	66 0f 2f 04 18       	comisd xmm0,QWORD PTR [rax+rbx*1]
 400062b:	0f 87 5f 02 00 00    	ja     4000890 <_etoa+0x2b0>
 4000631:	48 b8 f0 f4 ff ff ff 	movabs rax,0xfffffffffffff4f0
 4000638:	ff ff ff 
 400063b:	f2 0f 10 0c 03       	movsd  xmm1,QWORD PTR [rbx+rax*1]
 4000640:	66 0f 2f c8          	comisd xmm1,xmm0
 4000644:	0f 87 46 02 00 00    	ja     4000890 <_etoa+0x2b0>
 400064a:	66 0f ef c9          	pxor   xmm1,xmm1
 400064e:	66 0f 28 e0          	movapd xmm4,xmm0
 4000652:	66 0f 2f c8          	comisd xmm1,xmm0
 4000656:	76 0f                	jbe    4000667 <_etoa+0x87>
 4000658:	48 b8 88 f5 ff ff ff 	movabs rax,0xfffffffffffff588
 400065f:	ff ff ff 
 4000662:	66 0f 57 24 18       	xorpd  xmm4,XMMWORD PTR [rax+rbx*1]
 4000667:	44 89 fa             	mov    edx,r15d
 400066a:	b8 06 00 00 00       	mov    eax,0x6
 400066f:	66 48 0f 7e e7       	movq   rdi,xmm4
 4000674:	81 e2 00 04 00 00    	and    edx,0x400
 400067a:	66 0f ef d2          	pxor   xmm2,xmm2
 400067e:	44 0f 44 c0          	cmove  r8d,eax
 4000682:	48 c1 ef 34          	shr    rdi,0x34
 4000686:	66 48 0f 7e e0       	movq   rax,xmm4
 400068b:	81 e7 ff 07 00 00    	and    edi,0x7ff
 4000691:	81 ef ff 03 00 00    	sub    edi,0x3ff
 4000697:	f2 0f 2a d7          	cvtsi2sd xmm2,edi
 400069b:	48 bf f8 f4 ff ff ff 	movabs rdi,0xfffffffffffff4f8
 40006a2:	ff ff ff 
 40006a5:	f2 0f 59 14 1f       	mulsd  xmm2,QWORD PTR [rdi+rbx*1]
 40006aa:	48 bf 00 f5 ff ff ff 	movabs rdi,0xfffffffffffff500
 40006b1:	ff ff ff 
 40006b4:	f2 0f 58 14 1f       	addsd  xmm2,QWORD PTR [rdi+rbx*1]
 40006b9:	48 bf ff ff ff ff ff 	movabs rdi,0xfffffffffffff
 40006c0:	ff 0f 00 
 40006c3:	48 21 f8             	and    rax,rdi
 40006c6:	48 bf 00 00 00 00 00 	movabs rdi,0x3ff0000000000000
 40006cd:	00 f0 3f 
 40006d0:	48 09 f8             	or     rax,rdi
 40006d3:	48 bf 08 f5 ff ff ff 	movabs rdi,0xfffffffffffff508
 40006da:	ff ff ff 
 40006dd:	66 48 0f 6e c8       	movq   xmm1,rax
 40006e2:	f2 0f 5c 0c 1f       	subsd  xmm1,QWORD PTR [rdi+rbx*1]
 40006e7:	48 b8 10 f5 ff ff ff 	movabs rax,0xfffffffffffff510
 40006ee:	ff ff ff 
 40006f1:	48 bf 28 f5 ff ff ff 	movabs rdi,0xfffffffffffff528
 40006f8:	ff ff ff 
 40006fb:	f2 0f 59 0c 18       	mulsd  xmm1,QWORD PTR [rax+rbx*1]
 4000700:	48 b8 18 f5 ff ff ff 	movabs rax,0xfffffffffffff518
 4000707:	ff ff ff 
 400070a:	f2 0f 58 ca          	addsd  xmm1,xmm2
 400070e:	f2 0f 10 14 18       	movsd  xmm2,QWORD PTR [rax+rbx*1]
 4000713:	48 b8 20 f5 ff ff ff 	movabs rax,0xfffffffffffff520
 400071a:	ff ff ff 
 400071d:	f2 44 0f 2c d1       	cvttsd2si r10d,xmm1
 4000722:	66 0f ef c9          	pxor   xmm1,xmm1
 4000726:	f2 41 0f 2a ca       	cvtsi2sd xmm1,r10d
 400072b:	f2 0f 59 d1          	mulsd  xmm2,xmm1
 400072f:	f2 0f 58 14 18       	addsd  xmm2,QWORD PTR [rax+rbx*1]
 4000734:	f2 0f 59 0c 1f       	mulsd  xmm1,QWORD PTR [rdi+rbx*1]
 4000739:	48 bf 30 f5 ff ff ff 	movabs rdi,0xfffffffffffff530
 4000740:	ff ff ff 
 4000743:	f2 0f 2c c2          	cvttsd2si eax,xmm2
 4000747:	66 0f ef d2          	pxor   xmm2,xmm2
 400074b:	f2 0f 2a d0          	cvtsi2sd xmm2,eax
 400074f:	f2 0f 59 14 1f       	mulsd  xmm2,QWORD PTR [rdi+rbx*1]
 4000754:	05 ff 03 00 00       	add    eax,0x3ff
 4000759:	48 bf 38 f5 ff ff ff 	movabs rdi,0xfffffffffffff538
 4000760:	ff ff ff 
 4000763:	48 c1 e0 34          	shl    rax,0x34
 4000767:	f2 0f 5c ca          	subsd  xmm1,xmm2
 400076b:	66 0f 28 d9          	movapd xmm3,xmm1
 400076f:	66 0f 28 d1          	movapd xmm2,xmm1
 4000773:	f2 0f 59 d9          	mulsd  xmm3,xmm1
 4000777:	f2 0f 58 d1          	addsd  xmm2,xmm1
 400077b:	66 0f 28 fb          	movapd xmm7,xmm3
 400077f:	f2 0f 5e 3c 1f       	divsd  xmm7,QWORD PTR [rdi+rbx*1]
 4000784:	66 0f 28 eb          	movapd xmm5,xmm3
 4000788:	48 bf 40 f5 ff ff ff 	movabs rdi,0xfffffffffffff540
 400078f:	ff ff ff 
 4000792:	f2 0f 10 34 3b       	movsd  xmm6,QWORD PTR [rbx+rdi*1]
 4000797:	48 bf 48 f5 ff ff ff 	movabs rdi,0xfffffffffffff548
 400079e:	ff ff ff 
 40007a1:	f2 0f 58 fe          	addsd  xmm7,xmm6
 40007a5:	f2 0f 5e ef          	divsd  xmm5,xmm7
 40007a9:	66 48 0f 6e f8       	movq   xmm7,rax
 40007ae:	f2 0f 58 2c 1f       	addsd  xmm5,QWORD PTR [rdi+rbx*1]
 40007b3:	48 bf 50 f5 ff ff ff 	movabs rdi,0xfffffffffffff550
 40007ba:	ff ff ff 
 40007bd:	f2 0f 5e dd          	divsd  xmm3,xmm5
 40007c1:	f2 0f 10 2c 3b       	movsd  xmm5,QWORD PTR [rbx+rdi*1]
 40007c6:	48 bf 58 f5 ff ff ff 	movabs rdi,0xfffffffffffff558
 40007cd:	ff ff ff 
 40007d0:	f2 0f 5c e9          	subsd  xmm5,xmm1
 40007d4:	f2 0f 58 dd          	addsd  xmm3,xmm5
 40007d8:	f2 0f 5e d3          	divsd  xmm2,xmm3
 40007dc:	f2 0f 58 14 1f       	addsd  xmm2,QWORD PTR [rdi+rbx*1]
 40007e1:	f2 0f 59 d7          	mulsd  xmm2,xmm7
 40007e5:	66 0f 2f d4          	comisd xmm2,xmm4
 40007e9:	66 0f 6f ca          	movdqa xmm1,xmm2
 40007ed:	0f 87 3d 02 00 00    	ja     4000a30 <_etoa+0x450>
 40007f3:	41 8d 42 63          	lea    eax,[r10+0x63]
 40007f7:	45 31 db             	xor    r11d,r11d
 40007fa:	3d c6 00 00 00       	cmp    eax,0xc6
 40007ff:	41 0f 97 c3          	seta   r11b
 4000803:	41 83 c3 04          	add    r11d,0x4
 4000807:	41 f7 c7 00 08 00 00 	test   r15d,0x800
 400080e:	0f 84 c1 00 00 00    	je     40008d5 <_etoa+0x2f5>
 4000814:	48 b8 60 f5 ff ff ff 	movabs rax,0xfffffffffffff560
 400081b:	ff ff ff 
 400081e:	66 0f 2f 24 18       	comisd xmm4,QWORD PTR [rax+rbx*1]
 4000823:	0f 82 97 00 00 00    	jb     40008c0 <_etoa+0x2e0>
 4000829:	48 b8 68 f5 ff ff ff 	movabs rax,0xfffffffffffff568
 4000830:	ff ff ff 
 4000833:	f2 0f 10 14 03       	movsd  xmm2,QWORD PTR [rbx+rax*1]
 4000838:	66 0f 2f d4          	comisd xmm2,xmm4
 400083c:	0f 86 7e 00 00 00    	jbe    40008c0 <_etoa+0x2e0>
 4000842:	44 89 c0             	mov    eax,r8d
 4000845:	44 89 fa             	mov    edx,r15d
 4000848:	44 29 d0             	sub    eax,r10d
 400084b:	83 e8 01             	sub    eax,0x1
 400084e:	45 39 d0             	cmp    r8d,r10d
 4000851:	41 b8 00 00 00 00    	mov    r8d,0x0
 4000857:	44 0f 4f c0          	cmovg  r8d,eax
 400085b:	80 ce 04             	or     dh,0x4
 400085e:	45 85 e4             	test   r12d,r12d
 4000861:	0f 85 09 02 00 00    	jne    4000a70 <_etoa+0x490>
 4000867:	44 89 f8             	mov    eax,r15d
 400086a:	66 0f ef c9          	pxor   xmm1,xmm1
 400086e:	80 e4 f7             	and    ah,0xf7
 4000871:	80 cc 04             	or     ah,0x4
 4000874:	66 0f 2f c8          	comisd xmm1,xmm0
 4000878:	0f 87 c3 01 00 00    	ja     4000a41 <_etoa+0x461>
 400087e:	89 45 10             	mov    DWORD PTR [rbp+0x10],eax
 4000881:	45 31 c9             	xor    r9d,r9d
 4000884:	66 0f 28 c4          	movapd xmm0,xmm4
 4000888:	eb 0d                	jmp    4000897 <_etoa+0x2b7>
 400088a:	66 0f 1f 44 00 00    	nop    WORD PTR [rax+rax*1+0x0]
 4000890:	44 89 7d 10          	mov    DWORD PTR [rbp+0x10],r15d
 4000894:	45 89 e1             	mov    r9d,r12d
 4000897:	48 8d 65 d8          	lea    rsp,[rbp-0x28]
 400089b:	4c 89 f2             	mov    rdx,r14
 400089e:	4c 89 ef             	mov    rdi,r13
 40008a1:	48 b8 88 ca ff ff ff 	movabs rax,0xffffffffffffca88
 40008a8:	ff ff ff 
 40008ab:	48 01 d8             	add    rax,rbx
 40008ae:	5b                   	pop    rbx
 40008af:	41 5c                	pop    r12
 40008b1:	41 5d                	pop    r13
 40008b3:	41 5e                	pop    r14
 40008b5:	41 5f                	pop    r15
 40008b7:	5d                   	pop    rbp
 40008b8:	ff e0                	jmp    rax
 40008ba:	66 0f 1f 44 00 00    	nop    WORD PTR [rax+rax*1+0x0]
 40008c0:	83 fa 01             	cmp    edx,0x1
 40008c3:	44 89 c2             	mov    edx,r8d
 40008c6:	44 89 c0             	mov    eax,r8d
 40008c9:	83 d2 ff             	adc    edx,0xffffffff
 40008cc:	45 85 c0             	test   r8d,r8d
 40008cf:	0f 45 c2             	cmovne eax,edx
 40008d2:	41 89 c0             	mov    r8d,eax
 40008d5:	44 89 fa             	mov    edx,r15d
 40008d8:	45 89 e1             	mov    r9d,r12d
 40008db:	31 c0                	xor    eax,eax
 40008dd:	c6 45 c8 01          	mov    BYTE PTR [rbp-0x38],0x1
 40008e1:	83 e2 02             	and    edx,0x2
 40008e4:	45 29 d9             	sub    r9d,r11d
 40008e7:	45 39 e3             	cmp    r11d,r12d
 40008ea:	44 0f 43 c8          	cmovae r9d,eax
 40008ee:	83 fa 01             	cmp    edx,0x1
 40008f1:	19 ff                	sbb    edi,edi
 40008f3:	83 c7 01             	add    edi,0x1
 40008f6:	85 d2                	test   edx,edx
 40008f8:	40 88 7d af          	mov    BYTE PTR [rbp-0x51],dil
 40008fc:	44 0f 45 c8          	cmovne r9d,eax
 4000900:	45 85 d2             	test   r10d,r10d
 4000903:	74 04                	je     4000909 <_etoa+0x329>
 4000905:	f2 0f 5e e1          	divsd  xmm4,xmm1
 4000909:	44 89 f8             	mov    eax,r15d
 400090c:	66 0f ef c9          	pxor   xmm1,xmm1
 4000910:	80 e4 f7             	and    ah,0xf7
 4000913:	66 0f 2f c8          	comisd xmm1,xmm0
 4000917:	0f 87 3b 01 00 00    	ja     4000a58 <_etoa+0x478>
 400091d:	48 83 ec 08          	sub    rsp,0x8
 4000921:	48 89 4d b8          	mov    QWORD PTR [rbp-0x48],rcx
 4000925:	4c 89 f2             	mov    rdx,r14
 4000928:	66 0f 28 c4          	movapd xmm0,xmm4
 400092c:	50                   	push   rax
 400092d:	4c 89 ef             	mov    rdi,r13
 4000930:	48 b8 88 ca ff ff ff 	movabs rax,0xffffffffffffca88
 4000937:	ff ff ff 
 400093a:	48 01 d8             	add    rax,rbx
 400093d:	44 89 5d b0          	mov    DWORD PTR [rbp-0x50],r11d
 4000941:	44 89 55 b4          	mov    DWORD PTR [rbp-0x4c],r10d
 4000945:	48 89 75 c0          	mov    QWORD PTR [rbp-0x40],rsi
 4000949:	ff d0                	call   rax
 400094b:	80 7d c8 00          	cmp    BYTE PTR [rbp-0x38],0x0
 400094f:	5a                   	pop    rdx
 4000950:	59                   	pop    rcx
 4000951:	0f 84 c3 00 00 00    	je     4000a1a <_etoa+0x43a>
 4000957:	41 83 e7 20          	and    r15d,0x20
 400095b:	48 8d 70 01          	lea    rsi,[rax+0x1]
 400095f:	48 89 c2             	mov    rdx,rax
 4000962:	48 8b 4d b8          	mov    rcx,QWORD PTR [rbp-0x48]
 4000966:	41 83 ff 01          	cmp    r15d,0x1
 400096a:	48 89 75 c8          	mov    QWORD PTR [rbp-0x38],rsi
 400096e:	48 8b 75 c0          	mov    rsi,QWORD PTR [rbp-0x40]
 4000972:	19 ff                	sbb    edi,edi
 4000974:	83 e7 20             	and    edi,0x20
 4000977:	83 c7 45             	add    edi,0x45
 400097a:	41 ff d5             	call   r13
 400097d:	44 8b 55 b4          	mov    r10d,DWORD PTR [rbp-0x4c]
 4000981:	44 8b 5d b0          	mov    r11d,DWORD PTR [rbp-0x50]
 4000985:	6a 05                	push   0x5
 4000987:	48 8b 4d b8          	mov    rcx,QWORD PTR [rbp-0x48]
 400098b:	48 8b 55 c8          	mov    rdx,QWORD PTR [rbp-0x38]
 400098f:	4c 89 ef             	mov    rdi,r13
 4000992:	44 89 d0             	mov    eax,r10d
 4000995:	41 83 eb 01          	sub    r11d,0x1
 4000999:	48 8b 75 c0          	mov    rsi,QWORD PTR [rbp-0x40]
 400099d:	c1 f8 1f             	sar    eax,0x1f
 40009a0:	41 53                	push   r11
 40009a2:	41 89 c0             	mov    r8d,eax
 40009a5:	6a 00                	push   0x0
 40009a7:	6a 0a                	push   0xa
 40009a9:	45 31 d0             	xor    r8d,r10d
 40009ac:	41 c1 ea 1f          	shr    r10d,0x1f
 40009b0:	41 29 c0             	sub    r8d,eax
 40009b3:	45 89 d1             	mov    r9d,r10d
 40009b6:	48 b8 a8 c2 ff ff ff 	movabs rax,0xffffffffffffc2a8
 40009bd:	ff ff ff 
 40009c0:	48 01 d8             	add    rax,rbx
 40009c3:	4d 63 c0             	movsxd r8,r8d
 40009c6:	ff d0                	call   rax
 40009c8:	48 83 c4 20          	add    rsp,0x20
 40009cc:	80 7d af 00          	cmp    BYTE PTR [rbp-0x51],0x0
 40009d0:	74 48                	je     4000a1a <_etoa+0x43a>
 40009d2:	48 89 c3             	mov    rbx,rax
 40009d5:	4c 29 f3             	sub    rbx,r14
 40009d8:	49 39 dc             	cmp    r12,rbx
 40009db:	76 3d                	jbe    4000a1a <_etoa+0x43a>
 40009dd:	48 8b 75 c0          	mov    rsi,QWORD PTR [rbp-0x40]
 40009e1:	48 8b 4d b8          	mov    rcx,QWORD PTR [rbp-0x48]
 40009e5:	4c 89 65 c8          	mov    QWORD PTR [rbp-0x38],r12
 40009e9:	49 89 c4             	mov    r12,rax
 40009ec:	49 89 f6             	mov    r14,rsi
 40009ef:	49 89 cf             	mov    r15,rcx
 40009f2:	66 0f 1f 44 00 00    	nop    WORD PTR [rax+rax*1+0x0]
 40009f8:	4c 89 e2             	mov    rdx,r12
 40009fb:	4c 89 f9             	mov    rcx,r15
 40009fe:	4c 89 f6             	mov    rsi,r14
 4000a01:	bf 20 00 00 00       	mov    edi,0x20
 4000a06:	41 ff d5             	call   r13
 4000a09:	49 83 c4 01          	add    r12,0x1
 4000a0d:	48 83 c3 01          	add    rbx,0x1
 4000a11:	48 39 5d c8          	cmp    QWORD PTR [rbp-0x38],rbx
 4000a15:	77 e1                	ja     40009f8 <_etoa+0x418>
 4000a17:	4c 89 e0             	mov    rax,r12
 4000a1a:	48 8d 65 d8          	lea    rsp,[rbp-0x28]
 4000a1e:	5b                   	pop    rbx
 4000a1f:	41 5c                	pop    r12
 4000a21:	41 5d                	pop    r13
 4000a23:	41 5e                	pop    r14
 4000a25:	41 5f                	pop    r15
 4000a27:	5d                   	pop    rbp
 4000a28:	c3                   	ret
 4000a29:	0f 1f 80 00 00 00 00 	nop    DWORD PTR [rax+0x0]
 4000a30:	f2 0f 5e d6          	divsd  xmm2,xmm6
 4000a34:	41 83 ea 01          	sub    r10d,0x1
 4000a38:	66 0f 28 ca          	movapd xmm1,xmm2
 4000a3c:	e9 b2 fd ff ff       	jmp    40007f3 <_etoa+0x213>
 4000a41:	c6 45 c8 00          	mov    BYTE PTR [rbp-0x38],0x0
 4000a45:	45 31 c9             	xor    r9d,r9d
 4000a48:	45 31 db             	xor    r11d,r11d
 4000a4b:	41 89 d7             	mov    r15d,edx
 4000a4e:	c6 45 af 00          	mov    BYTE PTR [rbp-0x51],0x0
 4000a52:	45 31 d2             	xor    r10d,r10d
 4000a55:	0f 1f 00             	nop    DWORD PTR [rax]
 4000a58:	48 ba 88 f5 ff ff ff 	movabs rdx,0xfffffffffffff588
 4000a5f:	ff ff ff 
 4000a62:	66 0f 57 24 1a       	xorpd  xmm4,XMMWORD PTR [rdx+rbx*1]
 4000a67:	e9 b1 fe ff ff       	jmp    400091d <_etoa+0x33d>
 4000a6c:	0f 1f 40 00          	nop    DWORD PTR [rax+0x0]
 4000a70:	c6 45 af 00          	mov    BYTE PTR [rbp-0x51],0x0
 4000a74:	41 89 d7             	mov    r15d,edx
 4000a77:	45 89 e1             	mov    r9d,r12d
 4000a7a:	45 31 d2             	xor    r10d,r10d
 4000a7d:	c6 45 c8 00          	mov    BYTE PTR [rbp-0x38],0x0
 4000a81:	45 31 db             	xor    r11d,r11d
 4000a84:	e9 80 fe ff ff       	jmp    4000909 <_etoa+0x329>
 4000a89:	0f 1f 80 00 00 00 00 	nop    DWORD PTR [rax+0x0]

0000000004000a90 <_ftoa>:
 4000a90:	55                   	push   rbp
 4000a91:	4c 8d 15 f8 ff ff ff 	lea    r10,[rip+0xfffffffffffffff8]        # 4000a90 <_ftoa>
 4000a98:	49 bb 78 35 00 00 00 	movabs r11,0x3578
 4000a9f:	00 00 00 
 4000aa2:	4d 01 da             	add    r10,r11
 4000aa5:	49 89 f3             	mov    r11,rsi
 4000aa8:	48 89 e5             	mov    rbp,rsp
 4000aab:	41 57                	push   r15
 4000aad:	41 56                	push   r14
 4000aaf:	41 55                	push   r13
 4000ab1:	49 89 cd             	mov    r13,rcx
 4000ab4:	41 54                	push   r12
 4000ab6:	49 89 d4             	mov    r12,rdx
 4000ab9:	53                   	push   rbx
 4000aba:	48 83 ec 58          	sub    rsp,0x58
 4000abe:	66 0f 2e c0          	ucomisd xmm0,xmm0
 4000ac2:	44 89 4d ac          	mov    DWORD PTR [rbp-0x54],r9d
 4000ac6:	44 8b 75 10          	mov    r14d,DWORD PTR [rbp+0x10]
 4000aca:	0f 8a 02 02 00 00    	jp     4000cd2 <_ftoa+0x242>
 4000ad0:	48 b8 f0 f4 ff ff ff 	movabs rax,0xfffffffffffff4f0
 4000ad7:	ff ff ff 
 4000ada:	f2 41 0f 10 0c 02    	movsd  xmm1,QWORD PTR [r10+rax*1]
 4000ae0:	66 0f 2f c8          	comisd xmm1,xmm0
 4000ae4:	0f 87 a6 02 00 00    	ja     4000d90 <_ftoa+0x300>
 4000aea:	48 b8 e8 f4 ff ff ff 	movabs rax,0xfffffffffffff4e8
 4000af1:	ff ff ff 
 4000af4:	66 42 0f 2f 04 10    	comisd xmm0,QWORD PTR [rax+r10*1]
 4000afa:	0f 87 b0 01 00 00    	ja     4000cb0 <_ftoa+0x220>
 4000b00:	48 ba 70 f5 ff ff ff 	movabs rdx,0xfffffffffffff570
 4000b07:	ff ff ff 
 4000b0a:	66 42 0f 2f 04 12    	comisd xmm0,QWORD PTR [rdx+r10*1]
 4000b10:	0f 87 4a 02 00 00    	ja     4000d60 <_ftoa+0x2d0>
 4000b16:	48 b8 78 f5 ff ff ff 	movabs rax,0xfffffffffffff578
 4000b1d:	ff ff ff 
 4000b20:	f2 41 0f 10 0c 02    	movsd  xmm1,QWORD PTR [r10+rax*1]
 4000b26:	66 0f 2f c8          	comisd xmm1,xmm0
 4000b2a:	0f 87 30 02 00 00    	ja     4000d60 <_ftoa+0x2d0>
 4000b30:	66 0f ef c9          	pxor   xmm1,xmm1
 4000b34:	66 0f 2f c8          	comisd xmm1,xmm0
 4000b38:	0f 87 f2 01 00 00    	ja     4000d30 <_ftoa+0x2a0>
 4000b3e:	c6 45 ab 00          	mov    BYTE PTR [rbp-0x55],0x0
 4000b42:	41 f7 c6 00 04 00 00 	test   r14d,0x400
 4000b49:	0f 84 51 03 00 00    	je     4000ea0 <_ftoa+0x410>
 4000b4f:	45 31 c9             	xor    r9d,r9d
 4000b52:	48 8d 45 af          	lea    rax,[rbp-0x51]
 4000b56:	41 83 f8 09          	cmp    r8d,0x9
 4000b5a:	0f 86 71 04 00 00    	jbe    4000fd1 <_ftoa+0x541>
 4000b60:	49 83 c1 01          	add    r9,0x1
 4000b64:	41 83 e8 01          	sub    r8d,0x1
 4000b68:	42 c6 04 08 30       	mov    BYTE PTR [rax+r9*1],0x30
 4000b6d:	49 83 f9 1f          	cmp    r9,0x1f
 4000b71:	77 06                	ja     4000b79 <_ftoa+0xe9>
 4000b73:	41 83 f8 09          	cmp    r8d,0x9
 4000b77:	77 e7                	ja     4000b60 <_ftoa+0xd0>
 4000b79:	f2 42 0f 10 1c 12    	movsd  xmm3,QWORD PTR [rdx+r10*1]
 4000b7f:	f2 0f 2c c8          	cvttsd2si ecx,xmm0
 4000b83:	66 0f ef d2          	pxor   xmm2,xmm2
 4000b87:	66 0f 28 c8          	movapd xmm1,xmm0
 4000b8b:	48 b8 80 f5 ff ff ff 	movabs rax,0xfffffffffffff580
 4000b92:	ff ff ff 
 4000b95:	f2 0f 2a d1          	cvtsi2sd xmm2,ecx
 4000b99:	f2 0f 5c ca          	subsd  xmm1,xmm2
 4000b9d:	f2 41 0f 10 14 02    	movsd  xmm2,QWORD PTR [r10+rax*1]
 4000ba3:	f2 0f 59 cb          	mulsd  xmm1,xmm3
 4000ba7:	66 0f 2f ca          	comisd xmm1,xmm2
 4000bab:	0f 83 d7 02 00 00    	jae    4000e88 <_ftoa+0x3f8>
 4000bb1:	f2 48 0f 2c d9       	cvttsd2si rbx,xmm1
 4000bb6:	48 85 db             	test   rbx,rbx
 4000bb9:	0f 88 21 03 00 00    	js     4000ee0 <_ftoa+0x450>
 4000bbf:	66 0f ef d2          	pxor   xmm2,xmm2
 4000bc3:	f2 48 0f 2a d3       	cvtsi2sd xmm2,rbx
 4000bc8:	48 b8 20 f5 ff ff ff 	movabs rax,0xfffffffffffff520
 4000bcf:	ff ff ff 
 4000bd2:	f2 0f 5c ca          	subsd  xmm1,xmm2
 4000bd6:	f2 42 0f 10 14 10    	movsd  xmm2,QWORD PTR [rax+r10*1]
 4000bdc:	66 0f 2f ca          	comisd xmm1,xmm2
 4000be0:	0f 86 da 02 00 00    	jbe    4000ec0 <_ftoa+0x430>
 4000be6:	48 83 c3 01          	add    rbx,0x1
 4000bea:	0f 88 c0 03 00 00    	js     4000fb0 <_ftoa+0x520>
 4000bf0:	66 0f ef c9          	pxor   xmm1,xmm1
 4000bf4:	f2 48 0f 2a cb       	cvtsi2sd xmm1,rbx
 4000bf9:	66 0f 2f cb          	comisd xmm1,xmm3
 4000bfd:	72 05                	jb     4000c04 <_ftoa+0x174>
 4000bff:	83 c1 01             	add    ecx,0x1
 4000c02:	31 db                	xor    ebx,ebx
 4000c04:	45 85 c0             	test   r8d,r8d
 4000c07:	0f 85 a3 01 00 00    	jne    4000db0 <_ftoa+0x320>
 4000c0d:	66 0f ef c9          	pxor   xmm1,xmm1
 4000c11:	f2 0f 2a c9          	cvtsi2sd xmm1,ecx
 4000c15:	f2 0f 5c c1          	subsd  xmm0,xmm1
 4000c19:	f2 42 0f 10 0c 10    	movsd  xmm1,QWORD PTR [rax+r10*1]
 4000c1f:	66 0f 2f c8          	comisd xmm1,xmm0
 4000c23:	0f 86 1f 01 00 00    	jbe    4000d48 <_ftoa+0x2b8>
 4000c29:	66 0f 2f c1          	comisd xmm0,xmm1
 4000c2d:	0f 87 15 01 00 00    	ja     4000d48 <_ftoa+0x2b8>
 4000c33:	0f 1f 44 00 00       	nop    DWORD PTR [rax+rax*1+0x0]
 4000c38:	48 8d 55 af          	lea    rdx,[rbp-0x51]
 4000c3c:	0f 1f 40 00          	nop    DWORD PTR [rax+0x0]
 4000c40:	49 83 f9 20          	cmp    r9,0x20
 4000c44:	0f 84 d3 01 00 00    	je     4000e1d <_ftoa+0x38d>
 4000c4a:	48 63 c1             	movsxd rax,ecx
 4000c4d:	89 ce                	mov    esi,ecx
 4000c4f:	49 83 c1 01          	add    r9,0x1
 4000c53:	48 69 c0 67 66 66 66 	imul   rax,rax,0x66666667
 4000c5a:	c1 fe 1f             	sar    esi,0x1f
 4000c5d:	48 c1 f8 22          	sar    rax,0x22
 4000c61:	29 f0                	sub    eax,esi
 4000c63:	8d 34 80             	lea    esi,[rax+rax*4]
 4000c66:	01 f6                	add    esi,esi
 4000c68:	29 f1                	sub    ecx,esi
 4000c6a:	83 c1 30             	add    ecx,0x30
 4000c6d:	42 88 0c 0a          	mov    BYTE PTR [rdx+r9*1],cl
 4000c71:	89 c1                	mov    ecx,eax
 4000c73:	85 c0                	test   eax,eax
 4000c75:	75 c9                	jne    4000c40 <_ftoa+0x1b0>
 4000c77:	44 89 f0             	mov    eax,r14d
 4000c7a:	83 e0 03             	and    eax,0x3
 4000c7d:	83 f8 01             	cmp    eax,0x1
 4000c80:	0f 84 a6 01 00 00    	je     4000e2c <_ftoa+0x39c>
 4000c86:	49 83 f9 1f          	cmp    r9,0x1f
 4000c8a:	77 14                	ja     4000ca0 <_ftoa+0x210>
 4000c8c:	80 7d ab 00          	cmp    BYTE PTR [rbp-0x55],0x0
 4000c90:	0f 84 72 02 00 00    	je     4000f08 <_ftoa+0x478>
 4000c96:	42 c6 44 0d b0 2d    	mov    BYTE PTR [rbp+r9*1-0x50],0x2d
 4000c9c:	49 83 c1 01          	add    r9,0x1
 4000ca0:	8b 45 ac             	mov    eax,DWORD PTR [rbp-0x54]
 4000ca3:	41 56                	push   r14
 4000ca5:	4c 8d 45 b0          	lea    r8,[rbp-0x50]
 4000ca9:	50                   	push   rax
 4000caa:	eb 40                	jmp    4000cec <_ftoa+0x25c>
 4000cac:	0f 1f 40 00          	nop    DWORD PTR [rax+0x0]
 4000cb0:	41 f6 c6 04          	test   r14b,0x4
 4000cb4:	75 62                	jne    4000d18 <_ftoa+0x288>
 4000cb6:	48 b8 58 f4 ff ff ff 	movabs rax,0xfffffffffffff458
 4000cbd:	ff ff ff 
 4000cc0:	41 b9 03 00 00 00    	mov    r9d,0x3
 4000cc6:	4d 8d 04 02          	lea    r8,[r10+rax*1]
 4000cca:	8b 45 ac             	mov    eax,DWORD PTR [rbp-0x54]
 4000ccd:	41 56                	push   r14
 4000ccf:	50                   	push   rax
 4000cd0:	eb 1a                	jmp    4000cec <_ftoa+0x25c>
 4000cd2:	8b 45 ac             	mov    eax,DWORD PTR [rbp-0x54]
 4000cd5:	41 56                	push   r14
 4000cd7:	41 b9 03 00 00 00    	mov    r9d,0x3
 4000cdd:	50                   	push   rax
 4000cde:	48 b8 61 f4 ff ff ff 	movabs rax,0xfffffffffffff461
 4000ce5:	ff ff ff 
 4000ce8:	4d 8d 04 02          	lea    r8,[r10+rax*1]
 4000cec:	48 b8 a8 c1 ff ff ff 	movabs rax,0xffffffffffffc1a8
 4000cf3:	ff ff ff 
 4000cf6:	4c 89 e9             	mov    rcx,r13
 4000cf9:	4c 89 e2             	mov    rdx,r12
 4000cfc:	4c 89 de             	mov    rsi,r11
 4000cff:	4c 01 d0             	add    rax,r10
 4000d02:	ff d0                	call   rax
 4000d04:	5a                   	pop    rdx
 4000d05:	59                   	pop    rcx
 4000d06:	48 8d 65 d8          	lea    rsp,[rbp-0x28]
 4000d0a:	5b                   	pop    rbx
 4000d0b:	41 5c                	pop    r12
 4000d0d:	41 5d                	pop    r13
 4000d0f:	41 5e                	pop    r14
 4000d11:	41 5f                	pop    r15
 4000d13:	5d                   	pop    rbp
 4000d14:	c3                   	ret
 4000d15:	0f 1f 00             	nop    DWORD PTR [rax]
 4000d18:	48 b8 5c f4 ff ff ff 	movabs rax,0xfffffffffffff45c
 4000d1f:	ff ff ff 
 4000d22:	41 b9 04 00 00 00    	mov    r9d,0x4
 4000d28:	4d 8d 04 02          	lea    r8,[r10+rax*1]
 4000d2c:	eb 9c                	jmp    4000cca <_ftoa+0x23a>
 4000d2e:	66 90                	xchg   ax,ax
 4000d30:	f2 0f 5c c8          	subsd  xmm1,xmm0
 4000d34:	c6 45 ab 01          	mov    BYTE PTR [rbp-0x55],0x1
 4000d38:	66 0f 28 c1          	movapd xmm0,xmm1
 4000d3c:	e9 01 fe ff ff       	jmp    4000b42 <_ftoa+0xb2>
 4000d41:	0f 1f 80 00 00 00 00 	nop    DWORD PTR [rax+0x0]
 4000d48:	89 c8                	mov    eax,ecx
 4000d4a:	83 e0 01             	and    eax,0x1
 4000d4d:	83 f8 01             	cmp    eax,0x1
 4000d50:	83 d9 ff             	sbb    ecx,0xffffffff
 4000d53:	e9 e0 fe ff ff       	jmp    4000c38 <_ftoa+0x1a8>
 4000d58:	0f 1f 84 00 00 00 00 	nop    DWORD PTR [rax+rax*1+0x0]
 4000d5f:	00 
 4000d60:	44 89 75 10          	mov    DWORD PTR [rbp+0x10],r14d
 4000d64:	44 8b 4d ac          	mov    r9d,DWORD PTR [rbp-0x54]
 4000d68:	48 8d 65 d8          	lea    rsp,[rbp-0x28]
 4000d6c:	4c 89 e9             	mov    rcx,r13
 4000d6f:	48 b8 d8 c5 ff ff ff 	movabs rax,0xffffffffffffc5d8
 4000d76:	ff ff ff 
 4000d79:	5b                   	pop    rbx
 4000d7a:	4c 89 e2             	mov    rdx,r12
 4000d7d:	4c 89 de             	mov    rsi,r11
 4000d80:	41 5c                	pop    r12
 4000d82:	4c 01 d0             	add    rax,r10
 4000d85:	41 5d                	pop    r13
 4000d87:	41 5e                	pop    r14
 4000d89:	41 5f                	pop    r15
 4000d8b:	5d                   	pop    rbp
 4000d8c:	ff e0                	jmp    rax
 4000d8e:	66 90                	xchg   ax,ax
 4000d90:	8b 45 ac             	mov    eax,DWORD PTR [rbp-0x54]
 4000d93:	41 56                	push   r14
 4000d95:	41 b9 04 00 00 00    	mov    r9d,0x4
 4000d9b:	50                   	push   rax
 4000d9c:	48 b8 65 f4 ff ff ff 	movabs rax,0xfffffffffffff465
 4000da3:	ff ff ff 
 4000da6:	4d 8d 04 02          	lea    r8,[r10+rax*1]
 4000daa:	e9 3d ff ff ff       	jmp    4000cec <_ftoa+0x25c>
 4000daf:	90                   	nop
 4000db0:	48 8d 45 af          	lea    rax,[rbp-0x51]
 4000db4:	43 8d 54 08 e0       	lea    edx,[r8+r9*1-0x20]
 4000db9:	48 89 7d a0          	mov    QWORD PTR [rbp-0x60],rdi
 4000dbd:	4c 89 5d 98          	mov    QWORD PTR [rbp-0x68],r11
 4000dc1:	89 d7                	mov    edi,edx
 4000dc3:	49 89 c3             	mov    r11,rax
 4000dc6:	48 89 45 88          	mov    QWORD PTR [rbp-0x78],rax
 4000dca:	89 4d 94             	mov    DWORD PTR [rbp-0x6c],ecx
 4000dcd:	eb 41                	jmp    4000e10 <_ftoa+0x380>
 4000dcf:	90                   	nop
 4000dd0:	48 b8 cd cc cc cc cc 	movabs rax,0xcccccccccccccccd
 4000dd7:	cc cc cc 
 4000dda:	49 8d 71 01          	lea    rsi,[r9+0x1]
 4000dde:	41 8d 48 ff          	lea    ecx,[r8-0x1]
 4000de2:	48 f7 e3             	mul    rbx
 4000de5:	48 89 d8             	mov    rax,rbx
 4000de8:	48 c1 ea 03          	shr    rdx,0x3
 4000dec:	4c 8d 3c 92          	lea    r15,[rdx+rdx*4]
 4000df0:	4d 01 ff             	add    r15,r15
 4000df3:	4c 29 f8             	sub    rax,r15
 4000df6:	83 c0 30             	add    eax,0x30
 4000df9:	41 88 04 33          	mov    BYTE PTR [r11+rsi*1],al
 4000dfd:	48 83 fb 09          	cmp    rbx,0x9
 4000e01:	0f 86 29 01 00 00    	jbe    4000f30 <_ftoa+0x4a0>
 4000e07:	41 89 c8             	mov    r8d,ecx
 4000e0a:	48 89 d3             	mov    rbx,rdx
 4000e0d:	49 89 f1             	mov    r9,rsi
 4000e10:	41 39 f8             	cmp    r8d,edi
 4000e13:	75 bb                	jne    4000dd0 <_ftoa+0x340>
 4000e15:	48 8b 7d a0          	mov    rdi,QWORD PTR [rbp-0x60]
 4000e19:	4c 8b 5d 98          	mov    r11,QWORD PTR [rbp-0x68]
 4000e1d:	44 89 f0             	mov    eax,r14d
 4000e20:	83 e0 03             	and    eax,0x3
 4000e23:	83 f8 01             	cmp    eax,0x1
 4000e26:	0f 85 74 fe ff ff    	jne    4000ca0 <_ftoa+0x210>
 4000e2c:	8b 75 ac             	mov    esi,DWORD PTR [rbp-0x54]
 4000e2f:	85 f6                	test   esi,esi
 4000e31:	0f 84 4f fe ff ff    	je     4000c86 <_ftoa+0x1f6>
 4000e37:	80 7d ab 00          	cmp    BYTE PTR [rbp-0x55],0x0
 4000e3b:	75 06                	jne    4000e43 <_ftoa+0x3b3>
 4000e3d:	41 f6 c6 0c          	test   r14b,0xc
 4000e41:	74 04                	je     4000e47 <_ftoa+0x3b7>
 4000e43:	83 6d ac 01          	sub    DWORD PTR [rbp-0x54],0x1
 4000e47:	8b 55 ac             	mov    edx,DWORD PTR [rbp-0x54]
 4000e4a:	49 39 d1             	cmp    r9,rdx
 4000e4d:	0f 83 33 fe ff ff    	jae    4000c86 <_ftoa+0x1f6>
 4000e53:	48 8d 45 af          	lea    rax,[rbp-0x51]
 4000e57:	49 83 f9 1f          	cmp    r9,0x1f
 4000e5b:	0f 87 3f fe ff ff    	ja     4000ca0 <_ftoa+0x210>
 4000e61:	0f 1f 80 00 00 00 00 	nop    DWORD PTR [rax+0x0]
 4000e68:	49 83 c1 01          	add    r9,0x1
 4000e6c:	42 c6 04 08 30       	mov    BYTE PTR [rax+r9*1],0x30
 4000e71:	49 39 d1             	cmp    r9,rdx
 4000e74:	0f 84 0c fe ff ff    	je     4000c86 <_ftoa+0x1f6>
 4000e7a:	49 83 f9 20          	cmp    r9,0x20
 4000e7e:	75 e8                	jne    4000e68 <_ftoa+0x3d8>
 4000e80:	e9 1b fe ff ff       	jmp    4000ca0 <_ftoa+0x210>
 4000e85:	0f 1f 00             	nop    DWORD PTR [rax]
 4000e88:	66 0f 28 e1          	movapd xmm4,xmm1
 4000e8c:	f2 0f 5c e2          	subsd  xmm4,xmm2
 4000e90:	f2 48 0f 2c dc       	cvttsd2si rbx,xmm4
 4000e95:	48 0f ba fb 3f       	btc    rbx,0x3f
 4000e9a:	e9 17 fd ff ff       	jmp    4000bb6 <_ftoa+0x126>
 4000e9f:	90                   	nop
 4000ea0:	48 b8 68 f5 ff ff ff 	movabs rax,0xfffffffffffff568
 4000ea7:	ff ff ff 
 4000eaa:	45 31 c9             	xor    r9d,r9d
 4000ead:	41 b8 06 00 00 00    	mov    r8d,0x6
 4000eb3:	f2 41 0f 10 1c 02    	movsd  xmm3,QWORD PTR [r10+rax*1]
 4000eb9:	e9 c1 fc ff ff       	jmp    4000b7f <_ftoa+0xef>
 4000ebe:	66 90                	xchg   ax,ax
 4000ec0:	66 0f 2f d1          	comisd xmm2,xmm1
 4000ec4:	0f 87 3a fd ff ff    	ja     4000c04 <_ftoa+0x174>
 4000eca:	48 85 db             	test   rbx,rbx
 4000ecd:	0f 85 b5 00 00 00    	jne    4000f88 <_ftoa+0x4f8>
 4000ed3:	48 83 c3 01          	add    rbx,0x1
 4000ed7:	e9 28 fd ff ff       	jmp    4000c04 <_ftoa+0x174>
 4000edc:	0f 1f 40 00          	nop    DWORD PTR [rax+0x0]
 4000ee0:	48 89 d8             	mov    rax,rbx
 4000ee3:	48 89 da             	mov    rdx,rbx
 4000ee6:	66 0f ef d2          	pxor   xmm2,xmm2
 4000eea:	48 d1 e8             	shr    rax,1
 4000eed:	83 e2 01             	and    edx,0x1
 4000ef0:	48 09 d0             	or     rax,rdx
 4000ef3:	f2 48 0f 2a d0       	cvtsi2sd xmm2,rax
 4000ef8:	f2 0f 58 d2          	addsd  xmm2,xmm2
 4000efc:	e9 c7 fc ff ff       	jmp    4000bc8 <_ftoa+0x138>
 4000f01:	0f 1f 80 00 00 00 00 	nop    DWORD PTR [rax+0x0]
 4000f08:	41 f6 c6 04          	test   r14b,0x4
 4000f0c:	0f 85 8e 00 00 00    	jne    4000fa0 <_ftoa+0x510>
 4000f12:	41 f6 c6 08          	test   r14b,0x8
 4000f16:	0f 84 84 fd ff ff    	je     4000ca0 <_ftoa+0x210>
 4000f1c:	42 c6 44 0d b0 20    	mov    BYTE PTR [rbp+r9*1-0x50],0x20
 4000f22:	49 83 c1 01          	add    r9,0x1
 4000f26:	e9 75 fd ff ff       	jmp    4000ca0 <_ftoa+0x210>
 4000f2b:	0f 1f 44 00 00       	nop    DWORD PTR [rax+rax*1+0x0]
 4000f30:	89 cb                	mov    ebx,ecx
 4000f32:	48 8b 7d a0          	mov    rdi,QWORD PTR [rbp-0x60]
 4000f36:	8b 4d 94             	mov    ecx,DWORD PTR [rbp-0x6c]
 4000f39:	4c 8b 5d 98          	mov    r11,QWORD PTR [rbp-0x68]
 4000f3d:	48 83 fe 20          	cmp    rsi,0x20
 4000f41:	0f 84 a9 00 00 00    	je     4000ff0 <_ftoa+0x560>
 4000f47:	41 8d 40 fe          	lea    eax,[r8-0x2]
 4000f4b:	48 8b 55 88          	mov    rdx,QWORD PTR [rbp-0x78]
 4000f4f:	49 8d 44 01 02       	lea    rax,[r9+rax*1+0x2]
 4000f54:	85 db                	test   ebx,ebx
 4000f56:	0f 84 8f 00 00 00    	je     4000feb <_ftoa+0x55b>
 4000f5c:	0f 1f 40 00          	nop    DWORD PTR [rax+0x0]
 4000f60:	48 83 c6 01          	add    rsi,0x1
 4000f64:	c6 04 32 30          	mov    BYTE PTR [rdx+rsi*1],0x30
 4000f68:	48 83 fe 20          	cmp    rsi,0x20
 4000f6c:	0f 84 7e 00 00 00    	je     4000ff0 <_ftoa+0x560>
 4000f72:	48 39 c6             	cmp    rsi,rax
 4000f75:	75 e9                	jne    4000f60 <_ftoa+0x4d0>
 4000f77:	c6 44 05 b0 2e       	mov    BYTE PTR [rbp+rax*1-0x50],0x2e
 4000f7c:	4c 8d 48 01          	lea    r9,[rax+0x1]
 4000f80:	e9 b3 fc ff ff       	jmp    4000c38 <_ftoa+0x1a8>
 4000f85:	0f 1f 00             	nop    DWORD PTR [rax]
 4000f88:	f6 c3 01             	test   bl,0x1
 4000f8b:	0f 84 73 fc ff ff    	je     4000c04 <_ftoa+0x174>
 4000f91:	e9 3d ff ff ff       	jmp    4000ed3 <_ftoa+0x443>
 4000f96:	66 2e 0f 1f 84 00 00 	cs nop WORD PTR [rax+rax*1+0x0]
 4000f9d:	00 00 00 
 4000fa0:	42 c6 44 0d b0 2b    	mov    BYTE PTR [rbp+r9*1-0x50],0x2b
 4000fa6:	49 83 c1 01          	add    r9,0x1
 4000faa:	e9 f1 fc ff ff       	jmp    4000ca0 <_ftoa+0x210>
 4000faf:	90                   	nop
 4000fb0:	48 89 da             	mov    rdx,rbx
 4000fb3:	48 89 de             	mov    rsi,rbx
 4000fb6:	66 0f ef c9          	pxor   xmm1,xmm1
 4000fba:	48 d1 ea             	shr    rdx,1
 4000fbd:	83 e6 01             	and    esi,0x1
 4000fc0:	48 09 f2             	or     rdx,rsi
 4000fc3:	f2 48 0f 2a ca       	cvtsi2sd xmm1,rdx
 4000fc8:	f2 0f 58 c9          	addsd  xmm1,xmm1
 4000fcc:	e9 28 fc ff ff       	jmp    4000bf9 <_ftoa+0x169>
 4000fd1:	48 b8 b8 f3 ff ff ff 	movabs rax,0xfffffffffffff3b8
 4000fd8:	ff ff ff 
 4000fdb:	44 89 c2             	mov    edx,r8d
 4000fde:	4c 01 d0             	add    rax,r10
 4000fe1:	f2 0f 10 1c d0       	movsd  xmm3,QWORD PTR [rax+rdx*8]
 4000fe6:	e9 94 fb ff ff       	jmp    4000b7f <_ftoa+0xef>
 4000feb:	48 89 f0             	mov    rax,rsi
 4000fee:	eb 87                	jmp    4000f77 <_ftoa+0x4e7>
 4000ff0:	41 b9 20 00 00 00    	mov    r9d,0x20
 4000ff6:	e9 22 fe ff ff       	jmp    4000e1d <_ftoa+0x38d>
 4000ffb:	0f 1f 44 00 00       	nop    DWORD PTR [rax+rax*1+0x0]

0000000004001000 <_out_fct>:
 4001000:	40 84 ff             	test   dil,dil
 4001003:	74 13                	je     4001018 <_out_fct+0x18>
 4001005:	4c 8b 46 08          	mov    r8,QWORD PTR [rsi+0x8]
 4001009:	48 8b 06             	mov    rax,QWORD PTR [rsi]
 400100c:	40 0f be ff          	movsx  edi,dil
 4001010:	4c 89 c6             	mov    rsi,r8
 4001013:	ff e0                	jmp    rax
 4001015:	0f 1f 00             	nop    DWORD PTR [rax]
 4001018:	c3                   	ret
 4001019:	0f 1f 80 00 00 00 00 	nop    DWORD PTR [rax+0x0]

0000000004001020 <_vsnprintf>:
 4001020:	55                   	push   rbp
 4001021:	48 8d 05 f8 ff ff ff 	lea    rax,[rip+0xfffffffffffffff8]        # 4001020 <_vsnprintf>
 4001028:	49 bb e8 2f 00 00 00 	movabs r11,0x2fe8
 400102f:	00 00 00 
 4001032:	4c 01 d8             	add    rax,r11
 4001035:	48 89 e5             	mov    rbp,rsp
 4001038:	41 57                	push   r15
 400103a:	49 89 cf             	mov    r15,rcx
 400103d:	41 56                	push   r14
 400103f:	49 89 f6             	mov    r14,rsi
 4001042:	41 55                	push   r13
 4001044:	41 54                	push   r12
 4001046:	49 89 d4             	mov    r12,rdx
 4001049:	53                   	push   rbx
 400104a:	48 83 ec 58          	sub    rsp,0x58
 400104e:	48 89 45 b8          	mov    QWORD PTR [rbp-0x48],rax
 4001052:	48 89 7d c8          	mov    QWORD PTR [rbp-0x38],rdi
 4001056:	4c 89 45 c0          	mov    QWORD PTR [rbp-0x40],r8
 400105a:	48 85 f6             	test   rsi,rsi
 400105d:	0f 84 3d 06 00 00    	je     40016a0 <_vsnprintf+0x680>
 4001063:	48 8b 75 b8          	mov    rsi,QWORD PTR [rbp-0x48]
 4001067:	41 0f b6 07          	movzx  eax,BYTE PTR [r15]
 400106b:	45 31 ed             	xor    r13d,r13d
 400106e:	48 ba a8 c2 ff ff ff 	movabs rdx,0xffffffffffffc2a8
 4001075:	ff ff ff 
 4001078:	48 01 d6             	add    rsi,rdx
 400107b:	48 89 75 a0          	mov    QWORD PTR [rbp-0x60],rsi
 400107f:	84 c0                	test   al,al
 4001081:	75 32                	jne    40010b5 <_vsnprintf+0x95>
 4001083:	e9 c3 07 00 00       	jmp    400184b <_vsnprintf+0x82b>
 4001088:	0f 1f 84 00 00 00 00 	nop    DWORD PTR [rax+rax*1+0x0]
 400108f:	00 
 4001090:	49 8d 5d 01          	lea    rbx,[r13+0x1]
 4001094:	0f be f8             	movsx  edi,al
 4001097:	48 8b 45 c8          	mov    rax,QWORD PTR [rbp-0x38]
 400109b:	4c 89 e1             	mov    rcx,r12
 400109e:	4c 89 ea             	mov    rdx,r13
 40010a1:	4c 89 f6             	mov    rsi,r14
 40010a4:	ff d0                	call   rax
 40010a6:	49 89 dd             	mov    r13,rbx
 40010a9:	41 0f b6 07          	movzx  eax,BYTE PTR [r15]
 40010ad:	84 c0                	test   al,al
 40010af:	0f 84 e3 00 00 00    	je     4001198 <_vsnprintf+0x178>
 40010b5:	49 83 c7 01          	add    r15,0x1
 40010b9:	3c 25                	cmp    al,0x25
 40010bb:	75 d3                	jne    4001090 <_vsnprintf+0x70>
 40010bd:	31 c9                	xor    ecx,ecx
 40010bf:	90                   	nop
 40010c0:	41 0f be 3f          	movsx  edi,BYTE PTR [r15]
 40010c4:	49 8d 47 01          	lea    rax,[r15+0x1]
 40010c8:	48 89 c6             	mov    rsi,rax
 40010cb:	8d 57 e0             	lea    edx,[rdi-0x20]
 40010ce:	80 fa 10             	cmp    dl,0x10
 40010d1:	77 1d                	ja     40010f0 <_vsnprintf+0xd0>
 40010d3:	4c 8d 15 26 1f 00 00 	lea    r10,[rip+0x1f26]        # 4003000 <main+0x940>
 40010da:	0f b6 d2             	movzx  edx,dl
 40010dd:	49 8b 1c d2          	mov    rbx,QWORD PTR [r10+rdx*8]
 40010e1:	4c 01 d3             	add    rbx,r10
 40010e4:	ff e3                	jmp    rbx
 40010e6:	66 2e 0f 1f 84 00 00 	cs nop WORD PTR [rax+rax*1+0x0]
 40010ed:	00 00 00 
 40010f0:	8d 57 d0             	lea    edx,[rdi-0x30]
 40010f3:	80 fa 09             	cmp    dl,0x9
 40010f6:	0f 86 24 01 00 00    	jbe    4001220 <_vsnprintf+0x200>
 40010fc:	40 80 ff 2a          	cmp    dil,0x2a
 4001100:	0f 84 52 01 00 00    	je     4001258 <_vsnprintf+0x238>
 4001106:	4c 89 fe             	mov    rsi,r15
 4001109:	45 31 c9             	xor    r9d,r9d
 400110c:	49 89 c7             	mov    r15,rax
 400110f:	45 31 c0             	xor    r8d,r8d
 4001112:	40 80 ff 2e          	cmp    dil,0x2e
 4001116:	0f 84 b4 00 00 00    	je     40011d0 <_vsnprintf+0x1b0>
 400111c:	8d 47 98             	lea    eax,[rdi-0x68]
 400111f:	3c 12                	cmp    al,0x12
 4001121:	77 20                	ja     4001143 <_vsnprintf+0x123>
 4001123:	0f b6 c0             	movzx  eax,al
 4001126:	48 8d 15 5b 1f 00 00 	lea    rdx,[rip+0x1f5b]        # 4003088 <main+0x9c8>
 400112d:	48 03 14 c2          	add    rdx,QWORD PTR [rdx+rax*8]
 4001131:	ff e2                	jmp    rdx
 4001133:	0f 1f 44 00 00       	nop    DWORD PTR [rax+rax*1+0x0]
 4001138:	0f be 7e 01          	movsx  edi,BYTE PTR [rsi+0x1]
 400113c:	80 cd 01             	or     ch,0x1
 400113f:	49 83 c7 01          	add    r15,0x1
 4001143:	8d 47 db             	lea    eax,[rdi-0x25]
 4001146:	3c 53                	cmp    al,0x53
 4001148:	0f 87 42 01 00 00    	ja     4001290 <_vsnprintf+0x270>
 400114e:	48 8d 1d cb 1f 00 00 	lea    rbx,[rip+0x1fcb]        # 4003120 <main+0xa60>
 4001155:	0f b6 c0             	movzx  eax,al
 4001158:	48 8b 34 c3          	mov    rsi,QWORD PTR [rbx+rax*8]
 400115c:	48 01 de             	add    rsi,rbx
 400115f:	ff e6                	jmp    rsi
 4001161:	0f 1f 80 00 00 00 00 	nop    DWORD PTR [rax+0x0]
 4001168:	83 c9 01             	or     ecx,0x1
 400116b:	49 89 c7             	mov    r15,rax
 400116e:	e9 4d ff ff ff       	jmp    40010c0 <_vsnprintf+0xa0>
 4001173:	0f 1f 44 00 00       	nop    DWORD PTR [rax+rax*1+0x0]
 4001178:	83 c9 02             	or     ecx,0x2
 400117b:	eb ee                	jmp    400116b <_vsnprintf+0x14b>
 400117d:	0f 1f 00             	nop    DWORD PTR [rax]
 4001180:	83 c9 04             	or     ecx,0x4
 4001183:	eb e6                	jmp    400116b <_vsnprintf+0x14b>
 4001185:	0f 1f 00             	nop    DWORD PTR [rax]
 4001188:	83 c9 10             	or     ecx,0x10
 400118b:	eb de                	jmp    400116b <_vsnprintf+0x14b>
 400118d:	0f 1f 00             	nop    DWORD PTR [rax]
 4001190:	83 c9 08             	or     ecx,0x8
 4001193:	eb d6                	jmp    400116b <_vsnprintf+0x14b>
 4001195:	0f 1f 00             	nop    DWORD PTR [rax]
 4001198:	45 89 ef             	mov    r15d,r13d
 400119b:	4d 39 ec             	cmp    r12,r13
 400119e:	49 8d 44 24 ff       	lea    rax,[r12-0x1]
 40011a3:	4c 89 e1             	mov    rcx,r12
 40011a6:	4c 89 f6             	mov    rsi,r14
 40011a9:	4c 0f 46 e8          	cmovbe r13,rax
 40011ad:	48 8b 45 c8          	mov    rax,QWORD PTR [rbp-0x38]
 40011b1:	31 ff                	xor    edi,edi
 40011b3:	4c 89 ea             	mov    rdx,r13
 40011b6:	ff d0                	call   rax
 40011b8:	48 8d 65 d8          	lea    rsp,[rbp-0x28]
 40011bc:	44 89 f8             	mov    eax,r15d
 40011bf:	5b                   	pop    rbx
 40011c0:	41 5c                	pop    r12
 40011c2:	41 5d                	pop    r13
 40011c4:	41 5e                	pop    r14
 40011c6:	41 5f                	pop    r15
 40011c8:	5d                   	pop    rbp
 40011c9:	c3                   	ret
 40011ca:	66 0f 1f 44 00 00    	nop    WORD PTR [rax+rax*1+0x0]
 40011d0:	0f be 7e 01          	movsx  edi,BYTE PTR [rsi+0x1]
 40011d4:	80 cd 04             	or     ch,0x4
 40011d7:	8d 47 d0             	lea    eax,[rdi-0x30]
 40011da:	3c 09                	cmp    al,0x9
 40011dc:	76 1a                	jbe    40011f8 <_vsnprintf+0x1d8>
 40011de:	40 80 ff 2a          	cmp    dil,0x2a
 40011e2:	0f 84 d8 04 00 00    	je     40016c0 <_vsnprintf+0x6a0>
 40011e8:	4c 89 fe             	mov    rsi,r15
 40011eb:	49 83 c7 01          	add    r15,0x1
 40011ef:	e9 28 ff ff ff       	jmp    400111c <_vsnprintf+0xfc>
 40011f4:	0f 1f 40 00          	nop    DWORD PTR [rax+0x0]
 40011f8:	4c 89 fa             	mov    rdx,r15
 40011fb:	43 8d 04 80          	lea    eax,[r8+r8*4]
 40011ff:	49 83 c7 01          	add    r15,0x1
 4001203:	44 8d 44 47 d0       	lea    r8d,[rdi+rax*2-0x30]
 4001208:	41 0f be 3f          	movsx  edi,BYTE PTR [r15]
 400120c:	8d 47 d0             	lea    eax,[rdi-0x30]
 400120f:	3c 09                	cmp    al,0x9
 4001211:	76 e5                	jbe    40011f8 <_vsnprintf+0x1d8>
 4001213:	4c 89 fe             	mov    rsi,r15
 4001216:	4c 8d 7a 02          	lea    r15,[rdx+0x2]
 400121a:	e9 fd fe ff ff       	jmp    400111c <_vsnprintf+0xfc>
 400121f:	90                   	nop
 4001220:	45 31 c9             	xor    r9d,r9d
 4001223:	eb 07                	jmp    400122c <_vsnprintf+0x20c>
 4001225:	0f 1f 00             	nop    DWORD PTR [rax]
 4001228:	48 83 c0 01          	add    rax,0x1
 400122c:	43 8d 14 89          	lea    edx,[r9+r9*4]
 4001230:	4d 89 f8             	mov    r8,r15
 4001233:	49 89 c7             	mov    r15,rax
 4001236:	44 8d 4c 57 d0       	lea    r9d,[rdi+rdx*2-0x30]
 400123b:	0f be 38             	movsx  edi,BYTE PTR [rax]
 400123e:	8d 57 d0             	lea    edx,[rdi-0x30]
 4001241:	80 fa 09             	cmp    dl,0x9
 4001244:	76 e2                	jbe    4001228 <_vsnprintf+0x208>
 4001246:	48 89 c6             	mov    rsi,rax
 4001249:	4d 8d 78 02          	lea    r15,[r8+0x2]
 400124d:	e9 bd fe ff ff       	jmp    400110f <_vsnprintf+0xef>
 4001252:	66 0f 1f 44 00 00    	nop    WORD PTR [rax+rax*1+0x0]
 4001258:	48 8b 5d c0          	mov    rbx,QWORD PTR [rbp-0x40]
 400125c:	8b 03                	mov    eax,DWORD PTR [rbx]
 400125e:	83 f8 2f             	cmp    eax,0x2f
 4001261:	77 3d                	ja     40012a0 <_vsnprintf+0x280>
 4001263:	89 c2                	mov    edx,eax
 4001265:	83 c0 08             	add    eax,0x8
 4001268:	48 03 53 10          	add    rdx,QWORD PTR [rbx+0x10]
 400126c:	89 03                	mov    DWORD PTR [rbx],eax
 400126e:	44 8b 0a             	mov    r9d,DWORD PTR [rdx]
 4001271:	45 85 c9             	test   r9d,r9d
 4001274:	79 06                	jns    400127c <_vsnprintf+0x25c>
 4001276:	83 c9 02             	or     ecx,0x2
 4001279:	41 f7 d9             	neg    r9d
 400127c:	41 0f be 7f 01       	movsx  edi,BYTE PTR [r15+0x1]
 4001281:	49 83 c7 02          	add    r15,0x2
 4001285:	e9 85 fe ff ff       	jmp    400110f <_vsnprintf+0xef>
 400128a:	66 0f 1f 44 00 00    	nop    WORD PTR [rax+rax*1+0x0]
 4001290:	49 8d 5d 01          	lea    rbx,[r13+0x1]
 4001294:	e9 fe fd ff ff       	jmp    4001097 <_vsnprintf+0x77>
 4001299:	0f 1f 80 00 00 00 00 	nop    DWORD PTR [rax+0x0]
 40012a0:	48 8b 5d c0          	mov    rbx,QWORD PTR [rbp-0x40]
 40012a4:	48 8b 53 08          	mov    rdx,QWORD PTR [rbx+0x8]
 40012a8:	48 8d 42 08          	lea    rax,[rdx+0x8]
 40012ac:	48 89 43 08          	mov    QWORD PTR [rbx+0x8],rax
 40012b0:	eb bc                	jmp    400126e <_vsnprintf+0x24e>
 40012b2:	66 0f 1f 44 00 00    	nop    WORD PTR [rax+rax*1+0x0]
 40012b8:	89 f8                	mov    eax,edi
 40012ba:	83 e0 df             	and    eax,0xffffffdf
 40012bd:	3c 58                	cmp    al,0x58
 40012bf:	0f 84 a3 04 00 00    	je     4001768 <_vsnprintf+0x748>
 40012c5:	40 80 ff 6f          	cmp    dil,0x6f
 40012c9:	0f 84 1e 07 00 00    	je     40019ed <_vsnprintf+0x9cd>
 40012cf:	40 80 ff 62          	cmp    dil,0x62
 40012d3:	0f 84 af 07 00 00    	je     4001a88 <_vsnprintf+0xa68>
 40012d9:	83 e1 ef             	and    ecx,0xffffffef
 40012dc:	be 0a 00 00 00       	mov    esi,0xa
 40012e1:	40 80 ff 69          	cmp    dil,0x69
 40012e5:	74 0a                	je     40012f1 <_vsnprintf+0x2d1>
 40012e7:	40 80 ff 64          	cmp    dil,0x64
 40012eb:	0f 85 89 04 00 00    	jne    400177a <_vsnprintf+0x75a>
 40012f1:	89 c8                	mov    eax,ecx
 40012f3:	83 e0 fe             	and    eax,0xfffffffe
 40012f6:	f6 c5 04             	test   ch,0x4
 40012f9:	0f 45 c8             	cmovne ecx,eax
 40012fc:	48 8b 45 c0          	mov    rax,QWORD PTR [rbp-0x40]
 4001300:	89 ca                	mov    edx,ecx
 4001302:	8b 00                	mov    eax,DWORD PTR [rax]
 4001304:	81 e2 00 02 00 00    	and    edx,0x200
 400130a:	40 80 ff 69          	cmp    dil,0x69
 400130e:	0f 84 8c 04 00 00    	je     40017a0 <_vsnprintf+0x780>
 4001314:	40 80 ff 64          	cmp    dil,0x64
 4001318:	0f 84 82 04 00 00    	je     40017a0 <_vsnprintf+0x780>
 400131e:	85 d2                	test   edx,edx
 4001320:	0f 85 d1 06 00 00    	jne    40019f7 <_vsnprintf+0x9d7>
 4001326:	f6 c5 01             	test   ch,0x1
 4001329:	0f 85 30 07 00 00    	jne    4001a5f <_vsnprintf+0xa3f>
 400132f:	f6 c1 40             	test   cl,0x40
 4001332:	0f 85 95 06 00 00    	jne    40019cd <_vsnprintf+0x9ad>
 4001338:	f6 c1 80             	test   cl,0x80
 400133b:	0f 84 b2 07 00 00    	je     4001af3 <_vsnprintf+0xad3>
 4001341:	83 f8 2f             	cmp    eax,0x2f
 4001344:	0f 87 9e 08 00 00    	ja     4001be8 <_vsnprintf+0xbc8>
 400134a:	48 8b 5d c0          	mov    rbx,QWORD PTR [rbp-0x40]
 400134e:	89 c2                	mov    edx,eax
 4001350:	83 c0 08             	add    eax,0x8
 4001353:	89 03                	mov    DWORD PTR [rbx],eax
 4001355:	48 03 53 10          	add    rdx,QWORD PTR [rbx+0x10]
 4001359:	0f b7 02             	movzx  eax,WORD PTR [rdx]
 400135c:	51                   	push   rcx
 400135d:	41 51                	push   r9
 400135f:	45 31 c9             	xor    r9d,r9d
 4001362:	41 50                	push   r8
 4001364:	41 89 c0             	mov    r8d,eax
 4001367:	56                   	push   rsi
 4001368:	e9 8b 04 00 00       	jmp    40017f8 <_vsnprintf+0x7d8>
 400136d:	0f be 7e 01          	movsx  edi,BYTE PTR [rsi+0x1]
 4001371:	40 80 ff 6c          	cmp    dil,0x6c
 4001375:	0f 84 b9 04 00 00    	je     4001834 <_vsnprintf+0x814>
 400137b:	80 cd 01             	or     ch,0x1
 400137e:	49 83 c7 01          	add    r15,0x1
 4001382:	e9 bc fd ff ff       	jmp    4001143 <_vsnprintf+0x123>
 4001387:	0f be 7e 01          	movsx  edi,BYTE PTR [rsi+0x1]
 400138b:	40 80 ff 68          	cmp    dil,0x68
 400138f:	0f 84 8f 04 00 00    	je     4001824 <_vsnprintf+0x804>
 4001395:	80 c9 80             	or     cl,0x80
 4001398:	49 83 c7 01          	add    r15,0x1
 400139c:	e9 a2 fd ff ff       	jmp    4001143 <_vsnprintf+0x123>
 40013a1:	89 fa                	mov    edx,edi
 40013a3:	89 c8                	mov    eax,ecx
 40013a5:	48 8b 75 c0          	mov    rsi,QWORD PTR [rbp-0x40]
 40013a9:	83 e2 df             	and    edx,0xffffffdf
 40013ac:	80 cc 08             	or     ah,0x8
 40013af:	80 fa 47             	cmp    dl,0x47
 40013b2:	0f 44 c8             	cmove  ecx,eax
 40013b5:	83 e7 fd             	and    edi,0xfffffffd
 40013b8:	89 c8                	mov    eax,ecx
 40013ba:	83 c8 20             	or     eax,0x20
 40013bd:	40 80 ff 45          	cmp    dil,0x45
 40013c1:	0f 44 c8             	cmove  ecx,eax
 40013c4:	8b 46 04             	mov    eax,DWORD PTR [rsi+0x4]
 40013c7:	3d af 00 00 00       	cmp    eax,0xaf
 40013cc:	0f 87 2e 03 00 00    	ja     4001700 <_vsnprintf+0x6e0>
 40013d2:	89 c2                	mov    edx,eax
 40013d4:	83 c0 10             	add    eax,0x10
 40013d7:	48 03 56 10          	add    rdx,QWORD PTR [rsi+0x10]
 40013db:	89 46 04             	mov    DWORD PTR [rsi+0x4],eax
 40013de:	48 83 ec 08          	sub    rsp,0x8
 40013e2:	48 8b 7d c8          	mov    rdi,QWORD PTR [rbp-0x38]
 40013e6:	4c 89 f6             	mov    rsi,r14
 40013e9:	48 b8 d8 c5 ff ff ff 	movabs rax,0xffffffffffffc5d8
 40013f0:	ff ff ff 
 40013f3:	51                   	push   rcx
 40013f4:	4c 89 e1             	mov    rcx,r12
 40013f7:	f2 0f 10 02          	movsd  xmm0,QWORD PTR [rdx]
 40013fb:	4c 89 ea             	mov    rdx,r13
 40013fe:	48 8b 5d b8          	mov    rbx,QWORD PTR [rbp-0x48]
 4001402:	48 01 d8             	add    rax,rbx
 4001405:	ff d0                	call   rax
 4001407:	5e                   	pop    rsi
 4001408:	5f                   	pop    rdi
 4001409:	49 89 c5             	mov    r13,rax
 400140c:	e9 98 fc ff ff       	jmp    40010a9 <_vsnprintf+0x89>
 4001411:	89 c8                	mov    eax,ecx
 4001413:	48 8b 75 c0          	mov    rsi,QWORD PTR [rbp-0x40]
 4001417:	83 c8 20             	or     eax,0x20
 400141a:	40 80 ff 46          	cmp    dil,0x46
 400141e:	0f 44 c8             	cmove  ecx,eax
 4001421:	8b 46 04             	mov    eax,DWORD PTR [rsi+0x4]
 4001424:	3d af 00 00 00       	cmp    eax,0xaf
 4001429:	0f 87 e9 02 00 00    	ja     4001718 <_vsnprintf+0x6f8>
 400142f:	89 c2                	mov    edx,eax
 4001431:	83 c0 10             	add    eax,0x10
 4001434:	48 03 56 10          	add    rdx,QWORD PTR [rsi+0x10]
 4001438:	89 46 04             	mov    DWORD PTR [rsi+0x4],eax
 400143b:	48 83 ec 08          	sub    rsp,0x8
 400143f:	48 8b 7d c8          	mov    rdi,QWORD PTR [rbp-0x38]
 4001443:	4c 89 f6             	mov    rsi,r14
 4001446:	48 b8 88 ca ff ff ff 	movabs rax,0xffffffffffffca88
 400144d:	ff ff ff 
 4001450:	51                   	push   rcx
 4001451:	4c 89 e1             	mov    rcx,r12
 4001454:	f2 0f 10 02          	movsd  xmm0,QWORD PTR [rdx]
 4001458:	4c 89 ea             	mov    rdx,r13
 400145b:	eb a1                	jmp    40013fe <_vsnprintf+0x3de>
 400145d:	49 8d 5d 01          	lea    rbx,[r13+0x1]
 4001461:	4c 89 ea             	mov    rdx,r13
 4001464:	48 8b 45 c8          	mov    rax,QWORD PTR [rbp-0x38]
 4001468:	4c 89 e1             	mov    rcx,r12
 400146b:	4c 89 f6             	mov    rsi,r14
 400146e:	bf 25 00 00 00       	mov    edi,0x25
 4001473:	49 89 dd             	mov    r13,rbx
 4001476:	ff d0                	call   rax
 4001478:	e9 2c fc ff ff       	jmp    40010a9 <_vsnprintf+0x89>
 400147d:	48 8b 75 c0          	mov    rsi,QWORD PTR [rbp-0x40]
 4001481:	8b 06                	mov    eax,DWORD PTR [rsi]
 4001483:	83 f8 2f             	cmp    eax,0x2f
 4001486:	0f 87 a4 02 00 00    	ja     4001730 <_vsnprintf+0x710>
 400148c:	89 c2                	mov    edx,eax
 400148e:	83 c0 08             	add    eax,0x8
 4001491:	48 03 56 10          	add    rdx,QWORD PTR [rsi+0x10]
 4001495:	89 06                	mov    DWORD PTR [rsi],eax
 4001497:	48 8b 1a             	mov    rbx,QWORD PTR [rdx]
 400149a:	0f b6 13             	movzx  edx,BYTE PTR [rbx]
 400149d:	45 85 c0             	test   r8d,r8d
 40014a0:	0f 85 a9 02 00 00    	jne    400174f <_vsnprintf+0x72f>
 40014a6:	48 c7 c0 fe ff ff ff 	mov    rax,0xfffffffffffffffe
 40014ad:	84 d2                	test   dl,dl
 40014af:	0f 84 c9 06 00 00    	je     4001b7e <_vsnprintf+0xb5e>
 40014b5:	48 8d 74 03 01       	lea    rsi,[rbx+rax*1+0x1]
 40014ba:	48 89 d8             	mov    rax,rbx
 40014bd:	eb 0a                	jmp    40014c9 <_vsnprintf+0x4a9>
 40014bf:	90                   	nop
 40014c0:	48 39 c6             	cmp    rsi,rax
 40014c3:	0f 84 7b 03 00 00    	je     4001844 <_vsnprintf+0x824>
 40014c9:	48 83 c0 01          	add    rax,0x1
 40014cd:	80 38 00             	cmp    BYTE PTR [rax],0x0
 40014d0:	75 ee                	jne    40014c0 <_vsnprintf+0x4a0>
 40014d2:	29 d8                	sub    eax,ebx
 40014d4:	89 45 9c             	mov    DWORD PTR [rbp-0x64],eax
 40014d7:	89 c8                	mov    eax,ecx
 40014d9:	25 00 04 00 00       	and    eax,0x400
 40014de:	89 45 b0             	mov    DWORD PTR [rbp-0x50],eax
 40014e1:	74 0d                	je     40014f0 <_vsnprintf+0x4d0>
 40014e3:	8b 45 9c             	mov    eax,DWORD PTR [rbp-0x64]
 40014e6:	44 39 c0             	cmp    eax,r8d
 40014e9:	41 0f 47 c0          	cmova  eax,r8d
 40014ed:	89 45 9c             	mov    DWORD PTR [rbp-0x64],eax
 40014f0:	83 e1 02             	and    ecx,0x2
 40014f3:	89 4d 98             	mov    DWORD PTR [rbp-0x68],ecx
 40014f6:	0f 84 7c 03 00 00    	je     4001878 <_vsnprintf+0x858>
 40014fc:	84 d2                	test   dl,dl
 40014fe:	74 6c                	je     400156c <_vsnprintf+0x54c>
 4001500:	4c 29 eb             	sub    rbx,r13
 4001503:	4c 89 7d 88          	mov    QWORD PTR [rbp-0x78],r15
 4001507:	45 89 c7             	mov    r15d,r8d
 400150a:	48 89 5d a8          	mov    QWORD PTR [rbp-0x58],rbx
 400150e:	44 89 4d 90          	mov    DWORD PTR [rbp-0x70],r9d
 4001512:	eb 07                	jmp    400151b <_vsnprintf+0x4fb>
 4001514:	0f 1f 40 00          	nop    DWORD PTR [rax+0x0]
 4001518:	49 89 dd             	mov    r13,rbx
 400151b:	8b 4d b0             	mov    ecx,DWORD PTR [rbp-0x50]
 400151e:	85 c9                	test   ecx,ecx
 4001520:	74 10                	je     4001532 <_vsnprintf+0x512>
 4001522:	41 8d 4f ff          	lea    ecx,[r15-0x1]
 4001526:	45 85 ff             	test   r15d,r15d
 4001529:	0f 84 24 03 00 00    	je     4001853 <_vsnprintf+0x833>
 400152f:	41 89 cf             	mov    r15d,ecx
 4001532:	48 8b 45 c8          	mov    rax,QWORD PTR [rbp-0x38]
 4001536:	0f be fa             	movsx  edi,dl
 4001539:	4c 89 e1             	mov    rcx,r12
 400153c:	4c 89 ea             	mov    rdx,r13
 400153f:	4c 89 f6             	mov    rsi,r14
 4001542:	49 8d 5d 01          	lea    rbx,[r13+0x1]
 4001546:	ff d0                	call   rax
 4001548:	48 8b 45 a8          	mov    rax,QWORD PTR [rbp-0x58]
 400154c:	41 0f b6 54 05 01    	movzx  edx,BYTE PTR [r13+rax*1+0x1]
 4001552:	84 d2                	test   dl,dl
 4001554:	75 c2                	jne    4001518 <_vsnprintf+0x4f8>
 4001556:	44 8b 4d 90          	mov    r9d,DWORD PTR [rbp-0x70]
 400155a:	4c 8b 7d 88          	mov    r15,QWORD PTR [rbp-0x78]
 400155e:	8b 45 98             	mov    eax,DWORD PTR [rbp-0x68]
 4001561:	85 c0                	test   eax,eax
 4001563:	0f 84 3d fb ff ff    	je     40010a6 <_vsnprintf+0x86>
 4001569:	49 89 dd             	mov    r13,rbx
 400156c:	8b 45 9c             	mov    eax,DWORD PTR [rbp-0x64]
 400156f:	41 39 c1             	cmp    r9d,eax
 4001572:	0f 86 8e 03 00 00    	jbe    4001906 <_vsnprintf+0x8e6>
 4001578:	41 83 e9 01          	sub    r9d,0x1
 400157c:	4c 89 7d b0          	mov    QWORD PTR [rbp-0x50],r15
 4001580:	41 29 c1             	sub    r9d,eax
 4001583:	49 8d 45 01          	lea    rax,[r13+0x1]
 4001587:	49 8d 1c 01          	lea    rbx,[r9+rax*1]
 400158b:	49 89 c7             	mov    r15,rax
 400158e:	eb 04                	jmp    4001594 <_vsnprintf+0x574>
 4001590:	49 83 c7 01          	add    r15,0x1
 4001594:	4c 89 ea             	mov    rdx,r13
 4001597:	48 8b 45 c8          	mov    rax,QWORD PTR [rbp-0x38]
 400159b:	4d 89 fd             	mov    r13,r15
 400159e:	4c 89 e1             	mov    rcx,r12
 40015a1:	4c 89 f6             	mov    rsi,r14
 40015a4:	bf 20 00 00 00       	mov    edi,0x20
 40015a9:	ff d0                	call   rax
 40015ab:	49 39 df             	cmp    r15,rbx
 40015ae:	75 e0                	jne    4001590 <_vsnprintf+0x570>
 40015b0:	4c 8b 7d b0          	mov    r15,QWORD PTR [rbp-0x50]
 40015b4:	e9 ed fa ff ff       	jmp    40010a6 <_vsnprintf+0x86>
 40015b9:	48 8b 75 c0          	mov    rsi,QWORD PTR [rbp-0x40]
 40015bd:	83 c9 21             	or     ecx,0x21
 40015c0:	8b 06                	mov    eax,DWORD PTR [rsi]
 40015c2:	83 f8 2f             	cmp    eax,0x2f
 40015c5:	0f 87 bd 01 00 00    	ja     4001788 <_vsnprintf+0x768>
 40015cb:	89 c2                	mov    edx,eax
 40015cd:	83 c0 08             	add    eax,0x8
 40015d0:	48 03 56 10          	add    rdx,QWORD PTR [rsi+0x10]
 40015d4:	89 06                	mov    DWORD PTR [rsi],eax
 40015d6:	51                   	push   rcx
 40015d7:	6a 10                	push   0x10
 40015d9:	41 50                	push   r8
 40015db:	6a 10                	push   0x10
 40015dd:	4c 8b 02             	mov    r8,QWORD PTR [rdx]
 40015e0:	45 31 c9             	xor    r9d,r9d
 40015e3:	4c 89 ea             	mov    rdx,r13
 40015e6:	48 8b 7d c8          	mov    rdi,QWORD PTR [rbp-0x38]
 40015ea:	48 8b 45 a0          	mov    rax,QWORD PTR [rbp-0x60]
 40015ee:	4c 89 e1             	mov    rcx,r12
 40015f1:	4c 89 f6             	mov    rsi,r14
 40015f4:	ff d0                	call   rax
 40015f6:	48 83 c4 20          	add    rsp,0x20
 40015fa:	49 89 c5             	mov    r13,rax
 40015fd:	e9 a7 fa ff ff       	jmp    40010a9 <_vsnprintf+0x89>
 4001602:	49 8d 45 01          	lea    rax,[r13+0x1]
 4001606:	83 e1 02             	and    ecx,0x2
 4001609:	48 89 c3             	mov    rbx,rax
 400160c:	0f 84 fc 02 00 00    	je     400190e <_vsnprintf+0x8ee>
 4001612:	48 8b 75 c0          	mov    rsi,QWORD PTR [rbp-0x40]
 4001616:	8b 16                	mov    edx,DWORD PTR [rsi]
 4001618:	83 fa 2f             	cmp    edx,0x2f
 400161b:	0f 86 7c 03 00 00    	jbe    400199d <_vsnprintf+0x97d>
 4001621:	48 8b 75 c0          	mov    rsi,QWORD PTR [rbp-0x40]
 4001625:	48 8b 4e 08          	mov    rcx,QWORD PTR [rsi+0x8]
 4001629:	48 8d 51 08          	lea    rdx,[rcx+0x8]
 400162d:	48 89 56 08          	mov    QWORD PTR [rsi+0x8],rdx
 4001631:	4c 89 ea             	mov    rdx,r13
 4001634:	48 89 45 a8          	mov    QWORD PTR [rbp-0x58],rax
 4001638:	0f be 39             	movsx  edi,BYTE PTR [rcx]
 400163b:	4c 89 f6             	mov    rsi,r14
 400163e:	44 89 4d b0          	mov    DWORD PTR [rbp-0x50],r9d
 4001642:	48 8b 45 c8          	mov    rax,QWORD PTR [rbp-0x38]
 4001646:	4c 89 e1             	mov    rcx,r12
 4001649:	ff d0                	call   rax
 400164b:	44 8b 4d b0          	mov    r9d,DWORD PTR [rbp-0x50]
 400164f:	48 8b 45 a8          	mov    rax,QWORD PTR [rbp-0x58]
 4001653:	41 8d 51 fe          	lea    edx,[r9-0x2]
 4001657:	41 83 f9 01          	cmp    r9d,0x1
 400165b:	4d 8d 6c 15 02       	lea    r13,[r13+rdx*1+0x2]
 4001660:	0f 86 24 05 00 00    	jbe    4001b8a <_vsnprintf+0xb6a>
 4001666:	4c 89 7d b0          	mov    QWORD PTR [rbp-0x50],r15
 400166a:	49 89 df             	mov    r15,rbx
 400166d:	48 8b 5d c8          	mov    rbx,QWORD PTR [rbp-0x38]
 4001671:	0f 1f 80 00 00 00 00 	nop    DWORD PTR [rax+0x0]
 4001678:	4c 89 fa             	mov    rdx,r15
 400167b:	49 83 c7 01          	add    r15,0x1
 400167f:	4c 89 e1             	mov    rcx,r12
 4001682:	4c 89 f6             	mov    rsi,r14
 4001685:	bf 20 00 00 00       	mov    edi,0x20
 400168a:	ff d3                	call   rbx
 400168c:	4d 39 ef             	cmp    r15,r13
 400168f:	75 e7                	jne    4001678 <_vsnprintf+0x658>
 4001691:	4c 8b 7d b0          	mov    r15,QWORD PTR [rbp-0x50]
 4001695:	e9 0f fa ff ff       	jmp    40010a9 <_vsnprintf+0x89>
 400169a:	66 0f 1f 44 00 00    	nop    WORD PTR [rax+rax*1+0x0]
 40016a0:	48 b8 98 c1 ff ff ff 	movabs rax,0xffffffffffffc198
 40016a7:	ff ff ff 
 40016aa:	48 8b 4d b8          	mov    rcx,QWORD PTR [rbp-0x48]
 40016ae:	48 01 c8             	add    rax,rcx
 40016b1:	48 89 45 c8          	mov    QWORD PTR [rbp-0x38],rax
 40016b5:	e9 a9 f9 ff ff       	jmp    4001063 <_vsnprintf+0x43>
 40016ba:	66 0f 1f 44 00 00    	nop    WORD PTR [rax+rax*1+0x0]
 40016c0:	48 8b 5d c0          	mov    rbx,QWORD PTR [rbp-0x40]
 40016c4:	8b 13                	mov    edx,DWORD PTR [rbx]
 40016c6:	83 fa 2f             	cmp    edx,0x2f
 40016c9:	0f 87 94 01 00 00    	ja     4001863 <_vsnprintf+0x843>
 40016cf:	89 d0                	mov    eax,edx
 40016d1:	83 c2 08             	add    edx,0x8
 40016d4:	48 03 43 10          	add    rax,QWORD PTR [rbx+0x10]
 40016d8:	89 13                	mov    DWORD PTR [rbx],edx
 40016da:	44 8b 00             	mov    r8d,DWORD PTR [rax]
 40016dd:	0f be 7e 02          	movsx  edi,BYTE PTR [rsi+0x2]
 40016e1:	4c 8d 7e 03          	lea    r15,[rsi+0x3]
 40016e5:	45 85 c0             	test   r8d,r8d
 40016e8:	41 b8 00 00 00 00    	mov    r8d,0x0
 40016ee:	44 0f 49 00          	cmovns r8d,DWORD PTR [rax]
 40016f2:	48 83 c6 02          	add    rsi,0x2
 40016f6:	e9 21 fa ff ff       	jmp    400111c <_vsnprintf+0xfc>
 40016fb:	0f 1f 44 00 00       	nop    DWORD PTR [rax+rax*1+0x0]
 4001700:	48 8b 75 c0          	mov    rsi,QWORD PTR [rbp-0x40]
 4001704:	48 8b 56 08          	mov    rdx,QWORD PTR [rsi+0x8]
 4001708:	48 8d 42 08          	lea    rax,[rdx+0x8]
 400170c:	48 89 46 08          	mov    QWORD PTR [rsi+0x8],rax
 4001710:	e9 c9 fc ff ff       	jmp    40013de <_vsnprintf+0x3be>
 4001715:	0f 1f 00             	nop    DWORD PTR [rax]
 4001718:	48 8b 75 c0          	mov    rsi,QWORD PTR [rbp-0x40]
 400171c:	48 8b 56 08          	mov    rdx,QWORD PTR [rsi+0x8]
 4001720:	48 8d 42 08          	lea    rax,[rdx+0x8]
 4001724:	48 89 46 08          	mov    QWORD PTR [rsi+0x8],rax
 4001728:	e9 0e fd ff ff       	jmp    400143b <_vsnprintf+0x41b>
 400172d:	0f 1f 00             	nop    DWORD PTR [rax]
 4001730:	48 8b 75 c0          	mov    rsi,QWORD PTR [rbp-0x40]
 4001734:	48 8b 56 08          	mov    rdx,QWORD PTR [rsi+0x8]
 4001738:	48 8d 42 08          	lea    rax,[rdx+0x8]
 400173c:	48 89 46 08          	mov    QWORD PTR [rsi+0x8],rax
 4001740:	48 8b 1a             	mov    rbx,QWORD PTR [rdx]
 4001743:	0f b6 13             	movzx  edx,BYTE PTR [rbx]
 4001746:	45 85 c0             	test   r8d,r8d
 4001749:	0f 84 57 fd ff ff    	je     40014a6 <_vsnprintf+0x486>
 400174f:	44 89 c0             	mov    eax,r8d
 4001752:	84 d2                	test   dl,dl
 4001754:	0f 84 24 04 00 00    	je     4001b7e <_vsnprintf+0xb5e>
 400175a:	48 83 e8 01          	sub    rax,0x1
 400175e:	e9 52 fd ff ff       	jmp    40014b5 <_vsnprintf+0x495>
 4001763:	0f 1f 44 00 00       	nop    DWORD PTR [rax+rax*1+0x0]
 4001768:	40 80 ff 58          	cmp    dil,0x58
 400176c:	0f 85 e3 02 00 00    	jne    4001a55 <_vsnprintf+0xa35>
 4001772:	83 c9 20             	or     ecx,0x20
 4001775:	be 10 00 00 00       	mov    esi,0x10
 400177a:	83 e1 f3             	and    ecx,0xfffffff3
 400177d:	e9 6f fb ff ff       	jmp    40012f1 <_vsnprintf+0x2d1>
 4001782:	66 0f 1f 44 00 00    	nop    WORD PTR [rax+rax*1+0x0]
 4001788:	48 8b 75 c0          	mov    rsi,QWORD PTR [rbp-0x40]
 400178c:	48 8b 56 08          	mov    rdx,QWORD PTR [rsi+0x8]
 4001790:	48 8d 42 08          	lea    rax,[rdx+0x8]
 4001794:	48 89 46 08          	mov    QWORD PTR [rsi+0x8],rax
 4001798:	e9 39 fe ff ff       	jmp    40015d6 <_vsnprintf+0x5b6>
 400179d:	0f 1f 00             	nop    DWORD PTR [rax]
 40017a0:	85 d2                	test   edx,edx
 40017a2:	0f 85 72 02 00 00    	jne    4001a1a <_vsnprintf+0x9fa>
 40017a8:	f6 c5 01             	test   ch,0x1
 40017ab:	0f 85 e1 02 00 00    	jne    4001a92 <_vsnprintf+0xa72>
 40017b1:	f6 c1 40             	test   cl,0x40
 40017b4:	0f 85 f3 01 00 00    	jne    40019ad <_vsnprintf+0x98d>
 40017ba:	f6 c1 80             	test   cl,0x80
 40017bd:	0f 84 a0 03 00 00    	je     4001b63 <_vsnprintf+0xb43>
 40017c3:	83 f8 2f             	cmp    eax,0x2f
 40017c6:	0f 87 f0 03 00 00    	ja     4001bbc <_vsnprintf+0xb9c>
 40017cc:	48 8b 5d c0          	mov    rbx,QWORD PTR [rbp-0x40]
 40017d0:	89 c2                	mov    edx,eax
 40017d2:	83 c0 08             	add    eax,0x8
 40017d5:	89 03                	mov    DWORD PTR [rbx],eax
 40017d7:	48 03 53 10          	add    rdx,QWORD PTR [rbx+0x10]
 40017db:	0f bf 12             	movsx  edx,WORD PTR [rdx]
 40017de:	89 d7                	mov    edi,edx
 40017e0:	51                   	push   rcx
 40017e1:	c1 ff 1f             	sar    edi,0x1f
 40017e4:	41 51                	push   r9
 40017e6:	89 f8                	mov    eax,edi
 40017e8:	41 50                	push   r8
 40017ea:	31 d0                	xor    eax,edx
 40017ec:	56                   	push   rsi
 40017ed:	c1 ea 1f             	shr    edx,0x1f
 40017f0:	29 f8                	sub    eax,edi
 40017f2:	41 89 d1             	mov    r9d,edx
 40017f5:	4c 63 c0             	movsxd r8,eax
 40017f8:	48 8b 5d b8          	mov    rbx,QWORD PTR [rbp-0x48]
 40017fc:	4c 89 ea             	mov    rdx,r13
 40017ff:	48 8b 7d c8          	mov    rdi,QWORD PTR [rbp-0x38]
 4001803:	4c 89 e1             	mov    rcx,r12
 4001806:	48 b8 a8 c2 ff ff ff 	movabs rax,0xffffffffffffc2a8
 400180d:	ff ff ff 
 4001810:	4c 89 f6             	mov    rsi,r14
 4001813:	48 01 d8             	add    rax,rbx
 4001816:	ff d0                	call   rax
 4001818:	48 83 c4 20          	add    rsp,0x20
 400181c:	49 89 c5             	mov    r13,rax
 400181f:	e9 85 f8 ff ff       	jmp    40010a9 <_vsnprintf+0x89>
 4001824:	0f be 7e 02          	movsx  edi,BYTE PTR [rsi+0x2]
 4001828:	80 c9 c0             	or     cl,0xc0
 400182b:	4c 8d 7e 03          	lea    r15,[rsi+0x3]
 400182f:	e9 0f f9 ff ff       	jmp    4001143 <_vsnprintf+0x123>
 4001834:	0f be 7e 02          	movsx  edi,BYTE PTR [rsi+0x2]
 4001838:	80 cd 03             	or     ch,0x3
 400183b:	4c 8d 7e 03          	lea    r15,[rsi+0x3]
 400183f:	e9 ff f8 ff ff       	jmp    4001143 <_vsnprintf+0x123>
 4001844:	89 f0                	mov    eax,esi
 4001846:	e9 87 fc ff ff       	jmp    40014d2 <_vsnprintf+0x4b2>
 400184b:	45 31 ff             	xor    r15d,r15d
 400184e:	e9 48 f9 ff ff       	jmp    400119b <_vsnprintf+0x17b>
 4001853:	44 8b 4d 90          	mov    r9d,DWORD PTR [rbp-0x70]
 4001857:	4c 8b 7d 88          	mov    r15,QWORD PTR [rbp-0x78]
 400185b:	4c 89 eb             	mov    rbx,r13
 400185e:	e9 fb fc ff ff       	jmp    400155e <_vsnprintf+0x53e>
 4001863:	48 8b 5d c0          	mov    rbx,QWORD PTR [rbp-0x40]
 4001867:	48 8b 43 08          	mov    rax,QWORD PTR [rbx+0x8]
 400186b:	48 8d 50 08          	lea    rdx,[rax+0x8]
 400186f:	48 89 53 08          	mov    QWORD PTR [rbx+0x8],rdx
 4001873:	e9 62 fe ff ff       	jmp    40016da <_vsnprintf+0x6ba>
 4001878:	8b 4d 9c             	mov    ecx,DWORD PTR [rbp-0x64]
 400187b:	8d 41 01             	lea    eax,[rcx+0x1]
 400187e:	41 39 c9             	cmp    r9d,ecx
 4001881:	0f 86 0b 03 00 00    	jbe    4001b92 <_vsnprintf+0xb72>
 4001887:	41 8d 41 ff          	lea    eax,[r9-0x1]
 400188b:	48 89 5d 90          	mov    QWORD PTR [rbp-0x70],rbx
 400188f:	89 c2                	mov    edx,eax
 4001891:	49 8d 45 01          	lea    rax,[r13+0x1]
 4001895:	4c 89 7d 80          	mov    QWORD PTR [rbp-0x80],r15
 4001899:	4d 89 ef             	mov    r15,r13
 400189c:	29 ca                	sub    edx,ecx
 400189e:	4d 89 e5             	mov    r13,r12
 40018a1:	44 89 45 9c          	mov    DWORD PTR [rbp-0x64],r8d
 40018a5:	4c 8b 65 c8          	mov    r12,QWORD PTR [rbp-0x38]
 40018a9:	48 8d 0c 02          	lea    rcx,[rdx+rax*1]
 40018ad:	44 89 4d 88          	mov    DWORD PTR [rbp-0x78],r9d
 40018b1:	48 89 c3             	mov    rbx,rax
 40018b4:	48 89 4d a8          	mov    QWORD PTR [rbp-0x58],rcx
 40018b8:	eb 0a                	jmp    40018c4 <_vsnprintf+0x8a4>
 40018ba:	66 0f 1f 44 00 00    	nop    WORD PTR [rax+rax*1+0x0]
 40018c0:	48 83 c3 01          	add    rbx,0x1
 40018c4:	4c 89 fa             	mov    rdx,r15
 40018c7:	4c 89 e9             	mov    rcx,r13
 40018ca:	49 89 df             	mov    r15,rbx
 40018cd:	4c 89 f6             	mov    rsi,r14
 40018d0:	bf 20 00 00 00       	mov    edi,0x20
 40018d5:	41 ff d4             	call   r12
 40018d8:	48 3b 5d a8          	cmp    rbx,QWORD PTR [rbp-0x58]
 40018dc:	75 e2                	jne    40018c0 <_vsnprintf+0x8a0>
 40018de:	44 8b 4d 88          	mov    r9d,DWORD PTR [rbp-0x78]
 40018e2:	48 8b 5d 90          	mov    rbx,QWORD PTR [rbp-0x70]
 40018e6:	4d 89 ec             	mov    r12,r13
 40018e9:	4d 89 fd             	mov    r13,r15
 40018ec:	44 8b 45 9c          	mov    r8d,DWORD PTR [rbp-0x64]
 40018f0:	4c 8b 7d 80          	mov    r15,QWORD PTR [rbp-0x80]
 40018f4:	41 8d 41 01          	lea    eax,[r9+0x1]
 40018f8:	0f b6 13             	movzx  edx,BYTE PTR [rbx]
 40018fb:	89 45 9c             	mov    DWORD PTR [rbp-0x64],eax
 40018fe:	84 d2                	test   dl,dl
 4001900:	0f 85 fa fb ff ff    	jne    4001500 <_vsnprintf+0x4e0>
 4001906:	4c 89 eb             	mov    rbx,r13
 4001909:	e9 98 f7 ff ff       	jmp    40010a6 <_vsnprintf+0x86>
 400190e:	41 83 f9 01          	cmp    r9d,0x1
 4001912:	0f 86 82 02 00 00    	jbe    4001b9a <_vsnprintf+0xb7a>
 4001918:	41 8d 51 fe          	lea    edx,[r9-0x2]
 400191c:	4c 89 7d b0          	mov    QWORD PTR [rbp-0x50],r15
 4001920:	4d 89 ef             	mov    r15,r13
 4001923:	4d 89 e5             	mov    r13,r12
 4001926:	48 8d 1c 02          	lea    rbx,[rdx+rax*1]
 400192a:	49 89 c4             	mov    r12,rax
 400192d:	eb 05                	jmp    4001934 <_vsnprintf+0x914>
 400192f:	90                   	nop
 4001930:	49 83 c4 01          	add    r12,0x1
 4001934:	4c 89 fa             	mov    rdx,r15
 4001937:	48 8b 45 c8          	mov    rax,QWORD PTR [rbp-0x38]
 400193b:	4d 89 e7             	mov    r15,r12
 400193e:	4c 89 e9             	mov    rcx,r13
 4001941:	4c 89 f6             	mov    rsi,r14
 4001944:	bf 20 00 00 00       	mov    edi,0x20
 4001949:	ff d0                	call   rax
 400194b:	49 39 dc             	cmp    r12,rbx
 400194e:	75 e0                	jne    4001930 <_vsnprintf+0x910>
 4001950:	4d 89 ec             	mov    r12,r13
 4001953:	4d 89 fd             	mov    r13,r15
 4001956:	4c 8b 7d b0          	mov    r15,QWORD PTR [rbp-0x50]
 400195a:	49 83 c5 01          	add    r13,0x1
 400195e:	48 8b 4d c0          	mov    rcx,QWORD PTR [rbp-0x40]
 4001962:	8b 01                	mov    eax,DWORD PTR [rcx]
 4001964:	83 f8 2f             	cmp    eax,0x2f
 4001967:	77 22                	ja     400198b <_vsnprintf+0x96b>
 4001969:	89 c2                	mov    edx,eax
 400196b:	83 c0 08             	add    eax,0x8
 400196e:	48 03 51 10          	add    rdx,QWORD PTR [rcx+0x10]
 4001972:	89 01                	mov    DWORD PTR [rcx],eax
 4001974:	0f be 3a             	movsx  edi,BYTE PTR [rdx]
 4001977:	48 8b 45 c8          	mov    rax,QWORD PTR [rbp-0x38]
 400197b:	4c 89 e1             	mov    rcx,r12
 400197e:	48 89 da             	mov    rdx,rbx
 4001981:	4c 89 f6             	mov    rsi,r14
 4001984:	ff d0                	call   rax
 4001986:	e9 1e f7 ff ff       	jmp    40010a9 <_vsnprintf+0x89>
 400198b:	48 8b 4d c0          	mov    rcx,QWORD PTR [rbp-0x40]
 400198f:	48 8b 51 08          	mov    rdx,QWORD PTR [rcx+0x8]
 4001993:	48 8d 42 08          	lea    rax,[rdx+0x8]
 4001997:	48 89 41 08          	mov    QWORD PTR [rcx+0x8],rax
 400199b:	eb d7                	jmp    4001974 <_vsnprintf+0x954>
 400199d:	89 d1                	mov    ecx,edx
 400199f:	83 c2 08             	add    edx,0x8
 40019a2:	48 03 4e 10          	add    rcx,QWORD PTR [rsi+0x10]
 40019a6:	89 16                	mov    DWORD PTR [rsi],edx
 40019a8:	e9 84 fc ff ff       	jmp    4001631 <_vsnprintf+0x611>
 40019ad:	83 f8 2f             	cmp    eax,0x2f
 40019b0:	0f 87 83 01 00 00    	ja     4001b39 <_vsnprintf+0xb19>
 40019b6:	48 8b 5d c0          	mov    rbx,QWORD PTR [rbp-0x40]
 40019ba:	89 c2                	mov    edx,eax
 40019bc:	83 c0 08             	add    eax,0x8
 40019bf:	89 03                	mov    DWORD PTR [rbx],eax
 40019c1:	48 03 53 10          	add    rdx,QWORD PTR [rbx+0x10]
 40019c5:	0f be 12             	movsx  edx,BYTE PTR [rdx]
 40019c8:	e9 11 fe ff ff       	jmp    40017de <_vsnprintf+0x7be>
 40019cd:	83 f8 2f             	cmp    eax,0x2f
 40019d0:	0f 87 4e 01 00 00    	ja     4001b24 <_vsnprintf+0xb04>
 40019d6:	48 8b 5d c0          	mov    rbx,QWORD PTR [rbp-0x40]
 40019da:	89 c2                	mov    edx,eax
 40019dc:	83 c0 08             	add    eax,0x8
 40019df:	89 03                	mov    DWORD PTR [rbx],eax
 40019e1:	48 03 53 10          	add    rdx,QWORD PTR [rbx+0x10]
 40019e5:	0f b6 02             	movzx  eax,BYTE PTR [rdx]
 40019e8:	e9 6f f9 ff ff       	jmp    400135c <_vsnprintf+0x33c>
 40019ed:	be 08 00 00 00       	mov    esi,0x8
 40019f2:	e9 83 fd ff ff       	jmp    400177a <_vsnprintf+0x75a>
 40019f7:	83 f8 2f             	cmp    eax,0x2f
 40019fa:	0f 87 de 00 00 00    	ja     4001ade <_vsnprintf+0xabe>
 4001a00:	48 8b 5d c0          	mov    rbx,QWORD PTR [rbp-0x40]
 4001a04:	89 c2                	mov    edx,eax
 4001a06:	83 c0 08             	add    eax,0x8
 4001a09:	89 03                	mov    DWORD PTR [rbx],eax
 4001a0b:	48 03 53 10          	add    rdx,QWORD PTR [rbx+0x10]
 4001a0f:	51                   	push   rcx
 4001a10:	41 51                	push   r9
 4001a12:	41 50                	push   r8
 4001a14:	56                   	push   rsi
 4001a15:	e9 c3 fb ff ff       	jmp    40015dd <_vsnprintf+0x5bd>
 4001a1a:	83 f8 2f             	cmp    eax,0x2f
 4001a1d:	0f 87 a6 00 00 00    	ja     4001ac9 <_vsnprintf+0xaa9>
 4001a23:	48 8b 5d c0          	mov    rbx,QWORD PTR [rbp-0x40]
 4001a27:	89 c2                	mov    edx,eax
 4001a29:	83 c0 08             	add    eax,0x8
 4001a2c:	89 03                	mov    DWORD PTR [rbx],eax
 4001a2e:	48 03 53 10          	add    rdx,QWORD PTR [rbx+0x10]
 4001a32:	48 8b 02             	mov    rax,QWORD PTR [rdx]
 4001a35:	51                   	push   rcx
 4001a36:	41 51                	push   r9
 4001a38:	48 99                	cqo
 4001a3a:	41 50                	push   r8
 4001a3c:	48 89 d7             	mov    rdi,rdx
 4001a3f:	56                   	push   rsi
 4001a40:	48 31 c7             	xor    rdi,rax
 4001a43:	48 c1 e8 3f          	shr    rax,0x3f
 4001a47:	48 29 d7             	sub    rdi,rdx
 4001a4a:	49 89 c1             	mov    r9,rax
 4001a4d:	49 89 f8             	mov    r8,rdi
 4001a50:	e9 8e fb ff ff       	jmp    40015e3 <_vsnprintf+0x5c3>
 4001a55:	be 10 00 00 00       	mov    esi,0x10
 4001a5a:	e9 82 f8 ff ff       	jmp    40012e1 <_vsnprintf+0x2c1>
 4001a5f:	83 f8 2f             	cmp    eax,0x2f
 4001a62:	0f 87 e6 00 00 00    	ja     4001b4e <_vsnprintf+0xb2e>
 4001a68:	48 8b 5d c0          	mov    rbx,QWORD PTR [rbp-0x40]
 4001a6c:	89 c2                	mov    edx,eax
 4001a6e:	83 c0 08             	add    eax,0x8
 4001a71:	89 03                	mov    DWORD PTR [rbx],eax
 4001a73:	48 03 53 10          	add    rdx,QWORD PTR [rbx+0x10]
 4001a77:	51                   	push   rcx
 4001a78:	41 51                	push   r9
 4001a7a:	45 31 c9             	xor    r9d,r9d
 4001a7d:	41 50                	push   r8
 4001a7f:	56                   	push   rsi
 4001a80:	4c 8b 02             	mov    r8,QWORD PTR [rdx]
 4001a83:	e9 70 fd ff ff       	jmp    40017f8 <_vsnprintf+0x7d8>
 4001a88:	be 02 00 00 00       	mov    esi,0x2
 4001a8d:	e9 e8 fc ff ff       	jmp    400177a <_vsnprintf+0x75a>
 4001a92:	83 f8 2f             	cmp    eax,0x2f
 4001a95:	77 7b                	ja     4001b12 <_vsnprintf+0xaf2>
 4001a97:	48 8b 5d c0          	mov    rbx,QWORD PTR [rbp-0x40]
 4001a9b:	89 c2                	mov    edx,eax
 4001a9d:	83 c0 08             	add    eax,0x8
 4001aa0:	89 03                	mov    DWORD PTR [rbx],eax
 4001aa2:	48 03 53 10          	add    rdx,QWORD PTR [rbx+0x10]
 4001aa6:	48 8b 02             	mov    rax,QWORD PTR [rdx]
 4001aa9:	51                   	push   rcx
 4001aaa:	41 51                	push   r9
 4001aac:	48 99                	cqo
 4001aae:	41 50                	push   r8
 4001ab0:	48 89 d7             	mov    rdi,rdx
 4001ab3:	56                   	push   rsi
 4001ab4:	48 31 c7             	xor    rdi,rax
 4001ab7:	48 c1 e8 3f          	shr    rax,0x3f
 4001abb:	48 29 d7             	sub    rdi,rdx
 4001abe:	49 89 c1             	mov    r9,rax
 4001ac1:	49 89 f8             	mov    r8,rdi
 4001ac4:	e9 2f fd ff ff       	jmp    40017f8 <_vsnprintf+0x7d8>
 4001ac9:	48 8b 5d c0          	mov    rbx,QWORD PTR [rbp-0x40]
 4001acd:	48 8b 53 08          	mov    rdx,QWORD PTR [rbx+0x8]
 4001ad1:	48 8d 42 08          	lea    rax,[rdx+0x8]
 4001ad5:	48 89 43 08          	mov    QWORD PTR [rbx+0x8],rax
 4001ad9:	e9 54 ff ff ff       	jmp    4001a32 <_vsnprintf+0xa12>
 4001ade:	48 8b 5d c0          	mov    rbx,QWORD PTR [rbp-0x40]
 4001ae2:	48 8b 53 08          	mov    rdx,QWORD PTR [rbx+0x8]
 4001ae6:	48 8d 42 08          	lea    rax,[rdx+0x8]
 4001aea:	48 89 43 08          	mov    QWORD PTR [rbx+0x8],rax
 4001aee:	e9 1c ff ff ff       	jmp    4001a0f <_vsnprintf+0x9ef>
 4001af3:	83 f8 2f             	cmp    eax,0x2f
 4001af6:	0f 87 d5 00 00 00    	ja     4001bd1 <_vsnprintf+0xbb1>
 4001afc:	48 8b 5d c0          	mov    rbx,QWORD PTR [rbp-0x40]
 4001b00:	89 c2                	mov    edx,eax
 4001b02:	83 c0 08             	add    eax,0x8
 4001b05:	48 03 53 10          	add    rdx,QWORD PTR [rbx+0x10]
 4001b09:	89 03                	mov    DWORD PTR [rbx],eax
 4001b0b:	8b 02                	mov    eax,DWORD PTR [rdx]
 4001b0d:	e9 4a f8 ff ff       	jmp    400135c <_vsnprintf+0x33c>
 4001b12:	48 8b 5d c0          	mov    rbx,QWORD PTR [rbp-0x40]
 4001b16:	48 8b 53 08          	mov    rdx,QWORD PTR [rbx+0x8]
 4001b1a:	48 8d 42 08          	lea    rax,[rdx+0x8]
 4001b1e:	48 89 43 08          	mov    QWORD PTR [rbx+0x8],rax
 4001b22:	eb 82                	jmp    4001aa6 <_vsnprintf+0xa86>
 4001b24:	48 8b 5d c0          	mov    rbx,QWORD PTR [rbp-0x40]
 4001b28:	48 8b 53 08          	mov    rdx,QWORD PTR [rbx+0x8]
 4001b2c:	48 8d 42 08          	lea    rax,[rdx+0x8]
 4001b30:	48 89 43 08          	mov    QWORD PTR [rbx+0x8],rax
 4001b34:	e9 ac fe ff ff       	jmp    40019e5 <_vsnprintf+0x9c5>
 4001b39:	48 8b 5d c0          	mov    rbx,QWORD PTR [rbp-0x40]
 4001b3d:	48 8b 53 08          	mov    rdx,QWORD PTR [rbx+0x8]
 4001b41:	48 8d 42 08          	lea    rax,[rdx+0x8]
 4001b45:	48 89 43 08          	mov    QWORD PTR [rbx+0x8],rax
 4001b49:	e9 77 fe ff ff       	jmp    40019c5 <_vsnprintf+0x9a5>
 4001b4e:	48 8b 5d c0          	mov    rbx,QWORD PTR [rbp-0x40]
 4001b52:	48 8b 53 08          	mov    rdx,QWORD PTR [rbx+0x8]
 4001b56:	48 8d 42 08          	lea    rax,[rdx+0x8]
 4001b5a:	48 89 43 08          	mov    QWORD PTR [rbx+0x8],rax
 4001b5e:	e9 14 ff ff ff       	jmp    4001a77 <_vsnprintf+0xa57>
 4001b63:	83 f8 2f             	cmp    eax,0x2f
 4001b66:	77 3d                	ja     4001ba5 <_vsnprintf+0xb85>
 4001b68:	48 8b 5d c0          	mov    rbx,QWORD PTR [rbp-0x40]
 4001b6c:	89 c2                	mov    edx,eax
 4001b6e:	83 c0 08             	add    eax,0x8
 4001b71:	48 03 53 10          	add    rdx,QWORD PTR [rbx+0x10]
 4001b75:	89 03                	mov    DWORD PTR [rbx],eax
 4001b77:	8b 12                	mov    edx,DWORD PTR [rdx]
 4001b79:	e9 60 fc ff ff       	jmp    40017de <_vsnprintf+0x7be>
 4001b7e:	c7 45 9c 00 00 00 00 	mov    DWORD PTR [rbp-0x64],0x0
 4001b85:	e9 4d f9 ff ff       	jmp    40014d7 <_vsnprintf+0x4b7>
 4001b8a:	49 89 c5             	mov    r13,rax
 4001b8d:	e9 17 f5 ff ff       	jmp    40010a9 <_vsnprintf+0x89>
 4001b92:	89 45 9c             	mov    DWORD PTR [rbp-0x64],eax
 4001b95:	e9 64 fd ff ff       	jmp    40018fe <_vsnprintf+0x8de>
 4001b9a:	4c 89 eb             	mov    rbx,r13
 4001b9d:	49 89 c5             	mov    r13,rax
 4001ba0:	e9 b9 fd ff ff       	jmp    400195e <_vsnprintf+0x93e>
 4001ba5:	48 8b 5d c0          	mov    rbx,QWORD PTR [rbp-0x40]
 4001ba9:	48 8b 53 08          	mov    rdx,QWORD PTR [rbx+0x8]
 4001bad:	48 8d 42 08          	lea    rax,[rdx+0x8]
 4001bb1:	48 89 43 08          	mov    QWORD PTR [rbx+0x8],rax
 4001bb5:	8b 12                	mov    edx,DWORD PTR [rdx]
 4001bb7:	e9 22 fc ff ff       	jmp    40017de <_vsnprintf+0x7be>
 4001bbc:	48 8b 5d c0          	mov    rbx,QWORD PTR [rbp-0x40]
 4001bc0:	48 8b 53 08          	mov    rdx,QWORD PTR [rbx+0x8]
 4001bc4:	48 8d 42 08          	lea    rax,[rdx+0x8]
 4001bc8:	48 89 43 08          	mov    QWORD PTR [rbx+0x8],rax
 4001bcc:	e9 0a fc ff ff       	jmp    40017db <_vsnprintf+0x7bb>
 4001bd1:	48 8b 5d c0          	mov    rbx,QWORD PTR [rbp-0x40]
 4001bd5:	48 8b 53 08          	mov    rdx,QWORD PTR [rbx+0x8]
 4001bd9:	48 8d 42 08          	lea    rax,[rdx+0x8]
 4001bdd:	48 89 43 08          	mov    QWORD PTR [rbx+0x8],rax
 4001be1:	8b 02                	mov    eax,DWORD PTR [rdx]
 4001be3:	e9 74 f7 ff ff       	jmp    400135c <_vsnprintf+0x33c>
 4001be8:	48 8b 5d c0          	mov    rbx,QWORD PTR [rbp-0x40]
 4001bec:	48 8b 53 08          	mov    rdx,QWORD PTR [rbx+0x8]
 4001bf0:	48 8d 42 08          	lea    rax,[rdx+0x8]
 4001bf4:	48 89 43 08          	mov    QWORD PTR [rbx+0x8],rax
 4001bf8:	e9 5c f7 ff ff       	jmp    4001359 <_vsnprintf+0x339>
 4001bfd:	0f 1f 00             	nop    DWORD PTR [rax]

0000000004001c00 <_putchar>:
 4001c00:	49 bb 08 24 00 00 00 	movabs r11,0x2408
 4001c07:	00 00 00 
 4001c0a:	55                   	push   rbp
 4001c0b:	40 0f be ff          	movsx  edi,dil
 4001c0f:	48 b8 48 e4 ff ff ff 	movabs rax,0xffffffffffffe448
 4001c16:	ff ff ff 
 4001c19:	48 89 e5             	mov    rbp,rsp
 4001c1c:	41 57                	push   r15
 4001c1e:	4c 8d 3d db ff ff ff 	lea    r15,[rip+0xffffffffffffffdb]        # 4001c00 <_putchar>
 4001c25:	4d 01 df             	add    r15,r11
 4001c28:	4c 01 f8             	add    rax,r15
 4001c2b:	4c 8b 7d f8          	mov    r15,QWORD PTR [rbp-0x8]
 4001c2f:	c9                   	leave
 4001c30:	ff e0                	jmp    rax
 4001c32:	66 66 2e 0f 1f 84 00 	data16 cs nop WORD PTR [rax+rax*1+0x0]
 4001c39:	00 00 00 00 
 4001c3d:	0f 1f 00             	nop    DWORD PTR [rax]

0000000004001c40 <_out_char>:
 4001c40:	49 bb c8 23 00 00 00 	movabs r11,0x23c8
 4001c47:	00 00 00 
 4001c4a:	55                   	push   rbp
 4001c4b:	48 89 e5             	mov    rbp,rsp
 4001c4e:	41 57                	push   r15
 4001c50:	4c 8d 3d e9 ff ff ff 	lea    r15,[rip+0xffffffffffffffe9]        # 4001c40 <_out_char>
 4001c57:	4d 01 df             	add    r15,r11
 4001c5a:	40 84 ff             	test   dil,dil
 4001c5d:	75 09                	jne    4001c68 <_out_char+0x28>
 4001c5f:	4c 8b 7d f8          	mov    r15,QWORD PTR [rbp-0x8]
 4001c63:	c9                   	leave
 4001c64:	c3                   	ret
 4001c65:	0f 1f 00             	nop    DWORD PTR [rax]
 4001c68:	48 b8 f8 db ff ff ff 	movabs rax,0xffffffffffffdbf8
 4001c6f:	ff ff ff 
 4001c72:	40 0f be ff          	movsx  edi,dil
 4001c76:	4c 01 f8             	add    rax,r15
 4001c79:	4c 8b 7d f8          	mov    r15,QWORD PTR [rbp-0x8]
 4001c7d:	c9                   	leave
 4001c7e:	ff e0                	jmp    rax

0000000004001c80 <panic>:
 4001c80:	49 bb 88 23 00 00 00 	movabs r11,0x2388
 4001c87:	00 00 00 
 4001c8a:	55                   	push   rbp
 4001c8b:	48 89 e5             	mov    rbp,rsp
 4001c8e:	41 57                	push   r15
 4001c90:	4c 8d 3d e9 ff ff ff 	lea    r15,[rip+0xffffffffffffffe9]        # 4001c80 <panic>
 4001c97:	41 56                	push   r14
 4001c99:	4d 01 df             	add    r15,r11
 4001c9c:	41 55                	push   r13
 4001c9e:	41 54                	push   r12
 4001ca0:	49 89 fc             	mov    r12,rdi
 4001ca3:	53                   	push   rbx
 4001ca4:	48 81 ec 08 01 00 00 	sub    rsp,0x108
 4001cab:	48 89 b5 28 ff ff ff 	mov    QWORD PTR [rbp-0xd8],rsi
 4001cb2:	48 89 95 30 ff ff ff 	mov    QWORD PTR [rbp-0xd0],rdx
 4001cb9:	48 89 8d 38 ff ff ff 	mov    QWORD PTR [rbp-0xc8],rcx
 4001cc0:	4c 89 85 40 ff ff ff 	mov    QWORD PTR [rbp-0xc0],r8
 4001cc7:	4c 89 8d 48 ff ff ff 	mov    QWORD PTR [rbp-0xb8],r9
 4001cce:	84 c0                	test   al,al
 4001cd0:	74 29                	je     4001cfb <panic+0x7b>
 4001cd2:	0f 29 85 50 ff ff ff 	movaps XMMWORD PTR [rbp-0xb0],xmm0
 4001cd9:	0f 29 8d 60 ff ff ff 	movaps XMMWORD PTR [rbp-0xa0],xmm1
 4001ce0:	0f 29 95 70 ff ff ff 	movaps XMMWORD PTR [rbp-0x90],xmm2
 4001ce7:	0f 29 5d 80          	movaps XMMWORD PTR [rbp-0x80],xmm3
 4001ceb:	0f 29 65 90          	movaps XMMWORD PTR [rbp-0x70],xmm4
 4001cef:	0f 29 6d a0          	movaps XMMWORD PTR [rbp-0x60],xmm5
 4001cf3:	0f 29 75 b0          	movaps XMMWORD PTR [rbp-0x50],xmm6
 4001cf7:	0f 29 7d c0          	movaps XMMWORD PTR [rbp-0x40],xmm7
 4001cfb:	48 8d 45 10          	lea    rax,[rbp+0x10]
 4001cff:	41 bd 01 00 00 00    	mov    r13d,0x1
 4001d05:	c7 85 08 ff ff ff 08 	mov    DWORD PTR [rbp-0xf8],0x8
 4001d0c:	00 00 00 
 4001d0f:	49 be f8 0f 00 00 00 	movabs r14,0xff8
 4001d16:	00 00 00 
 4001d19:	48 89 85 10 ff ff ff 	mov    QWORD PTR [rbp-0xf0],rax
 4001d20:	48 8d 85 20 ff ff ff 	lea    rax,[rbp-0xe0]
 4001d27:	48 bb a8 e5 ff ff ff 	movabs rbx,0xffffffffffffe5a8
 4001d2e:	ff ff ff 
 4001d31:	48 89 85 18 ff ff ff 	mov    QWORD PTR [rbp-0xe8],rax
 4001d38:	48 b8 6a f4 ff ff ff 	movabs rax,0xfffffffffffff46a
 4001d3f:	ff ff ff 
 4001d42:	48 89 85 d8 fe ff ff 	mov    QWORD PTR [rbp-0x128],rax
 4001d49:	48 b8 80 f4 ff ff ff 	movabs rax,0xfffffffffffff480
 4001d50:	ff ff ff 
 4001d53:	48 89 85 e8 fe ff ff 	mov    QWORD PTR [rbp-0x118],rax
 4001d5a:	48 b8 91 f4 ff ff ff 	movabs rax,0xfffffffffffff491
 4001d61:	ff ff ff 
 4001d64:	48 89 85 e0 fe ff ff 	mov    QWORD PTR [rbp-0x120],rax
 4001d6b:	48 b8 78 dc ff ff ff 	movabs rax,0xffffffffffffdc78
 4001d72:	ff ff ff 
 4001d75:	c7 85 0c ff ff ff 30 	mov    DWORD PTR [rbp-0xf4],0x30
 4001d7c:	00 00 00 
 4001d7f:	48 89 85 f0 fe ff ff 	mov    QWORD PTR [rbp-0x110],rax
 4001d86:	66 2e 0f 1f 84 00 00 	cs nop WORD PTR [rax+rax*1+0x0]
 4001d8d:	00 00 00 
 4001d90:	44 89 e8             	mov    eax,r13d
 4001d93:	43 86 04 3e          	xchg   BYTE PTR [r14+r15*1],al
 4001d97:	84 c0                	test   al,al
 4001d99:	0f 85 e1 00 00 00    	jne    4001e80 <panic+0x200>
 4001d9f:	43 80 7c 3e 01 00    	cmp    BYTE PTR [r14+r15*1+0x1],0x0
 4001da5:	0f 85 e5 00 00 00    	jne    4001e90 <panic+0x210>
 4001dab:	43 c6 44 3e 01 01    	mov    BYTE PTR [r14+r15*1+0x1],0x1
 4001db1:	4a 8d 04 3b          	lea    rax,[rbx+r15*1]
 4001db5:	ff d0                	call   rax
 4001db7:	43 89 44 3e 04       	mov    DWORD PTR [r14+r15*1+0x4],eax
 4001dbc:	31 c0                	xor    eax,eax
 4001dbe:	43 86 04 3e          	xchg   BYTE PTR [r14+r15*1],al
 4001dc2:	48 c7 c2 ff ff ff ff 	mov    rdx,0xffffffffffffffff
 4001dc9:	48 b8 38 dc ff ff ff 	movabs rax,0xffffffffffffdc38
 4001dd0:	ff ff ff 
 4001dd3:	49 8d 3c 07          	lea    rdi,[r15+rax*1]
 4001dd7:	48 8d b5 07 ff ff ff 	lea    rsi,[rbp-0xf9]
 4001dde:	4c 89 e1             	mov    rcx,r12
 4001de1:	48 b8 18 d0 ff ff ff 	movabs rax,0xffffffffffffd018
 4001de8:	ff ff ff 
 4001deb:	4c 8d 85 08 ff ff ff 	lea    r8,[rbp-0xf8]
 4001df2:	4c 01 f8             	add    rax,r15
 4001df5:	ff d0                	call   rax
 4001df7:	ba 01 00 00 00       	mov    edx,0x1
 4001dfc:	eb 04                	jmp    4001e02 <panic+0x182>
 4001dfe:	66 90                	xchg   ax,ax
 4001e00:	f3 90                	pause
 4001e02:	89 d0                	mov    eax,edx
 4001e04:	43 86 04 3e          	xchg   BYTE PTR [r14+r15*1],al
 4001e08:	84 c0                	test   al,al
 4001e0a:	75 f4                	jne    4001e00 <panic+0x180>
 4001e0c:	43 80 7c 3e 01 00    	cmp    BYTE PTR [r14+r15*1+0x1],0x0
 4001e12:	74 0f                	je     4001e23 <panic+0x1a3>
 4001e14:	47 8b 64 3e 04       	mov    r12d,DWORD PTR [r14+r15*1+0x4]
 4001e19:	4c 01 fb             	add    rbx,r15
 4001e1c:	ff d3                	call   rbx
 4001e1e:	41 39 c4             	cmp    r12d,eax
 4001e21:	74 3d                	je     4001e60 <panic+0x1e0>
 4001e23:	48 8b 9d f0 fe ff ff 	mov    rbx,QWORD PTR [rbp-0x110]
 4001e2a:	48 8b 4d 08          	mov    rcx,QWORD PTR [rbp+0x8]
 4001e2e:	ba 77 03 00 00       	mov    edx,0x377
 4001e33:	48 b8 a9 f4 ff ff ff 	movabs rax,0xfffffffffffff4a9
 4001e3a:	ff ff ff 
 4001e3d:	4d 8d 04 07          	lea    r8,[r15+rax*1]
 4001e41:	48 8b 85 e8 fe ff ff 	mov    rax,QWORD PTR [rbp-0x118]
 4001e48:	4e 8d 14 3b          	lea    r10,[rbx+r15*1]
 4001e4c:	4a 8d 34 38          	lea    rsi,[rax+r15*1]
 4001e50:	48 8b 85 e0 fe ff ff 	mov    rax,QWORD PTR [rbp-0x120]
 4001e57:	4a 8d 3c 38          	lea    rdi,[rax+r15*1]
 4001e5b:	31 c0                	xor    eax,eax
 4001e5d:	41 ff d2             	call   r10
 4001e60:	43 c6 44 3e 01 00    	mov    BYTE PTR [r14+r15*1+0x1],0x0
 4001e66:	31 c0                	xor    eax,eax
 4001e68:	43 c7 44 3e 04 00 00 	mov    DWORD PTR [r14+r15*1+0x4],0x0
 4001e6f:	00 00 
 4001e71:	43 86 04 3e          	xchg   BYTE PTR [r14+r15*1],al
 4001e75:	eb fe                	jmp    4001e75 <panic+0x1f5>
 4001e77:	66 0f 1f 84 00 00 00 	nop    WORD PTR [rax+rax*1+0x0]
 4001e7e:	00 00 
 4001e80:	f3 90                	pause
 4001e82:	e9 09 ff ff ff       	jmp    4001d90 <panic+0x110>
 4001e87:	66 0f 1f 84 00 00 00 	nop    WORD PTR [rax+rax*1+0x0]
 4001e8e:	00 00 
 4001e90:	43 8b 44 3e 04       	mov    eax,DWORD PTR [r14+r15*1+0x4]
 4001e95:	89 85 fc fe ff ff    	mov    DWORD PTR [rbp-0x104],eax
 4001e9b:	4a 8d 04 3b          	lea    rax,[rbx+r15*1]
 4001e9f:	ff d0                	call   rax
 4001ea1:	39 85 fc fe ff ff    	cmp    DWORD PTR [rbp-0x104],eax
 4001ea7:	74 17                	je     4001ec0 <panic+0x240>
 4001ea9:	43 80 7c 3e 01 00    	cmp    BYTE PTR [r14+r15*1+0x1],0x0
 4001eaf:	0f 84 f6 fe ff ff    	je     4001dab <panic+0x12b>
 4001eb5:	31 c0                	xor    eax,eax
 4001eb7:	43 86 04 3e          	xchg   BYTE PTR [r14+r15*1],al
 4001ebb:	e9 d0 fe ff ff       	jmp    4001d90 <panic+0x110>
 4001ec0:	48 8b 85 d8 fe ff ff 	mov    rax,QWORD PTR [rbp-0x128]
 4001ec7:	48 8b 4d 08          	mov    rcx,QWORD PTR [rbp+0x8]
 4001ecb:	ba 75 03 00 00       	mov    edx,0x375
 4001ed0:	4c 8b 8d f0 fe ff ff 	mov    r9,QWORD PTR [rbp-0x110]
 4001ed7:	4e 8d 04 38          	lea    r8,[rax+r15*1]
 4001edb:	48 8b 85 e8 fe ff ff 	mov    rax,QWORD PTR [rbp-0x118]
 4001ee2:	4d 01 f9             	add    r9,r15
 4001ee5:	4a 8d 34 38          	lea    rsi,[rax+r15*1]
 4001ee9:	48 8b 85 e0 fe ff ff 	mov    rax,QWORD PTR [rbp-0x120]
 4001ef0:	4a 8d 3c 38          	lea    rdi,[rax+r15*1]
 4001ef4:	31 c0                	xor    eax,eax
 4001ef6:	41 ff d1             	call   r9
 4001ef9:	eb ae                	jmp    4001ea9 <panic+0x229>
 4001efb:	0f 1f 44 00 00       	nop    DWORD PTR [rax+rax*1+0x0]

0000000004001f00 <printf>:
 4001f00:	49 bb 08 21 00 00 00 	movabs r11,0x2108
 4001f07:	00 00 00 
 4001f0a:	55                   	push   rbp
 4001f0b:	48 89 e5             	mov    rbp,rsp
 4001f0e:	41 57                	push   r15
 4001f10:	4c 8d 3d e9 ff ff ff 	lea    r15,[rip+0xffffffffffffffe9]        # 4001f00 <printf>
 4001f17:	41 56                	push   r14
 4001f19:	4d 01 df             	add    r15,r11
 4001f1c:	41 55                	push   r13
 4001f1e:	41 54                	push   r12
 4001f20:	49 89 fc             	mov    r12,rdi
 4001f23:	53                   	push   rbx
 4001f24:	48 81 ec e8 00 00 00 	sub    rsp,0xe8
 4001f2b:	48 89 b5 28 ff ff ff 	mov    QWORD PTR [rbp-0xd8],rsi
 4001f32:	48 89 95 30 ff ff ff 	mov    QWORD PTR [rbp-0xd0],rdx
 4001f39:	48 89 8d 38 ff ff ff 	mov    QWORD PTR [rbp-0xc8],rcx
 4001f40:	4c 89 85 40 ff ff ff 	mov    QWORD PTR [rbp-0xc0],r8
 4001f47:	4c 89 8d 48 ff ff ff 	mov    QWORD PTR [rbp-0xb8],r9
 4001f4e:	84 c0                	test   al,al
 4001f50:	74 29                	je     4001f7b <printf+0x7b>
 4001f52:	0f 29 85 50 ff ff ff 	movaps XMMWORD PTR [rbp-0xb0],xmm0
 4001f59:	0f 29 8d 60 ff ff ff 	movaps XMMWORD PTR [rbp-0xa0],xmm1
 4001f60:	0f 29 95 70 ff ff ff 	movaps XMMWORD PTR [rbp-0x90],xmm2
 4001f67:	0f 29 5d 80          	movaps XMMWORD PTR [rbp-0x80],xmm3
 4001f6b:	0f 29 65 90          	movaps XMMWORD PTR [rbp-0x70],xmm4
 4001f6f:	0f 29 6d a0          	movaps XMMWORD PTR [rbp-0x60],xmm5
 4001f73:	0f 29 75 b0          	movaps XMMWORD PTR [rbp-0x50],xmm6
 4001f77:	0f 29 7d c0          	movaps XMMWORD PTR [rbp-0x40],xmm7
 4001f7b:	48 8d 45 10          	lea    rax,[rbp+0x10]
 4001f7f:	41 bd 01 00 00 00    	mov    r13d,0x1
 4001f85:	c7 85 08 ff ff ff 08 	mov    DWORD PTR [rbp-0xf8],0x8
 4001f8c:	00 00 00 
 4001f8f:	49 be f8 0f 00 00 00 	movabs r14,0xff8
 4001f96:	00 00 00 
 4001f99:	48 89 85 10 ff ff ff 	mov    QWORD PTR [rbp-0xf0],rax
 4001fa0:	48 8d 85 20 ff ff ff 	lea    rax,[rbp-0xe0]
 4001fa7:	48 bb a8 e5 ff ff ff 	movabs rbx,0xffffffffffffe5a8
 4001fae:	ff ff ff 
 4001fb1:	c7 85 0c ff ff ff 30 	mov    DWORD PTR [rbp-0xf4],0x30
 4001fb8:	00 00 00 
 4001fbb:	48 89 85 18 ff ff ff 	mov    QWORD PTR [rbp-0xe8],rax
 4001fc2:	66 0f 1f 44 00 00    	nop    WORD PTR [rax+rax*1+0x0]
 4001fc8:	44 89 ea             	mov    edx,r13d
 4001fcb:	43 86 14 3e          	xchg   BYTE PTR [r14+r15*1],dl
 4001fcf:	84 d2                	test   dl,dl
 4001fd1:	0f 85 09 01 00 00    	jne    40020e0 <printf+0x1e0>
 4001fd7:	43 80 7c 3e 01 00    	cmp    BYTE PTR [r14+r15*1+0x1],0x0
 4001fdd:	0f 85 0d 01 00 00    	jne    40020f0 <printf+0x1f0>
 4001fe3:	43 c6 44 3e 01 01    	mov    BYTE PTR [r14+r15*1+0x1],0x1
 4001fe9:	4a 8d 04 3b          	lea    rax,[rbx+r15*1]
 4001fed:	ff d0                	call   rax
 4001fef:	43 89 44 3e 04       	mov    DWORD PTR [r14+r15*1+0x4],eax
 4001ff4:	31 c0                	xor    eax,eax
 4001ff6:	43 86 04 3e          	xchg   BYTE PTR [r14+r15*1],al
 4001ffa:	48 c7 c2 ff ff ff ff 	mov    rdx,0xffffffffffffffff
 4002001:	48 b8 38 dc ff ff ff 	movabs rax,0xffffffffffffdc38
 4002008:	ff ff ff 
 400200b:	49 8d 3c 07          	lea    rdi,[r15+rax*1]
 400200f:	4c 89 e1             	mov    rcx,r12
 4002012:	48 b8 18 d0 ff ff ff 	movabs rax,0xffffffffffffd018
 4002019:	ff ff ff 
 400201c:	48 8d b5 07 ff ff ff 	lea    rsi,[rbp-0xf9]
 4002023:	4c 8d 85 08 ff ff ff 	lea    r8,[rbp-0xf8]
 400202a:	4c 01 f8             	add    rax,r15
 400202d:	ff d0                	call   rax
 400202f:	41 89 c4             	mov    r12d,eax
 4002032:	b8 01 00 00 00       	mov    eax,0x1
 4002037:	eb 09                	jmp    4002042 <printf+0x142>
 4002039:	0f 1f 80 00 00 00 00 	nop    DWORD PTR [rax+0x0]
 4002040:	f3 90                	pause
 4002042:	41 89 c5             	mov    r13d,eax
 4002045:	47 86 2c 3e          	xchg   BYTE PTR [r14+r15*1],r13b
 4002049:	45 84 ed             	test   r13b,r13b
 400204c:	75 f2                	jne    4002040 <printf+0x140>
 400204e:	43 80 7c 3e 01 00    	cmp    BYTE PTR [r14+r15*1+0x1],0x0
 4002054:	74 18                	je     400206e <printf+0x16e>
 4002056:	43 8b 44 3e 04       	mov    eax,DWORD PTR [r14+r15*1+0x4]
 400205b:	4c 01 fb             	add    rbx,r15
 400205e:	89 85 fc fe ff ff    	mov    DWORD PTR [rbp-0x104],eax
 4002064:	ff d3                	call   rbx
 4002066:	39 85 fc fe ff ff    	cmp    DWORD PTR [rbp-0x104],eax
 400206c:	74 45                	je     40020b3 <printf+0x1b3>
 400206e:	48 b8 a9 f4 ff ff ff 	movabs rax,0xfffffffffffff4a9
 4002075:	ff ff ff 
 4002078:	48 8b 4d 08          	mov    rcx,QWORD PTR [rbp+0x8]
 400207c:	ba 6c 03 00 00       	mov    edx,0x36c
 4002081:	4d 8d 04 07          	lea    r8,[r15+rax*1]
 4002085:	48 b8 80 f4 ff ff ff 	movabs rax,0xfffffffffffff480
 400208c:	ff ff ff 
 400208f:	49 b9 78 dc ff ff ff 	movabs r9,0xffffffffffffdc78
 4002096:	ff ff ff 
 4002099:	49 8d 34 07          	lea    rsi,[r15+rax*1]
 400209d:	4d 01 f9             	add    r9,r15
 40020a0:	48 b8 91 f4 ff ff ff 	movabs rax,0xfffffffffffff491
 40020a7:	ff ff ff 
 40020aa:	49 8d 3c 07          	lea    rdi,[r15+rax*1]
 40020ae:	31 c0                	xor    eax,eax
 40020b0:	41 ff d1             	call   r9
 40020b3:	43 c6 44 3e 01 00    	mov    BYTE PTR [r14+r15*1+0x1],0x0
 40020b9:	43 c7 44 3e 04 00 00 	mov    DWORD PTR [r14+r15*1+0x4],0x0
 40020c0:	00 00 
 40020c2:	47 86 2c 3e          	xchg   BYTE PTR [r14+r15*1],r13b
 40020c6:	48 81 c4 e8 00 00 00 	add    rsp,0xe8
 40020cd:	44 89 e0             	mov    eax,r12d
 40020d0:	5b                   	pop    rbx
 40020d1:	41 5c                	pop    r12
 40020d3:	41 5d                	pop    r13
 40020d5:	41 5e                	pop    r14
 40020d7:	41 5f                	pop    r15
 40020d9:	5d                   	pop    rbp
 40020da:	c3                   	ret
 40020db:	0f 1f 44 00 00       	nop    DWORD PTR [rax+rax*1+0x0]
 40020e0:	f3 90                	pause
 40020e2:	e9 e1 fe ff ff       	jmp    4001fc8 <printf+0xc8>
 40020e7:	66 0f 1f 84 00 00 00 	nop    WORD PTR [rax+rax*1+0x0]
 40020ee:	00 00 
 40020f0:	43 8b 44 3e 04       	mov    eax,DWORD PTR [r14+r15*1+0x4]
 40020f5:	88 95 fb fe ff ff    	mov    BYTE PTR [rbp-0x105],dl
 40020fb:	89 85 fc fe ff ff    	mov    DWORD PTR [rbp-0x104],eax
 4002101:	4a 8d 04 3b          	lea    rax,[rbx+r15*1]
 4002105:	ff d0                	call   rax
 4002107:	39 85 fc fe ff ff    	cmp    DWORD PTR [rbp-0x104],eax
 400210d:	0f b6 95 fb fe ff ff 	movzx  edx,BYTE PTR [rbp-0x105]
 4002114:	74 1a                	je     4002130 <printf+0x230>
 4002116:	43 80 7c 3e 01 00    	cmp    BYTE PTR [r14+r15*1+0x1],0x0
 400211c:	0f 84 c1 fe ff ff    	je     4001fe3 <printf+0xe3>
 4002122:	43 86 14 3e          	xchg   BYTE PTR [r14+r15*1],dl
 4002126:	e9 9d fe ff ff       	jmp    4001fc8 <printf+0xc8>
 400212b:	0f 1f 44 00 00       	nop    DWORD PTR [rax+rax*1+0x0]
 4002130:	48 b8 6a f4 ff ff ff 	movabs rax,0xfffffffffffff46a
 4002137:	ff ff ff 
 400213a:	48 8b 4d 08          	mov    rcx,QWORD PTR [rbp+0x8]
 400213e:	ba 6a 03 00 00       	mov    edx,0x36a
 4002143:	4d 8d 04 07          	lea    r8,[r15+rax*1]
 4002147:	e9 39 ff ff ff       	jmp    4002085 <printf+0x185>
 400214c:	0f 1f 40 00          	nop    DWORD PTR [rax+0x0]

0000000004002150 <sprintf>:
 4002150:	55                   	push   rbp
 4002151:	4c 8d 15 f8 ff ff ff 	lea    r10,[rip+0xfffffffffffffff8]        # 4002150 <sprintf>
 4002158:	49 bb b8 1e 00 00 00 	movabs r11,0x1eb8
 400215f:	00 00 00 
 4002162:	4d 01 da             	add    r10,r11
 4002165:	49 89 fb             	mov    r11,rdi
 4002168:	48 89 e5             	mov    rbp,rsp
 400216b:	41 54                	push   r12
 400216d:	49 89 f4             	mov    r12,rsi
 4002170:	48 81 ec d8 00 00 00 	sub    rsp,0xd8
 4002177:	48 89 95 50 ff ff ff 	mov    QWORD PTR [rbp-0xb0],rdx
 400217e:	48 89 8d 58 ff ff ff 	mov    QWORD PTR [rbp-0xa8],rcx
 4002185:	4c 89 85 60 ff ff ff 	mov    QWORD PTR [rbp-0xa0],r8
 400218c:	4c 89 8d 68 ff ff ff 	mov    QWORD PTR [rbp-0x98],r9
 4002193:	84 c0                	test   al,al
 4002195:	74 23                	je     40021ba <sprintf+0x6a>
 4002197:	0f 29 85 70 ff ff ff 	movaps XMMWORD PTR [rbp-0x90],xmm0
 400219e:	0f 29 4d 80          	movaps XMMWORD PTR [rbp-0x80],xmm1
 40021a2:	0f 29 55 90          	movaps XMMWORD PTR [rbp-0x70],xmm2
 40021a6:	0f 29 5d a0          	movaps XMMWORD PTR [rbp-0x60],xmm3
 40021aa:	0f 29 65 b0          	movaps XMMWORD PTR [rbp-0x50],xmm4
 40021ae:	0f 29 6d c0          	movaps XMMWORD PTR [rbp-0x40],xmm5
 40021b2:	0f 29 75 d0          	movaps XMMWORD PTR [rbp-0x30],xmm6
 40021b6:	0f 29 7d e0          	movaps XMMWORD PTR [rbp-0x20],xmm7
 40021ba:	48 8d 45 10          	lea    rax,[rbp+0x10]
 40021be:	4c 89 e1             	mov    rcx,r12
 40021c1:	4c 89 de             	mov    rsi,r11
 40021c4:	c7 85 28 ff ff ff 10 	mov    DWORD PTR [rbp-0xd8],0x10
 40021cb:	00 00 00 
 40021ce:	48 89 85 30 ff ff ff 	mov    QWORD PTR [rbp-0xd0],rax
 40021d5:	48 8d 85 40 ff ff ff 	lea    rax,[rbp-0xc0]
 40021dc:	4c 8d 85 28 ff ff ff 	lea    r8,[rbp-0xd8]
 40021e3:	48 c7 c2 ff ff ff ff 	mov    rdx,0xffffffffffffffff
 40021ea:	48 89 85 38 ff ff ff 	mov    QWORD PTR [rbp-0xc8],rax
 40021f1:	48 b8 88 c1 ff ff ff 	movabs rax,0xffffffffffffc188
 40021f8:	ff ff ff 
 40021fb:	c7 85 2c ff ff ff 30 	mov    DWORD PTR [rbp-0xd4],0x30
 4002202:	00 00 00 
 4002205:	49 8d 3c 02          	lea    rdi,[r10+rax*1]
 4002209:	48 b8 18 d0 ff ff ff 	movabs rax,0xffffffffffffd018
 4002210:	ff ff ff 
 4002213:	4c 01 d0             	add    rax,r10
 4002216:	ff d0                	call   rax
 4002218:	4c 8b 65 f8          	mov    r12,QWORD PTR [rbp-0x8]
 400221c:	c9                   	leave
 400221d:	c3                   	ret
 400221e:	66 90                	xchg   ax,ax

0000000004002220 <snprintf>:
 4002220:	55                   	push   rbp
 4002221:	4c 8d 15 f8 ff ff ff 	lea    r10,[rip+0xfffffffffffffff8]        # 4002220 <snprintf>
 4002228:	49 bb e8 1d 00 00 00 	movabs r11,0x1de8
 400222f:	00 00 00 
 4002232:	4d 01 da             	add    r10,r11
 4002235:	49 89 fb             	mov    r11,rdi
 4002238:	48 89 e5             	mov    rbp,rsp
 400223b:	41 55                	push   r13
 400223d:	49 89 d5             	mov    r13,rdx
 4002240:	41 54                	push   r12
 4002242:	49 89 f4             	mov    r12,rsi
 4002245:	48 81 ec d0 00 00 00 	sub    rsp,0xd0
 400224c:	48 89 8d 58 ff ff ff 	mov    QWORD PTR [rbp-0xa8],rcx
 4002253:	4c 89 85 60 ff ff ff 	mov    QWORD PTR [rbp-0xa0],r8
 400225a:	4c 89 8d 68 ff ff ff 	mov    QWORD PTR [rbp-0x98],r9
 4002261:	84 c0                	test   al,al
 4002263:	74 23                	je     4002288 <snprintf+0x68>
 4002265:	0f 29 85 70 ff ff ff 	movaps XMMWORD PTR [rbp-0x90],xmm0
 400226c:	0f 29 4d 80          	movaps XMMWORD PTR [rbp-0x80],xmm1
 4002270:	0f 29 55 90          	movaps XMMWORD PTR [rbp-0x70],xmm2
 4002274:	0f 29 5d a0          	movaps XMMWORD PTR [rbp-0x60],xmm3
 4002278:	0f 29 65 b0          	movaps XMMWORD PTR [rbp-0x50],xmm4
 400227c:	0f 29 6d c0          	movaps XMMWORD PTR [rbp-0x40],xmm5
 4002280:	0f 29 75 d0          	movaps XMMWORD PTR [rbp-0x30],xmm6
 4002284:	0f 29 7d e0          	movaps XMMWORD PTR [rbp-0x20],xmm7
 4002288:	48 8d 45 10          	lea    rax,[rbp+0x10]
 400228c:	4c 89 e9             	mov    rcx,r13
 400228f:	4c 89 e2             	mov    rdx,r12
 4002292:	4c 89 de             	mov    rsi,r11
 4002295:	48 89 85 30 ff ff ff 	mov    QWORD PTR [rbp-0xd0],rax
 400229c:	48 8d 85 40 ff ff ff 	lea    rax,[rbp-0xc0]
 40022a3:	4c 8d 85 28 ff ff ff 	lea    r8,[rbp-0xd8]
 40022aa:	48 89 85 38 ff ff ff 	mov    QWORD PTR [rbp-0xc8],rax
 40022b1:	48 b8 88 c1 ff ff ff 	movabs rax,0xffffffffffffc188
 40022b8:	ff ff ff 
 40022bb:	c7 85 28 ff ff ff 18 	mov    DWORD PTR [rbp-0xd8],0x18
 40022c2:	00 00 00 
 40022c5:	49 8d 3c 02          	lea    rdi,[r10+rax*1]
 40022c9:	48 b8 18 d0 ff ff ff 	movabs rax,0xffffffffffffd018
 40022d0:	ff ff ff 
 40022d3:	c7 85 2c ff ff ff 30 	mov    DWORD PTR [rbp-0xd4],0x30
 40022da:	00 00 00 
 40022dd:	4c 01 d0             	add    rax,r10
 40022e0:	ff d0                	call   rax
 40022e2:	48 81 c4 d0 00 00 00 	add    rsp,0xd0
 40022e9:	41 5c                	pop    r12
 40022eb:	41 5d                	pop    r13
 40022ed:	5d                   	pop    rbp
 40022ee:	c3                   	ret
 40022ef:	90                   	nop

00000000040022f0 <vprintf>:
 40022f0:	55                   	push   rbp
 40022f1:	48 8d 05 f8 ff ff ff 	lea    rax,[rip+0xfffffffffffffff8]        # 40022f0 <vprintf>
 40022f8:	48 89 f9             	mov    rcx,rdi
 40022fb:	49 89 f0             	mov    r8,rsi
 40022fe:	49 bb 18 1d 00 00 00 	movabs r11,0x1d18
 4002305:	00 00 00 
 4002308:	48 c7 c2 ff ff ff ff 	mov    rdx,0xffffffffffffffff
 400230f:	49 b9 18 d0 ff ff ff 	movabs r9,0xffffffffffffd018
 4002316:	ff ff ff 
 4002319:	48 bf 38 dc ff ff ff 	movabs rdi,0xffffffffffffdc38
 4002320:	ff ff ff 
 4002323:	4c 01 d8             	add    rax,r11
 4002326:	48 89 e5             	mov    rbp,rsp
 4002329:	48 01 c7             	add    rdi,rax
 400232c:	48 83 ec 10          	sub    rsp,0x10
 4002330:	4c 01 c8             	add    rax,r9
 4002333:	48 8d 75 ff          	lea    rsi,[rbp-0x1]
 4002337:	ff d0                	call   rax
 4002339:	c9                   	leave
 400233a:	c3                   	ret
 400233b:	0f 1f 44 00 00       	nop    DWORD PTR [rax+rax*1+0x0]

0000000004002340 <vsnprintf>:
 4002340:	48 8d 05 f9 ff ff ff 	lea    rax,[rip+0xfffffffffffffff9]        # 4002340 <vsnprintf>
 4002347:	49 89 c8             	mov    r8,rcx
 400234a:	48 89 d1             	mov    rcx,rdx
 400234d:	49 bb c8 1c 00 00 00 	movabs r11,0x1cc8
 4002354:	00 00 00 
 4002357:	49 b9 18 d0 ff ff ff 	movabs r9,0xffffffffffffd018
 400235e:	ff ff ff 
 4002361:	4c 01 d8             	add    rax,r11
 4002364:	48 89 f2             	mov    rdx,rsi
 4002367:	48 89 fe             	mov    rsi,rdi
 400236a:	48 bf 88 c1 ff ff ff 	movabs rdi,0xffffffffffffc188
 4002371:	ff ff ff 
 4002374:	48 01 c7             	add    rdi,rax
 4002377:	4c 01 c8             	add    rax,r9
 400237a:	ff e0                	jmp    rax
 400237c:	0f 1f 40 00          	nop    DWORD PTR [rax+0x0]

0000000004002380 <fctprintf>:
 4002380:	55                   	push   rbp
 4002381:	4c 8d 15 f8 ff ff ff 	lea    r10,[rip+0xfffffffffffffff8]        # 4002380 <fctprintf>
 4002388:	49 bb 88 1c 00 00 00 	movabs r11,0x1c88
 400238f:	00 00 00 
 4002392:	4d 01 da             	add    r10,r11
 4002395:	49 89 d3             	mov    r11,rdx
 4002398:	48 89 e5             	mov    rbp,rsp
 400239b:	48 81 ec e0 00 00 00 	sub    rsp,0xe0
 40023a2:	48 89 8d 68 ff ff ff 	mov    QWORD PTR [rbp-0x98],rcx
 40023a9:	4c 89 85 70 ff ff ff 	mov    QWORD PTR [rbp-0x90],r8
 40023b0:	4c 89 8d 78 ff ff ff 	mov    QWORD PTR [rbp-0x88],r9
 40023b7:	84 c0                	test   al,al
 40023b9:	74 20                	je     40023db <fctprintf+0x5b>
 40023bb:	0f 29 45 80          	movaps XMMWORD PTR [rbp-0x80],xmm0
 40023bf:	0f 29 4d 90          	movaps XMMWORD PTR [rbp-0x70],xmm1
 40023c3:	0f 29 55 a0          	movaps XMMWORD PTR [rbp-0x60],xmm2
 40023c7:	0f 29 5d b0          	movaps XMMWORD PTR [rbp-0x50],xmm3
 40023cb:	0f 29 65 c0          	movaps XMMWORD PTR [rbp-0x40],xmm4
 40023cf:	0f 29 6d d0          	movaps XMMWORD PTR [rbp-0x30],xmm5
 40023d3:	0f 29 75 e0          	movaps XMMWORD PTR [rbp-0x20],xmm6
 40023d7:	0f 29 7d f0          	movaps XMMWORD PTR [rbp-0x10],xmm7
 40023db:	48 8d 45 10          	lea    rax,[rbp+0x10]
 40023df:	4c 89 d9             	mov    rcx,r11
 40023e2:	48 89 bd 20 ff ff ff 	mov    QWORD PTR [rbp-0xe0],rdi
 40023e9:	4c 8d 85 38 ff ff ff 	lea    r8,[rbp-0xc8]
 40023f0:	48 89 85 40 ff ff ff 	mov    QWORD PTR [rbp-0xc0],rax
 40023f7:	48 8d 85 50 ff ff ff 	lea    rax,[rbp-0xb0]
 40023fe:	48 c7 c2 ff ff ff ff 	mov    rdx,0xffffffffffffffff
 4002405:	48 89 85 48 ff ff ff 	mov    QWORD PTR [rbp-0xb8],rax
 400240c:	48 b8 f8 cf ff ff ff 	movabs rax,0xffffffffffffcff8
 4002413:	ff ff ff 
 4002416:	49 8d 3c 02          	lea    rdi,[r10+rax*1]
 400241a:	48 b8 18 d0 ff ff ff 	movabs rax,0xffffffffffffd018
 4002421:	ff ff ff 
 4002424:	48 89 b5 28 ff ff ff 	mov    QWORD PTR [rbp-0xd8],rsi
 400242b:	48 8d b5 20 ff ff ff 	lea    rsi,[rbp-0xe0]
 4002432:	c7 85 38 ff ff ff 18 	mov    DWORD PTR [rbp-0xc8],0x18
 4002439:	00 00 00 
 400243c:	4c 01 d0             	add    rax,r10
 400243f:	c7 85 3c ff ff ff 30 	mov    DWORD PTR [rbp-0xc4],0x30
 4002446:	00 00 00 
 4002449:	ff d0                	call   rax
 400244b:	c9                   	leave
 400244c:	c3                   	ret
 400244d:	0f 1f 00             	nop    DWORD PTR [rax]

0000000004002450 <sys_putc>:
 4002450:	b8 00 00 00 00       	mov    eax,0x0
 4002455:	cd 80                	int    0x80
 4002457:	c3                   	ret

0000000004002458 <sys_close>:
 4002458:	b8 01 00 00 00       	mov    eax,0x1
 400245d:	cd 80                	int    0x80
 400245f:	c3                   	ret

0000000004002460 <sys_unlink>:
 4002460:	b8 02 00 00 00       	mov    eax,0x2
 4002465:	cd 80                	int    0x80
 4002467:	c3                   	ret

0000000004002468 <sys_dup>:
 4002468:	b8 03 00 00 00       	mov    eax,0x3
 400246d:	cd 80                	int    0x80
 400246f:	c3                   	ret

0000000004002470 <sys_dup2>:
 4002470:	b8 04 00 00 00       	mov    eax,0x4
 4002475:	cd 80                	int    0x80
 4002477:	c3                   	ret

0000000004002478 <sys_truncate>:
 4002478:	b8 05 00 00 00       	mov    eax,0x5
 400247d:	cd 80                	int    0x80
 400247f:	c3                   	ret

0000000004002480 <sys_fcntl>:
 4002480:	b8 06 00 00 00       	mov    eax,0x6
 4002485:	cd 80                	int    0x80
 4002487:	c3                   	ret

0000000004002488 <sys_ioctl>:
 4002488:	b8 07 00 00 00       	mov    eax,0x7
 400248d:	cd 80                	int    0x80
 400248f:	c3                   	ret

0000000004002490 <sys_lseek>:
 4002490:	b8 08 00 00 00       	mov    eax,0x8
 4002495:	cd 80                	int    0x80
 4002497:	c3                   	ret

0000000004002498 <sys_read>:
 4002498:	b8 09 00 00 00       	mov    eax,0x9
 400249d:	cd 80                	int    0x80
 400249f:	c3                   	ret

00000000040024a0 <sys_write>:
 40024a0:	b8 0a 00 00 00       	mov    eax,0xa
 40024a5:	cd 80                	int    0x80
 40024a7:	c3                   	ret

00000000040024a8 <sys_open>:
 40024a8:	b8 0b 00 00 00       	mov    eax,0xb
 40024ad:	cd 80                	int    0x80
 40024af:	c3                   	ret

00000000040024b0 <sys_openat>:
 40024b0:	b8 4c 00 00 00       	mov    eax,0x4c
 40024b5:	cd 80                	int    0x80
 40024b7:	c3                   	ret

00000000040024b8 <sys_create>:
 40024b8:	b8 0c 00 00 00       	mov    eax,0xc
 40024bd:	cd 80                	int    0x80
 40024bf:	c3                   	ret

00000000040024c0 <sys_mkdirat>:
 40024c0:	b8 0d 00 00 00       	mov    eax,0xd
 40024c5:	cd 80                	int    0x80
 40024c7:	c3                   	ret

00000000040024c8 <sys_readdir>:
 40024c8:	b8 0e 00 00 00       	mov    eax,0xe
 40024cd:	cd 80                	int    0x80
 40024cf:	c3                   	ret

00000000040024d0 <sys_linkat>:
 40024d0:	b8 0f 00 00 00       	mov    eax,0xf
 40024d5:	cd 80                	int    0x80
 40024d7:	c3                   	ret

00000000040024d8 <sys_mknodat>:
 40024d8:	b8 10 00 00 00       	mov    eax,0x10
 40024dd:	cd 80                	int    0x80
 40024df:	c3                   	ret

00000000040024e0 <sys_sync>:
 40024e0:	b8 11 00 00 00       	mov    eax,0x11
 40024e5:	cd 80                	int    0x80
 40024e7:	c3                   	ret

00000000040024e8 <sys_getattr>:
 40024e8:	b8 12 00 00 00       	mov    eax,0x12
 40024ed:	cd 80                	int    0x80
 40024ef:	c3                   	ret

00000000040024f0 <sys_setattr>:
 40024f0:	b8 13 00 00 00       	mov    eax,0x13
 40024f5:	cd 80                	int    0x80
 40024f7:	c3                   	ret

00000000040024f8 <sys_fstat>:
 40024f8:	b8 3f 00 00 00       	mov    eax,0x3f
 40024fd:	cd 80                	int    0x80
 40024ff:	c3                   	ret

0000000004002500 <sys_stat>:
 4002500:	b8 40 00 00 00       	mov    eax,0x40
 4002505:	cd 80                	int    0x80
 4002507:	c3                   	ret

0000000004002508 <sys_lstat>:
 4002508:	b8 41 00 00 00       	mov    eax,0x41
 400250d:	cd 80                	int    0x80
 400250f:	c3                   	ret

0000000004002510 <sys_fstatat>:
 4002510:	b8 42 00 00 00       	mov    eax,0x42
 4002515:	cd 80                	int    0x80
 4002517:	c3                   	ret

0000000004002518 <sys_uname>:
 4002518:	b8 44 00 00 00       	mov    eax,0x44
 400251d:	cd 80                	int    0x80
 400251f:	c3                   	ret

0000000004002520 <sys_chown>:
 4002520:	b8 45 00 00 00       	mov    eax,0x45
 4002525:	cd 80                	int    0x80
 4002527:	c3                   	ret

0000000004002528 <sys_fchown>:
 4002528:	b8 46 00 00 00       	mov    eax,0x46
 400252d:	cd 80                	int    0x80
 400252f:	c3                   	ret

0000000004002530 <sys_gettimeofday>:
 4002530:	b8 47 00 00 00       	mov    eax,0x47
 4002535:	cd 80                	int    0x80
 4002537:	c3                   	ret

0000000004002538 <sys_umask>:
 4002538:	b8 48 00 00 00       	mov    eax,0x48
 400253d:	cd 80                	int    0x80
 400253f:	c3                   	ret

0000000004002540 <sys_isatty>:
 4002540:	b8 49 00 00 00       	mov    eax,0x49
 4002545:	cd 80                	int    0x80
 4002547:	c3                   	ret

0000000004002548 <sys_park>:
 4002548:	b8 14 00 00 00       	mov    eax,0x14
 400254d:	cd 80                	int    0x80
 400254f:	c3                   	ret

0000000004002550 <sys_unpark>:
 4002550:	b8 15 00 00 00       	mov    eax,0x15
 4002555:	cd 80                	int    0x80
 4002557:	c3                   	ret

0000000004002558 <sys_fork>:
 4002558:	b8 36 00 00 00       	mov    eax,0x36
 400255d:	cd 80                	int    0x80
 400255f:	c3                   	ret

0000000004002560 <sys_waitpid>:
 4002560:	b8 3e 00 00 00       	mov    eax,0x3e
 4002565:	cd 80                	int    0x80
 4002567:	c3                   	ret

0000000004002568 <sys_wait>:
 4002568:	b8 43 00 00 00       	mov    eax,0x43
 400256d:	cd 80                	int    0x80
 400256f:	c3                   	ret

0000000004002570 <sys_exit>:
 4002570:	b8 16 00 00 00       	mov    eax,0x16
 4002575:	cd 80                	int    0x80
 4002577:	c3                   	ret

0000000004002578 <sys_getpid>:
 4002578:	b8 17 00 00 00       	mov    eax,0x17
 400257d:	cd 80                	int    0x80
 400257f:	c3                   	ret

0000000004002580 <sys_getppid>:
 4002580:	b8 18 00 00 00       	mov    eax,0x18
 4002585:	cd 80                	int    0x80
 4002587:	c3                   	ret

0000000004002588 <sys_sleep>:
 4002588:	b8 19 00 00 00       	mov    eax,0x19
 400258d:	cd 80                	int    0x80
 400258f:	c3                   	ret

0000000004002590 <sys_gettid>:
 4002590:	b8 1a 00 00 00       	mov    eax,0x1a
 4002595:	cd 80                	int    0x80
 4002597:	c3                   	ret

0000000004002598 <sys_thread_exit>:
 4002598:	b8 1b 00 00 00       	mov    eax,0x1b
 400259d:	cd 80                	int    0x80
 400259f:	c3                   	ret

00000000040025a0 <sys_thread_create>:
 40025a0:	b8 1c 00 00 00       	mov    eax,0x1c
 40025a5:	cd 80                	int    0x80
 40025a7:	c3                   	ret

00000000040025a8 <sys_thread_join>:
 40025a8:	b8 1d 00 00 00       	mov    eax,0x1d
 40025ad:	cd 80                	int    0x80
 40025af:	c3                   	ret

00000000040025b0 <sys_thread_self>:
 40025b0:	b8 28 00 00 00       	mov    eax,0x28
 40025b5:	cd 80                	int    0x80
 40025b7:	c3                   	ret

00000000040025b8 <sys_thread_yield>:
 40025b8:	b8 2c 00 00 00       	mov    eax,0x2c
 40025bd:	cd 80                	int    0x80
 40025bf:	c3                   	ret

00000000040025c0 <sys_pause>:
 40025c0:	b8 1e 00 00 00       	mov    eax,0x1e
 40025c5:	cd 80                	int    0x80
 40025c7:	c3                   	ret

00000000040025c8 <sys_kill>:
 40025c8:	b8 1f 00 00 00       	mov    eax,0x1f
 40025cd:	cd 80                	int    0x80
 40025cf:	c3                   	ret

00000000040025d0 <sys_alarm>:
 40025d0:	b8 20 00 00 00       	mov    eax,0x20
 40025d5:	cd 80                	int    0x80
 40025d7:	c3                   	ret

00000000040025d8 <sys_signal>:
 40025d8:	b8 21 00 00 00       	mov    eax,0x21
 40025dd:	cd 80                	int    0x80
 40025df:	c3                   	ret

00000000040025e0 <sys_sigprocmask>:
 40025e0:	b8 22 00 00 00       	mov    eax,0x22
 40025e5:	cd 80                	int    0x80
 40025e7:	c3                   	ret

00000000040025e8 <sys_sigpending>:
 40025e8:	b8 23 00 00 00       	mov    eax,0x23
 40025ed:	cd 80                	int    0x80
 40025ef:	c3                   	ret

00000000040025f0 <sys_sigaction>:
 40025f0:	b8 24 00 00 00       	mov    eax,0x24
 40025f5:	cd 80                	int    0x80
 40025f7:	c3                   	ret

00000000040025f8 <sys_pthread_kill>:
 40025f8:	b8 25 00 00 00       	mov    eax,0x25
 40025fd:	cd 80                	int    0x80
 40025ff:	c3                   	ret

0000000004002600 <sys_sigwait>:
 4002600:	b8 26 00 00 00       	mov    eax,0x26
 4002605:	cd 80                	int    0x80
 4002607:	c3                   	ret

0000000004002608 <sys_pthread_sigmask>:
 4002608:	b8 27 00 00 00       	mov    eax,0x27
 400260d:	cd 80                	int    0x80
 400260f:	c3                   	ret

0000000004002610 <sys_mmap>:
 4002610:	b8 29 00 00 00       	mov    eax,0x29
 4002615:	cd 80                	int    0x80
 4002617:	c3                   	ret

0000000004002618 <sys_munmap>:
 4002618:	b8 2a 00 00 00       	mov    eax,0x2a
 400261d:	cd 80                	int    0x80
 400261f:	c3                   	ret

0000000004002620 <sys_mprotect>:
 4002620:	b8 2b 00 00 00       	mov    eax,0x2b
 4002625:	cd 80                	int    0x80
 4002627:	c3                   	ret

0000000004002628 <sys_getpagesize>:
 4002628:	b8 2d 00 00 00       	mov    eax,0x2d
 400262d:	cd 80                	int    0x80
 400262f:	c3                   	ret

0000000004002630 <sys_getmemusage>:
 4002630:	b8 37 00 00 00       	mov    eax,0x37
 4002635:	cd 80                	int    0x80
 4002637:	c3                   	ret

0000000004002638 <sys_getuid>:
 4002638:	b8 2e 00 00 00       	mov    eax,0x2e
 400263d:	cd 80                	int    0x80
 400263f:	c3                   	ret

0000000004002640 <sys_getgid>:
 4002640:	b8 2f 00 00 00       	mov    eax,0x2f
 4002645:	cd 80                	int    0x80
 4002647:	c3                   	ret

0000000004002648 <sys_geteuid>:
 4002648:	b8 30 00 00 00       	mov    eax,0x30
 400264d:	cd 80                	int    0x80
 400264f:	c3                   	ret

0000000004002650 <sys_getegid>:
 4002650:	b8 31 00 00 00       	mov    eax,0x31
 4002655:	cd 80                	int    0x80
 4002657:	c3                   	ret

0000000004002658 <sys_setuid>:
 4002658:	b8 32 00 00 00       	mov    eax,0x32
 400265d:	cd 80                	int    0x80
 400265f:	c3                   	ret

0000000004002660 <sys_setgid>:
 4002660:	b8 33 00 00 00       	mov    eax,0x33
 4002665:	cd 80                	int    0x80
 4002667:	c3                   	ret

0000000004002668 <sys_seteuid>:
 4002668:	b8 34 00 00 00       	mov    eax,0x34
 400266d:	cd 80                	int    0x80
 400266f:	c3                   	ret

0000000004002670 <sys_setegid>:
 4002670:	b8 35 00 00 00       	mov    eax,0x35
 4002675:	cd 80                	int    0x80
 4002677:	c3                   	ret

0000000004002678 <sys_getcwd>:
 4002678:	b8 4a 00 00 00       	mov    eax,0x4a
 400267d:	cd 80                	int    0x80
 400267f:	c3                   	ret

0000000004002680 <sys_chdir>:
 4002680:	b8 4b 00 00 00       	mov    eax,0x4b
 4002685:	cd 80                	int    0x80
 4002687:	c3                   	ret

0000000004002688 <sys_getsid>:
 4002688:	b8 38 00 00 00       	mov    eax,0x38
 400268d:	cd 80                	int    0x80
 400268f:	c3                   	ret

0000000004002690 <sys_setsid>:
 4002690:	b8 39 00 00 00       	mov    eax,0x39
 4002695:	cd 80                	int    0x80
 4002697:	c3                   	ret

0000000004002698 <sys_getpgrp>:
 4002698:	b8 3a 00 00 00       	mov    eax,0x3a
 400269d:	cd 80                	int    0x80
 400269f:	c3                   	ret

00000000040026a0 <sys_setpgrp>:
 40026a0:	b8 3c 00 00 00       	mov    eax,0x3c
 40026a5:	cd 80                	int    0x80
 40026a7:	c3                   	ret

00000000040026a8 <sys_getpgid>:
 40026a8:	b8 3b 00 00 00       	mov    eax,0x3b
 40026ad:	cd 80                	int    0x80
 40026af:	c3                   	ret

00000000040026b0 <sys_setpgid>:
 40026b0:	b8 3d 00 00 00       	mov    eax,0x3d
 40026b5:	cd 80                	int    0x80
 40026b7:	c3                   	ret

Disassembly of section .text.startup:

00000000040026c0 <main>:
 40026c0:	48 b8 b0 25 00 04 00 	movabs rax,0x40025b0
 40026c7:	00 00 00 
 40026ca:	55                   	push   rbp
 40026cb:	48 89 e5             	mov    rbp,rsp
 40026ce:	53                   	push   rbx
 40026cf:	48 bb f8 25 00 04 00 	movabs rbx,0x40025f8
 40026d6:	00 00 00 
 40026d9:	48 83 ec 18          	sub    rsp,0x18
 40026dd:	c7 45 ec 00 00 00 00 	mov    DWORD PTR [rbp-0x14],0x0
 40026e4:	ff d0                	call   rax
 40026e6:	31 f6                	xor    esi,esi
 40026e8:	48 8d 7d ec          	lea    rdi,[rbp-0x14]
 40026ec:	48 ba 80 00 00 04 00 	movabs rdx,0x4000080
 40026f3:	00 00 00 
 40026f6:	48 63 c8             	movsxd rcx,eax
 40026f9:	48 b8 a0 25 00 04 00 	movabs rax,0x40025a0
 4002700:	00 00 00 
 4002703:	ff d0                	call   rax
 4002705:	48 b8 48 25 00 04 00 	movabs rax,0x4002548
 400270c:	00 00 00 
 400270f:	ff d0                	call   rax
 4002711:	8b 7d ec             	mov    edi,DWORD PTR [rbp-0x14]
 4002714:	be 16 00 00 00       	mov    esi,0x16
 4002719:	ff d3                	call   rbx
 400271b:	8b 7d ec             	mov    edi,DWORD PTR [rbp-0x14]
 400271e:	be 1b 00 00 00       	mov    esi,0x1b
 4002723:	ff d3                	call   rbx
 4002725:	8b 7d ec             	mov    edi,DWORD PTR [rbp-0x14]
 4002728:	be 0b 00 00 00       	mov    esi,0xb
 400272d:	ff d3                	call   rbx
 400272f:	8b 7d ec             	mov    edi,DWORD PTR [rbp-0x14]
 4002732:	be 1c 00 00 00       	mov    esi,0x1c
 4002737:	ff d3                	call   rbx
 4002739:	8b 7d ec             	mov    edi,DWORD PTR [rbp-0x14]
 400273c:	be 0b 00 00 00       	mov    esi,0xb
 4002741:	ff d3                	call   rbx
 4002743:	31 c0                	xor    eax,eax
 4002745:	48 bf 49 34 00 04 00 	movabs rdi,0x4003449
 400274c:	00 00 00 
 400274f:	48 ba 00 1f 00 04 00 	movabs rdx,0x4001f00
 4002756:	00 00 00 
 4002759:	ff d2                	call   rdx
 400275b:	eb fe                	jmp    400275b <main+0x9b>

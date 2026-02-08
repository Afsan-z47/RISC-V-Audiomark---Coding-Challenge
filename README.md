# RISC-V Audiomark Coding Challenge: Saturating Multiply-Accumulate Implementation

> [!NOTE]
> Github link : https://github.com/Afsan-z47/RISC-V-Audiomark---Coding-Challenge

## Overview
This repository contains an optimized implementation of a saturating multiply-accumulate function (`q15_axpy`) using RISC-V Vector (RVV) intrinsics. The implementation targets Q15 fixed-point arithmetic with saturation.

## Repo structure
```text
riscv-audiomark-challenge/
├── README.md
├── note.txt                  # Holds commands usually used
├── .gitignore
├── q15_axpy_challenge.c      # My running C file
├── q15_axpy_original.c       # Code Challenge defined syntaxed 
└── q15_axpy_optimized.c      # Shows the 28 register usage varient
```

## Challenge Description
Implement `y[i] = saturate_q15(a[i] + alpha * b[i])` where:
- Inputs: Q15 fixed-point arrays (int16_t)
- Operation: Multiply-accumulate with saturation
- Target: RISC-V RVV 1.0 architecture

# Design Decisions for RISC-V Vector Implementation

## Algorithm Analysis
The core operation is: `y[i] = saturate_q15(a[i] + alpha * b[i])`

### Mathematical Properties
- Q15 format: values in range [-32768, 32767]
- Multiplication result: fits in 32 bits (16 × 16)
- Accumulation: requires 32-bit intermediate
- Saturation: clip to Q15 range

## Vectorization Strategy

### 1. Vector Length Selection
- Use `vsetvl` to determine maximum vector length
- Process in chunks to handle any array size
- Chosen LMUL=4 for better most amount of instruction chaining.
- Did not choose LMUL=8, limited by architecture. No such inst. for sign-extension to 16 registers.

### 2. Data Flow

Load a[i] (16-bit) → Sign extend to 32-bit
Load b[i] (16-bit) → Multiply by alpha (vwmacc)
Add extended a[i] → 32-bit result
Saturate to 16-bit (vnclip) → Store y[i]

### 3. Key Intrinsics Used
- `__riscv_vle16_v_i16m4`: Load 16-bit vectors
- `__riscv_vsext_vf2_i32m8`: Sign extend to 32-bit
- `__riscv_vwmacc_vx_i32m8`: Widening multiply-accumulate
- `__riscv_vnclip_wx_i16m4`: Narrowing with saturation
- `__riscv_vse16_v_i16m4`: Store results

### 4. Saturation Implementation Choice

`vnclip` instruction
- Pros: Single instruction for saturation

> **Chosen**: `vnclip` with RDN (round down) as it seemed less heavy on the hardware.



## Performance Optimizations

### 1. Loop Unrolling
Not implemented because:
- Increases code complexity
- Leaving it up to the compiler.
- Branch Prediction should be good enough.

### 2. Memory Access Pattern
- Did not use pointer arithmetic for memory access. Left it up to the compiler. Code would get more complex.
- Sequential access → good for prefetching
- Could handle any amount data.

### 3. Register Usage
- Sign extension used LMUL = 8
- Registers used = 16.

> Priorotized reducing instruction count

## Edge Case Handling

When Array Size Not Multiple of VL
- Handled by `vsetvl` in loop condition
- Final iteration processes remaining (n-i) = vl elements

## Build

```bash
riscv32-unknown-elf-gcc -march=rv32imcbv -mabi=ilp32 -O2 src/q15_axpy_challenge.c -o q15_axpy.elf
```


Generated Assmebly:
```asm

000102f0 <q15_axpy_ref>:
   102f0:	02d05b63          	blez	a3,10326 <q15_axpy_ref+0x36>
   102f4:	20a6a6b3          	sh1add	a3,a3,a0
   102f8:	68a1                	lui	a7,0x8
   102fa:	7e61                	lui	t3,0xffff8
   102fc:	00059783          	lh	a5,0(a1)
   10300:	00051303          	lh	t1,0(a0)
   10304:	fff88813          	addi	a6,a7,-1 # 7fff <main-0x80b5>
   10308:	02e787b3          	mul	a5,a5,a4
   1030c:	979a                	add	a5,a5,t1
   1030e:	0117d563          	bge	a5,a7,10318 <q15_axpy_ref+0x28>
   10312:	01c7cb63          	blt	a5,t3,10328 <q15_axpy_ref+0x38>
   10316:	883e                	mv	a6,a5
   10318:	01061023          	sh	a6,0(a2)
   1031c:	0509                	addi	a0,a0,2
   1031e:	0589                	addi	a1,a1,2
   10320:	0609                	addi	a2,a2,2
   10322:	fcd51de3          	bne	a0,a3,102fc <q15_axpy_ref+0xc>
   10326:	8082                	ret
   10328:	7861                	lui	a6,0xffff8
   1032a:	b7fd                	j	10318 <q15_axpy_ref+0x28>

00010320 <q15_axpy_rvv>: #NO SPIKE ADJUSTMENT
   10320:	00a15073          	csrwi	vxrm,2
   10324:	ce8d                	beqz	a3,1035e <q15_axpy_rvv+0x3e>
   10326:	4801                	li	a6,0
   10328:	410687b3          	sub	a5,a3,a6
   1032c:	0d37f7d7          	vsetvli	a5,a5,e32,m8,ta,ma
   10330:	20a828b3          	sh1add	a7,a6,a0
   10334:	0208d807          	vle16.v	v16,(a7)
   10338:	20b828b3          	sh1add	a7,a6,a1
   1033c:	0208d207          	vle16.v	v4,(a7)
   10340:	20c828b3          	sh1add	a7,a6,a2
   10344:	983e                	add	a6,a6,a5
   10346:	4b03a457          	vsext.vf2	v8,v16
   1034a:	0ca07057          	vsetvli	zero,zero,e16,m4,ta,ma
   1034e:	f6476457          	vwmacc.vx	v8,a4,v4
   10352:	be803457          	vnclip.wi	v8,v8,0
   10356:	0208d427          	vse16.v	v8,(a7)
   1035a:	fcd867e3          	bltu	a6,a3,10328 <q15_axpy_rvv+0x8>
   1035e:	8082                	ret


0001032c <q15_axpy_rvv>: #SPIKE PK ADJUSTMENT
   1032c:	00a15073          	csrwi	vxrm,2
   10330:	caa1                	beqz	a3,10380 <q15_axpy_rvv+0x54>
   10332:	4801                	li	a6,0
   10334:	a019                	j	1033a <q15_axpy_rvv+0xe>
   10336:	00a15073          	csrwi	vxrm,2
   1033a:	410687b3          	sub	a5,a3,a6
   1033e:	20a828b3          	sh1add	a7,a6,a0
   10342:	0d37f7d7          	vsetvli	a5,a5,e32,m8,ta,ma
   10346:	0208d807          	vle16.v	v16,(a7)
   1034a:	20b828b3          	sh1add	a7,a6,a1
   1034e:	0208d207          	vle16.v	v4,(a7)
   10352:	20c828b3          	sh1add	a7,a6,a2
   10356:	4b03a457          	vsext.vf2	v8,v16
   1035a:	0ca07057          	vsetvli	zero,zero,e16,m4,ta,ma
   1035e:	f6476457          	vwmacc.vx	v8,a4,v4
   10362:	be803457          	vnclip.wi	v8,v8,0
   10366:	0208d427          	vse16.v	v8,(a7)
   1036a:	c2002373          	csrr	t1,vl
   1036e:	00f37663          	bgeu	t1,a5,1037a <q15_axpy_rvv+0x4e>
   10372:	0c17f7d7          	vsetvli	a5,a5,e8,m2,ta,ma
   10376:	0208d427          	vse16.v	v8,(a7)
   1037a:	983e                	add	a6,a6,a5
   1037c:	fad86de3          	bltu	a6,a3,10336 <q15_axpy_rvv+0xa>
   10380:	8082                	ret
```


# Performance Analysis

## Theoretical Speedup Calculation

### Scalar Implementation
```
Per element:
1. Load a[i]          (1 cycle)
2. Load b[i]          (1 cycle)
3. Multiply           (1-5 cycles)
4. Add                (1 cycle)
5. Compare for sat    (2 cycles)
6. Conditional move   (1-2 cycles)
7. Store              (1 cycle)
Total: ~8-12 cycles/element
```

### Vector Implementation (VLEN=128, LMUL=4)
```
Per VL elements (8 elements with LMUL=4):
1. vsetvl            (1 cycle)
2. vle16 x2          (2 cycles)
3. vsext             (1 cycle)
4. vwmacc            (2-4 cycles)
5. vnclip            (2 cycles)
6. vse16             (1 cycle)
Total: ~9 cycles / 32 elements = 0.28125 cycles/element
```

### Expected Speedup
```
Scalar: 10 cycles/element
Vector: 0.28125 cycles/element
Speedup: ~35.56x
```

But the vector instructions could be favored heavily in this calculation without considering many implementation constraints.

The following calculation could be more realistic considering LMUL = 1
```
Scalar: 10 cycles/element
Vector: 1.125 cycles/element # about 4 times more than LMUL = 4
Speedup: ~8.9x
```

## Actual Measurements

> Simulator Measurements are seems really inaccurate compared to our theoratical checks.

### Test Configuration (Spike)
- Simulator: Spike with RVV 1.0
- VLEN: 128 bits
- Array size: 4096 elements
- Alpha: 3
```bash
$ spike --isa=rv32imcbv_Zicntr /opt/riscv/PK/riscv32-unknown-elf/bin/pk q15_axpy.elf
Cycles ref: 53661
Verify RVV: OK (max diff = 0)
Cycles RVV: 114952

```
> Speed up = 0.466812235x

### Test Configuration (QEMU)
- Simulator: QEMU with RVV 1.0
- VLEN: 128 bits
- Array size: 4096 elements
- Alpha: 3

```bash
$ qemu-riscv32 q15_axpy.elf 
Cycles ref: 314805
Verify RVV: OK (max diff = 0)
Cycles RVV: 114952

```
> Speedup:                  2.86x

## Optimization Opportunities

### 1. Loop Unrolling
Process multiple VL per iteration to reduce `vsetvl` overhead

```c
size_t i = 0;
while(i < n) {
    size_t vl = __riscv_vsetvl_e16m4(n-i);

    vint16m4_t va = __riscv_vle16_v_i16m4(&a[i], vl);
    vint16m4_t va2 = __riscv_vle16_v_i16m4(&a[i+vl], vl);

    vint16m4_t vb = __riscv_vle16_v_i16m4(&b[i], vl);
    vint16m4_t vb2 = __riscv_vle16_v_i16m4(&b[i+vl], vl);

    vint32m8_t va_extended =  __riscv_vsext_vf2_i32m8(va, vl);
    vint32m8_t va2_extended =  __riscv_vsext_vf2_i32m8(va2, vl);

    vint32m8_t vd = __riscv_vwmacc_vx_i32m8(va_extended, alpha, vb, vl);
    vint32m8_t vd2 = __riscv_vwmacc_vx_i32m8(va2_extended, alpha, vb2, vl);

    vint16m4_t sat_vd = __riscv_vnclip_wx_i16m4(vd, 0, __RISCV_VXRM_RDN, vl);
    vint16m4_t sat_vd2 = __riscv_vnclip_wx_i16m4(vd2, 0, __RISCV_VXRM_RDN, vl);

    __riscv_vse16_v_i16m4(&y[i], sat_vd, vl);
    __riscv_vse16_v_i16m4(&y[i+vl], sat_vd2, vl);

    i += vl*2; // should have taken proper measures to check remaining elements in case its not a mode of vl.
}
```

> ***The generated assembly with this code filled 28 registers during the process where as the normal one would leave 16 of them.***

```asm
00010320 <q15_axpy_rvv>:
   10320:	00a15073          	csrwi	vxrm,2
   10324:	c6b5                	beqz	a3,10390 <q15_axpy_rvv+0x70>
   10326:	4801                	li	a6,0
   10328:	410687b3          	sub	a5,a3,a6
   1032c:	0d37f7d7          	vsetvli	a5,a5,e32,m8,ta,ma
   10330:	20a828b3          	sh1add	a7,a6,a0
   10334:	0208d207          	vle16.v	v4,(a7)
   10338:	20b828b3          	sh1add	a7,a6,a1
   1033c:	0208de07          	vle16.v	v28,(a7)
   10340:	010788b3          	add	a7,a5,a6
   10344:	20a8a333          	sh1add	t1,a7,a0
   10348:	02035c07          	vle16.v	v24,(t1)
   1034c:	4a43a857          	vsext.vf2	v16,v4
   10350:	20b8a333          	sh1add	t1,a7,a1
   10354:	02035207          	vle16.v	v4,(t1)
   10358:	0ca07057          	vsetvli	zero,zero,e16,m4,ta,ma
   1035c:	f7c76857          	vwmacc.vx	v16,a4,v28
   10360:	0d307057          	vsetvli	zero,zero,e32,m8,ta,ma
   10364:	4b83a457          	vsext.vf2	v8,v24
   10368:	0ca07057          	vsetvli	zero,zero,e16,m4,ta,ma
   1036c:	bf003857          	vnclip.wi	v16,v16,0
   10370:	20c82e33          	sh1add	t3,a6,a2
   10374:	f6476457          	vwmacc.vx	v8,a4,v4
   10378:	020e5827          	vse16.v	v16,(t3)
   1037c:	20c8a333          	sh1add	t1,a7,a2
   10380:	00f88833          	add	a6,a7,a5
   10384:	be803457          	vnclip.wi	v8,v8,0
   10388:	02035427          	vse16.v	v8,(t1)
   1038c:	f8d86ee3          	bltu	a6,a3,10328 <q15_axpy_rvv+0x8>
   10390:	8082                	ret
```


## Conclusion
The RVV implementation achieves significant speedup over scalar code. The key factors are:
1. Efficient use of widening/narrowing operations
2. Hardware saturation support via `vnclip`
3. Amortization of loop overhead over vector elements

## Changes in Header includes
I had to change some parts of the header as my toolchain did not seem to detect __riscv_v_intrinsic >= 1000000

I will keep the code with the code in `q15_axpy_original.c` with the original syntaxes intact.

## Problems faced in emulation
In Spike encountered page boundary issue and the code would go into a trap and will rewrite the vl, resulting in vse16.vx instructions to not follow the old VL=32, rather follows VL=4.
```bash
spike --isa=rv32imcbv_Zicntr -d /opt/riscv/PK/riscv32-unknown-elf/bin/pk q15_axpy.elf
```

```bash
core   0: 0x0001034e (0xf6476457) vwmacc.vx v8, a4, v4
(spike) 
core   0: 0x00010352 (0xbe803457) vnclip.wi v8, v8, 0
(spike) 
core   0: 0x00010356 (0x0208d427) vse16.v v8, (a7)
core   0: exception trap_store_page_fault, epc 0x00010356
core   0:           tval 0x0001b000
(spike) 
core   0: >>>>  trap_entry
core   0: 0x80001fd8 (0x14011173) csrrw   sp, sscratch, sp
(spike) 
core   0: 0x80001fdc (0x00011463) bnez    sp, pc + 8
(spike) reg 0 vl
0x00000020
(spike) until pc 0 10356
(spike) 
core   0: 0x00010356 (0x0208d427) vse16.v v8, (a7)
(spike) reg 0 vl        
0x00000004
(spike) 
core   0: 0x0001035a (0xfcd867e3) bltu    a6, a3, pc - 50
(spike) 
```

So I checked the csr for spike pk but this problem did not occur for QEMU. So, my resultant code looked like this:

```c
size_t i = 0;
while(i < n) {
    size_t vl = __riscv_vsetvl_e16m4(n-i);

    vint16m4_t va = __riscv_vle16_v_i16m4(&a[i], vl);
    vint16m4_t vb = __riscv_vle16_v_i16m4(&b[i], vl);
    vint32m8_t va_extended =  __riscv_vsext_vf2_i32m8(va, vl);

    vint32m8_t vd = __riscv_vwmacc_vx_i32m8(va_extended, alpha, vb, vl);

    vint16m4_t sat_vd = __riscv_vnclip_wx_i16m4(vd, 0, __RISCV_VXRM_RDN, vl);

    __riscv_vse16_v_i16m4(&y[i], sat_vd, vl);

    size_t trap_vl;
    asm volatile ("csrr %0, vl" : "=r"(trap_vl) : : "memory");
    if (trap_vl < vl){
        //NOTE: RETRY
        vl = __riscv_vsetvl_e16m4(vl);
        __riscv_vse16_v_i16m4(&y[i], sat_vd, vl);
    }
    i += vl;
}
```

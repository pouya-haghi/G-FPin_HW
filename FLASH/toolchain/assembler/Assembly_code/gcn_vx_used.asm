// v0 holds the results and v1 holds the weight (output stationary)
// x5 holds tm
// x6 holds tm_trip_count (fixed number)

// x7 holds the HBM address
// x28 offset for incrementing HBM address
// if the number of iterations is bigger than 2048 then use for loop otherwise, you can omit the for loop

// x29 holds tk
// x30 holds tk_trip_count

// x31 holds m
// x1 holds TM 

// 0 until 0x1000 always holds immediate values
// starting address of HBM data is 0x20000
// assuming HBM memory is byte-addressable

            // LOOP_TM
            addi x5, x0, 0x000 // init to zero
            addi x6, x0, 0x001 // 4 (TM)
            // LOOP_TK
            addi x30, x0, 0x017 // was 32 before (TK)
            addi x29, x0, 0x000 // init to zero
            // LOOP_M
            // addi x1, x0, 0x010 // 16
            // addi x31, x0, 0x000 // init to zero

            // configuring VLEN // based on RF access pattern (for example lets say you have an offset and then a for loop (like TRF) you can determine 3-bit VLEN 
            // then you know which index you should use for VRF
            // vsetivli zero, 256, e32, m2

            addi x28, x0, 0x800 /// equal to 2 K (unroll factor (VLEN for vsetivli)*64 Bytes (b/c of 512bits)) -- you have to increment vle base address by this 2K amount (see the below code)

LOOP_TM:    vsetivli zero, 1024, e32, m2 // has to be equal to vmacc_VLEN (EXACT)
            vle32.v v1, (x0) // vectorization for LOOP_TRF - re-init v1 to zero 
            addi x29, x0, 0x000 // re-init to zero
            lui x7, 0x00020 // x7 is now equal to 0x20000 which is 128K Bytes in decimal (this is enough to fill in all 2K entries of VRF *64 Bytes each entry)
LOOP_TK:    vsetivli zero, 32, e32, m2 // has to be equal to unroll factor
            vle32.v v0, (x7) // vectorization for LOOP_K - weights
            add x7, x7, x28 // LOOP_K

            vsetivli zero, 3, e32, m2// LOOP_M (has to be equal to vmacc_VLEN) (INEXACT)
            vmacc.vx v1, x0, vs // vs is stream_in 
            vmacc.vx v1, x1, vs
            vmacc.vx v1, x2, vs
            vmacc.vx v1, x3, vs
            vmacc.vx v1, x4, vs
            vmacc.vx v1, x5, vs
            vmacc.vx v1, x6, vs
            vmacc.vx v1, x7, vs
            vmacc.vx v1, x8, vs
            vmacc.vx v1, x9, vs
            vmacc.vx v1, x10, vs
            vmacc.vx v1, x11, vs
            vmacc.vx v1, x12, vs
            vmacc.vx v1, x13, vs
            vmacc.vx v1, x14, vs
            vmacc.vx v1, x15, vs
            vmacc.vx v1, x16, vs
            vmacc.vx v1, x17, vs
            vmacc.vx v1, x18, vs
            vmacc.vx v1, x19, vs
            vmacc.vx v1, x20, vs
            vmacc.vx v1, x21, vs
            vmacc.vx v1, x22, vs
            vmacc.vx v1, x23, vs
            vmacc.vx v1, x24, vs
            vmacc.vx v1, x25, vs
            vmacc.vx v1, x26, vs
            vmacc.vx v1, x27, vs
            vmacc.vx v1, x28, vs
            vmacc.vx v1, x29, vs
            vmacc.vx v1, x30, vs
            vmacc.vx v1, x31, vs

            addi x29, x29, 0x001 // LOOP_TK
            bne x29, x30, LOOP_TK // LOOP_TK
            
            vsetivli zero, 1024, e32, m2// has to be equal to vmacc vlen EXACT
            streamout.v v1 // LOOP_MSOUT
            
            addi x5, x5, 0x001 // LOOP_TM
            bne x5, x6, LOOP_TM // LOOP_TM
wfi

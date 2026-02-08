// -------------------- RVV implementation (mentees edit only here) ---------
void q15_axpy_rvv(const int16_t *a, const int16_t *b, int16_t *y, int n, int16_t alpha)
{	
	size_t i = 0;
	while(i < n) {
		size_t vl = __riscv_vsetvl_e16m4(n-i);

		//NOTE: Fault Only First Loads
		//vint16m8_t __riscv_vle16ff_v_i16m8(const int16_t *rs1, size_t *new_vl,size_t vl)
	
		vint16m4_t va = __riscv_vle16_v_i16m4(&a[i], vl);
		vint16m4_t vb = __riscv_vle16_v_i16m4(&b[i], vl);
		//NOTE: Sign extend the destination for multiply-add instruction
		vint32m8_t va_extended =  __riscv_vsext_vf2_i32m8(va, vl);

		//NOTE: (N bit * N bit) + N bit result won't overflow 2N bit
		//NOTE: vd[i] = +(x[rs1] * vs2[i]) + vd[i]
		// vint32m8_t vd = __riscv_vwmacc_va_i32m8(vint32m8_t vd, int16_t rs1, vint16m8_t vs2, size_t vl);
		vint32m8_t vd = __riscv_vwmacc_vx_i32m8(va_extended, alpha, vb, vl);

		//NOTE: Now clip it , varm = round down if that does improve performance
		//vint16m8_t __riscv_vnclip_wx_i16m8(vint32m8_t vs2, size_t rs1, unsigned int varm, size_t vl);
		vint16m4_t sat_vd = __riscv_vnclip_wx_i16m4(vd, 0, __RISCV_VXRM_RDN, vl);

		__riscv_vse16_v_i16m4(&y[i], sat_vd, vl);
		size_t trap_vl = __riscv_read_vl();

		if (trap_vl < vl){
			//NOTE: RETRY
			vl = __riscv_vsetvl_e16m4(vl);
			__riscv_vse16_v_i16m4(&y[i], sat_vd, vl);
		}
		
		i += vl;
	}
}



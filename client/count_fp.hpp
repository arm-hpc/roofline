#include <math.h>

// Counts how many Floating Point operations the given instructions microarchitecturally executes.



// Implementation for Arm and x86_64
#ifdef FLOATING_POINTS_ARM
int count_operations_per_instr(instr_t *instr){
	int opc = instr_get_opcode(instr);
	switch(opc){
	      // Single Floating Point operations
	      case OP_fabd:
	      case OP_fabs:
	      case OP_facge:
	      case OP_facgt:
	      case OP_fadd:
	      case OP_faddp:
	      case OP_fcmeq:
	      case OP_fcmge:
	      case OP_fcmgt:
	      case OP_fdiv:
	      case OP_fmax:
	      case OP_fmaxnm:
	      case OP_fmaxnmp:
	      case OP_fmaxp:
	      case OP_fmin:
	      case OP_fminnm:
	      case OP_fminnmp:
	      case OP_fminp:
	      case OP_fmul:
	      case OP_fmulx:
	      case OP_fneg:
	      case OP_frecps:
	      case OP_frsqrts:
	      case OP_fsqrt:
		      return 1;
	      // Fused Floating Point Operations
	      case OP_fmadd:
	      case OP_fmla:
	      case OP_fmlal:
	      case OP_fmlal2:
	      case OP_fmls:
	      case OP_fmlsl:
	      case OP_fmlsl2:
	      case OP_fmsub:
	      case OP_fnmadd:
	      case OP_fnmsub:
	      case OP_fnmul:
		      return 2;
	      // Vector operations TODO(Andrea)
	      // Fused Vector operations TODO(Andrea)

	      default:
		      return 0;
     }
}
#endif


#ifdef FLOATING_POINTS_X86

int count_operations_per_instr(instr_t *instr){
    int opc = instr_get_opcode(instr);

    switch (opc) {

    case OP_ucomiss:
    case OP_ucomisd:
    case OP_comiss:
    case OP_comisd:
    case OP_movmskps:
    case OP_movmskpd:
    case OP_sqrtps:
    case OP_sqrtss:
    case OP_sqrtpd:
    case OP_sqrtsd:
    case OP_rsqrtps:
    case OP_rsqrtss:
    case OP_rcpps:
    case OP_rcpss:
    case OP_andps:
    case OP_andpd:
    case OP_andnps:
    case OP_andnpd:
    case OP_orps:
    case OP_orpd:
    case OP_xorps:
    case OP_xorpd:
    case OP_addps:
    case OP_addss:
    case OP_addpd:
    case OP_addsd:
    case OP_mulps:
    case OP_mulss:
    case OP_mulpd:
    case OP_mulsd:
    case OP_subps:
    case OP_subss:
    case OP_subpd:
    case OP_subsd:
    case OP_minps:
    case OP_minss:
    case OP_minpd:
    case OP_minsd:
    case OP_divps:
    case OP_divss:
    case OP_divpd:
    case OP_divsd:
    case OP_maxps:
    case OP_maxss:
    case OP_maxpd:
    case OP_maxsd:
    case OP_cmpps:
    case OP_cmpss:
    case OP_cmppd:
    case OP_cmpsd:

    case OP_fadd:
    case OP_fmul:
    case OP_fcom:
    case OP_fcomp:
    case OP_fsub:
    case OP_fsubr:
    case OP_fdiv:
    case OP_fdivr:
    case OP_fiadd:
    case OP_fimul:
    case OP_ficom:
    case OP_ficomp:
    case OP_fisub:
    case OP_fisubr:
    case OP_fidiv:
    case OP_fidivr:
    case OP_fxch:
    case OP_fnop:
    case OP_fchs:
    case OP_fabs:
    case OP_ftst:
    case OP_fxam:
    case OP_fld1:
    case OP_fldl2t:
    case OP_fldl2e:
    case OP_fldpi:
    case OP_fldlg2:
    case OP_fldln2:
    case OP_fldz:
    case OP_f2xm1:
    case OP_fyl2x:
    case OP_fptan:
    case OP_fpatan:
    case OP_fxtract:
    case OP_fprem1:
    case OP_fdecstp:
    case OP_fincstp:
    case OP_fprem:
    case OP_fyl2xp1:
    case OP_fsqrt:
    case OP_fsincos:
    case OP_frndint:
    case OP_fscale:
    case OP_fsin:
    case OP_fcos:
    case OP_fcmovb:
    case OP_fcmove:
    case OP_fcmovbe:
    case OP_fcmovu:
    case OP_fucompp:
    case OP_fcmovnb:
    case OP_fcmovne:
    case OP_fcmovnbe:
    case OP_fcmovnu:
    case OP_fucomi:
    case OP_fcomi:
    case OP_ffree:
    case OP_fucom:
    case OP_fucomp:
    case OP_faddp:
    case OP_fmulp:
    case OP_fcompp:
    case OP_fsubrp:
    case OP_fsubp:
    case OP_fdivrp:
    case OP_fdivp:
    case OP_fucomip:
    case OP_fcomip:
    case OP_ffreep:

    /* SSE3/3D-Now!/SSE4 */
    case OP_haddpd:
    case OP_haddps:
    case OP_hsubpd:
    case OP_hsubps:
    case OP_addsubpd:
    case OP_addsubps:
    case OP_femms:
    case OP_movntss:
    case OP_movntsd:
    case OP_blendvps:
    case OP_blendvpd:
    case OP_roundps:
    case OP_roundpd:
    case OP_roundss:
    case OP_roundsd:
    case OP_blendps:
    case OP_blendpd:
    case OP_dpps:
    case OP_dppd:

    /* AVX */
    case OP_vucomiss:
    case OP_vucomisd:
    case OP_vcomiss:
    case OP_vcomisd:
    case OP_vmovmskps:
    case OP_vmovmskpd:
    case OP_vsqrtps:
    case OP_vsqrtss:
    case OP_vsqrtpd:
    case OP_vsqrtsd:
    case OP_vrsqrtps:
    case OP_vrsqrtss:
    case OP_vrcpps:
    case OP_vrcpss:
    case OP_vandps:
    case OP_vandpd:
    case OP_vandnps:
    case OP_vandnpd:
    case OP_vorps:
    case OP_vorpd:
    case OP_vxorps:
    case OP_vxorpd:
    case OP_vaddps:
    case OP_vaddss:
    case OP_vaddpd:
    case OP_vaddsd:
    case OP_vmulps:
    case OP_vmulss:
    case OP_vmulpd:
    case OP_vmulsd:
    case OP_vsubps:
    case OP_vsubss:
    case OP_vsubpd:
    case OP_vsubsd:
    case OP_vminps:
    case OP_vminss:
    case OP_vminpd:
    case OP_vminsd:
    case OP_vdivps:
    case OP_vdivss:
    case OP_vdivpd:
    case OP_vdivsd:
    case OP_vmaxps:
    case OP_vmaxss:
    case OP_vmaxpd:
    case OP_vmaxsd:
    case OP_vcmpps:
    case OP_vcmpss:
    case OP_vcmppd:
    case OP_vcmpsd:
    case OP_vhaddpd:
    case OP_vhaddps:
    case OP_vhsubpd:
    case OP_vhsubps:
    case OP_vaddsubpd:
    case OP_vaddsubps:
    case OP_vblendvps:
    case OP_vblendvpd:
    case OP_vroundps:
    case OP_vroundpd:
    case OP_vroundss:
    case OP_vroundsd:
    case OP_vblendps:
    case OP_vblendpd:
    case OP_vdpps:
    case OP_vdppd:
    case OP_vtestps:
    case OP_vtestpd:

    /* FMA */
    case OP_vfmadd132ps:
    case OP_vfmadd132pd:
    case OP_vfmadd213ps:
    case OP_vfmadd213pd:
    case OP_vfmadd231ps:
    case OP_vfmadd231pd:
    case OP_vfmadd132ss:
    case OP_vfmadd132sd:
    case OP_vfmadd213ss:
    case OP_vfmadd213sd:
    case OP_vfmadd231ss:
    case OP_vfmadd231sd:
    case OP_vfmaddsub132ps:
    case OP_vfmaddsub132pd:
    case OP_vfmaddsub213ps:
    case OP_vfmaddsub213pd:
    case OP_vfmaddsub231ps:
    case OP_vfmaddsub231pd:
    case OP_vfmsubadd132ps:
    case OP_vfmsubadd132pd:
    case OP_vfmsubadd213ps:
    case OP_vfmsubadd213pd:
    case OP_vfmsubadd231ps:
    case OP_vfmsubadd231pd:
    case OP_vfmsub132ps:
    case OP_vfmsub132pd:
    case OP_vfmsub213ps:
    case OP_vfmsub213pd:
    case OP_vfmsub231ps:
    case OP_vfmsub231pd:
    case OP_vfmsub132ss:
    case OP_vfmsub132sd:
    case OP_vfmsub213ss:
    case OP_vfmsub213sd:
    case OP_vfmsub231ss:
    case OP_vfmsub231sd:
    case OP_vfnmadd132ps:
    case OP_vfnmadd132pd:
    case OP_vfnmadd213ps:
    case OP_vfnmadd213pd:
    case OP_vfnmadd231ps:
    case OP_vfnmadd231pd:
    case OP_vfnmadd132ss:
    case OP_vfnmadd132sd:
    case OP_vfnmadd213ss:
    case OP_vfnmadd213sd:
    case OP_vfnmadd231ss:
    case OP_vfnmadd231sd:
    case OP_vfnmsub132ps:
    case OP_vfnmsub132pd:
    case OP_vfnmsub213ps:
    case OP_vfnmsub213pd:
    case OP_vfnmsub231ps:
    case OP_vfnmsub231pd:
    case OP_vfnmsub132ss:
    case OP_vfnmsub132sd:
    case OP_vfnmsub213ss:
    case OP_vfnmsub213sd:
    case OP_vfnmsub231ss:
    case OP_vfnmsub231sd:
	    return 1;

    default: return 0;
    }
}


#endif

//TODO: Check out for other possible registers
//TODO: This function can be improved way better using the DynamoRIO API. You should try differnt API calls.
//This functions checks out whether the given instruction is a FP one or not.
//by taking into account what kind of registers it's using.
bool is_vector_instruction(instr_t *instr){
	const char* name = get_register_name(opnd_get_reg(instr_get_dst(instr,0)));
	//Check if it's a vector instruction
	if(name[0] == 'q'){
		return true;
	}
	return false;

}

//TODO: This requires a proper structure, remove these ifdef and just keep those only where you really need them.

#ifdef FLOATING_POINTS_ARM
uint32_t count_fp_instr(instr_t *instr){
	int operations_per_instr = count_operations_per_instr(instr);
	//Check if it's a floating point operation
	if(operations_per_instr > 0){
		if(is_vector_instruction(instr)){
			// As described in http://dynamorio.org/docs/API_BT.html under 'AArch64 IR Variations',
			// we expect to find an additional immediate source operand to denote the width of vector registers.
			opnd_t width_operand = instr_get_src(instr, instr_num_srcs(instr) -1);
			DR_ASSERT_MSG(opnd_is_immed_int(width_operand), "ERROR: Roofline Client - I'm expecting immediated value specifying width of elements at the end of NEON instruction\n");
			int elem_width = (int)opnd_get_immed_int(width_operand);
			int reg_size = (int) opnd_size_in_bytes(opnd_get_size(instr_get_dst(instr,0)));
			// TODO(Andrea): Add some examplanation for this.
			int elem_width_in_bytes = (int) pow(2, elem_width);
			int num_elems = reg_size / elem_width_in_bytes;
			//TODO: I expect this division to do not have any remainder. Add further control?
			// TODO: You are returning an integer, not an unsigned int 32
			return num_elems * operations_per_instr;
		}

		else{
			// If it's a scalar instruction, just return the number of operations counted from count_operations_per_instr
			return operations_per_instr;
		}
	}
	// else, if it's not a floating point instructions, it will return 0.
	return 0;
}

#endif

// TODO: For X86, this is a really poor analysis, you're missing out all the benefits from vectorization and having instructions that fuse together multiple operations, such multiply/add
#ifdef FLOATING_POINTS_X86
uint32_t count_fp_instr(instr_t *instr){
	return (uint32_t) count_operations_per_instr(instr);
}
#endif

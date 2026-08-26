/*
 * 任务上下文切换
 */
#define PC_OFFSET				(0 * 4)
#define X1_RA_OFFSET			(1 * 4)
#define X2_SP_OFFSET			(2 * 4)
#define X3_GP_OFFSET			(3 * 4)
#define X4_TP_OFFSET			(4 * 4)
#define X5_T0_OFFSET			(5 * 4)
#define X6_T1_OFFSET			(6 * 4)
#define X7_T2_OFFSET			(7 * 4)
#define X8_S0_OFFSET			(8 * 4)
#define X9_S1_OFFSET			(9 * 4)
#define X10_A0_OFFSET			(10 * 4)
#define X11_A1_OFFSET			(11 * 4)
#define X12_A2_OFFSET			(12 * 4)
#define X13_A3_OFFSET			(13 * 4)
#define x14_A4_OFFSET			(14 * 4)
#define X15_A5_OFFSET			(15 * 4)
#define X16_A6_OFFSET			(16 * 4)
#define X17_A7_OFFSET			(17 * 4)
#define X18_S2_OFFSET			(18 * 4)
#define X19_S3_OFFSET			(19 * 4)
#define X20_S4_OFFSET			(20 * 4)
#define X21_S5_OFFSET			(21 * 4)
#define X22_S6_OFFSET			(22 * 4)
#define	X23_S7_OFFSET			(23 * 4)
#define X24_S8_OFFSET			(24 * 4)
#define X25_S9_OFFSET			(25 * 4)
#define X26_S10_OFFSET			(26 * 4)
#define X27_S11_OFFSET			(27 * 4)
#define X28_T3_OFFSET			(28 * 4)
#define X29_T4_OFFSET			(29 * 4)
#define X30_T5_OFFSET			(30 * 4)
#define X31_T6_OFFSET			(31 * 4)
#define MSTATUS_OFFSET			(32 * 4)

#define INTSYSCR				0x804
#define INESTEN					(1<<1)|(1<<0)	//禁止中断嵌套和和硬件压栈
	.text
/**
 * 切换至此第一个任务, 使用mret返回的方式
 * 切换之后，任务默认进入中断开启状态
 * os_task_switch_to(ctx)
 */
	.global os_task_switch_to
os_task_switch_to:
	csrc INTSYSCR,INESTEN	/*禁止中断嵌套，清除INESTEN位
							禁止中断嵌套的原因：因为在发生中断嵌套时，假设在中断开头调用asm("csrrw sp,mscratch,sp")，进行切换
							那么此时发生切换时，sp得到的是任务栈，因为发生两次嵌套，发生两次中断，就交换两次，就得到原来的sp值
							即任务栈，而不是系统栈，所以干脆把中断嵌套给关了*/
	la t0, _eusrstack
	csrw mscratch, t0

    lw t0, PC_OFFSET(a0)
    csrw mepc, t0

	lw t0, MSTATUS_OFFSET(a0)
	csrw mstatus, t0	/*这里设置mstatus的原因是：因为在mret时，会把MPIE的值给MIE，假设在进入函数前是关中断的
						  但是在mret时，会重新把MPIE的值给MIE，导致重新开启了中断*/
						//初始化为1880

	// 恢复所有寄存器,此时这里的a0是os_task_switch_to的第一个参数，也就是os_task_ctx_t * to这个地址值
	lw ra, X1_RA_OFFSET(a0)
	lw sp, X2_SP_OFFSET(a0)
	lw gp, X3_GP_OFFSET(a0)
	lw tp, X4_TP_OFFSET(a0)
	lw t0, X5_T0_OFFSET(a0)
	lw t1, X6_T1_OFFSET(a0)
	lw t2, X7_T2_OFFSET(a0)
	lw s0, X8_S0_OFFSET(a0)
	lw s1, X9_S1_OFFSET(a0)
	lw a1, X11_A1_OFFSET(a0)
	lw a2, X12_A2_OFFSET(a0)
	lw a3, X13_A3_OFFSET(a0)
	lw a4, x14_A4_OFFSET(a0)
	lw a5, X15_A5_OFFSET(a0)
	lw a6, X16_A6_OFFSET(a0)
	lw a7, X17_A7_OFFSET(a0)
	lw s2, X18_S2_OFFSET(a0)
	lw s3, X19_S3_OFFSET(a0)
	lw s4, X20_S4_OFFSET(a0)
	lw s5, X21_S5_OFFSET(a0)
	lw s6, X22_S6_OFFSET(a0)
	lw s7, X23_S7_OFFSET(a0)
	lw s8, X24_S8_OFFSET(a0)
	lw s9, X25_S9_OFFSET(a0)
	lw s10, X26_S10_OFFSET(a0)
	lw s11, X27_S11_OFFSET(a0)
	lw t3, X28_T3_OFFSET(a0)
	lw t4, X29_T4_OFFSET(a0)
	lw t5, X30_T5_OFFSET(a0)
	lw t6, X31_T6_OFFSET(a0)

    /*此时已经把os_task_ctx_t * to的值恢复到cpu寄存器里，但是os_task_ctx_t * to的a0还没有恢复，这里最后恢复a0
      这里a0最后恢复的原因是：前面恢复的值，是基于os_task_ctx_t * to的基地址做偏移进行读写的，最后单独恢复a0的值
      此时恢复后a0的值是任务函数的参数void *param  */
    lw a0, X10_A0_OFFSET(a0)

    mret
/**
 * 软件中断上下文切换，用于在中断里切换任务
 */
	.global SW_Handler
	.extern in_isr_ctx_from,in_isr_ctx_to,sw_clearpend,os_shced_switch_ctx
SW_Handler:
	sw a0, -4(sp)				// 注意，将a0的值暂存到栈中
	lw a0, in_isr_ctx_from			//读取当前任务的ctx位置

	// ----------------------------- 保存任务状态 ----------------------------
	// 保存所有寄存器
	sw ra, X1_RA_OFFSET(a0)
	sw sp, X2_SP_OFFSET(a0)
	sw gp, X3_GP_OFFSET(a0)
	sw tp, X4_TP_OFFSET(a0)
	sw t0, X5_T0_OFFSET(a0)
	sw t1, X6_T1_OFFSET(a0)
	sw t2, X7_T2_OFFSET(a0)
	sw s0, X8_S0_OFFSET(a0)
	sw s1, X9_S1_OFFSET(a0)
	sw a1, X11_A1_OFFSET(a0)
	sw a2, X12_A2_OFFSET(a0)
	sw a3, X13_A3_OFFSET(a0)
	sw a4, x14_A4_OFFSET(a0)
	sw a5, X15_A5_OFFSET(a0)
	sw a6, X16_A6_OFFSET(a0)
	sw a7, X17_A7_OFFSET(a0)
	sw s2, X18_S2_OFFSET(a0)
	sw s3, X19_S3_OFFSET(a0)
	sw s4, X20_S4_OFFSET(a0)
	sw s5, X21_S5_OFFSET(a0)
	sw s6, X22_S6_OFFSET(a0)
	sw s7, X23_S7_OFFSET(a0)
	sw s8, X24_S8_OFFSET(a0)
	sw s9, X25_S9_OFFSET(a0)
	sw s10, X26_S10_OFFSET(a0)
	sw s11, X27_S11_OFFSET(a0)
	sw t3, X28_T3_OFFSET(a0)
	sw t4, X29_T4_OFFSET(a0)
	sw t5, X30_T5_OFFSET(a0)
	sw t6, X31_T6_OFFSET(a0)

	//保存mstatus寄存器，并把保存值的MIE位清除，保证在这些任务切换期间不受中断影响
	csrr t0, mstatus
	andi t0, t0, ~(1 << 3)//相当于 t0 &= ~（1<<3）,注意这里不用把MPIE（bit7）清除，如果外面没有开中断，那么就永远关中断了
	sw t0, MSTATUS_OFFSET(a0)    //1800

	// 异常返回地址在mepc里，读取并保存
	csrr t1, mepc
	sw t1, PC_OFFSET(a0)

	// 再从栈中恢复a0，再保存到ctx
	lw t0, -4(sp)		// 取原来
	sw t0, X10_A0_OFFSET(a0)

	csrrw sp,mscratch,sp			//切换sp，为系统栈
    call  sw_clearpend				// 需要清0，否则持续触发
	call os_shced_switch_ctx
	csrrw sp,mscratch,sp

	// ----------------------------- 恢复任务状态 ----------------------------
	lw a1, in_isr_ctx_to

	// 任务恢复运行的地址，ctx->epc => mepc
	lw t0, PC_OFFSET(a1)
	csrw mepc, t0

	// 恢复下一任务寄存器

	lw t0,MSTATUS_OFFSET(a1)
	csrw mstatus, t0

	lw ra, X1_RA_OFFSET(a1)
	lw sp, X2_SP_OFFSET(a1)
	lw gp, X3_GP_OFFSET(a1)
	lw tp, X4_TP_OFFSET(a1)
	lw t0, X5_T0_OFFSET(a1)
	lw t1, X6_T1_OFFSET(a1)
	lw t2, X7_T2_OFFSET(a1)
	lw s0, X8_S0_OFFSET(a1)
	lw s1, X9_S1_OFFSET(a1)
	lw a0, X10_A0_OFFSET(a1)
	lw a2, X12_A2_OFFSET(a1)
	lw a3, X13_A3_OFFSET(a1)
	lw a4, x14_A4_OFFSET(a1)
	lw a5, X15_A5_OFFSET(a1)
	lw a6, X16_A6_OFFSET(a1)
	lw a7, X17_A7_OFFSET(a1)
	lw s2, X18_S2_OFFSET(a1)
	lw s3, X19_S3_OFFSET(a1)
	lw s4, X20_S4_OFFSET(a1)
	lw s5, X21_S5_OFFSET(a1)
	lw s6, X22_S6_OFFSET(a1)
	lw s7, X23_S7_OFFSET(a1)
	lw s8, X24_S8_OFFSET(a1)
	lw s9, X25_S9_OFFSET(a1)
	lw s10, X26_S10_OFFSET(a1)
	lw s11, X27_S11_OFFSET(a1)
	lw t3, X28_T3_OFFSET(a1)
	lw t4, X29_T4_OFFSET(a1)
	lw t5, X30_T5_OFFSET(a1)
	lw t6, X31_T6_OFFSET(a1)
	lw a1, X11_A1_OFFSET(a1)	// a1放到最后，因为前面用到了a1



	mret


/**
 * 任务级上下文切换，手动执行
 */
	.global os_task_switch
os_task_switch:
	// ----------------------------- 保存任务状态 ----------------------------
	sw ra, X1_RA_OFFSET(a0)
	sw sp, X2_SP_OFFSET(a0)
	sw gp, X3_GP_OFFSET(a0)
	sw tp, X4_TP_OFFSET(a0)
	sw t0, X5_T0_OFFSET(a0)
	sw t1, X6_T1_OFFSET(a0)
	sw t2, X7_T2_OFFSET(a0)
	sw s0, X8_S0_OFFSET(a0)
	sw s1, X9_S1_OFFSET(a0)
	sw a0, X10_A0_OFFSET(a0)
	sw a1, X11_A1_OFFSET(a0)
	sw a2, X12_A2_OFFSET(a0)
	sw a3, X13_A3_OFFSET(a0)
	sw a4, x14_A4_OFFSET(a0)
	sw a5, X15_A5_OFFSET(a0)
	sw a6, X16_A6_OFFSET(a0)
	sw a7, X17_A7_OFFSET(a0)
	sw s2, X18_S2_OFFSET(a0)
	sw s3, X19_S3_OFFSET(a0)
	sw s4, X20_S4_OFFSET(a0)
	sw s5, X21_S5_OFFSET(a0)
	sw s6, X22_S6_OFFSET(a0)
	sw s7, X23_S7_OFFSET(a0)
	sw s8, X24_S8_OFFSET(a0)
	sw s9, X25_S9_OFFSET(a0)
	sw s10, X26_S10_OFFSET(a0)
	sw s11, X27_S11_OFFSET(a0)
	sw t3, X28_T3_OFFSET(a0)
	sw t4, X29_T4_OFFSET(a0)
	sw t5, X30_T5_OFFSET(a0)
	sw t6, X31_T6_OFFSET(a0)
	sw ra, PC_OFFSET(a0)			// save pc

	//保存mstatus寄存器，并把保存值的MIE位清除，保证在这些任务切换期间不受中断影响
	csrr t0, mstatus
	andi t0, t0, ~((1 << 3) | (1 << 7))//相当于 t0 &= ~（1<<3）
	sw t0, MSTATUS_OFFSET(a0)    //1800

	call os_shced_switch_ctx
	
	// 恢复下一任务寄存器
	lw t0, PC_OFFSET(a1)
	csrw mepc, t0

	lw t0,MSTATUS_OFFSET(a1)
	csrw mstatus, t0

	lw ra, X1_RA_OFFSET(a1)
	lw sp, X2_SP_OFFSET(a1)
	lw gp, X3_GP_OFFSET(a1)
	lw tp, X4_TP_OFFSET(a1)
	lw t0, X5_T0_OFFSET(a1)
	lw t1, X6_T1_OFFSET(a1)
	lw t2, X7_T2_OFFSET(a1)
	lw s0, X8_S0_OFFSET(a1)
	lw s1, X9_S1_OFFSET(a1)
	lw a0, X10_A0_OFFSET(a1)
	lw a2, X12_A2_OFFSET(a1)
	lw a3, X13_A3_OFFSET(a1)
	lw a4, x14_A4_OFFSET(a1)
	lw a5, X15_A5_OFFSET(a1)
	lw a6, X16_A6_OFFSET(a1)
	lw a7, X17_A7_OFFSET(a1)
	lw s2, X18_S2_OFFSET(a1)
	lw s3, X19_S3_OFFSET(a1)
	lw s4, X20_S4_OFFSET(a1)
	lw s5, X21_S5_OFFSET(a1)
	lw s6, X22_S6_OFFSET(a1)
	lw s7, X23_S7_OFFSET(a1)
	lw s8, X24_S8_OFFSET(a1)
	lw s9, X25_S9_OFFSET(a1)
	lw s10, X26_S10_OFFSET(a1)
	lw s11, X27_S11_OFFSET(a1)
	lw t3, X28_T3_OFFSET(a1)
	lw t4, X29_T4_OFFSET(a1)
	lw t5, X30_T5_OFFSET(a1)
	lw t6, X31_T6_OFFSET(a1)
	/*同理，最后恢复a1，之前都是以a1作为基准进行偏移加载到cpu寄存器里，最后再把真正的a1值加载到cpu寄存器a1里*/
	lw a1, X11_A1_OFFSET(a1)

	mret


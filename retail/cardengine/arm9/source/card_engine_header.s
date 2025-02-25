@---------------------------------------------------------------------------------
	.section ".init"
@---------------------------------------------------------------------------------
	.global _start
	.global ce9
	.global ndsCodeStart
	.balign	4
	.arm

#define ICACHE_SIZE	0x2000
#define DCACHE_SIZE	0x1000
#define CACHE_LINE_SIZE	32

ce9:
	.word	ce9
patches_offset:
	.word	patches
thumbPatches_offset:
	.word	thumbPatches
intr_ipc_orig_return:
	.word	0x00000000
bootNdsCluster:
	.word	0x00000000
fileCluster:
	.word	0x00000000
saveCluster:
	.word	0x00000000
saveSize:
	.word	0x00000000
romFatTableCache:
	.word	0x00000000
savFatTableCache:
	.word	0x00000000
romFatTableCompressed:
	.hword	0x0000
savFatTableCompressed:
	.hword	0x0000
musicFatTableCache:
	.word	0x00000000
ramDumpCluster:
	.word	0x00000000
srParamsCluster:
	.word	0x00000000
screenshotCluster:
	.word	0x00000000
musicCluster:
	.word	0x00000000
musicsSize:
	.word	0x00000000
pageFileCluster:
	.word	0x00000000
manualCluster:
	.word	0x00000000
sharedFontCluster:
	.word	0x00000000
cardStruct0:
	.word	0x00000000
valueBits:
	.word	0x00000000
s2FlashcardId:
	.hword	0x0000
	.hword	0x0000 @ align
overlaysSize:
	.word	0x00000000
ioverlaysSize:
	.word	0x00000000
irqTable:
	.word	0x00000000
romLocation:
	.word	0x00000000
rumbleFrames:
	.word	30
	.word	30
rumbleForce:
	.word	1
	.word	1
prepareScreenshotPtr:
	.word prepareScreenshot
saveScreenshotPtr:
	.word saveScreenshot
prepareManualPtr:
	.word prepareManual
readManualPtr:
	.word readManual
restorePreManualPtr:
	.word restorePreManual

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

card_engine_start:

ipcSyncHandler:
@ Hook the return address, then go back to the original function
	stmdb	sp!, {lr}
	adr 	lr, code_handler_start_ipc
	ldr 	pc,	intr_ipc_orig_return

code_handler_start_ipc:
	push	{r0-r12} 
	bl		myIrqHandlerIPC @ jump to myIrqHandler
	pop   	{r0-r12,pc} 

.pool

.thumb
ndsCodeStart:
	mov r1, #0
	mov r2, #0
	mov r3, #0
	mov r4, #0
	mov r5, #0
	mov r6, #0
	mov r7, #0
	mov r8, r1
	mov r9, r1
	mov r10, r1
	mov r11, r1
	bx r0

.balign	4
.arm

patches:
.word	card_read_arm9
.word	card_irq_enable
.word	card_pull_out_arm9
.word	card_id_arm9
.word	card_dma_arm9
.word   nand_read_arm9
.word   nand_write_arm9
#ifdef NODSIWARE
.word   0x0
.word   0x0
.word   0x0
.word   0x0
.word   0x0
.word   0x0
.word   0x0
.word   0x0
.word   0x0
.word   0x0
.word   0x0
.word   0x0
.word   0x0
.word   0x0
.word   0x0
.word   0x0
#else
.word   dsiSaveCheckExists_arm
.word   dsiSaveGetResultCode_arm
.word   dsiSaveCreate_arm
.word   dsiSaveDelete_arm
.word   dsiSaveGetInfo_arm
.word   dsiSaveSetLength_arm
.word   dsiSaveOpen_arm
.word   dsiSaveOpenR_arm
.word   dsiSaveClose_arm
.word   dsiSaveGetLength_arm
.word   dsiSaveGetPosition_arm
.word   dsiSaveSeek_arm
.word   dsiSaveRead_arm
.word   dsiSaveWrite_arm
.word   musicPlay_arm
.word   musicStopEffect_arm
#endif
.word	cardStructArm9
.word   card_pull
.word   cacheFlushRef
.word   terminateForPullOutRef
.word   reset_arm9
#ifdef NODSIWARE
.word   0x0
.word   0x0
#else
.word   rumble_arm9
.word   rumble2_arm9
#endif
needFlushDCCache:
.word   0x0
.word   pdash_read
.word	ipcSyncHandler
thumbPatches:
.word	thumb_card_read_arm9
.word	thumb_card_irq_enable
.word	thumb_card_pull_out_arm9
.word	thumb_card_id_arm9
.word	thumb_card_dma_arm9
.word   thumb_nand_read_arm9
.word   thumb_nand_write_arm9
.word	cardStructArm9
.word   thumb_card_pull
.word   cacheFlushRef
.word   terminateForPullOutRef
.word   thumb_reset_arm9

@---------------------------------------------------------------------------------
card_read_arm9:
@---------------------------------------------------------------------------------
	ldr	pc, =cardRead
.pool
cardStructArm9:
.word    0x00000000     
cacheFlushRef:
.word    0x00000000  
terminateForPullOutRef:
.word    0x00000000  
cacheRef:
.word    0x00000000  
	.thumb
@---------------------------------------------------------------------------------
thumb_card_read_arm9:
@---------------------------------------------------------------------------------
	push {r6, lr}
	ldr	r6, =cardRead
    blx	r6
	pop	{r6, pc}
.pool
.balign	4
	.arm
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
card_id_arm9:
@---------------------------------------------------------------------------------
	ldr r0, cardIdData
	bx lr
cardIdData:
.word  0xC2FF01C0
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
card_dma_arm9:
@---------------------------------------------------------------------------------
	mov r0, #0
	bx      lr
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
card_pull_out_arm9:
card_pull:
@---------------------------------------------------------------------------------
	bx      lr
@---------------------------------------------------------------------------------

	.thumb
@---------------------------------------------------------------------------------
thumb_card_id_arm9:
@---------------------------------------------------------------------------------
	ldr r0, cardIdDataT
	bx      lr
.balign	4
cardIdDataT:
.word  0xC2FF01C0
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
thumb_card_dma_arm9:
@---------------------------------------------------------------------------------
	mov r0, #0
	bx      lr		
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
thumb_card_pull_out_arm9:
thumb_card_pull:
@---------------------------------------------------------------------------------
	bx      lr
@---------------------------------------------------------------------------------

	.arm
@---------------------------------------------------------------------------------
nand_read_arm9:
@---------------------------------------------------------------------------------
    ldr pc,= nandRead
.pool
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
nand_write_arm9:
@---------------------------------------------------------------------------------
    ldr pc,= nandWrite
.pool
@---------------------------------------------------------------------------------

#ifndef NODSIWARE
@---------------------------------------------------------------------------------
dsiSaveCheckExists_arm:
@---------------------------------------------------------------------------------
	ldr	pc, =dsiSaveCheckExists
.pool
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
dsiSaveGetResultCode_arm:
@---------------------------------------------------------------------------------
	ldr	pc, =dsiSaveGetResultCode
.pool
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
dsiSaveCreate_arm:
@---------------------------------------------------------------------------------
	ldr	pc, =dsiSaveCreate
.pool
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
dsiSaveDelete_arm:
@---------------------------------------------------------------------------------
	ldr	pc, =dsiSaveDelete
.pool
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
dsiSaveGetInfo_arm:
@---------------------------------------------------------------------------------
	ldr	pc, =dsiSaveGetInfo
.pool
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
dsiSaveSetLength_arm:
@---------------------------------------------------------------------------------
	ldr	pc, =dsiSaveSetLength
.pool
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
dsiSaveOpen_arm:
@---------------------------------------------------------------------------------
	ldr	pc, =dsiSaveOpen
.pool
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
dsiSaveOpenR_arm:
@---------------------------------------------------------------------------------
	mov r2, #1
	ldr	pc, =dsiSaveOpen
.pool
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
dsiSaveClose_arm:
@---------------------------------------------------------------------------------
	ldr	pc, =dsiSaveClose
.pool
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
dsiSaveGetLength_arm:
@---------------------------------------------------------------------------------
	ldr	pc, =dsiSaveGetLength
.pool
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
dsiSaveGetPosition_arm:
@---------------------------------------------------------------------------------
	ldr	pc, =dsiSaveGetPosition
.pool
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
dsiSaveSeek_arm:
@---------------------------------------------------------------------------------
	ldr	pc, =dsiSaveSeek
.pool
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
dsiSaveRead_arm:
@---------------------------------------------------------------------------------
	ldr	pc, =dsiSaveRead
.pool
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
dsiSaveWrite_arm:
@---------------------------------------------------------------------------------
	ldr	pc, =dsiSaveWrite
.pool
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
musicPlay_arm:
@---------------------------------------------------------------------------------
	ldr	pc, =musicPlay
.pool
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
musicStopEffect_arm:
@---------------------------------------------------------------------------------
	ldr	pc, =musicStopEffect
.pool
@---------------------------------------------------------------------------------
#endif

	.thumb
@---------------------------------------------------------------------------------
thumb_nand_read_arm9:
@---------------------------------------------------------------------------------
    push	{r6, lr}

	ldr		r6, =nandRead
    blx	r6

	pop	{r6, pc}
.pool
.balign	4
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
thumb_nand_write_arm9:
@---------------------------------------------------------------------------------
    push	{r6, lr}

	ldr		r6, =nandWrite
    blx	r6

	pop	{r6, pc}
.pool
.balign	4
@---------------------------------------------------------------------------------

	.arm
@---------------------------------------------------------------------------------
card_irq_enable:
@---------------------------------------------------------------------------------
	ldr pc,= myIrqEnable
.pool
@---------------------------------------------------------------------------------

	.thumb
@---------------------------------------------------------------------------------
thumb_card_irq_enable:
@---------------------------------------------------------------------------------
    push	{r6, lr}

	ldr	r6, =myIrqEnable
    blx	r6

	pop	{r6, pc}
.pool
.balign	4
@---------------------------------------------------------------------------------

	.arm
pdash_read:
    push	{r1-r11, lr}
    @mov     r0, r4 @DST
    @mov     r1, r5 @SRC
    @mov     r2, r6 @LEN
    @mov     r3, r10 @cardStruct
    add     r0, r0, #0x2C    
    ldr		r6, =cardReadPDash
    blx	r6
    pop	    {r1-r11, pc}
.pool

	.thumb   
@---------------------------------------------------------------------------------
thumb_reset_arm9:
@---------------------------------------------------------------------------------
    push	{r6, lr}

	ldr	r6, =reset
    blx	r6

	pop	{r6, pc}
.pool
@---------------------------------------------------------------------------------


	.arm
@---------------------------------------------------------------------------------
reset_arm9:
@---------------------------------------------------------------------------------
    ldr pc,= reset
.pool
@---------------------------------------------------------------------------------

#ifndef NODSIWARE
@---------------------------------------------------------------------------------
rumble_arm9:
@---------------------------------------------------------------------------------
	stmfd   sp!, {r1-r11,lr}

	ldr		r6, =rumble
    blx	r6
	nop

	ldmfd   sp!, {r1-r11,pc}
.pool
@---------------------------------------------------------------------------------

@---------------------------------------------------------------------------------
rumble2_arm9:
@---------------------------------------------------------------------------------
    stmfd   sp!, {r1-r11,lr}

	ldr		r6, =rumble2
    blx	r6
	nop

	ldmfd   sp!, {r1-r11,pc}
.pool
@---------------------------------------------------------------------------------
#endif

//---------------------------------------------------------------------------------
.global  getDtcmBase
.type	 getDtcmBase STT_FUNC
/*---------------------------------------------------------------------------------
	getDtcmBase
---------------------------------------------------------------------------------*/
getDtcmBase:
	mrc	p15, 0, r0, c9, c1, 0
	bx	lr


.global cacheFlush
.type	cacheFlush STT_FUNC
cacheFlush:
	stmfd   sp!, {r0-r11,lr}

	@disable interrupt
	ldr r8,= 0x4000208
	ldr r11,[r8]
	mov r7, #0
	str r7, [r8]

//---------------------------------------------------------------------------------
IC_InvalidateAll:
/*---------------------------------------------------------------------------------
	Clean and invalidate entire data cache
---------------------------------------------------------------------------------*/
	mcr	p15, 0, r7, c7, c5, 0

//---------------------------------------------------------------------------------
DC_FlushAll:
/*---------------------------------------------------------------------------------
	Clean and invalidate a range
---------------------------------------------------------------------------------*/
	mov	r1, #0
outer_loop:
	mov	r0, #0
inner_loop:
	orr	r2, r1, r0			@ generate segment and line address
	mcr p15, 0, r7, c7, c10, 4
	mcr	p15, 0, r2, c7, c14, 2		@ clean and flush the line
	add	r0, r0, #CACHE_LINE_SIZE
	cmp	r0, #DCACHE_SIZE/4
	bne	inner_loop
	add	r1, r1, #0x40000000
	cmp	r1, #0
	bne	outer_loop

//---------------------------------------------------------------------------------
DC_WaitWriteBufferEmpty:
//---------------------------------------------------------------------------------               
	MCR     p15, 0, R7,c7,c10, 4

	@restore interrupt
	str r11, [r8]

	ldmfd   sp!, {r0-r11,pc}
	.pool
	

card_engine_end:

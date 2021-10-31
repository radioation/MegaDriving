

*------------------------------------------------
*  LABELS                                        
*------------------------------------------------
		.set VDP_CTRL, 0xC00004
		.set VDP_DATA, 0xC00000
		
		.set PAL0_COLOR1, 0xC0020000
		.set VSCROLL_A,	0x40000010
		.set VSCROLL_B,	0x40020010
		.set HSCROLL_A,	0x7C000002
		.set HSCROLL_B,	0x7C020002

		.set LINE_DARK_COLOR, 0x666
		.set LINE_LIGHT_COLOR, 0xFFF

    
*------------------------------------------------
*  Functions
*------------------------------------------------
  .globl  HInter

HInter:
	move.l	%d0, -(%sp)     /* push device register 0 onto the stack */

	clr.l	%d0               /* Clear D0 and read the current scanline to it */
	move.b	(0xC00008), %d0		


	move.l	%a0, -(%sp)     /* push address register 0 onto the stack  */

	lea VscrollA, %a0         /* get 'vscrollA' array effective address to A0 */
	move.l	#VSCROLL_A, 0xC00004	| Vertical scrolling
	move.b	(%a0, %d0.w), 0xC00000

	lea colors, %a0         /* get 'colors' array effective address to A0 */
 	move.b  (%a0,%d0.w),%d0	/* check shading for current scanline */
	move.l	(%sp)+, %a0     /* restore address register 0 */
	tst.b	%d0               /* check shading value */
	jeq		LIGHT             /* jump to light coloring */

	move.l #PAL0_COLOR1, VDP_CTRL /* Tell VDP we want to change color 1 */
	clr.w %d0

DELAY1: 
	move.w	(VDP_CTRL), %d0		/* wait before we set color (to minimize dots) */
	btst.b  #0x02, %d0	
	beq DELAY1 

	move.w #LINE_DARK_COLOR, VDP_DATA  /* set the color */

	move.l	(%sp)+, %d0   /* restore data register 0 */
  rte

LIGHT:
	move.l #PAL0_COLOR1, VDP_CTRL /* Tell VDP we want to change color 1 */
	clr.w %d0

DELAY2: 
	move.w	(VDP_CTRL), %d0		/* wait before setting color */
	btst.b  #0x02, %d0	
	beq DELAY2 

	move.w #LINE_LIGHT_COLOR, VDP_DATA  /* set the color */

	move.l	(%sp)+, %d0   /* restore data register 0 */
  rte



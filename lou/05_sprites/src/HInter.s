

*------------------------------------------------
*  LABELS                                        
*------------------------------------------------
		.set VDP_CTRL, 0xC00004
		.set VDP_DATA, 0xC00000
		
		.set PAL0_COLOR0, 0xC0000000
    .set PAL0_COLOR1, 0xC0020000
    .set PAL0_COLOR2, 0xC0040000
    .set PAL0_COLOR3, 0xC0060000
    .set PAL0_COLOR4, 0xC0080000

		.set VSCROLL_A,	0x40000010
		.set VSCROLL_B,	0x40020010
		.set HSCROLL_A,	0x7C000002
		.set HSCROLL_B,	0x7C020002

		.set LINE_DARK_COLOR, 0x666
		.set LINE_LIGHT_COLOR, 0xFFF

		.set GRASS_DARK_COLOR, 0x086
		.set GRASS_LIGHT_COLOR, 0x0A0
    
*------------------------------------------------
*  Functions
*------------------------------------------------
  .globl  HInter

HInter:
	move.l	%d0, -(%sp)     /* push device register 0 onto the stack */

	clr.l	%d0               /* Clear D0 and read the current scanline to it */
	move.b	(0xC00008), %d0		

	cmp.w	#100, %d0
	jgt		WORK            /*  do if current line is below horizon */

	move.l	(%sp)+, %d0   /* restore data register 0 */
	rte

WORK:
	move.l	%a0, -(%sp)     /* push address register 0 onto the stack  */

	lea VscrollA, %a0         /* get 'vscrollA' array effective address to A0 */
	move.l	#VSCROLL_A, 0xC00004	/* Vertical scrolling */
	move.b	(%a0, %d0.w), 0xC00000

	lea colors, %a0         /* get 'colors' array effective address to A0 */
 	move.b  (%a0,%d0.w),%d0	/* check shading for current scanline */
	move.l	(%sp)+, %a0     /* restore address register 0 */
	tst.b	%d0               /* check shading value */
	jeq		LIGHT             /* jump to light coloring */

DARK:
LINE_DARK:
	move.b #2, %d0
	cmp.b line_color, %d0
	jne SET_LINE_DARK   /* if not dark, set the color */
	jmp GRASS_DARK
	move.l	(%sp)+, %d0   /* restore data register 0 */
	rte

SET_LINE_DARK:

	move.b %d0, line_color
	move.l #PAL0_COLOR1, VDP_CTRL /* Tell VDP we want to change color 1 */
	clr.w %d0

LINE_DELAY1: 
	move.w	(VDP_CTRL), %d0		/* wait before we set color (to minimize dots) */
	btst.b  #0x02, %d0	
	beq LINE_DELAY1 

	move.w #LINE_DARK_COLOR, VDP_DATA  /* set the color */

	move.l	(%sp)+, %d0   /* restore data register 0 */
  rte

GRASS_DARK:
	move.b #2, %d0
	cmp.b grass_color, %d0
	jne SET_GRASS_DARK   /* if not dark, set the color */
	move.l	(%sp)+, %d0   /* restore data register 0 */
	rte

SET_GRASS_DARK:

	move.b %d0, grass_color
	move.l #PAL0_COLOR2, VDP_CTRL /* Tell VDP we want to change color 1 */
	clr.w %d0

GRASS_DELAY1: 
	move.w	(VDP_CTRL), %d0		/* wait before we set color (to minimize dots) */
	btst.b  #0x02, %d0	
	beq GRASS_DELAY1 

	move.w #GRASS_DARK_COLOR, VDP_DATA  /* set the color */

	move.l	(%sp)+, %d0   /* restore data register 0 */
  rte


LIGHT:

LINE_LIGHT:	
	move.b #1, %d0
	cmp.b line_color, %d0
	jne SET_LINE_LIGHT   /* if not dark, set the color */
	JMP GRASS_LIGHT
	move.l	(%sp)+, %d0   /* restore data register 0 */
	rte

SET_LINE_LIGHT:
	move.b %d0, line_color
	move.l #PAL0_COLOR1, VDP_CTRL /* Tell VDP we want to change color 1 */
	clr.w %d0

LINE_DELAY2: 
	move.w	(VDP_CTRL), %d0		/* wait before setting color */
	btst.b  #0x02, %d0	
	beq LINE_DELAY2 

	move.w #LINE_LIGHT_COLOR, VDP_DATA  /* set the color */

	move.l	(%sp)+, %d0   /* restore data register 0 */
  rte

GRASS_LIGHT:
	move.b #1, %d0
	cmp.b grass_color, %d0
	jne SET_GRASS_LIGHT   /* if not dark, set the color */
	move.l	(%sp)+, %d0   /* restore data register 0 */
	rte

SET_GRASS_LIGHT:

	move.b %d0, grass_color
	move.l #PAL0_COLOR2, VDP_CTRL /* Tell VDP we want to change color 1 */
	clr.w %d0

GRASS_DELAY2: 
	move.w	(VDP_CTRL), %d0		/* wait before we set color (to minimize dots) */
	btst.b  #0x02, %d0	
	beq GRASS_DELAY2 

	move.w #GRASS_LIGHT_COLOR, VDP_DATA  /* set the color */

	move.l	(%sp)+, %d0   /* restore data register 0 */
  rte


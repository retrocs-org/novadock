	idnt	"plugin_src/aminet/aminet.c"
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	section	"CODE",code
	cnop	0,4
l1
	movem.l	l33,-(a7)
	moveq	#64,d6
	moveq	#42,d5
	moveq	#32,d4
	moveq	#92,d3
	move.l	(4+l35,a7),a2
	move.l	a2,d1
	beq	l4
	move.l	a2,a0
	inline
	move.l	a0,d0
.l1
	tst.b	(a0)+
	bne	.l1
	sub.l	a0,d0
	not.l	d0
	einline
	cmp.l	#160,d0
	bcs	l5
l4
	moveq	#0,d0
	bra	l2
l5
	move.l	a2,a0
	lea	l10,a1
	moveq	#8,d2
	inline
	moveq	#0,d0
	moveq	#0,d1
.l1
	subq.l	#1,d2
	bmi	.l3
	move.b	(a0)+,d0
	move.b	(a1)+,d1
	beq	.l2
	sub.l	d1,d0
	beq	.l1
	moveq	#0,d1
.l2
	sub.l	d1,d0
.l3
	einline
	tst.l	d0
	bne	l8
	lea	(8,a2),a1
	bra	l9
l8
	move.l	a2,a0
	lea	l14,a1
	moveq	#7,d2
	inline
	moveq	#0,d0
	moveq	#0,d1
.l1
	subq.l	#1,d2
	bmi	.l3
	move.b	(a0)+,d0
	move.b	(a1)+,d1
	beq	.l2
	sub.l	d1,d0
	beq	.l1
	moveq	#0,d1
.l2
	sub.l	d1,d0
.l3
	einline
	tst.l	d0
	bne	l12
	move.l	a2,d0
	addq.l	#7,d0
	bra	l13
l12
	moveq	#0,d0
l13
	move.l	d0,a1
l9
	move.l	a1,a0
	move.l	a1,d1
	beq	l15
	tst.b	(a0)
	beq	l15
	cmp.b	#47,(a0)
	beq	l15
	cmp.b	#63,(a0)
	beq	l15
	cmp.b	#35,(a0)
	bne	l16
l15
	moveq	#0,d0
	bra	l2
l16
	tst.b	(a0)
	beq	l32
	moveq	#34,d1
	moveq	#0,d0
l21
	cmp.b	(a0),d4
	bcc	l25
	cmp.b	(a0),d1
	beq	l25
	cmp.b	(a0),d5
	beq	l25
	cmp.b	(a0),d6
	beq	l25
	cmp.b	(a0),d3
	bne	l26
l25
	bra	l2
l26
	addq.l	#1,a0
	tst.b	(a0)
	bne	l21
l32
	moveq	#1,d0
l2
l33	reg	a2/d2/d3/d4/d5/d6
	movem.l	(a7)+,a2/d2/d3/d4/d5/d6
l35	equ	24
	rts
	cnop	0,4
l10
	dc.b	104
	dc.b	116
	dc.b	116
	dc.b	112
	dc.b	115
	dc.b	58
	dc.b	47
	dc.b	47
	dc.b	0
	cnop	0,4
l14
	dc.b	104
	dc.b	116
	dc.b	116
	dc.b	112
	dc.b	58
	dc.b	47
	dc.b	47
	dc.b	0
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	cnop	0,4
l36
	movem.l	l68,-(a7)
	moveq	#47,d6
	moveq	#58,d5
	moveq	#45,d4
	moveq	#102,d3
	moveq	#97,d1
	move.l	(4+l70,a7),a1
	move.l	a1,d0
	beq	l39
	move.l	a1,a0
	inline
	move.l	a0,d0
.l1
	tst.b	(a0)+
	bne	.l1
	sub.l	a0,d0
	not.l	d0
	einline
	cmp.l	#150,d0
	bls	l40
l39
	moveq	#0,d0
	bra	l37
l40
	move.l	a1,a2
	move.l	a1,a0
	tst.b	(a1)
	beq	l66
l64
	cmp.b	(a0),d5
	beq	l46
	cmp.b	(a0),d6
	bne	l47
l46
	lea	(1,a0),a2
l47
	addq.l	#1,a0
	tst.b	(a0)
	bne	l64
l66
	move.l	a2,a0
	lea	l51,a1
	moveq	#17,d2
	inline
	moveq	#0,d0
	moveq	#0,d1
.l1
	subq.l	#1,d2
	bmi	.l3
	move.b	(a0)+,d0
	move.b	(a1)+,d1
	beq	.l2
	sub.l	d1,d0
	beq	.l1
	moveq	#0,d1
.l2
	sub.l	d1,d0
.l3
	einline
	moveq	#97,d1
	tst.l	d0
	beq	l50
	moveq	#0,d0
	bra	l37
l50
	lea	(17,a2),a0
	tst.b	(a0)
	bne	l53
	moveq	#0,d0
	bra	l37
l53
	tst.b	(a0)
	beq	l67
	moveq	#57,d5
	moveq	#48,d2
	moveq	#0,d0
l54
	cmp.b	(a0),d2
	bgt	l61
	cmp.b	(a0),d5
	bge	l59
l61
	cmp.b	(a0),d1
	bgt	l60
	cmp.b	(a0),d3
	bge	l59
l60
	cmp.b	(a0),d4
	beq	l59
	bra	l37
l59
	addq.l	#1,a0
	tst.b	(a0)
	bne	l54
l67
	moveq	#58,d5
	moveq	#1,d0
l37
l68	reg	a2/d2/d3/d4/d5/d6
	movem.l	(a7)+,a2/d2/d3/d4/d5/d6
l70	equ	24
	rts
	cnop	0,4
l51
	dc.b	78
	dc.b	111
	dc.b	118
	dc.b	97
	dc.b	68
	dc.b	111
	dc.b	99
	dc.b	107
	dc.b	68
	dc.b	111
	dc.b	119
	dc.b	110
	dc.b	108
	dc.b	111
	dc.b	97
	dc.b	100
	dc.b	45
	dc.b	0
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	cnop	0,4
l71
	sub.w	#108,a7
	movem.l	l84,-(a7)
	move.l	(112+l86,a7),-(a7)
	move.l	#96,-(a7)
	lea	(17+l86,a7),a1
	move.l	a1,-(a7)
	jsr	_fgets
	add.w	#12,a7
	tst.l	d0
	bne	l75
	moveq	#0,d0
	bra	l72
l75
	lea	(9+l86,a7),a0
	inline
	move.l	a0,d0
.l1
	tst.b	(a0)+
	bne	.l1
	sub.l	a0,d0
	not.l	d0
	einline
	move.l	d0,d1
	beq	l76
	lea	(9+l86,a7),a1
	move.w	d1,d0
	subq.w	#1,d0
	cmp.b	#10,(0,a1,d0.w)
	bne	l76
	lea	(8+l86,a7),a1
	move.l	a1,-(a7)
	lea	(8+l86,a7),a1
	move.l	a1,-(a7)
	lea	(8+l86,a7),a1
	move.l	a1,-(a7)
	pea	l80
	lea	(25+l86,a7),a1
	move.l	a1,-(a7)
	jsr	_sscanf
	add.w	#20,a7
	moveq	#2,d2
	cmp.l	d0,d2
	beq	l77
l76
	moveq	#0,d0
	bra	l72
l77
	cmp.l	#67108864,(4+l86,a7)
	bhi	l81
	move.l	(0+l86,a7),d0
	cmp.l	(4+l86,a7),d0
	bls	l82
l81
	moveq	#0,d0
	bra	l72
l82
	move.l	(116+l86,a7),a0
	move.l	(0+l86,a7),(a0)
	move.l	(120+l86,a7),a0
	move.l	(4+l86,a7),(a0)
	moveq	#1,d0
l72
l84	reg	d2
	movem.l	(a7)+,d2
l86	equ	4
	add.w	#108,a7
	rts
	cnop	0,4
l80
	dc.b	37
	dc.b	108
	dc.b	117
	dc.b	32
	dc.b	37
	dc.b	108
	dc.b	117
	dc.b	32
	dc.b	37
	dc.b	99
	dc.b	0
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	cnop	0,4
l88
	movem.l	l93,-(a7)
	move.l	#89146,d1
	move.l	#89150,d0
	tst.l	l87
	beq	l92
	move.l	l87,a0
	move.l	(4+l95,a7),(0,a0,d1.l)
	move.l	(8+l95,a7),(0,a0,d0.l)
	add.l	#89154,a0
	move.l	(12+l95,a7),(a0)
l92
l93	reg
l95	equ	0
	rts
; stacksize=0
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	cnop	0,4
l96
	sub.w	#16,a7
	movem.l	l113,-(a7)
	move.l	#89142,d6
	move.l	#89239,d5
	move.l	#89138,d4
	move.w	#0,a2
	move.w	#1,a5
	move.w	#1,a3
	tst.l	l87
	beq	l97
	move.l	l87,a0
	tst.l	(4,a0)
	beq	l97
	move.l	l87,a0
	add.l	d5,a0
	cmp.l	(20+l115,a7),a0
	beq	l103
	move.l	l87,a0
	add.l	d5,a0
	move.l	(20+l115,a7),a1
	moveq	#127,d1
	inline
	move.l	a0,d0
	bra	.l2
.l1
	move.b	(a1),(a0)+
	beq	.l2
	addq.l	#1,a1
.l2
	subq.l	#1,d1
	bpl	.l1
	einline
l103
	move.l	l87,a0
	move.b	#0,(127,a0,d5.l)
	move.l	l87,a0
	move.l	(4,a0),a1
	move.l	(50,a1),a4
	move.l	(20,a0),a0
	move.l	_GfxBase,a6
	move.l	a4,a1
	jsr	-66(a6)
	move.l	l87,a1
	move.l	(a1),a0
	move.l	_IntuitionBase,a6
	jsr	-690(a6)
	tst.l	d0
	beq	l105
	move.l	d0,a0
	move.l	(4,a0),a0
	move.w	(14,a0),a2
	move.w	(4,a0),a5
	move.w	(10,a0),a3
	move.l	l87,a1
	move.l	(a1),a0
	move.l	_IntuitionBase,a6
	move.l	d0,a1
	jsr	-696(a6)
l105
	move.l	_GfxBase,a6
	move.l	a4,a1
	moveq	#0,d0
	jsr	-354(a6)
	moveq	#0,d0
	move.w	a2,d0
	move.l	_GfxBase,a6
	move.l	a4,a1
	jsr	-342(a6)
	move.l	l87,a1
	lea	(a1,d6.l),a0
	move.l	(a0),d1
	move.l	d1,d3
	subq.l	#7,d3
	add.l	d4,a1
	move.l	(a1),d2
	sub.l	#9,d2
	sub.l	#37,d1
	move.l	_GfxBase,a6
	move.l	a4,a1
	moveq	#8,d0
	jsr	-306(a6)
	move.l	l87,a1
	add.l	#89146,a1
	tst.l	(a1)
	beq	l112
	moveq	#0,d0
	move.w	a5,d0
	move.l	_GfxBase,a6
	move.l	a4,a1
	jsr	-342(a6)
	move.l	l87,a1
	lea	(a1,d6.l),a0
	move.l	(a0),d1
	move.l	d1,d3
	sub.l	#25,d3
	add.l	d4,a1
	move.l	(a1),d2
	sub.l	#11,d2
	sub.l	#35,d1
	move.l	_GfxBase,a6
	move.l	a4,a1
	moveq	#10,d0
	jsr	-306(a6)
	moveq	#0,d0
	move.w	a2,d0
	move.l	_GfxBase,a6
	move.l	a4,a1
	jsr	-342(a6)
	move.l	l87,a1
	lea	(a1,d6.l),a0
	move.l	(a0),d1
	move.l	d1,d3
	sub.l	#26,d3
	add.l	d4,a1
	move.l	(a1),d2
	sub.l	#12,d2
	sub.l	#34,d1
	move.l	_GfxBase,a6
	move.l	a4,a1
	moveq	#11,d0
	jsr	-306(a6)
	move.l	l87,a1
	add.l	#89154,a1
	tst.l	(a1)
	beq	l109
	move.l	l87,a0
	move.l	a0,a1
	add.l	#89138,a1
	move.l	(a1),d4
	sub.l	#24,d4
	move.l	a0,a1
	add.l	#89154,a1
	move.l	(a1),-(a7)
	add.l	#89150,a0
	move.l	(a0),-(a7)
	jsr	_nd_aminet_percent
	move.l	d0,d1
	move.l	d4,d3
	swap	d1
	swap	d3
	mulu.w	d4,d1
	mulu.w	d0,d3
	mulu.w	d4,d0
	add.w	d3,d1
	swap	d1
	clr.w	d1
	add.l	d1,d0
	moveq	#100,d1
	jsr	__divu
	move.l	d0,d2
	addq.w	#8,a7
	bra	l110
l109
	move.l	#89138,d4
	moveq	#0,d2
l110
	move.l	#89138,d4
	move.l	d2,(0+l115,a7)
	tst.l	d2
	beq	l112
	moveq	#0,d0
	move.w	a3,d0
	move.l	_GfxBase,a6
	move.l	a4,a1
	jsr	-342(a6)
	move.l	l87,a1
	move.l	(0,a1,d6.l),d1
	move.l	d1,d3
	sub.l	#27,d3
	moveq	#11,d2
	add.l	(0+l115,a7),d2
	sub.l	#33,d1
	move.l	_GfxBase,a6
	move.l	a4,a1
	moveq	#12,d0
	jsr	-306(a6)
l112
	moveq	#0,d0
	move.w	a5,d0
	move.l	_GfxBase,a6
	move.l	a4,a1
	jsr	-342(a6)
	move.l	l87,a1
	lea	(a1,d4.l),a0
	move.l	(a0),d2
	sub.l	#20,d2
	lea	(4+l115,a7),a2
	lea	(a1,d5.l),a0
	inline
	move.l	a0,d0
.l1
	tst.b	(a0)+
	bne	.l1
	sub.l	a0,d0
	not.l	d0
	einline
	move.l	l87,a0
	add.l	d5,a0
	move.l	_GfxBase,a6
	move.l	a4,a1
	move.l	#0,a3
	moveq	#1,d1
	moveq	#12,d3
	jsr	-696(a6)
	move.l	d0,a2
	move.l	l87,a0
	move.l	(0,a0,d6.l),d1
	sub.l	#11,d1
	move.l	_GfxBase,a6
	move.l	a4,a1
	moveq	#10,d0
	jsr	-240(a6)
	move.l	l87,a0
	add.l	d5,a0
	move.l	_GfxBase,a6
	move.l	a4,a1
	move.l	a2,d0
	jsr	-60(a6)
l97
l113	reg	a2/a3/a4/a5/a6/d2/d3/d4/d5/d6
	movem.l	(a7)+,a2/a3/a4/a5/a6/d2/d3/d4/d5/d6
l115	equ	40
	add.w	#16,a7
	rts
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	cnop	0,4
l116
	movem.l	l121,-(a7)
	tst.l	l87
	beq	l120
	move.l	#0,-(a7)
	move.l	#-1,-(a7)
	move.l	#-2146959354,-(a7)
	move.l	l87,a0
	move.l	(4,a0),a1
	move.l	(12,a0),a0
	move.l	_GadToolsBase,a6
	move.l	#0,a2
	move.l	a3,-(a7)
	lea	4(a7),a3
	jsr	-42(a6)
	movea.l	(a7)+,a3
	add.w	#12,a7
l120
l121	reg	a2/a6
	movem.l	(a7)+,a2/a6
l123	equ	8
	rts
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	cnop	0,4
l124
	movem.l	l127,-(a7)
	jsr	l116
	move.l	l87,a0
	pea	(28,a0)
	jsr	_NewList
	move.l	l87,a0
	move.l	a0,a1
	add.l	#89130,a1
	move.l	#0,(a1)
	add.l	#89134,a0
	moveq	#-1,d0
	move.l	d0,(a0)
	addq.w	#4,a7
l127	reg
l129	equ	0
	rts
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	cnop	0,4
l130
	movem.l	l135,-(a7)
	move.l	#89130,d4
	move.l	l87,a0
	move.l	(0,a0,d4.l),d3
	cmp.l	#512,d3
	bge	l131
	move.l	l87,a1
	add.l	#7210,a1
	move.l	#160,d2
	move.l	d2,d0
	move.l	d3,d1
	swap	d0
	swap	d1
	mulu.w	d3,d0
	mulu.w	d2,d1
	mulu.w	d3,d2
	add.w	d1,d0
	swap	d0
	clr.w	d0
	add.l	d0,d2
	lea	(a1,d2.l),a0
	move.l	(4+l137,a7),a1
	move.l	#159,d1
	inline
	move.l	a0,d0
	bra	.l2
.l1
	move.b	(a1),(a0)+
	beq	.l2
	addq.l	#1,a1
.l2
	subq.l	#1,d1
	bpl	.l1
	einline
	move.l	l87,a0
	add.l	#7210,a0
	add.l	d2,a0
	move.b	#0,(159,a0)
	move.l	l87,a1
	lea	(7210,a1),a0
	add.l	d2,a0
	add.l	#42,a1
	move.w	d3,d0
	muls.w	#14,d0
	move.l	a0,(10,a1,d0.w)
	move.l	l87,a0
	lea	(42,a0),a1
	add.w	d0,a1
	add.l	#28,a0
	move.l	_SysBase,a6
	jsr	-246(a6)
	move.l	l87,a0
	add.l	d4,a0
	addq.l	#1,(a0)
l131
l135	reg	a6/d2/d3/d4
	movem.l	(a7)+,a6/d2/d3/d4
l137	equ	16
	rts
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	cnop	0,4
l138
	movem.l	l141,-(a7)
	move.l	#0,-(a7)
	move.l	#-1,-(a7)
	move.l	#-2146959306,-(a7)
	move.l	#0,-(a7)
	move.l	#-2146959355,-(a7)
	move.l	l87,a0
	lea	(28,a0),a1
	move.l	a1,-(a7)
	move.l	#-2146959354,-(a7)
	move.l	(4,a0),a1
	move.l	(12,a0),a0
	move.l	_GadToolsBase,a6
	move.l	#0,a2
	move.l	a3,-(a7)
	lea	4(a7),a3
	jsr	-42(a6)
	movea.l	(a7)+,a3
	add.w	#28,a7
l141	reg	a2/a6
	movem.l	(a7)+,a2/a6
l143	equ	8
	rts
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	cnop	0,4
l144
	movem.l	l159,-(a7)
	tst.l	l87
	beq	l145
	move.l	l87,a0
	tst.l	(4,a0)
	beq	l150
	move.l	l87,a0
	move.l	(4,a0),a0
	move.l	_IntuitionBase,a6
	jsr	-72(a6)
l150
	move.l	l87,a0
	tst.l	(8,a0)
	beq	l152
	move.l	l87,a0
	move.l	(8,a0),a0
	move.l	_GadToolsBase,a6
	jsr	-36(a6)
l152
	move.l	l87,a0
	tst.l	(24,a0)
	beq	l154
	move.l	l87,a0
	move.l	(24,a0),a0
	move.l	_GadToolsBase,a6
	jsr	-132(a6)
l154
	move.l	l87,a0
	tst.l	(20,a0)
	beq	l156
	move.l	l87,a1
	move.l	(20,a1),a1
	move.l	_GfxBase,a6
	jsr	-78(a6)
l156
	move.l	l87,a0
	tst.l	(a0)
	beq	l158
	move.l	l87,a0
	move.l	(a0),a1
	move.l	_IntuitionBase,a6
	move.l	#0,a0
	jsr	-516(a6)
l158
	move.l	l87,a1
	move.l	_SysBase,a6
	jsr	-690(a6)
	move.l	#0,l87
l145
l159	reg	a6
	movem.l	(a7)+,a6
l161	equ	4
	rts
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	cnop	0,4
l162
	sub.w	#40,a7
	movem.l	l231,-(a7)
	move.l	#89142,d5
	move.l	#89138,d4
	move.l	#l165,(0+l233,a7)
	move.w	#8,(4+l233,a7)
	move.w	#0,(6+l233,a7)
	move.l	#6,a4
	tst.l	l87
	beq	l176
	move.l	l87,a0
	move.l	(4,a0),a0
	move.l	_IntuitionBase,a6
	jsr	-312(a6)
	move.l	l87,a0
	move.l	(4,a0),a0
	move.l	_IntuitionBase,a6
	jsr	-450(a6)
	moveq	#1,d0
	bra	l163
l176
	move.l	_SysBase,a6
	move.l	#89368,d0
	move.l	#65537,d1
	jsr	-684(a6)
	move.l	d0,l87
	bne	l178
	moveq	#0,d0
	bra	l163
l178
	move.l	l87,a0
	move.l	a0,a1
	add.l	#89134,a1
	moveq	#-1,d0
	move.l	d0,(a1)
	pea	(28,a0)
	jsr	_NewList
	lea	(4+l233,a7),a0
	move.l	_GfxBase,a6
	jsr	-72(a6)
	move.l	l87,a0
	move.l	d0,(20,a0)
	move.l	l87,a0
	addq.w	#4,a7
	tst.l	(20,a0)
	beq	l181
	move.l	_IntuitionBase,a6
	lea	l182,a0
	jsr	-510(a6)
	move.l	l87,a0
	move.l	d0,(a0)
	move.l	l87,a0
	tst.l	(a0)
	beq	l181
	move.l	l87,a1
	move.l	(a1),a0
	move.w	(12,a0),d0
	ext.l	d0
	sub.l	#12,d0
	move.l	l87,a0
	add.l	d4,a0
	move.l	d0,(a0)
	cmp.l	#740,(a0)
	ble	l186
	move.l	l87,a0
	move.l	#740,(0,a0,d4.l)
l186
	move.l	l87,a1
	move.l	(a1),a0
	lea	(14,a0),a1
	move.w	(a1),d0
	ext.l	d0
	move.b	(30,a0),d1
	ext.w	d1
	ext.l	d1
	sub.l	d1,d0
	subq.l	#8,d0
	move.l	l87,a0
	add.l	d5,a0
	move.l	d0,(a0)
	cmp.l	#460,(a0)
	ble	l188
	move.l	l87,a0
	move.l	#460,(0,a0,d5.l)
l188
	move.l	l87,a0
	cmp.l	#300,(0,a0,d4.l)
	blt	l181
	move.l	l87,a0
	cmp.l	#208,(0,a0,d5.l)
	blt	l181
	move.l	#0,-(a7)
	move.l	l87,a1
	move.l	(a1),a0
	move.l	_GadToolsBase,a6
	move.l	a1,-(a7)
	lea	4(a7),a1
	jsr	-126(a6)
	movea.l	(a7)+,a1
	move.l	l87,a0
	move.l	d0,(24,a0)
	move.l	l87,a0
	addq.w	#4,a7
	tst.l	(24,a0)
	beq	l181
	move.l	l87,a0
	addq.l	#8,a0
	move.l	_GadToolsBase,a6
	jsr	-114(a6)
	move.l	d0,d3
	beq	l181
	move.l	l87,a1
	move.l	(a1),a0
	lea	(35,a0),a1
	move.l	(40,a0),a0
	move.b	(a1),d1
	ext.w	d1
	ext.l	d1
	moveq	#0,d0
	move.w	(4,a0),d0
	add.l	d1,d0
	moveq	#7,d6
	add.l	d0,d6
	lea	(8+l233,a7),a2
	move.l	a2,a0
	moveq	#0,d0
	moveq	#30,d2
	inline
	move.l	a0,a1
	cmp.l	#16,d2
	blo	.l3
	move.l	a0,d1
	and.b	#1,d1
	beq	.l1
	move.b	d0,(a0)+
	subq.l	#1,d2
.l1
	move.b	d0,d1
	lsl.w	#8,d0
	move.b	d1,d0
	move.w	d0,d1
	swap	d0
	move.w	d1,d0
	moveq	#3,d1
	and.l	d2,d1
	sub.l	d1,d2
.l2
	move.l	d0,(a0)+
	subq.l	#4,d2
	bne	.l2
	move.w	d1,d2
.l3
	subq.w	#1,d2
	blo	.l5
.l4
	move.b	d0,(a0)+
	dbf	d2,.l4
.l5
	move.l	a1,d0
	einline
	lea	(0+l233,a7),a0
	move.l	a0,(20+l233,a7)
	move.l	l87,a0
	move.l	(24,a0),(30+l233,a7)
	move.w	#18,(14+l233,a7)
	move.w	d6,(10+l233,a7)
	move.w	#10,(8+l233,a7)
	add.l	d4,a0
	move.w	(2,a0),d0
	sub.w	#80,d0
	move.w	d0,(12+l233,a7)
	move.w	#1,(24+l233,a7)
	move.l	#0,-(a7)
	lea	l196,a0
	move.l	a0,-(a7)
	move.l	#-2146959315,-(a7)
	move.l	#80,-(a7)
	move.l	#-2146959314,-(a7)
	move.l	_GadToolsBase,a6
	moveq	#12,d0
	move.l	d3,a0
	move.l	a2,a1
	move.l	a2,-(a7)
	lea	4(a7),a2
	jsr	-30(a6)
	movea.l	(a7)+,a2
	move.l	l87,a0
	move.l	d0,(16,a0)
	move.l	d0,d3
	add.w	#20,a7
	tst.l	d0
	beq	l181
	move.l	l87,a0
	move.w	(2,a0,d4.l),d0
	sub.w	#64,d0
	move.w	d0,(8+l233,a7)
	move.w	#54,(12+l233,a7)
	move.w	#2,(24+l233,a7)
	move.l	#l199,(16+l233,a7)
	move.l	#0,-(a7)
	lea	(12+l233,a7),a1
	move.l	_GadToolsBase,a6
	moveq	#1,d0
	move.l	d3,a0
	move.l	a2,-(a7)
	lea	4(a7),a2
	jsr	-30(a6)
	movea.l	(a7)+,a2
	move.l	d0,d3
	addq.w	#4,a7
	beq	l181
	moveq	#0,d2
l202
	move.l	l87,a2
	add.l	d4,a2
	move.l	(a2),d1
	sub.l	#20,d1
	move.l	d1,d0
	move.l	d2,d7
	swap	d0
	swap	d7
	mulu.w	d2,d0
	mulu.w	d1,d7
	mulu.w	d2,d1
	add.w	d7,d0
	swap	d0
	clr.w	d0
	add.l	d0,d1
	move.l	d1,d0
	moveq	#3,d1
	jsr	__divs
	moveq	#10,d1
	add.w	d0,d1
	move.w	d1,(8+l233,a7)
	move.l	(a2),d1
	sub.l	#32,d1
	move.l	d1,d0
	moveq	#3,d1
	jsr	__divs
	move.w	d0,(12+l233,a7)
	move.w	d6,d0
	moveq	#24,d1
	add.w	d0,d1
	move.w	d1,(10+l233,a7)
	move.w	d2,d0
	moveq	#3,d1
	add.w	d0,d1
	move.w	d1,(24+l233,a7)
	move.w	d2,d0
	lsl.w	#2,d0
	lea	l166,a0
	move.l	(0,a0,d0.w),(16+l233,a7)
	move.l	#0,-(a7)
	lea	(12+l233,a7),a1
	move.l	_GadToolsBase,a6
	moveq	#1,d0
	move.l	d3,a0
	move.l	a2,-(a7)
	lea	4(a7),a2
	jsr	-30(a6)
	movea.l	(a7)+,a2
	move.l	d0,d3
	addq.w	#4,a7
	beq	l181
	addq.l	#1,d2
	moveq	#3,d0
	cmp.l	d2,d0
	bgt	l202
	move.w	#10,(8+l233,a7)
	move.w	d6,d1
	moveq	#49,d0
	add.w	d1,d0
	move.w	d0,(10+l233,a7)
	move.l	l87,a0
	lea	(a0,d4.l),a1
	move.w	(2,a1),d0
	sub.w	#20,d0
	move.w	d0,(12+l233,a7)
	lea	(a0,d5.l),a1
	move.w	(2,a1),d0
	sub.w	d1,d0
	sub.w	#139,d0
	move.w	d0,(14+l233,a7)
	move.w	#10,(24+l233,a7)
	move.l	#0,(16+l233,a7)
	move.l	#0,-(a7)
	move.l	#-1,-(a7)
	move.l	#-2146959306,-(a7)
	move.l	#18,-(a7)
	move.l	#-2146959352,-(a7)
	move.l	#0,-(a7)
	move.l	#-2146959307,-(a7)
	add.l	#28,a0
	move.l	a0,-(a7)
	move.l	#-2146959354,-(a7)
	lea	(44+l233,a7),a1
	move.l	_GadToolsBase,a6
	moveq	#4,d0
	move.l	d3,a0
	move.l	a2,-(a7)
	lea	4(a7),a2
	jsr	-30(a6)
	movea.l	(a7)+,a2
	move.l	l87,a0
	move.l	d0,(12,a0)
	move.l	d0,d3
	add.w	#36,a7
	tst.l	d0
	beq	l181
	moveq	#0,d2
l210
	move.w	d2,d0
	muls.w	#82,d0
	moveq	#10,d7
	add.w	d0,d7
	move.w	d7,(8+l233,a7)
	move.l	l87,a0
	move.w	(2,a0,d5.l),d0
	sub.w	#84,d0
	move.w	d0,(10+l233,a7)
	move.l	#4980754,(12+l233,a7)
	move.w	d2,d0
	moveq	#20,d7
	add.w	d0,d7
	move.w	d7,(24+l233,a7)
	tst.l	d2
	beq	l215
	move.l	#l217,d1
	bra	l216
l215
	move.l	#l218,d1
l216
	move.l	d1,(16+l233,a7)
	move.l	#0,-(a7)
	lea	(12+l233,a7),a1
	move.l	_GadToolsBase,a6
	moveq	#1,d0
	move.l	d3,a0
	move.l	a2,-(a7)
	lea	4(a7),a2
	jsr	-30(a6)
	movea.l	(a7)+,a2
	move.l	d0,d3
	addq.w	#4,a7
	beq	l181
	addq.l	#1,d2
	moveq	#2,d0
	cmp.l	d2,d0
	bgt	l210
	moveq	#0,d2
l221
	move.l	l87,a3
	lea	(a3,d4.l),a2
	move.l	(a2),d1
	sub.l	#20,d1
	move.l	d1,d0
	move.l	d2,d7
	swap	d0
	swap	d7
	mulu.w	d2,d0
	mulu.w	d1,d7
	mulu.w	d2,d1
	add.w	d7,d0
	swap	d0
	clr.w	d0
	add.l	d0,d1
	move.l	d1,d0
	moveq	#4,d1
	jsr	__divs
	moveq	#10,d1
	add.w	d0,d1
	move.w	d1,(8+l233,a7)
	lea	(a3,d5.l),a0
	move.w	(2,a0),d0
	sub.w	#60,d0
	move.w	d0,(10+l233,a7)
	move.l	(a2),d1
	sub.l	#20,d1
	move.l	d1,d0
	moveq	#4,d1
	jsr	__divs
	move.w	a4,d1
	sub.w	d1,d0
	move.w	d0,(12+l233,a7)
	move.w	d2,d0
	moveq	#30,d1
	add.w	d0,d1
	move.w	d1,(24+l233,a7)
	move.w	d2,d0
	lsl.w	#2,d0
	lea	l170,a0
	move.l	(0,a0,d0.w),(16+l233,a7)
	move.l	#0,-(a7)
	lea	(12+l233,a7),a1
	move.l	_GadToolsBase,a6
	moveq	#1,d0
	move.l	d3,a0
	move.l	a2,-(a7)
	lea	4(a7),a2
	jsr	-30(a6)
	movea.l	(a7)+,a2
	move.l	d0,d3
	addq.w	#4,a7
	beq	l181
	addq.l	#1,d2
	moveq	#4,d0
	cmp.l	d2,d0
	bgt	l221
	move.l	#0,-(a7)
	move.l	#4195964,-(a7)
	move.l	#-2147483542,-(a7)
	move.l	#4174,-(a7)
	move.l	#-2147483541,-(a7)
	move.l	l87,a0
	move.l	(8,a0),a1
	move.l	a1,-(a7)
	move.l	#-2147483540,-(a7)
	move.l	(0,a0,d5.l),-(a7)
	move.l	#-2147483545,-(a7)
	add.l	d4,a0
	move.l	(a0),d0
	move.l	d0,-(a7)
	move.l	#-2147483546,-(a7)
	move.l	l87,a1
	move.l	(a1),a0
	move.b	(30,a0),d1
	ext.w	d1
	ext.l	d1
	addq.l	#2,d1
	move.l	d1,-(a7)
	move.l	#-2147483547,-(a7)
	move.w	(12,a0),d1
	ext.l	d1
	sub.l	d0,d1
	move.l	d1,d0
	moveq	#2,d1
	jsr	__divs
	move.l	d0,-(a7)
	move.l	#-2147483548,-(a7)
	move.l	l87,a1
	move.l	(a1),a0
	move.l	a0,-(a7)
	move.l	#-2147483527,-(a7)
	lea	l227,a0
	move.l	a0,-(a7)
	move.l	#-2147483538,-(a7)
	move.l	_IntuitionBase,a6
	move.l	#0,a0
	move.l	a1,-(a7)
	lea	4(a7),a1
	jsr	-606(a6)
	movea.l	(a7)+,a1
	move.l	l87,a0
	move.l	d0,(4,a0)
	move.l	l87,a0
	add.w	#76,a7
	tst.l	(4,a0)
	beq	l181
	move.l	l87,a0
	move.l	(4,a0),a0
	move.l	_GadToolsBase,a6
	move.l	#0,a1
	jsr	-84(a6)
	pea	l230
	jsr	l96
	moveq	#1,d0
	addq.w	#4,a7
	bra	l163
l181
	jsr	l144
	moveq	#0,d0
l163
l231	reg	a2/a3/a4/a6/d2/d3/d4/d5/d6/d7
	movem.l	(a7)+,a2/a3/a4/a6/d2/d3/d4/d5/d6/d7
l233	equ	40
	add.w	#40,a7
	rts
	cnop	0,4
l217
	dc.b	78
	dc.b	101
	dc.b	120
	dc.b	116
	dc.b	0
	cnop	0,4
l218
	dc.b	80
	dc.b	114
	dc.b	101
	dc.b	118
	dc.b	105
	dc.b	111
	dc.b	117
	dc.b	115
	dc.b	0
	cnop	0,4
l165
	dc.b	116
	dc.b	111
	dc.b	112
	dc.b	97
	dc.b	122
	dc.b	46
	dc.b	102
	dc.b	111
	dc.b	110
	dc.b	116
	dc.b	0
	cnop	0,4
l167
	dc.b	82
	dc.b	101
	dc.b	99
	dc.b	101
	dc.b	110
	dc.b	116
	dc.b	0
	cnop	0,4
l168
	dc.b	67
	dc.b	97
	dc.b	116
	dc.b	101
	dc.b	103
	dc.b	111
	dc.b	114
	dc.b	105
	dc.b	101
	dc.b	115
	dc.b	0
	cnop	0,4
l169
	dc.b	66
	dc.b	97
	dc.b	99
	dc.b	107
	dc.b	0
	cnop	0,4
l171
	dc.b	79
	dc.b	112
	dc.b	101
	dc.b	110
	dc.b	0
	cnop	0,4
l172
	dc.b	82
	dc.b	101
	dc.b	97
	dc.b	100
	dc.b	109
	dc.b	101
	dc.b	0
	cnop	0,4
l173
	dc.b	71
	dc.b	101
	dc.b	116
	dc.b	0
	cnop	0,4
l174
	dc.b	67
	dc.b	108
	dc.b	111
	dc.b	115
	dc.b	101
	dc.b	0
	cnop	0,4
l182
	dc.b	87
	dc.b	111
	dc.b	114
	dc.b	107
	dc.b	98
	dc.b	101
	dc.b	110
	dc.b	99
	dc.b	104
	dc.b	0
	cnop	0,4
l196
	dc.b	0
	cnop	0,4
l199
	dc.b	83
	dc.b	101
	dc.b	97
	dc.b	114
	dc.b	99
	dc.b	104
	dc.b	0
	cnop	0,4
l227
	dc.b	78
	dc.b	111
	dc.b	118
	dc.b	97
	dc.b	68
	dc.b	111
	dc.b	99
	dc.b	107
	dc.b	32
	dc.b	65
	dc.b	77
	dc.b	73
	dc.b	78
	dc.b	69
	dc.b	84
	dc.b	0
	cnop	0,4
l230
	dc.b	67
	dc.b	104
	dc.b	111
	dc.b	111
	dc.b	115
	dc.b	101
	dc.b	32
	dc.b	82
	dc.b	101
	dc.b	99
	dc.b	101
	dc.b	110
	dc.b	116
	dc.b	44
	dc.b	32
	dc.b	67
	dc.b	97
	dc.b	116
	dc.b	101
	dc.b	103
	dc.b	111
	dc.b	114
	dc.b	105
	dc.b	101
	dc.b	115
	dc.b	32
	dc.b	111
	dc.b	114
	dc.b	32
	dc.b	101
	dc.b	110
	dc.b	116
	dc.b	101
	dc.b	114
	dc.b	32
	dc.b	97
	dc.b	32
	dc.b	115
	dc.b	101
	dc.b	97
	dc.b	114
	dc.b	99
	dc.b	104
	dc.b	46
	dc.b	0
	section	"DATA",data
	cnop	0,4
l166
	dc.l	l167
	dc.l	l168
	dc.l	l169
	cnop	0,4
l170
	dc.l	l171
	dc.l	l172
	dc.l	l173
	dc.l	l174
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	section	"CODE",code
	cnop	0,4
l234
	movem.l	l259,-(a7)
	moveq	#69,d6
	moveq	#4,d5
	move.l	#89239,d4
	move.l	#89134,d3
	tst.l	l87
	bne	l238
	moveq	#0,d0
	bra	l235
l238
	move.l	l87,a0
	move.l	(4,a0),a0
	move.l	(86,a0),a0
	move.l	_GadToolsBase,a6
	jsr	-72(a6)
	move.l	d0,a2
	move.l	a2,d1
	beq	l258
	move.l	#512,a5
l239
	move.l	(20,a2),a3
	move.w	(24,a2),a4
	moveq	#64,d1
	cmp.l	a3,d1
	bne	l243
	move.l	(28,a2),a0
	moveq	#0,d0
	move.w	(38,a0),d0
	bra	l244
l243
	moveq	#0,d0
l244
	move.w	d0,d2
	move.l	_GadToolsBase,a6
	move.l	a2,a1
	jsr	-78(a6)
	cmp.l	a3,a5
	beq	l245
	cmp.l	#1024,a3
	bne	l246
	cmp.w	a4,d6
	bne	l246
l245
	moveq	#33,d0
	bra	l235
l246
	cmp.l	a3,d5
	bne	l250
	move.l	l87,a0
	move.l	(4,a0),a0
	move.l	_GadToolsBase,a6
	jsr	-90(a6)
	move.l	l87,a0
	pea	(0,a0,d4.l)
	jsr	l96
	move.l	l87,a0
	move.l	(4,a0),a0
	move.l	_GadToolsBase,a6
	moveq	#1,d0
	jsr	-96(a6)
	move.l	l87,a0
	move.l	(4,a0),a0
	move.l	_GadToolsBase,a6
	move.l	#0,a1
	jsr	-84(a6)
	addq.w	#4,a7
l250
	cmp.w	#10,d2
	bne	l252
	moveq	#0,d1
	move.w	a4,d1
	move.l	d1,a0
	move.l	l87,a1
	move.l	a0,(0,a1,d3.l)
	moveq	#10,d0
	bra	l235
l252
	tst.w	d2
	beq	l254
	cmp.w	#1,d2
	bne	l256
	moveq	#2,d2
l256
	moveq	#0,d0
	move.w	d2,d0
	bra	l235
l254
	move.l	l87,a0
	move.l	(4,a0),a0
	move.l	(86,a0),a0
	move.l	_GadToolsBase,a6
	jsr	-72(a6)
	move.l	d0,a2
	move.l	a2,d1
	bne	l239
l258
	moveq	#0,d0
l235
l259	reg	a2/a3/a4/a5/a6/d2/d3/d4/d5/d6
	movem.l	(a7)+,a2/a3/a4/a5/a6/d2/d3/d4/d5/d6
l261	equ	40
	rts
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	cnop	0,4
l262
	movem.l	l275,-(a7)
	moveq	#0,d2
	move.l	#0,-(a7)
	move.l	_AslBase,a6
	moveq	#0,d0
	move.l	a0,-(a7)
	lea	4(a7),a0
	jsr	-48(a6)
	movea.l	(a7)+,a0
	move.l	d0,a2
	addq.w	#4,a7
	move.l	a2,d0
	bne	l266
	moveq	#0,d0
	bra	l263
l266
	move.l	#0,-(a7)
	move.l	#1,-(a7)
	move.l	#-2146959300,-(a7)
	move.l	#1,-(a7)
	move.l	#-2146959316,-(a7)
	move.l	(28+l277,a7),a0
	move.l	a0,-(a7)
	move.l	#-2146959352,-(a7)
	move.l	(32+l277,a7),a0
	move.l	a0,-(a7)
	move.l	#-2146959351,-(a7)
	lea	l269,a0
	move.l	a0,-(a7)
	move.l	#-2146959359,-(a7)
	move.l	_AslBase,a6
	move.l	a2,a0
	move.l	a1,-(a7)
	lea	4(a7),a1
	jsr	-60(a6)
	movea.l	(a7)+,a1
	add.w	#44,a7
	tst.w	d0
	beq	l271
	move.l	(8,a2),a0
	inline
	move.l	a0,d0
.l1
	tst.b	(a0)+
	bne	.l1
	sub.l	a0,d0
	not.l	d0
	einline
	cmp.l	(16+l277,a7),d0
	bcc	l271
	move.l	(8,a2),a1
	move.l	(12+l277,a7),a0
	inline
	move.l	a0,d0
.l1
	move.b	(a1)+,(a0)+
	bne	.l1
	einline
	move.l	(4,a2),d2
	move.l	_DOSBase,a6
	move.l	(12+l277,a7),d1
	move.l	(16+l277,a7),d3
	jsr	-882(a6)
	tst.w	d0
	bne	l272
	moveq	#0,d0
	bra	l273
l272
	moveq	#1,d0
l273
	move.l	d0,d2
l271
	move.l	_AslBase,a6
	move.l	a2,a0
	jsr	-54(a6)
	move.l	d2,d0
l263
l275	reg	a2/a6/d2/d3
	movem.l	(a7)+,a2/a6/d2/d3
l277	equ	16
	rts
	cnop	0,4
l269
	dc.b	83
	dc.b	97
	dc.b	118
	dc.b	101
	dc.b	32
	dc.b	65
	dc.b	109
	dc.b	105
	dc.b	110
	dc.b	101
	dc.b	116
	dc.b	32
	dc.b	112
	dc.b	97
	dc.b	99
	dc.b	107
	dc.b	97
	dc.b	103
	dc.b	101
	dc.b	0
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	cnop	0,4
l278
	sub.w	#12,a7
	movem.l	l281,-(a7)
	lea	(0+l283,a7),a0
	move.l	_DOSBase,a6
	move.l	a0,d1
	jsr	-192(a6)
	move.l	#1440,d0
	move.l	(0+l283,a7),d1
	move.l	d0,d2
	move.l	d1,d3
	swap	d2
	swap	d3
	mulu.w	d1,d2
	mulu.w	d0,d3
	mulu.w	d1,d0
	add.w	d3,d2
	swap	d2
	clr.w	d2
	add.l	d2,d0
	add.l	(4+l283,a7),d0
	move.l	#3000,d1
	move.l	d0,d2
	move.l	d1,d3
	swap	d2
	swap	d3
	mulu.w	d1,d2
	mulu.w	d0,d3
	mulu.w	d1,d0
	add.w	d3,d2
	swap	d2
	clr.w	d2
	add.l	d2,d0
	add.l	(8+l283,a7),d0
l281	reg	a6/d2/d3
	movem.l	(a7)+,a6/d2/d3
l283	equ	12
	add.w	#12,a7
	rts
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	cnop	0,4
l284
	movem.l	l289,-(a7)
	moveq	#0,d2
	move.l	_SysBase,a6
	jsr	-132(a6)
	move.l	_SysBase,a6
	move.l	(4+l291,a7),a1
	jsr	-390(a6)
	tst.l	d0
	beq	l288
	move.l	_SysBase,a6
	move.l	d0,a0
	move.l	(8+l291,a7),a1
	jsr	-366(a6)
	moveq	#1,d2
l288
	move.l	_SysBase,a6
	jsr	-138(a6)
	move.l	d2,d0
l289	reg	a6/d2
	movem.l	(a7)+,a6/d2
l291	equ	8
	rts
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	cnop	0,4
l292
	sub.w	#200,a7
	movem.l	l305,-(a7)
	move.l	(204+l307,a7),d3
	beq	l293
	move.l	d3,a0
	lea	l300,a1
	moveq	#17,d2
	inline
	moveq	#0,d0
	moveq	#0,d1
.l1
	subq.l	#1,d2
	bmi	.l3
	move.b	(a0)+,d0
	move.b	(a1)+,d1
	beq	.l2
	sub.l	d1,d0
	beq	.l1
	moveq	#0,d1
.l2
	sub.l	d1,d0
.l3
	einline
	tst.l	d0
	beq	l297
	move.l	d3,-(a7)
	jsr	l36
	addq.w	#4,a7
	tst.l	d0
	beq	l293
l297
	move.l	d3,a0
	inline
	move.l	a0,d0
.l1
	tst.b	(a0)+
	bne	.l1
	sub.l	a0,d0
	not.l	d0
	einline
	cmp.l	#150,d0
	bhi	l293
	move.l	d3,-(a7)
	pea	l301
	lea	(8+l307,a7),a2
	move.l	a2,-(a7)
	jsr	_sprintf
	move.l	_DOSBase,a6
	move.l	a2,d1
	jsr	-72(a6)
	move.l	d3,-(a7)
	pea	l302
	move.l	a2,-(a7)
	jsr	_sprintf
	move.l	_DOSBase,a6
	move.l	a2,d1
	jsr	-72(a6)
	pea	l303
	move.l	d3,-(a7)
	pea	l304
	move.l	a2,-(a7)
	jsr	_sprintf
	move.l	_DOSBase,a6
	move.l	a2,d1
	jsr	-72(a6)
	move.l	_DOSBase,a6
	move.l	d3,d1
	jsr	-72(a6)
	add.w	#40,a7
l293
l305	reg	a2/a6/d2/d3
	movem.l	(a7)+,a2/a6/d2/d3
l307	equ	16
	add.w	#200,a7
	rts
	cnop	0,4
l300
	dc.b	84
	dc.b	58
	dc.b	78
	dc.b	111
	dc.b	118
	dc.b	97
	dc.b	68
	dc.b	111
	dc.b	99
	dc.b	107
	dc.b	65
	dc.b	109
	dc.b	105
	dc.b	110
	dc.b	101
	dc.b	116
	dc.b	45
	dc.b	0
	cnop	0,4
l301
	dc.b	37
	dc.b	115
	dc.b	47
	dc.b	114
	dc.b	101
	dc.b	115
	dc.b	112
	dc.b	111
	dc.b	110
	dc.b	115
	dc.b	101
	dc.b	0
	cnop	0,4
l302
	dc.b	37
	dc.b	115
	dc.b	47
	dc.b	102
	dc.b	101
	dc.b	116
	dc.b	99
	dc.b	104
	dc.b	46
	dc.b	108
	dc.b	111
	dc.b	103
	dc.b	0
	cnop	0,4
l303
	dc.b	112
	dc.b	114
	dc.b	111
	dc.b	103
	dc.b	114
	dc.b	101
	dc.b	115
	dc.b	115
	dc.b	0
	cnop	0,4
l304
	dc.b	37
	dc.b	115
	dc.b	47
	dc.b	37
	dc.b	115
	dc.b	0
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	cnop	0,4
l308
	movem.l	l322,-(a7)
	moveq	#-1,d4
	moveq	#1,d3
	move.l	(4+l324,a7),d2
	beq	l309
	move.l	_SysBase,a6
	jsr	-132(a6)
	move.l	_SysBase,a6
	move.l	d2,a1
	jsr	-360(a6)
	move.l	_SysBase,a6
	jsr	-138(a6)
	move.l	_SysBase,a6
	move.l	d2,a0
	jsr	-372(a6)
	move.l	d0,a2
	tst.l	d0
	beq	l321
l320
	cmp.l	(24,a2),d3
	bne	l317
	tst.l	(14,a2)
	beq	l317
	move.l	d4,(24,a2)
	move.l	_SysBase,a6
	move.l	a2,a1
	jsr	-378(a6)
	bra	l319
l317
	pea	(1832,a2)
	jsr	l292
	move.l	_SysBase,a6
	move.l	a2,a1
	jsr	-690(a6)
	addq.w	#4,a7
l319
	move.l	_SysBase,a6
	move.l	d2,a0
	jsr	-372(a6)
	move.l	d0,a2
	tst.l	d0
	bne	l320
l321
	move.l	_SysBase,a6
	move.l	d2,a0
	jsr	-672(a6)
l309
l322	reg	a2/a6/d2/d3/d4
	movem.l	(a7)+,a2/a6/d2/d3/d4
l324	equ	20
	rts
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	cnop	0,4
l325
	sub.w	#204,a7
	movem.l	l385,-(a7)
	move.l	_SysBase,a6
	jsr	-666(a6)
	move.l	d0,d3
	move.l	_SysBase,a6
	jsr	-666(a6)
	move.l	d0,a4
	move.l	_SysBase,a6
	move.l	#1992,d0
	move.l	#65537,d1
	jsr	-684(a6)
	move.l	d0,a3
	move.l	#0,a2
	moveq	#0,d2
	tst.l	d3
	beq	l328
	move.l	a4,d0
	beq	l328
	move.l	a3,d0
	bne	l329
l328
	tst.l	d3
	beq	l333
	move.l	_SysBase,a6
	move.l	d3,a0
	jsr	-672(a6)
l333
	move.l	a4,d0
	beq	l335
	move.l	_SysBase,a6
	move.l	a4,a0
	jsr	-672(a6)
l335
	move.l	a3,d0
	beq	l337
	move.l	_SysBase,a6
	move.l	a3,a1
	jsr	-690(a6)
l337
	moveq	#20,d0
	bra	l326
l329
	move.l	_SysBase,a6
	move.l	#0,a1
	jsr	-294(a6)
	move.l	d0,a1
	move.l	#-1,(184,a1)
	move.l	_SysBase,a6
	jsr	-132(a6)
	move.l	_SysBase,a6
	lea	l340,a1
	jsr	-390(a6)
	tst.l	d0
	bne	l339
	move.l	d3,a1
	move.l	#l341,(10,a1)
	move.l	_SysBase,a6
	move.l	d3,a1
	jsr	-354(a6)
	move.l	#1,a2
l339
	move.l	_SysBase,a6
	jsr	-138(a6)
	move.l	a2,d0
	bne	l343
	move.l	_SysBase,a6
	move.l	d3,a0
	jsr	-672(a6)
	move.l	_SysBase,a6
	move.l	a4,a0
	jsr	-672(a6)
	move.l	_SysBase,a6
	move.l	a3,a1
	jsr	-690(a6)
	moveq	#10,d0
	bra	l326
l343
	move.l	#1313101638,(20,a3)
	moveq	#1,d0
	move.l	d0,(24,a3)
	move.w	#1992,(18,a3)
	move.l	a4,(14,a3)
	move.l	a3,-(a7)
	move.l	(212+l387,a7),-(a7)
	jsr	l284
	addq.w	#8,a7
	tst.l	d0
	beq	l376
	moveq	#0,d4
	move.l	d3,(0+l387,a7)
	tst.l	d2
	bne	l384
	move.l	#150,d3
l346
	move.l	_SysBase,a6
	move.l	a4,a0
	jsr	-372(a6)
	tst.l	d0
	beq	l352
	moveq	#1,d2
	bra	l353
l352
	move.l	_DOSBase,a6
	moveq	#2,d1
	jsr	-198(a6)
l353
	addq.l	#2,d4
	cmp.l	d4,d3
	ble	l384
	tst.l	d2
	beq	l346
l384
	move.l	(0+l387,a7),d3
	move.l	_SysBase,a6
	jsr	-132(a6)
	tst.l	d2
	bne	l356
	move.l	_SysBase,a6
	move.l	a4,a0
	jsr	-372(a6)
	tst.l	d0
	beq	l356
	moveq	#1,d2
l356
	tst.l	d2
	bne	l359
	move.b	#2,(14,a4)
	move.l	#0,(16,a4)
	moveq	#0,d0
	move.b	(15,a4),d0
	move.l	_SysBase,a6
	jsr	-336(a6)
l359
	move.l	_SysBase,a6
	jsr	-138(a6)
	tst.l	d2
	bne	l361
	move.l	_SysBase,a6
	jsr	-132(a6)
	move.l	_SysBase,a6
	move.l	d3,a1
	jsr	-360(a6)
	move.l	_SysBase,a6
	jsr	-138(a6)
	move.l	_SysBase,a6
	move.l	d3,a0
	jsr	-672(a6)
	moveq	#20,d0
	bra	l326
l361
	move.l	_SysBase,a6
	move.l	a4,a0
	jsr	-672(a6)
	move.l	#0,a4
	moveq	#1,d0
	cmp.l	(24,a3),d0
	bne	l376
	tst.b	(32,a3)
	beq	l376
	pea	(1832,a3)
	pea	l365
	lea	(12+l387,a7),a2
	move.l	a2,-(a7)
	jsr	_sprintf
	move.l	_DOSBase,a6
	move.l	#l366,d1
	move.l	#1005,d2
	jsr	-30(a6)
	move.l	d0,d6
	move.l	_DOSBase,a6
	move.l	a2,d1
	move.l	#1006,d2
	jsr	-30(a6)
	move.l	d0,d5
	moveq	#-1,d0
	move.l	d0,(28,a3)
	add.w	#12,a7
	tst.l	d6
	beq	l368
	tst.l	d5
	beq	l368
	move.l	#0,-(a7)
	move.l	#32768,-(a7)
	move.l	#-2147482637,-(a7)
	move.l	#-1,-(a7)
	move.l	#-2147482635,-(a7)
	move.l	#0,-(a7)
	move.l	#-2147482639,-(a7)
	move.l	d5,-(a7)
	move.l	#-2147482640,-(a7)
	move.l	d5,-(a7)
	move.l	#-2147483614,-(a7)
	move.l	d6,-(a7)
	move.l	#-2147483615,-(a7)
	lea	(32,a3),a0
	move.l	_DOSBase,a6
	move.l	a0,d1
	move.l	d2,-(a7)
	move.l	a7,d2
	add.l	#4,d2
	jsr	-606(a6)
	move.l	(a7)+,d2
	move.l	d0,(28,a3)
	add.w	#52,a7
l368
	tst.l	d6
	beq	l371
	move.l	_DOSBase,a6
	move.l	d6,d1
	jsr	-36(a6)
l371
	tst.l	d5
	beq	l373
	move.l	_DOSBase,a6
	move.l	d5,d1
	jsr	-36(a6)
l373
	move.l	_SysBase,a6
	jsr	-132(a6)
	move.l	_SysBase,a6
	move.l	d3,a1
	jsr	-360(a6)
	move.l	_SysBase,a6
	jsr	-138(a6)
	move.l	_SysBase,a6
	move.l	d3,a0
	jsr	-672(a6)
	moveq	#0,d3
	moveq	#2,d0
	move.l	d0,(24,a3)
	move.l	#0,(14,a3)
	move.l	a3,-(a7)
	move.l	(212+l387,a7),-(a7)
	jsr	l284
	addq.w	#8,a7
	tst.l	d0
	beq	l375
	move.l	#0,a3
	bra	l376
l375
	pea	(1832,a3)
	jsr	l292
	addq.w	#4,a7
l376
	move.l	a4,d0
	beq	l378
	move.l	_SysBase,a6
	move.l	a4,a0
	jsr	-672(a6)
l378
	move.l	a3,d0
	beq	l380
	move.l	_SysBase,a6
	move.l	a3,a1
	jsr	-690(a6)
l380
	tst.l	d3
	beq	l382
	move.l	_SysBase,a6
	jsr	-132(a6)
	move.l	_SysBase,a6
	move.l	d3,a1
	jsr	-360(a6)
	move.l	_SysBase,a6
	jsr	-138(a6)
	move.l	_SysBase,a6
	move.l	d3,a0
	jsr	-672(a6)
l382
	moveq	#0,d0
l326
l385	reg	a2/a3/a4/a6/d2/d3/d4/d5/d6
	movem.l	(a7)+,a2/a3/a4/a6/d2/d3/d4/d5/d6
l387	equ	36
	add.w	#204,a7
	rts
	cnop	0,4
l341
	dc.b	78
	dc.b	111
	dc.b	118
	dc.b	97
	dc.b	68
	dc.b	111
	dc.b	99
	dc.b	107
	dc.b	46
	dc.b	65
	dc.b	109
	dc.b	105
	dc.b	110
	dc.b	101
	dc.b	116
	dc.b	46
	dc.b	70
	dc.b	101
	dc.b	116
	dc.b	99
	dc.b	104
	dc.b	0
	cnop	0,4
l365
	dc.b	37
	dc.b	115
	dc.b	47
	dc.b	102
	dc.b	101
	dc.b	116
	dc.b	99
	dc.b	104
	dc.b	46
	dc.b	108
	dc.b	111
	dc.b	103
	dc.b	0
	cnop	0,4
l366
	dc.b	78
	dc.b	73
	dc.b	76
	dc.b	58
	dc.b	0
	cnop	0,4
l340
	dc.b	78
	dc.b	111
	dc.b	118
	dc.b	97
	dc.b	68
	dc.b	111
	dc.b	99
	dc.b	107
	dc.b	46
	dc.b	65
	dc.b	109
	dc.b	105
	dc.b	110
	dc.b	101
	dc.b	116
	dc.b	46
	dc.b	70
	dc.b	101
	dc.b	116
	dc.b	99
	dc.b	104
	dc.b	0
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	cnop	0,4
l400
	movem.l	l405,-(a7)
	move.l	_DOSBase,a6
	move.l	(4+l407,a7),d1
	moveq	#-2,d2
	jsr	-84(a6)
	tst.l	d0
	bne	l404
	moveq	#0,d0
	bra	l401
l404
	move.l	_DOSBase,a6
	move.l	d0,d1
	jsr	-90(a6)
	moveq	#1,d0
l401
l405	reg	a6/d2
	movem.l	(a7)+,a6/d2
l407	equ	8
	rts
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	cnop	0,4
l408
	movem.l	l416,-(a7)
	tst.l	l388
	beq	l412
	move.l	l388,-(a7)
	jsr	l308
	addq.w	#4,a7
l412
	tst.l	2296+l388
	bne	l414
	tst.b	84+l388
	beq	l414
	pea	84+l388
	jsr	l292
	addq.w	#4,a7
l414
	lea	l388,a0
	moveq	#0,d0
	move.l	#2304,d2
	inline
	move.l	a0,a1
	cmp.l	#16,d2
	blo	.l3
	move.l	a0,d1
	and.b	#1,d1
	beq	.l1
	move.b	d0,(a0)+
	subq.l	#1,d2
.l1
	move.b	d0,d1
	lsl.w	#8,d0
	move.b	d1,d0
	move.w	d0,d1
	swap	d0
	move.w	d1,d0
	moveq	#3,d1
	and.l	d2,d1
	sub.l	d1,d2
.l2
	move.l	d0,(a0)+
	subq.l	#4,d2
	bne	.l2
	move.w	d1,d2
.l3
	subq.w	#1,d2
	blo	.l5
.l4
	move.b	d0,(a0)+
	dbf	d2,.l4
.l5
	move.l	a1,d0
	einline
	move.l	#0,-(a7)
	move.l	#0,-(a7)
	move.l	#0,-(a7)
	jsr	l88
	pea	l391
	jsr	l96
	add.w	#16,a7
l416	reg	d2
	movem.l	(a7)+,d2
l418	equ	4
	rts
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	cnop	0,4
l419
	movem.l	l422,-(a7)
	lea	l391,a0
	move.l	(4+l424,a7),a1
	moveq	#127,d1
	inline
	move.l	a0,d0
	bra	.l2
.l1
	move.b	(a1),(a0)+
	beq	.l2
	addq.l	#1,a1
.l2
	subq.l	#1,d1
	bpl	.l1
	einline
	move.b	#0,127+l391
	pea	l391
	jsr	l96
	addq.w	#4,a7
l422	reg
l424	equ	0
	rts
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	cnop	0,4
l425
	sub.w	#2880,a7
	movem.l	l496,-(a7)
	move.l	(2892+l498,a7),d5
	move.l	(2888+l498,a7),a4
	tst.l	l388
	beq	l429
	pea	l430
	jsr	l419
	moveq	#0,d0
	addq.w	#4,a7
	bra	l426
l429
	move.l	_SysBase,a6
	jsr	-132(a6)
	move.l	_SysBase,a6
	lea	l434,a1
	jsr	-390(a6)
	tst.l	d0
	bne	l431
	moveq	#0,d2
	bra	l432
l431
	moveq	#1,d2
l432
	move.l	_SysBase,a6
	jsr	-138(a6)
	tst.l	d2
	beq	l436
	pea	l437
	jsr	l419
	moveq	#0,d0
	addq.w	#4,a7
	bra	l426
l436
	lea	l388,a0
	moveq	#0,d0
	move.l	#2304,d2
	inline
	move.l	a0,a1
	cmp.l	#16,d2
	blo	.l3
	move.l	a0,d1
	and.b	#1,d1
	beq	.l1
	move.b	d0,(a0)+
	subq.l	#1,d2
.l1
	move.b	d0,d1
	lsl.w	#8,d0
	move.b	d1,d0
	move.w	d0,d1
	swap	d0
	move.w	d1,d0
	moveq	#3,d1
	and.l	d2,d1
	sub.l	d1,d2
.l2
	move.l	d0,(a0)+
	subq.l	#4,d2
	bne	.l2
	move.w	d1,d2
.l3
	subq.w	#1,d2
	blo	.l5
.l4
	move.b	d0,(a0)+
	dbf	d2,.l4
.l5
	move.l	a1,d0
	einline
	jsr	l278
	move.l	d0,2284+l388
	move.l	a4,2300+l388
	tst.l	d5
	beq	l439
	lea	2044+l388,a0
	move.l	d5,a1
	move.l	#239,d1
	inline
	move.l	a0,d0
	bra	.l2
.l1
	move.b	(a1),(a0)+
	beq	.l2
	addq.l	#1,a1
.l2
	subq.l	#1,d1
	bpl	.l1
	einline
	move.b	#0,2283+l388
	move.l	d5,-(a7)
	jsr	l400
	addq.w	#4,a7
	tst.l	d0
	beq	l441
	pea	l442
	jsr	l419
	moveq	#0,d0
	addq.w	#4,a7
	bra	l426
l441
	lea	(0+l498,a7),a2
	move.l	a2,a0
	move.l	d5,a1
	inline
	move.l	a0,d0
.l1
	move.b	(a1)+,(a0)+
	bne	.l1
	einline
	move.l	_DOSBase,a6
	move.l	a2,d1
	jsr	-870(a6)
	move.l	d0,a0
	move.b	#0,(a0)
	bra	l443
l439
	lea	l444,a0
	lea	(0+l498,a7),a2
	move.w	(a0)+,(a2)+
	move.b	(a0)+,(a2)+
l443
	lea	(0+l498,a7),a0
	inline
	move.l	a0,d0
.l1
	tst.b	(a0)+
	bne	.l1
	sub.l	a0,d0
	not.l	d0
	einline
	cmp.l	#160,d0
	bcc	l447
	moveq	#0,d6
l448
	lea	(0+l498,a7),a1
	lea	84+l388,a0
	inline
	move.l	a0,d0
.l1
	move.b	(a1)+,(a0)+
	bne	.l1
	einline
	move.l	d6,-(a7)
	move.l	2284+l388,-(a7)
	move.l	_SysBase,a6
	move.l	#0,a1
	jsr	-294(a6)
	move.l	d0,a0
	move.l	a0,-(a7)
	tst.l	d5
	beq	l453
	lea	l455,a3
	bra	l454
l453
	lea	l456,a3
l454
	move.l	a3,-(a7)
	lea	(256+l498,a7),a2
	move.l	a2,-(a7)
	jsr	_sprintf
	move.l	_DOSBase,a6
	move.l	#84+l388,d1
	move.l	a2,d2
	move.l	#160,d3
	jsr	-882(a6)
	add.w	#20,a7
	tst.w	d0
	beq	l447
	move.l	_DOSBase,a6
	move.l	#84+l388,d1
	jsr	-120(a6)
	move.l	d0,a0
	move.l	a0,d1
	beq	l460
	move.l	_DOSBase,a6
	move.l	a0,d1
	jsr	-90(a6)
	bra	l450
l460
	addq.l	#1,d6
	moveq	#20,d7
	cmp.l	d6,d7
	bgt	l448
l450
	moveq	#20,d7
	cmp.l	d6,d7
	beq	l447
	pea	84+l388
	pea	l463
	lea	(248+l498,a7),a0
	move.l	a0,-(a7)
	jsr	_sprintf
	pea	l393
	move.l	#500,-(a7)
	lea	(460+l498,a7),a0
	move.l	a0,-(a7)
	jsr	_nd_quote_path
	add.w	#24,a7
	tst.l	d0
	beq	l447
	move.l	(2884+l498,a7),-(a7)
	move.l	#840,-(a7)
	lea	(948+l498,a7),a0
	move.l	a0,-(a7)
	jsr	_nd_quote_path
	add.w	#12,a7
	tst.l	d0
	beq	l447
	lea	(240+l498,a7),a0
	move.l	a0,-(a7)
	move.l	#420,-(a7)
	lea	(1788+l498,a7),a0
	move.l	a0,-(a7)
	jsr	_nd_quote_path
	add.w	#12,a7
	tst.l	d0
	beq	l447
	tst.l	d5
	beq	l469
	move.l	#1,a1
	bra	l470
l469
	move.l	#0,a1
l470
	move.l	a1,-(a7)
	lea	(1784+l498,a7),a0
	move.l	a0,-(a7)
	lea	(948+l498,a7),a0
	move.l	a0,-(a7)
	lea	(452+l498,a7),a0
	move.l	a0,-(a7)
	pea	l471
	pea	244+l388
	jsr	_sprintf
	move.l	_SysBase,a6
	jsr	-666(a6)
	move.l	d0,l388
	add.w	#24,a7
	beq	l447
	move.l	2284+l388,-(a7)
	move.l	_SysBase,a6
	move.l	#0,a1
	jsr	-294(a6)
	move.l	d0,a1
	move.l	a1,-(a7)
	pea	l474
	pea	4+l388
	jsr	_sprintf
	move.l	l388,a1
	move.l	#4+l388,(10,a1)
	move.l	_SysBase,a6
	jsr	-132(a6)
	move.l	l388,a1
	move.l	_SysBase,a6
	jsr	-354(a6)
	move.l	_SysBase,a6
	jsr	-138(a6)
	pea	4+l388
	lea	(460+l498,a7),a1
	move.l	a1,-(a7)
	pea	l475
	lea	(2228+l498,a7),a1
	move.l	a1,-(a7)
	jsr	_sprintf
	move.l	_DOSBase,a6
	move.l	#l476,d1
	move.l	#1005,d2
	jsr	-30(a6)
	move.l	d0,d3
	move.l	_DOSBase,a6
	move.l	#l477,d1
	move.l	#1006,d2
	jsr	-30(a6)
	move.l	d0,d4
	add.w	#32,a7
	tst.l	d3
	beq	l478
	tst.l	d4
	bne	l479
l478
	tst.l	d3
	beq	l482
	move.l	_DOSBase,a6
	move.l	d3,d1
	jsr	-36(a6)
l482
	tst.l	d4
	beq	l447
	move.l	_DOSBase,a6
	move.l	d4,d1
	jsr	-36(a6)
	bra	l447
l479
	move.l	#0,-(a7)
	move.l	#32768,-(a7)
	move.l	#-2147482637,-(a7)
	move.l	#1,-(a7)
	move.l	#-2147483613,-(a7)
	move.l	d4,-(a7)
	move.l	#-2147483614,-(a7)
	move.l	d3,-(a7)
	move.l	#-2147483615,-(a7)
	lea	(2236+l498,a7),a0
	move.l	_DOSBase,a6
	move.l	a0,d1
	move.l	d2,-(a7)
	move.l	a7,d2
	add.l	#4,d2
	jsr	-606(a6)
	move.l	(a7)+,d2
	add.w	#36,a7
	moveq	#-1,d1
	cmp.l	d0,d1
	bne	l486
	move.l	_DOSBase,a6
	move.l	d3,d1
	jsr	-36(a6)
	move.l	_DOSBase,a6
	move.l	d4,d1
	jsr	-36(a6)
	bra	l447
l486
	move.l	#0,-(a7)
	move.l	#0,-(a7)
	moveq	#4,d7
	cmp.l	a4,d7
	beq	l487
	moveq	#0,d0
	bra	l488
l487
	moveq	#1,d0
l488
	move.l	d0,-(a7)
	jsr	l88
	add.w	#12,a7
	moveq	#4,d7
	cmp.l	a4,d7
	bne	l491
	move.l	#l493,d1
	bra	l492
l491
	move.l	#l494,d1
l492
	move.l	d1,-(a7)
	jsr	l419
	moveq	#1,d0
	addq.w	#4,a7
	bra	l426
l447
	jsr	l408
	pea	l495
	jsr	l419
	moveq	#0,d0
	addq.w	#4,a7
l426
l496	reg	a2/a3/a4/a6/d2/d3/d4/d5/d6/d7
	movem.l	(a7)+,a2/a3/a4/a6/d2/d3/d4/d5/d6/d7
l498	equ	40
	add.w	#2880,a7
	rts
	cnop	0,4
l430
	dc.b	65
	dc.b	32
	dc.b	116
	dc.b	114
	dc.b	97
	dc.b	110
	dc.b	115
	dc.b	102
	dc.b	101
	dc.b	114
	dc.b	32
	dc.b	105
	dc.b	115
	dc.b	32
	dc.b	97
	dc.b	108
	dc.b	114
	dc.b	101
	dc.b	97
	dc.b	100
	dc.b	121
	dc.b	32
	dc.b	114
	dc.b	117
	dc.b	110
	dc.b	110
	dc.b	105
	dc.b	110
	dc.b	103
	dc.b	46
	dc.b	0
	cnop	0,4
l437
	dc.b	80
	dc.b	114
	dc.b	101
	dc.b	118
	dc.b	105
	dc.b	111
	dc.b	117
	dc.b	115
	dc.b	32
	dc.b	116
	dc.b	114
	dc.b	97
	dc.b	110
	dc.b	115
	dc.b	102
	dc.b	101
	dc.b	114
	dc.b	32
	dc.b	105
	dc.b	115
	dc.b	32
	dc.b	102
	dc.b	105
	dc.b	110
	dc.b	105
	dc.b	115
	dc.b	104
	dc.b	105
	dc.b	110
	dc.b	103
	dc.b	59
	dc.b	32
	dc.b	116
	dc.b	114
	dc.b	121
	dc.b	32
	dc.b	97
	dc.b	103
	dc.b	97
	dc.b	105
	dc.b	110
	dc.b	32
	dc.b	115
	dc.b	104
	dc.b	111
	dc.b	114
	dc.b	116
	dc.b	108
	dc.b	121
	dc.b	46
	dc.b	0
	cnop	0,4
l442
	dc.b	68
	dc.b	101
	dc.b	115
	dc.b	116
	dc.b	105
	dc.b	110
	dc.b	97
	dc.b	116
	dc.b	105
	dc.b	111
	dc.b	110
	dc.b	32
	dc.b	101
	dc.b	120
	dc.b	105
	dc.b	115
	dc.b	116
	dc.b	115
	dc.b	59
	dc.b	32
	dc.b	99
	dc.b	104
	dc.b	111
	dc.b	111
	dc.b	115
	dc.b	101
	dc.b	32
	dc.b	97
	dc.b	110
	dc.b	111
	dc.b	116
	dc.b	104
	dc.b	101
	dc.b	114
	dc.b	32
	dc.b	102
	dc.b	105
	dc.b	108
	dc.b	101
	dc.b	110
	dc.b	97
	dc.b	109
	dc.b	101
	dc.b	46
	dc.b	0
	cnop	0,4
l455
	dc.b	78
	dc.b	111
	dc.b	118
	dc.b	97
	dc.b	68
	dc.b	111
	dc.b	99
	dc.b	107
	dc.b	68
	dc.b	111
	dc.b	119
	dc.b	110
	dc.b	108
	dc.b	111
	dc.b	97
	dc.b	100
	dc.b	45
	dc.b	37
	dc.b	48
	dc.b	56
	dc.b	108
	dc.b	120
	dc.b	45
	dc.b	37
	dc.b	48
	dc.b	56
	dc.b	108
	dc.b	120
	dc.b	45
	dc.b	37
	dc.b	100
	dc.b	0
	cnop	0,4
l456
	dc.b	78
	dc.b	111
	dc.b	118
	dc.b	97
	dc.b	68
	dc.b	111
	dc.b	99
	dc.b	107
	dc.b	65
	dc.b	109
	dc.b	105
	dc.b	110
	dc.b	101
	dc.b	116
	dc.b	45
	dc.b	37
	dc.b	48
	dc.b	56
	dc.b	108
	dc.b	120
	dc.b	45
	dc.b	37
	dc.b	48
	dc.b	56
	dc.b	108
	dc.b	120
	dc.b	45
	dc.b	37
	dc.b	100
	dc.b	0
	cnop	0,4
l434
	dc.b	78
	dc.b	111
	dc.b	118
	dc.b	97
	dc.b	68
	dc.b	111
	dc.b	99
	dc.b	107
	dc.b	46
	dc.b	65
	dc.b	109
	dc.b	105
	dc.b	110
	dc.b	101
	dc.b	116
	dc.b	46
	dc.b	70
	dc.b	101
	dc.b	116
	dc.b	99
	dc.b	104
	dc.b	0
	cnop	0,4
l444
	dc.b	84
	dc.b	58
	dc.b	0
	cnop	0,4
l463
	dc.b	37
	dc.b	115
	dc.b	47
	dc.b	114
	dc.b	101
	dc.b	115
	dc.b	112
	dc.b	111
	dc.b	110
	dc.b	115
	dc.b	101
	dc.b	0
	cnop	0,4
l471
	dc.b	37
	dc.b	115
	dc.b	32
	dc.b	70
	dc.b	69
	dc.b	84
	dc.b	67
	dc.b	72
	dc.b	32
	dc.b	37
	dc.b	115
	dc.b	32
	dc.b	37
	dc.b	115
	dc.b	32
	dc.b	37
	dc.b	100
	dc.b	0
	cnop	0,4
l474
	dc.b	78
	dc.b	111
	dc.b	118
	dc.b	97
	dc.b	68
	dc.b	111
	dc.b	99
	dc.b	107
	dc.b	46
	dc.b	65
	dc.b	109
	dc.b	105
	dc.b	110
	dc.b	101
	dc.b	116
	dc.b	46
	dc.b	37
	dc.b	48
	dc.b	56
	dc.b	108
	dc.b	120
	dc.b	46
	dc.b	37
	dc.b	48
	dc.b	56
	dc.b	108
	dc.b	120
	dc.b	0
	cnop	0,4
l475
	dc.b	37
	dc.b	115
	dc.b	32
	dc.b	87
	dc.b	79
	dc.b	82
	dc.b	75
	dc.b	32
	dc.b	37
	dc.b	115
	dc.b	0
	cnop	0,4
l476
	dc.b	78
	dc.b	73
	dc.b	76
	dc.b	58
	dc.b	0
	cnop	0,4
l477
	dc.b	78
	dc.b	73
	dc.b	76
	dc.b	58
	dc.b	0
	cnop	0,4
l493
	dc.b	67
	dc.b	111
	dc.b	110
	dc.b	110
	dc.b	101
	dc.b	99
	dc.b	116
	dc.b	105
	dc.b	110
	dc.b	103
	dc.b	32
	dc.b	116
	dc.b	111
	dc.b	32
	dc.b	100
	dc.b	111
	dc.b	119
	dc.b	110
	dc.b	108
	dc.b	111
	dc.b	97
	dc.b	100
	dc.b	32
	dc.b	112
	dc.b	97
	dc.b	99
	dc.b	107
	dc.b	97
	dc.b	103
	dc.b	101
	dc.b	46
	dc.b	46
	dc.b	46
	dc.b	0
	cnop	0,4
l494
	dc.b	76
	dc.b	111
	dc.b	97
	dc.b	100
	dc.b	105
	dc.b	110
	dc.b	103
	dc.b	32
	dc.b	65
	dc.b	109
	dc.b	105
	dc.b	110
	dc.b	101
	dc.b	116
	dc.b	46
	dc.b	46
	dc.b	46
	dc.b	0
	cnop	0,4
l495
	dc.b	67
	dc.b	111
	dc.b	117
	dc.b	108
	dc.b	100
	dc.b	32
	dc.b	110
	dc.b	111
	dc.b	116
	dc.b	32
	dc.b	115
	dc.b	116
	dc.b	97
	dc.b	114
	dc.b	116
	dc.b	32
	dc.b	116
	dc.b	114
	dc.b	97
	dc.b	110
	dc.b	115
	dc.b	102
	dc.b	101
	dc.b	114
	dc.b	59
	dc.b	32
	dc.b	99
	dc.b	104
	dc.b	101
	dc.b	99
	dc.b	107
	dc.b	32
	dc.b	100
	dc.b	101
	dc.b	115
	dc.b	116
	dc.b	105
	dc.b	110
	dc.b	97
	dc.b	116
	dc.b	105
	dc.b	111
	dc.b	110
	dc.b	47
	dc.b	102
	dc.b	114
	dc.b	101
	dc.b	101
	dc.b	32
	dc.b	115
	dc.b	112
	dc.b	97
	dc.b	99
	dc.b	101
	dc.b	46
	dc.b	0
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	cnop	0,4
l499
	sub.w	#160,a7
	movem.l	l511,-(a7)
	tst.l	l87
	beq	l500
	jsr	l124
	moveq	#0,d2
	tst.l	10900+l389
	ble	l510
l509
	move.w	d2,d0
	muls.w	#218,d0
	lea	97+l389,a0
	pea	(0,a0,d0.w)
	lea	l389,a0
	add.w	d0,a0
	move.l	_DOSBase,a6
	move.l	a0,d1
	jsr	-870(a6)
	move.l	d0,-(a7)
	pea	l508
	lea	(12+l513,a7),a2
	move.l	a2,-(a7)
	jsr	_sprintf
	move.l	a2,-(a7)
	jsr	l130
	addq.l	#1,d2
	add.w	#20,a7
	cmp.l	10900+l389,d2
	blt	l509
l510
	jsr	l138
	move.l	#0,l397
	move.b	#0,l396
l500
l511	reg	a2/a6/d2
	movem.l	(a7)+,a2/a6/d2
l513	equ	12
	add.w	#160,a7
	rts
	cnop	0,4
l508
	dc.b	37
	dc.b	45
	dc.b	50
	dc.b	53
	dc.b	46
	dc.b	50
	dc.b	53
	dc.b	115
	dc.b	32
	dc.b	37
	dc.b	46
	dc.b	49
	dc.b	48
	dc.b	48
	dc.b	115
	dc.b	0
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	cnop	0,4
l514
	movem.l	l562,-(a7)
	moveq	#17,d4
	tst.l	l87
	beq	l515
	move.b	#0,l396
	jsr	l124
	lea	l390,a0
	moveq	#0,d0
	move.l	#10908,d2
	inline
	move.l	a0,a1
	cmp.l	#16,d2
	blo	.l3
	move.l	a0,d1
	and.b	#1,d1
	beq	.l1
	move.b	d0,(a0)+
	subq.l	#1,d2
.l1
	move.b	d0,d1
	lsl.w	#8,d0
	move.b	d1,d0
	move.w	d0,d1
	swap	d0
	move.w	d1,d0
	moveq	#3,d1
	and.l	d2,d1
	sub.l	d1,d2
.l2
	move.l	d0,(a0)+
	subq.l	#4,d2
	bne	.l2
	move.w	d1,d2
.l3
	subq.w	#1,d2
	blo	.l5
.l4
	move.b	d0,(a0)+
	dbf	d2,.l4
.l5
	move.l	a1,d0
	einline
	moveq	#0,d3
l560
	move.w	d3,d0
	move.w	d0,d1
	lsl.w	#2,d1
	lea	l535,a1
	move.l	(0,a1,d1.w),a1
	muls.w	#218,d0
	lea	l390,a0
	add.w	d0,a0
	inline
	move.l	a0,d0
.l1
	move.b	(a1)+,(a0)+
	bne	.l1
	einline
	lea	l517,a0
	move.l	(0,a0,d1.w),-(a7)
	jsr	l130
	addq.l	#1,d3
	addq.w	#4,a7
	cmp.l	d3,d4
	bgt	l560
	move.l	d4,10900+l390
	jsr	l138
	moveq	#1,d0
	move.l	d0,l397
	pea	l559
	jsr	l419
	addq.w	#4,a7
l515
l562	reg	d2/d3/d4
	movem.l	(a7)+,d2/d3/d4
l564	equ	12
	rts
	cnop	0,4
l518
	dc.b	66
	dc.b	117
	dc.b	115
	dc.b	105
	dc.b	110
	dc.b	101
	dc.b	115
	dc.b	115
	dc.b	0
	cnop	0,4
l519
	dc.b	67
	dc.b	111
	dc.b	109
	dc.b	109
	dc.b	117
	dc.b	110
	dc.b	105
	dc.b	99
	dc.b	97
	dc.b	116
	dc.b	105
	dc.b	111
	dc.b	110
	dc.b	0
	cnop	0,4
l520
	dc.b	68
	dc.b	101
	dc.b	109
	dc.b	111
	dc.b	115
	dc.b	0
	cnop	0,4
l521
	dc.b	68
	dc.b	101
	dc.b	118
	dc.b	101
	dc.b	108
	dc.b	111
	dc.b	112
	dc.b	109
	dc.b	101
	dc.b	110
	dc.b	116
	dc.b	0
	cnop	0,4
l522
	dc.b	68
	dc.b	105
	dc.b	115
	dc.b	107
	dc.b	32
	dc.b	116
	dc.b	111
	dc.b	111
	dc.b	108
	dc.b	115
	dc.b	0
	cnop	0,4
l523
	dc.b	68
	dc.b	111
	dc.b	99
	dc.b	117
	dc.b	109
	dc.b	101
	dc.b	110
	dc.b	116
	dc.b	97
	dc.b	116
	dc.b	105
	dc.b	111
	dc.b	110
	dc.b	0
	cnop	0,4
l524
	dc.b	68
	dc.b	114
	dc.b	105
	dc.b	118
	dc.b	101
	dc.b	114
	dc.b	115
	dc.b	0
	cnop	0,4
l525
	dc.b	71
	dc.b	97
	dc.b	109
	dc.b	101
	dc.b	115
	dc.b	0
	cnop	0,4
l526
	dc.b	71
	dc.b	114
	dc.b	97
	dc.b	112
	dc.b	104
	dc.b	105
	dc.b	99
	dc.b	115
	dc.b	0
	cnop	0,4
l527
	dc.b	73
	dc.b	110
	dc.b	102
	dc.b	111
	dc.b	114
	dc.b	109
	dc.b	97
	dc.b	116
	dc.b	105
	dc.b	111
	dc.b	110
	dc.b	0
	cnop	0,4
l528
	dc.b	77
	dc.b	97
	dc.b	103
	dc.b	97
	dc.b	122
	dc.b	105
	dc.b	110
	dc.b	101
	dc.b	115
	dc.b	0
	cnop	0,4
l529
	dc.b	77
	dc.b	105
	dc.b	115
	dc.b	99
	dc.b	101
	dc.b	108
	dc.b	108
	dc.b	97
	dc.b	110
	dc.b	101
	dc.b	111
	dc.b	117
	dc.b	115
	dc.b	0
	cnop	0,4
l530
	dc.b	77
	dc.b	111
	dc.b	100
	dc.b	117
	dc.b	108
	dc.b	101
	dc.b	115
	dc.b	0
	cnop	0,4
l531
	dc.b	77
	dc.b	117
	dc.b	115
	dc.b	105
	dc.b	99
	dc.b	0
	cnop	0,4
l532
	dc.b	80
	dc.b	105
	dc.b	99
	dc.b	116
	dc.b	117
	dc.b	114
	dc.b	101
	dc.b	115
	dc.b	0
	cnop	0,4
l533
	dc.b	84
	dc.b	101
	dc.b	120
	dc.b	116
	dc.b	0
	cnop	0,4
l534
	dc.b	85
	dc.b	116
	dc.b	105
	dc.b	108
	dc.b	105
	dc.b	116
	dc.b	105
	dc.b	101
	dc.b	115
	dc.b	0
	cnop	0,4
l536
	dc.b	98
	dc.b	105
	dc.b	122
	dc.b	0
	cnop	0,4
l537
	dc.b	99
	dc.b	111
	dc.b	109
	dc.b	109
	dc.b	0
	cnop	0,4
l538
	dc.b	100
	dc.b	101
	dc.b	109
	dc.b	111
	dc.b	0
	cnop	0,4
l539
	dc.b	100
	dc.b	101
	dc.b	118
	dc.b	0
	cnop	0,4
l540
	dc.b	100
	dc.b	105
	dc.b	115
	dc.b	107
	dc.b	0
	cnop	0,4
l541
	dc.b	100
	dc.b	111
	dc.b	99
	dc.b	115
	dc.b	0
	cnop	0,4
l542
	dc.b	100
	dc.b	114
	dc.b	105
	dc.b	118
	dc.b	101
	dc.b	114
	dc.b	0
	cnop	0,4
l543
	dc.b	103
	dc.b	97
	dc.b	109
	dc.b	101
	dc.b	0
	cnop	0,4
l544
	dc.b	103
	dc.b	102
	dc.b	120
	dc.b	0
	cnop	0,4
l545
	dc.b	105
	dc.b	110
	dc.b	102
	dc.b	111
	dc.b	0
	cnop	0,4
l546
	dc.b	109
	dc.b	97
	dc.b	103
	dc.b	115
	dc.b	0
	cnop	0,4
l547
	dc.b	109
	dc.b	105
	dc.b	115
	dc.b	99
	dc.b	0
	cnop	0,4
l548
	dc.b	109
	dc.b	111
	dc.b	100
	dc.b	115
	dc.b	0
	cnop	0,4
l549
	dc.b	109
	dc.b	117
	dc.b	115
	dc.b	0
	cnop	0,4
l550
	dc.b	112
	dc.b	105
	dc.b	120
	dc.b	0
	cnop	0,4
l551
	dc.b	116
	dc.b	101
	dc.b	120
	dc.b	116
	dc.b	0
	cnop	0,4
l552
	dc.b	117
	dc.b	116
	dc.b	105
	dc.b	108
	dc.b	0
	cnop	0,4
l559
	dc.b	83
	dc.b	101
	dc.b	108
	dc.b	101
	dc.b	99
	dc.b	116
	dc.b	32
	dc.b	97
	dc.b	32
	dc.b	99
	dc.b	97
	dc.b	116
	dc.b	101
	dc.b	103
	dc.b	111
	dc.b	114
	dc.b	121
	dc.b	44
	dc.b	32
	dc.b	116
	dc.b	104
	dc.b	101
	dc.b	110
	dc.b	32
	dc.b	79
	dc.b	112
	dc.b	101
	dc.b	110
	dc.b	46
	dc.b	32
	dc.b	78
	dc.b	111
	dc.b	32
	dc.b	112
	dc.b	97
	dc.b	99
	dc.b	107
	dc.b	97
	dc.b	103
	dc.b	101
	dc.b	32
	dc.b	114
	dc.b	117
	dc.b	110
	dc.b	115
	dc.b	32
	dc.b	97
	dc.b	117
	dc.b	116
	dc.b	111
	dc.b	109
	dc.b	97
	dc.b	116
	dc.b	105
	dc.b	99
	dc.b	97
	dc.b	108
	dc.b	108
	dc.b	121
	dc.b	46
	dc.b	0
	section	"DATA",data
	cnop	0,4
l517
	dc.l	l518
	dc.l	l519
	dc.l	l520
	dc.l	l521
	dc.l	l522
	dc.l	l523
	dc.l	l524
	dc.l	l525
	dc.l	l526
	dc.l	l527
	dc.l	l528
	dc.l	l529
	dc.l	l530
	dc.l	l531
	dc.l	l532
	dc.l	l533
	dc.l	l534
	cnop	0,4
l535
	dc.l	l536
	dc.l	l537
	dc.l	l538
	dc.l	l539
	dc.l	l540
	dc.l	l541
	dc.l	l542
	dc.l	l543
	dc.l	l544
	dc.l	l545
	dc.l	l546
	dc.l	l547
	dc.l	l548
	dc.l	l549
	dc.l	l550
	dc.l	l551
	dc.l	l552
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	section	"CODE",code
	cnop	0,4
l565
	sub.w	#644,a7
	movem.l	l580,-(a7)
	tst.l	l398
	bne	l569
	lea	l570,a0
	lea	(0+l582,a7),a1
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	bra	l575
l569
	moveq	#1,d0
	cmp.l	l398,d0
	bne	l573
	move.l	l399,-(a7)
	pea	l394
	pea	l574
	lea	(12+l582,a7),a0
	move.l	a0,-(a7)
	jsr	_sprintf
	add.w	#16,a7
	bra	l575
l573
	move.l	#241,-(a7)
	lea	(404+l582,a7),a0
	move.l	a0,-(a7)
	pea	l395
	jsr	_nd_aminet_encode_query
	add.w	#12,a7
	tst.l	d0
	bne	l577
	pea	l578
	jsr	l419
	moveq	#0,d0
	addq.w	#4,a7
	bra	l566
l577
	move.l	l399,-(a7)
	lea	(404+l582,a7),a0
	move.l	a0,-(a7)
	pea	l579
	lea	(12+l582,a7),a0
	move.l	a0,-(a7)
	jsr	_sprintf
	add.w	#16,a7
l575
	move.l	#0,-(a7)
	move.l	#1,-(a7)
	lea	(8+l582,a7),a0
	move.l	a0,-(a7)
	jsr	l425
	add.w	#12,a7
l566
l580	reg
l582	equ	0
	add.w	#644,a7
	rts
	cnop	0,4
l578
	dc.b	69
	dc.b	110
	dc.b	116
	dc.b	101
	dc.b	114
	dc.b	32
	dc.b	50
	dc.b	45
	dc.b	56
	dc.b	48
	dc.b	32
	dc.b	115
	dc.b	101
	dc.b	97
	dc.b	114
	dc.b	99
	dc.b	104
	dc.b	32
	dc.b	99
	dc.b	104
	dc.b	97
	dc.b	114
	dc.b	97
	dc.b	99
	dc.b	116
	dc.b	101
	dc.b	114
	dc.b	115
	dc.b	46
	dc.b	0
	cnop	0,4
l579
	dc.b	47
	dc.b	115
	dc.b	101
	dc.b	97
	dc.b	114
	dc.b	99
	dc.b	104
	dc.b	63
	dc.b	113
	dc.b	117
	dc.b	101
	dc.b	114
	dc.b	121
	dc.b	61
	dc.b	37
	dc.b	115
	dc.b	38
	dc.b	112
	dc.b	97
	dc.b	103
	dc.b	101
	dc.b	61
	dc.b	37
	dc.b	100
	dc.b	0
	cnop	0,4
l570
	dc.b	47
	dc.b	114
	dc.b	101
	dc.b	99
	dc.b	101
	dc.b	110
	dc.b	116
	dc.b	0
	cnop	0,4
l574
	dc.b	47
	dc.b	37
	dc.b	115
	dc.b	63
	dc.b	112
	dc.b	97
	dc.b	103
	dc.b	101
	dc.b	61
	dc.b	37
	dc.b	100
	dc.b	0
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	cnop	0,4
l583
	sub.w	#120,a7
	movem.l	l594,-(a7)
	tst.b	l396
	bne	l587
	pea	l588
	jsr	l419
	addq.w	#4,a7
	bra	l590
l587
	move.l	#120,-(a7)
	lea	(4+l596,a7),a0
	move.l	a0,-(a7)
	pea	l396
	jsr	_nd_aminet_readme_path
	add.w	#12,a7
	tst.l	d0
	beq	l590
	move.l	#0,-(a7)
	tst.l	(128+l596,a7)
	beq	l592
	moveq	#3,d0
	bra	l593
l592
	moveq	#2,d0
l593
	move.l	d0,-(a7)
	lea	(8+l596,a7),a0
	move.l	a0,-(a7)
	jsr	l425
	add.w	#12,a7
l590
l594	reg
l596	equ	0
	add.w	#120,a7
	rts
	cnop	0,4
l588
	dc.b	83
	dc.b	101
	dc.b	108
	dc.b	101
	dc.b	99
	dc.b	116
	dc.b	32
	dc.b	97
	dc.b	32
	dc.b	112
	dc.b	97
	dc.b	99
	dc.b	107
	dc.b	97
	dc.b	103
	dc.b	101
	dc.b	32
	dc.b	102
	dc.b	105
	dc.b	114
	dc.b	115
	dc.b	116
	dc.b	46
	dc.b	0
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	cnop	0,4
l597
	sub.w	#772,a7
	movem.l	l678,-(a7)
	moveq	#32,d6
	move.l	#89130,d5
	move.l	#160,a5
	move.l	#512,a4
	move.l	(780+l680,a7),a2
	moveq	#0,d3
	move.l	#0,a3
	pea	l600
	move.l	(780+l680,a7),-(a7)
	jsr	_fopen
	move.l	d0,d4
	addq.w	#8,a7
	bne	l602
	pea	l603
	jsr	l419
	addq.w	#4,a7
	bra	l670
l602
	moveq	#1,d7
	cmp.l	a2,d7
	beq	l604
	moveq	#5,d7
	cmp.l	a2,d7
	bne	l608
l604
	move.l	_SysBase,a6
	move.l	#10908,d0
	move.l	#65537,d1
	jsr	-684(a6)
	move.l	d0,a3
	tst.l	d0
	bne	l608
	move.l	d4,-(a7)
	jsr	_fclose
	pea	l609
	jsr	l419
	addq.w	#8,a7
	bra	l670
l608
	move.l	a2,(780+l680,a7)
	moveq	#1,d7
	cmp.l	a2,d7
	bne	l611
	move.l	a3,-(a7)
	tst.l	l398
	bne	l613
	move.l	l399,d0
	subq.l	#1,d0
	moveq	#50,d1
	move.l	d2,-(a7)
	move.l	d1,d7
	move.l	d0,d2
	swap	d7
	swap	d2
	mulu.w	d0,d7
	mulu.w	d1,d2
	mulu.w	d0,d1
	add.w	d2,d7
	swap	d7
	clr.w	d7
	add.l	d7,d1
	move.l	(a7)+,d2
	bra	l614
l613
	moveq	#0,d1
l614
	move.l	d1,-(a7)
	move.l	d4,-(a7)
	jsr	_nd_aminet_search
	move.l	d0,d3
	add.w	#12,a7
	beq	l646
	lea	(a3),a1
	lea	l389,a2
	move.l	#680,d7
l681:
	move.l	(a1)+,(a2)+
	move.l	(a1)+,(a2)+
	move.l	(a1)+,(a2)+
	move.l	(a1)+,(a2)+
	dbra	d7,l681
	move.l	(a1)+,(a2)+
	move.l	(a1)+,(a2)+
	move.l	(a1)+,(a2)+
	jsr	l499
	move.l	10900+l389,-(a7)
	move.l	l399,-(a7)
	pea	l617
	lea	(16+l680,a7),a2
	move.l	a2,-(a7)
	jsr	_sprintf
	move.l	a2,-(a7)
	jsr	l419
	add.w	#20,a7
	bra	l646
l611
	moveq	#5,d7
	cmp.l	a2,d7
	bne	l620
	move.l	a3,-(a7)
	pea	l394
	move.l	d4,-(a7)
	jsr	_nd_aminet_categories
	move.l	d0,d3
	add.w	#12,a7
	beq	l622
	lea	(a3),a1
	lea	l390,a6
	move.l	#680,d7
l682:
	move.l	(a1)+,(a6)+
	move.l	(a1)+,(a6)+
	move.l	(a1)+,(a6)+
	move.l	(a1)+,(a6)+
	dbra	d7,l682
	move.l	(a1)+,(a6)+
	move.l	(a1)+,(a6)+
	move.l	(a1)+,(a6)+
l622
	tst.l	d3
	beq	l646
	tst.l	l87
	beq	l646
	jsr	l124
	moveq	#0,d2
	tst.l	10900+l390
	ble	l675
l672
	move.w	d2,d0
	muls.w	#218,d0
	lea	l390,a0
	pea	(0,a0,d0.w)
	jsr	l130
	addq.l	#1,d2
	addq.w	#4,a7
	cmp.l	10900+l390,d2
	blt	l672
l675
	jsr	l138
	moveq	#2,d7
	move.l	d7,l397
	move.b	#0,l396
	pea	l630
	jsr	l419
	addq.w	#4,a7
	bra	l646
l620
	move.l	a2,(780+l680,a7)
	moveq	#2,d7
	cmp.l	a2,d7
	bne	l633
	lea	(204+l680,a7),a0
	move.l	a0,-(a7)
	move.l	d4,-(a7)
	jsr	_nd_aminet_details
	move.l	d0,d3
	addq.w	#8,a7
	beq	l646
	tst.l	l87
	beq	l646
	jsr	l124
	pea	l396
	jsr	l130
	lea	(208+l680,a7),a0
	move.l	a0,-(a7)
	jsr	l130
	lea	(333+l680,a7),a0
	move.l	a0,-(a7)
	pea	l637
	lea	(20+l680,a7),a2
	move.l	a2,-(a7)
	jsr	_sprintf
	move.l	a2,-(a7)
	jsr	l130
	lea	(551+l680,a7),a0
	move.l	a0,-(a7)
	pea	l638
	move.l	a2,-(a7)
	jsr	_sprintf
	move.l	a2,-(a7)
	jsr	l130
	lea	(688+l680,a7),a0
	move.l	a0,-(a7)
	pea	l639
	move.l	a2,-(a7)
	jsr	_sprintf
	move.l	a2,-(a7)
	jsr	l130
	lea	(462+l680,a7),a0
	move.l	a0,-(a7)
	pea	l640
	move.l	a2,-(a7)
	jsr	_sprintf
	move.l	a2,-(a7)
	jsr	l130
	jsr	l138
	moveq	#3,d7
	move.l	d7,l397
	pea	l641
	jsr	l419
	add.w	#76,a7
	bra	l646
l633
	moveq	#3,d7
	cmp.l	a2,d7
	bne	l646
	moveq	#1,d3
	tst.l	l87
	beq	l646
	jsr	l124
	move.l	l87,a0
	move.l	d3,(0+l680,a7)
	cmp.l	(0,a0,d5.l),a4
	ble	l676
	move.l	d4,-(a7)
	move.l	a5,-(a7)
	lea	(12+l680,a7),a0
	move.l	a0,-(a7)
	jsr	_fgets
	move.l	d3,(12+l680,a7)
	add.w	#12,a7
	tst.l	d0
	beq	l676
	moveq	#10,d3
	move.l	#512,d2
l647
	lea	(4+l680,a7),a0
	tst.b	(a0)
	beq	l677
	moveq	#13,d1
	moveq	#127,d0
l651
	cmp.b	(a0),d3
	beq	l655
	cmp.b	(a0),d1
	bne	l656
l655
	move.b	#0,(a0)
	bra	l677
l656
	cmp.b	(a0),d6
	bhi	l658
	cmp.b	(a0),d0
	bne	l659
l658
	move.b	d6,(a0)
l659
	addq.l	#1,a0
	tst.b	(a0)
	bne	l651
l677
	lea	(4+l680,a7),a0
	move.l	a0,-(a7)
	jsr	l130
	move.l	l87,a0
	addq.w	#4,a7
	cmp.l	(0,a0,d5.l),d2
	ble	l676
	move.l	d4,-(a7)
	move.l	a5,-(a7)
	lea	(12+l680,a7),a0
	move.l	a0,-(a7)
	jsr	_fgets
	add.w	#12,a7
	tst.l	d0
	bne	l647
l676
	move.l	(0+l680,a7),d3
	move.l	#512,a4
	jsr	l138
	moveq	#4,d7
	move.l	d7,l397
	move.l	l87,a0
	cmp.l	(0,a0,d5.l),a4
	bne	l663
	move.l	#l665,d0
	bra	l664
l663
	move.l	#l666,d0
l664
	move.l	d0,-(a7)
	jsr	l419
	addq.w	#4,a7
l646
	move.l	a3,d7
	beq	l668
	move.l	_SysBase,a6
	move.l	a3,a1
	jsr	-690(a6)
l668
	move.l	d4,-(a7)
	jsr	_fclose
	addq.w	#4,a7
	tst.l	d3
	bne	l670
	pea	l671
	jsr	l419
	addq.w	#4,a7
l670
l678	reg	a2/a3/a4/a5/a6/d2/d3/d4/d5/d6/d7
	movem.l	(a7)+,a2/a3/a4/a5/a6/d2/d3/d4/d5/d6/d7
l680	equ	44
	add.w	#772,a7
	rts
	cnop	0,4
l603
	dc.b	67
	dc.b	97
	dc.b	110
	dc.b	110
	dc.b	111
	dc.b	116
	dc.b	32
	dc.b	114
	dc.b	101
	dc.b	97
	dc.b	100
	dc.b	32
	dc.b	100
	dc.b	111
	dc.b	119
	dc.b	110
	dc.b	108
	dc.b	111
	dc.b	97
	dc.b	100
	dc.b	101
	dc.b	100
	dc.b	32
	dc.b	109
	dc.b	101
	dc.b	116
	dc.b	97
	dc.b	100
	dc.b	97
	dc.b	116
	dc.b	97
	dc.b	46
	dc.b	0
	cnop	0,4
l609
	dc.b	78
	dc.b	111
	dc.b	116
	dc.b	32
	dc.b	101
	dc.b	110
	dc.b	111
	dc.b	117
	dc.b	103
	dc.b	104
	dc.b	32
	dc.b	109
	dc.b	101
	dc.b	109
	dc.b	111
	dc.b	114
	dc.b	121
	dc.b	32
	dc.b	116
	dc.b	111
	dc.b	32
	dc.b	108
	dc.b	111
	dc.b	97
	dc.b	100
	dc.b	32
	dc.b	116
	dc.b	104
	dc.b	105
	dc.b	115
	dc.b	32
	dc.b	112
	dc.b	97
	dc.b	103
	dc.b	101
	dc.b	46
	dc.b	0
	cnop	0,4
l617
	dc.b	80
	dc.b	97
	dc.b	103
	dc.b	101
	dc.b	32
	dc.b	37
	dc.b	100
	dc.b	58
	dc.b	32
	dc.b	37
	dc.b	100
	dc.b	32
	dc.b	112
	dc.b	97
	dc.b	99
	dc.b	107
	dc.b	97
	dc.b	103
	dc.b	101
	dc.b	115
	dc.b	46
	dc.b	32
	dc.b	79
	dc.b	112
	dc.b	101
	dc.b	110
	dc.b	32
	dc.b	102
	dc.b	111
	dc.b	114
	dc.b	32
	dc.b	100
	dc.b	101
	dc.b	116
	dc.b	97
	dc.b	105
	dc.b	108
	dc.b	115
	dc.b	59
	dc.b	32
	dc.b	71
	dc.b	101
	dc.b	116
	dc.b	32
	dc.b	116
	dc.b	111
	dc.b	32
	dc.b	100
	dc.b	111
	dc.b	119
	dc.b	110
	dc.b	108
	dc.b	111
	dc.b	97
	dc.b	100
	dc.b	46
	dc.b	0
	cnop	0,4
l630
	dc.b	83
	dc.b	101
	dc.b	108
	dc.b	101
	dc.b	99
	dc.b	116
	dc.b	32
	dc.b	97
	dc.b	32
	dc.b	115
	dc.b	117
	dc.b	98
	dc.b	99
	dc.b	97
	dc.b	116
	dc.b	101
	dc.b	103
	dc.b	111
	dc.b	114
	dc.b	121
	dc.b	44
	dc.b	32
	dc.b	116
	dc.b	104
	dc.b	101
	dc.b	110
	dc.b	32
	dc.b	79
	dc.b	112
	dc.b	101
	dc.b	110
	dc.b	46
	dc.b	0
	cnop	0,4
l637
	dc.b	86
	dc.b	101
	dc.b	114
	dc.b	115
	dc.b	105
	dc.b	111
	dc.b	110
	dc.b	58
	dc.b	32
	dc.b	37
	dc.b	46
	dc.b	56
	dc.b	48
	dc.b	115
	dc.b	0
	cnop	0,4
l638
	dc.b	65
	dc.b	114
	dc.b	99
	dc.b	104
	dc.b	105
	dc.b	116
	dc.b	101
	dc.b	99
	dc.b	116
	dc.b	117
	dc.b	114
	dc.b	101
	dc.b	58
	dc.b	32
	dc.b	37
	dc.b	46
	dc.b	49
	dc.b	50
	dc.b	48
	dc.b	115
	dc.b	0
	cnop	0,4
l639
	dc.b	82
	dc.b	101
	dc.b	113
	dc.b	117
	dc.b	105
	dc.b	114
	dc.b	101
	dc.b	115
	dc.b	58
	dc.b	32
	dc.b	37
	dc.b	46
	dc.b	49
	dc.b	50
	dc.b	48
	dc.b	115
	dc.b	0
	cnop	0,4
l640
	dc.b	65
	dc.b	117
	dc.b	116
	dc.b	104
	dc.b	111
	dc.b	114
	dc.b	58
	dc.b	32
	dc.b	37
	dc.b	46
	dc.b	49
	dc.b	50
	dc.b	48
	dc.b	115
	dc.b	0
	cnop	0,4
l641
	dc.b	82
	dc.b	101
	dc.b	97
	dc.b	100
	dc.b	109
	dc.b	101
	dc.b	32
	dc.b	115
	dc.b	104
	dc.b	111
	dc.b	119
	dc.b	115
	dc.b	32
	dc.b	116
	dc.b	104
	dc.b	101
	dc.b	32
	dc.b	102
	dc.b	117
	dc.b	108
	dc.b	108
	dc.b	32
	dc.b	116
	dc.b	101
	dc.b	120
	dc.b	116
	dc.b	46
	dc.b	32
	dc.b	71
	dc.b	101
	dc.b	116
	dc.b	32
	dc.b	99
	dc.b	104
	dc.b	111
	dc.b	111
	dc.b	115
	dc.b	101
	dc.b	115
	dc.b	32
	dc.b	119
	dc.b	104
	dc.b	101
	dc.b	114
	dc.b	101
	dc.b	32
	dc.b	116
	dc.b	111
	dc.b	32
	dc.b	115
	dc.b	97
	dc.b	118
	dc.b	101
	dc.b	46
	dc.b	0
	cnop	0,4
l665
	dc.b	82
	dc.b	101
	dc.b	97
	dc.b	100
	dc.b	109
	dc.b	101
	dc.b	58
	dc.b	32
	dc.b	102
	dc.b	105
	dc.b	114
	dc.b	115
	dc.b	116
	dc.b	32
	dc.b	53
	dc.b	49
	dc.b	50
	dc.b	32
	dc.b	108
	dc.b	105
	dc.b	110
	dc.b	101
	dc.b	115
	dc.b	46
	dc.b	32
	dc.b	66
	dc.b	97
	dc.b	99
	dc.b	107
	dc.b	32
	dc.b	114
	dc.b	101
	dc.b	116
	dc.b	117
	dc.b	114
	dc.b	110
	dc.b	115
	dc.b	32
	dc.b	116
	dc.b	111
	dc.b	32
	dc.b	112
	dc.b	97
	dc.b	99
	dc.b	107
	dc.b	97
	dc.b	103
	dc.b	101
	dc.b	115
	dc.b	46
	dc.b	0
	cnop	0,4
l666
	dc.b	82
	dc.b	101
	dc.b	97
	dc.b	100
	dc.b	109
	dc.b	101
	dc.b	46
	dc.b	32
	dc.b	66
	dc.b	97
	dc.b	99
	dc.b	107
	dc.b	32
	dc.b	114
	dc.b	101
	dc.b	116
	dc.b	117
	dc.b	114
	dc.b	110
	dc.b	115
	dc.b	32
	dc.b	116
	dc.b	111
	dc.b	32
	dc.b	112
	dc.b	97
	dc.b	99
	dc.b	107
	dc.b	97
	dc.b	103
	dc.b	101
	dc.b	115
	dc.b	46
	dc.b	0
	cnop	0,4
l600
	dc.b	114
	dc.b	98
	dc.b	0
	cnop	0,4
l671
	dc.b	67
	dc.b	97
	dc.b	110
	dc.b	110
	dc.b	111
	dc.b	116
	dc.b	32
	dc.b	114
	dc.b	101
	dc.b	97
	dc.b	100
	dc.b	32
	dc.b	65
	dc.b	109
	dc.b	105
	dc.b	110
	dc.b	101
	dc.b	116
	dc.b	32
	dc.b	114
	dc.b	101
	dc.b	115
	dc.b	112
	dc.b	111
	dc.b	110
	dc.b	115
	dc.b	101
	dc.b	59
	dc.b	32
	dc.b	116
	dc.b	114
	dc.b	121
	dc.b	32
	dc.b	97
	dc.b	110
	dc.b	111
	dc.b	116
	dc.b	104
	dc.b	101
	dc.b	114
	dc.b	32
	dc.b	99
	dc.b	97
	dc.b	116
	dc.b	101
	dc.b	103
	dc.b	111
	dc.b	114
	dc.b	121
	dc.b	32
	dc.b	111
	dc.b	114
	dc.b	32
	dc.b	115
	dc.b	101
	dc.b	97
	dc.b	114
	dc.b	99
	dc.b	104
	dc.b	46
	dc.b	0
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	cnop	0,4
l683
	sub.w	#580,a7
	movem.l	l745,-(a7)
	moveq	#4,d6
	moveq	#1,d5
	move.l	#2044+l388,d4
	tst.l	l388
	beq	l737
	move.l	l388,a0
	move.l	_SysBase,a6
	jsr	-372(a6)
	move.l	d0,a3
	tst.l	d0
	beq	l744
	move.l	#1313101638,a5
	move.l	#128,a4
l688
	cmp.l	(20,a3),a5
	bne	l691
	cmp.w	#1992,(18,a3)
	beq	l692
l691
	tst.l	(14,a3)
	beq	l703
	move.l	_SysBase,a6
	move.l	a3,a1
	jsr	-378(a6)
	bra	l703
l692
	cmp.l	(24,a3),d5
	bne	l697
	tst.l	2296+l388
	beq	l699
	moveq	#-1,d0
	move.l	d0,(24,a3)
	bra	l700
l699
	lea	(32,a3),a0
	lea	244+l388,a1
	inline
	move.l	a0,d0
.l1
	move.b	(a1)+,(a0)+
	bne	.l1
	einline
	lea	(1832,a3),a0
	lea	84+l388,a1
	inline
	move.l	a0,d0
.l1
	move.b	(a1)+,(a0)+
	bne	.l1
	einline
	move.l	d5,2296+l388
l700
	move.l	_SysBase,a6
	move.l	a3,a1
	jsr	-378(a6)
	bra	l703
l697
	moveq	#2,d0
	cmp.l	(24,a3),d0
	bne	l703
	move.l	2300+l388,a2
	pea	(1832,a3)
	pea	l704
	lea	(12+l747,a7),a0
	move.l	a0,-(a7)
	jsr	_sprintf
	add.w	#12,a7
	tst.l	(28,a3)
	beq	l706
	pea	(1832,a3)
	pea	l707
	lea	(12+l747,a7),a2
	move.l	a2,-(a7)
	jsr	_sprintf
	pea	l708
	move.l	a2,-(a7)
	jsr	_fopen
	move.l	d0,d3
	move.b	#0,(224+l747,a7)
	add.w	#20,a7
	tst.l	d3
	beq	l710
	move.l	d3,-(a7)
	move.l	a4,-(a7)
	lea	(212+l747,a7),a0
	move.l	a0,-(a7)
	jsr	_fgets
	add.w	#12,a7
	tst.l	d0
	bne	l712
	move.b	#0,(204+l747,a7)
l712
	move.l	d3,-(a7)
	jsr	_fclose
	addq.w	#4,a7
l710
	lea	(204+l747,a7),a2
	pea	l713
	move.l	a2,-(a7)
	jsr	_strcspn
	move.b	#0,(0,a2,d0.w)
	addq.w	#8,a7
	tst.b	(204+l747,a7)
	beq	l715
	lea	(204+l747,a7),a0
	bra	l716
l715
	lea	l717,a0
l716
	move.l	a0,-(a7)
	jsr	l419
	addq.w	#4,a7
	bra	l735
l706
	cmp.l	a2,d6
	bne	l720
	move.l	d4,-(a7)
	jsr	l400
	addq.w	#4,a7
	tst.l	d0
	beq	l722
	pea	l723
	jsr	l419
	addq.w	#4,a7
	bra	l735
l722
	lea	(4+l747,a7),a0
	move.l	_DOSBase,a6
	move.l	a0,d1
	move.l	#2044+l388,d2
	jsr	-78(a6)
	tst.l	d0
	beq	l726
	move.l	d4,-(a7)
	pea	l727
	lea	(340+l747,a7),a0
	move.l	a0,-(a7)
	jsr	_sprintf
	add.w	#12,a7
	tst.l	_IconBase
	beq	l729
	lea	(332+l747,a7),a0
	move.l	a0,-(a7)
	jsr	l400
	addq.w	#4,a7
	tst.l	d0
	bne	l729
	move.l	_IconBase,a6
	moveq	#4,d0
	jsr	-120(a6)
	move.l	d0,d2
	beq	l729
	move.l	_IconBase,a6
	lea	2044+l388,a0
	move.l	d2,a1
	jsr	-84(a6)
	move.l	_IconBase,a6
	move.l	d2,a0
	jsr	-90(a6)
l729
	move.l	d4,-(a7)
	pea	l732
	lea	(212+l747,a7),a2
	move.l	a2,-(a7)
	jsr	_sprintf
	move.l	a2,-(a7)
	jsr	l419
	add.w	#16,a7
	bra	l735
l726
	pea	l734
	jsr	l419
	addq.w	#4,a7
	bra	l735
l720
	move.l	a2,-(a7)
	lea	(8+l747,a7),a0
	move.l	a0,-(a7)
	jsr	l597
	addq.w	#8,a7
l735
	pea	(1832,a3)
	jsr	l292
	move.l	_SysBase,a6
	move.l	a3,a1
	jsr	-690(a6)
	jsr	l408
	addq.w	#4,a7
	bra	l744
l703
	move.l	l388,a0
	move.l	_SysBase,a6
	jsr	-372(a6)
	move.l	d0,a3
	tst.l	d0
	bne	l688
l744
	tst.l	l388
	beq	l737
	jsr	l278
	sub.l	2284+l388,d0
	move.l	d0,(0+l747,a7)
	cmp.l	2300+l388,d6
	bne	l740
	move.l	#330,d1
	bra	l741
l740
	moveq	#45,d1
l741
	moveq	#50,d0
	move.l	d2,-(a7)
	move.l	d0,d7
	move.l	d1,d2
	swap	d7
	swap	d2
	mulu.w	d1,d7
	mulu.w	d0,d2
	mulu.w	d1,d0
	add.w	d2,d7
	swap	d7
	clr.w	d7
	add.l	d7,d0
	move.l	(a7)+,d2
	cmp.l	(0+l747,a7),d0
	bcc	l737
	pea	l742
	jsr	l419
	jsr	l408
	addq.w	#4,a7
l737
l745	reg	a2/a3/a4/a5/a6/d2/d3/d4/d5/d6/d7
	movem.l	(a7)+,a2/a3/a4/a5/a6/d2/d3/d4/d5/d6/d7
l747	equ	44
	add.w	#580,a7
	rts
	cnop	0,4
l707
	dc.b	37
	dc.b	115
	dc.b	47
	dc.b	102
	dc.b	101
	dc.b	116
	dc.b	99
	dc.b	104
	dc.b	46
	dc.b	108
	dc.b	111
	dc.b	103
	dc.b	0
	cnop	0,4
l708
	dc.b	114
	dc.b	98
	dc.b	0
	cnop	0,4
l713
	dc.b	13
	dc.b	10
	dc.b	0
	cnop	0,4
l717
	dc.b	65
	dc.b	109
	dc.b	105
	dc.b	110
	dc.b	101
	dc.b	116
	dc.b	32
	dc.b	116
	dc.b	114
	dc.b	97
	dc.b	110
	dc.b	115
	dc.b	102
	dc.b	101
	dc.b	114
	dc.b	32
	dc.b	102
	dc.b	97
	dc.b	105
	dc.b	108
	dc.b	101
	dc.b	100
	dc.b	59
	dc.b	32
	dc.b	99
	dc.b	104
	dc.b	101
	dc.b	99
	dc.b	107
	dc.b	32
	dc.b	116
	dc.b	104
	dc.b	101
	dc.b	32
	dc.b	110
	dc.b	101
	dc.b	116
	dc.b	119
	dc.b	111
	dc.b	114
	dc.b	107
	dc.b	46
	dc.b	0
	cnop	0,4
l727
	dc.b	37
	dc.b	115
	dc.b	46
	dc.b	105
	dc.b	110
	dc.b	102
	dc.b	111
	dc.b	0
	cnop	0,4
l732
	dc.b	83
	dc.b	97
	dc.b	118
	dc.b	101
	dc.b	100
	dc.b	58
	dc.b	32
	dc.b	37
	dc.b	46
	dc.b	49
	dc.b	49
	dc.b	48
	dc.b	115
	dc.b	0
	cnop	0,4
l723
	dc.b	78
	dc.b	111
	dc.b	116
	dc.b	32
	dc.b	115
	dc.b	97
	dc.b	118
	dc.b	101
	dc.b	100
	dc.b	58
	dc.b	32
	dc.b	100
	dc.b	101
	dc.b	115
	dc.b	116
	dc.b	105
	dc.b	110
	dc.b	97
	dc.b	116
	dc.b	105
	dc.b	111
	dc.b	110
	dc.b	32
	dc.b	97
	dc.b	108
	dc.b	114
	dc.b	101
	dc.b	97
	dc.b	100
	dc.b	121
	dc.b	32
	dc.b	101
	dc.b	120
	dc.b	105
	dc.b	115
	dc.b	116
	dc.b	115
	dc.b	46
	dc.b	0
	cnop	0,4
l734
	dc.b	67
	dc.b	97
	dc.b	110
	dc.b	110
	dc.b	111
	dc.b	116
	dc.b	32
	dc.b	115
	dc.b	97
	dc.b	118
	dc.b	101
	dc.b	32
	dc.b	99
	dc.b	111
	dc.b	109
	dc.b	112
	dc.b	108
	dc.b	101
	dc.b	116
	dc.b	101
	dc.b	100
	dc.b	32
	dc.b	100
	dc.b	111
	dc.b	119
	dc.b	110
	dc.b	108
	dc.b	111
	dc.b	97
	dc.b	100
	dc.b	59
	dc.b	32
	dc.b	99
	dc.b	104
	dc.b	101
	dc.b	99
	dc.b	107
	dc.b	32
	dc.b	102
	dc.b	114
	dc.b	101
	dc.b	101
	dc.b	32
	dc.b	115
	dc.b	112
	dc.b	97
	dc.b	99
	dc.b	101
	dc.b	46
	dc.b	0
	cnop	0,4
l704
	dc.b	37
	dc.b	115
	dc.b	47
	dc.b	114
	dc.b	101
	dc.b	115
	dc.b	112
	dc.b	111
	dc.b	110
	dc.b	115
	dc.b	101
	dc.b	0
	cnop	0,4
l742
	dc.b	84
	dc.b	114
	dc.b	97
	dc.b	110
	dc.b	115
	dc.b	102
	dc.b	101
	dc.b	114
	dc.b	32
	dc.b	116
	dc.b	105
	dc.b	109
	dc.b	101
	dc.b	100
	dc.b	32
	dc.b	111
	dc.b	117
	dc.b	116
	dc.b	59
	dc.b	32
	dc.b	114
	dc.b	101
	dc.b	116
	dc.b	114
	dc.b	121
	dc.b	32
	dc.b	97
	dc.b	102
	dc.b	116
	dc.b	101
	dc.b	114
	dc.b	32
	dc.b	116
	dc.b	104
	dc.b	101
	dc.b	32
	dc.b	98
	dc.b	97
	dc.b	99
	dc.b	107
	dc.b	103
	dc.b	114
	dc.b	111
	dc.b	117
	dc.b	110
	dc.b	100
	dc.b	32
	dc.b	116
	dc.b	114
	dc.b	97
	dc.b	110
	dc.b	115
	dc.b	102
	dc.b	101
	dc.b	114
	dc.b	32
	dc.b	101
	dc.b	110
	dc.b	100
	dc.b	115
	dc.b	46
	dc.b	0
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	cnop	0,4
l748
	sub.w	#336,a7
	movem.l	l769,-(a7)
	tst.l	l388
	beq	l749
	moveq	#4,d0
	cmp.l	2300+l388,d0
	bne	l749
	tst.l	2296+l388
	beq	l749
	pea	84+l388
	pea	l755
	lea	(16+l771,a7),a2
	move.l	a2,-(a7)
	jsr	_sprintf
	pea	l756
	move.l	a2,-(a7)
	jsr	_fopen
	move.l	d0,d2
	add.w	#20,a7
	beq	l749
	lea	(0+l771,a7),a0
	move.l	a0,-(a7)
	lea	(8+l771,a7),a0
	move.l	a0,-(a7)
	move.l	d2,-(a7)
	jsr	l71
	add.w	#12,a7
	tst.l	d0
	beq	l760
	tst.l	(0+l771,a7)
	beq	l760
	move.l	(4+l771,a7),d0
	cmp.l	2288+l388,d0
	bne	l759
	move.l	(0+l771,a7),d0
	cmp.l	2292+l388,d0
	beq	l760
l759
	move.l	(4+l771,a7),2288+l388
	move.l	(0+l771,a7),2292+l388
	move.l	(0+l771,a7),-(a7)
	move.l	(8+l771,a7),-(a7)
	move.l	#1,-(a7)
	jsr	l88
	add.w	#12,a7
	cmp.l	#1024,(0+l771,a7)
	bcc	l765
	move.l	(0+l771,a7),-(a7)
	move.l	(8+l771,a7),-(a7)
	move.l	(8+l771,a7),-(a7)
	move.l	(16+l771,a7),-(a7)
	jsr	_nd_aminet_percent
	addq.w	#8,a7
	move.l	d0,-(a7)
	pea	l766
	lea	(224+l771,a7),a0
	move.l	a0,-(a7)
	jsr	_sprintf
	add.w	#20,a7
	bra	l767
l765
	move.l	#1023,d0
	and.l	(0+l771,a7),d0
	moveq	#10,d1
	move.l	d0,d3
	move.l	d1,d4
	swap	d3
	swap	d4
	mulu.w	d1,d3
	mulu.w	d0,d4
	mulu.w	d1,d0
	add.w	d4,d3
	swap	d3
	clr.w	d3
	add.l	d3,d0
	moveq	#10,d1
	lsr.l	d1,d0
	move.l	d0,-(a7)
	moveq	#10,d1
	move.l	(4+l771,a7),d0
	lsr.l	d1,d0
	move.l	d0,-(a7)
	move.l	#1023,d0
	and.l	(12+l771,a7),d0
	moveq	#10,d1
	move.l	d0,d3
	move.l	d1,d4
	swap	d3
	swap	d4
	mulu.w	d1,d3
	mulu.w	d0,d4
	mulu.w	d1,d0
	add.w	d4,d3
	swap	d3
	clr.w	d3
	add.l	d3,d0
	moveq	#10,d1
	lsr.l	d1,d0
	move.l	d0,-(a7)
	moveq	#10,d1
	move.l	(16+l771,a7),d0
	lsr.l	d1,d0
	move.l	d0,-(a7)
	move.l	(16+l771,a7),-(a7)
	move.l	(24+l771,a7),-(a7)
	jsr	_nd_aminet_percent
	addq.w	#8,a7
	move.l	d0,-(a7)
	pea	l768
	lea	(232+l771,a7),a0
	move.l	a0,-(a7)
	jsr	_sprintf
	add.w	#28,a7
l767
	lea	(208+l771,a7),a0
	move.l	a0,-(a7)
	jsr	l419
	addq.w	#4,a7
l760
	move.l	d2,-(a7)
	jsr	_fclose
	addq.w	#4,a7
l749
l769	reg	a2/d2/d3/d4
	movem.l	(a7)+,a2/d2/d3/d4
l771	equ	16
	add.w	#336,a7
	rts
	cnop	0,4
l766
	dc.b	68
	dc.b	111
	dc.b	119
	dc.b	110
	dc.b	108
	dc.b	111
	dc.b	97
	dc.b	100
	dc.b	105
	dc.b	110
	dc.b	103
	dc.b	58
	dc.b	32
	dc.b	37
	dc.b	100
	dc.b	37
	dc.b	37
	dc.b	32
	dc.b	45
	dc.b	32
	dc.b	37
	dc.b	108
	dc.b	117
	dc.b	32
	dc.b	47
	dc.b	32
	dc.b	37
	dc.b	108
	dc.b	117
	dc.b	32
	dc.b	98
	dc.b	121
	dc.b	116
	dc.b	101
	dc.b	115
	dc.b	0
	cnop	0,4
l768
	dc.b	68
	dc.b	111
	dc.b	119
	dc.b	110
	dc.b	108
	dc.b	111
	dc.b	97
	dc.b	100
	dc.b	105
	dc.b	110
	dc.b	103
	dc.b	58
	dc.b	32
	dc.b	37
	dc.b	100
	dc.b	37
	dc.b	37
	dc.b	32
	dc.b	45
	dc.b	32
	dc.b	37
	dc.b	108
	dc.b	117
	dc.b	46
	dc.b	37
	dc.b	108
	dc.b	117
	dc.b	32
	dc.b	47
	dc.b	32
	dc.b	37
	dc.b	108
	dc.b	117
	dc.b	46
	dc.b	37
	dc.b	108
	dc.b	117
	dc.b	32
	dc.b	75
	dc.b	66
	dc.b	0
	cnop	0,4
l755
	dc.b	37
	dc.b	115
	dc.b	47
	dc.b	112
	dc.b	114
	dc.b	111
	dc.b	103
	dc.b	114
	dc.b	101
	dc.b	115
	dc.b	115
	dc.b	0
	cnop	0,4
l756
	dc.b	114
	dc.b	0
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	cnop	0,4
l773
	sub.w	#64,a7
	movem.l	l796,-(a7)
	move.l	(68+l798,a7),a3
	move.l	(72+l798,a7),a2
	jsr	l278
	move.l	d0,d5
	tst.b	l772
	beq	l793
	tst.l	l779
	beq	l783
	cmp.l	l777,a3
	bne	l783
	cmp.l	l778,a2
	beq	l793
l783
	tst.l	l779
	beq	l787
	cmp.l	l778,a2
	bne	l787
	cmp.l	a3,a2
	beq	l787
	move.l	d5,d0
	sub.l	l776,d0
	moveq	#25,d1
	cmp.l	d0,d1
	bhi	l793
l787
	move.l	a2,-(a7)
	move.l	a3,-(a7)
	pea	l791
	lea	(12+l798,a7),a0
	move.l	a0,-(a7)
	jsr	_sprintf
	move.l	d0,a4
	move.l	_DOSBase,a6
	move.l	#l772,d1
	move.l	#1006,d2
	jsr	-30(a6)
	move.l	d0,d4
	add.w	#16,a7
	beq	l793
	lea	(0+l798,a7),a0
	move.l	_DOSBase,a6
	move.l	d4,d1
	move.l	a0,d2
	move.l	a4,d3
	jsr	-48(a6)
	cmp.l	d0,a4
	bne	l795
	move.l	d5,l776
	move.l	a3,l777
	move.l	a2,l778
	moveq	#1,d0
	move.l	d0,l779
l795
	move.l	_DOSBase,a6
	move.l	d4,d1
	jsr	-36(a6)
l793
	moveq	#1,d0
l796	reg	a2/a3/a4/a6/d2/d3/d4/d5
	movem.l	(a7)+,a2/a3/a4/a6/d2/d3/d4/d5
l798	equ	32
	add.w	#64,a7
	rts
	cnop	0,4
l791
	dc.b	37
	dc.b	108
	dc.b	117
	dc.b	32
	dc.b	37
	dc.b	108
	dc.b	117
	dc.b	10
	dc.b	0
	section	"BSS",bss
	cnop	0,4
l776
	ds.b	4
	cnop	0,4
l777
	ds.b	4
	cnop	0,4
l778
	ds.b	4
	cnop	0,4
l779
	ds.b	4
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	section	"CODE",code
	cnop	0,4
l799
	sub.w	#640,a7
	movem.l	l869,-(a7)
	move.l	#89134,d4
	moveq	#2,d3
	moveq	#1,d2
	move.l	(644+l871,a7),d0
	moveq	#33,d1
	cmp.l	d0,d1
	bne	l803
	jsr	l144
	bra	l867
l803
	tst.l	l87
	beq	l867
	tst.l	l388
	beq	l807
	tst.l	d0
	beq	l867
	pea	l810
	jsr	l96
	addq.w	#4,a7
	bra	l867
l807
	moveq	#3,d1
	cmp.l	d0,d1
	bne	l812
	move.l	#0,l398
	move.l	d2,l399
	jsr	l565
	bra	l867
l812
	moveq	#4,d1
	cmp.l	d0,d1
	bne	l815
	jsr	l514
	bra	l867
l815
	moveq	#5,d1
	cmp.l	d0,d1
	bne	l818
	jsr	l499
	bra	l867
l818
	cmp.l	d0,d3
	bne	l821
	move.l	l87,a1
	move.l	(16,a1),a1
	move.l	(34,a1),a1
	move.l	(a1),a1
	lea	l395,a0
	moveq	#80,d1
	inline
	move.l	a0,d0
	bra	.l2
.l1
	move.b	(a1),(a0)+
	beq	.l2
	addq.l	#1,a1
.l2
	subq.l	#1,d1
	bpl	.l1
	einline
	move.b	#0,80+l395
	move.l	d3,l398
	move.l	d2,l399
	jsr	l565
	bra	l867
l821
	moveq	#20,d1
	cmp.l	d0,d1
	beq	l823
	moveq	#21,d1
	cmp.l	d0,d1
	bne	l824
l823
	tst.l	l397
	beq	l827
	pea	l828
	jsr	l419
	addq.w	#4,a7
	bra	l867
l827
	moveq	#20,d1
	cmp.l	d0,d1
	bne	l830
	cmp.l	l399,d2
	beq	l867
	subq.l	#1,l399
	bra	l833
l830
	moveq	#50,d1
	cmp.l	10900+l389,d1
	bgt	l867
	cmp.l	#1000,l399
	bge	l867
	addq.l	#1,l399
l833
	jsr	l565
	bra	l867
l824
	moveq	#10,d1
	cmp.l	d0,d1
	bne	l839
	move.l	l87,a0
	move.l	(0,a0,d4.l),a2
	tst.l	l397
	bne	l867
	move.l	a2,d1
	blt	l867
	cmp.l	10900+l389,a2
	bge	l867
	move.w	a2,d1
	muls.w	#218,d1
	lea	l389,a1
	add.w	d1,a1
	lea	l396,a0
	inline
	move.l	a0,d0
.l1
	move.b	(a1)+,(a0)+
	bne	.l1
	einline
	lea	97+l389,a0
	pea	(0,a0,d1.w)
	jsr	l419
	addq.w	#4,a7
	bra	l867
l839
	moveq	#30,d1
	cmp.l	d0,d1
	bne	l846
	move.l	l87,a0
	move.l	(0,a0,d4.l),a2
	cmp.l	l397,d2
	beq	l850
	cmp.l	l397,d3
	bne	l848
l850
	move.l	a2,d1
	blt	l848
	cmp.l	10900+l390,a2
	bge	l848
	move.w	a2,d0
	muls.w	#218,d0
	lea	l390,a1
	add.w	d0,a1
	lea	l394,a0
	inline
	move.l	a0,d0
.l1
	move.b	(a1)+,(a0)+
	bne	.l1
	einline
	move.l	d2,l399
	move.l	#47,-(a7)
	pea	l394
	jsr	_strchr
	addq.w	#8,a7
	tst.l	d0
	beq	l853
	move.l	d2,l398
	jsr	l565
	bra	l867
l853
	pea	l394
	pea	l855
	lea	(8+l871,a7),a2
	move.l	a2,-(a7)
	jsr	_sprintf
	move.l	#0,-(a7)
	move.l	#5,-(a7)
	move.l	a2,-(a7)
	jsr	l425
	add.w	#24,a7
	bra	l867
l848
	move.l	#0,-(a7)
	jsr	l583
	addq.w	#4,a7
	bra	l867
l846
	moveq	#31,d1
	cmp.l	d0,d1
	bne	l859
	move.l	d2,-(a7)
	jsr	l583
	addq.w	#4,a7
	bra	l867
l859
	moveq	#32,d1
	cmp.l	d0,d1
	bne	l867
	tst.b	l396
	bne	l864
	pea	l865
	jsr	l419
	addq.w	#4,a7
	bra	l867
l864
	move.l	#240,-(a7)
	lea	(404+l871,a7),a0
	move.l	a0,-(a7)
	move.l	_DOSBase,a6
	move.l	#l396,d1
	jsr	-870(a6)
	move.l	d0,-(a7)
	pea	l392
	jsr	l262
	add.w	#16,a7
	tst.l	d0
	beq	l867
	pea	l396
	pea	l868
	lea	(8+l871,a7),a2
	move.l	a2,-(a7)
	jsr	_sprintf
	lea	(412+l871,a7),a0
	move.l	a0,-(a7)
	move.l	#4,-(a7)
	move.l	a2,-(a7)
	jsr	l425
	add.w	#24,a7
l867
l869	reg	a2/a6/d2/d3/d4
	movem.l	(a7)+,a2/a6/d2/d3/d4
l871	equ	20
	add.w	#640,a7
	rts
	cnop	0,4
l810
	dc.b	84
	dc.b	114
	dc.b	97
	dc.b	110
	dc.b	115
	dc.b	102
	dc.b	101
	dc.b	114
	dc.b	32
	dc.b	105
	dc.b	110
	dc.b	32
	dc.b	112
	dc.b	114
	dc.b	111
	dc.b	103
	dc.b	114
	dc.b	101
	dc.b	115
	dc.b	115
	dc.b	46
	dc.b	32
	dc.b	89
	dc.b	111
	dc.b	117
	dc.b	32
	dc.b	99
	dc.b	97
	dc.b	110
	dc.b	32
	dc.b	99
	dc.b	108
	dc.b	111
	dc.b	115
	dc.b	101
	dc.b	32
	dc.b	116
	dc.b	104
	dc.b	105
	dc.b	115
	dc.b	32
	dc.b	119
	dc.b	105
	dc.b	110
	dc.b	100
	dc.b	111
	dc.b	119
	dc.b	46
	dc.b	0
	cnop	0,4
l828
	dc.b	66
	dc.b	97
	dc.b	99
	dc.b	107
	dc.b	32
	dc.b	114
	dc.b	101
	dc.b	116
	dc.b	117
	dc.b	114
	dc.b	110
	dc.b	115
	dc.b	32
	dc.b	116
	dc.b	111
	dc.b	32
	dc.b	112
	dc.b	97
	dc.b	99
	dc.b	107
	dc.b	97
	dc.b	103
	dc.b	101
	dc.b	115
	dc.b	46
	dc.b	0
	cnop	0,4
l855
	dc.b	47
	dc.b	116
	dc.b	114
	dc.b	101
	dc.b	101
	dc.b	63
	dc.b	112
	dc.b	97
	dc.b	116
	dc.b	104
	dc.b	61
	dc.b	37
	dc.b	115
	dc.b	0
	cnop	0,4
l865
	dc.b	83
	dc.b	101
	dc.b	108
	dc.b	101
	dc.b	99
	dc.b	116
	dc.b	32
	dc.b	97
	dc.b	32
	dc.b	112
	dc.b	97
	dc.b	99
	dc.b	107
	dc.b	97
	dc.b	103
	dc.b	101
	dc.b	32
	dc.b	102
	dc.b	105
	dc.b	114
	dc.b	115
	dc.b	116
	dc.b	46
	dc.b	0
	cnop	0,4
l868
	dc.b	47
	dc.b	37
	dc.b	115
	dc.b	0
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	public	_main
	cnop	0,4
_main
	sub.w	#492,a7
	movem.l	l979,-(a7)
	move.l	#4,a5
	move.l	(500+l981,a7),a3
	move.l	#0,a4
	moveq	#20,d7
	move.l	d7,(0+l981,a7)
	moveq	#1,d4
	moveq	#5,d7
	cmp.l	(496+l981,a7),d7
	bne	l875
	move.l	(4,a3),a0
	lea	l877,a1
	inline
	moveq	#0,d0
	moveq	#0,d1
.l1
	move.b	(a0)+,d0
	move.b	(a1)+,d1
	beq	.l2
	sub.l	d1,d0
	beq	.l1
	moveq	#0,d1
.l2
	sub.l	d1,d0
	einline
	tst.l	d0
	bne	l875
	move.l	(16,a3),a0
	lea	l881,a1
	inline
	moveq	#0,d0
	moveq	#0,d1
.l1
	move.b	(a0)+,d0
	move.b	(a1)+,d1
	beq	.l2
	sub.l	d1,d0
	beq	.l1
	moveq	#0,d1
.l2
	sub.l	d1,d0
	einline
	tst.l	d0
	bne	l878
	move.l	#1,a0
	bra	l879
l878
	move.l	#0,a0
l879
	move.l	a0,(0+l981,a7)
	move.l	a0,d7
	beq	l883
	move.l	(12,a3),a0
	inline
	move.l	a0,d0
.l1
	tst.b	(a0)+
	bne	.l1
	sub.l	a0,d0
	not.l	d0
	einline
	add.l	#10,d0
	cmp.l	#200,d0
	bcc	l883
	move.l	(12,a3),a1
	lea	l772,a0
	inline
	move.l	a0,d0
.l1
	move.b	(a1)+,(a0)+
	bne	.l1
	einline
	move.l	_DOSBase,a6
	move.l	#l772,d1
	jsr	-870(a6)
	move.l	d0,a0
	move.b	#0,(a0)
	move.l	_DOSBase,a6
	move.l	#l772,d1
	move.l	#l885,d2
	move.l	#200,d3
	jsr	-882(a6)
l883
	move.l	#128,-(a7)
	lea	(336+l981,a7),a0
	move.l	a0,-(a7)
	tst.l	(8+l981,a7)
	beq	l889
	move.l	#l773,(12+l981,a7)
	bra	l890
l889
	move.l	#0,(12+l981,a7)
l890
	move.l	(12+l981,a7),-(a7)
	move.l	(12+l981,a7),-(a7)
	tst.l	(16+l981,a7)
	beq	l892
	move.l	#300,(20+l981,a7)
	bra	l893
l892
	moveq	#30,d7
	move.l	d7,(20+l981,a7)
l893
	move.l	(20+l981,a7),-(a7)
	tst.l	(20+l981,a7)
	beq	l895
	move.l	#67108864,(20+l981,a7)
	bra	l896
l895
	move.l	#262144,(20+l981,a7)
l896
	move.l	(20+l981,a7),-(a7)
	move.l	(12,a3),-(a7)
	move.l	(8,a3),-(a7)
	jsr	_nd_aminet_fetch
	add.w	#32,a7
	tst.l	d0
	beq	l887
	moveq	#0,d0
	bra	l872
l887
	lea	(332+l981,a7),a0
	move.l	a0,-(a7)
	pea	l897
	jsr	_printf
	moveq	#20,d0
	addq.w	#8,a7
	bra	l872
l875
	moveq	#3,d7
	cmp.l	(496+l981,a7),d7
	bne	l899
	move.l	(4,a3),a0
	lea	l901,a1
	inline
	moveq	#0,d0
	moveq	#0,d1
.l1
	move.b	(a0)+,d0
	move.b	(a1)+,d1
	beq	.l2
	sub.l	d1,d0
	beq	.l1
	moveq	#0,d1
.l2
	sub.l	d1,d0
	einline
	tst.l	d0
	bne	l899
	move.l	(8,a3),-(a7)
	jsr	l325
	addq.w	#4,a7
	bra	l872
l899
	moveq	#2,d7
	cmp.l	(496+l981,a7),d7
	ble	l903
	moveq	#20,d0
	bra	l872
l903
	move.l	_SysBase,a6
	move.l	#0,a1
	jsr	-294(a6)
	move.l	d0,a1
	move.l	#-1,(184,a1)
	move.l	_DOSBase,a6
	jsr	-600(a6)
	move.l	d0,(4+l981,a7)
	beq	l904
	move.l	_DOSBase,a6
	move.l	(4+l981,a7),d1
	move.l	#l393,d2
	move.l	#240,d3
	jsr	-402(a6)
	tst.l	d0
	beq	l904
	move.l	_DOSBase,a6
	move.l	#l393,d1
	move.l	#l908,d2
	move.l	#240,d3
	jsr	-882(a6)
	tst.w	d0
	bne	l905
l904
	moveq	#20,d0
	bra	l872
l905
	move.l	_SysBase,a6
	lea	l909,a1
	moveq	#39,d0
	jsr	-552(a6)
	move.l	d0,_IntuitionBase
	move.l	_SysBase,a6
	lea	l910,a1
	moveq	#39,d0
	jsr	-552(a6)
	move.l	d0,_GfxBase
	move.l	_SysBase,a6
	lea	l911,a1
	moveq	#39,d0
	jsr	-552(a6)
	move.l	d0,_GadToolsBase
	move.l	_SysBase,a6
	lea	l912,a1
	moveq	#39,d0
	jsr	-552(a6)
	move.l	d0,_AslBase
	move.l	_SysBase,a6
	lea	l913,a1
	moveq	#39,d0
	jsr	-552(a6)
	move.l	d0,_IconBase
	tst.l	_IntuitionBase
	beq	l920
	tst.l	_GfxBase
	beq	l920
	tst.l	_GadToolsBase
	beq	l920
	tst.l	_AslBase
	beq	l920
	move.l	(4,a3),-(a7)
	lea	(12+l981,a7),a0
	move.l	a0,-(a7)
	jsr	_nd_client_init
	addq.w	#8,a7
	tst.l	d0
	beq	l920
l977
	tst.l	d4
	beq	l926
	move.l	#96,-(a7)
	pea	l392
	pea	l927
	pea	l928
	move.l	(4,a3),-(a7)
	jsr	_nd_plugin_option_string
	moveq	#0,d4
	add.w	#20,a7
l926
	jsr	l683
	jsr	l748
	tst.l	l388
	beq	l930
	cmp.l	2300+l388,a5
	beq	l930
	move.l	#l933,d5
	bra	l931
l930
	move.l	#l391,d5
l931
	move.l	d5,-(a7)
	pea	l934
	lea	(468+l981,a7),a2
	move.l	a2,-(a7)
	jsr	_sprintf
	move.l	#500,-(a7)
	move.l	a4,-(a7)
	move.l	#65535,-(a7)
	move.l	a2,-(a7)
	pea	l935
	lea	(40+l981,a7),a0
	move.l	a0,-(a7)
	jsr	_nd_client_update_quiet
	move.l	d0,d6
	move.b	d6,d0
	and.b	#2,d0
	add.w	#36,a7
	bne	l978
	move.b	d6,d0
	and.b	#4,d0
	beq	l939
	moveq	#1,d4
l939
	move.b	d6,d0
	and.b	#1,d0
	beq	l946
	tst.l	l87
	bne	l942
	moveq	#1,d2
	bra	l943
l942
	moveq	#0,d2
l943
	move.l	d2,a2
	jsr	l162
	tst.l	d0
	beq	l946
	move.l	a2,d7
	beq	l953
	tst.l	10900+l389
	beq	l950
	jsr	l499
	bra	l953
l950
	tst.l	l388
	bne	l953
	move.l	#0,l398
	moveq	#1,d7
	move.l	d7,l399
	jsr	l565
l953
	move.l	2292+l388,-(a7)
	move.l	2288+l388,-(a7)
	tst.l	l388
	beq	l956
	cmp.l	2300+l388,a5
	bne	l956
	moveq	#1,d0
	bra	l957
l956
	moveq	#0,d0
l957
	move.l	d0,-(a7)
	jsr	l88
	pea	l391
	jsr	l96
	add.w	#16,a7
l946
	jsr	l234
	move.l	d0,-(a7)
	jsr	l799
	addq.w	#4,a7
	tst.l	l87
	beq	l959
	move.l	l87,a0
	move.l	(4,a0),a0
	move.l	(86,a0),a0
	move.b	(15,a0),d0
	moveq	#1,d1
	lsl.l	d0,d1
	bra	l960
l959
	moveq	#0,d1
l960
	tst.l	l388
	beq	l962
	move.l	l388,a0
	move.b	(15,a0),d0
	moveq	#1,d3
	lsl.l	d0,d3
	bra	l963
l962
	moveq	#0,d3
l963
	move.l	d3,d0
	or.l	d1,d0
	move.l	d0,-(a7)
	tst.l	l388
	beq	l965
	move.l	#25,a1
	bra	l966
l965
	move.l	#500,a1
l966
	move.l	a1,-(a7)
	lea	(16+l981,a7),a0
	move.l	a0,-(a7)
	jsr	_nd_client_wait_signals
	move.l	d0,a4
	add.w	#12,a7
	bra	l977
l978
	jsr	l408
	lea	(8+l981,a7),a0
	move.l	a0,-(a7)
	jsr	_nd_client_close
	move.l	#0,(4+l981,a7)
	addq.w	#4,a7
l920
	jsr	l144
	tst.l	_IconBase
	beq	l968
	move.l	_IconBase,a1
	move.l	_SysBase,a6
	jsr	-414(a6)
l968
	tst.l	_AslBase
	beq	l970
	move.l	_AslBase,a1
	move.l	_SysBase,a6
	jsr	-414(a6)
l970
	tst.l	_GadToolsBase
	beq	l972
	move.l	_GadToolsBase,a1
	move.l	_SysBase,a6
	jsr	-414(a6)
l972
	tst.l	_GfxBase
	beq	l974
	move.l	_GfxBase,a1
	move.l	_SysBase,a6
	jsr	-414(a6)
l974
	tst.l	_IntuitionBase
	beq	l976
	move.l	_IntuitionBase,a1
	move.l	_SysBase,a6
	jsr	-414(a6)
l976
	move.l	(0+l981,a7),d0
l872
l979	reg	a2/a3/a4/a5/a6/d2/d3/d4/d5/d6/d7
	movem.l	(a7)+,a2/a3/a4/a5/a6/d2/d3/d4/d5/d6/d7
l981	equ	44
	add.w	#492,a7
	rts
	cnop	0,4
l885
	dc.b	112
	dc.b	114
	dc.b	111
	dc.b	103
	dc.b	114
	dc.b	101
	dc.b	115
	dc.b	115
	dc.b	0
	cnop	0,4
l881
	dc.b	49
	dc.b	0
	cnop	0,4
l897
	dc.b	37
	dc.b	115
	dc.b	10
	dc.b	0
	cnop	0,4
l927
	dc.b	82
	dc.b	65
	dc.b	77
	dc.b	58
	dc.b	0
	cnop	0,4
l928
	dc.b	68
	dc.b	82
	dc.b	65
	dc.b	87
	dc.b	69
	dc.b	82
	dc.b	0
	cnop	0,4
l933
	dc.b	84
	dc.b	114
	dc.b	97
	dc.b	110
	dc.b	115
	dc.b	102
	dc.b	101
	dc.b	114
	dc.b	114
	dc.b	105
	dc.b	110
	dc.b	103
	dc.b	46
	dc.b	46
	dc.b	46
	dc.b	0
	cnop	0,4
l934
	dc.b	37
	dc.b	46
	dc.b	51
	dc.b	49
	dc.b	115
	dc.b	0
	cnop	0,4
l935
	dc.b	65
	dc.b	77
	dc.b	73
	dc.b	78
	dc.b	69
	dc.b	84
	dc.b	0
	cnop	0,4
l877
	dc.b	70
	dc.b	69
	dc.b	84
	dc.b	67
	dc.b	72
	dc.b	0
	cnop	0,4
l901
	dc.b	87
	dc.b	79
	dc.b	82
	dc.b	75
	dc.b	0
	cnop	0,4
l908
	dc.b	65
	dc.b	109
	dc.b	105
	dc.b	110
	dc.b	101
	dc.b	116
	dc.b	0
	cnop	0,4
l909
	dc.b	105
	dc.b	110
	dc.b	116
	dc.b	117
	dc.b	105
	dc.b	116
	dc.b	105
	dc.b	111
	dc.b	110
	dc.b	46
	dc.b	108
	dc.b	105
	dc.b	98
	dc.b	114
	dc.b	97
	dc.b	114
	dc.b	121
	dc.b	0
	cnop	0,4
l910
	dc.b	103
	dc.b	114
	dc.b	97
	dc.b	112
	dc.b	104
	dc.b	105
	dc.b	99
	dc.b	115
	dc.b	46
	dc.b	108
	dc.b	105
	dc.b	98
	dc.b	114
	dc.b	97
	dc.b	114
	dc.b	121
	dc.b	0
	cnop	0,4
l911
	dc.b	103
	dc.b	97
	dc.b	100
	dc.b	116
	dc.b	111
	dc.b	111
	dc.b	108
	dc.b	115
	dc.b	46
	dc.b	108
	dc.b	105
	dc.b	98
	dc.b	114
	dc.b	97
	dc.b	114
	dc.b	121
	dc.b	0
	cnop	0,4
l912
	dc.b	97
	dc.b	115
	dc.b	108
	dc.b	46
	dc.b	108
	dc.b	105
	dc.b	98
	dc.b	114
	dc.b	97
	dc.b	114
	dc.b	121
	dc.b	0
	cnop	0,4
l913
	dc.b	105
	dc.b	99
	dc.b	111
	dc.b	110
	dc.b	46
	dc.b	108
	dc.b	105
	dc.b	98
	dc.b	114
	dc.b	97
	dc.b	114
	dc.b	121
	dc.b	0
	public	_nd_version
	cnop	0,4
_nd_version
	dc.b	36
	dc.b	86
	dc.b	69
	dc.b	82
	dc.b	58
	dc.b	32
	dc.b	65
	dc.b	109
	dc.b	105
	dc.b	110
	dc.b	101
	dc.b	116
	dc.b	32
	dc.b	48
	dc.b	46
	dc.b	49
	dc.b	32
	dc.b	40
	dc.b	49
	dc.b	54
	dc.b	46
	dc.b	49
	dc.b	49
	dc.b	46
	dc.b	50
	dc.b	48
	dc.b	50
	dc.b	54
	dc.b	41
	dc.b	0
	public	_IconBase
	section	"DATA",data
	cnop	0,4
_IconBase
	dc.l	0
	public	_IntuitionBase
	cnop	0,4
_IntuitionBase
	dc.l	0
	public	_GfxBase
	cnop	0,4
_GfxBase
	dc.l	0
	public	_GadToolsBase
	cnop	0,4
_GadToolsBase
	dc.l	0
	public	_AslBase
	cnop	0,4
_AslBase
	dc.l	0
	public	___stack
	cnop	0,4
___stack
	dc.l	32768
	public	__divs
	public	__divu
	public	_SysBase
	public	_DOSBase
	public	_NewList
	public	_fopen
	public	_fclose
	public	_printf
	public	_sprintf
	public	_sscanf
	public	_fgets
	public	_strchr
	public	_strcspn
	public	_nd_client_init
	public	_nd_client_update_quiet
	public	_nd_client_wait_signals
	public	_nd_client_close
	public	_nd_plugin_option_string
	public	_nd_quote_path
	public	_nd_aminet_encode_query
	public	_nd_aminet_readme_path
	public	_nd_aminet_search
	public	_nd_aminet_details
	public	_nd_aminet_percent
	public	_nd_aminet_categories
	public	_nd_aminet_fetch
	cnop	0,4
l391
	dc.b	67
	dc.b	108
	dc.b	105
	dc.b	99
	dc.b	107
	dc.b	32
	dc.b	116
	dc.b	111
	dc.b	32
	dc.b	98
	dc.b	114
	dc.b	111
	dc.b	119
	dc.b	115
	dc.b	101
	dc.b	32
	dc.b	65
	dc.b	109
	dc.b	105
	dc.b	110
	dc.b	101
	dc.b	116
	dc.b	0
	ds.b	105
	cnop	0,4
l392
	dc.b	82
	dc.b	65
	dc.b	77
	dc.b	58
	dc.b	0
	ds.b	91
	cnop	0,4
l399
	dc.l	1
	section	"BSS",bss
	cnop	0,4
l87
	ds.b	4
	cnop	0,4
l388
	ds.b	2304
	cnop	0,4
l389
	ds.b	10908
	cnop	0,4
l390
	ds.b	10908
	cnop	0,4
l393
	ds.b	240
	cnop	0,4
l394
	ds.b	48
	cnop	0,4
l395
	ds.b	81
	cnop	0,4
l396
	ds.b	97
	cnop	0,4
l397
	ds.b	4
	cnop	0,4
l398
	ds.b	4
	cnop	0,4
l772
	ds.b	200

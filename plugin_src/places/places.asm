	idnt	"plugin_src/places/places.c"
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	section	"CODE",code
	cnop	0,4
l1
	movem.l	l18,-(a7)
	moveq	#127,d3
	moveq	#32,d2
	move.l	(4+l20,a7),a2
	move.l	a2,d0
	beq	l4
	tst.b	(a2)
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
	cmp.l	#240,d0
	bcc	l4
	move.l	#58,-(a7)
	move.l	a2,-(a7)
	jsr	_strchr
	addq.w	#8,a7
	tst.l	d0
	bne	l5
l4
	moveq	#0,d0
	bra	l2
l5
	move.l	a2,a0
	tst.b	(a2)
	beq	l17
	moveq	#0,d0
l9
	cmp.b	(a0),d2
	bhi	l13
	cmp.b	(a0),d3
	bne	l14
l13
	bra	l2
l14
	addq.l	#1,a0
	tst.b	(a0)
	bne	l9
l17
	moveq	#1,d0
l2
l18	reg	a2/d2/d3
	movem.l	(a7)+,a2/d2/d3
l20	equ	12
	rts
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	cnop	0,4
l21
	movem.l	l48,-(a7)
	moveq	#47,d3
	move.l	(8+l50,a7),a3
	move.l	(4+l50,a7),a2
	move.l	a2,a0
	inline
	move.l	a0,d0
.l1
	tst.b	(a0)+
	bne	.l1
	sub.l	a0,d0
	not.l	d0
	einline
	move.l	d0,d1
	move.l	a3,a0
	inline
	move.l	a0,d0
.l1
	tst.b	(a0)+
	bne	.l1
	sub.l	a0,d0
	not.l	d0
	einline
	tst.l	d1
	beq	l45
	cmp.b	(-1,a2,d1.l),d3
	bne	l45
l42
	subq.l	#1,d1
	beq	l45
	cmp.b	(-1,a2,d1.l),d3
	beq	l42
l45
	tst.l	d0
	beq	l46
	cmp.b	(-1,a3,d0.l),d3
	bne	l46
l43
	subq.l	#1,d0
	beq	l46
	cmp.b	(-1,a3,d0.l),d3
	beq	l43
l46
	cmp.l	d1,d0
	beq	l35
	moveq	#0,d0
	bra	l22
l35
	moveq	#0,d2
	tst.l	d1
	beq	l47
l44
	lea	(a2,d2.l),a1
	moveq	#0,d0
	move.b	(a1),d0
	inline
	xref	___ctype
	lea	___ctype,a0
	btst	#0,(1,a0,d0.w)
	beq	.skip
	or.b	#$20,d0
.skip
	einline
	move.l	d0,a1
	lea	(a3,d2.l),a0
	moveq	#0,d0
	move.b	(a0),d0
	inline
	xref	___ctype
	lea	___ctype,a0
	btst	#0,(1,a0,d0.w)
	beq	.skip
	or.b	#$20,d0
.skip
	einline
	cmp.l	a1,d0
	beq	l41
	moveq	#0,d0
	bra	l22
l41
	addq.l	#1,d2
	cmp.l	d2,d1
	bhi	l44
l47
	moveq	#1,d0
l22
l48	reg	a2/a3/d2/d3
	movem.l	(a7)+,a2/a3/d2/d3
l50	equ	16
	rts
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	cnop	0,4
l51
	movem.l	l73,-(a7)
	move.l	(8+l75,a7),a5
	move.l	(12+l75,a7),a4
	move.l	(4+l75,a7),a3
	move.l	a3,d0
	beq	l54
	move.l	a4,-(a7)
	jsr	l1
	addq.w	#4,a7
	tst.l	d0
	beq	l54
	moveq	#32,d0
	cmp.l	(9216,a3),d0
	bgt	l55
l54
	moveq	#0,d0
	bra	l52
l55
	moveq	#0,d2
	tst.l	(9216,a3)
	ble	l72
l71
	move.l	a4,-(a7)
	move.w	d2,d0
	muls.w	#288,d0
	pea	(48,a3,d0.w)
	jsr	l21
	addq.w	#8,a7
	tst.l	d0
	beq	l63
	moveq	#0,d0
	bra	l52
l63
	addq.l	#1,d2
	cmp.l	(9216,a3),d2
	blt	l71
l72
	move.l	a5,d0
	beq	l64
	tst.b	(a5)
	bne	l65
l64
	move.l	a4,a0
	moveq	#47,d1
	inline
	moveq	#0,d0
.l1
	cmp.b	(a0),d1
	bne	.l2
	move.l	a0,d0
.l2
	tst.b	(a0)+
	bne	.l1
	einline
	move.l	d0,a1
	tst.l	d0
	beq	l68
	tst.b	(1,a1)
	beq	l68
	lea	(1,a1),a0
	bra	l69
l68
	move.l	a4,a0
l69
	move.l	a0,a5
l65
	lea	(9216,a3),a2
	move.w	(2,a2),d0
	muls.w	#288,d0
	lea	(a3,d0.w),a0
	move.l	a5,a1
	moveq	#47,d1
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
	move.w	(2,a2),d0
	muls.w	#288,d0
	move.b	#0,(47,a3,d0.w)
	move.w	(2,a2),d0
	muls.w	#288,d0
	lea	(a3,d0.w),a1
	lea	(48,a1),a0
	move.l	a4,a1
	inline
	move.l	a0,d0
.l1
	move.b	(a1)+,(a0)+
	bne	.l1
	einline
	addq.l	#1,(a2)
	moveq	#1,d0
l52
l73	reg	a2/a3/a4/a5/d2
	movem.l	(a7)+,a2/a3/a4/a5/d2
l75	equ	20
	rts
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	cnop	0,4
l77
	sub.w	#352,a7
	movem.l	l94,-(a7)
	moveq	#48,d6
	moveq	#96,d5
	move.l	(360+l96,a7),a5
	move.l	#l80,(0+l96,a7)
	move.l	#l81,(4+l96,a7)
	move.l	a5,(8+l96,a7)
	moveq	#8,d0
	move.l	d0,(12+l96,a7)
	move.l	#l82,(16+l96,a7)
	move.l	#l83,(20+l96,a7)
	move.l	a5,a0
	addq.l	#8,a0
	move.l	a0,(24+l96,a7)
	moveq	#8,d0
	move.l	d0,(28+l96,a7)
	moveq	#0,d1
l92
	moveq	#1,d2
	add.l	d1,d2
	move.l	d2,-(a7)
	pea	l88
	lea	(200+l96,a7),a2
	move.w	d1,d3
	move.w	d3,d4
	lsl.w	#1,d4
	move.w	d3,d0
	lsl.w	#5,d0
	lea	(a2,d0.w),a3
	move.l	a3,-(a7)
	jsr	_sprintf
	move.l	d2,-(a7)
	pea	l89
	moveq	#1,d0
	add.w	d4,d0
	lsl.w	#4,d0
	add.w	d0,a2
	move.l	a2,-(a7)
	jsr	_sprintf
	lea	(24+l96,a7),a0
	moveq	#2,d0
	add.w	d4,d0
	lsl.w	#4,d0
	lea	(a0,d0.w),a1
	move.l	a3,(a1)
	move.l	#l90,(4,a1)
	lea	(16,a5),a3
	moveq	#48,d0
	muls.w	d3,d0
	add.w	d0,a3
	move.l	a3,(8,a1)
	move.l	d6,(12,a1)
	moveq	#3,d0
	add.w	d4,d0
	lsl.w	#4,d0
	add.w	d0,a0
	move.l	a2,(a0)
	move.l	#l91,(4,a0)
	lea	(256,a5),a1
	moveq	#96,d0
	muls.w	d3,d0
	add.w	d0,a1
	move.l	a1,(8,a0)
	move.l	d5,(12,a0)
	move.l	d2,d1
	add.w	#24,a7
	moveq	#5,d0
	cmp.l	d2,d0
	bgt	l92
	move.l	#12,-(a7)
	lea	(4+l96,a7),a0
	move.l	a0,-(a7)
	move.l	(364+l96,a7),-(a7)
	jsr	_nd_plugin_option_strings
	add.w	#12,a7
l94	reg	a2/a3/a4/a5/d2/d3/d4/d5/d6
	movem.l	(a7)+,a2/a3/a4/a5/d2/d3/d4/d5/d6
l96	equ	36
	add.w	#352,a7
	rts
	cnop	0,4
l88
	dc.b	78
	dc.b	65
	dc.b	77
	dc.b	69
	dc.b	37
	dc.b	100
	dc.b	0
	cnop	0,4
l89
	dc.b	66
	dc.b	79
	dc.b	79
	dc.b	75
	dc.b	77
	dc.b	65
	dc.b	82
	dc.b	75
	dc.b	37
	dc.b	100
	dc.b	0
	cnop	0,4
l90
	dc.b	0
	cnop	0,4
l91
	dc.b	0
	cnop	0,4
l80
	dc.b	83
	dc.b	89
	dc.b	83
	dc.b	84
	dc.b	69
	dc.b	77
	dc.b	0
	cnop	0,4
l81
	dc.b	89
	dc.b	69
	dc.b	83
	dc.b	0
	cnop	0,4
l82
	dc.b	86
	dc.b	79
	dc.b	76
	dc.b	85
	dc.b	77
	dc.b	69
	dc.b	83
	dc.b	0
	cnop	0,4
l83
	dc.b	89
	dc.b	69
	dc.b	83
	dc.b	0
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	cnop	0,4
l97
	movem.l	l108,-(a7)
	moveq	#0,d4
	move.l	_DOSBase,a6
	move.l	(4+l110,a7),d1
	moveq	#-2,d2
	jsr	-84(a6)
	move.l	d0,d3
	bne	l101
	moveq	#0,d0
	bra	l98
l101
	move.l	_DOSBase,a6
	moveq	#2,d1
	moveq	#0,d2
	jsr	-228(a6)
	move.l	d0,a2
	move.l	a2,d1
	beq	l103
	move.l	_DOSBase,a6
	move.l	d3,d1
	move.l	a2,d2
	jsr	-102(a6)
	tst.l	d0
	beq	l106
	tst.l	(4,a2)
	ble	l106
	moveq	#1,d0
	bra	l107
l106
	moveq	#0,d0
l107
	move.l	d0,d4
	move.l	_DOSBase,a6
	moveq	#2,d1
	move.l	a2,d2
	jsr	-234(a6)
l103
	move.l	_DOSBase,a6
	move.l	d3,d1
	jsr	-90(a6)
	move.l	d4,d0
l98
l108	reg	a2/a6/d2/d3/d4
	movem.l	(a7)+,a2/a6/d2/d3/d4
l110	equ	20
	rts
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	cnop	0,4
l111
	sub.w	#240,a7
	movem.l	l166,-(a7)
	moveq	#58,d6
	moveq	#6,d5
	moveq	#5,d4
	move.l	(244+l168,a7),a2
	lea	l76,a0
	moveq	#0,d0
	move.l	#9220,d2
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
	move.l	a2,a0
	lea	l130,a1
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
	beq	l163
	moveq	#0,d2
l160
	move.w	d2,d0
	lsl.w	#2,d0
	lea	l121,a0
	move.l	(0,a0,d0.w),-(a7)
	jsr	l97
	addq.w	#4,a7
	tst.l	d0
	beq	l136
	move.w	d2,d0
	lsl.w	#2,d0
	lea	l121,a0
	move.l	(0,a0,d0.w),-(a7)
	lea	l114,a0
	move.l	(0,a0,d0.w),-(a7)
	pea	l76
	jsr	l51
	add.w	#12,a7
l136
	addq.l	#1,d2
	cmp.l	d2,d5
	bgt	l160
l163
	moveq	#0,d2
l161
	lea	(256,a2),a0
	move.w	d2,d0
	moveq	#96,d1
	muls.w	d0,d1
	pea	(0,a0,d1.w)
	muls.w	#48,d0
	pea	(16,a2,d0.w)
	pea	l76
	jsr	l51
	addq.l	#1,d2
	add.w	#12,a7
	cmp.l	d2,d4
	bgt	l161
	lea	(8,a2),a0
	lea	l143,a1
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
	beq	l112
	move.l	_DOSBase,a6
	moveq	#9,d1
	jsr	-654(a6)
	tst.l	d0
	beq	l112
	move.l	_DOSBase,a6
	move.l	d0,d1
	moveq	#8,d2
	jsr	-690(a6)
	move.l	d0,a3
	move.l	a3,d1
	beq	l165
	moveq	#47,d5
	move.l	#237,d4
l146
	tst.l	(8,a3)
	beq	l149
	move.l	(40,a3),d0
	lsl.l	#2,d0
	move.l	d0,a4
	tst.l	d0
	beq	l149
	moveq	#0,d3
	move.b	(a4),d3
	tst.l	d3
	ble	l149
	cmp.l	d3,d4
	blt	l149
	lea	(1,a4),a1
	lea	(0+l168,a7),a2
	move.l	a2,a0
	move.l	d3,d2
	inline
	move.l	a0,d0
	cmp.l	#16,d2
	blo	.l5
	moveq	#1,d1
	and.b	d0,d1
	beq	.l1
	move.b	(a1)+,(a0)+
	subq.l	#1,d2
.l1
	move.l	a1,d1
	and.b	#1,d1
	beq	.l3
	cmp.l	#$10000,d2
	blo	.l5
.l2
	move.b	(a1)+,(a0)+
	subq.l	#1,d2
	bne	.l2
	bra	.l7
.l3
	moveq	#3,d1
	and.l	d2,d1
	sub.l	d1,d2
.l4
	move.l	(a1)+,(a0)+
	subq.l	#4,d2
	bne	.l4
	move.w	d1,d2
.l5
	subq.w	#1,d2
	blo	.l7
.l6
	move.b	(a1)+,(a0)+
	dbf	d2,.l6
.l7
	einline
	move.w	d3,d0
	move.b	#0,(0,a2,d0.w)
	move.l	d6,-(a7)
	move.l	a2,-(a7)
	jsr	_strchr
	addq.w	#8,a7
	tst.l	d0
	bne	l149
	move.l	d5,-(a7)
	lea	(4+l168,a7),a0
	move.l	a0,-(a7)
	jsr	_strchr
	addq.w	#8,a7
	tst.l	d0
	bne	l149
	lea	(0+l168,a7),a0
	move.w	d3,d0
	move.b	#58,(0,a0,d0.w)
	addq.w	#1,d0
	move.b	#0,(0,a0,d0.w)
	move.l	a0,-(a7)
	move.l	#0,-(a7)
	pea	l76
	jsr	l51
	add.w	#12,a7
l149
	move.l	_DOSBase,a6
	move.l	a3,d1
	moveq	#8,d2
	jsr	-690(a6)
	move.l	d0,a3
	move.l	a3,d1
	bne	l146
l165
	moveq	#6,d5
	moveq	#5,d4
	move.l	_DOSBase,a6
	moveq	#9,d1
	jsr	-660(a6)
l112
l166	reg	a2/a3/a4/a6/d2/d3/d4/d5/d6
	movem.l	(a7)+,a2/a3/a4/a6/d2/d3/d4/d5/d6
l168	equ	36
	add.w	#240,a7
	rts
	cnop	0,4
l115
	dc.b	83
	dc.b	121
	dc.b	115
	dc.b	116
	dc.b	101
	dc.b	109
	dc.b	0
	cnop	0,4
l116
	dc.b	82
	dc.b	65
	dc.b	77
	dc.b	32
	dc.b	68
	dc.b	105
	dc.b	115
	dc.b	107
	dc.b	0
	cnop	0,4
l117
	dc.b	80
	dc.b	114
	dc.b	101
	dc.b	102
	dc.b	101
	dc.b	114
	dc.b	101
	dc.b	110
	dc.b	99
	dc.b	101
	dc.b	115
	dc.b	0
	cnop	0,4
l118
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
l119
	dc.b	84
	dc.b	111
	dc.b	111
	dc.b	108
	dc.b	115
	dc.b	0
	cnop	0,4
l120
	dc.b	84
	dc.b	114
	dc.b	97
	dc.b	115
	dc.b	104
	dc.b	0
	cnop	0,4
l122
	dc.b	83
	dc.b	89
	dc.b	83
	dc.b	58
	dc.b	0
	cnop	0,4
l123
	dc.b	82
	dc.b	65
	dc.b	77
	dc.b	58
	dc.b	0
	cnop	0,4
l124
	dc.b	83
	dc.b	89
	dc.b	83
	dc.b	58
	dc.b	80
	dc.b	114
	dc.b	101
	dc.b	102
	dc.b	115
	dc.b	0
	cnop	0,4
l125
	dc.b	83
	dc.b	89
	dc.b	83
	dc.b	58
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
l126
	dc.b	83
	dc.b	89
	dc.b	83
	dc.b	58
	dc.b	84
	dc.b	111
	dc.b	111
	dc.b	108
	dc.b	115
	dc.b	0
	cnop	0,4
l127
	dc.b	83
	dc.b	89
	dc.b	83
	dc.b	58
	dc.b	84
	dc.b	114
	dc.b	97
	dc.b	115
	dc.b	104
	dc.b	99
	dc.b	97
	dc.b	110
	dc.b	0
	cnop	0,4
l130
	dc.b	78
	dc.b	79
	dc.b	0
	cnop	0,4
l143
	dc.b	78
	dc.b	79
	dc.b	0
	section	"DATA",data
	cnop	0,4
l114
	dc.l	l115
	dc.l	l116
	dc.l	l117
	dc.l	l118
	dc.l	l119
	dc.l	l120
	cnop	0,4
l121
	dc.l	l122
	dc.l	l123
	dc.l	l124
	dc.l	l125
	dc.l	l126
	dc.l	l127
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	section	"CODE",code
	cnop	0,4
l169
	sub.w	#9768,a7
	movem.l	l179,-(a7)
	move.l	(9776+l181,a7),-(a7)
	jsr	l111
	lea	(4+l181,a7),a0
	moveq	#0,d0
	move.l	#168,d2
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
	move.l	9216+l76,(132+l181,a7)
	moveq	#0,d4
	addq.w	#4,a7
	tst.l	9216+l76
	ble	l178
l177
	move.w	d4,d3
	move.w	#288,d1
	muls.w	d3,d1
	lea	48+l76,a1
	pea	(0,a1,d1.w)
	lea	l76,a1
	pea	(0,a1,d1.w)
	pea	l176
	lea	(180+l181,a7),a2
	move.w	#300,d1
	muls.w	d3,d1
	add.w	d1,a2
	move.l	a2,-(a7)
	jsr	_sprintf
	lea	(16+l181,a7),a1
	move.w	d3,d1
	lsl.w	#2,d1
	move.l	a2,(0,a1,d1.w)
	addq.l	#1,d4
	add.w	#16,a7
	cmp.l	9216+l76,d4
	blt	l177
l178
	lea	(0+l181,a7),a1
	move.l	a1,-(a7)
	move.l	(9776+l181,a7),-(a7)
	jsr	_nd_clip_quick_open
	addq.w	#8,a7
l179	reg	a2/d2/d3/d4
	movem.l	(a7)+,a2/d2/d3/d4
l181	equ	16
	add.w	#9768,a7
	rts
	cnop	0,4
l176
	dc.b	37
	dc.b	115
	dc.b	10
	dc.b	37
	dc.b	115
	dc.b	0
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	cnop	0,4
l182
	sub.w	#996,a7
	movem.l	l206,-(a7)
	move.l	(1000+l208,a7),a2
	move.l	a2,-(a7)
	jsr	l1
	addq.w	#4,a7
	tst.l	d0
	beq	l185
	move.l	a2,-(a7)
	jsr	l97
	addq.w	#4,a7
	tst.l	d0
	bne	l186
l185
	moveq	#0,d0
	bra	l183
l186
	tst.l	_WorkbenchBase
	beq	l189
	move.l	_WorkbenchBase,a1
	cmp.w	#44,(20,a1)
	bcs	l189
	move.l	#0,-(a7)
	move.l	_WorkbenchBase,a6
	move.l	a2,a0
	move.l	a1,-(a7)
	lea	4(a7),a1
	jsr	-96(a6)
	movea.l	(a7)+,a1
	addq.w	#4,a7
	tst.w	d0
	beq	l189
	moveq	#1,d0
	bra	l183
l189
	move.l	a2,-(a7)
	move.l	#484,-(a7)
	lea	(8+l208,a7),a1
	move.l	a1,-(a7)
	jsr	_nd_quote_path
	add.w	#12,a7
	tst.l	d0
	bne	l193
	moveq	#0,d0
	bra	l183
l193
	move.l	_DOSBase,a6
	move.l	#l194,d1
	move.l	#1005,d2
	jsr	-30(a6)
	move.l	d0,d4
	move.l	_DOSBase,a6
	move.l	#l195,d1
	move.l	#1006,d2
	jsr	-30(a6)
	move.l	d0,d3
	tst.l	d4
	beq	l196
	tst.l	d3
	bne	l197
l196
	tst.l	d4
	beq	l200
	move.l	_DOSBase,a6
	move.l	d4,d1
	jsr	-36(a6)
l200
	tst.l	d3
	beq	l202
	move.l	_DOSBase,a6
	move.l	d3,d1
	jsr	-36(a6)
l202
	moveq	#0,d0
	bra	l183
l197
	lea	(0+l208,a7),a1
	move.l	a1,-(a7)
	pea	l203
	lea	(492+l208,a7),a2
	move.l	a2,-(a7)
	jsr	_sprintf
	move.l	#0,-(a7)
	move.l	#1,-(a7)
	move.l	#-2147483613,-(a7)
	move.l	d3,-(a7)
	move.l	#-2147483614,-(a7)
	move.l	d4,-(a7)
	move.l	#-2147483615,-(a7)
	move.l	_DOSBase,a6
	move.l	a2,d1
	move.l	d2,-(a7)
	move.l	a7,d2
	add.l	#4,d2
	jsr	-606(a6)
	move.l	(a7)+,d2
	add.w	#40,a7
	moveq	#-1,d1
	cmp.l	d0,d1
	bne	l205
	move.l	_DOSBase,a6
	move.l	d4,d1
	jsr	-36(a6)
	move.l	_DOSBase,a6
	move.l	d3,d1
	jsr	-36(a6)
	moveq	#0,d0
	bra	l183
l205
	moveq	#1,d0
l183
l206	reg	a2/a6/d2/d3/d4
	movem.l	(a7)+,a2/a6/d2/d3/d4
l208	equ	20
	add.w	#996,a7
	rts
	cnop	0,4
l194
	dc.b	78
	dc.b	73
	dc.b	76
	dc.b	58
	dc.b	0
	cnop	0,4
l195
	dc.b	78
	dc.b	73
	dc.b	76
	dc.b	58
	dc.b	0
	cnop	0,4
l203
	dc.b	67
	dc.b	58
	dc.b	87
	dc.b	66
	dc.b	82
	dc.b	85
	dc.b	78
	dc.b	32
	dc.b	37
	dc.b	115
	dc.b	0
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	public	_main
	cnop	0,4
_main
	sub.w	#1092,a7
	movem.l	l266,-(a7)
	move.l	#500,d6
	move.l	(1100+l268,a7),a3
	moveq	#0,d4
	moveq	#1,d3
	moveq	#20,d5
	lea	l211,a0
	lea	(324+l268,a7),a1
	moveq.l	#1,d7
l269:
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	dbra	d7,l269
	moveq	#2,d7
	cmp.l	(1096+l268,a7),d7
	ble	l213
	moveq	#20,d0
	bra	l209
l213
	move.l	_SysBase,a6
	move.l	#0,a1
	jsr	-294(a6)
	move.l	d0,a1
	move.l	#-1,(184,a1)
	move.l	_SysBase,a6
	lea	l214,a1
	moveq	#39,d0
	jsr	-552(a6)
	move.l	d0,_IntuitionBase
	move.l	_SysBase,a6
	lea	l215,a1
	moveq	#39,d0
	jsr	-552(a6)
	move.l	d0,_GfxBase
	move.l	_SysBase,a6
	lea	l216,a1
	moveq	#36,d0
	jsr	-552(a6)
	move.l	d0,_WorkbenchBase
	tst.l	_IntuitionBase
	beq	l221
	tst.l	_GfxBase
	beq	l221
	move.l	(4,a3),-(a7)
	lea	(4+l268,a7),a0
	move.l	a0,-(a7)
	jsr	_nd_client_init
	addq.w	#8,a7
	tst.l	d0
	beq	l221
	jsr	_nd_clip_quick_create
	move.l	d0,d2
	bne	l223
	lea	(0+l268,a7),a0
	move.l	a0,-(a7)
	jsr	_nd_client_close
	addq.w	#4,a7
	bra	l221
l223
	pea	l224
	pea	l225
	pea	l226
	pea	l227
	pea	l228
	move.l	d2,-(a7)
	jsr	_nd_clip_quick_labels
	move.l	#65535,a4
	add.w	#24,a7
l229
	tst.l	d3
	beq	l234
	move.l	d2,-(a7)
	jsr	_nd_clip_quick_close
	lea	(360+l268,a7),a0
	move.l	a0,-(a7)
	move.l	(4,a3),-(a7)
	jsr	l77
	moveq	#0,d3
	add.w	#12,a7
l234
	move.l	d6,-(a7)
	move.l	d4,-(a7)
	move.l	a4,-(a7)
	lea	(336+l268,a7),a0
	move.l	a0,-(a7)
	pea	l235
	lea	(20+l268,a7),a0
	move.l	a0,-(a7)
	jsr	_nd_client_update_quiet
	move.b	d0,d1
	and.b	#2,d1
	add.w	#24,a7
	beq	l237
	moveq	#0,d5
	bra	l265
l237
	move.b	d0,d1
	and.b	#4,d1
	beq	l239
	moveq	#1,d3
	bra	l229
l239
	move.b	d0,d1
	and.b	#1,d1
	beq	l246
	move.l	d2,-(a7)
	jsr	_nd_clip_quick_active
	addq.w	#4,a7
	tst.l	d0
	beq	l243
	move.l	d2,-(a7)
	jsr	_nd_clip_quick_close
	addq.w	#4,a7
	bra	l246
l243
	lea	(356+l268,a7),a0
	move.l	a0,-(a7)
	move.l	d2,-(a7)
	jsr	l169
	addq.w	#8,a7
	tst.l	d0
	bne	l246
	lea	l247,a0
	lea	(324+l268,a7),a1
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.w	(a0)+,(a1)+
	move.b	(a0)+,(a1)+
l246
	move.l	d2,-(a7)
	jsr	_nd_clip_quick_events
	addq.w	#4,a7
	tst.l	d0
	beq	l251
	move.l	d2,-(a7)
	jsr	_nd_clip_quick_selected
	move.l	d0,a2
	move.l	d2,-(a7)
	jsr	_nd_clip_quick_close
	addq.w	#8,a7
	move.l	a2,d7
	blt	l251
	cmp.l	9216+l76,a2
	bge	l251
	move.w	a2,d1
	muls.w	#288,d1
	lea	48+l76,a0
	pea	(0,a0,d1.w)
	jsr	l182
	addq.w	#4,a7
	tst.l	d0
	beq	l254
	move.l	#l256,d1
	bra	l255
l254
	move.l	#l257,d1
l255
	lea	(324+l268,a7),a0
	move.l	d1,a1
	inline
	move.l	a0,d0
.l1
	move.b	(a1)+,(a0)+
	bne	.l1
	einline
l251
	move.l	d2,-(a7)
	jsr	_nd_clip_quick_signals
	move.l	d0,-(a7)
	move.l	d6,-(a7)
	lea	(12+l268,a7),a0
	move.l	a0,-(a7)
	jsr	_nd_client_wait_signals
	move.l	d0,d4
	add.w	#16,a7
	bra	l229
l265
	move.l	d2,-(a7)
	jsr	_nd_clip_quick_free
	lea	(4+l268,a7),a0
	move.l	a0,-(a7)
	jsr	_nd_client_close
	addq.w	#8,a7
l221
	tst.l	_WorkbenchBase
	beq	l259
	move.l	_WorkbenchBase,a1
	move.l	_SysBase,a6
	jsr	-414(a6)
l259
	tst.l	_GfxBase
	beq	l261
	move.l	_GfxBase,a1
	move.l	_SysBase,a6
	jsr	-414(a6)
l261
	tst.l	_IntuitionBase
	beq	l263
	move.l	_IntuitionBase,a1
	move.l	_SysBase,a6
	jsr	-414(a6)
l263
	move.l	d5,d0
l209
l266	reg	a2/a3/a4/a6/d2/d3/d4/d5/d6/d7
	movem.l	(a7)+,a2/a3/a4/a6/d2/d3/d4/d5/d6/d7
l268	equ	40
	add.w	#1092,a7
	rts
	cnop	0,4
l247
	dc.b	80
	dc.b	105
	dc.b	99
	dc.b	107
	dc.b	101
	dc.b	114
	dc.b	32
	dc.b	117
	dc.b	110
	dc.b	97
	dc.b	118
	dc.b	97
	dc.b	105
	dc.b	108
	dc.b	97
	dc.b	98
	dc.b	108
	dc.b	101
	dc.b	0
	cnop	0,4
l256
	dc.b	67
	dc.b	108
	dc.b	105
	dc.b	99
	dc.b	107
	dc.b	32
	dc.b	102
	dc.b	111
	dc.b	114
	dc.b	32
	dc.b	112
	dc.b	108
	dc.b	97
	dc.b	99
	dc.b	101
	dc.b	115
	dc.b	0
	cnop	0,4
l257
	dc.b	68
	dc.b	114
	dc.b	97
	dc.b	119
	dc.b	101
	dc.b	114
	dc.b	32
	dc.b	117
	dc.b	110
	dc.b	97
	dc.b	118
	dc.b	97
	dc.b	105
	dc.b	108
	dc.b	97
	dc.b	98
	dc.b	108
	dc.b	101
	dc.b	59
	dc.b	32
	dc.b	99
	dc.b	104
	dc.b	101
	dc.b	99
	dc.b	107
	dc.b	32
	dc.b	112
	dc.b	97
	dc.b	116
	dc.b	104
	dc.b	0
	cnop	0,4
l235
	dc.b	80
	dc.b	76
	dc.b	65
	dc.b	67
	dc.b	69
	dc.b	83
	dc.b	0
	cnop	0,4
l211
	dc.b	67
	dc.b	108
	dc.b	105
	dc.b	99
	dc.b	107
	dc.b	32
	dc.b	102
	dc.b	111
	dc.b	114
	dc.b	32
	dc.b	112
	dc.b	108
	dc.b	97
	dc.b	99
	dc.b	101
	dc.b	115
	dc.b	0
	ds.b	15
	cnop	0,4
l214
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
l215
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
l216
	dc.b	119
	dc.b	111
	dc.b	114
	dc.b	107
	dc.b	98
	dc.b	101
	dc.b	110
	dc.b	99
	dc.b	104
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
l224
	dc.b	80
	dc.b	108
	dc.b	97
	dc.b	99
	dc.b	101
	dc.b	0
	cnop	0,4
l225
	dc.b	67
	dc.b	108
	dc.b	105
	dc.b	99
	dc.b	107
	dc.b	32
	dc.b	111
	dc.b	114
	dc.b	32
	dc.b	82
	dc.b	101
	dc.b	116
	dc.b	117
	dc.b	114
	dc.b	110
	dc.b	32
	dc.b	116
	dc.b	111
	dc.b	32
	dc.b	111
	dc.b	112
	dc.b	101
	dc.b	110
	dc.b	32
	dc.b	45
	dc.b	32
	dc.b	69
	dc.b	115
	dc.b	99
	dc.b	32
	dc.b	116
	dc.b	111
	dc.b	32
	dc.b	99
	dc.b	108
	dc.b	111
	dc.b	115
	dc.b	101
	dc.b	0
	cnop	0,4
l226
	dc.b	78
	dc.b	111
	dc.b	118
	dc.b	97
	dc.b	68
	dc.b	111
	dc.b	99
	dc.b	107
	dc.b	32
	dc.b	80
	dc.b	108
	dc.b	97
	dc.b	99
	dc.b	101
	dc.b	115
	dc.b	0
	cnop	0,4
l227
	dc.b	78
	dc.b	111
	dc.b	32
	dc.b	112
	dc.b	108
	dc.b	97
	dc.b	99
	dc.b	101
	dc.b	115
	dc.b	32
	dc.b	99
	dc.b	111
	dc.b	110
	dc.b	102
	dc.b	105
	dc.b	103
	dc.b	117
	dc.b	114
	dc.b	101
	dc.b	100
	dc.b	0
	cnop	0,4
l228
	dc.b	80
	dc.b	76
	dc.b	65
	dc.b	67
	dc.b	69
	dc.b	83
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
	dc.b	80
	dc.b	108
	dc.b	97
	dc.b	99
	dc.b	101
	dc.b	115
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
	public	_IntuitionBase
	section	"DATA",data
	cnop	0,4
_IntuitionBase
	dc.l	0
	public	_WorkbenchBase
	cnop	0,4
_WorkbenchBase
	dc.l	0
	public	_GfxBase
	cnop	0,4
_GfxBase
	dc.l	0
	public	___stack
	cnop	0,4
___stack
	dc.l	16384
	public	_SysBase
	public	_DOSBase
	public	_sprintf
	public	_strchr
	public	_nd_client_init
	public	_nd_client_update_quiet
	public	_nd_client_wait_signals
	public	_nd_client_close
	public	_nd_plugin_option_strings
	public	_nd_quote_path
	public	_nd_clip_quick_create
	public	_nd_clip_quick_open
	public	_nd_clip_quick_signals
	public	_nd_clip_quick_active
	public	_nd_clip_quick_events
	public	_nd_clip_quick_labels
	public	_nd_clip_quick_selected
	public	_nd_clip_quick_close
	public	_nd_clip_quick_free
	section	"BSS",bss
	cnop	0,4
l76
	ds.b	9220

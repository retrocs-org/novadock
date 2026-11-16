	idnt	"plugin_src/download/download.c"
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
	sub.w	#12,a7
	movem.l	l74,-(a7)
	lea	(0+l76,a7),a0
	move.l	_DOSBase,a6
	move.l	a0,d1
	jsr	-192(a6)
	move.l	#1440,d0
	move.l	(0+l76,a7),d1
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
	add.l	(4+l76,a7),d0
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
	add.l	(8+l76,a7),d0
l74	reg	a6/d2/d3
	movem.l	(a7)+,a6/d2/d3
l76	equ	12
	add.w	#12,a7
	rts
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	cnop	0,4
l77
	movem.l	l82,-(a7)
	moveq	#0,d2
	move.l	_SysBase,a6
	jsr	-132(a6)
	move.l	_SysBase,a6
	move.l	(4+l84,a7),a1
	jsr	-390(a6)
	tst.l	d0
	beq	l81
	move.l	_SysBase,a6
	move.l	d0,a0
	move.l	(8+l84,a7),a1
	jsr	-366(a6)
	moveq	#1,d2
l81
	move.l	_SysBase,a6
	jsr	-138(a6)
	move.l	d2,d0
l82	reg	a6/d2
	movem.l	(a7)+,a6/d2
l84	equ	8
	rts
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	cnop	0,4
l85
	sub.w	#200,a7
	movem.l	l94,-(a7)
	move.l	(204+l96,a7),d2
	beq	l86
	move.l	d2,-(a7)
	jsr	l36
	addq.w	#4,a7
	tst.l	d0
	beq	l86
	move.l	d2,a0
	inline
	move.l	a0,d0
.l1
	tst.b	(a0)+
	bne	.l1
	sub.l	a0,d0
	not.l	d0
	einline
	cmp.l	#150,d0
	bhi	l86
	move.l	d2,-(a7)
	pea	l92
	lea	(8+l96,a7),a2
	move.l	a2,-(a7)
	jsr	_sprintf
	move.l	_DOSBase,a6
	move.l	a2,d1
	jsr	-72(a6)
	move.l	d2,-(a7)
	pea	l93
	move.l	a2,-(a7)
	jsr	_sprintf
	move.l	_DOSBase,a6
	move.l	a2,d1
	jsr	-72(a6)
	move.l	_DOSBase,a6
	move.l	d2,d1
	jsr	-72(a6)
	add.w	#24,a7
l86
l94	reg	a2/a6/d2
	movem.l	(a7)+,a2/a6/d2
l96	equ	12
	add.w	#200,a7
	rts
	cnop	0,4
l92
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
l93
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
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	cnop	0,4
l97
	movem.l	l111,-(a7)
	moveq	#-1,d4
	moveq	#1,d3
	move.l	(4+l113,a7),d2
	beq	l98
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
	beq	l110
l109
	cmp.l	(24,a2),d3
	bne	l106
	tst.l	(14,a2)
	beq	l106
	move.l	d4,(24,a2)
	move.l	_SysBase,a6
	move.l	a2,a1
	jsr	-378(a6)
	bra	l108
l106
	pea	(1832,a2)
	jsr	l85
	move.l	_SysBase,a6
	move.l	a2,a1
	jsr	-690(a6)
	addq.w	#4,a7
l108
	move.l	_SysBase,a6
	move.l	d2,a0
	jsr	-372(a6)
	move.l	d0,a2
	tst.l	d0
	bne	l109
l110
	move.l	_SysBase,a6
	move.l	d2,a0
	jsr	-672(a6)
l98
l111	reg	a2/a6/d2/d3/d4
	movem.l	(a7)+,a2/a6/d2/d3/d4
l113	equ	20
	rts
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	cnop	0,4
l114
	sub.w	#204,a7
	movem.l	l174,-(a7)
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
	beq	l117
	move.l	a4,d0
	beq	l117
	move.l	a3,d0
	bne	l118
l117
	tst.l	d3
	beq	l122
	move.l	_SysBase,a6
	move.l	d3,a0
	jsr	-672(a6)
l122
	move.l	a4,d0
	beq	l124
	move.l	_SysBase,a6
	move.l	a4,a0
	jsr	-672(a6)
l124
	move.l	a3,d0
	beq	l126
	move.l	_SysBase,a6
	move.l	a3,a1
	jsr	-690(a6)
l126
	moveq	#20,d0
	bra	l115
l118
	move.l	_SysBase,a6
	move.l	#0,a1
	jsr	-294(a6)
	move.l	d0,a1
	move.l	#-1,(184,a1)
	move.l	_SysBase,a6
	jsr	-132(a6)
	move.l	_SysBase,a6
	lea	l129,a1
	jsr	-390(a6)
	tst.l	d0
	bne	l128
	move.l	d3,a1
	move.l	#l130,(10,a1)
	move.l	_SysBase,a6
	move.l	d3,a1
	jsr	-354(a6)
	move.l	#1,a2
l128
	move.l	_SysBase,a6
	jsr	-138(a6)
	move.l	a2,d0
	bne	l132
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
	bra	l115
l132
	move.l	#1313101638,(20,a3)
	moveq	#1,d0
	move.l	d0,(24,a3)
	move.w	#1992,(18,a3)
	move.l	a4,(14,a3)
	move.l	a3,-(a7)
	move.l	(212+l176,a7),-(a7)
	jsr	l77
	addq.w	#8,a7
	tst.l	d0
	beq	l165
	moveq	#0,d4
	move.l	d3,(0+l176,a7)
	tst.l	d2
	bne	l173
	move.l	#150,d3
l135
	move.l	_SysBase,a6
	move.l	a4,a0
	jsr	-372(a6)
	tst.l	d0
	beq	l141
	moveq	#1,d2
	bra	l142
l141
	move.l	_DOSBase,a6
	moveq	#2,d1
	jsr	-198(a6)
l142
	addq.l	#2,d4
	cmp.l	d4,d3
	ble	l173
	tst.l	d2
	beq	l135
l173
	move.l	(0+l176,a7),d3
	move.l	_SysBase,a6
	jsr	-132(a6)
	tst.l	d2
	bne	l145
	move.l	_SysBase,a6
	move.l	a4,a0
	jsr	-372(a6)
	tst.l	d0
	beq	l145
	moveq	#1,d2
l145
	tst.l	d2
	bne	l148
	move.b	#2,(14,a4)
	move.l	#0,(16,a4)
	moveq	#0,d0
	move.b	(15,a4),d0
	move.l	_SysBase,a6
	jsr	-336(a6)
l148
	move.l	_SysBase,a6
	jsr	-138(a6)
	tst.l	d2
	bne	l150
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
	bra	l115
l150
	move.l	_SysBase,a6
	move.l	a4,a0
	jsr	-672(a6)
	move.l	#0,a4
	moveq	#1,d0
	cmp.l	(24,a3),d0
	bne	l165
	tst.b	(32,a3)
	beq	l165
	pea	(1832,a3)
	pea	l154
	lea	(12+l176,a7),a2
	move.l	a2,-(a7)
	jsr	_sprintf
	move.l	_DOSBase,a6
	move.l	#l155,d1
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
	beq	l157
	tst.l	d5
	beq	l157
	move.l	#0,-(a7)
	move.l	#131072,-(a7)
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
l157
	tst.l	d6
	beq	l160
	move.l	_DOSBase,a6
	move.l	d6,d1
	jsr	-36(a6)
l160
	tst.l	d5
	beq	l162
	move.l	_DOSBase,a6
	move.l	d5,d1
	jsr	-36(a6)
l162
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
	move.l	(212+l176,a7),-(a7)
	jsr	l77
	addq.w	#8,a7
	tst.l	d0
	beq	l164
	move.l	#0,a3
	bra	l165
l164
	pea	(1832,a3)
	jsr	l85
	addq.w	#4,a7
l165
	move.l	a4,d0
	beq	l167
	move.l	_SysBase,a6
	move.l	a4,a0
	jsr	-672(a6)
l167
	move.l	a3,d0
	beq	l169
	move.l	_SysBase,a6
	move.l	a3,a1
	jsr	-690(a6)
l169
	tst.l	d3
	beq	l171
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
l171
	moveq	#0,d0
l115
l174	reg	a2/a3/a4/a6/d2/d3/d4/d5/d6
	movem.l	(a7)+,a2/a3/a4/a6/d2/d3/d4/d5/d6
l176	equ	36
	add.w	#204,a7
	rts
	cnop	0,4
l130
	dc.b	78
	dc.b	111
	dc.b	118
	dc.b	97
	dc.b	68
	dc.b	111
	dc.b	99
	dc.b	107
	dc.b	46
	dc.b	68
	dc.b	111
	dc.b	119
	dc.b	110
	dc.b	108
	dc.b	111
	dc.b	97
	dc.b	100
	dc.b	46
	dc.b	70
	dc.b	101
	dc.b	116
	dc.b	99
	dc.b	104
	dc.b	0
	cnop	0,4
l154
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
l155
	dc.b	78
	dc.b	73
	dc.b	76
	dc.b	58
	dc.b	0
	cnop	0,4
l129
	dc.b	78
	dc.b	111
	dc.b	118
	dc.b	97
	dc.b	68
	dc.b	111
	dc.b	99
	dc.b	107
	dc.b	46
	dc.b	68
	dc.b	111
	dc.b	119
	dc.b	110
	dc.b	108
	dc.b	111
	dc.b	97
	dc.b	100
	dc.b	46
	dc.b	70
	dc.b	101
	dc.b	116
	dc.b	99
	dc.b	104
	dc.b	0
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	cnop	0,4
l183
	movem.l	l188,-(a7)
	move.l	_DOSBase,a6
	move.l	(4+l190,a7),d1
	moveq	#-2,d2
	jsr	-84(a6)
	tst.l	d0
	bne	l187
	moveq	#0,d0
	bra	l184
l187
	move.l	_DOSBase,a6
	move.l	d0,d1
	jsr	-90(a6)
	moveq	#1,d0
l184
l188	reg	a6/d2
	movem.l	(a7)+,a6/d2
l190	equ	8
	rts
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	cnop	0,4
l191
	sub.w	#32,a7
	movem.l	l198,-(a7)
	move.l	#l194,(0+l200,a7)
	move.l	#l195,(4+l200,a7)
	move.l	#l181,(8+l200,a7)
	moveq	#96,d0
	move.l	d0,(12+l200,a7)
	move.l	#l196,(16+l200,a7)
	move.l	#l197,(20+l200,a7)
	move.l	#l182,(24+l200,a7)
	moveq	#96,d0
	move.l	d0,(28+l200,a7)
	move.l	#2,-(a7)
	lea	(4+l200,a7),a0
	move.l	a0,-(a7)
	move.l	(44+l200,a7),-(a7)
	jsr	_nd_plugin_option_strings
	add.w	#12,a7
l198	reg
l200	equ	0
	add.w	#32,a7
	rts
	cnop	0,4
l194
	dc.b	80
	dc.b	82
	dc.b	79
	dc.b	71
	dc.b	82
	dc.b	65
	dc.b	77
	dc.b	0
	cnop	0,4
l195
	dc.b	67
	dc.b	58
	dc.b	99
	dc.b	117
	dc.b	114
	dc.b	108
	dc.b	0
	cnop	0,4
l196
	dc.b	68
	dc.b	82
	dc.b	65
	dc.b	87
	dc.b	69
	dc.b	82
	dc.b	0
	cnop	0,4
l197
	dc.b	82
	dc.b	65
	dc.b	77
	dc.b	58
	dc.b	0
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	cnop	0,4
l201
	movem.l	l209,-(a7)
	tst.l	l179
	beq	l205
	move.l	l179,-(a7)
	jsr	l97
	addq.w	#4,a7
l205
	tst.l	2048+l179
	bne	l207
	tst.b	84+l179
	beq	l207
	pea	84+l179
	jsr	l85
	addq.w	#4,a7
l207
	lea	l179,a0
	moveq	#0,d0
	move.l	#2052,d2
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
l209	reg	d2
	movem.l	(a7)+,d2
l211	equ	4
	rts
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	cnop	0,4
l212
	sub.w	#2704,a7
	movem.l	l277,-(a7)
	moveq	#20,d6
	tst.l	l178
	beq	l215
	tst.l	l179
	beq	l216
l215
	moveq	#0,d0
	bra	l213
l216
	move.l	_SysBase,a6
	jsr	-132(a6)
	move.l	_SysBase,a6
	lea	l221,a1
	jsr	-390(a6)
	tst.l	d0
	bne	l218
	moveq	#0,d2
	bra	l219
l218
	moveq	#1,d2
l219
	move.l	_SysBase,a6
	jsr	-138(a6)
	tst.l	d2
	beq	l223
	lea	l224,a0
	lea	l180,a1
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.b	(a0)+,(a1)+
	moveq	#0,d0
	bra	l213
l223
	pea	160+l177
	jsr	l183
	addq.w	#4,a7
	tst.l	d0
	beq	l226
	lea	l227,a0
	lea	l180,a1
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.w	(a0)+,(a1)+
	move.b	(a0)+,(a1)+
	moveq	#-1,d0
	bra	l213
l226
	move.l	_DOSBase,a6
	move.l	#l181,d1
	moveq	#-2,d2
	jsr	-84(a6)
	tst.l	d0
	bne	l229
	lea	l230,a0
	lea	l180,a1
	moveq.l	#1,d1
l280:
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	dbra	d1,l280
	move.l	(a0)+,(a1)+
	moveq	#-1,d0
	bra	l213
l229
	lea	(4+l279,a7),a0
	move.l	_DOSBase,a6
	move.l	d0,d1
	move.l	a0,d2
	move.l	#180,d3
	move.l	d0,(0+l279,a7)
	jsr	-402(a6)
	move.l	d0,d2
	move.l	(0+l279,a7),d0
	move.l	_DOSBase,a6
	move.l	d0,d1
	jsr	-90(a6)
	tst.l	d2
	bne	l232
	moveq	#-1,d0
	bra	l213
l232
	lea	(184+l279,a7),a2
	move.l	a2,a0
	lea	160+l177,a1
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
	tst.b	(184+l279,a7)
	beq	l233
	lea	(184+l279,a7),a0
	inline
	move.l	a0,d0
.l1
	tst.b	(a0)+
	bne	.l1
	sub.l	a0,d0
	not.l	d0
	einline
	moveq	#100,d1
	cmp.l	d0,d1
	bcc	l234
l233
	lea	l236,a0
	lea	l180,a1
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	moveq	#-1,d0
	bra	l213
l234
	lea	l179,a0
	moveq	#0,d0
	move.l	#2052,d2
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
	jsr	l71
	move.l	d0,2044+l179
	moveq	#0,d4
l275
	lea	(184+l279,a7),a1
	lea	84+l179,a0
	inline
	move.l	a0,d0
.l1
	move.b	(a1)+,(a0)+
	bne	.l1
	einline
	move.l	d4,-(a7)
	move.l	2044+l179,-(a7)
	move.l	_SysBase,a6
	move.l	#0,a1
	jsr	-294(a6)
	move.l	d0,a0
	move.l	a0,-(a7)
	pea	l241
	lea	(360+l279,a7),a2
	move.l	a2,-(a7)
	jsr	_sprintf
	move.l	_DOSBase,a6
	move.l	#84+l179,d1
	move.l	a2,d2
	move.l	#160,d3
	jsr	-882(a6)
	add.w	#20,a7
	tst.w	d0
	bne	l243
	moveq	#-1,d0
	bra	l213
l243
	move.l	_DOSBase,a6
	move.l	#84+l179,d1
	jsr	-120(a6)
	tst.l	d0
	beq	l245
	move.l	_DOSBase,a6
	move.l	d0,d1
	jsr	-90(a6)
	bra	l276
l245
	addq.l	#1,d4
	cmp.l	d4,d6
	bgt	l275
l276
	cmp.l	d4,d6
	bne	l247
	move.b	#0,84+l179
	lea	l248,a0
	lea	l180,a1
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.b	(a0)+,(a1)+
	moveq	#-1,d0
	bra	l213
l247
	pea	84+l179
	pea	l249
	lea	(352+l279,a7),a0
	move.l	a0,-(a7)
	jsr	_sprintf
	lea	(16+l279,a7),a0
	move.l	a0,-(a7)
	move.l	#370,-(a7)
	lea	(544+l279,a7),a0
	move.l	a0,-(a7)
	jsr	_nd_quote_path
	add.w	#24,a7
	tst.l	d0
	beq	l254
	pea	l177
	move.l	#340,-(a7)
	lea	(902+l279,a7),a0
	move.l	a0,-(a7)
	jsr	_nd_quote_path
	add.w	#12,a7
	tst.l	d0
	beq	l254
	lea	(344+l279,a7),a0
	move.l	a0,-(a7)
	move.l	#370,-(a7)
	lea	(1242+l279,a7),a0
	move.l	a0,-(a7)
	jsr	_nd_quote_path
	add.w	#12,a7
	tst.l	d0
	beq	l254
	lea	(894+l279,a7),a0
	move.l	a0,-(a7)
	lea	(1238+l279,a7),a0
	move.l	a0,-(a7)
	lea	(532+l279,a7),a0
	move.l	a0,-(a7)
	pea	l255
	pea	244+l179
	jsr	_sprintf
	move.l	_SysBase,a6
	jsr	-666(a6)
	move.l	d0,l179
	add.w	#20,a7
	beq	l254
	move.l	2044+l179,-(a7)
	move.l	_SysBase,a6
	move.l	#0,a1
	jsr	-294(a6)
	move.l	d0,a1
	move.l	a1,-(a7)
	pea	l258
	pea	4+l179
	jsr	_sprintf
	move.l	l179,a1
	move.l	#4+l179,(10,a1)
	move.l	_SysBase,a6
	jsr	-132(a6)
	move.l	l179,a1
	move.l	_SysBase,a6
	jsr	-354(a6)
	move.l	_SysBase,a6
	jsr	-138(a6)
	move.l	(2724+l279,a7),-(a7)
	move.l	#500,-(a7)
	lea	(1628+l279,a7),a1
	move.l	a1,-(a7)
	jsr	_nd_quote_path
	add.w	#28,a7
	tst.l	d0
	beq	l254
	pea	4+l179
	lea	(1608+l279,a7),a0
	move.l	a0,-(a7)
	pea	l261
	lea	(2116+l279,a7),a0
	move.l	a0,-(a7)
	jsr	_sprintf
	move.l	_DOSBase,a6
	move.l	#l262,d1
	move.l	#1005,d2
	jsr	-30(a6)
	move.l	d0,d5
	move.l	_DOSBase,a6
	move.l	#l263,d1
	move.l	#1006,d2
	jsr	-30(a6)
	move.l	d0,d3
	add.w	#16,a7
	tst.l	d5
	beq	l264
	tst.l	d3
	bne	l265
l264
	tst.l	d5
	beq	l268
	move.l	_DOSBase,a6
	move.l	d5,d1
	jsr	-36(a6)
l268
	tst.l	d3
	beq	l254
	move.l	_DOSBase,a6
	move.l	d3,d1
	jsr	-36(a6)
	bra	l254
l265
	move.l	#0,-(a7)
	move.l	#32768,-(a7)
	move.l	#-2147482637,-(a7)
	move.l	#1,-(a7)
	move.l	#-2147483613,-(a7)
	move.l	d3,-(a7)
	move.l	#-2147483614,-(a7)
	move.l	d5,-(a7)
	move.l	#-2147483615,-(a7)
	lea	(2140+l279,a7),a0
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
	bne	l272
	move.l	_DOSBase,a6
	move.l	d5,d1
	jsr	-36(a6)
	move.l	_DOSBase,a6
	move.l	d3,d1
	jsr	-36(a6)
	bra	l254
l272
	lea	l273,a0
	lea	l180,a1
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	moveq	#1,d0
	bra	l213
l254
	jsr	l201
	lea	l274,a0
	lea	l180,a1
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.w	(a0)+,(a1)+
	move.b	(a0)+,(a1)+
	moveq	#-1,d0
l213
l277	reg	a2/a6/d2/d3/d4/d5/d6
	movem.l	(a7)+,a2/a6/d2/d3/d4/d5/d6
l279	equ	28
	add.w	#2704,a7
	rts
	cnop	0,4
l224
	dc.b	87
	dc.b	97
	dc.b	105
	dc.b	116
	dc.b	105
	dc.b	110
	dc.b	103
	dc.b	32
	dc.b	102
	dc.b	111
	dc.b	114
	dc.b	32
	dc.b	97
	dc.b	110
	dc.b	111
	dc.b	116
	dc.b	104
	dc.b	101
	dc.b	114
	dc.b	32
	dc.b	100
	dc.b	111
	dc.b	119
	dc.b	110
	dc.b	108
	dc.b	111
	dc.b	97
	dc.b	100
	dc.b	0
	cnop	0,4
l227
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
	dc.b	0
	cnop	0,4
l230
	dc.b	99
	dc.b	117
	dc.b	114
	dc.b	108
	dc.b	32
	dc.b	110
	dc.b	111
	dc.b	116
	dc.b	32
	dc.b	102
	dc.b	111
	dc.b	117
	dc.b	110
	dc.b	100
	dc.b	59
	dc.b	32
	dc.b	99
	dc.b	104
	dc.b	101
	dc.b	99
	dc.b	107
	dc.b	32
	dc.b	69
	dc.b	100
	dc.b	105
	dc.b	116
	dc.b	32
	dc.b	115
	dc.b	101
	dc.b	116
	dc.b	116
	dc.b	105
	dc.b	110
	dc.b	103
	dc.b	115
	dc.b	0
	cnop	0,4
l236
	dc.b	85
	dc.b	115
	dc.b	101
	dc.b	32
	dc.b	97
	dc.b	32
	dc.b	102
	dc.b	117
	dc.b	108
	dc.b	108
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
	dc.b	112
	dc.b	97
	dc.b	116
	dc.b	104
	dc.b	0
	cnop	0,4
l241
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
l248
	dc.b	67
	dc.b	97
	dc.b	110
	dc.b	110
	dc.b	111
	dc.b	116
	dc.b	32
	dc.b	99
	dc.b	114
	dc.b	101
	dc.b	97
	dc.b	116
	dc.b	101
	dc.b	32
	dc.b	115
	dc.b	116
	dc.b	97
	dc.b	103
	dc.b	105
	dc.b	110
	dc.b	103
	dc.b	32
	dc.b	100
	dc.b	114
	dc.b	97
	dc.b	119
	dc.b	101
	dc.b	114
	dc.b	0
	cnop	0,4
l221
	dc.b	78
	dc.b	111
	dc.b	118
	dc.b	97
	dc.b	68
	dc.b	111
	dc.b	99
	dc.b	107
	dc.b	46
	dc.b	68
	dc.b	111
	dc.b	119
	dc.b	110
	dc.b	108
	dc.b	111
	dc.b	97
	dc.b	100
	dc.b	46
	dc.b	70
	dc.b	101
	dc.b	116
	dc.b	99
	dc.b	104
	dc.b	0
	cnop	0,4
l249
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
l255
	dc.b	37
	dc.b	115
	dc.b	32
	dc.b	45
	dc.b	45
	dc.b	100
	dc.b	105
	dc.b	115
	dc.b	97
	dc.b	98
	dc.b	108
	dc.b	101
	dc.b	32
	dc.b	45
	dc.b	45
	dc.b	102
	dc.b	97
	dc.b	105
	dc.b	108
	dc.b	32
	dc.b	45
	dc.b	45
	dc.b	108
	dc.b	111
	dc.b	99
	dc.b	97
	dc.b	116
	dc.b	105
	dc.b	111
	dc.b	110
	dc.b	32
	dc.b	45
	dc.b	45
	dc.b	112
	dc.b	114
	dc.b	111
	dc.b	116
	dc.b	111
	dc.b	32
	dc.b	61
	dc.b	104
	dc.b	116
	dc.b	116
	dc.b	112
	dc.b	44
	dc.b	104
	dc.b	116
	dc.b	116
	dc.b	112
	dc.b	115
	dc.b	32
	dc.b	45
	dc.b	45
	dc.b	112
	dc.b	114
	dc.b	111
	dc.b	116
	dc.b	111
	dc.b	45
	dc.b	114
	dc.b	101
	dc.b	100
	dc.b	105
	dc.b	114
	dc.b	32
	dc.b	61
	dc.b	104
	dc.b	116
	dc.b	116
	dc.b	112
	dc.b	44
	dc.b	104
	dc.b	116
	dc.b	116
	dc.b	112
	dc.b	115
	dc.b	32
	dc.b	45
	dc.b	45
	dc.b	99
	dc.b	111
	dc.b	110
	dc.b	110
	dc.b	101
	dc.b	99
	dc.b	116
	dc.b	45
	dc.b	116
	dc.b	105
	dc.b	109
	dc.b	101
	dc.b	111
	dc.b	117
	dc.b	116
	dc.b	32
	dc.b	51
	dc.b	48
	dc.b	32
	dc.b	45
	dc.b	45
	dc.b	109
	dc.b	97
	dc.b	120
	dc.b	45
	dc.b	116
	dc.b	105
	dc.b	109
	dc.b	101
	dc.b	32
	dc.b	54
	dc.b	48
	dc.b	48
	dc.b	32
	dc.b	45
	dc.b	45
	dc.b	112
	dc.b	114
	dc.b	111
	dc.b	103
	dc.b	114
	dc.b	101
	dc.b	115
	dc.b	115
	dc.b	45
	dc.b	98
	dc.b	97
	dc.b	114
	dc.b	32
	dc.b	45
	dc.b	45
	dc.b	111
	dc.b	117
	dc.b	116
	dc.b	112
	dc.b	117
	dc.b	116
	dc.b	32
	dc.b	37
	dc.b	115
	dc.b	32
	dc.b	45
	dc.b	45
	dc.b	32
	dc.b	37
	dc.b	115
	dc.b	0
	cnop	0,4
l258
	dc.b	78
	dc.b	111
	dc.b	118
	dc.b	97
	dc.b	68
	dc.b	111
	dc.b	99
	dc.b	107
	dc.b	46
	dc.b	68
	dc.b	111
	dc.b	119
	dc.b	110
	dc.b	108
	dc.b	111
	dc.b	97
	dc.b	100
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
l261
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
l262
	dc.b	78
	dc.b	73
	dc.b	76
	dc.b	58
	dc.b	0
	cnop	0,4
l263
	dc.b	78
	dc.b	73
	dc.b	76
	dc.b	58
	dc.b	0
	cnop	0,4
l273
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
	dc.b	0
	cnop	0,4
l274
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
	dc.b	100
	dc.b	111
	dc.b	119
	dc.b	110
	dc.b	108
	dc.b	111
	dc.b	97
	dc.b	100
	dc.b	101
	dc.b	114
	dc.b	0
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	cnop	0,4
l281
	movem.l	l292,-(a7)
	move.l	#-320+l177,d2
	moveq	#1,d1
	moveq	#1,d0
	cmp.l	l178,d0
	bge	l291
l290
	move.w	d1,d0
	muls.w	#320,d0
	lea	l177,a0
	move.l	d2,a1
	lea	(0,a0,d0.w),a2
	lea	(0,a1,d0.w),a3
	moveq.l	#79,d3
l295:
	move.b	(a2)+,(a3)+
	move.b	(a2)+,(a3)+
	move.b	(a2)+,(a3)+
	move.b	(a2)+,(a3)+
	dbra	d3,l295
	addq.l	#1,d1
	cmp.l	l178,d1
	blt	l290
l291
	tst.l	l178
	beq	l289
	subq.l	#1,l178
l289
l292	reg	a2/a3/d2/d3
	movem.l	(a7)+,a2/a3/d2/d3
l294	equ	16
	rts
; stacksize=16
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	cnop	0,4
l296
	sub.w	#180,a7
	movem.l	l341,-(a7)
	move.l	#1313101638,d6
	moveq	#2,d5
	move.l	#160+l177,d4
	moveq	#1,d3
	moveq	#0,d2
	tst.l	l179
	beq	l335
	move.l	l179,a0
	move.l	_SysBase,a6
	jsr	-372(a6)
	move.l	d0,a2
	tst.l	d0
	beq	l340
	move.l	#-1,a4
	move.w	#1992,a3
l301
	cmp.l	(20,a2),d6
	bne	l304
	cmp.w	(18,a2),a3
	beq	l305
l304
	tst.l	(14,a2)
	beq	l316
	move.l	_SysBase,a6
	move.l	a2,a1
	jsr	-378(a6)
	bra	l316
l305
	cmp.l	(24,a2),d3
	bne	l310
	tst.l	2048+l179
	beq	l312
	move.l	a4,(24,a2)
	bra	l313
l312
	lea	(32,a2),a0
	lea	244+l179,a1
	inline
	move.l	a0,d0
.l1
	move.b	(a1)+,(a0)+
	bne	.l1
	einline
	lea	(1832,a2),a0
	lea	84+l179,a1
	inline
	move.l	a0,d0
.l1
	move.b	(a1)+,(a0)+
	bne	.l1
	einline
	move.l	d3,2048+l179
l313
	move.l	_SysBase,a6
	move.l	a2,a1
	jsr	-378(a6)
	bra	l316
l310
	cmp.l	(24,a2),d5
	bne	l316
	pea	(1832,a2)
	pea	l317
	lea	(8+l343,a7),a0
	move.l	a0,-(a7)
	jsr	_sprintf
	add.w	#12,a7
	tst.l	(28,a2)
	beq	l319
	move.l	(28,a2),-(a7)
	pea	l320
	pea	l180
	jsr	_sprintf
	add.w	#12,a7
	bra	l329
l319
	move.l	d4,-(a7)
	jsr	l183
	addq.w	#4,a7
	tst.l	d0
	beq	l323
	lea	l324,a0
	lea	l180,a1
	moveq.l	#1,d0
l344:
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	dbra	d0,l344
	move.l	(a0)+,(a1)+
	move.w	(a0)+,(a1)+
	bra	l329
l323
	lea	(0+l343,a7),a0
	move.l	_DOSBase,a6
	move.l	a0,d1
	move.l	#160+l177,d2
	jsr	-78(a6)
	tst.l	d0
	beq	l327
	move.l	d4,-(a7)
	pea	l328
	pea	l180
	jsr	_sprintf
	add.w	#12,a7
	bra	l329
l327
	lea	l330,a0
	lea	l180,a1
	moveq.l	#1,d0
l345:
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	dbra	d0,l345
	move.w	(a0)+,(a1)+
l329
	pea	(1832,a2)
	jsr	l85
	move.l	_SysBase,a6
	move.l	a2,a1
	jsr	-690(a6)
	moveq	#1,d2
	addq.w	#4,a7
	bra	l340
l316
	move.l	l179,a0
	move.l	_SysBase,a6
	jsr	-372(a6)
	move.l	d0,a2
	tst.l	d0
	bne	l301
l340
	tst.l	d2
	beq	l332
	jsr	l201
	jsr	l281
	bra	l335
l332
	tst.l	2048+l179
	bne	l336
	jsr	l71
	sub.l	2044+l179,d0
	cmp.l	#200,d0
	bhi	l334
l336
	jsr	l71
	sub.l	2044+l179,d0
	cmp.l	#32500,d0
	bls	l335
l334
	lea	l338,a0
	lea	l180,a1
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.w	(a0)+,(a1)+
	move.b	(a0)+,(a1)+
	jsr	l201
	jsr	l281
l335
l341	reg	a2/a3/a4/a6/d2/d3/d4/d5/d6
	movem.l	(a7)+,a2/a3/a4/a6/d2/d3/d4/d5/d6
l343	equ	36
	add.w	#180,a7
	rts
	cnop	0,4
l317
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
l320
	dc.b	68
	dc.b	111
	dc.b	119
	dc.b	110
	dc.b	108
	dc.b	111
	dc.b	97
	dc.b	100
	dc.b	32
	dc.b	102
	dc.b	97
	dc.b	105
	dc.b	108
	dc.b	101
	dc.b	100
	dc.b	32
	dc.b	40
	dc.b	99
	dc.b	117
	dc.b	114
	dc.b	108
	dc.b	32
	dc.b	37
	dc.b	108
	dc.b	100
	dc.b	41
	dc.b	0
	cnop	0,4
l324
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
	dc.b	0
	cnop	0,4
l328
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
	dc.b	48
	dc.b	48
	dc.b	115
	dc.b	0
	cnop	0,4
l330
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
	dc.b	0
	cnop	0,4
l338
	dc.b	68
	dc.b	111
	dc.b	119
	dc.b	110
	dc.b	108
	dc.b	111
	dc.b	97
	dc.b	100
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
	dc.b	0
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	cnop	0,4
l346
	sub.w	#444,a7
	movem.l	l399,-(a7)
	moveq	#46,d6
	moveq	#48,d5
	moveq	#57,d4
	moveq	#-1,d3
	move.l	(448+l401,a7),a5
	lea	l349,a6
	move.l	a0,-(a7)
	lea	(a5),a0
	moveq.l	#1,d7
l402:
	move.b	(a6)+,(a0)+
	move.b	(a6)+,(a0)+
	move.b	(a6)+,(a0)+
	move.b	(a6)+,(a0)+
	dbra	d7,l402
	move.l	(a7)+,a0
	tst.l	l179
	bne	l351
	move.l	#65535,d0
	bra	l347
l351
	pea	84+l179
	pea	l352
	lea	(16+l401,a7),a2
	move.l	a2,-(a7)
	jsr	_sprintf
	move.l	_DOSBase,a6
	move.l	a2,d1
	moveq	#-2,d2
	jsr	-84(a6)
	move.l	d0,a4
	add.w	#12,a7
	move.l	a4,d1
	beq	l354
	move.l	_DOSBase,a6
	moveq	#2,d1
	moveq	#0,d2
	jsr	-228(a6)
	move.l	d0,a2
	move.l	a2,d1
	beq	l356
	move.l	_DOSBase,a6
	move.l	a4,d1
	move.l	a2,d2
	jsr	-102(a6)
	tst.l	d0
	beq	l358
	move.l	(124,a2),-(a7)
	pea	l359
	move.l	(456+l401,a7),-(a7)
	jsr	_sprintf
	add.w	#12,a7
l358
	move.l	_DOSBase,a6
	moveq	#2,d1
	move.l	a2,d2
	jsr	-234(a6)
l356
	move.l	_DOSBase,a6
	move.l	a4,d1
	jsr	-90(a6)
l354
	pea	84+l179
	pea	l360
	lea	(16+l401,a7),a2
	move.l	a2,-(a7)
	jsr	_sprintf
	pea	l361
	move.l	a2,-(a7)
	jsr	_fopen
	move.l	d0,d2
	add.w	#20,a7
	bne	l363
	move.l	#65535,d0
	bra	l347
l363
	move.l	#1,-(a7)
	move.l	#0,-(a7)
	move.l	d2,-(a7)
	jsr	_fseek
	move.l	d2,(12+l401,a7)
	add.w	#12,a7
	tst.l	d0
	bne	l397
	move.l	d2,-(a7)
	jsr	_ftell
	move.l	d2,(4+l401,a7)
	addq.w	#4,a7
	tst.l	d0
	ble	l397
	move.l	#-1,-(a7)
	cmp.l	#255,d0
	ble	l370
	move.l	d0,d7
	sub.l	#255,d7
	move.l	d7,(8+l401,a7)
	bra	l371
l370
	move.l	#0,(8+l401,a7)
l371
	move.l	(8+l401,a7),-(a7)
	move.l	d2,-(a7)
	jsr	_fseek
	move.l	d2,(12+l401,a7)
	add.w	#12,a7
	tst.l	d0
	bne	l397
	move.l	d2,-(a7)
	move.l	#255,-(a7)
	move.l	#1,-(a7)
	lea	(200+l401,a7),a2
	move.l	a2,-(a7)
	jsr	_fread
	move.b	#0,(0,a2,d0.w)
	move.l	a2,a3
	move.l	d2,(16+l401,a7)
	add.w	#16,a7
	tst.b	(a2)
	beq	l397
	moveq	#37,d2
	move.l	#10,a2
l372
	cmp.b	(a3),d2
	bne	l388
	move.l	a3,a1
	lea	(188+l401,a7),a0
	cmp.l	a3,a0
	bcc	l398
	cmp.b	(-1,a1),d5
	bgt	l382
	cmp.b	(-1,a1),d4
	bge	l396
l382
	cmp.b	(-1,a1),d6
	bne	l398
l396
	subq.l	#1,a1
	lea	(188+l401,a7),a0
	cmp.l	a1,a0
	bcc	l398
	cmp.b	(-1,a1),d5
	bgt	l385
	cmp.b	(-1,a1),d4
	bge	l396
l385
	cmp.b	(-1,a1),d6
	beq	l396
l398
	cmp.l	a1,a3
	bls	l388
	move.l	a2,-(a7)
	move.l	#0,-(a7)
	move.l	a1,-(a7)
	jsr	_strtol
	move.l	d0,d3
	add.w	#12,a7
l388
	addq.l	#1,a3
	tst.b	(a3)
	bne	l372
l397
	move.l	(0+l401,a7),d2
	move.l	d2,-(a7)
	jsr	_fclose
	addq.w	#4,a7
	tst.l	d3
	bge	l390
	move.l	#65535,a0
	bra	l391
l390
	moveq	#99,d7
	cmp.l	d3,d7
	bge	l393
	moveq	#99,d1
	bra	l394
l393
	move.l	d3,d1
l394
	move.l	d1,a0
l391
	move.l	a0,d0
l347
l399	reg	a2/a3/a4/a5/a6/d2/d3/d4/d5/d6/d7
	movem.l	(a7)+,a2/a3/a4/a5/a6/d2/d3/d4/d5/d6/d7
l401	equ	44
	add.w	#444,a7
	rts
	cnop	0,4
l359
	dc.b	37
	dc.b	108
	dc.b	100
	dc.b	32
	dc.b	98
	dc.b	121
	dc.b	116
	dc.b	101
	dc.b	115
	dc.b	0
	cnop	0,4
l349
	dc.b	48
	dc.b	32
	dc.b	98
	dc.b	121
	dc.b	116
	dc.b	101
	dc.b	115
	dc.b	0
	cnop	0,4
l352
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
l360
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
l361
	dc.b	114
	dc.b	98
	dc.b	0
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	cnop	0,4
l403
	sub.w	#208,a7
	movem.l	l424,-(a7)
	lea	(48+l426,a7),a2
	move.l	a2,a0
	lea	l182,a1
	inline
	move.l	a0,d0
.l1
	move.b	(a1)+,(a0)+
	bne	.l1
	einline
	move.l	_DOSBase,a6
	move.l	a2,d1
	move.l	#l412,d2
	move.l	#160,d3
	jsr	-882(a6)
	tst.w	d0
	bne	l411
	lea	l413,a0
	lea	(48+l426,a7),a1
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.b	(a0)+,(a1)+
l411
	move.l	#l414,(0+l426,a7)
	move.l	#l415,(4+l426,a7)
	move.l	#160,(8+l426,a7)
	move.l	#l416,(12+l426,a7)
	lea	(48+l426,a7),a0
	move.l	a0,(16+l426,a7)
	move.l	#160,(20+l426,a7)
	move.l	#l417,(24+l426,a7)
	move.l	#l180,(28+l426,a7)
	move.l	#0,(32+l426,a7)
	move.l	#l418,(36+l426,a7)
	tst.l	l178
	beq	l420
	move.l	#160+l177,d1
	bra	l421
l420
	move.l	#l422,d1
l421
	move.l	d1,(40+l426,a7)
	move.l	#0,(44+l426,a7)
	move.l	#3,-(a7)
	pea	l406
	move.l	#4,-(a7)
	lea	(12+l426,a7),a0
	move.l	a0,-(a7)
	pea	l423
	jsr	_nd_panel_open
	add.w	#20,a7
l424	reg	a2/a6/d2/d3
	movem.l	(a7)+,a2/a6/d2/d3
l426	equ	16
	add.w	#208,a7
	rts
	cnop	0,4
l407
	dc.b	81
	dc.b	117
	dc.b	101
	dc.b	117
	dc.b	101
	dc.b	32
	dc.b	85
	dc.b	82
	dc.b	76
	dc.b	0
	cnop	0,4
l408
	dc.b	67
	dc.b	108
	dc.b	101
	dc.b	97
	dc.b	114
	dc.b	32
	dc.b	119
	dc.b	97
	dc.b	105
	dc.b	116
	dc.b	105
	dc.b	110
	dc.b	103
	dc.b	0
	cnop	0,4
l409
	dc.b	67
	dc.b	108
	dc.b	111
	dc.b	115
	dc.b	101
	dc.b	0
	cnop	0,4
l412
	dc.b	100
	dc.b	111
	dc.b	119
	dc.b	110
	dc.b	108
	dc.b	111
	dc.b	97
	dc.b	100
	dc.b	0
	cnop	0,4
l413
	dc.b	82
	dc.b	65
	dc.b	77
	dc.b	58
	dc.b	100
	dc.b	111
	dc.b	119
	dc.b	110
	dc.b	108
	dc.b	111
	dc.b	97
	dc.b	100
	dc.b	0
	cnop	0,4
l414
	dc.b	85
	dc.b	82
	dc.b	76
	dc.b	0
	cnop	0,4
l415
	dc.b	0
	cnop	0,4
l416
	dc.b	83
	dc.b	97
	dc.b	118
	dc.b	101
	dc.b	32
	dc.b	97
	dc.b	115
	dc.b	0
	cnop	0,4
l417
	dc.b	83
	dc.b	116
	dc.b	97
	dc.b	116
	dc.b	117
	dc.b	115
	dc.b	0
	cnop	0,4
l418
	dc.b	65
	dc.b	99
	dc.b	116
	dc.b	105
	dc.b	118
	dc.b	101
	dc.b	0
	cnop	0,4
l422
	dc.b	78
	dc.b	111
	dc.b	110
	dc.b	101
	dc.b	0
	cnop	0,4
l423
	dc.b	78
	dc.b	111
	dc.b	118
	dc.b	97
	dc.b	68
	dc.b	111
	dc.b	99
	dc.b	107
	dc.b	32
	dc.b	68
	dc.b	111
	dc.b	119
	dc.b	110
	dc.b	108
	dc.b	111
	dc.b	97
	dc.b	100
	dc.b	0
	section	"DATA",data
	cnop	0,4
l406
	dc.l	l407
	dc.l	l408
	dc.l	l409
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	section	"CODE",code
	public	_main
	cnop	0,4
_main
	sub.w	#424,a7
	movem.l	l513,-(a7)
	move.l	(432+l515,a7),a3
	moveq	#0,d2
	move.l	#0,(0+l515,a7)
	moveq	#1,d4
	moveq	#3,d7
	cmp.l	(428+l515,a7),d7
	bne	l430
	move.l	(4,a3),a0
	lea	l432,a1
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
	bne	l430
	move.l	(8,a3),-(a7)
	jsr	l114
	addq.w	#4,a7
	bra	l427
l430
	moveq	#2,d7
	cmp.l	(428+l515,a7),d7
	ble	l434
	moveq	#20,d0
	bra	l427
l434
	move.l	_SysBase,a6
	move.l	#0,a1
	jsr	-294(a6)
	move.l	d0,a1
	move.l	#-1,(184,a1)
	move.l	(4,a3),-(a7)
	lea	(12+l515,a7),a1
	move.l	a1,-(a7)
	jsr	_nd_client_init
	addq.w	#8,a7
	tst.l	d0
	bne	l511
	moveq	#20,d0
	bra	l427
l511
	tst.l	d4
	beq	l442
	move.l	(4,a3),-(a7)
	jsr	l191
	moveq	#0,d4
	addq.w	#4,a7
l442
	jsr	l296
	tst.l	l178
	beq	l447
	tst.l	l179
	bne	l447
	move.l	(a3),-(a7)
	jsr	l212
	addq.w	#4,a7
	tst.l	d0
	bge	l447
	jsr	l281
l447
	lea	(332+l515,a7),a0
	move.l	a0,-(a7)
	jsr	l346
	move.l	d0,(8+l515,a7)
	addq.w	#4,a7
	tst.l	l179
	beq	l449
	lea	(332+l515,a7),a1
	bra	l450
l449
	lea	l180,a1
l450
	move.l	a1,-(a7)
	pea	l451
	lea	(388+l515,a7),a0
	move.l	a0,-(a7)
	jsr	_sprintf
	move.l	l178,-(a7)
	pea	l452
	lea	(432+l515,a7),a0
	move.l	a0,-(a7)
	jsr	_sprintf
	add.w	#24,a7
	tst.l	l178
	beq	l454
	lea	(412+l515,a7),a4
	bra	l455
l454
	lea	l456,a4
l455
	move.l	a4,-(a7)
	lea	(12+l515,a7),a2
	move.l	a2,-(a7)
	jsr	_nd_client_set_badge
	move.l	d0,d3
	move.l	#500,-(a7)
	move.l	(12+l515,a7),-(a7)
	move.l	(20+l515,a7),-(a7)
	lea	(400+l515,a7),a0
	move.l	a0,-(a7)
	pea	l457
	move.l	a2,-(a7)
	jsr	_nd_client_update_quiet
	or.l	d0,d3
	move.b	d3,d0
	and.b	#2,d0
	add.w	#32,a7
	bne	l512
	move.b	d3,d0
	and.b	#4,d0
	beq	l461
	moveq	#1,d4
l461
	move.b	d3,d0
	and.b	#1,d0
	beq	l466
	tst.l	d2
	beq	l465
	move.l	d2,-(a7)
	jsr	_nd_panel_raise
	addq.w	#4,a7
	bra	l466
l465
	jsr	l403
	move.l	d0,d2
l466
	tst.l	d2
	beq	l468
	pea	l180
	move.l	#2,-(a7)
	move.l	d2,-(a7)
	jsr	_nd_panel_set
	add.w	#12,a7
	tst.l	l178
	beq	l470
	move.l	#160+l177,d6
	bra	l471
l470
	move.l	#l472,d6
l471
	move.l	d6,-(a7)
	move.l	#3,-(a7)
	move.l	d2,-(a7)
	jsr	_nd_panel_set
	add.w	#12,a7
l468
	move.l	d2,-(a7)
	jsr	_nd_panel_event
	move.l	d0,(4+l515,a7)
	addq.w	#4,a7
	moveq	#100,d7
	cmp.l	d0,d7
	bne	l474
	move.l	#0,-(a7)
	move.l	d2,-(a7)
	jsr	_nd_panel_get
	move.l	d0,a5
	move.l	#1,-(a7)
	move.l	d2,-(a7)
	jsr	_nd_panel_get
	move.l	d0,a2
	add.w	#16,a7
	moveq	#8,d7
	cmp.l	l178,d7
	bgt	l476
	pea	l477
	move.l	d2,-(a7)
	jsr	_nd_panel_title
	addq.w	#8,a7
	bra	l506
l476
	move.l	a5,-(a7)
	jsr	l1
	addq.w	#4,a7
	tst.l	d0
	bne	l480
	pea	l481
	move.l	d2,-(a7)
	jsr	_nd_panel_title
	addq.w	#8,a7
	bra	l506
l480
	move.l	a2,d7
	beq	l483
	tst.b	(a2)
	beq	l483
	move.l	#58,-(a7)
	move.l	a2,-(a7)
	jsr	_strchr
	addq.w	#8,a7
	tst.l	d0
	beq	l483
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
	bcc	l483
	move.l	_DOSBase,a6
	move.l	a2,d1
	jsr	-870(a6)
	move.l	d0,a0
	tst.b	(a0)
	bne	l484
l483
	pea	l489
	move.l	d2,-(a7)
	jsr	_nd_panel_title
	addq.w	#8,a7
	bra	l506
l484
	move.l	a2,-(a7)
	jsr	l183
	addq.w	#4,a7
	tst.l	d0
	beq	l492
	pea	l493
	move.l	d2,-(a7)
	jsr	_nd_panel_title
	addq.w	#8,a7
	bra	l506
l492
	move.w	2+l178,d0
	muls.w	#320,d0
	lea	l177,a0
	add.w	d0,a0
	move.l	a5,a1
	inline
	move.l	a0,d0
.l1
	move.b	(a1)+,(a0)+
	bne	.l1
	einline
	move.w	2+l178,d0
	muls.w	#320,d0
	lea	160+l177,a0
	add.w	d0,a0
	move.l	a2,a1
	inline
	move.l	a0,d0
.l1
	move.b	(a1)+,(a0)+
	bne	.l1
	einline
	addq.l	#1,l178
	pea	l495
	move.l	#0,-(a7)
	move.l	d2,-(a7)
	jsr	_nd_panel_set
	pea	l496
	move.l	d2,-(a7)
	jsr	_nd_panel_title
	add.w	#20,a7
	bra	l506
l474
	moveq	#101,d7
	cmp.l	d0,d7
	bne	l499
	tst.l	l179
	beq	l501
	moveq	#1,d1
	bra	l502
l501
	moveq	#0,d1
l502
	move.l	d1,l178
	pea	l503
	move.l	d2,-(a7)
	jsr	_nd_panel_title
	addq.w	#8,a7
	bra	l506
l499
	moveq	#-1,d7
	cmp.l	d0,d7
	beq	l505
	moveq	#102,d7
	cmp.l	d0,d7
	bne	l506
l505
	move.l	d2,-(a7)
	jsr	_nd_panel_close
	moveq	#0,d2
	addq.w	#4,a7
l506
	move.l	d2,-(a7)
	jsr	_nd_panel_signals
	move.l	d0,-(a7)
	tst.l	l179
	beq	l509
	moveq	#50,d5
	bra	l510
l509
	move.l	#500,d5
l510
	move.l	d5,-(a7)
	lea	(20+l515,a7),a0
	move.l	a0,-(a7)
	jsr	_nd_client_wait_signals
	move.l	d0,(16+l515,a7)
	add.w	#16,a7
	bra	l511
l512
	move.l	d2,-(a7)
	jsr	_nd_panel_close
	jsr	l201
	lea	(12+l515,a7),a0
	move.l	a0,-(a7)
	jsr	_nd_client_close
	moveq	#0,d0
	addq.w	#8,a7
l427
l513	reg	a2/a3/a4/a5/a6/d2/d3/d4/d5/d6/d7
	movem.l	(a7)+,a2/a3/a4/a5/a6/d2/d3/d4/d5/d6/d7
l515	equ	44
	add.w	#424,a7
	rts
	cnop	0,4
l472
	dc.b	78
	dc.b	111
	dc.b	110
	dc.b	101
	dc.b	0
	cnop	0,4
l495
	dc.b	0
	cnop	0,4
l496
	dc.b	78
	dc.b	111
	dc.b	118
	dc.b	97
	dc.b	68
	dc.b	111
	dc.b	99
	dc.b	107
	dc.b	32
	dc.b	68
	dc.b	111
	dc.b	119
	dc.b	110
	dc.b	108
	dc.b	111
	dc.b	97
	dc.b	100
	dc.b	32
	dc.b	45
	dc.b	32
	dc.b	85
	dc.b	82
	dc.b	76
	dc.b	32
	dc.b	113
	dc.b	117
	dc.b	101
	dc.b	117
	dc.b	101
	dc.b	100
	dc.b	0
	cnop	0,4
l477
	dc.b	68
	dc.b	111
	dc.b	119
	dc.b	110
	dc.b	108
	dc.b	111
	dc.b	97
	dc.b	100
	dc.b	32
	dc.b	113
	dc.b	117
	dc.b	101
	dc.b	117
	dc.b	101
	dc.b	32
	dc.b	105
	dc.b	115
	dc.b	32
	dc.b	102
	dc.b	117
	dc.b	108
	dc.b	108
	dc.b	0
	cnop	0,4
l481
	dc.b	69
	dc.b	110
	dc.b	116
	dc.b	101
	dc.b	114
	dc.b	32
	dc.b	97
	dc.b	110
	dc.b	32
	dc.b	72
	dc.b	84
	dc.b	84
	dc.b	80
	dc.b	32
	dc.b	111
	dc.b	114
	dc.b	32
	dc.b	72
	dc.b	84
	dc.b	84
	dc.b	80
	dc.b	83
	dc.b	32
	dc.b	85
	dc.b	82
	dc.b	76
	dc.b	32
	dc.b	119
	dc.b	105
	dc.b	116
	dc.b	104
	dc.b	111
	dc.b	117
	dc.b	116
	dc.b	32
	dc.b	99
	dc.b	114
	dc.b	101
	dc.b	100
	dc.b	101
	dc.b	110
	dc.b	116
	dc.b	105
	dc.b	97
	dc.b	108
	dc.b	115
	dc.b	0
	cnop	0,4
l489
	dc.b	69
	dc.b	110
	dc.b	116
	dc.b	101
	dc.b	114
	dc.b	32
	dc.b	97
	dc.b	32
	dc.b	102
	dc.b	117
	dc.b	108
	dc.b	108
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
	dc.b	102
	dc.b	105
	dc.b	108
	dc.b	101
	dc.b	110
	dc.b	97
	dc.b	109
	dc.b	101
	dc.b	0
	cnop	0,4
l493
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
	dc.b	0
	cnop	0,4
l503
	dc.b	78
	dc.b	111
	dc.b	118
	dc.b	97
	dc.b	68
	dc.b	111
	dc.b	99
	dc.b	107
	dc.b	32
	dc.b	68
	dc.b	111
	dc.b	119
	dc.b	110
	dc.b	108
	dc.b	111
	dc.b	97
	dc.b	100
	dc.b	32
	dc.b	45
	dc.b	32
	dc.b	119
	dc.b	97
	dc.b	105
	dc.b	116
	dc.b	105
	dc.b	110
	dc.b	103
	dc.b	32
	dc.b	113
	dc.b	117
	dc.b	101
	dc.b	117
	dc.b	101
	dc.b	32
	dc.b	99
	dc.b	108
	dc.b	101
	dc.b	97
	dc.b	114
	dc.b	101
	dc.b	100
	dc.b	0
	cnop	0,4
l451
	dc.b	37
	dc.b	46
	dc.b	51
	dc.b	49
	dc.b	115
	dc.b	0
	cnop	0,4
l452
	dc.b	37
	dc.b	100
	dc.b	0
	cnop	0,4
l456
	dc.b	0
	cnop	0,4
l457
	dc.b	68
	dc.b	79
	dc.b	87
	dc.b	78
	dc.b	76
	dc.b	79
	dc.b	65
	dc.b	68
	dc.b	0
	cnop	0,4
l432
	dc.b	87
	dc.b	79
	dc.b	82
	dc.b	75
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
	dc.b	68
	dc.b	111
	dc.b	119
	dc.b	110
	dc.b	108
	dc.b	111
	dc.b	97
	dc.b	100
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
	public	___stack
	section	"DATA",data
	cnop	0,4
___stack
	dc.l	32768
	public	_SysBase
	public	_DOSBase
	public	_fopen
	public	_fclose
	public	_sprintf
	public	_fread
	public	_fseek
	public	_ftell
	public	_strtol
	public	_strchr
	public	_nd_client_init
	public	_nd_client_set_badge
	public	_nd_client_update_quiet
	public	_nd_client_wait_signals
	public	_nd_client_close
	public	_nd_plugin_option_strings
	public	_nd_panel_open
	public	_nd_panel_close
	public	_nd_panel_raise
	public	_nd_panel_signals
	public	_nd_panel_event
	public	_nd_panel_get
	public	_nd_panel_set
	public	_nd_panel_title
	public	_nd_quote_path
	cnop	0,4
l180
	dc.b	82
	dc.b	101
	dc.b	97
	dc.b	100
	dc.b	121
	dc.b	0
	ds.b	122
	section	"BSS",bss
	cnop	0,4
l177
	ds.b	2560
	cnop	0,4
l178
	ds.b	4
	cnop	0,4
l179
	ds.b	2052
	cnop	0,4
l181
	ds.b	96
	cnop	0,4
l182
	ds.b	96

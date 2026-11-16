	idnt	"plugin_src/aminet/nd_aminet.c"
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	section	"CODE",code
	cnop	0,4
l1
	movem.l	l17,-(a7)
	move.l	(8+l19,a7),a3
	move.l	(4+l19,a7),a2
	tst.b	(a2)
	beq	l16
	tst.b	(a3)
	beq	l16
l15
	moveq	#0,d0
	move.b	(a2)+,d0
	inline
	xref	___ctype
	lea	___ctype,a0
	btst	#0,(1,a0,d0.w)
	beq	.skip
	or.b	#$20,d0
.skip
	einline
	move.l	d0,a1
	moveq	#0,d0
	move.b	(a3)+,d0
	inline
	xref	___ctype
	lea	___ctype,a0
	btst	#0,(1,a0,d0.w)
	beq	.skip
	or.b	#$20,d0
.skip
	einline
	cmp.l	a1,d0
	beq	l9
	moveq	#0,d0
	bra	l2
l9
	tst.b	(a2)
	beq	l16
	tst.b	(a3)
	bne	l15
l16
	tst.b	(a2)
	bne	l13
	tst.b	(a3)
	bne	l13
	moveq	#1,d0
	bra	l14
l13
	moveq	#0,d0
l14
l2
l17	reg	a2/a3
	movem.l	(a7)+,a2/a3
l19	equ	8
	rts
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	cnop	0,4
l20
	movem.l	l31,-(a7)
	move.l	(4+l33,a7),a3
	move.l	(8+l33,a7),a2
	tst.b	(a2)
	beq	l30
l29
	tst.b	(a3)
	beq	l26
	moveq	#0,d0
	move.b	(a3)+,d0
	inline
	xref	___ctype
	lea	___ctype,a0
	btst	#0,(1,a0,d0.w)
	beq	.skip
	or.b	#$20,d0
.skip
	einline
	move.l	d0,a1
	moveq	#0,d0
	move.b	(a2)+,d0
	inline
	xref	___ctype
	lea	___ctype,a0
	btst	#0,(1,a0,d0.w)
	beq	.skip
	or.b	#$20,d0
.skip
	einline
	cmp.l	a1,d0
	beq	l27
l26
	moveq	#0,d0
	bra	l21
l27
	tst.b	(a2)
	bne	l29
l30
	moveq	#1,d0
l21
l31	reg	a2/a3
	movem.l	(a7)+,a2/a3
l33	equ	8
	rts
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	public	_nd_aminet_path_valid
	cnop	0,4
_nd_aminet_path_valid
	movem.l	l72,-(a7)
	moveq	#2,d6
	moveq	#46,d5
	move.l	(4+l74,a7),a2
	moveq	#0,d4
	moveq	#0,d3
	moveq	#0,d2
	move.l	a2,d7
	beq	l36
	tst.b	(a2)
	beq	l36
	pea	l40
	move.l	a2,-(a7)
	jsr	_strstr
	addq.w	#8,a7
	tst.l	d0
	beq	l37
l36
	moveq	#0,d0
	bra	l34
l37
	tst.b	(a2)
	beq	l71
	moveq	#0,d0
	move.l	#97,a0
l41
	move.b	(a2)+,d1
	addq.l	#1,d4
	cmp.l	d4,a0
	bhi	l45
	bra	l34
l45
	cmp.b	#47,d1
	bne	l47
	tst.l	d2
	beq	l48
	addq.l	#1,d3
	cmp.l	d3,d6
	bge	l49
l48
	bra	l34
l49
	moveq	#0,d2
	bra	l51
l47
	cmp.b	#65,d1
	bcs	l59
	cmp.b	#90,d1
	bls	l53
l59
	cmp.b	#97,d1
	bcs	l58
	cmp.b	#122,d1
	bls	l53
l58
	cmp.b	#48,d1
	bcs	l57
	cmp.b	#57,d1
	bls	l53
l57
	cmp.b	#95,d1
	beq	l53
	cmp.b	#45,d1
	beq	l53
	cmp.b	#43,d1
	beq	l53
	cmp.b	d1,d5
	beq	l53
	bra	l34
l53
	tst.l	d2
	bne	l64
	cmp.b	d1,d5
	bne	l64
	bra	l34
l64
	addq.l	#1,d2
l51
	tst.b	(a2)
	bne	l41
l71
	cmp.l	d3,d6
	bne	l68
	tst.l	d2
	ble	l68
	moveq	#1,d0
	bra	l69
l68
	moveq	#0,d0
l69
l34
l72	reg	a2/d2/d3/d4/d5/d6/d7
	movem.l	(a7)+,a2/d2/d3/d4/d5/d6/d7
l74	equ	28
	rts
	cnop	0,4
l40
	dc.b	46
	dc.b	46
	dc.b	0
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	public	_nd_aminet_encode_query
	cnop	0,4
_nd_aminet_encode_query
	movem.l	l102,-(a7)
	moveq	#127,d6
	moveq	#32,d5
	move.l	(12+l104,a7),d4
	move.l	(4+l104,a7),a3
	move.l	(8+l104,a7),a2
	moveq	#0,d1
	moveq	#0,d3
	move.l	a3,d7
	beq	l78
	move.l	a2,d7
	beq	l78
	tst.l	d4
	bne	l79
l78
	moveq	#0,d0
	bra	l75
l79
	move.b	#0,(a2)
	tst.b	(a3)
	beq	l101
	move.l	#81,a4
l82
	move.b	(a3)+,d2
	addq.l	#1,d3
	cmp.l	d3,a4
	bls	l85
	cmp.b	d2,d5
	bhi	l85
	cmp.b	d2,d6
	bne	l86
l85
	moveq	#0,d0
	bra	l75
l86
	moveq	#0,d0
	move.b	d2,d0
	lea	1+___ctype,a0
	add.l	d0,a0
	moveq	#7,d0
	and.b	(a0),d0
	beq	l90
	cmp.b	#128,d2
	bcc	l90
	moveq	#1,d0
	add.l	d1,d0
	cmp.l	d0,d4
	bhi	l93
	moveq	#0,d0
	bra	l75
l93
	lea	(a2,d1.l),a0
	addq.l	#1,d1
	move.b	d2,(a0)
	bra	l94
l90
	moveq	#3,d0
	add.l	d1,d0
	cmp.l	d0,d4
	bhi	l96
	moveq	#0,d0
	bra	l75
l96
	lea	(a2,d1.l),a0
	addq.l	#1,d1
	move.b	#37,(a0)
	moveq	#0,d0
	move.b	d2,d0
	asr.l	#4,d0
	lea	l77,a0
	lea	(a2,d1.l),a1
	addq.l	#1,d1
	move.b	(0,a0,d0.w),(a1)
	moveq	#0,d0
	move.b	d2,d0
	and.w	#15,d0
	lea	l77,a0
	lea	(a2,d1.l),a1
	addq.l	#1,d1
	move.b	(0,a0,d0.w),(a1)
l94
	tst.b	(a3)
	bne	l82
l101
	move.b	#0,(0,a2,d1.l)
	moveq	#2,d7
	cmp.l	d3,d7
	bls	l97
	moveq	#0,d0
	bra	l98
l97
	moveq	#1,d0
l98
l75
l102	reg	a2/a3/a4/d2/d3/d4/d5/d6/d7
	movem.l	(a7)+,a2/a3/a4/d2/d3/d4/d5/d6/d7
l104	equ	36
	rts
; stacksize=36
	cnop	0,4
l77
	dc.b	48
	dc.b	49
	dc.b	50
	dc.b	51
	dc.b	52
	dc.b	53
	dc.b	54
	dc.b	55
	dc.b	56
	dc.b	57
	dc.b	65
	dc.b	66
	dc.b	67
	dc.b	68
	dc.b	69
	dc.b	70
	dc.b	0
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	public	_nd_aminet_plain
	cnop	0,4
_nd_aminet_plain
	subq.w	#4,a7
	movem.l	l172,-(a7)
	move.l	(16+l174,a7),a5
	move.l	(12+l174,a7),a4
	move.l	(8+l174,a7),a2
	moveq	#0,d3
	moveq	#0,d4
	moveq	#0,d5
	move.l	a5,d7
	beq	l105
	tst.b	(a2)
	beq	l171
	moveq	#1,d0
	add.l	d3,d0
	cmp.l	d0,a5
	bls	l171
l170
	move.b	(a2)+,d2
	cmp.b	#60,d2
	bne	l114
	moveq	#1,d4
	bra	l110
l114
	tst.l	d4
	beq	l116
	cmp.b	#62,d2
	bne	l110
	moveq	#0,d4
	bra	l110
l116
	cmp.b	#38,d2
	bne	l122
	move.l	#59,-(a7)
	move.l	a2,-(a7)
	jsr	_strchr
	move.l	d0,a3
	addq.w	#8,a7
	move.l	a3,d7
	beq	l122
	move.l	a3,d0
	sub.l	a2,d0
	moveq	#12,d7
	cmp.l	d0,d7
	ble	l122
	move.l	a2,a0
	lea	l126,a1
	moveq	#4,d2
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
	bne	l125
	moveq	#38,d2
	bra	l157
l125
	move.l	a2,a0
	lea	l130,a1
	moveq	#3,d2
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
	bne	l129
	moveq	#60,d2
	bra	l157
l129
	move.l	a2,a0
	lea	l134,a1
	moveq	#3,d2
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
	bne	l133
	moveq	#62,d2
	bra	l157
l133
	move.l	a2,a0
	lea	l138,a1
	moveq	#5,d2
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
	bne	l137
	moveq	#34,d2
	bra	l157
l137
	move.l	a2,a0
	lea	l143,a1
	moveq	#5,d2
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
	beq	l140
	move.l	a2,a0
	lea	l144,a1
	moveq	#4,d2
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
	bne	l141
l140
	moveq	#39,d2
	bra	l157
l141
	move.l	a2,a0
	lea	l148,a1
	moveq	#5,d2
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
	bne	l147
	moveq	#32,d2
	bra	l157
l147
	cmp.b	#35,(a2)
	bne	l151
	move.l	#10,-(a7)
	lea	(4+l174,a7),a0
	move.l	a0,-(a7)
	pea	(1,a2)
	jsr	_strtoul
	move.l	d0,d1
	add.w	#12,a7
	cmp.l	(0+l174,a7),a3
	bne	l153
	moveq	#32,d7
	cmp.l	d1,d7
	bhi	l153
	moveq	#127,d7
	cmp.l	d1,d7
	bls	l153
	move.b	d1,d0
	moveq	#0,d6
	move.b	d0,d6
	bra	l154
l153
	moveq	#63,d6
l154
	move.b	d6,d2
	bra	l157
l151
	moveq	#63,d2
l157
	lea	(1,a3),a2
l122
	cmp.b	#32,d2
	bls	l158
	cmp.b	#127,d2
	beq	l158
	cmp.b	#128,d2
	bcs	l159
	cmp.b	#160,d2
	bcc	l159
l158
	tst.l	d3
	bne	l163
	moveq	#0,d0
	bra	l164
l163
	moveq	#1,d0
l164
	move.l	d0,d5
	bra	l110
l159
	tst.l	d5
	beq	l167
	moveq	#2,d0
	add.l	d3,d0
	cmp.l	d0,a5
	bls	l167
	lea	(a4,d3.l),a0
	addq.l	#1,d3
	move.b	#32,(a0)
l167
	moveq	#0,d5
	lea	(a4,d3.l),a0
	addq.l	#1,d3
	move.b	d2,(a0)
l110
	tst.b	(a2)
	beq	l171
	moveq	#1,d0
	add.l	d3,d0
	cmp.l	d0,a5
	bhi	l170
l171
	move.b	#0,(0,a4,d3.l)
l105
l172	reg	a2/a3/a4/a5/d2/d3/d4/d5/d6/d7
	movem.l	(a7)+,a2/a3/a4/a5/d2/d3/d4/d5/d6/d7
l174	equ	40
	addq.w	#4,a7
	rts
	cnop	0,4
l126
	dc.b	97
	dc.b	109
	dc.b	112
	dc.b	59
	dc.b	0
	cnop	0,4
l130
	dc.b	108
	dc.b	116
	dc.b	59
	dc.b	0
	cnop	0,4
l134
	dc.b	103
	dc.b	116
	dc.b	59
	dc.b	0
	cnop	0,4
l138
	dc.b	113
	dc.b	117
	dc.b	111
	dc.b	116
	dc.b	59
	dc.b	0
	cnop	0,4
l143
	dc.b	97
	dc.b	112
	dc.b	111
	dc.b	115
	dc.b	59
	dc.b	0
	cnop	0,4
l144
	dc.b	35
	dc.b	51
	dc.b	57
	dc.b	59
	dc.b	0
	cnop	0,4
l148
	dc.b	110
	dc.b	98
	dc.b	115
	dc.b	112
	dc.b	59
	dc.b	0
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	public	_nd_aminet_readme_path
	cnop	0,4
_nd_aminet_readme_path
	movem.l	l189,-(a7)
	move.l	(8+l191,a7),a3
	move.l	(4+l191,a7),a2
	move.l	a2,-(a7)
	jsr	_nd_aminet_path_valid
	addq.w	#4,a7
	tst.l	d0
	bne	l178
	moveq	#0,d0
	bra	l175
l178
	move.l	a2,a0
	inline
	move.l	a0,d0
.l1
	tst.b	(a0)+
	bne	.l1
	sub.l	a0,d0
	not.l	d0
	einline
	move.l	d0,d2
	move.l	a2,a0
	moveq	#46,d1
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
	beq	l180
	move.l	a1,d0
	sub.l	a2,d0
	bra	l181
l180
	move.l	d2,d0
l181
	move.l	d0,d3
	moveq	#7,d1
	cmp.l	d2,d1
	bcc	l183
	pea	l185
	pea	(-7,a2,d2.l)
	jsr	l1
	addq.w	#8,a7
	tst.l	d0
	beq	l183
	move.l	d2,d3
	subq.l	#7,d3
l183
	moveq	#8,d0
	add.l	d3,d0
	cmp.l	(12+l191,a7),d0
	bls	l187
	moveq	#0,d0
	bra	l175
l187
	move.b	#47,(a3)
	lea	(1,a3),a0
	move.l	a2,a1
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
	lea	l188,a1
	lea	(1,a3,d3.l),a4
	moveq.l	#1,d1
l192:
	move.b	(a1)+,(a4)+
	move.b	(a1)+,(a4)+
	move.b	(a1)+,(a4)+
	move.b	(a1)+,(a4)+
	dbra	d1,l192
	moveq	#1,d0
l175
l189	reg	a2/a3/a4/d2/d3
	movem.l	(a7)+,a2/a3/a4/d2/d3
l191	equ	20
	rts
	cnop	0,4
l185
	dc.b	46
	dc.b	116
	dc.b	97
	dc.b	114
	dc.b	46
	dc.b	103
	dc.b	122
	dc.b	0
	cnop	0,4
l188
	dc.b	46
	dc.b	114
	dc.b	101
	dc.b	97
	dc.b	100
	dc.b	109
	dc.b	101
	dc.b	0
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	cnop	0,4
l193
	sub.w	#100,a7
	movem.l	l222,-(a7)
	move.l	(112+l224,a7),a3
	pea	l199
	move.l	(108+l224,a7),-(a7)
	jsr	_strstr
	addq.w	#8,a7
	tst.l	d0
	beq	l194
	pea	l200
	move.l	(108+l224,a7),-(a7)
	jsr	_strstr
	addq.w	#8,a7
	tst.l	d0
	beq	l194
	pea	l201
	move.l	d0,-(a7)
	jsr	_strstr
	move.l	d0,a4
	addq.w	#8,a7
	move.l	a4,d1
	beq	l194
	addq.l	#7,a4
	move.l	#34,-(a7)
	move.l	a4,-(a7)
	jsr	_strchr
	move.l	d0,d3
	addq.w	#8,a7
	beq	l194
	move.l	d3,d4
	sub.l	a4,d4
	moveq	#97,d1
	cmp.l	d4,d1
	bls	l194
	lea	(0+l224,a7),a2
	move.l	a2,a0
	move.l	a4,a1
	move.l	d4,d2
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
	move.w	d4,d0
	move.b	#0,(0,a2,d0.w)
	move.l	a2,-(a7)
	jsr	_nd_aminet_path_valid
	addq.w	#4,a7
	tst.l	d0
	beq	l194
	lea	(10904,a3),a0
	addq.l	#1,(a0)
	move.l	(a0),d1
	cmp.l	(108+l224,a7),d1
	ble	l194
	moveq	#50,d1
	cmp.l	(10900,a3),d1
	ble	l194
	lea	(0+l224,a7),a1
	move.w	(10902,a3),d0
	muls.w	#218,d0
	lea	(a3,d0.w),a0
	inline
	move.l	a0,d0
.l1
	move.b	(a1)+,(a0)+
	bne	.l1
	einline
	pea	l213
	move.l	d3,-(a7)
	jsr	_strstr
	move.l	d0,a2
	addq.w	#8,a7
	move.l	a2,d0
	beq	l215
	move.l	#62,-(a7)
	move.l	a2,-(a7)
	jsr	_strchr
	move.l	d0,a2
	addq.w	#8,a7
	move.l	a2,d1
	beq	l215
	pea	l218
	addq.l	#1,a2
	move.l	a2,-(a7)
	jsr	_strstr
	move.l	d0,d3
	addq.w	#8,a7
	beq	l215
	move.l	d3,a0
	move.b	#0,(a0)
	move.l	#121,-(a7)
	move.w	(10902,a3),d0
	muls.w	#218,d0
	pea	(97,a3,d0.w)
	move.l	a2,-(a7)
	jsr	_nd_aminet_plain
	add.w	#12,a7
	bra	l219
l215
	move.w	(10902,a3),d0
	muls.w	#218,d0
	move.b	#0,(97,a3,d0.w)
l219
	move.w	(10902,a3),d0
	muls.w	#218,d0
	tst.b	(97,a3,d0.w)
	bne	l221
	move.l	#121,-(a7)
	move.w	(10902,a3),d0
	muls.w	#218,d0
	pea	(97,a3,d0.w)
	lea	(8+l224,a7),a0
	move.l	a0,-(a7)
	jsr	_nd_aminet_plain
	add.w	#12,a7
l221
	lea	(10900,a3),a0
	addq.l	#1,(a0)
l194
l222	reg	a2/a3/a4/d2/d3/d4
	movem.l	(a7)+,a2/a3/a4/d2/d3/d4
l224	equ	24
	add.w	#100,a7
	rts
	cnop	0,4
l199
	dc.b	112
	dc.b	107
	dc.b	103
	dc.b	95
	dc.b	114
	dc.b	111
	dc.b	119
	dc.b	0
	cnop	0,4
l200
	dc.b	110
	dc.b	97
	dc.b	109
	dc.b	101
	dc.b	95
	dc.b	99
	dc.b	111
	dc.b	108
	dc.b	0
	cnop	0,4
l201
	dc.b	104
	dc.b	114
	dc.b	101
	dc.b	102
	dc.b	61
	dc.b	34
	dc.b	47
	dc.b	0
	cnop	0,4
l213
	dc.b	104
	dc.b	114
	dc.b	101
	dc.b	102
	dc.b	61
	dc.b	34
	dc.b	47
	dc.b	112
	dc.b	97
	dc.b	99
	dc.b	107
	dc.b	97
	dc.b	103
	dc.b	101
	dc.b	47
	dc.b	0
	cnop	0,4
l218
	dc.b	60
	dc.b	47
	dc.b	97
	dc.b	62
	dc.b	0
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	public	_nd_aminet_search
	cnop	0,4
_nd_aminet_search
	sub.w	#12,a7
	movem.l	l273,-(a7)
	move.l	(16+l275,a7),a5
	move.l	(24+l275,a7),a4
	moveq	#0,d4
	moveq	#0,d5
	moveq	#0,d2
	move.l	#0,a3
	move.l	#0,a2
	moveq	#0,d3
	move.l	#0,(4+l275,a7)
	move.b	#0,(8+l275,a7)
	move.l	a5,d7
	beq	l229
	move.l	a4,d7
	beq	l229
	tst.l	(20+l275,a7)
	blt	l229
	move.l	d2,(0+l275,a7)
	cmp.l	#50000,(20+l275,a7)
	blt	l230
l229
	moveq	#0,d0
	bra	l225
l230
	move.l	a4,a0
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
	move.l	a5,-(a7)
	jsr	_fgetc
	move.l	d0,d6
	addq.w	#4,a7
	moveq	#-1,d0
	cmp.l	d6,d0
	beq	l272
	move.l	(0+l275,a7),d2
l234
	move.w	d3,d1
	lea	l228,a0
	add.w	d1,a0
	move.b	(a0),d1
	ext.w	d1
	ext.l	d1
	cmp.l	d6,d1
	bne	l238
	addq.l	#1,d3
	move.w	d3,d1
	lea	l228,a0
	tst.b	(0,a0,d1.w)
	bne	l241
	move.l	#1,a2
	moveq	#0,d3
	bra	l241
l238
	moveq	#70,d1
	cmp.l	d6,d1
	bne	l243
	moveq	#1,d0
	bra	l244
l243
	moveq	#0,d0
l244
	move.l	d0,d3
l241
	move.l	#3,-(a7)
	lea	(9+l275,a7),a0
	move.l	a0,-(a7)
	lea	(12+l275,a7),a0
	move.l	a0,-(a7)
	jsr	_memmove
	move.b	d6,(19+l275,a7)
	add.w	#12,a7
	tst.l	d5
	bne	l246
	cmp.b	#60,(5+l275,a7)
	bne	l246
	cmp.b	#116,(6+l275,a7)
	bne	l246
	cmp.b	#114,(7+l275,a7)
	bne	l246
	moveq	#1,d5
	moveq	#0,d2
	moveq	#3,d4
	lea	l250,a0
	lea	l227,a1
	move.w	(a0)+,(a1)+
	move.b	(a0)+,(a1)+
	bra	l257
l246
	tst.l	d5
	beq	l257
	moveq	#1,d1
	add.l	d4,d1
	cmp.l	#8192,d1
	bcc	l254
	move.b	d6,d1
	move.w	d4,d0
	addq.l	#1,d4
	lea	l227,a0
	move.b	d1,(0,a0,d0.w)
	bra	l255
l254
	moveq	#1,d2
l255
	lea	(4+l275,a7),a0
	lea	l258,a1
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
	bne	l257
	tst.l	d2
	bne	l260
	move.w	d4,d1
	lea	l227,a0
	move.b	#0,(0,a0,d1.w)
	pea	l263
	pea	l227
	jsr	_strstr
	addq.w	#8,a7
	tst.l	d0
	beq	l262
	move.l	#1,a3
l262
	move.l	a4,-(a7)
	move.l	(24+l275,a7),-(a7)
	pea	l227
	jsr	l193
	add.w	#12,a7
l260
	moveq	#0,d5
l257
	move.l	a5,-(a7)
	jsr	_fgetc
	move.l	d0,d6
	addq.w	#4,a7
	moveq	#-1,d1
	cmp.l	d6,d1
	bne	l234
l272
	moveq	#16,d1
	and.l	(24,a5),d1
	bne	l266
	tst.l	d5
	bne	l266
	move.l	a3,d7
	bne	l265
	move.l	a2,d7
	bne	l265
	tst.l	(10904,a4)
	ble	l266
l265
	moveq	#1,d0
	bra	l267
l266
	moveq	#0,d0
l267
l225
l273	reg	a2/a3/a4/a5/d2/d3/d4/d5/d6/d7
	movem.l	(a7)+,a2/a3/a4/a5/d2/d3/d4/d5/d6/d7
l275	equ	40
	add.w	#12,a7
	rts
	cnop	0,4
l250
	dc.b	60
	dc.b	116
	dc.b	114
	dc.b	0
	cnop	0,4
l263
	dc.b	115
	dc.b	111
	dc.b	114
	dc.b	116
	dc.b	95
	dc.b	108
	dc.b	105
	dc.b	110
	dc.b	107
	dc.b	95
	dc.b	110
	dc.b	97
	dc.b	109
	dc.b	101
	dc.b	0
	cnop	0,4
l258
	dc.b	60
	dc.b	47
	dc.b	116
	dc.b	114
	dc.b	0
	cnop	0,4
l228
	dc.b	70
	dc.b	111
	dc.b	117
	dc.b	110
	dc.b	100
	dc.b	32
	dc.b	48
	dc.b	32
	dc.b	109
	dc.b	97
	dc.b	116
	dc.b	99
	dc.b	104
	dc.b	105
	dc.b	110
	dc.b	103
	dc.b	0
	section	"BSS",bss
	cnop	0,4
l227
	ds.b	8192
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	section	"CODE",code
	public	_nd_aminet_details
	cnop	0,4
_nd_aminet_details
	sub.w	#512,a7
	movem.l	l315,-(a7)
	moveq	#10,d6
	moveq	#120,d5
	move.l	#512,d4
	move.l	(516+l317,a7),a5
	move.l	(520+l317,a7),a3
	move.l	a3,a0
	moveq	#0,d0
	move.l	#565,d2
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
	moveq	#1,d3
	move.l	a5,-(a7)
	move.l	d4,-(a7)
	lea	(8+l317,a7),a0
	move.l	a0,-(a7)
	jsr	_fgets
	add.w	#12,a7
	tst.l	d0
	beq	l314
l313
	move.l	#58,-(a7)
	lea	(4+l317,a7),a1
	move.l	a1,-(a7)
	jsr	_strchr
	move.l	d0,a4
	move.l	#0,a2
	moveq	#0,d2
	addq.w	#8,a7
	move.l	a4,d1
	bne	l283
	cmp.b	(0+l317,a7),d6
	beq	l314
	cmp.b	#13,(0+l317,a7)
	bne	l307
	bra	l314
l283
	move.b	#0,(a4)+
	pea	l289
	lea	(4+l317,a7),a1
	move.l	a1,-(a7)
	jsr	l1
	addq.w	#8,a7
	tst.l	d0
	beq	l288
	move.l	a3,a2
	moveq	#121,d2
	bra	l304
l288
	pea	l293
	lea	(4+l317,a7),a1
	move.l	a1,-(a7)
	jsr	l1
	addq.w	#8,a7
	tst.l	d0
	beq	l292
	lea	(202,a3),a2
	moveq	#121,d2
	bra	l304
l292
	pea	l297
	lea	(4+l317,a7),a1
	move.l	a1,-(a7)
	jsr	l1
	addq.w	#8,a7
	tst.l	d0
	beq	l296
	lea	(121,a3),a2
	moveq	#81,d2
	bra	l304
l296
	pea	l301
	lea	(4+l317,a7),a1
	move.l	a1,-(a7)
	jsr	l1
	addq.w	#8,a7
	tst.l	d0
	beq	l300
	lea	(323,a3),a2
	moveq	#121,d2
	bra	l304
l300
	pea	l305
	lea	(4+l317,a7),a1
	move.l	a1,-(a7)
	jsr	l1
	addq.w	#8,a7
	tst.l	d0
	beq	l304
	lea	(444,a3),a2
	moveq	#121,d2
l304
	move.l	a2,d1
	beq	l307
	move.l	d2,-(a7)
	move.l	a2,-(a7)
	move.l	a4,-(a7)
	jsr	_nd_aminet_plain
	add.w	#12,a7
l307
	move.l	d3,a1
	addq.l	#1,d3
	cmp.l	a1,d5
	ble	l314
	move.l	a5,-(a7)
	move.l	d4,-(a7)
	lea	(8+l317,a7),a1
	move.l	a1,-(a7)
	jsr	_fgets
	add.w	#12,a7
	tst.l	d0
	bne	l313
l314
	moveq	#16,d1
	and.l	(24,a5),d1
	bne	l311
	tst.b	(a3)
	beq	l311
	moveq	#1,d0
	bra	l312
l311
	moveq	#0,d0
l312
l315	reg	a2/a3/a4/a5/d2/d3/d4/d5/d6
	movem.l	(a7)+,a2/a3/a4/a5/d2/d3/d4/d5/d6
l317	equ	36
	add.w	#512,a7
	rts
	cnop	0,4
l289
	dc.b	83
	dc.b	104
	dc.b	111
	dc.b	114
	dc.b	116
	dc.b	0
	cnop	0,4
l293
	dc.b	65
	dc.b	117
	dc.b	116
	dc.b	104
	dc.b	111
	dc.b	114
	dc.b	0
	cnop	0,4
l297
	dc.b	86
	dc.b	101
	dc.b	114
	dc.b	115
	dc.b	105
	dc.b	111
	dc.b	110
	dc.b	0
	cnop	0,4
l301
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
	dc.b	0
	cnop	0,4
l305
	dc.b	82
	dc.b	101
	dc.b	113
	dc.b	117
	dc.b	105
	dc.b	114
	dc.b	101
	dc.b	115
	dc.b	0
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	public	_nd_aminet_http_header
	cnop	0,4
_nd_aminet_http_header
	subq.w	#4,a7
	movem.l	l405,-(a7)
	moveq	#32,d4
	moveq	#9,d3
	move.l	(12+l407,a7),a5
	moveq	#0,d5
	tst.l	(8+l407,a7)
	beq	l320
	tst.l	(16+l407,a7)
	beq	l320
	pea	l325
	move.l	(12+l407,a7),-(a7)
	jsr	l20
	addq.w	#8,a7
	tst.l	d0
	bne	l321
	pea	l326
	move.l	(12+l407,a7),-(a7)
	jsr	l20
	addq.w	#8,a7
	tst.l	d0
	bne	l321
l320
	moveq	#0,d0
	bra	l318
l321
	move.l	(16+l407,a7),a0
	moveq	#-1,d1
	move.l	d1,(a0)
	pea	l327
	move.l	(12+l407,a7),-(a7)
	jsr	_strstr
	move.l	d0,a4
	addq.w	#8,a7
	move.l	a4,d1
	bne	l329
	moveq	#0,d0
	bra	l318
l329
	addq.l	#2,a4
	tst.b	(a4)
	beq	l400
	move.l	a4,a0
	lea	l334,a1
	moveq	#2,d2
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
	beq	l400
l395
	moveq	#0,d2
	pea	l335
	move.l	a4,-(a7)
	jsr	_strstr
	move.l	d0,a3
	addq.w	#8,a7
	move.l	a3,d1
	bne	l337
	moveq	#0,d0
	bra	l318
l337
	pea	l340
	move.l	a4,-(a7)
	jsr	l20
	addq.w	#8,a7
	tst.l	d0
	beq	l339
	move.l	d5,d0
	addq.l	#1,d5
	tst.l	d0
	beq	l342
	moveq	#0,d0
	bra	l318
l342
	lea	(15,a4),a2
	cmp.l	a2,a3
	bls	l401
	cmp.b	(a2),d4
	beq	l396
	cmp.b	(a2),d3
	bne	l401
l396
	addq.l	#1,a2
	cmp.l	a2,a3
	bls	l401
	cmp.b	(a2),d4
	beq	l396
	cmp.b	(a2),d3
	beq	l396
l401
	cmp.l	a2,a3
	bne	l351
	moveq	#0,d0
	bra	l318
l351
	move.l	d5,(0+l407,a7)
	cmp.l	a2,a3
	bls	l402
	move.l	d5,(0+l407,a7)
	cmp.b	#48,(a2)
	blt	l402
	move.l	d5,(0+l407,a7)
	cmp.b	#57,(a2)
	bgt	l402
	moveq	#57,d6
	moveq	#48,d5
	moveq	#48,d4
l352
	move.b	(a2)+,d0
	ext.w	d0
	ext.l	d0
	move.l	d0,d3
	sub.l	d4,d3
	move.l	a5,d0
	moveq	#10,d1
	jsr	__divu
	cmp.l	d2,d0
	bcs	l357
	move.l	a5,d0
	moveq	#10,d1
	jsr	__divu
	cmp.l	d2,d0
	bne	l358
	move.l	a5,d0
	moveq	#10,d1
	jsr	__divu
	move.l	d1,d0
	cmp.l	d3,d0
	bcc	l358
l357
	moveq	#0,d0
	bra	l318
l358
	moveq	#10,d0
	move.l	d0,d1
	move.l	d2,d7
	swap	d1
	swap	d7
	mulu.w	d2,d1
	mulu.w	d0,d7
	mulu.w	d2,d0
	add.w	d7,d1
	swap	d1
	clr.w	d1
	add.l	d1,d0
	move.l	d3,d2
	add.l	d0,d2
	cmp.l	a2,a3
	bls	l402
	cmp.b	(a2),d5
	bgt	l402
	cmp.b	(a2),d6
	bge	l352
l402
	move.l	(0+l407,a7),d5
	moveq	#32,d4
	moveq	#9,d3
	cmp.l	a2,a3
	bls	l403
	cmp.b	(a2),d4
	beq	l398
	cmp.b	(a2),d3
	bne	l403
l398
	addq.l	#1,a2
	cmp.l	a2,a3
	bls	l403
	cmp.b	(a2),d4
	beq	l398
	cmp.b	(a2),d3
	beq	l398
l403
	cmp.l	a2,a3
	beq	l371
	moveq	#0,d0
	bra	l318
l371
	move.l	(16+l407,a7),a0
	move.l	d2,(a0)
	bra	l386
l339
	pea	l376
	move.l	a4,-(a7)
	jsr	l20
	addq.w	#8,a7
	tst.l	d0
	bne	l373
	pea	l377
	move.l	a4,-(a7)
	jsr	l20
	addq.w	#8,a7
	tst.l	d0
	beq	l386
l373
	move.l	#58,-(a7)
	move.l	a4,-(a7)
	jsr	_strchr
	move.l	d0,a0
	lea	(1,a0),a2
	addq.w	#8,a7
	cmp.l	a2,a3
	bls	l404
	cmp.b	(a2),d4
	beq	l399
	cmp.b	(a2),d3
	bne	l404
l399
	addq.l	#1,a2
	cmp.l	a2,a3
	bls	l404
	cmp.b	(a2),d4
	beq	l399
	cmp.b	(a2),d3
	beq	l399
l404
	move.l	a3,d0
	sub.l	a2,d0
	moveq	#8,d1
	cmp.l	d0,d1
	bne	l385
	pea	l388
	move.l	a2,-(a7)
	jsr	l20
	addq.w	#8,a7
	tst.l	d0
	bne	l386
l385
	moveq	#0,d0
	bra	l318
l386
	lea	(2,a3),a4
	tst.b	(a4)
	beq	l400
	move.l	a4,a0
	lea	l390,a1
	moveq	#2,d2
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
	bne	l395
l400
	move.l	a4,a0
	lea	l394,a1
	moveq	#2,d2
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
	bne	l391
	moveq	#1,d0
	bra	l392
l391
	moveq	#0,d0
l392
l318
l405	reg	a2/a3/a4/a5/d2/d3/d4/d5/d6/d7
	movem.l	(a7)+,a2/a3/a4/a5/d2/d3/d4/d5/d6/d7
l407	equ	40
	addq.w	#4,a7
	rts
	cnop	0,4
l388
	dc.b	105
	dc.b	100
	dc.b	101
	dc.b	110
	dc.b	116
	dc.b	105
	dc.b	116
	dc.b	121
	dc.b	0
	cnop	0,4
l335
	dc.b	13
	dc.b	10
	dc.b	0
	cnop	0,4
l340
	dc.b	67
	dc.b	111
	dc.b	110
	dc.b	116
	dc.b	101
	dc.b	110
	dc.b	116
	dc.b	45
	dc.b	76
	dc.b	101
	dc.b	110
	dc.b	103
	dc.b	116
	dc.b	104
	dc.b	58
	dc.b	0
	cnop	0,4
l376
	dc.b	84
	dc.b	114
	dc.b	97
	dc.b	110
	dc.b	115
	dc.b	102
	dc.b	101
	dc.b	114
	dc.b	45
	dc.b	69
	dc.b	110
	dc.b	99
	dc.b	111
	dc.b	100
	dc.b	105
	dc.b	110
	dc.b	103
	dc.b	58
	dc.b	0
	cnop	0,4
l377
	dc.b	67
	dc.b	111
	dc.b	110
	dc.b	116
	dc.b	101
	dc.b	110
	dc.b	116
	dc.b	45
	dc.b	69
	dc.b	110
	dc.b	99
	dc.b	111
	dc.b	100
	dc.b	105
	dc.b	110
	dc.b	103
	dc.b	58
	dc.b	0
	cnop	0,4
l325
	dc.b	72
	dc.b	84
	dc.b	84
	dc.b	80
	dc.b	47
	dc.b	49
	dc.b	46
	dc.b	48
	dc.b	32
	dc.b	50
	dc.b	48
	dc.b	48
	dc.b	32
	dc.b	0
	cnop	0,4
l326
	dc.b	72
	dc.b	84
	dc.b	84
	dc.b	80
	dc.b	47
	dc.b	49
	dc.b	46
	dc.b	49
	dc.b	32
	dc.b	50
	dc.b	48
	dc.b	48
	dc.b	32
	dc.b	0
	cnop	0,4
l327
	dc.b	13
	dc.b	10
	dc.b	0
	cnop	0,4
l334
	dc.b	13
	dc.b	10
	dc.b	0
	cnop	0,4
l390
	dc.b	13
	dc.b	10
	dc.b	0
	cnop	0,4
l394
	dc.b	13
	dc.b	10
	dc.b	0
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	public	_nd_aminet_archive_header
	cnop	0,4
_nd_aminet_archive_header
	movem.l	l451,-(a7)
	move.l	(12+l453,a7),a3
	move.l	(8+l453,a7),a2
	move.l	(4+l453,a7),a0
	moveq	#46,d1
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
	move.l	d0,d2
	beq	l410
	move.l	a2,d3
	bne	l411
l410
	moveq	#0,d0
	bra	l408
l411
	pea	l415
	move.l	d2,-(a7)
	jsr	l1
	addq.w	#8,a7
	tst.l	d0
	beq	l414
	moveq	#4,d3
	cmp.l	a3,d3
	bhi	l418
	cmp.b	#80,(a2)
	bne	l418
	cmp.b	#75,(1,a2)
	bne	l418
	cmp.b	#3,(2,a2)
	bne	l422
	cmp.b	#4,(3,a2)
	beq	l417
l422
	cmp.b	#5,(2,a2)
	bne	l418
	cmp.b	#6,(3,a2)
	bne	l418
l417
	moveq	#1,d0
	bra	l419
l418
	moveq	#0,d0
l419
	bra	l408
l414
	pea	l428
	move.l	d2,-(a7)
	jsr	l1
	addq.w	#8,a7
	tst.l	d0
	bne	l425
	pea	l429
	move.l	d2,-(a7)
	jsr	l1
	addq.w	#8,a7
	tst.l	d0
	beq	l426
l425
	moveq	#3,d3
	cmp.l	a3,d3
	bhi	l432
	cmp.b	#31,(a2)
	bne	l432
	cmp.b	#139,(1,a2)
	bne	l432
	cmp.b	#8,(2,a2)
	bne	l432
	move.l	#1,a0
	bra	l433
l432
	move.l	#0,a0
l433
	move.l	a0,d0
	bra	l408
l426
	pea	l439
	move.l	d2,-(a7)
	jsr	l1
	addq.w	#8,a7
	tst.l	d0
	bne	l436
	pea	l440
	move.l	d2,-(a7)
	jsr	l1
	addq.w	#8,a7
	tst.l	d0
	beq	l437
l436
	moveq	#7,d3
	cmp.l	a3,d3
	bhi	l443
	cmp.b	#45,(2,a2)
	bne	l443
	cmp.b	#108,(3,a2)
	bne	l443
	cmp.b	#104,(4,a2)
	beq	l446
	cmp.b	#122,(4,a2)
	bne	l443
l446
	cmp.b	#48,(5,a2)
	bcs	l443
	cmp.b	#57,(5,a2)
	bhi	l443
	cmp.b	#45,(6,a2)
	bne	l443
	moveq	#1,d1
	bra	l444
l443
	moveq	#0,d1
l444
	move.l	d1,d0
	bra	l408
l437
	moveq	#0,d0
l408
l451	reg	a2/a3/d2/d3
	movem.l	(a7)+,a2/a3/d2/d3
l453	equ	16
	rts
	cnop	0,4
l415
	dc.b	46
	dc.b	122
	dc.b	105
	dc.b	112
	dc.b	0
	cnop	0,4
l428
	dc.b	46
	dc.b	103
	dc.b	122
	dc.b	0
	cnop	0,4
l429
	dc.b	46
	dc.b	116
	dc.b	103
	dc.b	122
	dc.b	0
	cnop	0,4
l439
	dc.b	46
	dc.b	108
	dc.b	104
	dc.b	97
	dc.b	0
	cnop	0,4
l440
	dc.b	46
	dc.b	108
	dc.b	122
	dc.b	104
	dc.b	0
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	public	_nd_aminet_percent
	cnop	0,4
_nd_aminet_percent
	subq.w	#4,a7
	movem.l	l468,-(a7)
	move.l	(12+l470,a7),d2
	move.l	(8+l470,a7),a2
	moveq	#0,d4
	moveq	#100,d5
	tst.l	d2
	bne	l457
	moveq	#-1,d0
	bra	l454
l457
	cmp.l	a2,d2
	bhi	l459
	moveq	#100,d0
	bra	l454
l459
	move.l	d2,d0
	moveq	#100,d1
	jsr	__divu
	move.l	d0,(0+l470,a7)
	move.l	d2,d0
	moveq	#100,d1
	jsr	__divu
	move.l	d1,d0
	move.l	d0,d6
	cmp.l	d4,d5
	ble	l467
	move.l	#99,a3
l460
	move.l	d5,d1
	add.l	d4,d1
	addq.l	#1,d1
	move.l	d1,d0
	moveq	#2,d1
	jsr	__divs
	move.l	d0,d3
	move.l	d0,d2
	move.l	(0+l470,a7),d1
	move.l	d3,-(a7)
	move.l	d2,d7
	move.l	d1,d3
	swap	d7
	swap	d3
	mulu.w	d1,d7
	mulu.w	d2,d3
	mulu.w	d1,d2
	add.w	d3,d7
	swap	d7
	clr.w	d7
	add.l	d7,d2
	move.l	(a7)+,d3
	move.l	d0,d1
	move.l	d6,d7
	swap	d1
	swap	d7
	mulu.w	d6,d1
	mulu.w	d0,d7
	mulu.w	d6,d0
	add.w	d7,d1
	swap	d1
	clr.w	d1
	add.l	d1,d0
	add.l	a3,d0
	moveq	#100,d1
	jsr	__divu
	add.l	d2,d0
	cmp.l	a2,d0
	bhi	l464
	move.l	d3,d4
	bra	l465
l464
	move.l	d3,d5
	subq.l	#1,d5
l465
	cmp.l	d4,d5
	bgt	l460
l467
	move.l	d4,d0
l454
l468	reg	a2/a3/d2/d3/d4/d5/d6/d7
	movem.l	(a7)+,a2/a3/d2/d3/d4/d5/d6/d7
l470	equ	32
	addq.w	#4,a7
	rts
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	public	_nd_aminet_category_valid
	cnop	0,4
_nd_aminet_category_valid
	movem.l	l499,-(a7)
	moveq	#47,d6
	moveq	#95,d5
	move.l	(4+l501,a7),d1
	moveq	#0,d3
	moveq	#0,d4
	moveq	#0,d2
	move.l	d1,a2
	tst.l	d1
	beq	l473
	tst.b	(a2)
	beq	l473
	move.l	d1,a0
	inline
	move.l	a0,d0
.l1
	tst.b	(a0)+
	bne	.l1
	sub.l	a0,d0
	not.l	d0
	einline
	moveq	#40,d7
	cmp.l	d0,d7
	bcs	l473
	pea	l478
	move.l	d1,-(a7)
	jsr	_strstr
	addq.w	#8,a7
	tst.l	d0
	beq	l474
l473
	moveq	#0,d0
	bra	l471
l474
	tst.b	(a2)
	beq	l498
l497
	cmp.b	(a2),d6
	bne	l484
	tst.l	d2
	beq	l485
	move.l	d3,d0
	addq.l	#1,d3
	tst.l	d0
	beq	l486
l485
	moveq	#0,d0
	bra	l471
l486
	moveq	#0,d2
	bra	l492
l484
	moveq	#0,d0
	move.b	(a2),d0
	lea	1+___ctype,a1
	add.l	d0,a1
	moveq	#7,d0
	and.b	(a1),d0
	bne	l489
	cmp.b	(a2),d5
	bne	l490
l489
	addq.l	#1,d2
	addq.l	#1,d4
	bra	l492
l490
	moveq	#0,d0
	bra	l471
l492
	addq.l	#1,a2
	tst.b	(a2)
	bne	l497
l498
	tst.l	d2
	beq	l495
	tst.l	d4
	ble	l495
	moveq	#1,d0
	bra	l496
l495
	moveq	#0,d0
l496
l471
l499	reg	a2/d2/d3/d4/d5/d6/d7
	movem.l	(a7)+,a2/d2/d3/d4/d5/d6/d7
l501	equ	28
	rts
	cnop	0,4
l478
	dc.b	46
	dc.b	46
	dc.b	0
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	public	_nd_aminet_categories
	cnop	0,4
_nd_aminet_categories
	sub.w	#4196,a7
	movem.l	l552,-(a7)
	moveq	#47,d6
	move.l	(4208+l554,a7),a5
	move.l	a5,a0
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
	move.l	(4204+l554,a7),a0
	inline
	move.l	a0,d0
.l1
	tst.b	(a0)+
	bne	.l1
	sub.l	a0,d0
	not.l	d0
	einline
	move.l	d0,d5
	move.l	(4204+l554,a7),-(a7)
	jsr	_nd_aminet_category_valid
	addq.w	#4,a7
	tst.l	d0
	beq	l504
	move.l	#47,-(a7)
	move.l	(4208+l554,a7),-(a7)
	jsr	_strchr
	addq.w	#8,a7
	tst.l	d0
	beq	l505
l504
	moveq	#0,d0
	bra	l502
l505
	move.l	(4200+l554,a7),-(a7)
	move.l	#4096,-(a7)
	lea	(8+l554,a7),a0
	move.l	a0,-(a7)
	jsr	_fgets
	add.w	#12,a7
	tst.l	d0
	beq	l549
l546
	lea	(0+l554,a7),a4
	pea	l514
	move.l	a4,-(a7)
	jsr	_strstr
	move.l	d0,a4
	addq.w	#8,a7
	move.l	a4,d1
	beq	l550
l547
	addq.l	#6,a4
	cmp.b	(a4),d6
	bne	l516
	addq.l	#1,a4
l516
	move.l	a4,a0
	lea	l519,a1
	moveq	#5,d2
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
	bne	l518
	addq.l	#5,a4
	bra	l522
l518
	move.l	a4,a0
	lea	l523,a1
	moveq	#10,d2
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
	bne	l522
	add.l	#10,a4
l522
	move.l	#34,-(a7)
	move.l	a4,-(a7)
	jsr	_strchr
	move.l	d0,d3
	addq.w	#8,a7
	beq	l550
	move.l	d3,d4
	sub.l	a4,d4
	moveq	#97,d1
	cmp.l	d4,d1
	bhi	l527
	move.l	d3,a4
	bra	l513
l527
	lea	(4096+l554,a7),a2
	move.l	a2,a0
	move.l	a4,a1
	move.l	d4,d2
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
	move.w	d4,d1
	move.b	#0,(0,a2,d1.w)
	move.l	d3,a4
	move.l	a2,a0
	move.l	(4204+l554,a7),a1
	move.l	d5,d2
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
	bne	l513
	lea	(4096+l554,a7),a0
	move.w	d5,d1
	cmp.b	(0,a0,d1.w),d6
	bne	l513
	lea	(4096+l554,a7),a0
	move.l	a0,-(a7)
	jsr	_nd_aminet_category_valid
	addq.w	#4,a7
	tst.l	d0
	beq	l513
	moveq	#0,d2
	tst.l	(10900,a5)
	ble	l551
l548
	lea	(4096+l554,a7),a1
	move.w	d2,d1
	muls.w	#218,d1
	lea	(a5,d1.w),a0
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
	beq	l551
	addq.l	#1,d2
	cmp.l	(10900,a5),d2
	blt	l548
l551
	cmp.l	(10900,a5),d2
	blt	l513
	moveq	#50,d1
	cmp.l	(10900,a5),d1
	ble	l513
	lea	(4096+l554,a7),a3
	lea	(10900,a5),a2
	move.w	(2,a2),d1
	muls.w	#218,d1
	lea	(a5,d1.w),a0
	move.l	a3,a1
	inline
	move.l	a0,d0
.l1
	move.b	(a1)+,(a0)+
	bne	.l1
	einline
	move.w	(2,a2),d1
	muls.w	#218,d1
	lea	(a5,d1.w),a1
	lea	(97,a1),a0
	move.l	a3,a1
	inline
	move.l	a0,d0
.l1
	move.b	(a1)+,(a0)+
	bne	.l1
	einline
	addq.l	#1,(a2)
l513
	pea	l541
	move.l	a4,-(a7)
	jsr	_strstr
	move.l	d0,a4
	addq.w	#8,a7
	move.l	a4,d1
	bne	l547
l550
	move.l	(4200+l554,a7),-(a7)
	move.l	#4096,-(a7)
	lea	(8+l554,a7),a0
	move.l	a0,-(a7)
	jsr	_fgets
	add.w	#12,a7
	tst.l	d0
	bne	l546
l549
	move.l	(10900,a5),(10904,a5)
	move.l	(4200+l554,a7),a0
	moveq	#16,d1
	and.l	(24,a0),d1
	bne	l544
	tst.l	(10900,a5)
	ble	l544
	moveq	#1,d0
	bra	l545
l544
	moveq	#0,d0
l545
l502
l552	reg	a2/a3/a4/a5/d2/d3/d4/d5/d6
	movem.l	(a7)+,a2/a3/a4/a5/d2/d3/d4/d5/d6
l554	equ	36
	add.w	#4196,a7
	rts
	cnop	0,4
l519
	dc.b	116
	dc.b	114
	dc.b	101
	dc.b	101
	dc.b	47
	dc.b	0
	cnop	0,4
l523
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
	dc.b	0
	cnop	0,4
l514
	dc.b	104
	dc.b	114
	dc.b	101
	dc.b	102
	dc.b	61
	dc.b	34
	dc.b	0
	cnop	0,4
l541
	dc.b	104
	dc.b	114
	dc.b	101
	dc.b	102
	dc.b	61
	dc.b	34
	dc.b	0
	public	__divs
	public	__divu
	public	_memmove
	public	_strchr
	public	_strstr
	public	_strtoul
	public	___ctype
	public	_fgets
	public	_fgetc

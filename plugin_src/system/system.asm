	idnt	"plugin_src/system/system.c"
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	section	"CODE",code
	cnop	0,4
l1
	movem.l	l25,-(a7)
	move.l	(4+l27,a7),d2
	move.l	#128,d0
	and.l	d2,d0
	beq	l5
	move.l	#l7,d1
	bra	l6
l5
	moveq	#8,d0
	and.l	d2,d0
	beq	l9
	lea	l11,a0
	bra	l10
l9
	moveq	#4,d0
	and.l	d2,d0
	beq	l13
	move.l	#l15,d3
	bra	l14
l13
	moveq	#2,d0
	and.l	d2,d0
	beq	l17
	lea	l19,a1
	bra	l18
l17
	moveq	#1,d0
	and.l	d2,d0
	beq	l21
	move.l	#l23,d0
	bra	l22
l21
	move.l	#l24,d0
l22
	move.l	d0,a1
l18
	move.l	a1,d3
l14
	move.l	d3,a0
l10
	move.l	a0,d1
l6
	move.l	d1,d0
l25	reg	d2/d3
	movem.l	(a7)+,d2/d3
l27	equ	8
	rts
; stacksize=8
	cnop	0,4
l7
	dc.b	54
	dc.b	56
	dc.b	48
	dc.b	54
	dc.b	48
	dc.b	0
	cnop	0,4
l11
	dc.b	54
	dc.b	56
	dc.b	48
	dc.b	52
	dc.b	48
	dc.b	0
	cnop	0,4
l15
	dc.b	54
	dc.b	56
	dc.b	48
	dc.b	51
	dc.b	48
	dc.b	0
	cnop	0,4
l19
	dc.b	54
	dc.b	56
	dc.b	48
	dc.b	50
	dc.b	48
	dc.b	0
	cnop	0,4
l23
	dc.b	54
	dc.b	56
	dc.b	48
	dc.b	49
	dc.b	48
	dc.b	0
	cnop	0,4
l24
	dc.b	54
	dc.b	56
	dc.b	48
	dc.b	48
	dc.b	48
	dc.b	0
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	cnop	0,4
l28
	movem.l	l47,-(a7)
	moveq	#32,d5
	move.l	(12+l49,a7),d1
	moveq	#0,d3
	moveq	#0,d6
	tst.l	d1
	bne	l32
	moveq	#0,d0
	bra	l29
l32
	moveq	#63,d2
l45
	cmp.l	d2,d5
	bgt	l38
	move.l	d2,d0
	sub.l	d5,d0
	move.l	(4+l49,a7),d7
	lsr.l	d0,d7
	move.l	d7,d0
	moveq	#1,d4
	and.l	d0,d4
	move.l	d1,(12+l49,a7)
	bra	l39
l38
	move.l	(8+l49,a7),d0
	lsr.l	d2,d0
	moveq	#1,d4
	and.l	d0,d4
	move.l	d1,(12+l49,a7)
l39
	move.l	#2147483648,d0
	and.l	d3,d0
	move.l	d3,d1
	lsl.l	#1,d1
	or.l	d4,d1
	move.l	#4294967295,d3
	and.l	d1,d3
	tst.l	d0
	bne	l40
	move.l	(12+l49,a7),d1
	move.l	d1,(12+l49,a7)
	cmp.l	d3,d1
	bhi	l41
l40
	move.l	(12+l49,a7),d1
	move.l	d3,d0
	sub.l	d1,d0
	move.l	#4294967295,d3
	and.l	d0,d3
	cmp.l	d2,d5
	bgt	l44
	move.l	#4294967295,d0
	bra	l29
l44
	moveq	#1,d0
	lsl.l	d2,d0
	or.l	d0,d6
l41
	subq.l	#1,d2
	bge	l45
	move.l	d6,d0
l29
l47	reg	d2/d3/d4/d5/d6/d7
	movem.l	(a7)+,d2/d3/d4/d5/d6/d7
l49	equ	24
	rts
; stacksize=24
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	cnop	0,4
l50
	movem.l	l54,-(a7)
	move.l	(4+l56,a7),a2
	move.l	a2,d0
	moveq	#60,d1
	jsr	__divu
	move.l	d1,d0
	move.l	d0,-(a7)
	move.l	a2,d0
	moveq	#60,d1
	jsr	__divu
	moveq	#60,d1
	jsr	__divu
	move.l	d1,d0
	move.l	d0,-(a7)
	move.l	a2,d0
	move.l	#3600,d1
	jsr	__divu
	moveq	#24,d1
	jsr	__divu
	move.l	d1,d0
	move.l	d0,-(a7)
	move.l	a2,d0
	move.l	#86400,d1
	jsr	__divu
	move.l	d0,-(a7)
	pea	l53
	move.l	(28+l56,a7),-(a7)
	jsr	_sprintf
	add.w	#24,a7
l54	reg	a2
	movem.l	(a7)+,a2
l56	equ	4
	rts
	cnop	0,4
l53
	dc.b	37
	dc.b	108
	dc.b	117
	dc.b	32
	dc.b	100
	dc.b	32
	dc.b	37
	dc.b	48
	dc.b	50
	dc.b	108
	dc.b	117
	dc.b	58
	dc.b	37
	dc.b	48
	dc.b	50
	dc.b	108
	dc.b	117
	dc.b	58
	dc.b	37
	dc.b	48
	dc.b	50
	dc.b	108
	dc.b	117
	dc.b	0
	opt o+,ol+,op+,oc+,ot+,oj+,ob+,om+
	public	_main
	cnop	0,4
_main
	sub.w	#536,a7
	movem.l	l133,-(a7)
	move.l	(544+l135,a7),a4
	moveq	#0,d2
	move.l	#0,(8+l135,a7)
	moveq	#0,d6
	moveq	#1,d3
	move.l	#0,(16+l135,a7)
	moveq	#20,d7
	move.l	d7,(4+l135,a7)
	moveq	#2,d7
	cmp.l	(540+l135,a7),d7
	ble	l62
	moveq	#20,d0
	bra	l57
l62
	move.l	_SysBase,a6
	jsr	-666(a6)
	move.l	d0,(20+l135,a7)
	bne	l64
	moveq	#20,d0
	bra	l57
l64
	move.l	_SysBase,a6
	move.l	(20+l135,a7),a0
	moveq	#40,d0
	jsr	-654(a6)
	move.l	d0,a5
	tst.l	d0
	beq	l69
	move.l	_SysBase,a6
	lea	l68,a0
	moveq	#2,d0
	move.l	a5,a1
	moveq	#0,d1
	jsr	-444(a6)
	tst.b	d0
	bne	l69
	moveq	#1,d0
	move.l	d0,(16+l135,a7)
	move.l	(20,a5),_TimerBase
	move.l	(4,a4),-(a7)
	lea	(36+l135,a7),a0
	move.l	a0,-(a7)
	jsr	_nd_client_init
	addq.w	#8,a7
	tst.l	d0
	beq	l69
	move.l	_SysBase,a1
	moveq	#0,d0
	move.w	(296,a1),d0
	move.l	d0,a1
	move.l	a1,-(a7)
	jsr	l1
	move.l	d0,a3
	move.l	_SysBase,a6
	lea	l72,a1
	moveq	#0,d0
	jsr	-552(a6)
	move.l	d0,(8+l135,a7)
	addq.w	#4,a7
	beq	l74
	move.l	(4+l135,a7),a1
	moveq	#0,d7
	move.w	(22,a1),d7
	move.l	d7,a1
	move.l	a1,-(a7)
	move.l	(8+l135,a7),a1
	moveq	#0,d7
	move.w	(20,a1),d7
	move.l	d7,a1
	move.l	a1,-(a7)
	pea	l75
	lea	(404+l135,a7),a1
	move.l	a1,-(a7)
	jsr	_sprintf
	move.l	_SysBase,a6
	move.l	(20+l135,a7),a1
	jsr	-414(a6)
	move.l	a5,(16+l135,a7)
	move.l	d6,(28+l135,a7)
	add.w	#16,a7
	bra	l131
l74
	lea	l77,a1
	lea	(392+l135,a7),a6
	move.l	(a1)+,(a6)+
	move.l	(a1)+,(a6)+
	move.l	(a1)+,(a6)+
	move.l	a5,(0+l135,a7)
	move.l	d6,(12+l135,a7)
l131
	move.l	#1,a5
l76
	tst.l	d3
	beq	l83
	move.l	#16,-(a7)
	lea	(444+l135,a7),a0
	move.l	a0,-(a7)
	pea	l84
	pea	l85
	move.l	(4,a4),-(a7)
	jsr	_nd_plugin_option_string
	moveq	#0,d3
	add.w	#20,a7
l83
	lea	(24+l135,a7),a0
	move.l	_TimerBase,a6
	jsr	-60(a6)
	move.l	d0,-(a7)
	move.l	(32+l135,a7),-(a7)
	move.l	(32+l135,a7),-(a7)
	jsr	l28
	move.l	d0,a2
	lea	(468+l135,a7),a0
	move.l	a0,-(a7)
	move.l	a2,-(a7)
	jsr	l50
	lea	(460+l135,a7),a0
	lea	l89,a1
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
	add.w	#20,a7
	tst.l	d0
	bne	l87
	moveq	#0,d4
	bra	l88
l87
	lea	(440+l135,a7),a0
	lea	l93,a1
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
	bne	l91
	moveq	#1,d5
	bra	l92
l91
	lea	(440+l135,a7),a0
	lea	l97,a1
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
	bne	l95
	moveq	#2,d6
	bra	l96
l95
	move.l	(8+l135,a7),d0
	move.l	#250,d1
	jsr	__divu
	moveq	#3,d1
	jsr	__divu
	move.l	d1,d0
	move.l	d0,d6
l96
	move.l	d6,d5
l92
	move.l	d5,d4
l88
	move.l	d4,a0
	move.l	a0,(4+l135,a7)
	tst.l	d4
	bne	l99
	move.l	a3,-(a7)
	pea	l100
	lea	(512+l135,a7),a0
	move.l	a0,-(a7)
	jsr	_sprintf
	add.w	#12,a7
	bra	l105
l99
	cmp.l	a0,a5
	bne	l103
	lea	(392+l135,a7),a0
	move.l	a0,-(a7)
	pea	l104
	lea	(512+l135,a7),a0
	move.l	a0,-(a7)
	jsr	_sprintf
	add.w	#12,a7
	bra	l105
l103
	move.l	a2,d0
	moveq	#60,d1
	jsr	__divu
	moveq	#60,d1
	jsr	__divu
	move.l	d1,d0
	move.l	d0,-(a7)
	move.l	a2,d0
	move.l	#3600,d1
	jsr	__divu
	moveq	#24,d1
	jsr	__divu
	move.l	d1,d0
	move.l	d0,-(a7)
	move.l	a2,d0
	move.l	#86400,d1
	jsr	__divu
	move.l	d0,-(a7)
	pea	l106
	lea	(520+l135,a7),a0
	move.l	a0,-(a7)
	jsr	_sprintf
	add.w	#20,a7
l105
	move.l	#500,-(a7)
	move.l	(16+l135,a7),-(a7)
	move.l	#65535,-(a7)
	lea	(516+l135,a7),a0
	move.l	a0,-(a7)
	pea	l107
	lea	(52+l135,a7),a0
	move.l	a0,-(a7)
	jsr	_nd_client_update_quiet
	move.l	d0,d1
	move.b	d1,d0
	and.b	#2,d0
	add.w	#24,a7
	bne	l132
	move.b	d1,d0
	and.b	#4,d0
	beq	l111
	moveq	#1,d3
l111
	move.b	d1,d0
	and.b	#1,d0
	beq	l113
	tst.l	d2
	bne	l113
	move.l	#l115,(356+l135,a7)
	move.l	a3,(360+l135,a7)
	move.l	#0,(364+l135,a7)
	move.l	#l116,(368+l135,a7)
	lea	(392+l135,a7),a0
	move.l	a0,(372+l135,a7)
	move.l	#0,(376+l135,a7)
	move.l	#l117,(380+l135,a7)
	lea	(456+l135,a7),a0
	move.l	a0,(384+l135,a7)
	move.l	#0,(388+l135,a7)
	move.l	a5,-(a7)
	pea	l59
	move.l	#3,-(a7)
	lea	(368+l135,a7),a0
	move.l	a0,-(a7)
	pea	l118
	jsr	_nd_panel_open
	move.l	d0,d2
	add.w	#20,a7
	bra	l121
l113
	move.b	d1,d0
	and.b	#1,d0
	beq	l121
	move.l	d2,-(a7)
	jsr	_nd_panel_raise
	addq.w	#4,a7
l121
	tst.l	d2
	beq	l123
	lea	(456+l135,a7),a0
	move.l	a0,-(a7)
	move.l	#2,-(a7)
	move.l	d2,-(a7)
	jsr	_nd_panel_set
	add.w	#12,a7
l123
	move.l	d2,-(a7)
	jsr	_nd_panel_event
	addq.w	#4,a7
	moveq	#-1,d7
	cmp.l	d0,d7
	beq	l124
	moveq	#100,d7
	cmp.l	d0,d7
	bne	l125
l124
	move.l	d2,-(a7)
	jsr	_nd_panel_close
	moveq	#0,d2
	addq.w	#4,a7
l125
	move.l	d2,-(a7)
	jsr	_nd_panel_signals
	move.l	d0,-(a7)
	move.l	#50,-(a7)
	lea	(44+l135,a7),a0
	move.l	a0,-(a7)
	jsr	_nd_client_wait_signals
	move.l	d0,(28+l135,a7)
	move.l	(28+l135,a7),d0
	add.l	(24+l135,a7),d0
	move.l	#750,d1
	jsr	__divu
	move.l	d1,d0
	move.l	d0,(24+l135,a7)
	add.w	#16,a7
	bra	l76
l132
	move.l	(0+l135,a7),a5
	move.l	d2,-(a7)
	jsr	_nd_panel_close
	lea	(36+l135,a7),a0
	move.l	a0,-(a7)
	jsr	_nd_client_close
	move.l	#0,(12+l135,a7)
	addq.w	#8,a7
l69
	tst.l	(16+l135,a7)
	beq	l128
	move.l	_SysBase,a6
	move.l	a5,a1
	jsr	-450(a6)
l128
	move.l	a5,d7
	beq	l130
	move.l	_SysBase,a6
	move.l	a5,a0
	jsr	-660(a6)
l130
	move.l	_SysBase,a6
	move.l	(20+l135,a7),a0
	jsr	-672(a6)
	move.l	(4+l135,a7),d0
l57
l133	reg	a2/a3/a4/a5/a6/d2/d3/d4/d5/d6/d7
	movem.l	(a7)+,a2/a3/a4/a5/a6/d2/d3/d4/d5/d6/d7
l135	equ	44
	add.w	#536,a7
	rts
	cnop	0,4
l75
	dc.b	37
	dc.b	117
	dc.b	46
	dc.b	37
	dc.b	117
	dc.b	0
	cnop	0,4
l84
	dc.b	82
	dc.b	111
	dc.b	116
	dc.b	97
	dc.b	116
	dc.b	101
	dc.b	0
	cnop	0,4
l85
	dc.b	68
	dc.b	73
	dc.b	83
	dc.b	80
	dc.b	76
	dc.b	65
	dc.b	89
	dc.b	0
	cnop	0,4
l115
	dc.b	67
	dc.b	80
	dc.b	85
	dc.b	0
	cnop	0,4
l116
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
l117
	dc.b	85
	dc.b	112
	dc.b	116
	dc.b	105
	dc.b	109
	dc.b	101
	dc.b	0
	cnop	0,4
l118
	dc.b	78
	dc.b	111
	dc.b	118
	dc.b	97
	dc.b	68
	dc.b	111
	dc.b	99
	dc.b	107
	dc.b	32
	dc.b	83
	dc.b	121
	dc.b	115
	dc.b	116
	dc.b	101
	dc.b	109
	dc.b	0
	cnop	0,4
l89
	dc.b	67
	dc.b	80
	dc.b	85
	dc.b	0
	cnop	0,4
l93
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
l97
	dc.b	85
	dc.b	112
	dc.b	116
	dc.b	105
	dc.b	109
	dc.b	101
	dc.b	0
	cnop	0,4
l100
	dc.b	67
	dc.b	80
	dc.b	85
	dc.b	32
	dc.b	37
	dc.b	115
	dc.b	0
	cnop	0,4
l104
	dc.b	87
	dc.b	66
	dc.b	32
	dc.b	37
	dc.b	46
	dc.b	50
	dc.b	48
	dc.b	115
	dc.b	0
	cnop	0,4
l106
	dc.b	37
	dc.b	108
	dc.b	117
	dc.b	32
	dc.b	100
	dc.b	32
	dc.b	37
	dc.b	48
	dc.b	50
	dc.b	108
	dc.b	117
	dc.b	58
	dc.b	37
	dc.b	48
	dc.b	50
	dc.b	108
	dc.b	117
	dc.b	0
	cnop	0,4
l107
	dc.b	83
	dc.b	89
	dc.b	83
	dc.b	84
	dc.b	69
	dc.b	77
	dc.b	0
	cnop	0,4
l60
	dc.b	67
	dc.b	108
	dc.b	111
	dc.b	115
	dc.b	101
	dc.b	0
	cnop	0,4
l68
	dc.b	116
	dc.b	105
	dc.b	109
	dc.b	101
	dc.b	114
	dc.b	46
	dc.b	100
	dc.b	101
	dc.b	118
	dc.b	105
	dc.b	99
	dc.b	101
	dc.b	0
	cnop	0,4
l72
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
l77
	dc.b	85
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
	section	"DATA",data
	cnop	0,4
l59
	dc.l	l60
	public	_nd_version
	section	"CODE",code
	cnop	0,4
_nd_version
	dc.b	36
	dc.b	86
	dc.b	69
	dc.b	82
	dc.b	58
	dc.b	32
	dc.b	83
	dc.b	121
	dc.b	115
	dc.b	116
	dc.b	101
	dc.b	109
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
	public	_TimerBase
	section	"DATA",data
	cnop	0,4
_TimerBase
	dc.l	0
	public	___stack
	cnop	0,4
___stack
	dc.l	16384
	public	__divu
	public	_SysBase
	public	_sprintf
	public	_nd_client_init
	public	_nd_client_update_quiet
	public	_nd_client_wait_signals
	public	_nd_client_close
	public	_nd_plugin_option_string
	public	_nd_panel_open
	public	_nd_panel_close
	public	_nd_panel_raise
	public	_nd_panel_signals
	public	_nd_panel_event
	public	_nd_panel_set

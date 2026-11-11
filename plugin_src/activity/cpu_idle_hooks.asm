	idnt	"NovaDock CPU idle timing hooks"
	section	"CODE",code

	public	_nd_cpu_idle_launch
	public	_nd_cpu_idle_switch

	xref	_TimerBase
	xref	_nd_cpu_idle_ticks
	xref	_nd_cpu_idle_started
	xref	_nd_cpu_hook_eclock

; Exec calls these handlers while switching tasks. Preserve every register so
; the measurement is invisible to both the scheduler and the measured task.
_nd_cpu_idle_launch:
	movem.l	d0-d7/a0-a6,-(sp)
	move.l	_TimerBase,a6
	move.l	a6,d0
	tst.l	d0
	beq.s	.launch_done
	lea	_nd_cpu_hook_eclock,a0
	jsr	-60(a6)		; timer.device/ReadEClock
	move.l	_nd_cpu_hook_eclock+4,_nd_cpu_idle_started
.launch_done:
	movem.l	(sp)+,d0-d7/a0-a6
	rts

_nd_cpu_idle_switch:
	movem.l	d0-d7/a0-a6,-(sp)
	move.l	_TimerBase,a6
	move.l	a6,d0
	tst.l	d0
	beq.s	.switch_done
	lea	_nd_cpu_hook_eclock,a0
	jsr	-60(a6)		; timer.device/ReadEClock
	move.l	_nd_cpu_hook_eclock+4,d0
	sub.l	_nd_cpu_idle_started,d0
	add.l	d0,_nd_cpu_idle_ticks
.switch_done:
	movem.l	(sp)+,d0-d7/a0-a6
	rts

	end

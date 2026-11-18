        section code,code
        public _nd_warp_password_hook_entry
        xref _nd_warp_password_hook
_nd_warp_password_hook_entry:
        movem.l d1-d7/a0-a6,-(sp)
        move.l a1,-(sp)
        move.l a2,-(sp)
        move.l a0,-(sp)
        jsr _nd_warp_password_hook
        lea 12(sp),sp
        movem.l (sp)+,d1-d7/a0-a6
        rts

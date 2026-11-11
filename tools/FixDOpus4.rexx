/* NovaDock Directory Opus 4 display repair.
 * Creates a normal saved configuration on the Workbench screen, then
 * restarts DOpus with that configuration.
 */
options results

address command 'Assign DOpus: SYS:DirOpus4'
address command 'Run >NIL: DOpus:DirectoryOpus -cDOpus:S/DirectoryOpus.CFG'

port = ''
do tries = 1 to 100 while port = ''
    if show('P', 'DOPUS.1') then port = 'DOPUS.1'
    else if show('P', 'DOPUS') then port = 'DOPUS'
    else address command 'Wait 1'
end

if port = '' then do
    say 'Directory Opus did not publish its ARexx port.'
    exit 10
end

address value port
'Modify ScreenMode 1'
'SaveConfig DOpus:S/DirectoryOpus.CFG'
'Quit FORCE'

address command 'Wait 2'
address command 'Run >NIL: DOpus:DirectoryOpus -cDOpus:S/DirectoryOpus.CFG'
exit 0

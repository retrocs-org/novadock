/* NovaDock Script tile example.
 *
 * The tile runs this script as  ScriptTile.rexx UPDATE <id>  at its refresh
 * interval and as  ScriptTile.rexx CLICK <id>  when clicked.  Whatever the
 * script returns becomes the tile:
 *
 *   value
 *   caption|value
 *   caption|value|meter          meter 0-100, empty or - for none
 *   caption|value|meter|badge    empty badge removes it
 *
 * The default host is NOVADOCK, so dock commands work directly, e.g.
 *   'SETBADGE' id '"!"'
 */
PARSE ARG action id .

IF action = 'CLICK' THEN
    RETURN 'DATE|'DATE('E')

/* How much of the day has passed, as the tile's meter. */
seconds = TIME('S')
RETURN 'TIME|'LEFT(TIME(),5)'|'(seconds * 100) % 86400

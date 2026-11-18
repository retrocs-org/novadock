# CALC plugin

Add CALC from Add Item → Plugin and click its tile to open the native calculator.
Enter an expression and press Return or Evaluate. The result, hexadecimal and
binary forms appear below it. Copy result uses the Amiga clipboard; History
opens the latest 32 expressions and results, with Reuse and Clear controls.
History is kept for the current plugin session.

Expressions support decimal fractions, `0x` hexadecimal, `0b` binary,
parentheses, unary `+`, `-`, `~`, arithmetic `+ - * / %`, bitwise `& ^ |`
and logical shifts `<< >>`. Arithmetic precedence follows C. Bitwise operations,
remainder and shifts require integers; shift counts are 0–31. Negative integers
use a 32-bit two's-complement representation for hex/binary conversion.

Results are limited to -2147483648 through 4294967295. Decimal output rounds to
six fractional digits and removes trailing zeroes. Fractional results show
Integer only in the hex/binary fields. Division by zero, overflow, malformed
input and excessive nesting produce an error without executing anything.
Input is limited to 159 characters, 64 tokens and 16 nested primary expressions.

CALC uses the operating system's mathieeedoubbas.library, so it does not require
an FPU. Calculations run only when requested. Clipboard writes run in a separate
bounded worker, allowing the dock and calculator to remain responsive if the
clipboard owner is slow. The artwork ships in Calc.assets beside the plugin.

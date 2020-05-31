# exports symbols name_data and name_size
.macro defdata name filename
    .section .rodata
    .global \name\()_data
    .type   \name\()_data, @object
    .align  4
\name\()_data:# meaningful space
    .incbin "\filename"
\name\()_end:# meaningful space
    .global \name\()_size
    .type   \name\()_size, @object
    .align  4
\name\()_size:# meaningful space
    .int    \name\()_end - \name\()_data
.endm

s/warning: \(control reaches end of non-void function\|too few arguments for format\|too many arguments for format\|cannot pass objects of non-POD type `[^']\+' through `[^']\+'; call will abort at runtime\|[a-z ]\+ format, [a-z ]\+ arg (arg [0-9]\+)\)$/error: \1/
s/error: \(unsigned int format, pointer arg (arg [0-9]\+)\)/warning: \1/

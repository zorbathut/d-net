s/warning: \(control reaches end of non-void function\|no return statement in function returning non-void\|too few arguments for format\|too many arguments for format\|cannot pass objects of non-POD type `[^']\+' through `[^']\+'; call will abort at runtime\|[a-z ]\+ format, [a-z ]\+ arg (arg [0-9]\+)\)$/error: \1/
s/error: \(unsigned int format, pointer arg (arg [0-9]\+)\)/warning: \1/

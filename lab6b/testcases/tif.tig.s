BEGIN function
L17
mov $0x0, d
mov 0x0(d), d
mov d, d
mov $0x4, d
push d
mov $0x9, n
push n
mov $0x0, p
mov 0x0(p), o
push o
call l
mov l, f
push f
push d
call v
jmp y
L16

END function

BEGIN function
L19
mov $0x0, �
mov 0x0(�), �
mov $0x4, �
mov 0x4(�), �
cmp �, �
jg L13
L14
mov $0x4, �
mov 0x4(�), �
mov �, e
L15
jmp �
L13
mov $0x0, �
mov 0x0(�), �
mov �, e
jmp �
L18

END function


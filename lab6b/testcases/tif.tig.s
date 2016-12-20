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
mov $0x0, Å
mov 0x0(Å), Ä
mov $0x4, É
mov 0x4(É), Ç
cmp Ä, Ç
jg L13
L14
mov $0x4, Ö
mov 0x4(Ö), Ñ
mov Ñ, e
L15
jmp à
L13
mov $0x0, ä
mov 0x0(ä), â
mov â, e
jmp å
L18

END function


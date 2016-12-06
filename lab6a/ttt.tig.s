BEGIN function
L20mov $0x2, 103mov $0x0, 105mov 0x0(105), 104mov 103, 0x4(104)jmp L19L19
END function

BEGIN function
L22L15mov $0x1, 109mov 109, 102mov $0x4, 112mov 0x4(112), 111mov $0x3, 113cmp 111, 113jl L17L18mov $0x0, 114mov 114, 102L17mov $0x0, 117cmp 102, 117je L14L16mov $0x4, 118mov $0x0, 120mov 0x0(120), 119mov 118, 0x4(119)jmp L15L14jmp L21L21
END function


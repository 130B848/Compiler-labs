BEGIN function
L12mov $0x0, 106push 106call initRecordmov initRecord, 100mov $0x0, 114mov 0x0(114), 113add $0x4, 112mov 112, 103mov $0x0, 118push 118mov $0xa, 119push 119call initArraymov initArray, 102mov 102, (103)mov $0x4, 124mov 0x4(124), 123jmp L11L11
END function


.text
target:
addi $v0, $zero, 111
addi $a0, $zero, 0
syscall
addi $v0, $zero, 17
addi $a0, $zero, 0
syscall
j target
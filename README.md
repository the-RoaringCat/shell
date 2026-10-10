## A Shell
There should be a lot of bugs, that I have not fixed or I did not realise.

# How to use
On your linux
```
$ gcc wish.c -o wish
$ ./wish
```

# Built in command
```
exit
echo
cd
```

# Parse
The user input is first tokenized. For example: 
```
capoo> abc "123" x > y < z | alpha & beta \>escaped a\b a"b" "a"b
tarray len = 15
tarray cap = 16
Token 0: type: WORD value: abc
Token 1: type: WORD value: 123
Token 2: type: WORD value: x
Token 3: type: REDIR_OUT value: (null)
Token 4: type: WORD value: y
Token 5: type: REDIR_IN value: (null)
Token 6: type: WORD value: z
Token 7: type: PIPE value: (null)
Token 8: type: WORD value: alpha
Token 9: type: PARALLEL value: (null)
Token 10: type: WORD value: beta
Token 11: type: WORD value: >escaped
Token 12: type: WORD value: ab
Token 13: type: WORD value: ab
Token 14: type: WORD value: ab
```

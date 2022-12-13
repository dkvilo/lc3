.ORIG x3000
	lea r0, hello
	puts
	HALT
hello:	.STRINGZ "Hello, World"
	.END

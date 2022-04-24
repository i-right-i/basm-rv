# Start Comment with pound/number sign.

@Constant [1A]			# Use the @name to defines constant only in hexdecimal
@constant [2a]			# Constants are case sensetive, hexdecimal is not case sensetive.
						# Constants Must be defined before used
						
:Start:					# Creates a Label - Labels are just named address offsets.
						
						
ADD 	x0,x0,x0		# For instance this NOP instructions address is equal to :Start:
ADDI	x1,x0,@constant 	# Add's Immedant value of constant to Register x1
BEQ		x0,x1,>Start	# If x1 = zero jump to :Start:  
ADD 	x1,x2,x3		# x1 = x2 + x3 

jalr	x3,x0,>Start	# Jump to :Start: create infinate loop

$	[deadbeef]			# Use dollar sign to store data, use caution not to read as OP code.
$	@Constants			# Constants work here as well

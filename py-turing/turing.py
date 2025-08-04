from enum import Enum
from collections import defaultdict

Action = Enum('Action', [('R',0),('L',1)])

class Turing():
	def __init__(self):
		self.sHALT = 0
		self.state = 0
		self.tape_index = 0

		self.transitions = defaultdict(lambda: defaultdict(lambda: (0,0,0)))
		self.tape = defaultdict(lambda: 0)

	def step(self):
		#print(f"state:{self.state}")
		if self.state == self.sHALT:
			return True

		tape_symbol = self.tape[self.tape_index]
		transition_entry = self.transitions[self.state][tape_symbol]
		match transition_entry:
			case (next_state, write_character, action):
				self.tape[self.tape_index] = write_character
				self.state = next_state

				# go left
				if action == Action.L:
					self.tape_index -= 1
				else: # else go right
					self.tape_index += 1
				#print(f"state:{self.state}\n")
				return self.state == self.sHALT
def busy_beaver_5_state_2_symbol():
	turing = Turing()

	def _ready_tape():
		for i in range(12289):
			turing.tape[i]
	
	_ready_tape()
	
	R = Action.R
	L = Action.L

	#		 next,write,action
	turing.transitions[1][0] = (2,1,R)
	turing.transitions[1][1] = (3,1,L)

	turing.transitions[2][0] = (3,1,R)
	turing.transitions[2][1] = (2,1,R)

	turing.transitions[3][0] = (4,1,R)
	turing.transitions[3][1] = (5,0,L)

	turing.transitions[4][0] = (1,1,L)
	turing.transitions[4][1] = (4,1,L)

	turing.transitions[5][0] = (0,1,R)
	turing.transitions[5][1] = (1,0,L)

	turing.sHALT = 0
	turing.state = 1

	init_tape = ""
	#init_tape = "111011000"
	counter = 0
	for c in init_tape:
		if c == '0':
			turing.tape[counter] = 0
		elif c == '1':
			turing.tape[counter] = 1
		counter += 1

	counter = 0
	print("turing machine busy beaver 5 states 2 symbols")
	#print(f"init tape: {turing.tape}")
	while not turing.step():
		counter += 1
		#print(f"state: {turing.state}, tape: ", end="")
		#for thing in turing.tape:
		#	print(f"{turing.tape[thing]}",end="")
		#print("")
	print(f"halted after {counter} steps, tape is this long: {len(turing.tape)}")

busy_beaver_5_state_2_symbol()
#import cProfile
#cProfile.run('busy_beaver_5_state_2_symbol()')

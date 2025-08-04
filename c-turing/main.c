#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STATES 6

#define TRUE 1
#define FALSE 0
#define TAPE_SIZE 13000

#define RIGHT 0
#define LEFT 1

#define m_SET_TRANSITION(table, state, symbol, next_state, write, action) table[state][symbol] = (struct turing_transition) {next_state,write,action}

struct turing_transition{
	int next_state;
	int write;
	int action;
};

int state = 0;
int tape[TAPE_SIZE] = {0};
int tape_i = 0;

struct turing_transition (*transitions)[STATES][2];

int turing_reset(){
	state = 0;
	//tape = {0};
	//memset(tape, 0, TAPE_SIZE * sizeof tape);
	tape_i = 0;
	// TODO reset transitions
}

//#define TURING_DEBUG 0

int turing_step(){
	// halting state
	if(state == 0){
		return TRUE;
	}

	#ifdef TURING_DEBUG
	printf("tapei: %d\n", tape_i);
	#endif
	int fixed_tape_i = (tape_i+TAPE_SIZE) % TAPE_SIZE;
	if(fixed_tape_i < 0 || fixed_tape_i >= TAPE_SIZE){
		printf("%d tapei too small or too large\n", fixed_tape_i);
		return TRUE;
	}
	int tape_symbol = tape[fixed_tape_i];
	#ifdef TURING_DEBUG
		printf("fixed_tape_i: %d, state: %d, symbol: %d\n",
			fixed_tape_i, state, tape_symbol);
	#endif

	struct turing_transition t_entry = (*transitions)[state][tape_symbol];
	#ifdef TURING_DEBUG
		printf("next_state: %d, write: %d, action: %d\n",
			t_entry.next_state, t_entry.write, t_entry.action);
	#endif

	tape[fixed_tape_i] = t_entry.write;
	state = t_entry.next_state;
	if(t_entry.action == RIGHT){
		tape_i++;
	}else if(t_entry.action == LEFT){
		tape_i--;
	}

	#ifdef TURING_DEBUG
	printf("hi\n\n");
	#endif

	if(state == 0){
		return TRUE;
	}else{
		return FALSE;
	}
}

struct turing_transition bb5_transition_table[STATES][2];

void init_bb5(){
        m_SET_TRANSITION(bb5_transition_table, 1,0, 2,1,RIGHT);
        m_SET_TRANSITION(bb5_transition_table, 1,1, 3,1,LEFT);

	m_SET_TRANSITION(bb5_transition_table, 2,0, 3,1,RIGHT);
	m_SET_TRANSITION(bb5_transition_table, 2,1, 2,1,RIGHT);

	m_SET_TRANSITION(bb5_transition_table, 3,0, 4,1,RIGHT);
	m_SET_TRANSITION(bb5_transition_table, 3,1, 5,0,LEFT);

	m_SET_TRANSITION(bb5_transition_table, 4,0, 1,1,LEFT);
	m_SET_TRANSITION(bb5_transition_table, 4,1, 4,1,LEFT);

	m_SET_TRANSITION(bb5_transition_table, 5,0, 0,1,RIGHT);
	m_SET_TRANSITION(bb5_transition_table, 5,1, 1,0,LEFT);

	transitions = &bb5_transition_table;

	state = 1;
}

int main(){
	printf("BB5\n");

	init_bb5();

	int counter=1;
	while(turing_step() != TRUE){
		counter++;
		//keep doing step until HALT
	}
	printf("%d steps\n", counter);

	return 0;

}

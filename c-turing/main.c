#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define T_STATES 6

#define T_TRUE 1
#define T_FALSE 0
#define T_TAPE_SIZE 13000

#define T_RIGHT 0
#define T_LEFT 1

#define m_SET_TRANSITION(table, state, symbol, next_state, write, action) table[state][symbol] = (struct t_transition) {next_state,write,action}

struct t_transition{
	int next_state;
	int write;
	int action;
};

int t_state = 0;
int t_tape[T_TAPE_SIZE] = {0};
int t_tape_i = 0;

struct t_transition (*t_transitions)[T_STATES][2];

//#define T_DEBUG

int t_step(){
	// halting state
	if(t_state == 0){
		return T_TRUE;
	}

	#ifdef T_DEBUG
	printf("t_tapei: %d\n", t_tape_i);
	#endif
	int fixed_tape_i = (t_tape_i+T_TAPE_SIZE) % T_TAPE_SIZE;
	if(fixed_tape_i < 0 || fixed_tape_i >= T_TAPE_SIZE){
		printf("%d tapei too small or too large\n", fixed_tape_i);
		return T_TRUE;
	}
	int tape_symbol = t_tape[fixed_tape_i];
	#ifdef T_DEBUG
		printf("fixed_tape_i: %d, state: %d, symbol: %d\n",
			fixed_tape_i, t_state, tape_symbol);
	#endif

	struct t_transition t_entry = (*t_transitions)[t_state][tape_symbol];
	#ifdef T_DEBUG
		printf("next_state: %d, write: %d, action: %d\n",
			t_entry.next_state, t_entry.write, t_entry.action);
	#endif

	t_tape[fixed_tape_i] = t_entry.write;
	t_state = t_entry.next_state;
	if(t_entry.action == T_RIGHT){
		t_tape_i++;
	}else if(t_entry.action == T_LEFT){
		t_tape_i--;
	}

	#ifdef T_DEBUG
	printf("hi\n\n");
	#endif

	if(t_state == 0){
		return T_TRUE;
	}else{
		return T_FALSE;
	}
}

struct t_transition bb5_transition_table[T_STATES][2];

//typedef struct t_transition_string { int x[5]; }
//	t_transition_string;
//typedef struct t_transition_func_string { t_transition_string x[T_STATES*2]; }
//	t_transition_func_string;
typedef struct t_transition t_trans_func_table[T_STATES][2];
#define T_TRANS_MAX_FUNC_STR_SIZE (T_STATES-1)*2*5 + 1
typedef int t_trans_func_str[T_TRANS_MAX_FUNC_STR_SIZE];
// 5,1, 1,0,0
// (state,symbol, next_state,write,action)+
// 

enum t_trans_func_str_options{
	T_STR_OPT_EXPLICIT_STATE_SYMBOL,
	T_STR_OPT_IMPLICIT_STATE_SYMBOL
};

void t_load_trans_func_str(
		t_trans_func_table func_table,
		t_trans_func_str func_str){
	if(func_str[0] == T_STR_OPT_EXPLICIT_STATE_SYMBOL){
	for(int i=1;i<T_TRANS_MAX_FUNC_STR_SIZE;i+=5){
		int state = func_str[i];
		int symbol = func_str[i+1];
		int next_state = func_str[i+2];
		int write = func_str[i+3];
		int action = func_str[i+4];
		m_SET_TRANSITION(func_table, state,symbol, next_state,write,action);
	}
	}else if(func_str[0] == T_STR_OPT_IMPLICIT_STATE_SYMBOL){
	int state = 0;
	int symbol = 0;
	for(int i=1;i<T_TRANS_MAX_FUNC_STR_SIZE;i+=3){
		if(((i-1)/3)%2 == 0)state++;
		if(symbol == 2)symbol = 0;
                int next_state = func_str[i+0];
                int write = func_str[i+1];
                int action = func_str[i+2];
                m_SET_TRANSITION(func_table, state,symbol++, next_state,write,action);
        }
	}
}



void init_bb5_new(){
	t_trans_func_str bb5_func_str1 = {
        T_STR_OPT_EXPLICIT_STATE_SYMBOL,
        1,1, 3,1,T_LEFT,
        1,0, 2,1,T_RIGHT,
        2,0, 3,1,T_RIGHT,
        2,1, 2,1,T_RIGHT,
        3,0, 4,1,T_RIGHT,
        3,1, 5,0,T_LEFT,
        4,0, 1,1,T_LEFT,
        4,1, 4,1,T_LEFT,
        5,0, 0,1,T_RIGHT,
        5,1, 1,0,T_LEFT
	};

	t_trans_func_str bb5_func_str2 = {
	T_STR_OPT_IMPLICIT_STATE_SYMBOL,
        2,1,T_RIGHT,
        3,1,T_LEFT,
        3,1,T_RIGHT,
        2,1,T_RIGHT,
        4,1,T_RIGHT,
        5,0,T_LEFT,
        1,1,T_LEFT,
        4,1,T_LEFT,
        0,1,T_RIGHT,
        1,0,T_LEFT
        };

	t_load_trans_func_str(bb5_transition_table, bb5_func_str2);
	t_transitions = &bb5_transition_table;
	t_state = 1;
}

int triple_maxes[3] = {T_STATES-1,1,1};
int triple[3] = {0,0,0};
int next_triple(){
	if(++triple[2] > triple_maxes[2]){
		triple[2] = 0;
		if(++triple[1] > triple_maxes[1]){
			triple[1] = 0;
			if(++triple[0] > triple_maxes[0]){
				triple[0] = triple_maxes[0];
				triple[1] = triple_maxes[1];
				triple[2] = triple_maxes[2];
				return T_TRUE;
			}
		}
	}
	return T_FALSE;
}
void reset_triple(){
	triple[0] = 0;
	triple[1] = 0;
	triple[2] = 0;
}

int main(){
	printf("BB5\n");

	//init_bb5();
	init_bb5_new();

	int counter=1;
	while(t_step() != T_TRUE){
		counter++;
		//keep doing step until HALT
	}
	printf("%d steps\n", counter);

	return 0;

}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

//#define T_DEBUG

#define T_MAX_STATES (6)

#define T_TRUE (1)
#define T_FALSE (0)
#define T_TAPE_SIZE (1<<15)

#define T_RIGHT (0)
#define T_LEFT (1)

#define MAX(a,b) (a>=b ? a:b)
#define MIN(a,b) (a<=b ? a:b)

#define m_SET_TRANSITION(table, state, symbol, next_state, write, action) table[state][symbol] = (struct t_transition) {next_state,write,action}

struct t_transition{
	int next_state;
	int write;
	int action;
};

int t_number_of_states = 6;

int t_state = 1;
char t_tape[T_TAPE_SIZE] = {0};
int t_tape_i = T_TAPE_SIZE/2;
int t_leftmost_i = T_TAPE_SIZE/2;
int t_rightmost_i = T_TAPE_SIZE/2 + 1;

struct t_transition t_transitions_table[T_MAX_STATES][2] = {0};
struct t_transition (*t_transitions)[T_MAX_STATES][2] = &t_transitions_table;

int t_step(){
	// halting state
	if(t_state == 0){
		#ifdef T_DEBUG
		printf("halted\n");
		#endif
		return T_TRUE;
	}

	#ifdef T_DEBUG
	printf("t_tapei: %d\n", t_tape_i);
	#endif
	//int fixed_tape_i = (t_tape_i+T_TAPE_SIZE) % T_TAPE_SIZE;
	if(t_tape_i < 0 || t_tape_i >= T_TAPE_SIZE){
		printf("%d tapei too small or too large\n", t_tape_i);
		return T_TRUE;
	}
	int tape_symbol = t_tape[t_tape_i];
	#ifdef T_DEBUG
	printf("t_tape_i: %d, state: %d, symbol: %d\n",
		t_tape_i, t_state, tape_symbol);
	#endif

	struct t_transition t_entry = (*t_transitions)[t_state][tape_symbol];
	#ifdef T_DEBUG
	printf("next_state: %d, write: %d, action: %d\n",
		t_entry.next_state, t_entry.write, t_entry.action);
	#endif

	// printf("wrote %d at %d\n", t_entry.write, t_tape_i);
	t_tape[t_tape_i] = t_entry.write;
	t_state = t_entry.next_state;

	if(t_entry.action == T_RIGHT){
		// printf("moved right to %d\n", t_tape_i+1);
		t_tape_i++;
		t_rightmost_i = MAX(t_rightmost_i, t_tape_i+1);
	}else if(t_entry.action == T_LEFT){
		// printf("moved left to %d\n", t_tape_i-1);
		t_tape_i--;
		t_leftmost_i = MIN(t_leftmost_i, t_tape_i);
	}

	
	#ifdef T_DEBUG
	printf("hi\n\n");
	#endif

	if(t_state == 0){
		#ifdef T_DEBUG
		printf("halted\n");
		#endif
		return T_TRUE;
	}else{
		return T_FALSE;
	}
}

void turing_reset(){
	t_state = 1;
	t_tape_i = T_TAPE_SIZE/2;
	t_leftmost_i = T_TAPE_SIZE/2;
	t_rightmost_i = T_TAPE_SIZE/2 + 1;

	memset(t_tape, 0, sizeof(t_tape));
	memset(t_transitions, 0, sizeof(t_transitions)*2*T_MAX_STATES);

	t_transitions = &t_transitions_table;
}

char* turing_output_start(){
	return t_tape+t_leftmost_i;
}

char* turing_output_end(){
	return t_tape+t_rightmost_i;
}

int turing_output_size(){
	int diff = t_rightmost_i - t_leftmost_i;
	if(diff == 0)return 1;
	return diff;
}

char action_pp[2] = {'R','L'};
void print_turing_transition_table(){
	printf("state symbol: (next_state, write, action)\n");
	for(int i=0;i<t_number_of_states;i++){
		struct t_transition te = (*t_transitions)[i][0];
		printf("%d 0: (%d,%d,%c) \n",
			i, te.next_state, te.write, action_pp[te.action]);
		te = (*t_transitions)[i][1];
		printf("%d 1: (%d,%d,%c) \n",
			i, te.next_state, te.write, action_pp[te.action]);
	}
}

struct t_transition bb5_transition_table[T_MAX_STATES][2];


typedef struct t_transition t_trans_func_table[T_MAX_STATES][2];
#define T_TRANS_MAX_FUNC_STR_SIZE ((T_MAX_STATES-1)*2*5 + 1)
// typedef int t_trans_func_str[T_TRANS_MAX_FUNC_STR_SIZE];
// 5,1, 1,0,0
// (state,symbol, next_state,write,action)+
// 

enum t_trans_func_str_options{
	T_STR_OPT_EXPLICIT_STATE_SYMBOL,
	T_STR_OPT_IMPLICIT_STATE_SYMBOL
};

int t_trans_func_str_size(int enum_options){
	if(enum_options == T_STR_OPT_EXPLICIT_STATE_SYMBOL){
		return 10*(t_number_of_states-1) + 1;
	}else if(enum_options == T_STR_OPT_IMPLICIT_STATE_SYMBOL){
		return 6*(t_number_of_states-1) + 1;
	}
}

void t_load_trans_func_str(
		t_trans_func_table func_table,
		int func_str[]){
	int const enum_options = func_str[0];
	if(enum_options == T_STR_OPT_EXPLICIT_STATE_SYMBOL){
	for(int i=1;i<t_trans_func_str_size(enum_options)-1;i+=5){
		int state = func_str[i];
		int symbol = func_str[i+1];
		int next_state = func_str[i+2];
		int write = func_str[i+3];
		int action = func_str[i+4];
		m_SET_TRANSITION(func_table, state,symbol, next_state,write,action);
	}
	}else if(enum_options == T_STR_OPT_IMPLICIT_STATE_SYMBOL){
	int state = 0;
	int symbol = 0;
	for(int i=1;i<t_trans_func_str_size(enum_options)-1;i+=3){
		if(((i-1)/3)%2 == 0)state++;
		if(symbol == 2)symbol = 0;
                int next_state = func_str[i+0];
                int write = func_str[i+1];
                int action = func_str[i+2];
                m_SET_TRANSITION(func_table, state,symbol++, next_state,write,action);
        }
	}
}

int bb5_func_str1[] = {
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

int bb5_func_str2[] = {
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

void init_bb5_new(){
	printf("BB(5)\n");
	t_load_trans_func_str(bb5_transition_table, bb5_func_str2);
	t_transitions = &bb5_transition_table;
	t_state = 1;
}

int t_turing_triplet_str_size = 0;

int next_triple(int triple[], int triple_maxes[]){
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

int zero_triple(int triple[]){
    triple[0] = 0;
    triple[1] = 0;
    triple[2] = 0;
}

int last_triple_i = 0;
int first_triple_i = 0;
int next_turing_triple_str(int the_str[]){
	int triple_maxes[3] = {t_number_of_states-1, 1, 1};
	int triple_i = last_triple_i;
	int* current_triple = &the_str[triple_i];

	while(next_triple(current_triple, triple_maxes) == T_TRUE){
		zero_triple(current_triple);
		triple_i -= 3;
		current_triple = &the_str[triple_i];
		if(triple_i < 0)return T_TRUE;
	}
	triple_i = last_triple_i;
	current_triple = &the_str[triple_i];
	return T_FALSE;
}

int actual_turing_str[T_TRANS_MAX_FUNC_STR_SIZE] = {0};
int* turing_str = &actual_turing_str[1];
void printturing(uint64_t count){
    for(int i = 0 ; i<t_turing_triplet_str_size;i++){
        printf("%d ", turing_str[i]);
    }
    printf(",%ld\n", count);
}

void reset_turing_str(){
	memset(actual_turing_str, 0, sizeof(int)*T_TRANS_MAX_FUNC_STR_SIZE);
}

void print_all_turing_strings(){
	uint64_t count = 1;
	do{
		if(count > 20636)
				printturing(count);
		count++;
	}while(next_turing_triple_str(turing_str) == T_FALSE);
}

void load_machine_from_triples_str(){
	actual_turing_str[0] = T_STR_OPT_IMPLICIT_STATE_SYMBOL;
	
	t_load_trans_func_str(t_transitions_table, actual_turing_str);
	t_transitions = &t_transitions_table;
}

int BB_steps[] = {0,3,6,21,107,47176870};

int run_halted = T_TRUE;

uint64_t run_until_halt_or_bb(){
	uint64_t counter = 1;
	run_halted = T_TRUE;
	while(t_step() == T_FALSE ){
		if(++counter > BB_steps[t_number_of_states-1]){
			run_halted = T_FALSE;
			break;
		}
	}
	return counter;
}

uint64_t run_until_halt(uint64_t last_step){
	uint64_t counter = 1;
	if(last_step == 0){
		run_halted = T_FALSE;
		return counter;
	}
	while(t_step() == T_FALSE){
		if(counter++ == last_step){
			run_halted = T_FALSE;
			return counter;
		}
	}
	run_halted = T_TRUE;
	return counter;
}

void change_state_number(int n){
	t_number_of_states = n+1;
	t_turing_triplet_str_size = 6*(t_number_of_states-1);
	last_triple_i = t_turing_triplet_str_size-3;
}

void bb5_load_from_str_triples(){
	printf("BB5 from str triple test\n");
	memcpy(actual_turing_str, bb5_func_str2,
		sizeof(int)*t_trans_func_str_size(T_STR_OPT_IMPLICIT_STATE_SYMBOL));
	load_machine_from_triples_str();
}

int total = 0;
int halters = 0;

int load_run_halt_or_bb_next_machine(){
	turing_reset();
	load_machine_from_triples_str();
	uint64_t steps = run_until_halt_or_bb();
	if(run_halted == T_TRUE){
		halters++;
		//print_turing_transition_table();
		//printf("%ld steps\n", steps);
	}
	total++;
	return next_turing_triple_str(turing_str);
}

void load_run_halt_bb_all_turing_strs(){
	while(load_run_halt_or_bb_next_machine() == T_FALSE){}
}

// 1RB0LA_1RC1RA_0LD0RD_0RB1LA
int random_tm1[] = {
	T_STR_OPT_IMPLICIT_STATE_SYMBOL,
		//A1, B2, C3, D4
        2,1,T_RIGHT,	//1RB
        1,0,T_LEFT,		//0LA
        3,1,T_RIGHT,	//1RC
        1,1,T_RIGHT,	//1RA
        4,0,T_LEFT,		//0LD	
        4,0,T_RIGHT,	//0RD
        2,0,T_RIGHT,	//0RB
        1,1,T_LEFT,		//1LA
};

void load_random_tm(){
	// printf("Random TM test\n");
	change_state_number(4);
	t_load_trans_func_str(t_transitions_table, random_tm1);
}

void do_test(int n){
	run_until_halt(n);
	printf("step %d: ", n);
	for(int i = 0; i < turing_output_size(); i++){
		printf("%d", *(turing_output_start()+i));
	}

	// for(int* p = turing_output_start(); p < turing_output_end(); p++){
	// 	printf("%d", *p);
	// }
	printf("\n");
}

/*
if the two arrays have n equal elements,
return T_TRUE, otherwise return T_FALSE
*/
char array_compare(char* a, char* b, int n){
	for(int i=0;i<n;i++){
		if(a[i] != b[i])return T_FALSE;
	}
	return T_TRUE;
}

int find_shortest_tm_test(char* string, int length, int max_states){
	int state_i = 0;
	while(state_i++ <= max_states){
		printf("checking all %d state TMs...\n", state_i);
		change_state_number(state_i);
		while(load_run_halt_or_bb_next_machine() == T_FALSE){
			if(run_halted == T_TRUE && length == turing_output_size()){
				if(array_compare(string, turing_output_start(), length) == T_TRUE){
					printf("found shortest str\n");
					print_turing_transition_table();
					for(int i = 0; i < turing_output_size(); i++){
						printf("%d", *(turing_output_start()+i));
					}
					return state_i;
				}
			}
		}
	}
}

#define MAX_STRING_SIZE (14)
#define STRING_FREQUENCIES_SIZE (1<<MAX_STRING_SIZE+1)
int string_frequencies[STRING_FREQUENCIES_SIZE] = {0};
int strings_greater_than_max = 0;
int total_strings = 0;

/*
length >= 1
*/
int string_start_index(int length){
	int index=0;
	for(int i=1;i<length;i++){
		index += 1<<(i);
	}
	return index;
}

/*
right being the lowest bit
*/
int convert_binary_array_to_num(char* str, int length){
	int num = 0;
	for(int i=length-1,j=0; i>=0; i--,j++){
		if(str[i] == 1)num += 1<<j;
	}
	return num;
}

void enumerate_turing_strings_test(int max_states){
	int state_i = 1;
	while(state_i <= max_states){
		printf("checking all %d state TMs...\n", state_i);
		change_state_number(state_i);
		while(load_run_halt_or_bb_next_machine() == T_FALSE){
			if(run_halted == T_TRUE){
				const int output_length = turing_output_size();
				if(output_length <= MAX_STRING_SIZE){
					int num_value = convert_binary_array_to_num(turing_output_start(), output_length);
					string_frequencies[string_start_index(output_length)+num_value]++;
				}else{
					strings_greater_than_max++;
				}
				total_strings++;
			}
		}
		state_i++;
	}
}

int main(){
	// for(int i=0;i<100;i++){
	// 	turing_reset();
	// 	load_random_tm();
	// 	do_test(i);
	// }

	// char test_str[] = {1,0,1};
	// int length = sizeof(test_str)/sizeof(char);
	int max_states = 3;
	// find_shortest_tm_test(test_str, length, max_states);
	enumerate_turing_strings_test(max_states);
	printf("strings_greater_than_max: %d, total_strings: %d \n",
		strings_greater_than_max, total_strings);

	// int states_to_test = 3;
	// change_state_number(states_to_test);
	// print_all_turing_strings();
	//load_machine_from_triples_str();
	//printf("%ld steps\n", run_until_halt_or_bb());

	// load_run_halt_bb_all_turing_strs();
	// printf("%d states, %d halters, %d total, %f percent halted\n",states_to_test, halters, total,
	// 	(float)halters/(float)total);

	// change_state_number(5);
	// init_bb5_new();
	// bb5_load_from_str_triples();
	//print_turing_transition_table();
	//printf("%d\n", t_trans_func_str_size(T_STR_OPT_IMPLICIT_STATE_SYMBOL));
	// printf("%ld steps\n", run_until_halt_or_bb());
	return 0;

}

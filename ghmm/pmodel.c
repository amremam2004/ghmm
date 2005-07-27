#include "pmodel.h"
#include "mes.h"

static int pstate_alloc(pstate *state, int M, int in_states, int out_states) {
# define CUR_PROC "pstate_alloc"
  int res = -1;
  if(!m_calloc(state->b, M)) {mes_proc(); goto STOP;}
  if (out_states > 0) {
    if(!m_calloc(state->out_id, out_states)) {mes_proc(); goto STOP;}
    if(!m_calloc(state->out_a, out_states)) {mes_proc(); goto STOP;}
  }
  if (in_states > 0) {
    if(!m_calloc(state->in_id, in_states)) {mes_proc(); goto STOP;}
    if(!m_calloc(state->in_a, in_states)) {mes_proc(); goto STOP;}
  }
  res = 0;
STOP:
  return(res);
# undef CUR_PROC
} /* model_state_alloc */

void pstate_clean(pstate *my_state) {
#define CUR_PROC "pstate_clean"
  if (!my_state) return;
  
  if (my_state->b)
    m_free(my_state->b);
  
  if (my_state->out_id)
    m_free(my_state->out_id);
  
  if (my_state->in_id)
    m_free(my_state->in_id);
  
  if (my_state->out_a)
    m_free(my_state->out_a);
  
  if (my_state->in_a)
    m_free(my_state->in_a);
  
  if (my_state->class_change)
    m_free(my_state->class_change);

  my_state->pi         = 0;
  my_state->b          = NULL;
  my_state->out_id     = NULL;  
  my_state->in_id      = NULL;
  my_state->out_a      = NULL;
  my_state->in_a       = NULL;
  my_state->out_states = 0;
  my_state->in_states  = 0;
  my_state->fix        = 0;  

#undef CUR_PROC
} /* pstate_clean */

/* use this to allocate the memory for a pmodel and set the pointers to NULL */
pmodel * init_pmodel() {
  pmodel * mo;
  mo = (pmodel*)calloc(1, sizeof(pmodel));
  if (!mo)
    return NULL;
  return mo;
}

pclass_change_context * init_pclass_change_context() {
#define CUR_PROC "init_pclass_change_context"
  pclass_change_context * pccc;
  if (!m_calloc(pccc, 1)) {mes_proc(); return NULL;}
  pccc->get_class = &default_transition_class;
  pccc->user_data = NULL;
  return pccc;
#undef CUR_PROC
}

int pmodel_free(pmodel *mo) {
#define CUR_PROC "pmodel_free"
  int i;
  mes_check_ptr(mo, return(-1));
  if( !mo ) return(0);
  if (mo->s) {
    for (i = 0; i < mo->N; i++)
      pstate_clean(&(mo->s[i]));
    m_free(mo->s);
  }
  if (mo->  silent)
    m_free(mo->silent);

  if (mo -> tied_to)
    m_free(mo->tied_to);
  
  /*if ((*mo) -> emission_order) Moved to state
    m_free((*mo)->emission_order);*/
  
  if (mo-> topo_order)
    m_free(mo->topo_order);

  if (mo->number_of_alphabets > 0)
    m_free(mo->size_of_alphabet);
         
  m_free(mo);
  return(0);
#undef CUR_PROC
} /* model_free */  

pstate * get_pstateptr(pstate * ary, int index){ return ary + index; }

/* get the emission index for a pair of symbols 
   if the pair cannot be emmited this returns the size of the emission table 
   plus 1 => emission table[size of table] should be 1 and the actual size
   should be one more... */
int pair(int symbol_x, int symbol_y, int alphabet_size, int off_x, int off_y) {
  if (off_x == 0 && symbol_y >= 0)
    return symbol_y;
  if (off_y == 0 && symbol_x >= 0)
    return symbol_x;
  if (symbol_x < 0 || symbol_y < 0) {
    if (off_x == 0 || off_y == 0) 
      return alphabet_size;
    else
      return (alphabet_size - 1) * (alphabet_size - 1) + 1;
  }
  return(symbol_x * alphabet_size + symbol_y);
}

int emission_table_size(pmodel* mo, int state_index) {
  /* the alphabet is over single sequences so get the maximal index for the
     lookup of emission probabilities and use it to determine the size of
     the lookup table */
  int size =  mo->size_of_alphabet[mo->s[state_index].alphabet];
  return pair(size - 1, size - 1, size, mo->s[state_index].offset_x, mo->s[state_index].offset_y) + 1;
}
 
void print_pstate(pstate * s) {
  printf("offset x: %i\n", s->offset_x);
  printf("offset y: %i\n", s->offset_y);
  printf("alphabet: %i\n", s->alphabet);
  printf("kclasses: %i\n", s->kclasses);
  printf("in_state: %i\n", s->in_states);
  int i;
  for (i=0; i<s->in_states; i++)
    printf("%i ", s->in_id[i]);
  printf("\n");
  printf("probabilities...\n");
}
 
void print_pmodel(pmodel* mo) {
  printf("Pair HMM model\n");
  printf("max offset x: %i\n", mo->max_offset_x);
  printf("max offset y: %i\n", mo->max_offset_y);
  printf("Number of states: %i\n", mo->N);
  int i;
  for (i=0; i<mo->N; i++) {
    printf("State %i:\n", i);
    print_pstate(&(mo->s[i]));
  }
}

int default_transition_class(pmodel * mo, mysequence * X, mysequence * Y, int index_x, int index_y, void * user_data) {
  return 0;
}

void set_to_default_transition_class(pclass_change_context * pccc) {
  if (pccc){
    pccc->get_class = &default_transition_class;
    pccc->user_data = NULL;
  }
  else
    fprintf(stderr, "set_to_default_transition_class: No class change context\n");
}
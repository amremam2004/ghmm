/*******************************************************************************
  author       : Wasinee Rungsarityotin
  filename     : ghmm/ghmm/sdmodel.h
  created      : DATE: 2. April 2003
  $Id$

__copyright__

*******************************************************************************/


#ifndef SDMODEL_H
#define SDMODEL_H

#ifdef __cplusplus
extern "C" {
#endif

/**@name HMM-Modell */
/*@{ (Doc++-Group: model) */

/** @name state
    The basic structure, keeps all parameters that belong to a state. 
*/
struct sdstate {
  /** Initial probability */ 
  double pi;
  /** Output probability */
  double *b;
  /** ID of the following state */ 
  int *out_id;  
  /** ID of the previous state */    
  int *in_id;

  /** transition probs to successor states. It is a
   matrix in case of mult. transition matrices (COS > 1)*/
  double **out_a; 
  /** transition probs from predecessor states. It is a
   matrix in case of mult. transition matrices (COS > 1) */ 
  double **in_a;

  /** Transition probability to a successor 
      double *out_a; */
  /** Transition probablity to a precursor 
      double *in_a;*/

  /** Number of successor states */     
  int out_states; 
  /** Number of precursor states */
  int in_states;  
  /** if fix == 1 --> b stays fix during the training */
  int fix;
};
typedef struct sdstate sdstate;

/** @name model
    The complete HMM. Contains all parameters, that define a HMM.
*/
struct sdmodel {
  /** Number of states */
  int N;
  /** Number of outputs */   
  int M;   
 /** smodel includes continuous model with one transition matrix 
      (cos  is set to 1) and an extension for models with several matrices
      (cos is set to a positive integer value > 1).*/
  int cos;
  /** Vector of the states */
  sdstate *s; 
  /** Prior for the a priori probability for the model.
      A value of -1 indicates that no prior is defined. */
  double prior;
};
typedef struct sdmodel sdmodel;


#ifdef __cplusplus
}
#endif


/*
  Important: The inclusion of sequence.h ist not done before this point in
  order to avoid error by compiling.
*/
#include <ghmm/sequence.h>
#include <ghmm/scanner.h>


#ifdef __cplusplus
extern "C" {
#endif


/** Frees the memory of a model.
    @return 0 for succes; -1 for error
    @param mo:  pointer to a model */
int     sdmodel_free(sdmodel **mo);

/**
   Reads in ASCII data to initialize an array of models. Memory allocation for
   the models is done here.
   @return array of pointers to the models
   @param filename:   the ASCII input file
   @param mo_number:  filled with number of models read */
/**model** model_read(char *filename, int *mo_number); */

/**
   Reads in a model, where the model parameters are explicit given in
   matrix form. Memory allocation for the model is also done here.
   @return pointer to the model
   @param s:       scanner
   @param multip:  multiplicity; gives how many copies should 
   be made of the model 
   model*  model_direct_read(scanner_t *s, int *multip);
*/

/**
   Produces simple left-right models given sequences. 
   The function "model_generate_from_sequence" is called for each 
   model that should be made. The sequences are read in from the
   ASCII file and thrown away again when leaving the function.
   @return vector of models
   @param s:          scanner
   @param new_models: number of models to produce 
   model **model_from_sequence_ascii(scanner_t *s, long *mo_number);
*/

/** 
    Produces simple left-right models given sequences. The sequences
    are not read in from file, but exists already as a structur.
    @return vector of models
    @param s:          scanner
    @param new_models: number of models to produce 
    model **model_from_sequence(sequence_t *sq, long *mo_number);
*/

/**
   Copies a given model. Allocates the necessary memory.
   @return copy of the model
   @param mo:  model to copy */
sdmodel*  sdmodel_copy(const sdmodel *mo);

/**
   Tests if all standardization requirements of model are fulfilled. 
   (That is, if the sum of the probabilities is 1).
   @return 0 for succes; -1 for error
   @param mo:  model to test 
   int     model_check(const model* mo);
*/

/**
   Tests if number of states and number of outputs in the models match.
   @return 0 for succes; -1 for error
   @param mo:           vector of models
   @param model_number: numbr of models 
   int     model_check_compatibility(model **mo, int model_number);
*/

/**
   Produces a model, which generates the given sequence with probability 1.
   The model is a strict left-right model with one state for each element 
   in the sequence and the output in state i is the i-th value in the sequence 
   with probability 1. The model also has a final state, a state with no output.
   @return         pointer to the produced model 
   @param seq:      sequence
   @param seq_len:  length of the sequence
   @param anz_symb: number of symbols in the sequence
   
   model*  model_generate_from_sequence(const int *seq, int seq_len,int anz_symb);
*/

/** 
    Produces sequences to a given model. All memory that is needed for the 
    sequences is allocated inside the function. It is possible to define
    the length of the sequences global (global_len > 0) or it can be set 
    inside the function, when a final state in the model is reach (a state
    with no output). If the model has no final state, the sequences will
    have length MAX_SEQ_LEN.
    @return             pointer to an array of sequences
    @param mo:          model
    @param seed:        initial parameter for the random value generator
                        (an integer). If seed == 0, then the random value
			generator is not initialized.
    @param global_len:  length of sequences (=0: automatically via final states)
    @param seq_number:  number of sequences
*/
sequence_t *sdmodel_generate_sequences(sdmodel* mo, int seed, int global_len,
				     long seq_number, int Tmax);

/**
   Calculates the sum log( P( O | lambda ) ).
   Sequences, that can not be generated from the given model, are neglected.
   @return    log(P)
   @param mo model
   @param sq sequences       
double sdmodel_likelihood(sdmodel *mo, sequence_t *sq);
*/

/**
   Writes a model in matrix format.
   @param file: output file
   @param mo:   model
*/
void sdmodel_print(FILE *file, sdmodel *mo); 

/**
   Writes transition matrix of a model.
   @param file: output file
   @param mo:   model
   @param tab:  format: leading tabs
   @param separator: format: seperator for columns
   @param ending:    format: end of a row  
*/
void sdmodel_Ak_print(FILE *file, sdmodel *mo, int k, char *tab, char *separator, 
		   char *ending);
/**
   Writes output matrix of a model.
   @param file: output file
   @param mo:   model
   @param tab:  format: leading tabs
   @param separator: format: seperator for columns
   @param ending:    format: end of a row  
*/
void sdmodel_B_print(FILE *file, sdmodel *mo, char *tab, char *separator, 
		   char *ending);
/**
   Writes initial allocation vector of a matrix.
   @param file: output file
   @param mo:   model
   @param tab:  format: leading Tabs
   @param separator: format: seperator for columns
   @param ending:    format: end of a row  
*/
void sdmodel_Pi_print(FILE *file, sdmodel *mo, char *tab, char *separator, 
		    char *ending);
/**
   Writes fix vector of a matrix.
   @param file: output file
   @param mo:   model
   @param tab:  format: leading Tabs
   @param separator: format: seperator for columns
   @param ending:    format: end of a row  

   void model_fix_print(FILE *file, model *mo, char *tab, char *separator, 
   char *ending);
*/

/**
   Writes transposed transition matrix of a model.
   @param file: output file
   @param mo:   model
   @param tab:  format: leading Tabs
   @param separator: format: seperator for columns
   @param ending:    format: end of a row  

   void model_A_print_transp(FILE *file, model *mo, char *tab, char *separator, 
			  char *ending);
*/

/**
   Writes transposed output matrix of a model.
   @param file: output file
   @param mo:   model
   @param tab:  format: leading Tabs
   @param separator: format: seperator for columns
   @param ending:    format: end of a row    
   void model_B_print_transp(FILE *file, model *mo, char *tab, char *separator, 
			  char *ending);
*/


/**
   Writes transposed initial allocation vector of a matrix.
   @param file: output file
   @param mo:   model
   @param tab:  format: leading Tabs
   @param separator: format: seperator for columns
   @param ending:    format: end of a row  

   void model_Pi_print_transp(FILE *file, model *mo, char *tab, char *ending);
*/

/**
   Writes a HMM in matrix format. The input model must be of type
   model_direct, that is, have the parameters saved as matrices.
   @param file:   output file
   @param mo_d:   model of type model_direct
   @param multip: number of copies to write
   void model_direct_print(FILE *file, model_direct *mo_d, int multip);
*/

/** 
    Writes the parameters of a model sorted by states. 
    Is not very concise.   
    @param file: output file
    @param mo:   model
*/
void sdmodel_states_print(FILE *file, sdmodel *mo); 

/** 
    Frees all memory from a model, sets the pointers to NULL and 
    variables to zero.
    @param mo_d  HMM structure (\Ref{struct model_direct})
    @param check Check structure (\Ref{struct hmm_check_t})
    void model_direct_clean(model_direct *mo_d, hmm_check_t *check); 
*/

/** 
    Tests compatibility of the model components.
    @return 0 for success; -1 for failure 
    @param mo_d  HMM structure  (\Ref{struct model_direct})
    @param check Check structure  (\Ref{struct hmm_check_t})
    int model_direct_check_data(model_direct *mo_d, hmm_check_t *check); 
*/

/** Computes probabilistic distance of two models
    @return the distance
    @param m0  model used for generating random output
    @param m  model to compare with
    @param maxT  maximum output length (for HMMs with absorbing states multiple
                 sequences with a toal langth of at least maxT will be 
		 generated)
    @param symmetric  flag, whether to symmetrize distance (not implemented yet)
    @param verbose  flag, whether to monitor distance in 40 steps. 
                    Prints to stdout (yuk!)
   double model_prob_distance(model *m0, model *m, int maxT, int symmetric, int verbose);
*/

/**
   Copies a given state. Allocates the necessary memory.
   @author Peter Pipenbacher
   @return copy of the state
   @param my_state:  state to copy */
#if 0
  state* state_copy(state *my_state);
#endif  

/**
   Copies a given state to a given destination.
   @author Peter Pipenbacher
   @param source:  state to copy 
   @param dest:    destination */
#if 0
  void state_copy_to(state *source, state* dest);
#endif
  
#ifdef __cplusplus
}
#endif

#endif

/*@} (Doc++-Group: model) */

/*******************************************************************************
  author       : Utz J. Pape, Benjamin Georgi
  filename     : ghmm/ghmm/foba.c
  created      : TIME: 00:00:00     DATE: Tue 00. xxx 0000
  $Id$

__copyright__

*******************************************************************************/

#include "ghmm.h"
#include "model.h"
#include "math.h"
#include "const.h"
#include "matrix.h"
#include "mes.h"
#include <assert.h>
#include "modelutil.h"

static int foba_initforward(model *mo, double *alpha_1, int symb, 
			    double *scale) {
# define CUR_PROC "foba_initforward"
  int res = -1;
  int i,j,id,in_id;
  double c_0;
  scale[0] = 0.0;
 
  //printf(" *** foba_initforward\n");
  
  //iterate over non-silent states
  //printf(" *** iterate over non-silent states \n");
  for (i = 0; i < mo->N; i++) {
    printf("silent[%d] = %d\n",i,mo->silent[i]); 
    if (!(mo->silent[i])) {
      alpha_1[i] = mo->s[i].pi * mo->s[i].b[symb];
	  //printf("\nalpha1[%i]=%f\n",i,alpha_1[i]);
	  
	  scale[0] += alpha_1[i];
    }
  }
  //iterate over silent states
  //printf(" *** iterate over silent states \n");
  if (mo->model_type == kSilentStates){
      for (i=0;i<mo->topo_order_length;i++)
      {
         id = mo->topo_order[i];
         alpha_1[id] = mo->s[id].pi;
      
	     //printf("\nsilent_start alpha1[%i]=%f\n",id,alpha_1[id]);
      
	     for (j=0;j<mo->s[id].in_states;j++){ 
	       in_id = mo->s[id].in_id[j];
	       alpha_1[id] += mo->s[id].in_a[j] * alpha_1[in_id];
	  
		   //printf("\n\tsilent_run alpha1[%i]=%f\n",id,alpha_1[id]);
		
	    }
        scale[0] += alpha_1[id];
      }
  }
  //printf("\n%f\n",scale[0]);
  
  if (scale[0] >= EPS_PREC) {
     c_0 = 1/scale[0];
    for (i = 0; i < mo->N; i++) 
      alpha_1[i] *= c_0;
    }
  res = 0;
  return(0); /* attention: scale[0] might be 0 */
# undef CUR_PROC
} /* foba_initforward */

/*----------------------------------------------------------------------------*/

static double foba_stepforward(state *s, double *alpha_t, const double b_symb) {
  int i, id;
  double value = 0.0;
  
  //printf(" *** foba_stepforward\n");
  
  for (i = 0; i < s->in_states; i++) {
    id = s->in_id[i];
    value += s->in_a[i] * alpha_t[id];
 	//printf("    state %d, value %f, p_symb %f\n",id, value, b_symb);
  }
  value *= b_symb; 
  return(value);

} /* foba_stepforward */

/*============================================================================*/

int foba_forward(model *mo, const int *O, int len, double **alpha, 
		 double *scale, double *log_p) {
# define CUR_PROC "foba_forward"
  int res = -1;
  int i, t, id,k;
  double c_t;
  double log_scale_sum = 0.0;
  double non_silent_salpha_sum = 0.0;
  double salpha_log = 0.0;
  
 
  if (mo->model_type == kSilentStates){
    //printf("silent states require topological ordering\n");
	model_topo_ordering(mo);
  }
  
  foba_initforward(mo, alpha[0], O[0], scale);
  if (scale[0] < EPS_PREC) {
    /* means: first symbol can't be generated by hmm */
    *log_p = +1;
  }
  else {
  *log_p = - log(1/scale[0]);
  for (t = 1; t < len; t++) {
      scale[t] = 0.0;
	  
	  //printf("\n\nStep t=%i mit len=%i, O[i]=%i\n",t,len,O[t]);
	 // printf("iterate over non-silent state\n");
	  //iterate over non-silent states
      for (i = 0; i < mo->N; i++) {
	    if (mo->model_type != kSilentStates || !(mo->silent[i])) {
          
		//  printf("  akt_ state %d\n",i);
		  alpha[t][i] = foba_stepforward(&mo->s[i], alpha[t-1], mo->s[i].b[O[t]]);
	      scale[t] +=  alpha[t][i];
		}
      }
      //iterate over silent state
	  // printf("iterate over silent state\n");
      if (mo->model_type == kSilentStates ) {
	    for (i=0;i<mo->topo_order_length;i++) {
	      //printf("\nget id\n");
	      id = mo->topo_order[i];
   		  //printf("  akt_ state %d\n",id);
		  //printf("\nin stepforward\n");
	      alpha[t][id] = foba_stepforward(&mo->s[id], alpha[t],1);
	      //printf("\nnach stepforward\n");
	      scale[t] += alpha[t][id];
	    }
      }
      
	  if (scale[t] < EPS_PREC) {
		printf("\nscale kleiner als eps\n");
	    /* O-string  can't be generated by hmm */
	    *log_p = +1.0;
	    break;
      }
	  c_t = 1/scale[t];
	  for (i = 0; i < mo->N; i++) {
			alpha[t][i] *= c_t;
      }	
	  
	  if ( mo->model_type != kSilentStates) {  
		/*sum log(c[t]) scaling values to get  log( P(O|lambda) ) */
	    //printf("log_p %f -= log(%f) = ",*log_p,c_t);
	    *log_p -= log(c_t);
	    //printf(" %f\n",*log_p); 
	  }
    }
  	if (mo->model_type == kSilentStates && *log_p != +1 ) {
	 	 //printf("silent model\n");
		 for (i=0;i<len;i++){
		   log_scale_sum += log(scale[i]);
	     }  
      	 for(i=0;i < mo->N;i++){
	  	 	if ( !(mo->silent[i]) ) {
	 			non_silent_salpha_sum += alpha[len-1][i];
			}
		 }
		 salpha_log = log(non_silent_salpha_sum);
		 *log_p = log_scale_sum + salpha_log;
	}
  }
  
  //printf("\nin forward: log_p = %f\n",*log_p);
  
  if (*log_p == 1.0){
	  res = -1;
  }
  else {
	  res = 0;
  }	  
			  	  
  return res;
# undef CUR_PROC
} /* foba_forward */

/*============================================================================*/

int foba_descale(double **alpha, double *scale, int t, int n, double **newalpha) {
# define CUR_PROC "foba_descale"
  int i,j,k;
  //printf("\nAngekommen, t=%i, n=%i\n",t,n);
  for (i=0;i<t;i++) {
	  //printf("i=%i\n",i);
      for (j=0;j<n;j++)	{
	  	//printf("\tj=%i\n",j);
	  	newalpha[i][j] = alpha[i][j];
	  	//newalpha[i][j] *= scale[j];
	  	//printf(".");
	 	for (k=0;k<=i;k++) {
	    	//printf(",");
       	 	newalpha[i][j] *= scale[k];
      	}	
	  }
    }
  //printf("\ndescale geschafft\n");
  return 0;
# undef CUR_PROC
} /* foba_descale */


/***************************** Backward Algorithm *************************************************************/

/*int foba_initbackward(model *mo, double *beta_1, int symb, double *scale){
  int i,j;
	
  for (i = 0; i < mo->N; i++) {
    sum = 0.0;
    for (j = 0; j < mo->s[i].out_states; j++) {
	   sum += mo->s[i].out_a[j] * mo->s[j_id].b[symb]; // TEST 
    }
	beta_1[i] = sum;
  }	
} */
  
int foba_backward(model *mo, const int *O, int len, double **beta, const double *scale) {
# define CUR_PROC "foba_backward"
  double *beta_tmp, sum;
  double p;
  int i, j, j_id, t, k, id;
  int r;
  int res = -1;
  double emission;
  
  if (!m_calloc(beta_tmp, mo->N)) {mes_proc(); goto STOP;}
  for (t = 0; t < len; t++)
    mes_check_0(scale[t], goto STOP);
  
  /* topological ordering for models with silent states */
  if (mo->model_type == kSilentStates){
    //printf("silent states require topological ordering\n");
    model_topo_ordering(mo);
  }
  
  //printf("\n\n** initializing...\n");
  
  
  /* initialize */
  for (i = 0; i < mo->N; i++) {
    if (mo->model_type != kSilentStates || !(mo->silent[i])) {
		beta[len-1][i] = 1;
	}
	else {
		beta[len-1][i] = 0;	
	}	
	beta_tmp[i] = beta[len-1][i]/scale[len-1];
  }

  
  /*####################################*/
	//printf("scale:\n");
    //for(r =0;r < len; r++){
	//	printf("%f, ",scale[r]);
	//} 
	//printf("\nbeta_tmp:\n");
    //for(r =0;r < mo->N; r++){
	//	printf("%f, ",beta_tmp[r]);
	//}
	printf("\n");  		  
  /*####################################*/
      
  //printf("** iterating...\n");
    
  /* Backward Step for t = T-1, ..., 0 */
  /* beta_tmp: Vector for storage of scaled beta in one time step 
     loop over reverse topological ordering of silent states, non-silent states  */
  for (t = len-2; t >= 0; t--) {
    
 	//printf("\n*** O(%d) = %d\n",t+1,O[t+1]);

	if(mo->model_type == kSilentStates) {
      //printf("  * silent states:\n");
	  for(k=mo->topo_order_length-1;k>=0;k--){
		id = mo->topo_order[k];

		//printf("  silent[%d] = %d\n",id,mo->silent[id]);
		
		assert(mo->silent[id] == 1);
	  
	    sum = 0.0;
        for (j = 0; j < mo->s[id].out_states; j++) {
	     j_id = mo->s[id].out_id[j];
	     //printf("  von %d nach %d mit %f\n",id,j_id,mo->s[id].out_a[j]);
	     //printf("  [%d,%d] sum += %f * 1 * %f\n",id,j_id,mo->s[id].out_a[j],beta[t+1][j_id]);
	   
	     sum += mo->s[id].out_a[j] * mo->s[j_id].b[O[t+1]] * beta_tmp[j_id];
		 //sum += mo->s[id].out_a[j] * mo->s[j_id].b[O[t+1]] * beta[t+1][j_id]; // TEST 
		 
        }	

		//if(sum > 0.0)
	    //  printf("  --> beta[%d][%d] = %f\n",t+1,id,sum);

		
		beta[t+1][id] = sum;
	  }
    }

	//printf("  * non-silent states:\n");
	for (i = 0; i < mo->N; i++) {
	  if (mo->model_type != kSilentStates || !(mo->silent[i])) {		
		
        sum = 0.0;
        for (j = 0; j < mo->s[i].out_states; j++) {
	      j_id = mo->s[i].out_id[j];
	      if (mo->silent[j_id] == 0){
			emission = mo->s[j_id].b[O[t+1]];
		  }
		  else {
			emission = 1;
		  }	  	  
	      //printf("  von %d nach %d mit %f\n",i,j_id,mo->s[i].out_a[j]);
	      //printf("  [%d,%d] sum += %f * %f * %f\n",i,j_id,mo->s[i].out_a[j],mo->s[j_id].b[O[t+1]],beta[t+1][j_id]);

          sum += mo->s[i].out_a[j] * emission * beta_tmp[j_id];
	      // sum += mo->s[i].out_a[j] * emission * beta[t+1][j_id]; // TEST 
        }
		
	    //if(sum > 0.0)
	    //  printf("  --> beta[%d][%d] = %f\n",t,i,sum);
	  
	    beta[t][i] = sum;
	  }	
    }
  
	for (i = 0; i < mo->N; i++) 
      beta_tmp[i] = beta[t][i]/scale[t];
  
  
  /*##################################  
  printf("\n");
  for (r = len-1; r >= 0; r--) {
    printf("%d | ",O[r]);
	for (i = 0; i < mo->N; i++) {
		printf("%f ",beta[r][i]);
	}
	printf("\n");
  }			
   /*##################################*/   
	
  }
  
  /* Termination, (does not work that way with scaling)
  p = 0.0;
  for (i = 0; i < mo->N; i++) {
	 p += mo->s[i].pi * mo->s[i].b[O[0]]*beta[0][i];
  }	 
  printf("Backward: %f\n",p); */
	  
res = 0;
STOP:
  m_free(beta_tmp);
  return(res);
# undef CUR_PROC
} /* foba_backward */


/*============================================================================*/
int foba_logp(model *mo, const int *O, int len, double *log_p) {
# define CUR_PROC "foba_logp"
  int res = -1;
  double **alpha, *scale = NULL;
 
  alpha = stat_matrix_d_alloc(len, mo->N);
  if (!alpha) {mes_proc(); goto STOP;}
  if (!m_calloc(scale, len)) {mes_proc(); goto STOP;}
  /* run foba_forward */
  if (foba_forward(mo, O, len, alpha, scale, log_p) == -1 )
      { mes_proc(); goto STOP; }
  
  
  res = 0;
STOP:
  stat_matrix_d_free(&alpha);
  m_free(scale);
  return(res);
# undef CUR_PROC
} /* foba_logp */

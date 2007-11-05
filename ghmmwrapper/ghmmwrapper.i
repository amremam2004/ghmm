/*******************************************************************************
*
*       This file is part of the General Hidden Markov Model Library,
*       GHMM version __VERSION__, see http://ghmm.org
*
*       Filename: ghmmwrapper.i
*       Authors:   Wasinee Rungsarityotin, Benjamin Georgi, Janne Grunau
*
*       Copyright (C) 1998-2004 Alexander Schliep
*       Copyright (C) 1998-2001 ZAIK/ZPR, Universitaet zu Koeln
*       Copyright (C) 2002-2004 Max-Planck-Institut fuer Molekulare Genetik,
*                               Berlin
*
*       Contact: schliep@ghmm.org
*
*       This library is free software; you can redistribute it and/or
*       modify it under the terms of the GNU Library General Public
*       License as published by the Free Software Foundation; either
*       version 2 of the License, or (at your option) any later version.
*
*       This library is distributed in the hope that it will be useful,
*       but WITHOUT ANY WARRANTY; without even the implied warranty of
*       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*       Library General Public License for more details.
*
*       You should have received a copy of the GNU Library General Public
*       License along with this library; if not, write to the Free
*       Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*
*
*       This file is version $Revision$
*                       from $Date$
*             last change by $Author$.
*
*******************************************************************************/

%module ghmmwrapper

%include carrays.i
%include cmalloc.i
%include cpointer.i
%include cstring.i

%include constraints.i
%include exception.i    
%include typemaps.i

/* this will not work if we build ghmmwrapper out of the ghmm tree
   include ../config.h twice, once for swig and once for CC */
%{
#include "../config.h"
#include "sclass_change.h"
#include "pclasschange.h"
%}

%include "../config.h"
%include "sclass_change.h"
%include "pclasschange.h"

// Constraints on GHMM date types - no NULL pointers as function arguments
%apply Pointer NONNULL { ghmm_cmodel * };
%apply Pointer NONNULL { ghmm_cmodel ** };
%apply Pointer NONNULL { ghmm_dmodel * };
%apply Pointer NONNULL { ghmm_dmodel ** };
%apply Pointer NONNULL { ghmm_dpmodel * };
%apply Pointer NONNULL { ghmm_dpmodel ** };
%apply Pointer NONNULL { ghmm_dsmodel * };
%apply Pointer NONNULL { ghmm_dsmodel ** };
%apply Pointer NONNULL { ghmm_cstate * };
%apply Pointer NONNULL { ghmm_dstate * };
%apply Pointer NONNULL { ghmm_dpstate * };
%apply Pointer NONNULL { ghmm_dsstate * };
%apply Pointer NONNULL { ghmm_cseq * };
%apply Pointer NONNULL { ghmm_cseq ** };
%apply Pointer NONNULL { ghmm_dseq * };
%apply Pointer NONNULL { ghmm_dseq ** };
%apply Pointer NONNULL { ghmm_dpseq * };
%apply Pointer NONNULL { ghmm_dpseq ** };
%apply Pointer NONNULL { scluster * };


/*==========================================================================
  ========================== test for obsolete features ==================== */
#ifdef GHMM_OBSOLETE

#define SMO_FILE_SUPPORT 1
#define ASCI_SEQ_FILE    1
#define SEQ_LABEL_FIELD  1

#else

#define SMO_FILE_SUPPORT 0
#define ASCI_SEQ_FILE    0
#define SEQ_LABEL_FIELD  0

#endif /* GHMM_OBSOLETE */

/*==========================================================================
  ========================== constants for model types ===================== */
#define kNotSpecified (0)

/** Model is a left-right */
#define kLeftRight (1)

/** Model contains silent states (i.e., states without emissions) */
#define kSilentStates (1 << 2)

/** Model has states with tied emission probabilities */
#define kTiedEmissions (1 << 3)
#define kUntied (-1)

/** Model has states emission probabilities conditioned on previous orders */
#define kHigherOrderEmissions (1 << 4)

/** Model has background distributions */
#define kBackgroundDistributions (1 << 5)
#define kNoBackgroundDistribution (-1)

/** Model is a class HMM with labeled states */
#define kLabeledStates (1 << 6)

#define kTransitionClasses (1 << 7)

#define kDiscreteHMM (1 << 8)

#define kContinuousHMM (1 << 9)

#define kPairHMM (1 << 10)


/* ============== constants ================================================= */
/** 
    Convergence: Halt criterium for Baum-Welch reestimation if the difference
    of log(P) in two consecutive iterations is smaller than (EPS\_ITER\_BW * log(P))..
*/
#define EPS_ITER_BW      0.0001

/**
  Maximum number of iterations in reestimate
  */
#define MAX_ITER_BW      500


/*============================================================================
  =============== Logging Function Callbacks ================================= */
// Grab a Python function object as a Python object.
#ifdef SWIG<python>
%typemap(in) PyObject *pyfunc {
  if (!PyCallable_Check($input)) {
    PyErr_SetString(PyExc_TypeError, "Need a callable object!");
    return NULL;
  }
  $1 = $input;
}
#endif

%{
  /* This function matches the prototype of the normal C callback
     function for our widget. However, we use the clientdata pointer
     for holding a reference to a Python callable object. */
  
  static void PythonCallBack(int level, const char * message, void *clientdata)
    {
      PyObject *func, *arglist;
      
      // Get Python function
      func = (PyObject *) clientdata;
      // Build Python arguments
      arglist = Py_BuildValue("(is)", level, message);
      // Call Python
      PyEval_CallObject(func, arglist);
      // Trash arguments
      Py_DECREF(arglist);
    }
%}

%inline %{
// Set a Python function object as a callback function
// Note : PyObject *pyfunc is remapped with a typempap
static void set_pylogging(PyObject *pyfunc) {
  ghmm_set_logfunc(PythonCallBack, (void *) pyfunc);
  Py_INCREF(pyfunc);
}
%}


/*==========================================================================
  ===== Random Number Generator (RNG) ====================================== */
/* Important! initialise rng  */
extern void ghmm_rng_init(void);

/* Initialise random timeseed */
extern void ghmm_rng_timeseed(GHMM_RNG * r);

%inline %{
	void time_seed(void){
		ghmm_rng_timeseed(RNG);
	}
%}


/* for truncated gaussian pdf */
extern double ighmm_rand_normal_density_trunc(double x, double mean, double u, double a);


/*==========================================================================
  ===== import C structs as shadow classes ================================= */
%define STRUCT_ARRAY(type, name)
%inline %{
        type* name ## _array_alloc(size_t number)  { return calloc(number, sizeof(type)); }
        type* name ## _array_getRef(type* self, size_t index) { return self+index; }
%}
%enddef

%define REFERENCE_ARRAY(type, name)
%inline %{
        type** name ## _array_alloc(size_t number)  { return calloc(number, sizeof(type*)); }
        type*  name ## _array_getitem(type** self, size_t index) { return self[index]; }
        void   name ## _array_setitem(type** self, size_t index, type* value) { self[index] = value; }
%}
%enddef

%include wrapper_alphabet.i

%include wrapper_cseq.i
%include wrapper_dseq.i
%include wrapper_dpseq.i

%include wrapper_cmodel.i
%include wrapper_dmodel.i
%include wrapper_dpmodel.i

%include wrapper_xmlfile.i


/*==========================================================================
  ===== general functions, type arrays and matrices ======================== */
extern void free(void*);

%define ARRAY(type)
%inline %{
        type* type ## _array_alloc(size_t length) { return malloc(length*sizeof(type)); }
        type  type ## _array_getitem(type* self, size_t index) { return self[index]; }
        void  type ## _array_setitem(type* self, size_t index, type value) { self[index] = value; }
%}
%enddef
ARRAY(int)
ARRAY(long)
ARRAY(double)

%define MATRIX(type)
%inline %{
        type** type ## _matrix_alloc(size_t rows, size_t cols)
            {
                int i;
                type** mat = malloc(rows * sizeof(type*));
                for (i=0; i<rows; ++i) mat[i] = malloc(cols * sizeof(type));
                return mat;
            }
        type** type ## _matrix_alloc_row(size_t rows) { return malloc(rows * sizeof(type*)); }
        void   type ## _matrix_free(type** mat, size_t rows)
            {
                int i;
                for (i=0; i<rows; ++i) free(mat[i]);
                free(mat);
            }
        type*  type ## _matrix_get_col(type** self, size_t index) { return self[index]; }
        void   type ## _matrix_set_col(type** self, size_t index, type* col) { self[index] = col; }
        type   type ## _matrix_getitem(type** self, size_t row, size_t col) { return self[row][col]; }
        void   type ## _matrix_setitem(type** self, size_t row, size_t col, type value) { self[row][col] = value; }
%}
%enddef
MATRIX(int)
MATRIX(double)

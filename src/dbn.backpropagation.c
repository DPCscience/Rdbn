#include <R.h>
#include <Rdefines.h>
#include <Rmath.h>
#include <Rinternals.h>
#include <R_ext/Rdynload.h>
#include <R_ext/Applic.h>
#include <pthread.h>
#include "dbn.h"
#include "dbn.backpropagation.h"
#include "rbm.h"
#include "matrix_functions.h"


/****************************************************************************
 *
 * Compute.
 * 
 * For a given input, computes the output layer.
 */

 // This version stores intermediate layers, and returns a double**.  
// Perfect for use with backpropagation!
double **dbn_compute_store_layers(dbn_t *dbn, double *input) {
  double **layer_output= (double**)Calloc(dbn[0].n_layers, double*);
  layer_output[0]= vector_copy(input, dbn[0].rbms[0].n_inputs);
  for(int i=0;i<dbn[0].n_rbms;i++) {
    layer_output[i+1]= get_layer_outputs(dbn, i, layer_output[i], 1);
  }
  return(layer_output);
}


/****************************************************************************
 *
 * Backpropagation.
 * 
 * After initial training, backpropagation is used to refine the network
 * for descriminitive classification.
 *
 * Took most info from Hinton's Coursea lecture on backpropagation ... Last slide in lecture 3d.
 * Here: https://d396qusza40orc.cloudfront.net/neuralnets/lecture_slides%2Flec3.pdf
 * (NOTE: May require Couresea account).
 */

void compute_layer_error(dbn_t *dbn, int layer, double **observed_output, double *neuron_error, double *next_layer_neuron_error, delta_w_t batch) {
  int n_outputs_cl= dbn[0].rbms[layer].n_outputs; // # outputs in current layer
  int n_inputs_cl= dbn[0].rbms[layer].n_inputs;   // # inputs in current layer

  // Compute error for this layer.
  for(int i=0;i<n_inputs_cl;i++) {
    if(layer>0) next_layer_neuron_error[i]= 0;
    for(int j=0;j<n_outputs_cl;j++) {
      // Compute error derivites for the weights ... (dE/w_{i,j}).
	  double previous_ij= get_matrix_value(batch.delta_w, j, i);
      set_matrix_value(batch.delta_w, j, i, previous_ij+observed_output[layer][i]*neuron_error[j]); 

      // Compute error derivites for the biases.  Conceptually similar to a connection with a neuron of constant output (==1).
      // see: http://stackoverflow.com/questions/3775032/how-to-update-the-bias-in-neural-network-backpropagation
      // At this time, I am NOT updating input biases using backpropagation
      if(i==0) batch.delta_output_bias[j]+= neuron_error[j]; //*observed_output (==DEFINED_AS== 1);
  
      // Compute error for neurons in an internal 'hidden' layer [dE/dy_{i}].
      // dE/dy_{i} = \sum_j w_{i,j}* dE/dz_{j}; where j= \set(outputs); i= \set(inputs).
      if(layer>0) // No need to compute in the last layer.
        next_layer_neuron_error[i]+= neuron_error[j]*get_matrix_value(dbn[0].rbms[layer].io_weights, j, i);
    }
  }
}
 
/* Returns the error derivitives for a particular example.  Equilavent to do_batch_member in rbm.c. */
void backpropagation(dbn_t *dbn, double *input, double *expected_output, delta_w_t *batch) {
  double **observed_output= dbn_compute_store_layers(dbn, input); // Compute the output of the neural network.

  double *next_layer_neuron_error, *neuron_error; // Stores dE/dz.
  
  // Compute last layer error term: dE/dz_j= y_j * (1-y_j) * dE/dy_j
  // Where dE/dy_j= (t_j-y_j) ==> t --> target output.  y--> observed output.
  int n_outputs_ll= dbn[0].rbms[dbn[0].n_rbms-1].n_outputs; // n_outputs in the last layer.
  int layer_index= dbn[0].n_layers-1; // Index of the layer in the double **.
  neuron_error= (double*)Calloc(n_outputs_ll, double);
//  neuron_errors[layer_index]= (double*)Calloc(n_outputs_ll, double);
  for(int j=0;j<n_outputs_ll;j++) {// Foreach neuron in the output layer.
    double oo= observed_output[layer_index][j];
    neuron_error[j]= oo*(1-oo)*(oo-expected_output[j]); // Compute dE/dz_j
  }
  
  // Makes a single pass over the entire deep belief network.
  // Foreach layer, compute the layer's weight error derivitives (dE/dw) and the next neuron output error term (dE/dz).
  for(int layer=(dbn[0].n_rbms-1);layer>=0;layer--) {
    int n_inputs_cl= dbn[0].rbms[layer].n_inputs;   // # inputs in current layer

    if(layer>0) next_layer_neuron_error= (double*)Calloc(n_inputs_cl, double);
    compute_layer_error(dbn, layer, observed_output, neuron_error, next_layer_neuron_error, batch[layer]);     // Shortcut, for code readability.  Computes the error term for the current layer.

    Free(neuron_error);
    if(layer>0) neuron_error= next_layer_neuron_error;
  }
  
  // Free temporary storage ...
  for(int i=0;i<dbn[0].n_layers;i++)
    Free(observed_output[i]);
  Free(observed_output);
}

void *dbn_backprop_partial_minibatch(void *ptab) {
  dbn_pthread_arg_t *pta= (dbn_pthread_arg_t*)ptab;

  for(int i=0;i<pta[0].do_n_elements;i++) {
    backpropagation(pta[0].dbn, pta[0].input, pta[0].expected_output, pta[0].batch);

    // Increment pointers.
    pta[0].input+= pta[0].dbn[0].rbms[0].n_inputs;
    pta[0].expected_output+= pta[0].dbn[0].rbms[pta[0].dbn[0].n_rbms-1].n_outputs;
  }
}

/////////////IF NO PTREADS, USE THIS. ///////////////////////////////////////////////
/* Runs the backpropagation algorithm over each element of a mini-batch. */
void backpropagation_minibatch(dbn_t *dbn, double *input, double *expected_output) {
  dbn_pthread_arg_t pta;
  pta.dbn= dbn;
  pta.input= input;
  pta.expected_output= expected_output;
  pta.do_n_elements= dbn[0].batch_size;
  pta.batch= alloc_dwt_from_dbn(dbn);
  dbn_backprop_partial_minibatch(&pta);
  
  // Update the weights.
  for(int i=0;i<dbn[0].n_rbms;i++) {
    apply_delta_w(&(dbn[0].rbms[i]), pta.batch[i]);
  }
  free_delta_w_ptr(pta.batch, dbn[0].n_rbms);
}
/////////////\IF NO PTREADS, USE THIS. ///////////////////////////////////////////////

/////////////IF PTREADS, USE THIS. ///////////////////////////////////////////////
/* Runs the backpropagation algorithm over each element of a mini-batch. */
void backpropagation_minibatch_pthreads(dbn_t *dbn, double *input, double *expected_output, int n_threads) {

  // Activate each as a separate thread.
  dbn_pthread_arg_t *pta= (dbn_pthread_arg_t*)Calloc(n_threads, dbn_pthread_arg_t);
  pthread_t *threads= (pthread_t*)Calloc(n_threads, pthread_t);
  int n_per_batch= floor(dbn[0].batch_size/n_threads);
  int remainder= (dbn[0].batch_size%n_threads==0)?n_per_batch:(dbn[0].batch_size%n_threads);
  for(int i=0;i<n_threads;i++) {
    // Set up data passed to partial_minibatch()
    pta[i].dbn= dbn;
    pta[i].input= input;
    pta[i].expected_output= expected_output;
    pta[i].batch= alloc_dwt_from_dbn(dbn);
    pta[i].do_n_elements= (i<(n_threads-1))?n_per_batch:remainder; // For the last thread, only run remaining elements.
	  
    pthread_create(threads+i, NULL, dbn_backprop_partial_minibatch, (void*)(pta+i));
	
	// Increment pointers for the next thread.
	input+= pta[i].do_n_elements*dbn[0].rbms[0].n_inputs;
	expected_output+= pta[i].do_n_elements*dbn[0].rbms[dbn[0].n_rbms-1].n_outputs;
  }

  // Wait for threads to complete, and combine the data into a single vector.
  delta_w_t *batch;
  for(int i=0;i<n_threads;i++) {
    pthread_join(threads[i], NULL);
	
    if(i==0) {
      batch= pta[i].batch;
    }
    else {
      for(int j=0;j<dbn[0].n_rbms;j++) {
        sum_delta_w(batch, pta[i].batch);
      }
      free_delta_w_ptr(pta[i].batch, dbn[0].n_rbms);
    }
  }
  Free(pta); Free(threads);
 
  // Update the weights.
  for(int i=0;i<dbn[0].n_rbms;i++) {
    apply_delta_w(&(dbn[0].rbms[i]), batch[i]);
  }
  free_delta_w_ptr(batch, dbn[0].n_rbms);
}
/////////////\IF PTREADS, USE THIS. ///////////////////////////////////////////////

/*
 * Refines the deep belief network using backpropagation of error derivitives for a given number of epocs.
 *
 * Arguments: 
 *   dbn           --> Deep belief network -- full specification ...
 *   input_example --> Input example used for training.  Each columns represents a unique training case.
 *   output_example--> Output example used for training.  
 *   n_examples    --> Specifies the number of examples provided.
 *   n_epocs       --> Specifies the number of times the training program loops over the examples.
 *
 * Assumptions: 
 *   --> n_examples is a multiple of dbn.batch_size ... additional examples are ignored.
 */

void dbn_refine(dbn_t *dbn, double *input_example, double *output_example, int n_examples, int n_epocs, int n_threads) {
  double *current_input, *current_output;
  int n_training_iterations= floor(n_examples/dbn[0].batch_size); 
  
  for(int i=0;i<n_epocs;i++) {
    current_input= input_example; // Reset training pointer.
    current_output= output_example;
    for(int j=0;j<n_training_iterations;j++) {
      backpropagation_minibatch_pthreads(dbn, current_input, current_output, n_threads);  // Do a minibatch using the current position of the training pointer.
      current_input+= dbn[0].batch_size*dbn[0].rbms[0].n_inputs; // Increment the input_example pointer batch_size # of columns.
      current_output+=dbn[0].batch_size*dbn[0].rbms[dbn[0].n_rbms-1].n_outputs; // Increment the input_example pointer batch_size # of columns.
	}
  }
}
/*
 *  R interface to refining a deep belief network for discriminitive tasks using backpropagation ...
 */ 
SEXP backpropagation_dbn_R(SEXP dbn_r, SEXP training_data_r, SEXP training_labels_r, SEXP n_epocs_r, SEXP n_threads_r) {
  dbn_t *dbn= dbn_r_to_c(dbn_r); // Get values from R function.
  
  int n_examples= Rf_nrows(training_data_r)/dbn[0].rbms[0].n_inputs;
  int n_threads= INTEGER(n_threads_r)[0];
  double *input_examples= REAL(training_data_r);
  double *output_examples= REAL(training_labels_r);

  int n_epocs= INTEGER(n_epocs_r)[0];

  dbn_refine(dbn, input_examples, output_examples, n_examples, n_epocs, n_threads);
  
  return(dbn_r);
}

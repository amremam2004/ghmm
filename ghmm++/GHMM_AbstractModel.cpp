/*
  created: 05 Feb 2002 by Peter Pipenbacher
  authors: Peter Pipenbacher (pipenb@zpr.uni-koeln.de)
  file   : $Source$
  $Id$

  __copyright__
  
 */

#include <xmlio/XMLIO_Document.h>
#include "ghmm++/GHMM_AbstractModel.h"
#include "ghmm++/GHMM_Transition.h"
#include "ghmm++/GHMM_State.h"
#include "ghmm++/GHMM_GMLTransition.h"
#include "ghmm++/GHMM_GMLState.h"
#include "ghmm++/GHMM_Alphabet.h"


#ifdef HAVE_NAMESPACES
using namespace std;
#endif


GHMM_AbstractModel::GHMM_AbstractModel() {
  tag               = "hmm";
  xmlio_indent_type = XMLIO_INDENT_BOTH;
  node_tag          = "state";
  edge_tag          = "transition";
}


GHMM_AbstractModel::~GHMM_AbstractModel() {
  clean();
}


const char* GHMM_AbstractModel::toString() const {
  return "GHMM_AbstractModel";
}


void GHMM_AbstractModel::print(FILE *file) const {
}


int GHMM_AbstractModel::check() const {
  return 0;
}


void GHMM_AbstractModel::setNodeTag( const string &tag )       { node_tag = tag; }

void GHMM_AbstractModel::setTransitionTag( const string &tag ) { edge_tag = tag; }


void GHMM_AbstractModel::setTransition(GHMM_Transition* transition) {
  GHMM_State* source = getState(transition->source);
  GHMM_State* target = getState(transition->target);

  source->changeOutEdge(target->index,transition->prob);
  target->changeInEdge(source->index,transition->prob);
}


void GHMM_AbstractModel::setTransition(int start, int end, double prob) {
  GHMM_State* source = getState(start);
  GHMM_State* target = getState(end);

  source->changeOutEdge(target->index,prob);
  target->changeInEdge(source->index,prob);
}


void GHMM_AbstractModel::setTransition(const string& start, const string& end, double prob) {
  setTransition(getStateIndex(start),getStateIndex(end),prob);
}


GHMM_State* GHMM_AbstractModel::getState(const string& id) const {
  map<string,int>::const_iterator iter;
  iter = state_by_id.find(id);
  if (iter == state_by_id.end()) {
    fprintf(stderr,"State '%s' not found in model.\n",id.c_str());
    return NULL;
  }
  
  return states[iter->second];
}


GHMM_State* GHMM_AbstractModel::getState(int index) const {
  if (index >= (int) states.size()) {
    fprintf(stderr,"GHMM_AbstractModel::getState(int):\n");
    fprintf(stderr,"State no. %d does not exist. Model has %d states.\n",index,(int) states.size());
    exit(1);
  }

  return states[index];
}


int GHMM_AbstractModel::getNumberOfTransitionMatrices() const {
  return 0;
}


void GHMM_AbstractModel::clean() {
  unsigned int i;

  for (i = 0; i < states.size(); ++i)
    SAFE_DELETE(states[i]);
  states.clear();

  for (i = 0; i < transitions.size(); ++i)
    SAFE_DELETE(transitions[i]);
  transitions.clear();
}


int GHMM_AbstractModel::getStateIndex(const string& id) const {
  map<string,int>::const_iterator iter;

  iter = state_by_id.find(id);
  if (iter == state_by_id.end())
    return -1;

  return iter->second;
}


void GHMM_AbstractModel::stateIDChanged(const string& old_id, const string& new_id) {
  int index = state_by_id[old_id];

  state_by_id.erase(old_id);
  state_by_id[new_id] = index;
}


void GHMM_AbstractModel::addStateID(const string& new_id, int index) {
  state_by_id[new_id] = index;
}


const int GHMM_AbstractModel::XMLIO_writeContent(XMLIO_Document& writer) {
  int total_bytes = 0;
  int result;
  unsigned int i;

  writer.changeIndent(2);
  result = writer.writeEndl();

  if (result < 0)
    return result;
  total_bytes += result;

  /* write alphabet */
  if (getAlphabet()) {
    result = writer.writeElement(getAlphabet());

    if (result < 0)
      return result;
    total_bytes += result;

    result = writer.writef("\n\n");

    if (result < 0)
      return result;
    total_bytes += result;
  }

  /* write states */
  for (i = 0; i < states.size(); ++i) {
    if (i > 0) {
      result = writer.writeEndl();

      if (result < 0)
	return result;
      total_bytes += result;
    }

    result = writer.writeElement(states[i]);
  
    if (result < 0)
      return result;
    total_bytes += result;

    result = writer.writeEndl();

    if (result < 0)
      return result;
    total_bytes += result;
  }

  /* write transitions */
  createTransitions();
  for (i = 0; i < transitions.size(); ++i) {
    writer.writeEndl();
    writer.writeElement(transitions[i]);
    writer.writeEndl();
  }
  cleanTransitions();

  return 0;
}


void GHMM_AbstractModel::createTransitions() {
  unsigned int i;
  int edge;

  for (i = 0; i < states.size(); ++i)
    for (edge = 0; edge < states[i]->getOutEdges(); ++edge)
      transitions.push_back(states[i]->createTransition(edge));
}


void GHMM_AbstractModel::cleanTransitions() {
  unsigned int i;

  for (i = 0; i < transitions.size(); ++i)
    SAFE_DELETE(transitions[i]);
  transitions.clear();
}


GHMM_Alphabet* GHMM_AbstractModel::getAlphabet() const {
  return NULL;
}


XMLIO_Element* GHMM_AbstractModel::XMLIO_startTag(const string& tag, XMLIO_Attributes &attrs) {
  
  if ( node_tag == "state" && edge_tag == "transition" )
    {
      if (tag == node_tag) {
	GHMM_State* ghmm_state = new GHMM_State(this,states.size(),attrs);
	states.push_back(ghmm_state);
	
	state_by_id[ghmm_state->id] = states.size() - 1;
	
	/* Pass all nested elements to this state. */
	return ghmm_state;
      }

      if  (tag == edge_tag ) {
	GHMM_Transition* transition = new GHMM_Transition(attrs);
	transitions.push_back(transition);
	
	return transition;
      }
    }

  if ( node_tag == "node" && edge_tag == "edge" ) {
    if (tag == node_tag) {
      GHMM_State* ghmm_state = new GHMM_GMLState(this,states.size(),attrs);
      states.push_back(ghmm_state);
    
      state_by_id[ghmm_state->id] = states.size() - 1;
      
      /* Pass all nested elements to this state. */
      return ghmm_state;
    }

    if (tag == edge_tag ) {
      GHMM_Transition* transition = new GHMM_GMLTransition(attrs);
      transitions.push_back(transition);
    
      return transition;
    }
  }

  fprintf(stderr,"\t\ttag '%s' not recognized in hmm element\n",tag.c_str());
  exit(1);
  
  return NULL;
}


GHMM_ModelType GHMM_AbstractModel::getModelType() const {
  return (GHMM_ModelType) 0;
}

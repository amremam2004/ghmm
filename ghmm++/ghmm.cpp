/*
  author: Achim Gaedke
  created: 9. Juli 2001
  file: xmlio/examples/ghmm++/ghmm.cpp
  $Id$
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ghmm.h"
#include <xmlio/XMLIO_IgnoreObject.h>

#ifdef HAVE_NAMESPACES
using namespace std;
#endif

hmm::hmm(const string& tag, XMLIO_Attributes &attributes)
{
  /* do some initialisation */ 
  hmm_model=NULL;
  shmm_model=NULL;
  Initial=NULL;
  ghmm_Emissions=NULL;
  ghmm_graph=NULL;
  type='\0';

  /* read attributes */
  
  XMLIO_Attributes::const_iterator pos;
  /* madatory argument */  
  pos=attributes.find("type");
  if (pos!=attributes.end())
    {
      if (pos->second=="discrete")
	{
	  /* discrete type */
	  type='d';
	}
      else if (pos->second=="continuous")
	{
	  /* continuous type */
	  type='c';
	}
      else if (pos->second=="switched")
	{
	  /* switched type */
	  type='s';
	}
      else
	{
	  cerr<<toString()<<": unknown type "<<pos->second<<endl;
	}
    }
  else
    {
      /**/
      cerr<<toString()<<": type attribute missing"<<endl;
    }

  pos=attributes.find("prior");
  if (pos!=attributes.end())
    {
      double* tmp_prior_pointer=NULL;
      (void)XMLIO_evaluate_token(pos->second,0,pos->second.size(),tmp_prior_pointer);
      if (tmp_prior_pointer==NULL)
	{
	  cerr<<toString()<<": attribute value of prior is not a double"<<endl;
	  prior=1;
	}
      else
	{
	  prior=*tmp_prior_pointer;
	  delete tmp_prior_pointer;
	}
    }
}

XMLIO_Object* hmm::XMLIO_startTag(const string& tag, XMLIO_Attributes &attributes)
{
  /* what's next? */
  if (tag=="Graph")
    {
      if (ghmm_graph!=NULL)
	{
	  cerr<<toString()<<" only one graph expected"<<endl;
	  return new XMLIO_IgnoreObject();
	}
      else
	{
	  ghmm_graph=new Graph(tag,attributes);
	  return ghmm_graph;
	}
    }
  else if (tag=="InitialStates")
    {
      if (Initial!=NULL)
	{
	  cerr<<toString()<<": initial States allready existing, appending new section"<<endl;
	}
      else
	{
	  Initial=new InitialStates(tag,attributes);
	}
      return Initial;
    }
  else if (tag=="Alphabet")
    {
      cerr<<toString()<<": "<<tag<<" not implemented... DoItNOW"<<endl;
      return new XMLIO_IgnoreObject();      
    }
  else if (tag=="Emissions")
    {
      if (ghmm_Emissions!=NULL)
	{
	  cerr<<toString()<<" only one Emissions section expected"<<endl;
	  return new XMLIO_IgnoreObject();
	}
      else
	{
	  ghmm_Emissions=new Emissions(tag,attributes);
	  return ghmm_Emissions;
	}
    }
  else
    {
      cerr<<toString()<<": found unexpected element "<<tag<<", ignoring"<<endl;
      return new XMLIO_IgnoreObject();
    }
}

void hmm::XMLIO_endTag(const string& tag)
{
  /* not needed now */
}

void hmm::XMLIO_getCharacters(const string& characters)
{
  /* do nothing...*/
}

void hmm::XMLIO_finishedReading()
{
  /* done at graph...*/
  /* more needed here */
}


void hmm::print() const
{
  cout<<toString()<<":"<<endl;
  if (Initial!=NULL)
      Initial->print();
  else
      cout<<"InitialStates empty"<<endl;
  if (ghmm_graph!=NULL)
    ghmm_graph->print();
  else
    cout<<"no graph!"<<endl;
  if (ghmm_Emissions!=NULL)
    ghmm_Emissions->print();
  else
    cout<<"no emissions!"<<endl;
  cout<<"Alphabet: ToDoNOW"<<endl;
}

const char* hmm::toString() const
{
  return "hmm";
}

model* hmm::create_model() const
{
  /* model type mismatch */
  if (type!='d')
    return (model*)NULL;

  /* are necessary informations available? */
  if (ghmm_graph==NULL || Initial==NULL)
    {
      return (model*)NULL;
    }

  /* alpahabet and emissions */

  /* allocate model */
  model* return_model=(model*)malloc(sizeof(model));
  if (return_model==NULL)
    {
      cerr<<"could not allocate model structure"<<endl;
      return (model*)NULL;
    }
  
  return_model->N=ghmm_graph->vector<Node*>::size();   /* number of states */
  /* number of symbols */
  return_model->M=0; /* ToDo: Alphabet->vector<Emission*>::size() */
  return_model->prior=prior;

  /* allocate state array */
  return_model->s=(state*)malloc(sizeof(state)*return_model->N); /* state pointer array */
  if (return_model->s==NULL)
    {
      cerr<<"could not allocate states array"<<endl;
      return NULL;
    }

  /* initialise states */
  int state_counter=0;
  vector<Node*>::const_iterator pos=ghmm_graph->vector<Node*>::begin();
  while (pos!=ghmm_graph->vector<Node*>::end())
    {
      const string& state_id=(*pos)->get_id();
      state* this_state=&(return_model->s[state_counter]);
      /* emission probabilities */
      this_state->b=(double*)malloc(sizeof(double)*return_model->M);
      if (this_state->b==NULL)
	{
	  cerr<<"could not allocate emmision probabilities vector"<<endl;
	  return NULL;
	}

      /* Initial State probability */
      map<State*,double>* initial_state_map=Initial->get_map();
      map<State*,double>::iterator state_pos=initial_state_map->begin();
      while(state_pos!=initial_state_map->end() && (state_pos->first)->get_id()!=state_id)
	++state_pos;
      if (state_pos!=initial_state_map->end())
	this_state->pi=state_pos->second;
      else
	this_state->pi=0;

      /* transitions to_from */
      const set<int>& transition_idx=ghmm_graph->get_to_from_transitions(state_id);
      this_state->in_states=transition_idx.size(); /* number of incoming states */
      this_state->in_id=(int*)malloc(sizeof(int)*this_state->in_states); /* id of incoming states */
      this_state->in_a=(double*)malloc(sizeof(double)*this_state->in_states); /* prob of incoming states */
      size_t array_pos=0;
      set<int>::const_iterator tranisiton_pos=transition_idx.begin();
      while (tranisiton_pos!=transition_idx.end())
	{
	  this_state->in_id[array_pos]=*tranisiton_pos;
	  Edge* my_edge=((vector<Edge*>)*ghmm_graph)[*tranisiton_pos];
	  this_state->in_a[array_pos]=my_edge->get_weight();
	  ++tranisiton_pos;
	  array_pos++;
	}

      this_state->out_states=0; /* number of outgoing states */
      this_state->out_id=NULL; /* id of outgoing states */
      this_state->out_a=NULL; /* prob of outgoing states */

      ++pos;
      state_counter++;
    }

  return return_model;
}

const string& hmm::get_id() const
{
  return id;
}

smodel* hmm::create_smodel() const
{
  if (type!='c' || type!='s') return (smodel*)NULL;

  return NULL;
}


hmm::~hmm()
{
  SAFE_DELETE(ghmm_Emissions);
  SAFE_DELETE(ghmm_graph);
  SAFE_DELETE(Initial);
}

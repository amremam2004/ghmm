#!/usr/bin/env python
################################################################################
#
#       This file is part of Gato (Graph Algorithm Toolbox) 
#       version _VERSION_ from _BUILDDATE_. You can find more information at 
#       http://www.zpr.uni-koeln.de/~gato
#
#	file:   HMMEd.py
#	author: Alexander Schliep (schliep@zpr.uni-koeln.de)
#
#       Copyright (C) 1998-2002, Alexander Schliep, Winfried Hochstaettler and 
#       ZAIK/ZPR, Universitaet zu Koeln
#                                   
#       Contact: schliep@zpr.uni-koeln.de, wh@zpr.uni-koeln.de             
#
#       Information: http://gato.sf.net
#
#       This library is free software; you can redistribute it and/or
#       modify it under the terms of the GNU Library General Public
#       License as published by the Free Software Foundation; either
#       version 2 of the License, or (at your option) any later version.
#
#       This library is distributed in the hope that it will be useful,
#       but WITHOUT ANY WARRANTY; without even the implied warranty of
#       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#       Library General Public License for more details.
#
#       You should have received a copy of the GNU Library General Public
#       License along with this library; if not, write to the Free
#       Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
#
#
#       This file is version $Revision$ 
#                       from $Date$
#             last change by $Author$.
#
################################################################################

from DataStructures import Point2D
from Graph import Graph
from Gred import *
from GraphUtil import GraphInformer, VertexWeight
from GraphDisplay import GraphDisplay
from GatoUtil import stripPath, extension
from GatoGlobals import *
from GraphEditor import EditWeightsDialog
from tkFileDialog import askopenfilename, asksaveasfilename
import tkMessageBox
from tkMessageBox import askokcancel

import tkSimpleDialog 
import whrandom
import string
import types
import copy
import ProbEditorBasics
import ProbEditorDialogs

import xmlutil
from xmlutil import NamedDistributions, XMLElementWriter, toprettyxml, DiscreteHMMAlphabet, HMMClass, HMMState, HMM 

import EditObjectAttributesDialog
from EditObjectAttributesDialog import EditObjectAttributesDialog, ValidatingString, ValidatingInt, ValidatingFloat, PopupableInt, Probability, DefaultedInt, DefaultedString


from MapEditor import MapEditor, NamedCollectionEditor
import editobj, editobj.editor as editor


def typed_assign(var, val):
    result = type(var)(val)
    result.__dict__ = var.__dict__
    #result.__dict__ = copy.copy(var.__dict__)
    return result

def listFromCSV(s, type):
    return map(type,string.split(s,','))

def csvFromList(list, perRow = None):
    if perRow == None:
        return string.join(map(str,list), ', ')
    else:
        result = ""
        for start in xrange(0, len(list), perRow):
            result += string.join(map(str,list[start:start+perRow]), ', ') + ',\n'
        return result[0:len(result)-2]
        

class ValidStringEditor(editor.StringEditor):

    require_right_menu = 0

    def get_value(self):
        value = getattr(self.obj, self.attr, "")
        self.unicode = isinstance(value, unicode)
        return value
  
    def set_value(self, value):
        editor.Editor.set_value(self, ValidatingString(value))


class ProbabilityEditor(editor.FloatEditor):
    
    def get_value(self): return str(getattr(self.obj, self.attr, ""))
    
    def set_value(self, value):
        editor.Editor.set_value(self, Probability(editor.editobj.eval(value)))

class NamedDistributions(xmlutil.NamedDistributions ):

    def __init__(self, itsHMM):
        xmlutil.NamedDistributions.__init__(self, itsHMM)
        
    def editDistributions(self, master):        
        editor = NamedCollectionEditor(master, self)

    def names(self):
        return self.dist.keys()

    def add(self, name):
        order = tkSimpleDialog.askinteger("Distribution %s" % name, "Order", initialvalue=0)
        tmp = [1.0 / self.itsHMM.hmmAlphabet.size()] * self.itsHMM.hmmAlphabet.size()
        p = tmp * (self.itsHMM.hmmAlphabet.size() ** order)
        print "adding", name, order, p
        self.addDistribution(name, order, p)

    def delete(self, name):
        self.deleteDistribution(name)
        
    def edit(self, master, name):
        if self.order[name] != 0:
            print "Sorry, cannot edit higher order distributions yet"
        else:
            emission_probabilities = ProbEditorBasics.ProbDict({})
            
            for code in self.itsHMM.hmmAlphabet.name.keys():
                label = self.itsHMM.hmmAlphabet.name[code]
                weight = self.dist[name][code]
                emission_probabilities.update({label:weight})
                
            e = ProbEditorBasics.emission_data(emission_probabilities)
            d = ProbEditorDialogs.emission_dialog(master, e,
                                                  "background emission probs %s" % name)
            if d.success():
                # write back normalized probabilities
                for key in emission_probabilities.keys():
                    code = self.itsHMM.hmmAlphabet.name2code[key]
                    weight = self.dist[name][code] = emission_probabilities[key] / emission_probabilities.sum


class DiscreteHMMAlphabet( xmlutil.DiscreteHMMAlphabet ):
    def __init__(self):
        xmlutil.DiscreteHMMAlphabet.__init__(self)
        
    def size(self):
        return len(self.name.keys())

    def edit(self, master):        
        mapedit = MapEditor(master, [self.name], ['code','name'], [3,5])
        print mapedit.result
        if mapedit.result != None:
            
            new_keys = []
            for (code_str, name) in mapedit.result:
                code = int(code_str)
                self.name[code] = name
                self.name2code[name] = code
                new_keys.append(code)
            
            for key in self.name.keys():
                if key not in new_keys:
                    del self.name2code[self.name[key]]
                    del self.name[key] 
                else:
                    self.name2code[self.name[key]] = key

class HMMState( xmlutil.HMMState ):
    def __init__(self):
        xmlutil.HMMState.__init__(self)
  
    
class HMMClass( xmlutil.HMMClass ):
    def __init__(self):
        xmlutil.HMMClass.__init__(self)
	
    def edit(self, master):        
        mapedit = MapEditor(master, [self.name, self.desc], ['code','name','desc'], [3,5,35])
        print mapedit.result
        if mapedit.result != None:
            
            new_keys = []
            for (code_str, name, desc) in mapedit.result:
                code = int(code_str)
                self.name[code] = name
                self.desc[code] = desc
                self.name2code[name] = code
                new_keys.append(code)
            
            for key in self.name.keys():
                if key not in new_keys:
                    del self.name2code[self.name[key]]
                    del self.name[key] 
                    del self.desc[key]
                else:
                    self.name2code[self.name[key]] = key

class HMM (xmlutil.HMM):

    def __init__(self, itsEditor, XMLFileName = None):
        self.itsEditor = itsEditor
        xmlutil.HMM.__init__(self, XMLFileName)
        
class HMMEditor(SAGraphEditor):

    def __init__(self, master=None):
	SAGraphEditor.__init__(self, master)	
	self.HMM = None

    def CreateWidgets(self):

        toolbar = Frame(self, cursor='hand2', relief=FLAT)
        toolbar.pack(side=LEFT, fill=Y) # Allows horizontal growth

        extra = Frame(toolbar, cursor='hand2', relief=SUNKEN, borderwidth=2)
        extra.pack(side=TOP) # Allows horizontal growth
        extra.rowconfigure(6,weight=1)
        extra.bind("<Enter>", lambda e, gd=self:gd.DefaultInfo())

        px = 0 
        py = 3 

        self.toolVar = StringVar()

        import GatoIcons
        # Load Icons
        self.vertexIcon = PhotoImage(data=GatoIcons.vertex)
        self.edgeIcon   = PhotoImage(data=GatoIcons.edge)
        self.deleteIcon = PhotoImage(data=GatoIcons.delete)
        self.swapIcon   = PhotoImage(data=GatoIcons.swap)
        self.editIcon   = PhotoImage(data=GatoIcons.edit)
        self.propIcon   = PhotoImage(data=GatoIcons.edit)
        
        b = Radiobutton(extra, width=32, padx=px, pady=py, 
                        text='Add or move vertex',  
                        command=self.ChangeTool,
                        var = self.toolVar, value='AddOrMoveVertex', 
                        indicator=0, image=self.vertexIcon)
        b.grid(row=0, column=0, padx=2, pady=2)
        b.bind("<Enter>", lambda e, gd=self:gd.UpdateInfo('Add or move vertex'))
        self.defaultButton = b # default doesnt work as config option


        b = Radiobutton(extra, width=32, padx=px, pady=py, 
                        text='Add edge', 
                        command=self.ChangeTool,
                        var = self.toolVar, value='AddEdge', indicator=0,
                        image=self.edgeIcon)
        b.grid(row=1, column=0, padx=2, pady=2)
        b.bind("<Enter>", lambda e, gd=self:gd.UpdateInfo('Add edge'))


        b = Radiobutton(extra, width=32, padx=px, pady=py, 
                        text='Delete edge or vertex', 
                        command=self.ChangeTool,
                        var = self.toolVar, value='DeleteEdgeOrVertex', indicator=0,
                        image=self.deleteIcon)
        b.grid(row=2, column=0, padx=2, pady=2)
        b.bind("<Enter>", lambda e, gd=self:gd.UpdateInfo('Delete edge or vertex'))


        b = Radiobutton(extra, width=32, padx=px, pady=py, 
                        text='Swap orientation', 
                        command=self.ChangeTool,
                        var = self.toolVar, value='SwapOrientation', indicator=0,
                        image=self.swapIcon)
        b.grid(row=3, column=0, padx=2, pady=2)
        b.bind("<Enter>", lambda e, gd=self:gd.UpdateInfo('Swap orientation'))


        b = Radiobutton(extra, width=32, padx=px, pady=py, 
                        text='Edit Weight', 
                        command=self.ChangeTool,
                        var = self.toolVar, value='EditWeight', indicator=0,
                        image=self.editIcon)
        b.grid(row=4, column=0, padx=2, pady=2)
        b.bind("<Enter>", lambda e, gd=self:gd.UpdateInfo('Edit Weight'))

        b = Radiobutton(extra, width=32, padx=px, pady=py, 
                        text='Edit Properties', 
                        command=self.ChangeTool,
                        var = self.toolVar, value='EditProperties', indicator=0,
                        image=self.editIcon)
        b.grid(row=5, column=0, padx=2, pady=2)
        b.bind("<Enter>", lambda e, gd=self:gd.UpdateInfo('Edit Properties'))

        # disable the EditProperties button
        #b = Radiobutton(extra, width=32, padx=px, pady=py, 
        #                text='Edit State', 
        #                command=self.ChangeTool,
        #                var = self.toolVar, value='EditState', indicator=0,
        #               image=self.editIcon)
        #b.grid(row=6, column=0, padx=2, pady=2)
        #b.bind("<Enter>", lambda e, gd=self:gd.UpdateInfo('Edit State'))

        GraphEditor.CreateWidgets(self)


    #----- Tools Menu callbacks
    def ChangeTool(self):
        self.SetEditMode(self.toolVar.get())

    def MouseUp(self,event):
	if self.mode == 'AddOrMoveVertex':
	    self.AddOrMoveVertexUp(event)
	elif self.mode == 'AddEdge':
	    self.AddEdgeUp(event)
	elif self.mode == 'DeleteEdgeOrVertex':
	    self.DeleteEdgeOrVertexUp(event)
	elif self.mode == 'SwapOrientation':
	    self.SwapOrientationUp(event)
	elif self.mode == 'EditWeight':
	    self.EditWeightUp(event)
	elif self.mode == 'EditProperties':
	    self.EditPropertiesUp(event)
        elif self.mode == 'EditState': # test EditObj
            self.EditStateUp(event)

    def makeMenuBar(self):
	self.menubar = Menu(self,tearoff=0)

	# Add file menu
	self.fileMenu = Menu(self.menubar, tearoff=0)
	self.fileMenu.add_command(label='New',            command=self.NewGraph)
	self.fileMenu.add_command(label='Open ...',       command=self.OpenGraph)
	self.fileMenu.add_command(label='Save',	     command=self.SaveGraph)
	self.fileMenu.add_command(label='Save as ...',    command=self.SaveAsGraph)
	self.fileMenu.add_separator()
	self.fileMenu.add_command(label='Export EPSF...', command=self.ExportEPSF)
	self.fileMenu.add_separator()
	self.fileMenu.add_command(label='Quit',	     command=self.Quit)
	self.menubar.add_cascade(label="File", menu=self.fileMenu, 
				 underline=0)
	
	self.graphMenu = Menu(self.menubar, tearoff=0)
	self.graphMenu.add_command(label='Edit HMM', command=self.EditHMM)
	self.graphMenu.add_command(label='Edit Class label', command=self.EditClassLabel)
        # XXX Note if we change alphabet, we have to change all emissions 
	#self.graphMenu.add_command(label='Edit Alphabet', command=self.EditAlphabet)
	#self.graphMenu.add_command(label='Edit Emissions', command=self.EditEmissions)
	self.graphMenu.add_command(label='Edit Prior', command=self.EditPrior)
	self.graphMenu.add_command(label='Edit Background Distributions', command=self.EditBackgroundDistributions)
	self.graphMenu.add_separator()
	self.graphMenu.add_checkbutton(label='Grid', 
						  command=self.ToggleGridding)	
	self.menubar.add_cascade(label="HMM", menu=self.graphMenu, 
				 underline=0)

	self.master.configure(menu=self.menubar)

    def SetGraphMenuOptions(self):
	if not self.gridding:
	    self.graphMenu.invoke(self.graphMenu.index('Grid'))	
	

    ############################################################
    #
    # Menu Commands
    #
    # The menu commands are passed as call back parameters to 
    # the menu items.
    #
    def NewGraph(self, nrOfSymbols=0, newInput=1):
	self.HMM = HMM(self)

	self.HMM.G.euclidian = 0
	self.HMM.G.directed = 1
	self.HMM.G.simple = 0
	self.graphName = "New"
	self.ShowGraph(self.HMM.G,self.graphName)
	self.RegisterGraphInformer(HMMInformer(self.HMM))
	self.fileName = None
	self.SetTitle("HMMEd _VERSION_ - New Graph")
	self.SetGraphMenuOptions()
        if newInput:
            if nrOfSymbols == 0:
                nrOfSymbols  = tkSimpleDialog.askinteger("New HMM",
                                                         "Enter the number of output symbols")
                for i in xrange(nrOfSymbols):
                    self.HMM.G.vertexWeights[i] = VertexWeight(0.0)
		
		self.HMM.hmmAlphabet.buildAlphabets(nrOfSymbols)
		
    def OpenGraph(self):

        self.DeleteDrawItems() # clear screen
	if self.hasGraph == 1:
	    self.HMM.G.Clear()
	    self.hasGraph = 0
	    
	  
	file = askopenfilename(title="Open HMM",
			       defaultextension=".xml",
			       filetypes = (("XML", ".xml"),
                                            )
			       )
	if file is "": 
	    print "cancelled"
	else:
	    self.fileName = file
	    self.graphName = stripPath(file)
	    e = extension(file)
            
	    if e == 'xml':
		self.HMM.OpenXML(file)
	    else:
		print "Unknown extension"
		return

	    self.ShowGraph(self.HMM.G, self.graphName)
	    self.RegisterGraphInformer(HMMInformer(self.HMM))
	    self.SetTitle("HMMEd _VERSION_ - " + self.graphName)

	    if not self.gridding:
		self.graphMenu.invoke(self.graphMenu.index('Grid'))	


    def SaveGraph(self):
	if self.fileName != None:
	    self.HMM.SaveAs(self.fileName)
	else:
	    self.SaveAsGraph()
	

    def SaveAsGraph(self):
	file = asksaveasfilename(title="Save HMM",
				 defaultextension=".xml",
				 filetypes = ( ("XML", ".xml"), ("GHMM", ".ghmm"), )
                                 )
	if file is "": 
	    print "cancelled"
	else:
            print file
            ext = string.lower(os.path.splitext(file)[1])
            if ext == '.xml':
                self.fileName = file
		try:
		    self.HMM.SaveAs(file)
		    self.graphName = stripPath(file)
		    self.SetTitle("HMMEd _VERSION_ - " + self.graphName)
		except:
		    print "Cannot save due to error in the model: initial or alphabets."
            else:
                self.fileName = file
                self.HMM.SaveAsGHMM(file)
                self.graphName = stripPath(file)
                self.SetTitle("HMMEd _VERSION_ - " + self.graphName)
                

    def EditWeightUp(self,event):
        """ Need to have editors multiple sets of transition probability """
	if event.widget.find_withtag(CURRENT):
	    widget = event.widget.find_withtag(CURRENT)[0]
	    tags = self.canvas.gettags(widget)
	    if "edges" in tags:
 		(tail,head) = self.edge[widget]

                transition_probabilities=ProbEditorBasics.ProbDict({})
		for head in self.HMM.G.OutNeighbors(tail):
		    weight=self.HMM.G.edgeWeights[0][(tail,head)]
		    label = "-> %d" % head
                    transition_probabilities.update({label:weight})

                if transition_probabilities.sum==0:
                    key_list=transition_probabilities.keys()
                    for key in key_list:
                        transition_probabilities[key]=1.0/len(key_list)
                e=ProbEditorBasics.emission_data(transition_probabilities)
                d=ProbEditorDialogs.emission_dialog(self,
                                                    e,
                                                    "transition probs from state %d" % tail)
                if d.success():
                    # write back normalized probabilities
                    for key in transition_probabilities.keys():
                        head = int(key[3:])
                        self.HMM.G.edgeWeights[0][(tail,head)]=transition_probabilities[key]/transition_probabilities.sum

	    else: # We have a vertex
		v = self.FindVertex(event)
		if v != None:
                    state = self.HMM.state[v]
                    if state.order > 0:
                        print "Ooops. Cant edit higher order states"
                        return
                    
                    if state.tiedto != '':
                        msg = "The emission parameters of state %s you attempted to edit are tied to those of state %s." %  (state.id, state.tiedto)
                        #print "Note:", msg
                        if not askokcancel("Edit Tied State", msg + "Edit those of state %s instead?" % state.tiedto):
                            return
                        else:
                            state = self.HMM.state[self.HMM.id2index[state.tiedto]]

                    if state.emissions == []:
                        state.emissions = [1.0 / self.HMM.hmmAlphabet.size()] * self.HMM.hmmAlphabet.size()
                    emission_probabilities = ProbEditorBasics.ProbDict({})

                    for code in self.HMM.hmmAlphabet.name.keys():
                        label = self.HMM.hmmAlphabet.name[code]
                        weight = state.emissions[code-1]
                        emission_probabilities.update({label:weight})
                        
                    # Normalize ... should be member function
                    if abs(emission_probabilities.sum - 1.0) > 0.01:
                        key_list = emission_probabilities.keys()
                        for key in key_list:
                            emission_probabilities[key] = 1.0 / len(key_list)

                            
                    e = ProbEditorBasics.emission_data(emission_probabilities)
                    d = ProbEditorDialogs.emission_dialog(self, e, "emission probs of state %s" % state.id)
                    if d.success():
                        # write back normalized probabilities
                        for key in emission_probabilities.keys():
                            code = self.HMM.hmmAlphabet.name2code[key]
                            state.emissions[code] = emission_probabilities[key] / emission_probabilities.sum	

    def EditStateUp(self,event):
        print 'Calling EditObj'
	if event.widget.find_withtag(CURRENT):
	    widget = event.widget.find_withtag(CURRENT)[0]
	    tags = self.canvas.gettags(widget)
	    if not "edges" in tags:
		v = self.FindVertex(event)
                # print "Found Vertex " + "%s" % v

                # hidden attributes set to None
                editor.register_attr("state_class", None)
                editor.register_attr("emissions", None) # hidden
                editor.register_attr("itsHMM", None)
                editor.register_attr("desc", None)
                editor.register_attr("index", None)
                editor.register_attr("pos", None)
                editor.register_attr("id", None)
                editor.register_attr("order", None)
                editor.register_attr("tiedto", None)
                editor.register_attr("reading_frame", None)
                editor.register_attr("duration", None)
                editor.register_attr("background", None)
                
                # register values
                editor.register_attr("label", ValidStringEditor)
                editor.register_attr("initial", ProbabilityEditor)
                
                # Edit this State with editobj widget  
                editobj.edit(self.HMM.state[v])


    def EditPropertiesUp(self,event):
	if event.widget.find_withtag(CURRENT):
	    widget = event.widget.find_withtag(CURRENT)[0]
	    tags = self.canvas.gettags(widget)
	    if not "edges" in tags:
		v = self.FindVertex(event)
                # print "Found Vertex " + "%s" % v
                d = EditObjectAttributesDialog(self, self.HMM.state[v], HMMState.editableAttr)

                # We only show the label out of the editable items
                self.HMM.G.labeling[v] = ValidatingString("%s" % (self.HMM.state[v].label)) # XXX Hack Aaaargh!
                self.UpdateVertexLabel(v, 0)
                self.HMM.id2index[self.HMM.state[v].id] = v

                

    def EditHMM(self):
        d = EditObjectAttributesDialog(self, self.HMM, self.HMM.editableAttr['HMM'])

    def EditClassLabel(self):
        self.HMM.hmmClass.edit(self)

    def EditAlphabet(self):
        self.HMM.hmmAlphabet.edit(self)
        
    def EditPrior(self):
	if self.HMM.G.Order() == 0:
	    return
        
        emission_probabilities = ProbEditorBasics.ProbDict({})
        for state in self.HMM.state.values():
            label = state.id
            weight = state.initial
            emission_probabilities.update({label:weight})

        e = ProbEditorBasics.emission_data(emission_probabilities)
        d = ProbEditorDialogs.emission_dialog(self, e, "initial probabilities")
        u = 1.0 / len(emission_probabilities.keys())
        
        if d.success():
            # write back normalized probabilities
            for key in emission_probabilities.keys():
                state = self.HMM.state[self.HMM.id2index[key]]
                if emission_probabilities.sum == 0.0:
                    state.initial = typed_assign(state.initial, u)
                else:
                    state.initial = typed_assign(state.initial,
                                                 emission_probabilities[key] / emission_probabilities.sum)
    def EditBackgroundDistributions(self):
        self.HMM.backgroundDistributions.editDistributions(self)
    
    def AddVertexCanvas(self,x,y):
	v = GraphDisplay.AddVertexCanvas(self, x, y)
        print "AddVertex ", v, "at ",x,y
        self.HMM.AddState(v)
	state = self.HMM.state[v]
	self.HMM.G.embedding[state.index] = self.G.embedding[v]
	
    def MoveVertex(self,v,x,y,doUpdate=None):
	GraphDisplay.MoveVertex(self, v,x,y,doUpdate)
	state = self.HMM.state[v] # transfer the coordinate
	self.HMM.G.embedding[state.index] = self.G.embedding[v]

    def AddEdge(self,tail,head):
        GraphDisplay.AddEdge(self,tail,head)
        self.HMM.G.edgeWeights[0][(tail, head)] = 1.0
	
    def DeleteVertex(self,v):
        self.HMM.DeleteState(v)
        SAGraphEditor.DeleteVertex(self,v)

    def ShowCoords(self,event):
        pass

    
class HMMInformer(GraphInformer):
    def __init__(self, itsHMM):
        GraphInformer.__init__(self, itsHMM.G)
        self.itsHMM = itsHMM

    def VertexInfo(self,v):
	""" v is the vertex id on the canvas, mapping to internal representation must be done from v->index """
	state = self.itsHMM.state[v]
	try:
	    self.itsHMM.hmmClass.name[state.state_class]
	    msg = "State '%s' class=%s (%s:%s) order=%d" % (state.id, state.state_class,
							    self.itsHMM.hmmClass.name[state.state_class],
							    self.itsHMM.hmmClass.desc[state.state_class],
							    state.order)
	except:
	    msg = "State '%s' class=%s" % (state.id, state.state_class)
							    
        if state.order == 0 and state.emissions != []:
            emsg = ""
            for e in state.emissions:
                emsg += " %0.3f " % e
            msg += ":[ %s ]" % emsg
        return msg
    

    def EdgeInfo(self,tail,head):
        """ Display a list of transition probabilities for this edge
            (We only draw the edge once, not as many as a number of classes)
        """
	
        tail_state = self.itsHMM.state[tail]        
        head_state = self.itsHMM.state[head]
        # p = self.itsHMM.G.edgeWeights[0][(tail, head)]
        transitions  = []
	if self.itsHMM.hmmClass.size > 1:
	    nr_classes = int(self.itsHMM.hmmClass.high())-int(self.itsHMM.hmmClass.low())+1
	    for cl in range(nr_classes):
		if self.itsHMM.G.edgeWeights[cl].has_key( (tail,head) ):
		    transitions.append(self.itsHMM.G.edgeWeights[cl][(tail,head)])
		else:
		    transitions.append(0.0)

	    plist = csvFromList( transitions )
	    msg = "Transition: '%s' -> '%s'(%2d):" % (tail_state.id, head_state.id, nr_classes)
	    msg = msg + plist
	else:
	    msg = "Transition: '%s' -> '%s'" % (tail_state.id, head_state.id)
        return msg
       
        
    
        
################################################################################
if __name__ == '__main__':
    
    # Make HMMState available in the EditObj evaluation environment
    editobj.EVAL_ENV["HMMState"] = HMMState
    
    graphEditor = HMMEditor(Tk())
    graphEditor.NewGraph(2)
    graphEditor.mainloop()

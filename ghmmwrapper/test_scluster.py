from new_ghmmex import *

gsl_rng_init() #Init random number generator

#smodel_run_clustering("test2.smo","test10.sqd")

# initializing clustering environment
res = scluster_env("test2.smo","test10.sqd")

change = 1

# two iterations of clustering
for i in range(2):
	change = res.cluster_onestep()

# adding model to clustering	
print "***** adding 1 model"
m = SHMM("test_add.smo") # reading .smo file into SHMM object
res.add_SHMM_model(m,0.2)  

# two iterations of clustering
for i in range(2):
	change = res.cluster_onestep(1)

# adding sequences to clustering
print "***** adding 10 sequences"
s2 = sequence_d(seq_d_read("test_add.sqd")) # reading .sqd file into sequence_d object
res.add_sequence_d(s2)
print "new number of sequences " + str(res.seq.seq_number)


# two iterations of clustering
for i in range(2):
	change = res.cluster_onestep(1)

# Output
# printing clustering to STDOUT
#print res.sclustering 
# saving results in files clust1.
#scluster_print(res.sclustering.scluster_t, res.seq,["clust1","clust1","clust1","clust1"])

print "***** removing model with ID 1"
res.remove_model(1)

# two iterations of clustering
for i in range(2):
	change = res.cluster_onestep(1)


# distance between models
# extracting pointers to smodels out of scluster_t struct
m0 = get_smodel_ptr(res.sclustering.scluster_t.smo,0) 
m1 = get_smodel_ptr(res.sclustering.scluster_t.smo,1)

#s1 = SHMM("blae",0,m0,1)  # initializing Python SHMM object from smodel pointers
#s2 = SHMM("blae",0,m1,1)  #  m0 and m1
#print s1  # printing out these models for testing purposes
#print s2

print "***** calculating model distance"
d = prob_distance_smodel(m0,m1,2000) # (third argument is sequence lenght)
print "distance(m0,m1) = " + str(d)

d2 = prob_distance_smodel(m1,m0,2000) # (third argument is sequence lenght)
print "distance(m1,m0) = " + str(d2)

d3 = smodel_prob_distance(m1,m0,2000,1,0) # (third argument is sequence lenght, optional fourth argument is flag for symmetric distance)
print "symmetric distance(m1,m0) = " + str(d3)

# merging two models
print "\n\n*** adding yet another model"
m = SHMM("test.smo") # reading .smo file into SHMM object
res.add_SHMM_model(m,0.15)

# two iterations of clustering
for i in range(2):
	change = res.cluster_onestep(1)

print "***  merging models with IDs 0 and 2"
res.merge_models(0,2)

# two iterations of clustering
for i in range(2):
	change = res.cluster_onestep(1)

# deviating models
res.deviate_model(0, 0.001, 0.001, 0.001,  0.001)
# arguments are: (	  model ID, 
#	 				  deviation of transitions,
#					  deviation of means,
#					  deviation of variances,
#					  deviation of weights,
#				 )
# deviation d means new_value = old_value + x, where x is randomly from [-d,d]



# freeing memory  
#print "freeing ..."
#res.free_scluster()  



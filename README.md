# gng.R

Fork of (<a href="https://github.com/gmum/gmum.r">gmum.R</a>) adapting the existing Growing Neural Gas algorithm for high dimensions and model sizes through the usage of Approximate Nearest Neighbour techniques. It internally uses an <a href="https://github.com/andrusha97/online-hnsw">online variant of HNSW</a>

Package includes:
* Growing Neural Gas clustering algorithm from the original authors
* Approximate Growing Neural Gas clustering algorithm

## Links
* Installation: [src/README.md](src/README.md)

## Growing Neural Gas

Maintained subpackage containing the original GNG algorithm and the Approximate GNG Algorithm. Both produce topological graphs that can be easily convert to igraph or dumped to file and loaded in the future. The original GNG features and documentation remain the same.


### Example: cluster sift1M dataset with Approximate GNG

In this example we construct a clustering of the <a href="http://corpus-texmex.irisa.fr/">SIFT1M dataset</a>. 
```R
library(gmum.r)

# Load data
# Already standardized and in csv form
sift1M_norm <- read.csv("sift1M_data.csv")

# Train in an offline manner
# Cluster SIFT1M into 1K nodes over 1K iterations
gng <- ApproximateGNG(sift1M_norm , max.nodes=1000, max.iter=1000, max_links = 16, efSearch = 16, efConstruction = 32)

# Train in an online manner
gng <- ApproximateGNG(train.online = TRUE, dim=ncol(sift1M_norm), max.nodes=1000, max_links = 16, efSearch = 16, efConstruction = 32)
insertExamples(gng, sift1M_norm)
run(gng)
Sys.sleep(100)
pause(gng)

# After training confirm construction recal ~= 0.95, if not, retrain with greater efConstruction parameter
# estimation of construction recall can be done with
gng$getConstructionRecall(-1)
```

Usage is much the same as the original GNG but several performance indicators and QoL changes were added
```R
# Get Average Path Length of the HNSW Algorithm
gng$getHNSWAvgPathLength(gng$getNumberNodes())

# If recall parameter was set to TRUE during construction, we can get the 2NN search recall with
gng$getKey1Recall()
gng$getKey2Recall()

# Number of operations made
gng$getNRemovals()
gng$getNMoves()
gng$getNInsertions()
gng$getNSearches()

# Quantization Error
getQuantizationError(gng, sift1M_norm)

# Get Clustering dataframe (#nodes x dim)
getNodes(gng)
```

## Approximate GNG Hyperparameters
Hyperparams specific to Approximate Growing Neural Gas are mostly related to those of HNSW
* `max_links` Defines number of links created for a new node of the HNSW graph upon insertion. 2-100 is a reasonable range. defining it to be higher works best on datasets with high intrinsic dimensionality or when high recall is needed
* `efConstruction` 	
Can be seen as controlling the trade-off between index construction time and index quality of the HNSW. Should be increased until we achieve at least 95% construction recall
* `efSearch` Higher efSearch translates to better search recall but slower search times. Controls how approximate we can allow the ANN step (and consequently the GNG model) to be in exchange for slower GNG time
* `nsw` Allows to constrict the HNSW to just 1 level, making it behave like the NSW algorithm
* `recall` Tells the algorithm to calculate search recall during the process. Severely impacts GNG time. Useful for performance testing and debugging.

## Performance as model size increases
![Performance](https://github.com/Silveryu/gng.r/blob/master/R/Figures/approxGNG_time_nnodes.png)





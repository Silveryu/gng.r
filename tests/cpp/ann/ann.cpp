//
// Created by silveryu on 10/11/19.
//


#include "gng/gng.h"
#include "gng/gng_server.h"
#include "utils/utils.h"

#include "gtest/gtest.h"

#include <algorithm>
#include <utility>
#include <vector>
#include <cmath>
#include <benchmark/benchmark.h>

using namespace std;
using namespace gmum;
using namespace std::chrono;


double random_double(double min, double max) {
    return min + (max - min) * ((double) rand()) / RAND_MAX;
}

double* set_up(unsigned int dim = 10, unsigned int nExamples = 100){

    cerr << "Allocating " << dim * nExamples << endl << flush;
    // creating random samples
    double* vect = new double[(dim) * nExamples];

    for (int i = 0; i < nExamples; ++i) {
        for (int j = 0; j <= dim; ++j)
            if (j == 0)
                vect[j + (i) * (dim)] = 0.0;
            else if (j < dim)
                vect[j + (i) * (dim)] = random_double(0, 1);
    }
    return vect;
}

int main() {

    unsigned int dim = 128;
    unsigned int nExamples = 100;
//min iters is 100
    int max_iter = 100;

    double *vect = set_up(dim, nExamples);



    auto* ann = new hnsw::hnsw_index<long , std::vector<double>, hnsw::l2_square_distance_t>();

    ann->options.max_links = 16;
    ann->options.ef_construction = 32;
    for(int i = 0; i < nExamples; ++i) {
        ann->insert(i, std::vector<double>(vect + i*dim, vect + i*dim + dim));
    }


    cout << "avg: " <<  ann->get_average_path_length();

}




#include "gng/gng.h"
#include "gng/gng_server.h"
#include "utils/utils.h"

#include "gtest/gtest.h"

#include "matplotlibcpp.h"

#include <algorithm>
#include <utility>
#include <vector>
#include <cmath>
#include <benchmark/benchmark.h>

using namespace std;
using namespace gmum;
using namespace std::chrono;

enum algorithms{
    ONLINE_HNSW,
    UNIFORM_GRID,
    LINEAR
};

template<typename T>
void vec_print(std::vector<T> const &v)
{
    for (auto i: v) {
        std::cout << i << ' ';
    }
}

float random_float(float min, float max) {
    return min + (max - min) * ((float) rand()) / RAND_MAX;
}

float* set_up(unsigned int dim = 100, unsigned int nExamples = 1000){

    cerr << "Allocating " << dim * nExamples << endl << flush;
    // creating random samples
    float* vect = new float[(dim) * nExamples];

    for (int i = 0; i < nExamples; ++i) {
        for (int j = 0; j <= dim; ++j)
            if (j == 0)
                vect[j + (i) * (dim)] = 0.0;
            else if (j < dim)
                vect[j + (i) * (dim)] = random_float(0, 1);
    }
    return vect;
}


// Adapted from R code to allow C++ profiling
void server(GNGServer& gng, float * vect, unsigned int dim, unsigned int nExamples,
            float min_relative_diff = 1e-3){

    const GNGConfiguration config = gng.getConfiguration();

    int iter = 0;
    int previous_iter = -1;
    float best_previous_error = -1;
    int error_index = -1;
    float current_best = 1e10;
    int initial_patience = 3;
    int patience = initial_patience;
    int max_iter = config.max_iter;

    gng.insertExamples(vect, nullptr, nullptr, nExamples, dim);
    gng.run();

    try {
        while(gng.getCurrentIteration() == 0 || gng.isRunning()){}

        while(iter == 0 || gng.isRunning()){
            sleep(100);
            iter = gng.getCurrentIteration();

            if(previous_iter != iter && iter % (max_iter / 100) == 0){
                cout << "Iteration " << iter << endl;
            }

            vector<float> error = gng.getMeanErrorStatistics();
            if(error.size() > 5 ) {

                best_previous_error = *std::min_element(std::end(error) - 5, std::end(error));

                if (error_index - gng.getGNGErrorIndex() > 4 &&
                    (current_best - best_previous_error) < current_best * min_relative_diff) {
                    patience--;
                    if (patience <= 0) {
                        cout << "Best error during training: " << current_best;
                        cout << "Best error in 5 previous iterations " << best_previous_error;
                        vector<float> sample(std::end(error) - 5, std::end(error));
                        vec_print(sample);
                    }
                } else {
                    patience = initial_patience;
                }

                error_index = gng.getGNGErrorIndex();
                current_best = min(best_previous_error, current_best);

            }
        }

        previous_iter = iter;

        if(gng.isRunning()){
            gng.terminate();
        }

        gng.getAlgorithm().updateClustering();

    }catch(const std::exception& e){
        cerr << "gngServer terminated:\n" <<  e.what() << endl;
        if(gng.isRunning()){
            gng.terminate();
        }
    }
}


static void TestBenchmark(benchmark::State& state, algorithms alg) {
    unsigned int dim = 100;
    unsigned int nExamples = 100;
    //min iters is 100
    int max_iter = 100;

    float* vect = set_up(dim, nExamples);
    GNGConfiguration config = GNGConfiguration::getDefaultConfiguration();
    config.datasetType = GNGConfiguration::DatasetSeq;

    switch(alg){
        case ONLINE_HNSW:
            config.uniformgrid_optimization = false;
            config.ann_optimization = true;
            config.ann_approach = GNGConfiguration::ONLINE_HNSW;
            break;
        case UNIFORM_GRID:
            config.uniformgrid_optimization = true;
            config.ann_optimization = false;
            break;
        default:
            config.uniformgrid_optimization = false;
            config.ann_optimization = false;

    }

    // max iter ensures it trains in offline mode
    config.max_iter = max_iter;
    config.dim = dim;

    GNGServer gng(&config);

    for (auto _ : state) {
        cout << "startng "<< endl;
        server(gng, vect, dim, nExamples);
        cout << "endng "<< endl;
    }

    state.counters.insert({{"dim", dim}, {"nExamples", nExamples}, {"max_iter", max_iter}, {"quantError", gng.getMeanError()}});
}


BENCHMARK_CAPTURE(TestBenchmark, online_hnsw, algorithms::ONLINE_HNSW)->Unit(benchmark::kMillisecond)->Iterations(1);

BENCHMARK_CAPTURE(TestBenchmark, linear, algorithms::LINEAR)->Unit(benchmark::kMillisecond)->Iterations(1);




BENCHMARK_MAIN();



/*
TEST(GNGNumericTest, ANNperf){


    //namespace plt = matplotlibcpp;
    //plt::plot(gng.getMeanErrorStatistics());
    //plt::title("Uniform grid optimization learning curve");
    //plt::ylabel("Quantization error");
   // plt::show();

   // ASSERT_GE(fabs(results.first), 550.0);
    // ASSERT_LE(fabs(results.second), 1e-2);
}

TEST(GNGNumericTest, Origperf){

    //namespace plt = matplotlibcpp;
    //plt::plot(gng.getMeanErrorStatistics());
    //plt::title("Uniform grid optimization learning curve");
    //plt::ylabel("Quantization error");
    // plt::show();

    // ASSERT_GE(fabs(results.first), 550.0);
    // ASSERT_LE(fabs(results.second), 1e-2);
}

//  500 dims :   31.1577 error, 335 nodes -> nor
// 500 dims 41.7867, 15 nodes -> ann

// 10 000 dims : 849.097, 13  -> ann
// 10 000 dims : 829.137, 54 -> nor*/

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

enum algorithms{
    ONLINE_HNSW,
    ONLINE_HSNW_MV,
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


// Adapted from R code to allow C++ profiling
void server(GNGServer& gng, double * vect, unsigned int dim, unsigned int nExamples,
            double min_relative_diff = 1e-3){
    const GNGConfiguration config = gng.getConfiguration();

    int iter = 0;
    int previous_iter = -1;
    double best_previous_error = -1;
    int error_index = -1;
    double current_best = 1e10;
    int initial_patience = 3;
    int patience = initial_patience;
    int max_iter = config.max_iter;

    cout << "insertExamples" << endl;

    gng.insertExamples(vect, nullptr, nullptr, nExamples, dim);

    cout << "run" << endl;
    gng.run();

    try {
        while(gng.getCurrentIteration() == 0 || gng.isRunning()){}

        while(iter == 0 || gng.isRunning()){
            sleep(100);
            iter = gng.getCurrentIteration();

            if(previous_iter != iter && iter % (max_iter / 100) == 0){
                cout << "Iteration " << iter << endl;
            }

            vector<double> error = gng.getMeanErrorStatistics();
            if(error.size() > 5 ) {

                best_previous_error = *std::min_element(std::end(error) - 5, std::end(error));

                if (error_index - gng.getGNGErrorIndex() > 4 &&
                    (current_best - best_previous_error) < current_best * min_relative_diff) {
                    patience--;
                    if (patience <= 0) {
                        cout << "Best error during training: " << current_best;
                        cout << "Best error in 5 previous iterations " << best_previous_error;
                        vector<double> sample(std::end(error) - 5, std::end(error));
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
    unsigned int dim = 1000;
    unsigned int nExamples = 1000;
    //min iters is 100
    int max_iter = 100;

    double* vect = set_up(dim, nExamples);

    GNGConfiguration config = GNGConfiguration::getDefaultConfiguration();
    config.datasetType = GNGConfiguration::DatasetSeq;

    switch(alg){
        case ONLINE_HNSW:
            config.uniformgrid_optimization = false;
            config.ann_optimization = true;
            config.ann_approach = GNGConfiguration::ONLINE_HNSW;
            break;
        case ONLINE_HSNW_MV:
            config.uniformgrid_optimization = false;
            config.ann_optimization = true;
            config.ann_approach = GNGConfiguration::ONLINE_HNSW_MV;
            config.nsw = false;
            config.efSearch = 32;
            config.efConstruction = 32;
            config.max_links = 36;
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
    config.verbosity = 0;

    GNGServer gng(&config);

    for (auto _ : state) {
        cout << "starting "<< endl;
        server(gng, vect, dim, nExamples);
        cout << "endng "<< endl;


    }

    state.counters.insert({{"dim", dim}, {"nExamples", nExamples}, {"max_iter", max_iter}, {"quantError", gng.getMeanError()}});
}


BENCHMARK_CAPTURE(TestBenchmark, online_hnsw, algorithms::ONLINE_HSNW_MV)->Unit(benchmark::kMillisecond)->Iterations(1);

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
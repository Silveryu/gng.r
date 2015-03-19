#include "gtest/gtest.h"
#include "cluster_reader.hpp"
#include "clustering_comparator.hpp"
#include "hartigan.hpp"
#include "random_assignment.hpp"
#include "cec.hpp"
#include "cec_configuration.hpp"
#include "algorithm.hpp"
#include <vector>
#include <armadillo>

using namespace gmum;

TEST(CEC_UCI_TestCase, dummy)
{
    ClusterReader cluster_reader("UCI");
    Params params;
    params.nclusters = 7;
    params.kill_threshold = 0.05;
    params.dataset = cluster_reader.get_points_in_matrix();
    params.nstart = 5;
    params.assignment_type = kkmeanspp;
    params.cluster_type = kstandard;

    CecConfiguration conf;
    conf.set_params(params);
    conf.set_algorithm("hartigan");
    CecModel cec(&conf);
    std::cout << cec.get_energy();
}


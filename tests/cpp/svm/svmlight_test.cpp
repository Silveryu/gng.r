#include <armadillo>
#include <iostream>
#include <string>
#include "gtest/gtest.h"

#include "svm/log.h"
#include "svmlight_runner.h"
#include "svm_basic.h"

namespace {

int log_level = LogLevel::TRACE;

double *null_double_ptr = 0;
long *null_long_ptr = 0;
long null_long = 0;

// The fixture for testing class SVMLightRunner
class SVMLightRunnerTest: public ::testing::Test {

protected:

    SVMLightRunnerTest() {
        svmlr = SVMLightRunner();
        second_svmlr = SVMLightRunner();
        svm_config = SVMConfiguration();
        svm_config.log.verbosity = log_level;
        second_svm_config = SVMConfiguration();
        second_svm_config.log.verbosity = log_level;

        learing_data_01
            << 0.5 << 1.0 << 0.0 << 1.0 << arma::endr
            << 0.4 << 1.1 << 0.0 << 1.0 << arma::endr
            << 0.5 << 0.9 << 1.0 << 0.0 << arma::endr
            << 0.5 << 1.0 << 1.0 << 0.0 << arma::endr
            << 0.4 << 1.1 << 1.0 << 0.0 << arma::endr;
        learing_target_01 << -1 << -1 << 1 << 1 << 1;

        learing_target_02 << 2 << 2 << 4 << 4 << 4;

        testing_data_01
            << 0.4 << 0.9 << 0.0 << 1.0 << arma::endr
            << 0.5 << 0.9 << 0.0 << 1.0 << arma::endr
            << 0.4 << 1.0 << 1.0 << 0.0 << arma::endr
            << 0.5 << 1.0 << 1.0 << 0.0 << arma::endr;
        testing_target_01 << -1 << -1 << 1 << 1;

        testing_target_02 << 2 << 2 << 4 << 4;
    }

    virtual ~SVMLightRunnerTest() {}

    /// Called immediately after the constructor (righ before each test)
    virtual void SetUp() {}

    /// Called immediately after each test (right before the destructor)
    virtual void TearDown() {}

    /* Objects declared here can be used by all tests in the test case */

    SVMLightRunner svmlr;
    SVMLightRunner second_svmlr;
    SVMConfiguration svm_config;
    SVMConfiguration second_svm_config;

    arma::mat learing_data_01;
    arma::vec learing_target_01;
    arma::vec learing_target_02;
    arma::mat testing_data_01;
    arma::vec testing_target_01;
    arma::vec testing_target_02;
};


} // end namespace

/* Fixture tests */

TEST_F(SVMLightRunnerTest, processRequest_learning) {
    std::cout << "SVMConfiguration data..." << std::endl;
    svm_config.data = learing_data_01;
    std::cout << "SVMConfiguration target..." << std::endl;
    svm_config.target = learing_target_01;
    std::cout << "SVMConfiguration setPrediction..." << std::endl;
    svm_config.setPrediction(false);
    std::cout << "Processing request." << std::endl;
    svmlr.processRequest(svm_config);

    // kernel_type - LINEAR
    ASSERT_EQ(svm_config.kernel_type, 0);
    // -d int      -> parameter d in polynomial kernel
    ASSERT_EQ(svm_config.degree, 3);
    // -g float    -> parameter gamma in rbf kernel
    ASSERT_EQ(svm_config.gamma, 1);
    // -s float    -> parameter s in sigmoid/poly kernel
    ASSERT_EQ(svm_config.coef0, 1);
    // -r float    -> parameter c in sigmoid/poly kernel
    ASSERT_EQ(svm_config.C, 1);
    // -u string   -> parameter of user defined kernel
    ASSERT_EQ(
        std::string(svm_config.kernel_parm_custom), std::string("empty"));
    // highest feature index - no assignment to read-only data
    ASSERT_EQ(svm_config.data.n_cols, 4);
    // number of support vectors
    ASSERT_EQ(svm_config.l, 5);
    // threshold b
    ASSERT_DOUBLE_EQ(svm_config.threshold_b, -0.11450359507913976);
}

TEST_F(SVMLightRunnerTest, processRequest_classification) {
    std::cout << "Testing learing..." << std::endl << std::flush;
    svm_config.data = learing_data_01;
    svm_config.target = learing_target_01;
    svm_config.setPrediction(false);
    svmlr.processRequest(svm_config);

    std::cout << "Testing prediction..." << std::endl << std::flush;
    svm_config.data = testing_data_01;
    svm_config.setPrediction(true);
    svmlr.processRequest(svm_config);

    for (int i = 0; i < 4; ++i) {
        ASSERT_DOUBLE_EQ(svm_config.result[i], testing_target_01[i]);
    }
}

TEST_F(SVMLightRunnerTest, test_globals_cleaning) {
    std::cout << "Testing whether globals are being initialized properly"
        << std::endl << std::flush;
    extern double *primal;
    ASSERT_EQ(primal, null_double_ptr);
    extern double *dual;
    ASSERT_EQ(dual, null_double_ptr);
    extern long precision_violations;
    ASSERT_EQ(precision_violations, null_long);
    extern double *buffer;
    ASSERT_EQ(buffer, null_double_ptr);
    extern long *nonoptimal;
    ASSERT_EQ(nonoptimal, null_long_ptr);
    extern long smallroundcount;
    ASSERT_EQ(smallroundcount, null_long);
    extern long roundnumber;
    ASSERT_EQ(roundnumber, null_long);
    extern long kernel_cache_statistic;
    ASSERT_EQ(kernel_cache_statistic, null_long);
}

TEST_F(SVMLightRunnerTest, processRequest_classification_tagged_classes) {
    std::cout << "Testing learing..." << std::endl << std::flush;
    svm_config.data = learing_data_01;
    svm_config.target = learing_target_02;
    svm_config.setPrediction(false);
    svmlr.processRequest(svm_config);

    std::cout << "Testing prediction..." << std::endl << std::flush;
    svm_config.data = testing_data_01;
    svm_config.setPrediction(true);
    svmlr.processRequest(svm_config);

    for (int i = 0; i < 4; ++i) {
        ASSERT_DOUBLE_EQ(svm_config.result[i], testing_target_02[i]);
    }
}

TEST_F(SVMLightRunnerTest, processRequest_with_poly_kernel) {
    std::cout << "Testing learing..." << std::endl << std::flush;
    svm_config.setKernel(std::string("poly"));
    svm_config.data = learing_data_01;
    svm_config.target = learing_target_02;
    svm_config.setPrediction(false);
    svmlr.processRequest(svm_config);

    std::cout << "Testing prediction..." << std::endl << std::flush;
    svm_config.data = testing_data_01;
    svm_config.setPrediction(true);
    svmlr.processRequest(svm_config);

    for (int i = 0; i < 4; ++i) {
        ASSERT_DOUBLE_EQ(svm_config.result[i], testing_target_02[i]);
    }
}

TEST_F(SVMLightRunnerTest, processRequest_with_rbf_kernel) {
    std::cout << "Testing learing..." << std::endl << std::flush;
    svm_config.setKernel(std::string("rbf"));
    svm_config.data = learing_data_01;
    svm_config.target = learing_target_02;
    svm_config.setPrediction(false);
    svmlr.processRequest(svm_config);

    std::cout << "Testing prediction..." << std::endl << std::flush;
    svm_config.data = testing_data_01;
    svm_config.setPrediction(true);
    svmlr.processRequest(svm_config);

    for (int i = 0; i < 4; ++i) {
        ASSERT_DOUBLE_EQ(svm_config.result[i], testing_target_02[i]);
    }
}


#include <armadillo>
#include <iostream>
#include <string>
#include "gtest/gtest.h"

#include "svm_helpers.h"
#include "svm/log.h"
#include "svm_basic.h"

namespace {

int log_level = LogLevel::DEBUG;

double *null_double_ptr = 0;
long *null_long_ptr = 0;
long null_long = 0;

// The fixture for testing class SVMLightRunner
class SVMConfigurationTest: public ::testing::Test {

protected:

    SVMConfigurationTest() {
        std::cout << "Creating SVMConfiguration..." << std::endl << std::flush;
        svm_config = SVMConfiguration();
        std::cout << "Setting logger..." << std::endl << std::flush;
        svm_config.log.verbosity = log_level;

        sparse_matrix_csc_01_row_indices =
            helper_sparse_matrix_csc_01_row_indices();
        sparse_matrix_csc_01_column_pointers =
            helper_sparse_matrix_csc_01_column_pointers();
        sparse_matrix_csc_01_values =
            helper_sparse_matrix_csc_01_values();
        sparse_matrix_csc_01_nrow =
            helper_sparse_matrix_csc_01_nrow();
        sparse_matrix_csc_01_ncol =
            helper_sparse_matrix_csc_01_ncol();

        std::cout << "Starting test..." << std::endl << std::flush;
    }

    virtual ~SVMConfigurationTest() {}

    /// Called immediately after the constructor (righ before each test)
    virtual void SetUp() {}

    /// Called immediately after each test (right before the destructor)
    virtual void TearDown() {}

    /* Objects declared here can be used by all tests in the test case */

    SVMConfiguration svm_config;

	arma::uvec sparse_matrix_csc_01_row_indices;
    arma::uvec sparse_matrix_csc_01_column_pointers;
    arma::vec sparse_matrix_csc_01_values;
    size_t sparse_matrix_csc_01_nrow;
    size_t sparse_matrix_csc_01_ncol;
};


} // end namespace

/* Fixture tests */

TEST_F(SVMConfigurationTest, setSparseData) {
    std::cout << "SVMConfiguration sparse data..." << std::endl;
    svm_config.setSparseData(
        sparse_matrix_csc_01_row_indices,
        sparse_matrix_csc_01_column_pointers,
        sparse_matrix_csc_01_values,
        sparse_matrix_csc_01_nrow,
        sparse_matrix_csc_01_ncol,
        true
    );
    std::cout << svm_config.sparse_data;
}

/*
TEST_F(SVMConfigurationTest, processRequest_learning) {
    std::cout << "SVMConfiguration data..." << std::endl;
    svm_config.data = learning_data_01;
    std::cout << "SVMConfiguration target..." << std::endl;
    svm_config.target = learning_target_01;
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
    ASSERT_DOUBLE_EQ(svm_config.threshold_b, 0.11450359507913976);
}

TEST_F(SVMConfigurationTest, processRequest_classification) {
    std::cout << "Testing learning..." << std::endl << std::flush;
    svm_config.data = learning_data_01;
    svm_config.target = learning_target_01;
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

TEST_F(SVMConfigurationTest, test_globals_cleaning) {
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

TEST_F(SVMConfigurationTest, processRequest_classification_tagged_classes) {
    std::cout << "Testing learning..." << std::endl << std::flush;
    svm_config.data = learning_data_01;
    svm_config.target = learning_target_02;
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

TEST_F(SVMConfigurationTest, processRequest_with_poly_kernel) {
    std::cout << "Testing learning..." << std::endl << std::flush;
    svm_config.setKernel(std::string("poly"));
    svm_config.data = learning_data_01;
    svm_config.target = learning_target_02;
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

TEST_F(SVMConfigurationTest, processRequest_with_rbf_kernel) {
    std::cout << "Testing learning..." << std::endl << std::flush;
    svm_config.setKernel(std::string("rbf"));
    svm_config.data = learning_data_01;
    svm_config.target = learning_target_02;
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

TEST_F(SVMConfigurationTest, processRequest_with_sigmoid_kernel) {
    std::cout << "Testing learning..." << std::endl << std::flush;
    svm_config.setKernel(std::string("sigmoid"));
    svm_config.data = learning_data_01;
    svm_config.target = learning_target_02;
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

TEST_F(SVMConfigurationTest, integration_svmclient_predict) {
    std::cout << "Testing learning..." << std::endl << std::flush;
    svm_config.data = learning_data_01;
    svm_config.target = learning_target_01;
    svm_config.setPrediction(false);
    svmlr.processRequest(svm_config);

    std::cout << "Testing SVMClient prediction..." << std::endl << std::flush;
    svm_config.data = testing_data_01;
    svm_config.setPrediction(true);
    SVMClient *svm_client = new SVMClient(&svm_config);
    svm_client->predict(testing_data_01);
    SVMConfiguration client_config = svm_client->getConfiguration();
    
    for (int i = 0; i < 4; ++i) {
        ASSERT_DOUBLE_EQ(
            client_config.result[i], testing_target_01[i]);
    }
}
*/


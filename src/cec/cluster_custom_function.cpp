#include "cluster_custom_function.hpp"

namespace gmum {
ClusterCustomFunction::ClusterCustomFunction(int count,
		const arma::rowvec& mean, const arma::mat& cov_mat,
		const std::string& function_name) :
		ClusterUseCovMat(count, mean, cov_mat), m_function_name(function_name) {
	m_entropy = calculate_entropy(m_n, cov_mat);
}

ClusterCustomFunction::ClusterCustomFunction(unsigned int id,
		const std::vector<unsigned int> &assignment, const arma::mat &points,
		const std::string &function_name) :
		ClusterUseCovMat(id, assignment, points), m_function_name(function_name) {
	m_entropy = calculate_entropy(m_n, m_cov_mat);
}

double ClusterCustomFunction::calculate_entropy(int n,
		const arma::mat &cov_mat) {
#ifdef RCPP_INTERFACE
	Rcpp::Environment my_env = Rcpp::Environment::global_env();
	Rcpp::Function my_function = my_env[m_function_name];
	return Rcpp::as<double>(
			my_function(Rcpp::Named("m", Rcpp::wrap(n)),
					Rcpp::Named("sigma", Rcpp::wrap(cov_mat))));
#else
	std::cerr << "Rcpp support not compiled\n";
	exit(1);
#endif
}

ClusterCustomFunction* ClusterCustomFunction::clone()
{
    return new ClusterCustomFunction(m_count, m_mean, m_cov_mat, m_function_name);
}

}

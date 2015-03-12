#include "cec.hpp"
#include "hartigan.hpp"
#include "assignment.hpp"
#include "kmeanspp_assignment.hpp"
#include "centroids_assignment.hpp"
#include "random_assignment.hpp"

using namespace gmum;

void CecModel::clear_clusters() {
	if (m_clusters.empty()) {
		return;
	}
	for (std::vector<Cluster*>::iterator it = m_clusters.begin();
			it != m_clusters.end(); ++it) {
		delete *it;
	}
	m_clusters.clear();
}

Cluster* CecModel::create_cluster(const ClusterParams &params, int i) {
	Cluster * cluster = 0;
	switch (params.type) {
	case kno_type: // TODO: handle knoType parameter
	case kmix: // TODO: handle kmix parameter
		break;
	case kstandard:
		cluster = new ClusterStandard(i, m_assignment, m_points);
		break;
	case kfull: {
		const ClusterFullParams &ptr =
				static_cast<const ClusterFullParams&>(params);
		cluster = new ClusterCovMat(ptr.cov_mat, i, m_assignment, m_points);
		break;
	}
	case kdiagonal:
		cluster = new ClusterDiagonal(i, m_assignment, m_points);
		break;
	case ksphere:
		cluster = new ClusterSpherical(i, m_assignment, m_points);
		break;
	case kfsphere: {
		const ClusterFsphereParams &ptr =
				static_cast<const ClusterFsphereParams&>(params);
		cluster = new ClusterConstRadius(ptr.radius, i, m_assignment, m_points);
		break;
	}
#ifdef RCPP_INTERFACE
		case kcustom: {
			const ClusterCustomParams &ptr =
			static_cast<const ClusterCustomParams&>(params);
			cluster = new ClusterCustomFunction(i, m_assignment, m_points, ptr.function);
			break;
		}
#endif
	}
	return cluster;
}

CecModel::~CecModel() {
	clear_clusters();
}

CecModel::CecModel(CecConfiguration *cfg) :
		m_config(cfg) {
	find_best_cec();
}
CecModel::CecModel(CecModel &other) :
		m_config(0) {
	*this = other;
}

CecModel& CecModel::operator=(CecModel& other) {
	if (this != &other) {
		m_result = other.m_result;
		m_assignment = other.m_assignment;
		m_points = other.m_points;
		m_algorithm = other.m_algorithm;
		m_kill_threshold = other.m_kill_threshold;
		m_inv_set = other.m_inv_set;
		m_inv = other.m_inv;
		m_config = other.m_config;
		clear_clusters();
		for (std::vector<Cluster*>::iterator it = other.m_clusters.begin();
				it != other.m_clusters.end(); ++it) {
			m_clusters.push_back((*it)->clone());
		}
	}
	return *this;
}

void CecModel::init(boost::shared_ptr<Algorithm> algorithm,
		std::vector<unsigned int>& assignment) {
	Params params = m_config->get_params();
	m_assignment = assignment;
	m_points = *(params.dataset);
	m_algorithm = algorithm;
	m_kill_threshold = params.kill_threshold;
	clear_clusters();
    m_clusters.reserve(params.nclusters);

	int i = 0;
	if (params.cluster_type == kmix) {
		BOOST_FOREACH(boost::shared_ptr < ClusterParams > cluster,
				params.clusters)
		{
			m_clusters.push_back(create_cluster(*cluster, i));
			++i;
		}
	} else {
		//TODO: why pointer?
		ClusterParams *cluster = 0;
		switch (params.cluster_type) {
		case kfsphere: {
			ClusterFsphereParams *proxy = new ClusterFsphereParams();
			proxy->radius = params.radius;
			cluster = proxy;
			break;
		}
		case kfull: {
			ClusterFullParams *proxy = new ClusterFullParams();
			proxy->cov_mat = params.cov_mat;
			cluster = proxy;

			break;
		}
#ifdef RCPP_INTERFACE
			case kcustom: {
				ClusterCustomParams *proxy = new ClusterCustomParams();
				proxy->function = params.function;
				cluster = proxy;
				break;
			}
#endif
		default:
			/*case standard:
			 case diagonal:
			 case sphere:*/
			cluster = new ClusterParams(params.cluster_type);
			break;
		}
		for (unsigned int i = 0; i < params.nclusters; ++i)
			m_clusters.push_back(create_cluster(*cluster, i));
		//TODO: redelete
		delete cluster;
	}

}

void CecModel::find_best_cec() {
	std::vector<unsigned int> assignment;
	Params params = m_config->get_params();
    boost::shared_ptr<Hartigan> hartigan = boost::make_shared<Hartigan>(params.log_nclusters, params.log_energy, params.it_max);
    boost::shared_ptr<Assignment> assignment_type;

	switch (params.assignment_type) {
	case krandom:
        assignment_type = boost::make_shared<RandomAssignment>(*(params.dataset), params.nclusters);
		break;
	case kkmeanspp:
        assignment_type = boost::make_shared<KmeansppAssignment>(*(params.dataset), params.nclusters);
		break;
	case kcentroids:
        assignment_type = boost::make_shared<CentroidsAssignment>(*(params.dataset), params.nclusters, params.centroids);
		break;
	}

    assignment.resize(params.dataset->n_rows);
    (*assignment_type)(assignment);

	init(hartigan, assignment);

	try {
		loop();
		CecModel best_cec = *this;
		for (unsigned int i = 1; i < params.nstart; ++i) {
			(*assignment_type)(assignment);
			init(hartigan, assignment);
			loop();

			if (m_result.energy < best_cec.get_result().energy) {
				best_cec = *this;
			}
		}
		*this = best_cec;
	} catch (std::exception &e) {
#ifdef RCPP_INTERFACE
		Rcpp::stop(std::string("exception ") + e.what() + " caught in CEC_new");
#else
		std::cerr << std::string("exception ") + e.what() + " caught in CEC_new"
				<< std::endl;
		exit(1);
#endif
	}
}

void CecModel::loop() {
	m_result = m_algorithm->loop(m_points, m_assignment, m_kill_threshold,
			m_clusters);
	int nclusters = m_clusters.size();
	m_inv_set.resize(nclusters, false);
	m_inv.resize(nclusters);
}

void CecModel::single_loop() {
	SingleResult sr = m_algorithm->single_loop(m_points, m_assignment,
			m_kill_threshold, m_clusters);
	m_result.append(sr, m_config->get_params().log_nclusters,
			m_config->get_params().log_energy);
}

double CecModel::entropy() {
	double s = 0.0;

	BOOST_FOREACH(Cluster * cluster, m_clusters)
	{
		s += cluster->entropy();
	}
	return s;
}

std::vector<unsigned int> CecModel::get_assignment() const {
	return m_assignment;
}

void CecModel::set_assignment(std::vector<unsigned int>& assignment) {
	//TODO: handle set assignment
	m_assignment = assignment;
}

arma::mat CecModel::get_points() {
	return m_points;
}

std::vector<arma::rowvec> CecModel::centers() const {
	std::vector < arma::rowvec > array;
	array.reserve(m_clusters.size());
	for (unsigned int i = 0; i < m_clusters.size(); ++i) {
		array.push_back(m_clusters[i]->get_mean());
	}
	return array;
}

std::vector<arma::mat> CecModel::cov() const {
	std::vector < arma::mat > array;
	array.reserve(m_clusters.size());

	for (unsigned int i = 0; i < m_clusters.size(); ++i) {
		array.push_back(m_clusters[i]->get_cov_mat(i, m_assignment, m_points));
	}

	return array;
}

unsigned int CecModel::iters() const {
	return m_result.iterations;
}

std::list<unsigned int> CecModel::get_nclusters() const {
	return m_result.nclusters;
}

std::list<double> CecModel::get_energy_history() const {
	return m_result.energy_history;
}

double CecModel::get_energy() const {
	return m_result.energy;
}

unsigned int CecModel::predict(std::vector<double> vec) {
	arma::rowvec x = arma::conv_to < arma::rowvec > ::from(vec);

	unsigned int assign = 0;
	double result = std::numeric_limits<double>::min();
	for (unsigned int i = 0; i < m_clusters.size(); ++i) {
		arma::mat cov_mat = m_clusters[i]->get_cov_mat(i, m_assignment,
				m_points);
		arma::rowvec mean = m_clusters[i]->get_mean();
		if (!m_inv_set[i]) {
			arma::mat Q, R;
			arma::qr_econ(Q, R, cov_mat);
			int dim = mean.n_cols;
			arma::mat Id = arma::eye(dim, dim);
			m_inv[i] = solve(R, Id) * Q.t();
			m_inv_set[i] = true;
		}

		double const_multiplier = pow(2 * M_PI, -0.5 * x.n_cols)
				* pow(arma::det(cov_mat), -0.5);
		double scalar = arma::as_scalar(
				(x - mean) * m_inv[i] * ((x - mean).t()));
		double exponens = exp(-0.5 * scalar);
		double p = m_clusters[i]->size() / (double) m_points.n_rows;
		double tmp = p + const_multiplier * exponens;

		if (tmp > result) {
			result = p + const_multiplier * exponens;
			assign = i;
		}
	}

	return assign;
}

const gmum::TotalResult& CecModel::get_result() const {
	return m_result;
}

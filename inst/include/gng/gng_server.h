/*
 * File: GNGServer.h
 * Author: staszek
 *
 * Created on October 17, 2013, 8:12 PM
 */
#ifndef GNGSERVER_H
#define GNGSERVER_H

#include <globals.h>
#include <gng_algorithm.h>
#include <gng_configuration.h>
#include <gng_dataset.h>
#include <gng_defines.h>
#include <gng_graph.h>
#include <cstdlib>
#include <cstddef>
#include <map>
#include <exception>
#include <memory>

#include "utils/threading.h"
#include "utils/utils.h"


#ifdef RCPP_INTERFACE
#include <RcppArmadillo.h>
using namespace Rcpp;
using namespace arma;
#endif

static int gng_server_count=0;

using namespace gmum;

/** Holds together all logic and objects.*/
class GNGServer {
public:
	boost::shared_ptr<Logger> m_logger;

	/**Construct GNGServer using configuration*/
	GNGServer(GNGConfiguration configuration, std::istream * input_graph);

	GNGServer(std::string filename);

	void run();

	void pause();

	bool hasStarted() const;

	void terminate();

	void setVerbosity(int verbosity) {
		this->m_logger->verbosity = verbosity;
	}

	float nodeDistance(int id1, int id2) const;

	void save(std::string filename);

	unsigned int getCurrentIteration() const;

	///Exports GNG state to file
	void exportToGraphML(std::string filename);

	///Insert examples
	void insertExamples(float * positions, float * extra,
			float * probability, unsigned int count, unsigned int dim);


    unsigned getDatasetSize() const;
	unsigned getGNGErrorIndex() const;
	bool isRunning() const;
	vector<float> getMeanErrorStatistics();
	unsigned int getNumberNodes() const;
	float getMeanError();
	GNGConfiguration getConfiguration();
	GNGAlgorithm & getAlgorithm();
	GNGGraph & getGraph();
	GNGDataset & getDatabase();

	~GNGServer();

	//Constructor needed for RCPPInterface
    GNGServer(GNGConfiguration * configuration);

#ifdef RCPP_INTERFACE

	///Moderately slow function returning node descriptors
	Rcpp::List getNode(int index);

	int Rpredict(Rcpp::NumericVector & r_ex);

	Rcpp::NumericVector RgetClustering();

	Rcpp::NumericVector RgetErrorStatistics();

	void RinsertExamples(Rcpp::NumericMatrix & r_points);

	void RinsertLabeledExamples(Rcpp::NumericMatrix & r_points,
			Rcpp::NumericVector r_extra);

	//This is tricky - used only by convertToIGraph in R, because
	//it might happen that we delete nodes and have bigger index of the last node
	//than actual nodes (especially in the case of utility version of GNG)
	unsigned int _getLastNodeIndex() const;

	///Calls updateClustering on the GNGAlgorithm object
	void _updateClustering();
#endif

private:
	int m_index;

	bool m_running_thread_created;

	gmum::gmum_thread * algorithm_thread;

	/** Mutex used for synchronization of graph access*/
	gmum::recursive_mutex grow_mutex;
	/** Mutex used for synchronization of graph access*/
	gmum::recursive_mutex database_mutex;
	/** Mutex used for synchronization of graph access*/
	gmum::recursive_mutex stat_mutex;

	GNGConfiguration current_configuration;

	std::unique_ptr<gmum::GNGAlgorithm> gngAlgorithm;
	std::unique_ptr<gmum::GNGGraph> gngGraph;
	std::unique_ptr<gmum::GNGDataset> gngDataset;

	//Called from constructors
	void init(GNGConfiguration configuration, std::istream * input_graph = 0);

private:
	/** Run GNG Server - runs in separate thread and returns control
	 * @note Runs one extra threads for communication.
	 */
	static void _run(void * server) {
        // This shouldn't be necessary but R behaves strangely in this matter.
		GNGServer * gng_server = (GNGServer*) server;
		try {
			DBG_PTR(gng_server->m_logger,10, "GNGServer::run::proceeding to algorithm");
			gng_server->getAlgorithm().runAlgorithm();
		} catch (std::exception & e) {
			DBG_PTR(gng_server->m_logger,10, e.what());
		}
	}

public:
	//legacy code
	static GNGServer * constructTestServer(GNGConfiguration config) {
		return new GNGServer(config, 0 /*input_graph*/);
	}
};

#endif
/* GNGSERVER_H */

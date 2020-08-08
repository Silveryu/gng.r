/*
 * File: GNGAlgorithm.cpp
 * Author: staszek "kudkudak" jastrzebski <grimghil<at>gmail.com>
 *
 * Created on 11 sierpień 2012, 10:02
 */

//TODO: refactor getExample
#include <cstdlib>
#include <utility>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <gng_algorithm.h>

using namespace boost;
using namespace gmum;
using namespace std;

namespace gmum {

GNGNode ** GNGAlgorithm::LargestErrorNodesLazy() {
	GNGNode ** largest = new GNGNode*[2];
	GNGNode * gng_node;

	FOREACH(int it, errorHeap.getLazyList())
	{
		gng_node = &m_g[it];
		errorHeap.insert(gng_node->nr, gng_node->error);
	}

	errorHeap.getLazyList().clear();

	ErrorNode max;
	//Extract max until you get correct one (that is not lazy)
	do {
		max = errorHeap.extractMax();

		DBG_PTR(m_logger, 4, "GNGAlgorithm::LargestErrorLazy::found max " + to_string(max.i));

		GNGNode * gng_node = &m_g[max.i];
		if (gng_node->error_cycle != c) {
			fixErrorNew(gng_node);
			errorHeap.update(gng_node->nr, gng_node->error);
		} else {
			largest[0] = gng_node;
			int j = 0;
			double error = 0.0;
			DBG_PTR(m_logger, 4, "GNGAlgorithm::LargestErrorLazy::found max " + to_string(max.i));

			BOOST_FOREACH(GNGEdge * edg, *largest[0])
			{
				++j;
				fixErrorNew(&m_g[(edg)->nr]);

				if (j == 1) {
					largest[1] = &m_g[(edg)->nr];
					error = largest[1]->error;
					;
					continue;
				}

				double new_error = m_g[(edg)->nr].error;

				if (error < new_error) {
					error = new_error;
					largest[1] = &m_g[(edg)->nr];
				}
			}
			break;
		}

	} while (1); DBG_PTR(m_logger, 3, "GNGAlgorithm::LargestErrorLazy::returning");
	return largest;

}

GNGGraph* GNGGraphAccessHack::pool = 0;

GNGAlgorithm::GNGAlgorithm(GNGGraph * g, GNGDataset* db,
		double * boundingbox_origin, double * boundingbox_axis, double l,
		int max_nodes, int max_age, double alpha, double betha, double lambda,
		double eps_w, double eps_n, int dim, bool uniformgrid_optimization,
        bool ann_optimization, int ann_approach,
        int max_links, int efConstruction, int efSearch, bool nsw, bool recall,
		bool lazyheap_optimization, unsigned int utility_option,
		double utility_k, int max_iter, int seed, boost::shared_ptr<Logger> logger) :
		m_g(*g), g_db(db), c(0), s(0), m_max_nodes(max_nodes), m_max_age(
				max_age), m_alpha(alpha), m_betha(betha), m_lambda(lambda), m_eps_w(
				eps_w), m_eps_n(eps_n), m_density_threshold(0.1), m_grow_rate(
				1.5), errorHeap(), dim(dim), m_toggle_uniformgrid(
				uniformgrid_optimization), m_toggle_ann(ann_optimization), m_ann_approach(ann_approach), m_max_links(max_links),
				m_efConstruction(efConstruction), m_efSearch(efSearch), m_nsw(nsw), m_recall(recall),
				m_toggle_lazyheap(lazyheap_optimization),  m_utility_option(
				utility_option), m_mean_error(1000), m_utility_k(utility_k), 
                max_iter(max_iter), m_logger(
				logger), m_iteration(0),
				m_gng_status(GNG_TERMINATED),
				m_gng_status_request(GNG_TERMINATED), mt_rand(seed) {

	DBG_PTR(m_logger, 1, "GNGAlgorithm:: Constructing object");
	DBG_PTR(m_logger, 10,
			"GNGAlgorithm::Constructed object with utility "
					+ to_string(utility_option) + " " + to_string(utility_k));
	// used to profile ANN approach

	if (m_toggle_uniformgrid) {
		ug = new UniformGrid<std::vector<Node>, Node, int>(boundingbox_origin,
				boundingbox_axis, l, dim, m_grow_rate, m_density_threshold, 0.4,
				m_logger);

		GNGGraphAccessHack::pool = &m_g;

		ug->setDistFunction(GNGGraphAccessHack::dist);

		// Restore uniform grid state
		int maximum_index = m_g.get_maximum_index();
		REP(i, maximum_index + 1)
			if (m_g.existsNode(i))
				ug->insert(m_g[i].position, m_g[i].nr);

	}
	else if(m_toggle_ann){
	    nInsertions = 0;
	    nRemovals = 0;
	    nSearches = 0;
	    nMoves = 0;
        approxSearchCount = 0;
        key1Count = 0;
        key2Count = 0;

	    ann = new hnsw::hnsw_index<long , std::vector<double>, hnsw::l2_square_distance_t>();

        // ann = (faiss::IndexHNSWFlat*)faiss::index_factory(dim, "IDMap,HNSW32", faiss::METRIC_L2);
        ann->options.nsw = m_nsw;
        ann->options.max_links = m_max_links;
        ann->options.ef_construction = m_efConstruction;

        // online hnsw
        // cout << "M: " << m_max_links << endl;
        // cout << "efSearch: "<<  m_efSearch << endl;
        // cout << "efConstruction: " << ann->options.ef_construction << endl;


        GNGGraphAccessHack::pool = &m_g;

        int maximum_index = m_g.get_maximum_index();

        for(size_t i=0; i<maximum_index+1; ++i) {
            if (m_g.existsNode(i)) {
                 ann->insert(m_g[i].nr, std::vector<double>(m_g[i].position, m_g[i].position + dim));
            }
        }

	}

	if (m_toggle_lazyheap) {
		int maximum_index = m_g.get_maximum_index();
		REP(i, maximum_index + 1)
			if (m_g.existsNode(i))
				setErrorNew(&m_g[i], m_g[i].error);
	}

	m_betha_powers_size = m_lambda * 10;
	m_betha_powers = new double[m_betha_powers_size];

	REP(i, m_betha_powers_size)
		m_betha_powers[i] = std::pow(m_betha, (double) (i));

	m_betha_powers_to_n_length = m_max_nodes * 2;
	m_betha_powers_to_n = new double[m_max_nodes * 2];

	REP(i, m_max_nodes * 2)
		m_betha_powers_to_n[i] = std::pow(m_betha, m_lambda * (double) (i));
	DBG_PTR(m_logger, 1, "GNGAlgorithm:: Constructed object");
}

void GNGAlgorithm::randomInit() {

	DBG_PTR(m_logger, 3, "randomInit::Drawing examples");

	int ex1 = g_db->drawExample();
	int ex2 = g_db->drawExample();

	DBG_PTR(m_logger, 3, "randomInit::Drawn 2");
	int index = 0;
	while (ex2 == ex1 && index < 100) {
		++index;
		ex2 = g_db->drawExample();
	}
	DBG_PTR(m_logger, 3, "randomInit::database_size = " + to_string(g_db->size()));
	DBG_PTR(m_logger, 3,
			"randomInit::drawn " + to_string(ex1) + " " + to_string(ex2));

	const double * ex1_ptr = g_db->getPosition(ex1);
	const double * ex1_extra_ptr = g_db->getExtraData(ex1);
	const double * ex2_ptr = g_db->getPosition(ex2);
	const double * ex2_extra_ptr = g_db->getExtraData(ex2);

	m_g.newNode(ex1_ptr);
	m_g.newNode(ex2_ptr);

	if (ex1_extra_ptr)
		m_g[0].extra_data = ex1_extra_ptr[0];
	if (ex2_extra_ptr)
		m_g[1].extra_data = ex2_extra_ptr[0];

	DBG_PTR(m_logger, 3,
			"randomInit::created nodes graph size="
					+ to_string(m_g.get_number_nodes()));

#ifdef GMUM_DEBUG_2
	ASSERT(m_g.get_number_nodes()==2);
#endif

	if (m_toggle_uniformgrid) {
		ug->insert(m_g[0].position, 0);
		ug->insert(m_g[1].position, 1);
	}
    else if(m_toggle_ann){

        ann->insert(0, vector<double>{m_g[0].position, m_g[0].position + dim});
        ann->insert(1, vector<double>{m_g[1].position, m_g[1].position + dim});
        nInsertions += 2;
    }

	if (m_toggle_lazyheap) {
		setErrorNew(&m_g[0], 0.0);
		setErrorNew(&m_g[1], 0.0);
	}
	if (this->m_utility_option == BasicUtility) {
		setUtility(0, 0.001);
		setUtility(1, 0.001);
	}
}

void GNGAlgorithm::addNewNode() {
	using namespace std;

    if (m_max_nodes <= m_g.get_number_nodes()) {
		DBG_PTR(m_logger, 4,
				"GNGAlgorith::AddNewNode:: achieved maximum number of nodes");
		return;
	}

	LOG_PTR(m_logger, 10, "GNGAlgorith::AddNewNode "+to_string(m_g.get_number_nodes()));

	DBG_PTR(m_logger, 4, "GNGAlgorith::AddNewNode::start search");

	if (m_toggle_lazyheap)
		DBG_PTR(m_logger, 4,
				"GNGAlgorithm::AddNewNode:: " + to_string(m_toggle_lazyheap)
						+ " : )= toggle_lazyheap");

	GNGNode ** error_nodes_new;

	if (m_toggle_lazyheap)
		error_nodes_new = LargestErrorNodesLazy();
	else
		error_nodes_new = LargestErrorNodes();

	DBG_PTR(m_logger, 4, "GNGAlgorith::AddNewNode::search completed");

	if (!error_nodes_new[0] || !error_nodes_new[1])
		return;

	DBG_PTR(m_logger, 4, "GNGAlgorith::AddNewNode::search completed and successful");

	

	double * position = new double[this->dim]; //param

	//TODO: < GNG_DIM?
	for (int i = 0; i < this->dim; ++i) //param
		position[i] = (error_nodes_new[0]->position[i]
				+ error_nodes_new[1]->position[i]) / 2;

	//In case pool has been reallocated
	int er_nr1 = error_nodes_new[0]->nr, er_nr2 = error_nodes_new[1]->nr;
	int new_node_index = m_g.newNode(&position[0]);
	error_nodes_new[0] = &m_g[er_nr1];
	error_nodes_new[1] = &m_g[er_nr2];

	//Vote for extra data
	m_g[new_node_index].extra_data = (error_nodes_new[0]->extra_data
			+ error_nodes_new[1]->extra_data) / 2.0;

	if (m_toggle_uniformgrid) {
        ug->insert(m_g[new_node_index].position, new_node_index);
    }
	else if(m_toggle_ann){
        ann->insert(new_node_index, std::vector<double>(m_g[new_node_index].position, m_g[new_node_index].position + dim));
        nInsertions += 1;
	}

	DBG_PTR(m_logger, 4, "GNGAlgorith::AddNewNode::added " + to_string(m_g[new_node_index]));

	m_g.removeUDEdge(error_nodes_new[0]->nr, error_nodes_new[1]->nr);

	DBG_PTR(m_logger, 3, "GNGAlgorith::AddNewNode::removed edge beetwen " +
			to_string(error_nodes_new[0]->nr) + " and" + to_string(error_nodes_new[1]->nr));
	DBG_PTR(m_logger, 2,
			"GNGAlgorithm::AddNewNode::largest error node after removing edge : "
					+ to_string(*error_nodes_new[0]));

	m_g.addUDEdge(error_nodes_new[0]->nr, new_node_index);

	m_g.addUDEdge(new_node_index, error_nodes_new[1]->nr);

	DBG_PTR(m_logger, 3, "GNGAlgorith::AddNewNode::add edge beetwen " +
			to_string(error_nodes_new[0]->nr) + " and" + to_string(new_node_index));

	if (!m_toggle_lazyheap) {
		decreaseError(error_nodes_new[0]);
		decreaseError(error_nodes_new[1]);
		setError(&m_g[new_node_index],
				(error_nodes_new[0]->error + error_nodes_new[1]->error) / 2);
	} else {
		decreaseErrorNew(error_nodes_new[0]);
		decreaseErrorNew(error_nodes_new[1]);
		setErrorNew(&m_g[new_node_index],
				(error_nodes_new[0]->error + error_nodes_new[1]->error) / 2);
	}

	if (this->m_utility_option == BasicUtility)
		this->setUtility(new_node_index,
				0.5
						* (getUtility(error_nodes_new[0]->nr)
								+ getUtility(error_nodes_new[1]->nr)));

	delete[] error_nodes_new;
	DBG_PTR(m_logger, 3, "GNGAlgorith::AddNewNode::delete done");
}


int GNGAlgorithm::predict(const std::vector<double> & ex) {

	if (m_g.get_number_nodes() == 0)
		return -1; //No node

	if (ex.size() != g_db->getGNGDim())
		throw BasicException("Wrong example dimensionality");

	return _getNearestNeurons(&ex[0], true).first;
}

std::pair<double, int> GNGAlgorithm::adapt(const double * ex,
		const double * extra) {
	DBG_PTR(m_logger, 4, "GNGAlgorith::Adapt::commence search");

	std::pair<int, int> nearest = _getNearestNeurons(ex, false);
	GNGNode * nearest_0 = &m_g[nearest.first], * nearest_1 = &m_g[nearest.second];


	DBG_PTR(m_logger, 4, "GNGAlgorith::Adapt::found nearest nodes to the drawn example " + to_string(*nearest_0) + " " + to_string(*nearest_1));

	double error = m_g.get_dist(nearest_0->position, ex);

	if (this->m_utility_option == BasicUtility) {

		DBG_PTR(m_logger, 4, "GNGAlgorithm::Adapt::setting utility");

		double error_2 = m_g.get_dist(nearest_1->position, ex);

		this->setUtility(nearest_0->nr,
				this->getUtility(nearest_0->nr) + error_2 - error);
	}

	DBG_PTR(m_logger, 3, "GNGAlgorith::Adapt::increasing error");

	if (!m_toggle_lazyheap)
		increaseError(nearest_0, error);
	else
		increaseErrorNew(nearest_0, error);

	DBG_PTR(m_logger, 3, "GNGAlgorith::Adapt::accounted for the error");

	if (m_toggle_uniformgrid) {
        ug->remove(nearest_0->position);
    }
	else if(m_toggle_ann){

	    if(m_ann_approach == GNGConfiguration::ONLINE_HNSW){
            ann->remove(nearest_0->nr);
            nRemovals += 1;
        }
	}
	for (int i = 0; i < this->dim; ++i)
		nearest_0->position[i] += m_eps_w * (ex[i] - nearest_0->position[i]);

	//Adapt to extra dimensionality if present (TODO: refactor)
	if (extra)
		nearest_0->extra_data = (nearest_0->extra_data + extra[0]) / 2.0;

	if (m_toggle_uniformgrid) {
        ug->insert(nearest_0->position, nearest_0->nr);
    }
	else if(m_toggle_ann){

	    switch(m_ann_approach){
	        case GNGConfiguration::ONLINE_HNSW:
                ann->insert(nearest_0->nr, vector<double>{nearest_0->position,nearest_0->position + dim});
                nInsertions += 1;
                break;
	        case GNGConfiguration::ONLINE_HNSW_MV:
                ann->move(nearest_0->nr, vector<double>{nearest_0->position,nearest_0->position + dim});
                nMoves += 1;
                break;
        }

	}

	if (nearest_0->edgesCount) {
		FOREACH(GNGEdge * edg, *nearest_0)
		{
			if (m_toggle_uniformgrid) {
                ug->remove(m_g[(edg)->nr].position);
            }
			else if(m_toggle_ann){

			    if(m_ann_approach == GNGConfiguration::ONLINE_HNSW) {
                    ann->remove((edg)->nr);
                    nRemovals +=1;
                }
			}

			for (int i = 0; i < this->dim; ++i) { //param accounting
				m_g[(edg)->nr].position[i] += m_eps_n
						* (ex[i] - m_g[(edg)->nr].position[i]);
			}

			//Adapt to extra dimensionality if present (TODO: refactor)
			if (extra) {
				m_g[(edg)->nr].extra_data = (0.9 * m_g[(edg)->nr].extra_data
						+ extra[0] * 0.1);
			}

			if (m_toggle_uniformgrid) {
                ug->insert(m_g[(edg)->nr].position, (edg)->nr);
            }
			else if(m_toggle_ann) {
			    switch(m_ann_approach){
			        case GNGConfiguration::ONLINE_HNSW:
                        ann->insert((edg)->nr, vector<double>(m_g[(edg)->nr].position, m_g[(edg)->nr].position + dim));
                        nInsertions += 1;
                        break;
			        case GNGConfiguration::ONLINE_HNSW_MV:
                        ann->move((edg)->nr, vector<double>{m_g[(edg)->nr].position,m_g[(edg)->nr].position + dim});
                        nMoves += 1;
                        break;
                }
            }
		}
	}

	DBG_PTR(m_logger, 4,
			"GNGAlgorith::Adapt::position of the winner and neighbour mutated");

	if (!m_g.isEdge(nearest_0->nr, nearest_1->nr)) {
		m_g.addUDEdge(nearest_0->nr, nearest_1->nr);
		DBG_PTR(m_logger, 4,
				"GNGAlgorith::Adapt::added edge beetwen "
						+ to_string(nearest_0->nr) + " and "
						+ to_string(nearest_1->nr));
	}

	bool BYPASS = false;

	DBG_PTR(m_logger, 4, "GNGAlgorith::Adapt::commence scan of edges");

	//TODO: assuming here GNGNode not any arbitrary node :/
	GNGNode::EdgeIterator edg = nearest_0->begin();
	while (edg != nearest_0->end()) {
		(*edg)->age++;
		(((*edg)->rev))->age++;

		if ((*edg)->nr == nearest_1->nr) {
			(*edg)->age = 0;
			(((*edg)->rev))->age = 0;
		}

		if ((*edg)->age > m_max_age) {
			int nr = (*edg)->nr;

			//Note that this is O(E), but average number of edges is very small, so it is OK
			edg = m_g.removeUDEdge(nearest_0->nr, nr);

			if (m_g[nr].edgesCount == 0 && this->m_utility_option == None) {

				DBG_PTR(m_logger, 8, "GNGAlgorith:: remove node because no edges");

#ifdef DEBUG_GMUM
				FOREACH(GNGEdge* edg2, m_g[nr])
				{
				 CERR("WARNING: GNGAlgorithm:: edges count of neighbours of erased node, shouldn't happen! ");
				}
#endif

				if (m_toggle_uniformgrid) {
                    ug->remove(m_g[nr].position);
                }
                else if(m_toggle_ann){
                    ann->remove(nr);
                    nRemovals += 1;
                }

				DBG_PTR(m_logger, 8,
						"GNGAlgorithm::Adapt() Erasing node "
								+ to_string<int>(nr));
				DBG_PTR(m_logger, 8,
						"GNGAlgorithm::Adapt() First coordinate "
								+ to_string<double>(m_g[nr].position[0]));

				m_g.deleteNode(nr);
			}
			if (m_g[nearest_0->nr].edgesCount == 0
					&& this->m_utility_option == None) {

				LOG_PTR(m_logger, 1,
						"GNGAlgorithm::Adapt() remove node because no edges, shouldn't happen"); //Shouldn't happen

				if (m_toggle_uniformgrid) {
                    ug->remove(m_g[nearest_0->nr].position);
                }
				else if(m_toggle_ann) {
                    ann->remove(nearest_0->nr);
                    nRemovals += 1;
                }

				m_g.deleteNode(nearest_0->nr);
				break;
			}
			if (edg != nearest_0->end())
				--edg;
			else
				break;
			DBG_PTR(m_logger, 3, "GNGAlgorith::Adapt::Removal completed");
		}
		++edg;
	}

	//erase nodes
	if (this->m_utility_option == BasicUtility)
		this->utilityCriterionCheck();

	return std::pair<double, int>(error, nearest.first);
}

double GNGAlgorithm::calculateAccumulatedError() {

	int maximum_index = m_g.get_maximum_index();
	m_accumulated_error = 0.0;

	if (this->m_toggle_lazyheap) {

		m_g.lock();
		int maximum_index = m_g.get_maximum_index();
		m_accumulated_error = 0.0;

		REP(i, maximum_index + 1)
			if (m_g.existsNode(i))
				m_accumulated_error += m_g[i].error;

		m_g.unlock();
		return m_accumulated_error;
	} else {
		m_g.lock();
		m_accumulated_error = 0.0;
		REP(i, maximum_index + 1)
			if (m_g.existsNode(i))
				m_accumulated_error += m_g[i].error;

		m_g.unlock();
		return m_accumulated_error;
	}
}



void GNGAlgorithm::resizeUniformGrid() {

	DBG_PTR(m_logger, 6, "GNGAlgorithm::Resize Uniform Grid");
	DBG_PTR(m_logger, 6,
			"GNGAlgorithm::Resize Uniform Grid old_l="
					+ to_string(ug->getCellLength()));
	DBG_PTR(m_logger, 6,
			"GNGAlgorithm::Resize Uniform Grid new_l="
					+ to_string(ug->getCellLength() / m_grow_rate));

	ug->new_l(ug->getCellLength() / m_grow_rate);

	int maximum_index = m_g.get_maximum_index();

	REP(i, maximum_index + 1)
		if (m_g.existsNode(i))
			ug->insert(m_g[i].position, m_g[i].nr);

}

GNGNode ** GNGAlgorithm::LargestErrorNodes() {
	DBG_PTR(m_logger, 2, "LargestErrorNodes::started procedure");

	GNGNode ** largest = new GNGNode*[2];

	largest[0] = 0;
	largest[1] = 0;
	double error = -1.0;

	REP(i, m_g.get_maximum_index() + 1)
		if (m_g.existsNode(i))
			error = std::max(error, m_g[i].error);

	REP(i, m_g.get_maximum_index() + 1)
		if (m_g.existsNode(i))
			if (m_g[i].error == error)
				largest[0] = &m_g[i];

	if (largest[0]->edgesCount == 0) { //{largest[0]->error=0; return largest;} //error?
		m_g.deleteNode(largest[0]->nr);
		return largest;
	}

	int j = 0;

	FOREACH(GNGEdge* edg, *largest[0])
	{
		++j;

		if (j == 1) {
			largest[1] = &m_g[(edg)->nr];
			error = largest[1]->error;
			continue;
		}

		double new_error = m_g[(edg)->nr].error;

		if (error < new_error) {
			error = new_error;
			largest[1] = &m_g[(edg)->nr];
		}
	}

	return largest;
}

void GNGAlgorithm::updateClustering() {
	gmum::scoped_lock<GNGDataset> db_lock(*g_db);
	for(unsigned int i=0;i<g_db->size();++i){
		set_clustering(i, _getNearestNeurons(g_db->getPosition(i), true).first);
	}
}

void GNGAlgorithm::runAlgorithm() { //1 thread needed to do it (the one that computes)    
    m_gng_status = m_gng_status_request = GNG_RUNNING;
	//Initialize global counters
	s = 0;
	c = 0; // cycle variable for lazyheap optimization

	while (g_db->size() < 2) {
        this->status_change_mutex.lock();
		while (m_gng_status_request == GNG_PAUSED) {
			if (m_gng_status_request == GNG_TERMINATED){
				m_gng_status = GNG_TERMINATED;
				status_change_mutex.unlock();
				return;
			}
			this->status_change_condition.wait(this->status_change_mutex);
		}
        this->status_change_mutex.unlock();
	}

	if (m_g.get_number_nodes() == 0) {
		gmum::scoped_lock<GNGDataset> db_lock(*g_db);
		gmum::scoped_lock<GNGGraph> graph_lock(m_g);
		randomInit();
	} else if (m_g.get_number_nodes() == 1) {
		throw BasicException("Incorrect passed graph to GNGAlgorithm");
	}

	//We have to calculate error so we will collect error from adapt
	//and when count is > dataset size we will set m_mean_error
	double accumulated_error = 0.0;
	double time_elapsed =0., time_elapsed_last_error=0.;
	int accumulated_error_count = 0, accumulated_error_count_last = 0;

	LOG_PTR(m_logger, 3, "GNGAlgorithm::init successful, starting the loop");

	for(int iteration=0; iteration<max_iter || max_iter==-1; ++iteration){
        this->status_change_mutex.lock();
		while (this->m_gng_status_request == GNG_PAUSED) {
			m_gng_status = m_gng_status_request;
			this->status_change_condition.wait(this->status_change_mutex);
		}
		if (this->m_gng_status_request == GNG_TERMINATED){
	        LOG_PTR(m_logger, 5, "GNGAlgorithm::terminate");
		    this->status_change_mutex.unlock();
			break;
		}
		this->status_change_mutex.unlock();
		m_gng_status = GNG_RUNNING;

		double dt =0.;
		boost::posix_time::ptime start = boost::posix_time::microsec_clock::local_time();

		for (s = 0; s < m_lambda; ++s) { //global counter!!

			const double * position, *vertex_data;
			unsigned int ex = 0;
			{
				//Fined grained locks are necessary to prevent deadlocks
				gmum::scoped_lock<GNGDataset> db_lock(*g_db);
				ex = g_db->drawExample();
				position = g_db->getPosition(ex);
				vertex_data = g_db->getExtraData(ex);

				gmum::scoped_lock<GNGGraph> graph_lock(m_g);
				std::pair<double, int> adapt_result = adapt(position, vertex_data);

				ASSERT(adapt_result.second >= 0);
				set_clustering(ex, adapt_result.second);
	
				accumulated_error += adapt_result.first;
				
				accumulated_error_count += 1;
			}

		}

#ifdef GMUM_DEBUG
		for (int i = 0; i <= m_g.get_maximum_index(); ++i) { //another idea for storing list of actual nodes?
			if (m_g.existsNode(i) && m_g[i].edgesCount == 0 && m_utility_option == None) {
				CERR("Error at " + to_string<int>(i)));
			}
		}
#endif

		dt = ((boost::posix_time::microsec_clock::local_time() - start).total_milliseconds()+ 1.)/1000.0 ;
		time_elapsed += dt;
		time_elapsed_last_error += dt;

		//Calculate mini-batch error
		if ((time_elapsed_last_error > 0.1 && accumulated_error_count > 5 * m_g.get_number_nodes()) ||
				accumulated_error_count > 15 * m_g.get_number_nodes()) {
			gmum::scoped_lock<gmum::fast_mutex> stat_lock(m_statistics_mutex);

			m_mean_error.push_back(make_pair(time_elapsed,
					accumulated_error/(double)accumulated_error_count
					));

			accumulated_error_count_last = accumulated_error_count;
			time_elapsed_last_error = 0.0;
			accumulated_error = 0.0;
			accumulated_error_count = 0;
		}

		{
			gmum::scoped_lock<GNGGraph> graph_lock(m_g);
			addNewNode();

			if (m_toggle_uniformgrid && ug->check_grow()) {
				DBG_PTR(m_logger, 10, "GNGAlgorithm:: resizing uniform grid");
				resizeUniformGrid();
			}

			++c; //epoch
			if (!m_toggle_lazyheap)
				decreaseAllErrors();
			if (this->m_utility_option == BasicUtility)
				decreaseAllUtility();
		}
		++m_iteration;

		DBG_PTR(m_logger, 9, "GNGAlgorithm::iteration "+to_string(m_iteration));
	}
	m_gng_status = GNG_TERMINATED;
	DBG_PTR(m_logger, 30, "GNGAlgorithm::Terminated server");
}










/** Start algorithm loop */
void GNGAlgorithm::run(bool synchronized) {
    //TODO: refactor run to resume?
    if(m_gng_status == GNG_TERMINATED){
        return;
    }

    if(m_gng_status != GNG_RUNNING){
	    m_gng_status_request = GNG_RUNNING;
	    this->status_change_condition.notify_all();
    }
    if(this->g_db->size() > 2 && synchronized){ 
        //Algorithm should start. Run is synchronized. 
        //Terminated is also accepted state
        while(m_gng_status == GNG_PAUSED){
            gmum::sleep(10);
        }
    }
}

bool GNGAlgorithm::isRunning(){
	return this->m_gng_status == GNG_RUNNING;
}

/** Pause algorithm loop */
void GNGAlgorithm::pause(bool synchronized) {
    if(this->m_gng_status != GNG_PAUSED){
	    this->m_gng_status_request = GNG_PAUSED;
	    this->status_change_condition.notify_all();
    }
    if(this->g_db->size() > 2 && synchronized){
         //Terminated is also accepted state
         while(m_gng_status == GNG_RUNNING){
			 this->m_gng_status_request = GNG_PAUSED;
             gmum::sleep(10);
         }
    }
}

/** Terminate the algorithm */
void GNGAlgorithm::terminate(bool synchronized) {
    if(this->m_gng_status != GNG_TERMINATED){
	    this->m_gng_status_request = GNG_TERMINATED;
	    this->status_change_condition.notify_all();
    }
    if(synchronized){
        while(m_gng_status == GNG_RUNNING){
			this->m_gng_status_request = GNG_TERMINATED;
            gmum::sleep(10);        
        }
    }
}

void GNGAlgorithm::setMaxNodes(int value) {
	m_max_nodes = value;
}

int GNGAlgorithm::getIteration() const{
	return m_iteration;
}

unsigned GNGAlgorithm::getErrorIndex() const{
	return m_mean_error.size();
}

double GNGAlgorithm::getMeanError() {

	gmum::scoped_lock<gmum::fast_mutex> alg_lock(m_statistics_mutex);
	DBG_PTR(m_logger, 3, gmum::to_string(m_mean_error.size()));
	if(m_mean_error.size() == 0){
		return -1.0;
	}else{

		return m_mean_error[m_mean_error.size()-1].second;
	}
}

vector<pair<double, double> > GNGAlgorithm::getMeanErrorStatistics() {
	gmum::scoped_lock<gmum::fast_mutex> alg_lock(m_statistics_mutex);
	if(m_mean_error.size() == 0){
		return vector<pair<double, double> >(1, make_pair<double,double>(0., std::numeric_limits<double>::max()));
	}else{
		return vector<pair<double, double> >(m_mean_error.begin(), m_mean_error.end());
	}
}

//Retrieve clustering result.
//@note pauses algorithm as many
const vector<int> & GNGAlgorithm::get_clustering(){
	bool was_running = false;
	if(isRunning()){
		was_running = true;
		pause();
	}
	updateClustering();
	vector<int> & result = clustering_result;
	if(was_running)
		run();

	return result;
}

 GNGAlgorithm::~GNGAlgorithm() {
	delete[] m_betha_powers_to_n;
	delete[] m_betha_powers;
}



std::pair<int, int> GNGAlgorithm::_getNearestNeurons(const double *ex, bool exact){
	if (m_toggle_uniformgrid) {
			DBG_PTR(m_logger, 1, "GNGAlgorithm::Adapt::Graph size " + to_string(m_g.get_number_nodes()));
			std::vector<int> nearest_index = ug->findNearest(ex, 2); //TwoNearestNodes(ex->position);
			DBG_PTR(m_logger, 1, "GNGAlgorithm::Adapt::Found nearest");

			#ifdef GMUM_DEBUG_2
					if (nearest_index[0] == nearest_index[1]) {
						throw BasicException("Found same nearest_indexes");  //something went wrong (-1==-1 też)
					}
			#endif


			#ifdef GMUM_DEBUG_2
					ASSERT(m_g[nearest_index[1]].position > m_g.get_dist(m_g[nearest_index[0]].position, ex));
			#endif

        return std::pair<int, int>(nearest_index[0], nearest_index[1]);

    }
	else if(m_toggle_ann && !exact){

        long key1, key2;
        int k=2;

        vector<double> query{ex, ex+dim};
        //vector of struct {key, distance}
        //insert efSearch
        auto results = ann->search( query, k, m_efSearch);
        nSearches += 1;
        key1 = results.front().key;
        key2 = results.back().key;
        //ann->search(1, ex, k, D, I);

        //ann->add_with_ids(2, m_g[0].position, ids);
            // code block

        DBG_PTR(m_logger, 1, "GNGAlgorithm::Adapt::Found nearest");

        // For test measure purposes only, calculate recall compared to exact ann
        if (m_recall){
            approxSearchCount += 1;

            // do normal search
            int start_index = 0;
            while (!m_g.existsNode(start_index))
                ++start_index;

            double dist0 = m_g.get_dist(ex, m_g[start_index].position);
            int best_0 = start_index, best_1 = -1;
            for (int i = start_index + 1; i <= m_g.get_maximum_index(); ++i) {
                if (m_g.existsNode(i)) {
                    double new_dist = m_g.get_dist(ex, m_g[i].position);
                    if (dist0 > new_dist) {
                        dist0 = new_dist;
                        best_0 = i;
                    }
                }
            }

            DBG_PTR(m_logger, 1, "finding next\n");

            start_index = 0;
            while (!m_g.existsNode(start_index) || start_index == best_0)
                ++start_index;
            double dist1 = m_g.get_dist(ex, m_g[start_index].position);
            best_1 = start_index;

            for (int i = start_index + 1; i <= m_g.get_maximum_index(); ++i) { //another idea for storing list of actual nodes?
                if (m_g.existsNode(i) && i != best_0) {
                    double new_dist = m_g.get_dist(ex, m_g[i].position);
                    if (dist1 > new_dist) {
                        dist1 = new_dist;
                        best_1 = i;
                    }
                }
            }

            if(best_0 == key1){
                key1Count += 1;
            }

            if(best_1 == key2){
                key2Count += 1;
            }
        }



#ifdef GMUM_DEBUG_2
        if (nearest_index[0] == nearest_index[1]) {
            throw BasicException("Found same nearest_indexes");  //something went wrong (-1==-1 też)
        }
#endif


#ifdef GMUM_DEBUG_2
        ASSERT(m_g[nearest_index[1]].position > m_g.get_dist(m_g[nearest_index[0]].position, ex));
#endif

        return std::pair<int, int>(key1, key2);

    } else {

        DBG_PTR(m_logger, 1, "GNGAlgorithm::just called TwoNearestNodes");

			int start_index = 0;
			while (!m_g.existsNode(start_index))
				++start_index;

			double dist0 = m_g.get_dist(ex, m_g[start_index].position);
			int best_0 = start_index, best_1 = -1;
			for (int i = start_index + 1; i <= m_g.get_maximum_index(); ++i) {
				if (m_g.existsNode(i)) {
					double new_dist = m_g.get_dist(ex, m_g[i].position);
					if (dist0 > new_dist) {
						dist0 = new_dist;
						best_0 = i;
					}
				}
			}

			DBG_PTR(m_logger, 1, "finding next\n");

			start_index = 0;
			while (!m_g.existsNode(start_index) || start_index == best_0)
				++start_index;
			double dist1 = m_g.get_dist(ex, m_g[start_index].position);
			best_1 = start_index;

			for (int i = start_index + 1; i <= m_g.get_maximum_index(); ++i) { //another idea for storing list of actual nodes?
				if (m_g.existsNode(i) && i != best_0) {
					double new_dist = m_g.get_dist(ex, m_g[i].position);
					if (dist1 > new_dist) {
						dist1 = new_dist;
						best_1 = i;
					}
				}
			}


			#ifdef GMUM_DEBUG_2
				ASSERT(dist1 > dist0);
			#endif

			return std::pair<int, int>(best_0, best_1);
		}
}


void GNGAlgorithm::resetUniformGrid(double * orig, double *axis, double l) {
	ug->purge(orig, axis, l);
	int maximum_index = m_g.get_maximum_index();

	REP(i, maximum_index + 1)
	{
		if (m_g.existsNode(i))
			ug->insert(m_g[i].position, m_g[i].nr);
	}
}

bool GNGAlgorithm::stoppingCriterion() {
	return m_g.get_number_nodes() > m_max_nodes;
}

void GNGAlgorithm::increaseErrorNew(GNGNode * node, double error) {
	fixErrorNew(node);
	ASSERT(m_lambda - s <= m_betha_powers_size -1);
	node->error += m_betha_powers[m_lambda - s] * error;
	errorHeap.updateLazy(node->nr);
}

void GNGAlgorithm::fixErrorNew(GNGNode * node) {

	if (node->error_cycle == c)
		return;

	while(c - node->error_cycle > m_betha_powers_to_n_length - 1){
		DBG_PTR(m_logger, 5, "Recreating m_betha_powers_to_n");
		delete[] m_betha_powers_to_n;
		m_betha_powers_to_n_length *= 2;
		m_betha_powers_to_n = new double[m_betha_powers_to_n_length];
		REP(i, m_betha_powers_to_n_length)
		m_betha_powers_to_n[i] = std::pow(m_betha, m_lambda * (double) (i));
	}

	ASSERT(c - node->error_cycle  <= m_betha_powers_to_n_length -1);

	node->error = m_betha_powers_to_n[c - node->error_cycle] * node->error;
	node->error_cycle = c;

}


void GNGAlgorithm::set_clustering(unsigned int ex, unsigned int node_idx){

	if(ex + 1 > clustering_result.size()){
		DBG_PTR(m_logger, 6, "Resizing clustering_result to "+to_string(g_db->size()));
		clustering_result.resize(g_db->size());
	}

	//Can potentially happen in case of shrinkage of dataset size
	if(ex + 1 > clustering_result.size()){
		CERR("g_db->size mismatch with ex index?\n");
		return;
	}


	clustering_result[ex] = node_idx;
}

double GNGAlgorithm::getMaximumError() const {
	double max_error = 0;
	int maximum_index = m_g.get_maximum_index();
	REP(i,maximum_index+1)
		if (m_g.existsNode(i))
			max_error = std::max(max_error, m_g[i].error);
	return max_error;
}

void GNGAlgorithm::decreaseAllErrorsNew() {
	return;
}

void GNGAlgorithm::decreaseErrorNew(GNGNode * node) {
	fixErrorNew(node);
	node->error = m_alpha * node->error;
	errorHeap.updateLazy(node->nr);
}

void GNGAlgorithm::setErrorNew(GNGNode * node, double error) {
	node->error = error;
	node->error_cycle = c;
	errorHeap.insertLazy(node->nr);
}

void GNGAlgorithm::increaseError(GNGNode * node, double error) {
	node->error += error;
}

void GNGAlgorithm::decreaseAllErrors() {
	int maximum_index = m_g.get_maximum_index();
	REP(i,maximum_index+1)
		if (m_g.existsNode(i))
			m_g[i].error = m_betha * m_g[i].error;
}

void GNGAlgorithm::decreaseError(GNGNode * node) {
	node->error = m_alpha * node->error;
}

void GNGAlgorithm::setError(GNGNode * node, double error) {
	node->error = error;
}

// Note: this code is not optimal and is inserted only for research purposes

double GNGAlgorithm::getUtility(int i) {
	return m_g[i].utility;
}

void GNGAlgorithm::setUtility(int i, double u) {
	m_g[i].utility = u;
}

void GNGAlgorithm::utilityCriterionCheck() {

	if (m_g.get_number_nodes() < 10)
		return; //just in case

	double max_error = this->getMaximumError();
	int maximum_index = m_g.get_maximum_index();

	double min_utility = 100000000;
	int min_utility_index = -1;

	for (int i = 0; i <= maximum_index; ++i)
		if (min_utility > getUtility(i)) {
			min_utility = getUtility(i);
			min_utility_index = i;
		}

	if (m_g.existsNode(min_utility_index) && max_error / getUtility(min_utility_index) > m_utility_k) {

		DBG_PTR(m_logger,2, "GNGAlgorithm:: removing node with utility "+gmum::to_string(getUtility(min_utility_index)) + " max error "+gmum::to_string(max_error));

		DBG_PTR(m_logger,2,gmum::to_string<double>(max_error));

		GNGNode::EdgeIterator edg = m_g[min_utility_index].begin();
		while (edg != m_g[min_utility_index].end()) {
			int nr = (*edg)->nr;
			edg = m_g.removeUDEdge(min_utility_index, nr);
		}

		m_g.deleteNode(min_utility_index);
		setUtility(min_utility_index, 0);
	}

}
void GNGAlgorithm::decreaseAllUtility() {
	int maximum_index = m_g.get_maximum_index();
	for (int i = 0; i <= maximum_index; ++i)
		if (m_g.existsNode(i))
			setUtility(i, getUtility(i) * (m_betha));
}












}

/* 
 * File:   UniformGrid.h
 * Author: staszek
 *
 * Created on August 19, 2012, 7:02 AM
 */

#ifndef UNIFORM_GRID_H
#define UNIFORM_GRID_H

#include <boost/smart_ptr/shared_ptr.hpp>
#include <globals.h>
#include <logger.h>
#include <utils.h>
#include <algorithm>
#include <vector>

using namespace std;

namespace gmum {
/**
 * Is not thread safe !! Very important (speed purposes)
 */
template<class VectorContainer, class ListContainer, class T = int>
class UniformGrid {
	typedef ListContainer Node;
	typedef VectorContainer NodeArray;

public:
	UniformGrid(float * origin, int *dim, int gng_dim, float m_grow_factor =
			1.5, float m_density_threshold = 2.0,
			float m_density_threshold_min = 0.4,
			boost::shared_ptr<Logger> logger = boost::shared_ptr<Logger>());

	UniformGrid(float * origin, float *axis, float l, int gng_dim,
			float m_grow_factor = 1.5, float m_density_threshold = 2.0,
			float m_density_threshold_min = 0.4,
			boost::shared_ptr<Logger> logger = boost::shared_ptr<Logger>());

	vector<T> findNearest(const float *p, int n = 2);

	// Calculates new size given growth factor
	long int calculate_new_size(float *origin, float *axis, float l);

	void purge(float *origin, int* dim, float l);

	void purge(float *origin, float *axis, float l);

	void new_l(float l);

	int insert(float *p, T x);

	bool remove(float *p);

	// Calculates if growing the grid will payoff
	bool check_grow();

	float getCellLength() const {
		return m_l;
	}

	float getDensity() const {
		return m_density;
	}

	int getCapacity() const {
		return SIZE(m_grid);
	}

	int getNodes() const {
		return m_nodes;
	}

	int getDimension(int axis) const {
		return m_dim[axis];
	}

	void setDistFunction(float (*dist_fnc)(T, float*)) {
		m_dist_fnc = dist_fnc;
	}

	~UniformGrid();

private:
	// Check if search was successful
	bool searchSuccessful(float min_dist = -1);

	void scanCell(int k, float* query);

	void crawl(int current_dim, int fixed_dim);

	bool scanCorners();

	T find(float *p);

private:
	/*** Maximum size pass which UG won't grow */
	static const int MAX_SIZE = 1000000;

	NodeArray m_grid;

	//global variables for search query
	int s_found_cells[4];
	float s_found_cells_dist[4];
	int s_search_query;

	int *s_center;
	int s_radius;
	int *s_pos;
	float *s_query;

	float (*m_dist_fnc)(T, float*); //distance function;

	float m_l;
	float m_h;

	float m_density;
	float m_density_threshold;
	float m_density_threshold_min;
	float m_grow_factor;

	int neighbourhood_size; //=3^d

	int gng_dim;

	//TODO: erase GNG_MAX_DIM
	float m_axis[GNG_MAX_DIM];

	int m_nodes;

	boost::shared_ptr<Logger> m_logger;

	int* m_dim; //number of uniform cells along certain axis

	int* m_tmp_int; //avoid alloc on heap all the time in calccell <- one thread!

	vector<int> m_neigh;

	//TODO: erase GNG_MAX_DIM
	float m_origin[GNG_MAX_DIM];

	int getIndex(int *p) {
		int value = p[0];
		float mul = m_dim[0];
		for (int i = 1; i < this->gng_dim; ++i) {
			value += p[i] * mul;
			mul *= m_dim[i];
		}

		return value;
	}
	int * calculateCell(const float *p) {
		//int * m_tmp_int = new int[this->gng_dim];
		for (int i = 0; i < this->gng_dim; ++i) {
			m_tmp_int[i] = (int) ((p[i] - m_origin[i]) / m_l);
		}
		return &m_tmp_int[0];
	}

	bool _inside(int x) {
		return (x) >= 0 && (x) < m_grid.size();
	}

	bool isZero(float x) {
		return x > -EPS && x < EPS;
	}

	unsigned int calculate_cell_side(float axis, float l, int old_dim) {
		return max(old_dim + 1, (int) ((axis) / (l)) + 1);
	}
};

#include <uniform_grid.hpp>

}

#endif

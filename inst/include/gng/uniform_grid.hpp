/*
 * File:   UniformGrid.hpp
 * Author: staszek
 *
 * Created on September 3, 2012, 9:03 AM
 */
using namespace gmum; 


template<class VectorContainer, class ListContainer, class T>
void UniformGrid<VectorContainer, ListContainer, T>::new_l(float l) {
	float *org = new float[this->gng_dim];
	float *axis = new float[this->gng_dim];
	memcpy(org, m_origin, this->gng_dim * sizeof(float));
	memcpy(axis, m_axis, this->gng_dim * sizeof(float));
	purge(org, axis, l);
	delete[] org;
	delete[] axis;
}
template<class VectorContainer, class ListContainer, class T>
void UniformGrid<VectorContainer, ListContainer, T>::purge(float *origin,
		float *axis_array, float l) {
	int * dim = new int[this->gng_dim];
	memcpy(m_axis, axis_array, sizeof(float) * this->gng_dim);

	// If length has not been specified, it means that we want to have minimum number of cells possible
	// Which amounts to picking the smallest axis
	if (l == -1.0) {
		float l_min = axis_array[0];
		REP(i, this->gng_dim)
		l_min = min(l_min, axis_array[0]);
		l = l_min * 1.01; // For numerical stability
	}

	REP(i, this->gng_dim)
	{
		dim[i] = calculate_cell_side(axis_array[i], l, m_dim[i]);
	}

	purge(origin, dim, l);
	delete[] dim;
}
template<class VectorContainer, class ListContainer, class T>
void UniformGrid<VectorContainer, ListContainer, T>::purge(float *origin,
		int* dim, float l) {
	m_l = l;
	memcpy(&m_dim[0], dim, sizeof(int) * this->gng_dim);
	memcpy(&m_origin, origin, sizeof(float) * this->gng_dim);

	m_density = 0.0;
	m_density_threshold = 0.1;
	m_grow_factor = 1.5;
	m_nodes = 0;

	m_grid.clear();

	int new_size = 1;

	REP(i, this->gng_dim)
	{
		new_size *= m_dim[i];
		REPORT(new_size);
		REPORT(m_dim[i]);
	}

	m_grid.resize(new_size);
}

template<class VectorContainer, class ListContainer, class T>
void UniformGrid<VectorContainer, ListContainer, T>::scanCell(int k,
		float* query) {
	float dist_candidate;

	// Not implemented search routine for > 2
	if (s_search_query != 2)
		throw "Not implemented for >2 search query..";

	BOOST_FOREACH(int node, m_grid[k])
	{

		dist_candidate = m_dist_fnc(node, query);

		//
		if (node != s_found_cells[1]
				&& (s_found_cells_dist[0] < 0
						|| dist_candidate <= s_found_cells_dist[0])) {

			//Overwrite worst
			s_found_cells_dist[0] = dist_candidate;
			s_found_cells[0] = node;

			//Swap it to the right place
			for (int j = 1; j < s_search_query; ++j) {
				if (s_found_cells_dist[j] < 0
						|| dist_candidate <= s_found_cells_dist[j]) {
					std::swap(s_found_cells[j], s_found_cells[j - 1]);
					std::swap(s_found_cells_dist[j], s_found_cells_dist[j - 1]);

				}
			}

		}

	}
}

template<class VectorContainer, class ListContainer, class T>
void UniformGrid<VectorContainer, ListContainer, T>::crawl(int current_dim,
		int fixed_dim) {

	if (current_dim == fixed_dim) {
		if (current_dim >= this->gng_dim - 1) {
			scanCell(getIndex(s_pos), s_query);
		}            //skip current dimension
		else
			crawl(current_dim + 1, fixed_dim);
	} else {
		int from, to;

		//skip corners
		if (current_dim < fixed_dim) {
			from = std::max(s_center[current_dim] - s_radius + 1, 0);
			to = std::min(s_center[current_dim] + s_radius - 1,
					m_dim[current_dim] - 1);
		} else {
			from = std::max(s_center[current_dim] - s_radius, 0);
			to = std::min(s_center[current_dim] + s_radius,
					m_dim[current_dim] - 1);
		}

		for (int i = from; i <= to; ++i) {

			s_pos[current_dim] = i;

			if (current_dim == this->gng_dim - 1) {
				scanCell(getIndex(s_pos), s_query);
			} else {
				crawl(current_dim + 1, fixed_dim);
			}
		}

	}

	return;
}

/** Uses cached values (s_pos, s_center, s_radius) to search cells */
template<class VectorContainer, class ListContainer, class T>
bool UniformGrid<VectorContainer, ListContainer, T>::scanCorners() {

	int left, right;

	bool scanned = false;

	memcpy(s_pos, s_center, sizeof(int) * this->gng_dim);

	REP(i, this->gng_dim)
	{
		left = s_center[i] - s_radius;
		right = s_center[i] + s_radius;

		if (s_center[i] - s_radius >= 0) {
			s_pos[i] = left;
			crawl(0, i);
			scanned = true;
		}
		if (s_center[i] + s_radius < m_dim[i]) {
			s_pos[i] = right;
			crawl(0, i);
			scanned = true;
		}

		s_pos[i] = s_center[i];
	}

	return scanned;
}

//mutates pos!

template<class VectorContainer, class ListContainer, class T>
int UniformGrid<VectorContainer, ListContainer, T>::insert(float *p, T x) {
	//memcpy(&m_copy[0],p,sizeof(float)*this->gng_dim);
	int * index = calculateCell(p);
	int k = getIndex(index);

	if (!_inside(k))
		return 0;

	m_grid[k].push_back(x);
	m_nodes++;
	m_density = (float) m_nodes / (float) SIZE(m_grid);
	return k;
}

template<class VectorContainer, class ListContainer, class T>
std::vector<T> UniformGrid<VectorContainer, ListContainer, T>::findNearest(
		const float *p, int n) { //returns indexes (values whatever)
	s_search_query = n;


	int * center = calculateCell(p);

	REP(i, n)
	{
		s_found_cells[i] = -1;
		s_found_cells_dist[i] = -1;
	}
	memcpy(s_center, center, sizeof(int) * this->gng_dim);

	memcpy(s_query, p, sizeof(float) * this->gng_dim);

	int center_id = getIndex(center);

	int size = SIZE(m_grid);

	//Check if inside uniform grid
	if (!_inside(center_id)) {
		vector<int> returned_value;
		returned_value.push_back(-1);
		returned_value.push_back(-1);
		return returned_value;

	}
	float border, border_squared, tmp;
	s_found_cells_dist[0] = s_found_cells_dist[1] = -1;
	s_found_cells[0] = s_found_cells[1] = -1;

	//init of the search
	scanCell(center_id, s_query);

	if (s_found_cells[0] == s_found_cells[1] && s_found_cells[0] != -1) {
		DBG_PTR(m_logger, 10, "UniformGrid:: Found two same nodes in one cell!!");
	}

	for (int i = 0; i < this->gng_dim; ++i) {
		tmp = abs((p[i] - m_origin[i] - center[i] * m_l))
				< abs((p[i] - m_origin[i] - (center[i] + 1) * m_l)) ?
				abs((p[i] - m_origin[i] - center[i] * m_l)) :
				abs((p[i] - m_origin[i] - (center[i] + 1) * m_l));
		if (border > tmp || i == 0)
			border = tmp;
	}

	border_squared = border * border;

	s_radius = 0;

	DBG_PTR(m_logger, 2,
			"UniformGird:: scanned straightforward cell proceeding to crawling");

	//No more cells to search
	if (m_grid.size() == 1) {
		std::vector<T> ret(2);
		ret[1] = s_found_cells[0];
		ret[0] = s_found_cells[1];

		return ret;
	}

	while (!searchSuccessful(border_squared)) {
		++s_radius;
		border += m_l;
		border_squared = border * border;

		if (!scanCorners())
			break; //false if no cells to check (no cell checked)
	}

	std::vector<T> ret(2);

	//Reversed array - see scanCell
	ret[1] = s_found_cells[0];
	ret[0] = s_found_cells[1];

	return ret;
}

template<class VectorContainer, class ListContainer, class T>
bool UniformGrid<VectorContainer, ListContainer, T>::remove(float *p) { //returns indexes (values whatever)
	int * cell = calculateCell(p);
	int index = getIndex(cell);

	for(typename Node::iterator node = m_grid[index].begin();
			node != m_grid[index].end(); ++node)
	{
		if (isZero(m_dist_fnc(*node, p))) {
			m_grid[index].erase(node);
			--m_nodes;
			return true;
		}
	}
	return false;
}

template<class VectorContainer, class ListContainer, class T>
UniformGrid<VectorContainer, ListContainer, T>::~UniformGrid() {
	delete[] s_center;
	delete[] s_pos;
	delete[] s_query;
	delete[] m_dim;
	delete[] m_tmp_int;
}

//TODO: extract constructor base
template<class VectorContainer, class ListContainer, class T>
 UniformGrid<VectorContainer, ListContainer, T>::UniformGrid(float * origin, int *dim, int gng_dim, float m_grow_factor, float m_density_threshold,
		float m_density_threshold_min,
		boost::shared_ptr<Logger> logger) :
		m_dist_fnc(0), gng_dim(gng_dim), m_density_threshold(
				m_density_threshold), m_density_threshold_min(
				m_density_threshold_min), m_grow_factor(m_grow_factor), m_logger(
				logger) {
	neighbourhood_size = int(pow(3.0, (float) gng_dim));

	this->m_density_threshold = m_density_threshold;
	this->m_density_threshold_min = m_density_threshold_min;
	this->m_grow_factor = m_grow_factor;

	s_center = new int[this->gng_dim];

	s_pos = new int[this->gng_dim];
	s_query = new float[this->gng_dim];
	m_dim = new int[this->gng_dim]; //number of uniform cells along certain axis
	m_tmp_int = new int[this->gng_dim]; //avoid alloc on heap all the time in calccell <- one thread!

	//Zero
	for (int i = 0; i < this->gng_dim; ++i)
		s_center[i] = s_pos[i] = s_query[i] = m_dim[i] = m_tmp_int[i] = 0;


	purge(origin, dim, -1.0);
}

template<class VectorContainer, class ListContainer, class T>
bool UniformGrid<VectorContainer, ListContainer, T>::searchSuccessful(float min_dist) {
		REP(i, s_search_query)
		{
			if (s_found_cells[i] == -1 || s_found_cells_dist[i] > min_dist)
				return false;
		}
		return true;
	}

template<class VectorContainer, class ListContainer, class T>
UniformGrid<VectorContainer, ListContainer, T>::UniformGrid(float * origin, float *axis, float l, int gng_dim,
		float m_grow_factor, float m_density_threshold,
		float m_density_threshold_min,
		boost::shared_ptr<Logger> logger) :
		m_dist_fnc(0), gng_dim(gng_dim), m_density_threshold(
				m_density_threshold), m_density_threshold_min(
				m_density_threshold_min), m_grow_factor(m_grow_factor), m_logger(
				logger) {
	neighbourhood_size = int(pow(3.0, (float) gng_dim));

	s_center = new int[this->gng_dim];

	s_pos = new int[this->gng_dim];
	s_query = new float[this->gng_dim];
	m_dim = new int[this->gng_dim]; //number of uniform cells along certain axis
	m_tmp_int = new int[this->gng_dim]; //avoid alloc on heap all the time in calccell <- one thread!

	//Zero
	for (int i = 0; i < this->gng_dim; ++i)
		s_center[i] = s_pos[i] = s_query[i] = m_dim[i] = m_tmp_int[i] = 0;

	purge(origin, axis, l);

}


template<class VectorContainer, class ListContainer, class T>
long int UniformGrid<VectorContainer, ListContainer, T>::calculate_new_size(float *origin, float *axis, float l) {
	unsigned long int result = 1;

	REP(i, this->gng_dim)
	{
		result *= calculate_cell_side(axis[i], l, m_dim[i]);
		if (result > UniformGrid::MAX_SIZE)
			return -1;
	}
	return result;
}


template<class VectorContainer, class ListContainer, class T>
bool UniformGrid<VectorContainer, ListContainer, T>::check_grow() {
	unsigned long int result = this->calculate_new_size(m_origin, m_axis,
			m_l / m_grow_factor);

	if (result == -1)
		return false;

	float avg_density = m_nodes / (float) m_grid.size();
	float new_avg_density = m_nodes
			/ (float) (this->calculate_new_size(m_origin, m_axis,
					m_l / m_grow_factor));

	return avg_density > m_density_threshold
			&& new_avg_density > m_density_threshold_min;
}




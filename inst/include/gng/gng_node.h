/*
 * File: SHGraphDefs.h
 * Author: staszek
 *
 * Created on 11 sierpień 2012, 08:18
 */

#ifndef SHGRAPHDEFS_H
#define SHGRAPHDEFS_H

#include <globals.h>
#include <iostream>
#include <cmath>
#include <vector>
#include <algorithm>
#include <cstring>
#include <iterator>

/**
 * Basic interface for Edge in GNGGraph.
 */
class GNGEdge {
public:
	GNGEdge * rev;
	int nr;
	float error;
	int age;

	GNGEdge() :
			error(0), age(0) {
	}

	GNGEdge(int nr) :
			nr(nr), error(0.0), age(0) {
	}
};

//Dump format [N nodes] [dim] (floats) N * node->dump()

///Warning dev note: GNGNode fields have to be properly copied/initialized form 
///both GNGAlgorithm and 2 functions in GNGNode
///I should improve it shortly
class GNGNode: public std::vector<GNGEdge*> {
public:
	//TODO: change to GNGEdge, no need for dandling pointers
	typedef std::vector<GNGEdge*>::iterator EdgeIterator;

	float utility; //0
	int error_cycle; //1
	float error; //2
	int edgesCount; //3
	int nr; //4
	bool _position_owner; //5
	unsigned int dim; //6
	float extra_data; //7 - extra data that is voted among vertices when adapting
	float *position; //8... 8+dim-1

	// Construct empty GNGNode
	GNGNode() {
		//prevent memory corruption
		_position_owner = false;
		position = 0;
	}

	~GNGNode() {
		if (_position_owner) {
			delete[] position;
		}
	}

	float dist(GNGNode * gnode) const { //dist doesnt account for param
		using namespace std;
		float ret = 0;
		for (size_t i = 0; i < dim; ++i)
			ret += (this->position[i] - gnode->position[i])
					* (this->position[i] - gnode->position[i]);
		return sqrt(ret);
	}

	friend std::ostream& operator<<(std::ostream& out, const GNGNode & node) {
		out << node.nr << "(" << node.error << ")(";
		for (size_t i = 0; i < node.dim; ++i) {
			out << node.position[i] << ",";
		}
		out << ")";

		return out;
	}

	vector<float> dumpEdges() {
		vector<float> dump(1 + this->size(), 0.0);
		dump[0] = this->size();
		for (size_t i = 0; i < this->size(); ++i)
			dump[i + 1] = (*this)[i]->nr;
		return dump;
	}

	///Dumps to vector of numbers
	vector<float> dumpVertexData() {
		vector<float> dump(8 + dim, 0.0);
		dump[0] = utility;
		dump[1] = error_cycle;
		dump[2] = error;
		dump[3] = edgesCount;
		dump[4] = nr;
		dump[5] = (int) _position_owner;
		dump[6] = dim;
		dump[7] = extra_data;
		for (size_t i = 0; i < dim; ++i) {
			dump[i + 8] = position[i];
		}
		return dump;
	}
	//Loads from vector
	void loadVertexData(vector<float> & x, float * position_ptr) {

		utility = x[0];
		error_cycle = x[1];
		error = x[2];
		edgesCount = (int) x[3];
		nr = (int) x[4];
		_position_owner = x[5];
		dim = x[6];
		extra_data = x[7];
		position = position_ptr;
		for (size_t i = 0; i < dim; ++i) {
			position[i] = x[i + 8];
		}
	}
	void loadVertexData(vector<float>::iterator & itr, int gng_dim,
			float * position_ptr) {
		vector<float> dump;
		dump.reserve(8 + gng_dim);

		std::copy(itr + 1, itr + 9 + gng_dim, std::back_inserter(dump));
		std::advance(itr, (8 + gng_dim));

		this->loadVertexData(dump, position_ptr);
	}

};

#endif        /* SHGRAPHDEFS_H */

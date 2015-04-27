/*
 * LibSVMRunner.cpp
 *
 *  Created on: Apr 7, 2014
 *      Author: sacherus
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "libsvm_runner.h"
#include "svm_basic.h"
#include "svm_utils.h"

#define Malloc(type,n) (type *)malloc((n)*sizeof(type))
svm_parameter get_default_params();


LibSVMRunner::LibSVMRunner() {
	// TODO Auto-generated constructor stub
}

LibSVMRunner::~LibSVMRunner() {
	// TODO Auto-generated destructor stub
}

void LibSVMRunner::processRequest(SVMConfiguration& config) {

//	Training
	if (!config.isPrediction()) {
		svm_node** node = 0;
		if(config.isSparse()) {
            node = ArmaSpMatToSvmNode(config.sparse_data);
		} else {
			node = armatlib(config.data);
		}
		svm_parameter* param = configuration_to_problem(config);
		prob.l = config.target.n_rows;
		prob.y = vectlib(config.target);
		prob.x = node;
		save_model_to_config(config, param, prob);
		config.w = (config.support_vectors * config.alpha_y);

	} else {
		arma_prediction(config);
	}
}

bool LibSVMRunner::canHandle(SVMConfiguration& config) {
    if (config.use_example_weights) {
        return false;
    }
	return config.library == LIBSVM;
}

bool LibSVMRunner::save_model_to_config(SVMConfiguration& config,
		svm_parameter* param, svm_problem& problem) {

	const char *error_msg;

	error_msg = svm_check_parameter(&prob, param, config.log);

	if (error_msg) {
		LOG(config.log, LogLevel::ERR, "ERROR: " + to_string(error_msg))
		return false;
	}
	//int* nr = Malloc(int, 1);
	int* nclasses = Malloc(int, 1);

	model = svm_train(&prob, param, config.log);
	//*nr = config.support_vectors.n_rows; //support vectors
	*nclasses = model->nr_class;
	config.nr_class = model->nr_class;
	LOG(config.log, LogLevel::TRACE, "save_model_to_config writing down alphas, nclasses= " + svm_to_str(config.nr_class));

	int nr_support_vectors = model->l;
    //conversion vec->SpCol
    arma::vec alpha_y_tmp = arma::vec(model->sv_coef[0], nr_support_vectors);
    //not my fault. Arma fault :)
	config.alpha_y = arma::zeros(nr_support_vectors);
    for(int i=0;i<nr_support_vectors;++i){
        if(alpha_y_tmp(i) != 0){
            config.alpha_y(i) = alpha_y_tmp(i);
        }
    }

	if(config.nr_class != 2) {
		throw std::invalid_argument( "Code is not implemented for more than 2 classes right now");
	}

	config.b = -model->rho[0];
	// memcpy(config.rho, ,
	// 		config.nr_class * (config.nr_class - 1) / 2 * sizeof(double));

	//config.sv_indices = (int*) malloc(config.l * sizeof(int));
	//svm_get_sv_indices(model, config.sv_indices, config.log);

	int dim = config.getDataDim();
	ASSERT(dim > 0);
	//config.support_vectors = SvmUtils::libtoarma(model->SV, nr_support_vectors, dim);
    //
	LOG(config.log, LogLevel::TRACE, "save_model_to_config writing down SV, n_SV = " + svm_to_str(nr_support_vectors));
	config.support_vectors = SvmUtils::SvmNodeToArmaSpMat(model->SV, nr_support_vectors, dim);
	LOG(config.log, LogLevel::TRACE, "save_model_to_config wrote down SV, n_SV = " + svm_to_str(config.support_vectors.n_cols));
	LOG(config.log, LogLevel::TRACE, "save_model_to_config wrote down SV, dim = " + svm_to_str(config.support_vectors.n_rows));

	//	TODO: WTF!!!!!???
	if (config.svm_type < 2) {
		config.label = (int *) malloc(*nclasses * sizeof(int));
		config.nSV = (int *) malloc(*nclasses * sizeof(int));
		memcpy(config.label, model->label, *nclasses * sizeof(int));
		memcpy(config.nSV, model->nSV, *nclasses * sizeof(int));
	}

    config.neg_target = model->label[1];
    config.pos_target = model->label[0];

	svm_destroy_param(param,config.log);
	svm_free_and_destroy_model(&model,config.log);

	return true;
}

svm_model* LibSVMRunner::load_model_from_config(SVMConfiguration& config,
		svm_parameter* param) {

	const char *error_msg;
	error_msg = svm_check_parameter(&prob, param,config.log);

	if (error_msg) {
		LOG(config.log, LogLevel::ERR, "ERROR: " + to_string(error_msg))
		return 0;
	}

	model = Malloc(svm_model, 1);

	model->l =  config.getSVCount(); //support vectors number
	model->nr_class = config.nr_class;
	model->param = *param;

	model->sv_coef = (double **) malloc(model->nr_class * sizeof(double*));
	for (int i = 0; i < config.nr_class - 1; i++) {
		model->sv_coef[i] = (double *) malloc(config.getSVCount() * sizeof(double));
		std::copy(config.alpha_y.begin(), config.alpha_y.end(), model->sv_coef[i *  config.getSVCount()]);
	}

    model->SV = armatlib(arma::mat(config.support_vectors.t()));
	// FIXME: Why below is not working?
    //model->SV = ArmaSpMatToSvmNode(config.support_vectors);

	model->rho = (double *) malloc(
			config.nr_class * (config.nr_class - 1) / 2 * sizeof(double));

	//i need change sign in b
	double local_rho = -config.b;
	
	if(config.nr_class != 2) {
		throw std::invalid_argument( "Code is not implemented for more than 2 classes right now");
	}

	memcpy(model->rho, &local_rho,
			config.nr_class * (config.nr_class - 1) / 2 * sizeof(double));

	model->free_sv = 1;


	if (config.svm_type < 2) {
		model->label = (int *) malloc(config.nr_class * sizeof(int));
		model->nSV = (int *) malloc(config.nr_class * sizeof(int));
		memcpy(model->label, config.label, config.nr_class * sizeof(int));
		memcpy(model->nSV, config.nSV, config.nr_class * sizeof(int));
	}

	return model;
}

svm_parameter* LibSVMRunner::configuration_to_problem(
		SVMConfiguration& config) {
	svm_parameter* param;
	param = Malloc(svm_parameter, 1);
	param->svm_type = config.svm_type;
	// param->kernel_type = config.kernel_type;
	param->degree = config.degree;
	param->gamma = config.gamma;	// 1/num_features
	param->coef0 = config.coef0;
	param->nu = config.nu;
	param->cache_size = config.cache_size;
	param->C = config.C;
	param->eps = config.eps;
	param->p = config.p;
	param->shrinking = config.shrinking;
	param->probability = config.probability;
	param->nr_weight = config.class_weight_length;
	param->weight_label = config.libsvm_class_weights_labels;
	param->weight = config.libsvm_class_weights;
	param->max_iter = config.max_iter;

	if ( config.kernel_type == _LINEAR ) {
			param->kernel_type = LINEAR;
		}
		else if ( config.kernel_type == _POLY ) {
			param->kernel_type = POLY;
		}
		else if ( config.kernel_type == _RBF ) {
			param->kernel_type = RBF;
		}
		else if ( config.kernel_type == _SIGMOID ) {
			param->kernel_type = SIGMOID;
		}
	return param;
}

/*
 * Prediction
 */

struct svm_node *x;
int max_nr_attr = 64;

int predict_probability = 0;

/*
 struct svm_model* model;
 static char *line = NULL;
 static int max_line_len;
 */

/*
 * Armadillo matrix format to libsvm format
 */
struct svm_node ** LibSVMRunner::armatlib(arma::mat x) {
	int r = x.n_rows;
	int c = x.n_cols;
	struct svm_node** sparse;
	int i, ii, count;

	sparse = (struct svm_node **) malloc(r * sizeof(struct svm_node *));
	/* iterate over rows */
	for (i = 0; i < r; i++) {
		/* determine nr. of non-zero elements */
		/* iterate over columns */
		for (count = ii = 0; ii < c; ii++)
			if (x(i, ii) != 0)
				count++;

		/* allocate memory for column elements */
		sparse[i] = (struct svm_node *) malloc(
				(count + 1) * sizeof(struct svm_node));

		/* set column elements */
		for (count = ii = 0; ii < c; ii++)
			if (x(i, ii) != 0) {
				sparse[i][count].index = ii + 1;
				sparse[i][count].value = x(i, ii);
				count++;
			}

		/* set termination element */
		sparse[i][count].index = -1;
	}

	return sparse;
}

/*
 * Vector with target to lisvm format
 */
double * LibSVMRunner::vectlib(arma::vec target) {
	double* return_target;
	return_target = Malloc(double, target.n_rows);
	for (unsigned int i = 0; i < target.n_rows; i++) {
		return_target[i] = target(i);
	}
	return return_target;
}

svm_node** LibSVMRunner::SparseToSVMNode(arma::vec& x, int r, arma::Col<int>& rowindex, arma::Col<int>& colindex) { 
    struct svm_node** sparse;
    int i, ii, count = 0, nnz = 0;

    sparse = (struct svm_node **) malloc (r * sizeof(struct svm_node*));
    for (i = 0; i < r; i++) {
	/* allocate memory for column elements */
	nnz = rowindex[i+1] - rowindex[i];
	sparse[i] = (struct svm_node *) malloc ((nnz + 1) * sizeof(struct svm_node));

	/* set column elements */
	for (ii = 0; ii < nnz; ii++) {
	    sparse[i][ii].index = colindex[count];
	    sparse[i][ii].value = x[count];
	    count++;
	}

	/* set termination element */
	sparse[i][ii].index = -1;
    }    

    return sparse;
}


void LibSVMRunner::arma_prediction(SVMConfiguration& config) {
	struct svm_model* m;
	struct svm_node ** train;
	svm_parameter *params;
	int training_examples = config.getDataExamplesNumber();

	params = configuration_to_problem(config);
	m = load_model_from_config(config, params);

//	TODO: READ MODEL FROM PARAMETERS
	if(config.isSparse()) {
        train = ArmaSpMatToSvmNode(config.sparse_data);
	} else {
		train = armatlib(config.data);
	}
	double* ret = Malloc(double, training_examples);

	for (int i = 0; i < training_examples; i++)
		ret[i] = svm_predict(m, train[i],config.log);

	arma::vec ret_vec(ret, training_examples);
	config.result = ret_vec;
	/* TODO: CLEAN MEMORY IN BETTER WAY THINK OF OTHER PARAMETERS
	 * Clean memory:
	 * -array matrix
	 * -model
	 */
	for (int i = 0; i < training_examples; i++)
		free(train[i]);
	free(train);
	//TODO: THIS SHOULD WORK WITH PREDICTIONS 2X, now it's not working
//	svm_free_and_destroy_model(&m);
	svm_destroy_param(params,config.log);
	free(ret);
}

svm_node **LibSVMRunner::ArmaSpMatToSvmNode(arma::sp_mat & sparse_data) {
    int max_rows = sparse_data.n_rows + 1;
    svm_node **sn = new svm_node*[sparse_data.n_cols + 1];
    svm_node * tmp_col = new svm_node[max_rows];
    long int current_col_counter;
    long int row;
    for (unsigned int col = 0; col < sparse_data.n_cols; ++col) {
        current_col_counter = 0;
        row = -1;

        for (
            arma::sp_mat::iterator it = sparse_data.begin_col(col);
            it != sparse_data.end_col(col); ++it
        ) {
            tmp_col[current_col_counter].value = *it;
            tmp_col[current_col_counter++].index = it.row() + 1;
        }
        
        sn[col] = new svm_node[current_col_counter + 1];
        memcpy(sn[col], tmp_col, current_col_counter * sizeof(svm_node));
        sn[col][current_col_counter].index = -1.0;
    }
    delete [] tmp_col;
    return sn;
}


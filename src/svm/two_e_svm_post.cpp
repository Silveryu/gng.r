#include <cmath>

#include "two_e_svm_post.h"
#include "svm_handler.h"
#include "svm_basic.h"
#include "svm_utils.h"

using namespace arma;
using namespace std;

void TwoeSVMPostprocessor::processRequest(SVMConfiguration &data) {
  if (!data.prediction) {

		//TODO: Use copying memory for better memory optimalization
		//DIM PROJECTION: examps x dim x dim x 1 = exams x 1
    	arma::vec w = data.inv_of_sqrt_of_cov.t() * arma::vec(data.w);
    	
		double p_plus = stddev(SvmUtils::matrixByValue(data.data, data.target, data.pos_target) * w);
		double p_minus = stddev(SvmUtils::matrixByValue(data.data, data.target, data.neg_target) * w);
    
		//-rho = b
		//TODO: consider multiclass examples
		double b_dash = data.getB() + (p_plus - p_minus) / (p_plus + p_minus);
		data.setB(b_dash);

		// keep w as sparse 
		data.w = arma::sp_mat(w.n_elem, 1);
    	for (int i = 0; i != w.n_elem; ++i) {
      		if (w(i) != 0) data.w(i,0) = w(i);
    	}
	}
	data.data = data.tmp_data;
	data.target = data.tmp_target;
}

bool TwoeSVMPostprocessor::canHandle(SVMConfiguration& data) {
	return true;
}

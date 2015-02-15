#ifndef CLUSTERPARAMS_HPP
#define CLUSTERPARAMS_HPP

#include <armadillo>
#include <string>
namespace gmum {

enum ClusterType {
    kstandard, kfull, kdiagonal, ksphere, kfsphere, kno_type, kmix, kcustom
};

/**
 * You need to store somewhere data about cluster. Each type can have its own.
 */
struct ClusterParams {
    ClusterType type;
    ClusterParams(ClusterType _type) : type(_type) { }
};

struct ClusterFullParams: public ClusterParams {
    bool cov_mat_set;
    arma::mat cov_mat;
    ClusterFullParams(arma::mat _cov_mat) : ClusterParams(kfull), cov_mat_set(true), cov_mat(_cov_mat) { }
    ClusterFullParams() : ClusterParams(kfull), cov_mat_set(false) { }
};

struct ClusterFsphereParams: public ClusterParams {
    bool radius_set;
    double radius;
    ClusterFsphereParams(double _radius) : ClusterParams(kfsphere), radius_set(true), radius(_radius) { }
    ClusterFsphereParams() : ClusterParams(kfsphere), radius_set(false) { }
};

struct ClusterCustomParams: public ClusterParams {
    bool function_name_set;
    std::string function_name;
    ClusterCustomParams(std::string _function_name) : ClusterParams(kcustom), function_name_set(true), function_name(_function_name) { }
    ClusterCustomParams() : ClusterParams(kcustom), function_name_set(false) { }
};

}

#endif

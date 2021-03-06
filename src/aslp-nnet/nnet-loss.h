// nnet/nnet-loss.h

// Copyright 2011-2015  Brno University of Technology (author: Karel Vesely)
// Copyright 2016  ASLP (Author: zhangbinbin liwenpeng duwei)

// See ../../COPYING for clarification regarding multiple authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
// THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED
// WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE,
// MERCHANTABLITY OR NON-INFRINGEMENT.
// See the Apache 2 License for the specific language governing permissions and
// limitations under the License.

#ifndef ASLP_NNET_NNET_LOSS_H_
#define ASLP_NNET_NNET_LOSS_H_

#include "base/kaldi-common.h"
#include "util/kaldi-holder.h"
#include "aslp-cudamatrix/cu-matrix.h"
#include "aslp-cudamatrix/cu-vector.h"
#include "aslp-cudamatrix/cu-array.h"
#include "hmm/posterior.h"

namespace kaldi {
namespace aslp_nnet {


class LossItf {
 public:
  LossItf() { 
    tmp_frame_weights_ = new Vector<BaseFloat>;
  }
  virtual ~LossItf() { 
    delete tmp_frame_weights_;
  }

  /// Evaluate cross entropy using target-matrix (supports soft labels),
  virtual void Eval(const VectorBase<BaseFloat> &frame_weights, 
            const CuMatrixBase<BaseFloat> &net_out, 
            const CuMatrixBase<BaseFloat> &target,
            CuMatrix<BaseFloat> *diff) = 0;

  /// Evaluate cross entropy using target-posteriors (supports soft labels),
  virtual void Eval(const VectorBase<BaseFloat> &frame_weights, 
            const CuMatrixBase<BaseFloat> &net_out, 
            const Posterior &target,
            CuMatrix<BaseFloat> *diff) = 0;
  /// Evaluate without frame weight, add for simplify the train interface
  virtual void Eval(const CuMatrixBase<BaseFloat> &net_out, 
            const Posterior &target,
            CuMatrix<BaseFloat> *diff) {
    tmp_frame_weights_->Resize(target.size(), kUndefined);
    tmp_frame_weights_->Set(1.0);
    Eval(*tmp_frame_weights_, net_out, target, diff);
  }
  /// Generate string with error report,
  virtual std::string Report() = 0;

  /// Get loss value (frame average),
  virtual BaseFloat AvgLoss() = 0;
 protected:
  Vector<BaseFloat> *tmp_frame_weights_; 
};


class Xent : public LossItf {
 public:
  Xent() : frames_(0.0), correct_(0.0), loss_(0.0), entropy_(0.0), 
           likelyhood_(0.0), 
           frames_progress_(0.0), loss_progress_(0.0), 
           entropy_progress_(0.0), likelyhood_progress_(0.0) { }
  ~Xent() { }

  /// Evaluate cross entropy using target-matrix (supports soft labels),
  void Eval(const VectorBase<BaseFloat> &frame_weights, 
            const CuMatrixBase<BaseFloat> &net_out, 
            const CuMatrixBase<BaseFloat> &target,
            CuMatrix<BaseFloat> *diff);

  /// Evaluate cross entropy using target-posteriors (supports soft labels),
  void Eval(const VectorBase<BaseFloat> &frame_weights, 
            const CuMatrixBase<BaseFloat> &net_out, 
            const Posterior &target,
            CuMatrix<BaseFloat> *diff);
  
  /// Generate string with error report,
  std::string Report();

  /// Get loss value (frame average),
  BaseFloat AvgLoss() {
    return (loss_ - entropy_) / frames_;
  }

 private: 
  double frames_;
  double correct_;
  double loss_;
  double entropy_;
  double likelyhood_;

  // partial results during training
  double frames_progress_;
  double loss_progress_;
  double entropy_progress_;
  double likelyhood_progress_;
  std::vector<float> loss_vec_;

  // weigting buffer,
  CuVector<BaseFloat> frame_weights_;
  CuVector<BaseFloat> target_sum_;

  // loss computation buffers
  CuMatrix<BaseFloat> tgt_mat_;
  CuMatrix<BaseFloat> xentropy_aux_;
  CuMatrix<BaseFloat> entropy_aux_;

  // likelyhood computation buffers 
  CuMatrix<BaseFloat> likelyhood_aux_;

  // frame classification buffers, 
  CuArray<int32> max_id_out_;
  CuArray<int32> max_id_tgt_;
};


class Mse : public LossItf {
 public:
  Mse() : frames_(0.0), loss_(0.0), 
          frames_progress_(0.0), loss_progress_(0.0) { }
  ~Mse() { }

  /// Evaluate mean square error using target-matrix,
  void Eval(const VectorBase<BaseFloat> &frame_weights, 
            const CuMatrixBase<BaseFloat>& net_out, 
            const CuMatrixBase<BaseFloat>& target,
            CuMatrix<BaseFloat>* diff);

  /// Evaluate mean square error using target-posteior,
  void Eval(const VectorBase<BaseFloat> &frame_weights, 
            const CuMatrixBase<BaseFloat>& net_out, 
            const Posterior& target,
            CuMatrix<BaseFloat>* diff);
  
  /// Generate string with error report
  std::string Report();

  /// Get loss value (frame average),
  BaseFloat AvgLoss() {
    return loss_ / frames_;
  }

 private:
  double frames_;
  double loss_;
  
  double frames_progress_;
  double loss_progress_;
  std::vector<float> loss_vec_;

  CuVector<BaseFloat> frame_weights_;
  CuMatrix<BaseFloat> tgt_mat_;
  CuMatrix<BaseFloat> diff_pow_2_;
};


class MultiTaskLoss : public LossItf {
 public:
  MultiTaskLoss() { }
  ~MultiTaskLoss() {
    while (loss_vec_.size() > 0) {
      delete loss_vec_.back();
      loss_vec_.pop_back();
    }
  }

  /// Initialize from string, the format for string 's' is :
  /// 'multitask,<type1>,<dim1>,<weight1>,...,<typeN>,<dimN>,<weightN>'
  ///
  /// Practically it can look like this :
  /// 'multitask,xent,2456,1.0,mse,440,0.001'
  void InitFromString(const std::string& s);

  /// Evaluate mean square error using target-matrix,
  void Eval(const VectorBase<BaseFloat> &frame_weights, 
            const CuMatrixBase<BaseFloat>& net_out, 
            const CuMatrixBase<BaseFloat>& target,
            CuMatrix<BaseFloat>* diff) {
    KALDI_ERR << "This is not supposed to be called!";
  }

  /// Evaluate mean square error using target-posteior,
  void Eval(const VectorBase<BaseFloat> &frame_weights, 
            const CuMatrixBase<BaseFloat>& net_out, 
            const Posterior& target,
            CuMatrix<BaseFloat>* diff);
  
  /// Generate string with error report
  std::string Report();

  /// Get loss value (frame average),
  BaseFloat AvgLoss();

 private:
  std::vector<LossItf*>  loss_vec_;
  std::vector<int32>     loss_dim_;
  std::vector<BaseFloat> loss_weights_;
  
  std::vector<int32>     loss_dim_offset_;

  CuMatrix<BaseFloat>    tgt_mat_;
};

} // namespace aslp_nnet
} // namespace kaldi

#endif


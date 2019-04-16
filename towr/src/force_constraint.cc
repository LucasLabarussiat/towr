/******************************************************************************
Copyright (c) 2018, Alexander W. Winkler. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
******************************************************************************/

#include <towr/constraints/force_constraint.h>

#include <towr/variables/variable_names.h>

#include <towr/constraints/swing_constraint.h>
#include <towr/constraints/base_motion_constraint.h>
#include <towr/variables/cartesian_dimensions.h>
#include <iostream>

using namespace std;

namespace towr {

ForceConstraint::ForceConstraint (const HeightMap::Ptr& terrain,
                                  double force_limit,
                                  EE ee,
								  const SplineHolder& spline_holder)
    :ifopt::ConstraintSet(kSpecifyLater, "force-" + id::EEForceNodes(ee))
{
  terrain_ = terrain;
  fn_max_  = force_limit;
  mu_      = terrain->GetFrictionCoeff();
  ee_      = ee;
  base_angular_ = EulerConverter(spline_holder.base_angular_);
//  base_angular_ = spline_holder.base_angular_;

//  n_constraints_per_node_ = 1 + 2*k2D + 2; // positive normal force + 4 friction pyramid constraints
//  n_constraints_per_node_ = 1;
  n_constraints_per_node_ = 1 + 2*k2D;
  n_constraints_drift_node_ = 3;

  //new: plus pos and vel of every node: Node::n_derivatives*k2D
//  n_constraints_per_node_ = 1 + 2*k2D + Node::n_derivatives*k2D;
//  n_constraints_per_node_ = 1 + 2*k2D + 1;

//  cout << "n_constraints_per_node: " << n_constraints_per_node_ << endl;
  cout << "ee_: " << ee_ << endl;

}

void
ForceConstraint::InitVariableDependedQuantities (const VariablesPtr& x)
{
  ee_force_  = x->GetComponent<NodesVariablesPhaseBased>(id::EEForceNodes(ee_));
  ee_motion_ = x->GetComponent<NodesVariablesPhaseBased>(id::EEMotionNodes(ee_));


//    Vector3d vector_base_to_ee_B = b_R_w*(vector_base_to_ee_W);
//base_angular_


    //  ee_motion_ = x->GetComponent<NodesVariablesPhaseBased>(ee_motion_id_);

  //take all nodes because we have pure driving:
  	pure_stance_force_node_ids_ = ee_force_->GetIndicesOfAllNodes();
//  	pure_swing_node_ids_ = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};

//  int constraint_count = pure_stance_force_node_ids_.size()*n_constraints_per_node_+pure_swing_node_ids_.size();
  	int nodes_per_phase = params_.force_polynomials_per_stance_phase_ + 1; //only the first phase, the others have one less (one node is always shared!)

//  	int constraint_count = pure_stance_force_node_ids_.size()*n_constraints_per_node_;
//  		int constraint_count = (1*nodes_per_phase+4*(nodes_per_phase-1))*n_constraints_per_node_ + (nodes_per_phase-1)*n_constraints_drift_node_;

  	int constraint_count = 0;
  	if (ee_ == 0 or ee_ == 1){
  		constraint_count = pure_stance_force_node_ids_.size()*n_constraints_per_node_;
  	}
  	if (ee_ == 2 or ee_ == 3){
  		constraint_count = (2*nodes_per_phase-1)*n_constraints_per_node_ + (nodes_per_phase-1)*n_constraints_drift_node_;
  	}
//
//  	if (ee_ == 0 or ee_ == 1){
//  		int constraint_count = 40*8;
//  	}
//  	else if (ee_ == 2 or ee_ == 3){
//  	int constraint_count = 40*8 + 39*3;
//  	}

  SetRows(constraint_count);
//  cout << "number of nodes: " << pure_stance_force_node_ids_.size() << endl;
//  cout << "number of TOTAL constraints (per node*nodes): " << constraint_count << endl;
//  for (int f_node_id : pure_stance_force_node_ids_) {
//	  cout << "node ids used: " << pure_stance_force_node_ids_[f_node_id] << ", " << endl;
//  }
}

Eigen::VectorXd
ForceConstraint::GetValues () const
{
  VectorXd g(GetRows());

  int row=0;
  auto force_nodes = ee_force_->GetNodes();
  auto nodes = ee_motion_->GetNodes();	//new
  for (int f_node_id : pure_stance_force_node_ids_) {
    int phase  = ee_force_->GetPhase(f_node_id, ee_);

//    EulerConverter::MatrixSXd b_R_w = base_angular_.GetRotationMatrixBaseToWorld(t).transpose();
//    	int ee_id = ee_;
////    	cout << "ee_id: " << ee_id << endl;
//
//    	//get times of different phases and for different ee
//    	double time_3nodes = 0.0;
//    	double t = 0.0; //get time from the current node!

    	//times designed for 20 nodes!
//    	if (phase == 0){
//    		if (f_node_id > 7 and f_node_id < 15 and (ee_id == 0 or ee_id == 1)){ //and add condition that this is only valid for front feet!
//    			t = 1.0 + (f_node_id - 7)*0.125;
//    			time_3nodes = 0.375;
//    		}
////    		t = (f_node_id+1)*0.0625;
//    		else if (f_node_id > 15 and (ee_id == 0 or ee_id == 1)){
//    			t = 2.0;
//    			time_3nodes = 0.375;
//    		}
//    		else if (ee_id == 2 or ee_id == 3){
//    			time_3nodes = 0.15;
//    		}
//    	}
//    	else if (phase == 2){
////    		t = (f_node_id-78)*0.0128 + 2;
//    		t = 2.0;
//    		time_3nodes = 0.075;
//    	}
//    	else if (phase == 1){
//    		time_3nodes = 0.15;
//    		t = 1 + (f_node_id - 19)*0.05;
//    	}

//    	Spline::GetLocalTime()

//    	  EulerConverter::MatrixSXd b_C_w = base_angular_.GetRotationMatrixBaseToWorld(t);
//    	  EulerConverter::MatrixSXd w_C_b = base_angular_.GetRotationMatrixBaseToWorld(t).transpose();
//
//    	  Vector3d v_wrt_b = w_C_b*nodes.at(f_node_id).at(kVel);
//    	  Vector3d v_b_y = {0, v_wrt_b(1), 0};
//    	  Vector3d v_b_x = {v_wrt_b(0), 0, 0};
//    	  Vector3d v_x = b_C_w*v_b_x;
//    	  Vector3d v_y = b_C_w*v_b_y;

//    if (phase == 0 or ee_ == 0 or ee_ == 3){
//    if (phase == 0){
//    Vector3d p = ee_motion_->GetValueAtStartOfPhase(phase); // doesn't change during stance phase
    //new
    Vector3d p = nodes.at(f_node_id).p();

    Vector3d n = terrain_->GetNormalizedBasis(HeightMap::Normal, p.x(), p.y());
    Vector3d f = force_nodes.at(f_node_id).p();

    // unilateral force
    g(row++) = f.transpose() * n; // >0 (unilateral forces)

    // frictional pyramid
    //new force restriction in t1 direction, so that movement allowed!


//    double z_rot = nodes.at(f_node_id).ang.p();
//    double z_rot = 0;
//    if (f_node_id > 6)
//    	double z_rot = (f_node_id-6)*0.083;
//
//    Eigen::Matrix3d RotMat_base_world_wrt_zrot = EulerConverter::GetRotationMatrixBaseToWorld({0.0,0.0,z_rot});
//    Eigen::Matrix3d RotMat_world_base_wrt_zrot = RotMat_base_world_wrt_zrot.inverse();


    Vector3d t1 = terrain_->GetNormalizedBasis(HeightMap::Tangent1, p.x(), p.y());
    Vector3d t2 = terrain_->GetNormalizedBasis(HeightMap::Tangent2, p.x(), p.y());

    //smooth z-forces
//    	auto curr = force_nodes.at(f_node_id);
//    	Vector3d prev = force_nodes.at(0).p().topRows<k3D>();
//    	Vector3d next = force_nodes.at(0).p().topRows<k3D>();
//
//    	Vector3d force_diff_xyz;
//    	Vector3d xyz_center;
//    	Vector3d des_force_change;
//
//    	if (f_node_id > 0 and f_node_id < (pure_stance_force_node_ids_.size()-1)){
//    		prev = force_nodes.at(f_node_id-1).p().topRows<k3D>();
//    		next = force_nodes.at(f_node_id+1).p().topRows<k3D>();
//
//    		force_diff_xyz    = next - prev;
//    		xyz_center      = prev + 0.5*force_diff_xyz;
//    		des_force_change = force_diff_xyz/time_3nodes;
//    	}
//    	else if (f_node_id == 0){
//    	    next = force_nodes.at(f_node_id+1).p();
//    	    //time zu time2nodes wechseln!
//    	    force_diff_xyz    = next - prev;
//    	    xyz_center      = prev;
//    	    double time_2nodes = time_3nodes/3*2;
//    	    des_force_change = force_diff_xyz/time_2nodes;
//    	}
//    	else if (f_node_id == (pure_stance_force_node_ids_.size()-1)){
//    				prev = force_nodes.at(f_node_id-1).p().topRows<k3D>();
//    	    	    next = force_nodes.at(f_node_id).p().topRows<k3D>();
//    	    	    force_diff_xyz    = next - prev;
//    	    	    xyz_center      = next;
//    	    	    double time_2nodes = time_3nodes/3*2;
//    	    	    des_force_change = force_diff_xyz/time_2nodes;
//    	    	}


    if (phase == 0 or phase == 1 or phase == 2 or phase == 3 or phase == 5){
    	g(row++) = f.transpose() * (t1 - mu_*n); // t1 < mu*n
    	g(row++) = f.transpose() * (t1 + mu_*n); // t1 > -mu*n
//    	g(row++) = f.transpose() * (t1 - mu_*n); // t1 < mu*n
//    	g(row++) = f.transpose() * (t1 + mu_*n); // t1 > -mu*n
    	g(row++) = f.transpose() * (t2 - mu_*n); // t2 < mu*n
    	g(row++) = f.transpose() * (t2 + mu_*n); // t2 > -mu*n
//    	g(row++) = nodes.at(f_node_id).v().y(); // keine y geschw. der vorderen Füsse!

//    	if (phase == 0 and (ee_ == 2 or ee_ == 3)){
//    		g(row++) = nodes.at(f_node_id).v().y();
//    		g(row++) = 0;
////    		g(row++) = nodes.at(f_node_id).v().x();
//    	}
//    	else if (f_node_id < 8){
//    		g(row++) = nodes.at(f_node_id).v().y();
//    		g(row++) = 0;
////    		g(row++) = nodes.at(f_node_id).v().x();
////    		g(row++) = 0;
//    	}
//    	else {
//    		g(row++) = 0;
//    		g(row++) = 0;
////    		g(row++) = v_y(0);
////    		g(row++) = v_y(1);
////    		g(row++) = v_x(0);		//caution, robot cant turn drift more than 90degrees,
//    								//otherwise it will have to drive in negative x direction
//    	}
    }

    else if (phase == 4){
    	g(row++) = f.transpose() * (t1 - mu_*n); // t1 = mu*n nur in positive x-richtung driften!
//    	    g(row++) = f.transpose() * (t1 + mu_*n); // t1 > -mu*n
    	g(row++) = f.transpose() * (t2 - mu_*n); // t2 = mu*n
//    	g(row++) = f.transpose() * (t2 + mu_*n); // t2 = -mu*n
//    	g(row++) = f.transpose() * (t1 + mu_*n); // t1 > -mu*n
//    	g(row++) = 0; //damit Anzahl constraints stimmt.
//    	g(row++) = 0; //damit Anzahl constraints stimmt.
//    	g(row++) = 0; //damit Anzahl constraints stimmt.
//    	g(row++) = 0; //damit Anzahl constraints stimmt.
//    	g(row++) = 0;
//    	g(row++) = 0;
    }

////    for (auto dim : {X,Y,Z}) {
//    for (auto dim : {Z}) {
////    	if (f_node_id > 0 and f_node_id < (pure_stance_force_node_ids_.size()-1)){
//      g(row++) = curr.p()(dim) - xyz_center(dim);
////      g(row++) = curr.v()(dim) - des_force_change(dim);
////    	}
////    	else {
////    		g(row++) = 0;
////    		g(row++) = 0;
////    	}
//    }

  }

  return g;
}

ForceConstraint::VecBound
ForceConstraint::GetBounds () const
{
  VecBound bounds;

  for (int f_node_id : pure_stance_force_node_ids_) {
	int phase  = ee_force_->GetPhase(f_node_id, ee_);

	if (phase == 0 or phase == 1 or phase == 2 or phase == 3 or phase == 5){
	bounds.push_back(ifopt::Bounds(0.0, fn_max_)); // unilateral forces
    bounds.push_back(ifopt::BoundSmallerZero); // f_t1 >  mu*n
    bounds.push_back(ifopt::BoundGreaterZero); // f_t1 < -mu*n
    bounds.push_back(ifopt::BoundSmallerZero); // f_t2 <  mu*n
    bounds.push_back(ifopt::BoundGreaterZero); // f_t2 > -mu*n
//    bounds.push_back(ifopt::BoundGreaterZero); //nur geschwindigkeit in pos. x-rtg!
//    bounds.push_back(ifopt::BoundZero); //no y-velocity
//    bounds.push_back(ifopt::BoundZero); //no y-velocity
//    bounds.push_back(ifopt::BoundGreaterZero);
//    bounds.push_back(ifopt::BoundGreaterZero);
//    bounds.push_back(ifopt::BoundZero);
//    bounds.push_back(ifopt::BoundZero);

    }

    else if (phase == 4) {
//    else {
    	bounds.push_back(ifopt::Bounds(0.0, fn_max_)); // unilateral forces
    	bounds.push_back(ifopt::BoundZero); // f_t1 =  mu*n
    	bounds.push_back(ifopt::BoundZero); // f_t2 =  mu*n
//    	bounds.push_back(ifopt::BoundZero); //
//    	bounds.push_back(ifopt::BoundZero); //
//    	bounds.push_back(ifopt::BoundZero); //
//    	bounds.push_back(ifopt::BoundZero); //
//    	bounds.push_back(ifopt::BoundZero); //
//    	bounds.push_back(ifopt::BoundZero);
    }
  }

  return bounds;
}

void
ForceConstraint::FillJacobianBlock (std::string var_set,
                                    Jacobian& jac) const
{

  if (var_set == ee_force_->GetName()) {
      int row = 0;
      for (int f_node_id : pure_stance_force_node_ids_) {


        // unilateral force
    	auto nodes = ee_motion_->GetNodes();	//new
    	Vector3d p = nodes.at(f_node_id).p();
        int phase   = ee_force_->GetPhase(f_node_id, ee_);
//        Vector3d p  = ee_motion_->GetValueAtStartOfPhase(phase); // doesn't change during phase
        Vector3d n  = terrain_->GetNormalizedBasis(HeightMap::Normal,   p.x(), p.y());
        Vector3d t1 = terrain_->GetNormalizedBasis(HeightMap::Tangent1, p.x(), p.y());
        Vector3d t2 = terrain_->GetNormalizedBasis(HeightMap::Tangent2, p.x(), p.y());

        for (auto dim : {X,Y,Z}) {
          int idx = ee_force_->GetOptIndex(NodesVariables::NodeValueInfo(f_node_id, kPos, dim));

          int row_reset=row;

          if (phase == 0 or phase == 1 or phase == 2 or phase == 3 or phase == 5){
          jac.coeffRef(row_reset++, idx) = n(dim);              // unilateral force
          jac.coeffRef(row_reset++, idx) = t1(dim)-mu_*n(dim);  // f_t1 <  mu*n
          jac.coeffRef(row_reset++, idx) = t1(dim)+mu_*n(dim);  // f_t1 > -mu*n
          jac.coeffRef(row_reset++, idx) = t2(dim)-mu_*n(dim);  // f_t2 <  mu*n
          jac.coeffRef(row_reset++, idx) = t2(dim)+mu_*n(dim);  // f_t2 > -mu*n
//              		jac.coeffRef(row_reset++, idx) = 0;
//              		jac.coeffRef(row_reset++, idx) = 0;



//          jac.coeffRef(row_reset++, idx) = 0;
//          jac.coeffRef(row_reset++, idx) = 0;
//          jac.coeffRef(row_reset++, idx) = 0; //adjust these!
//                    jac.coeffRef(row_reset++, idx) = 0;

          }

          if (phase == 4){
                    jac.coeffRef(row_reset++, idx) = n(dim);              // unilateral force
                    jac.coeffRef(row_reset++, idx) = t1(dim)-mu_*n(dim);  // f_t1 <  mu*n
//                    jac.coeffRef(row_reset++, idx) = t1(dim)+mu_*n(dim);  // f_t1 > -mu*n
                    jac.coeffRef(row_reset++, idx) = t2(dim)-mu_*n(dim);  // f_t2 >  mu*n
//                    jac.coeffRef(row_reset++, idx) = t2(dim)+mu_*n(dim);  // f_t2 > -mu*n
//                    jac.coeffRef(row_reset++, idx) = 0;
//                    jac.coeffRef(row_reset++, idx) = 0;
//                    jac.coeffRef(row_reset++, idx) = 0;
//                    jac.coeffRef(row_reset++, idx) = 0;
//                    jac.coeffRef(row_reset++, idx) = 0; //adjust these!
//                              jac.coeffRef(row_reset++, idx) = 0;
           }
        }

        if (phase == 4)
        	row += n_constraints_drift_node_;
        else
        	row += n_constraints_per_node_;
      }
    }

    if (var_set == ee_motion_->GetName()) {
      int row = 0;
      auto force_nodes = ee_force_->GetNodes();
      auto nodes = ee_motion_->GetNodes();								//new
      for (int f_node_id : pure_stance_force_node_ids_) {
        int phase  = ee_force_->GetPhase(f_node_id, ee_);
        int ee_node_id = ee_motion_->GetNodeIDAtStartOfPhase(phase);

//        Vector3d p = ee_motion_->GetValueAtStartOfPhase(phase); // doesn't change during pahse
        Vector3d p = nodes.at(f_node_id).p();							//new
        Vector3d f = force_nodes.at(f_node_id).p();

        for (auto dim : {X_,Y_}) {
          Vector3d dn  = terrain_->GetDerivativeOfNormalizedBasisWrt(HeightMap::Normal, dim, p.x(), p.y());
          Vector3d dt1 = terrain_->GetDerivativeOfNormalizedBasisWrt(HeightMap::Tangent1, dim, p.x(), p.y());
          Vector3d dt2 = terrain_->GetDerivativeOfNormalizedBasisWrt(HeightMap::Tangent2, dim, p.x(), p.y());

//          int idx = ee_motion_->GetOptIndex(NodesVariables::NodeValueInfo(ee_node_id, kPos, dim));
          int idx = ee_motion_->GetOptIndex(NodesVariables::NodeValueInfo(f_node_id, kPos, dim));
          int row_reset=row;

          // unilateral force
          jac.coeffRef(row_reset++, idx) = f.transpose()*dn;
//          if (phase == 0){
          // friction force tangent 1 derivative
//          jac.coeffRef(row_reset++, idx) = f.transpose()*(dt2-mu_*dn);
//                  	  jac.coeffRef(row_reset++, idx) = f.transpose()*(dt2+mu_*dn);


          // friction force tangent 2 derivative

          if (phase == 0 or phase == 1 or phase == 2 or phase == 3 or phase == 5){
        	  jac.coeffRef(row_reset++, idx) = f.transpose()*(dt1-mu_*dn);
        	  jac.coeffRef(row_reset++, idx) = f.transpose()*(dt1+mu_*dn);
        	  jac.coeffRef(row_reset++, idx) = f.transpose()*(dt2-mu_*dn);
        	  jac.coeffRef(row_reset++, idx) = f.transpose()*(dt2+mu_*dn);
//        	  jac.coeffRef(row_reset++, idx) = f.transpose()*(dt1-mu_*dn);
//        	            jac.coeffRef(row_reset++, idx) = f.transpose()*(dt1+mu_*dn);


        	  //derivatives falsch!!
//        	  jac.coeffRef(row_reset++, ee_motion_->GetOptIndex(NodesVariables::NodeValueInfo(f_node_id, kVel, dim))) =  0.0;
//        	  jac.coeffRef(row_reset++, ee_motion_->GetOptIndex(NodesVariables::NodeValueInfo(f_node_id, kVel, dim))) =  0.0;
//        	  jac.coeffRef(row_reset++, idx) = 0; //adjust these!
//        	            jac.coeffRef(row_reset++, idx) = 0;

//        	  if ((phase == 0 and (ee_ == 2 or ee_ == 3)) or f_node_id < 8){
//        		  if (dim == X_){
//        			  jac.coeffRef(row_reset++, ee_motion_->GetOptIndex(NodesVariables::NodeValueInfo(f_node_id,   kVel, dim))) = 0.0;
//        			  jac.coeffRef(row_reset++, idx) = 0;
//        		  }
//        		  if (dim == Y_){
//        	          	  jac.coeffRef(row_reset++, ee_motion_->GetOptIndex(NodesVariables::NodeValueInfo(f_node_id,   kVel, dim))) =  1.0;  // current node
//        	          	  jac.coeffRef(row_reset++, idx) = 0;
//        		  }
//        	   }
//        	    else {
////        	                		jac.coeffRef(row_reset++, ee_motion_->GetOptIndex(NodesVariables::NodeValueInfo(f_node_id,   kVel, dim))) = 0.0;
//        	                		jac.coeffRef(row_reset++, idx) = 0;
//        	                		jac.coeffRef(row_reset++, idx) = 0;
//        	    }
          }

          else if (phase == 4){
        	  jac.coeffRef(row_reset++, idx) = f.transpose()*(dt1-mu_*dn);
        	  jac.coeffRef(row_reset++, idx) = f.transpose()*(dt2-mu_*dn);
//        	  jac.coeffRef(row_reset++, idx) = f.transpose()*(dt2+mu_*dn);
//	          jac.coeffRef(row_reset++, idx) = f.transpose()*(dt1+mu_*dn);
//        	  jac.coeffRef(row_reset++, idx) = 0;
//        	  jac.coeffRef(row_reset++, idx) = 0;
//        	  jac.coeffRef(row_reset++, idx) = 0;
//        	  jac.coeffRef(row_reset++, idx) = 0;
//        	  jac.coeffRef(row_reset++, idx) = 0; //adjust these!
//        	            jac.coeffRef(row_reset++, idx) = 0;
          }
        }
//
//                row += n_constraints_per_node_;
//        if (phase == 0){
        if (phase == 4)
                row += n_constraints_drift_node_;
        else
        	row += n_constraints_per_node_;

//                	row += 8;
//                }
//                else if (phase == 1){
//                	row += 3;
//                }
//                else
//                	row += 1;
//        row += n_constraints_per_node_;
      }
  }
}

} /* namespace towr */

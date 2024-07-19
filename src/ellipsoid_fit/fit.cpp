/*      File: fit.cpp
*       This file is part of the program ellipsoid-fit
*       Program description : Ellipsoid fitting in C++ using Eigen. Widely inspired by https://www.mathworks.com/matlabcentral/fileexchange/24693-ellipsoid-fit
*       Copyright (C) 2018-2021 -  Benjamin Navarro (LIRMM) CK-Explorer (). All Right reserved.
*
*       This software is free software: you can redistribute it and/or modify
*       it under the terms of the LGPL license as published by
*       the Free Software Foundation, either version 3
*       of the License, or (at your option) any later version.
*       This software is distributed in the hope that it will be useful,
*       but WITHOUT ANY WARRANTY without even the implied warranty of
*       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*       LGPL License for more details.
*
*       You should have received a copy of the GNU Lesser General Public License version 3 and the
*       General Public License version 3 along with this program.
*       If not, see <http://www.gnu.org/licenses/>.
*/
#include <ellipsoid/fit.h>
#include <Eigen/Eigenvalues>

namespace ellipsoid {

Parameters fit(const Eigen::Matrix<double, Eigen::Dynamic, 3>& data,
                EllipsoidType type) {
    return fit(data, nullptr, nullptr, nullptr, type);
}

Parameters fit(const Eigen::Matrix<double, Eigen::Dynamic, 3>& data,
                Eigen::Matrix<double, 10, 1>* coefficients_p, 
                EllipsoidType type) {
    return fit(data, coefficients_p, nullptr, nullptr, type);
}

Parameters fit(const Eigen::Matrix<double, Eigen::Dynamic, 3>& data,
                Eigen::Vector3d* eval_p, Eigen::Matrix3d* evec_column_p,
                EllipsoidType type) {
    return fit(data, nullptr, eval_p, evec_column_p, type);
}

Parameters fit(const Eigen::Matrix<double, Eigen::Dynamic, 3>& data,
                Eigen::Matrix<double, 10, 1>* coefficients_p, 
                Eigen::Vector3d* eval_p, Eigen::Matrix3d* evec_column_p,
                EllipsoidType type) {
    Parameters params;

    const auto& x = data.col(0);
    const auto& y = data.col(1);
    const auto& z = data.col(2);

    auto x_sq = x.cwiseProduct(x).eval();
    auto y_sq = y.cwiseProduct(y).eval();
    auto z_sq = z.cwiseProduct(z).eval();

    /*
     * fit ellipsoid in the form Ax^2 + By^2 + Cz^2 + 2Dxy + 2Exz + 2Fyz + 2Gx +
     * 2Hy + 2Iz + J = 0 and A + B + C = 3 constraint removing one extra
     * parameter
     */
    Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> D;
    switch (type) {
    case EllipsoidType::Arbitrary:
        D.resize(data.rows(), 9);
        D.col(0) = x_sq + y_sq - 2. * z_sq;
        D.col(1) = x_sq + z_sq - 2. * y_sq;
        D.col(2) = 2. * x.cwiseProduct(y);
        D.col(3) = 2. * x.cwiseProduct(z);
        D.col(4) = 2. * y.cwiseProduct(z);
        D.col(5) = 2. * x;
        D.col(6) = 2. * y;
        D.col(7) = 2. * z;
        D.col(8).setOnes();
        break;
    case EllipsoidType::XYEqual:
        D.resize(data.rows(), 8);
        D.col(0) = x_sq + y_sq - 2. * z_sq;
        D.col(1) = 2. * x.cwiseProduct(y);
        D.col(2) = 2. * x.cwiseProduct(z);
        D.col(3) = 2. * y.cwiseProduct(z);
        D.col(4) = 2. * x;
        D.col(5) = 2. * y;
        D.col(6) = 2. * z;
        D.col(7).setOnes();
        break;
    case EllipsoidType::XZEqual:
        D.resize(data.rows(), 8);
        D.col(0) = x_sq + z_sq - 2. * y_sq;
        D.col(1) = 2. * x.cwiseProduct(y);
        D.col(2) = 2. * x.cwiseProduct(z);
        D.col(3) = 2. * y.cwiseProduct(z);
        D.col(4) = 2. * x;
        D.col(5) = 2. * y;
        D.col(6) = 2. * z;
        D.col(7).setOnes();
        break;
    case EllipsoidType::Sphere:
        D.resize(data.rows(), 4);
        D.col(0) = 2. * x;
        D.col(1) = 2. * y;
        D.col(2) = 2. * z;
        D.col(3).setOnes();
        break;
    case EllipsoidType::Aligned:
        D.resize(data.rows(), 6);
        D.col(0) = x_sq + y_sq - 2. * z_sq;
        D.col(1) = x_sq + z_sq - 2. * y_sq;
        D.col(2) = 2. * x;
        D.col(3) = 2. * y;
        D.col(4) = 2. * z;
        D.col(5).setOnes();
        break;
    case EllipsoidType::AlignedXYEqual:
        D.resize(data.rows(), 5);
        D.col(0) = x_sq + y_sq - 2. * z_sq;
        D.col(1) = 2. * x;
        D.col(2) = 2. * y;
        D.col(3) = 2. * z;
        D.col(4).setOnes();
        break;
    case EllipsoidType::AlignedXZEqual:
        D.resize(data.rows(), 5);
        D.col(0) = x_sq + z_sq - 2. * y_sq;
        D.col(1) = 2. * x;
        D.col(2) = 2. * y;
        D.col(3) = 2. * z;
        D.col(4).setOnes();
        break;
    }

    // solve the normal system of equations
    auto d2 = (x_sq + y_sq + z_sq).eval(); // the RHS of the llsq problem (y's)
    auto u = (D.transpose() * D)
                 .bdcSvd(Eigen::ComputeFullU | Eigen::ComputeFullV)
                 .solve(D.transpose() * d2)
                 .eval(); // solution to the normal equations

    /*
     * find the ellipsoid parameters
     * convert back to the conventional algebraic form
     */
    Eigen::Matrix<double, 10, 1> v;
    switch (type) {
    case EllipsoidType::Arbitrary:
        v(0) = u(0) + u(1) - 1.;
        v(1) = u(0) - 2. * u(1) - 1.;
        v(2) = u(1) - 2. * u(0) - 1.;
        v.segment<7>(3) = u.segment<7>(2);
        break;
    case EllipsoidType::XYEqual:
        v(0) = u(0) - 1.;
        v(1) = u(0) - 1.;
        v(2) = -2. * u(0) - 1.;
        v.segment<7>(3) = u.segment<7>(1);
        break;
    case EllipsoidType::XZEqual:
        v(0) = u(0) - 1.;
        v(1) = -2. * u(0) - 1.;
        v(2) = u(0) - 1.;
        v.segment<7>(3) = u.segment<7>(1);
        break;
    case EllipsoidType::Sphere:
        v.segment<3>(0).setConstant(-1.);
        v.segment<3>(3).setZero();
        v.segment<4>(6) = u.segment<4>(0);
        break;
    case EllipsoidType::Aligned:
        v(0) = u(0) + u(1) - 1.;
        v(1) = u(0) - 2. * u(1) - 1.;
        v(2) = u(1) - 2. * u(0) - 1.;
        v.segment<3>(3).setZero();
        v.segment<4>(6) = u.segment<4>(2);
        break;
    case EllipsoidType::AlignedXYEqual:
        v(0) = u(0) - 1.;
        v(1) = u(0) - 1.;
        v(2) = -2. * u(0) - 1.;
        v.segment<3>(3).setZero();
        v.segment<4>(6) = u.segment<4>(1);
        break;
    case EllipsoidType::AlignedXZEqual:
        v(0) = u(0) - 1.;
        v(1) = -2. * u(0) - 1.;
        v(2) = u(0) - 1.;
        v.segment<3>(3).setZero();
        v.segment<4>(6) = u.segment<4>(1);
        break;
    }

    // form the algebraic form of the ellipsoid
    Eigen::Matrix4d A;
    A << v(0), v(3), v(4), v(6), v(3), v(1), v(5), v(7), v(4), v(5), v(2), v(8),
        v(6), v(7), v(8), v(9);

    // find the center of the ellipsoid
    params.center = -A.block<3, 3>(0, 0)
                         .bdcSvd(Eigen::ComputeFullU | Eigen::ComputeFullV)
                         .solve(v.segment<3>(6));
    // form the corresponding translation matrix
    Eigen::Matrix4d T(Eigen::Matrix4d::Identity());
    T.block<1, 3>(3, 0) = params.center.transpose();
    // translate to the center
    auto R = (T * A * T.transpose()).eval();
    // solve the eigenproblem
    Eigen::EigenSolver<Eigen::Matrix3d> solver(R.block<3, 3>(0, 0) / -R(3, 3));
    Eigen::Vector3d eval = solver.eigenvalues().real();
    Eigen::Matrix3d evec_column = solver.eigenvectors().real();
    // determine the configuration with the minimum angle of rotation from ref. frame
    eigenOrder::leastRotationAngle(eval, evec_column);
    // compute the ellipsoid axes' radius
    params.radii = eval.cwiseInverse().cwiseSqrt(); // output NaN for hyperboloid surface

    // get the coefficients of the algebraic form
    if (coefficients_p != nullptr) {
        *coefficients_p = v;
    }

    // get the arranged eigenvalues
    if (eval_p != nullptr) {
        *eval_p = eval;
    }

    // get the arranged eigenvectors
    if (evec_column_p != nullptr) {
        *evec_column_p = evec_column;
    }

    return params;
}

} // namespace ellipsoid

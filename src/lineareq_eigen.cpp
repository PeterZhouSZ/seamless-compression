#include <Eigen/Sparse>
#include <Eigen/IterativeLinearSolvers>
#include "lineareq.h"

using namespace Eigen;

//#define SOLVER_USE_FACTORIZATION

bool LinearEquationSet::solve(std::vector<scalar> &solution){

    int n = nvar;
    int m = neq;
    VectorXd x(n), b(m);

    SparseMatrix<double> A(m,n);

    // fill A and b
    {
        int i=0;
        std::vector<Eigen::Triplet<double>> tvec;
        for (const auto& eqset : eqgroups) {
            for (const LinearExp& le:eqset.second) {
                for (const auto& t : le.terms)
                    tvec.push_back(Eigen::Triplet<double>(i, t.first, t.second));
                b(i) = -le.b;
                i++;
            }
        }
        A.setFromTriplets(tvec.begin(), tvec.end());
    }

#ifdef SOLVER_USE_FACTORIZATION
    Eigen::SimplicialLDLT<SparseMatrix<double>> ldlt;
    ldlt.compute(A.transpose() * A);
    assert(ldlt.info() == Eigen::Success);
    x = ldlt.solve(A.transpose() * b);
    bool ok = (ldlt.info() == Eigen::Success);
    LeastSquaresConjugateGradient< SparseMatrix<double> > lscg;
#else
    LeastSquaresConjugateGradient< SparseMatrix<double> > lscg;
    lscg.compute(A);
    if (solution.size() != nvar) {
       initializeVars(solution);
    } else {
        for (int i=0; i<n; i++)
            x[i] = solution[i] ;
    }
    lscg.setTolerance(1e-14);
    x = lscg.solveWithGuess(b,x);
    //x = lscg.solve(b);
    std::cout << "#iterations:     " << lscg.iterations() << std::endl;
    std::cout << "estimated error: " << lscg.error()      << std::endl;
    /*// update b, and solve again
    x = lscg.solve(b);*/
    bool ok = (lscg.info() == Eigen::Success);
#endif

    solution.resize(n);
    for (int i=0; i<n; i++)
        solution[i] = x[i];

    return ok;
}

/**********************************************************************

Copyright 2017 The University of Leeds

This file is part of the R humanleague package.

humanleague is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

humanleague is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

A copy of the GNU General Public License can be found in LICENCE.txt
in the project's root directory, or at <http://www.gnu.org/licenses/>.

**********************************************************************/

#include <Rcpp.h>
using namespace Rcpp;

#include "QIWS.h"
#include "CQIWS.h"
#include "RQIWS.h"
#include "GQIWS.h"
#include "IPF.h"
#include "QSIPF.h"
#include "Integerise.h"

#include "UnitTester.h"

#include <vector>

//#include <csignal>

// // Handler for ctrl-C
// extern "C" void sigint_handler(int)
// {
//   // throw back to R
//   throw std::runtime_error("User interrupt");

// }
// Enable ctrl-C to interrupt the code
// TODO this doesnt seem to work, perhaps another approach (like a separate thread?)
//void (*oldhandler)(int) = signal(SIGINT, sigint_handler);

// Flatten N-D population array into N*P table
template<size_t D>
DataFrame flatten(const size_t pop, const NDArray<D,uint32_t>& t)
{
  std::vector<std::vector<int>> list = listify<D>(pop, t);

  // DataFrame interface is poor and appears buggy. Best approach seems to insert columns in List then assign to DataFrame at end
  List proxyDf;
  std::string s("C");
  for (size_t i = 0; i < D; ++i)
  {
    proxyDf[std::string(s + std::to_string(i)).c_str()] = list[i];
  }

  return DataFrame(proxyDf);
}

template<typename S>
void doSolve(List& result, IntegerVector dims, const std::vector<std::vector<uint32_t>>& m)
{
  S solver(m);
  result["method"] = "QIWS";
  result["conv"] = solver.solve();
  result["chiSq"] = solver.chiSq();
  std::pair<double, bool> pVal = solver.pValue();
  result["pValue"] = pVal.first;
  if (!pVal.second)
  {
    result["warning"] = "p-value may be inaccurate";
  }
  result["error.margins"] = std::vector<uint32_t>(solver.residuals(), solver.residuals() + S::Dim);
  const typename S::table_t& t = solver.result();
  const NDArray<S::Dim, double>& p = solver.stateProbabilities();
  Index<S::Dim, Index_Unfixed> idx(t.sizes());
  IntegerVector values(t.storageSize());
  NumericVector probs(t.storageSize());
  while (!idx.end())
  {
    values[idx.colMajorOffset()] = t[idx];
    probs[idx.colMajorOffset()] = p[idx];
    ++idx;
  }
  values.attr("dim") = dims;
  probs.attr("dim") = dims;
  result["p.hat"] = probs;
  result["x.hat"] = values;
  result["pop"] = flatten<S::Dim>(solver.population(), t);
}


void doSolveConstrained(List& result, IntegerVector dims, const std::vector<std::vector<uint32_t>>& m, const NDArray<2, bool>& permitted)
{
  CQIWS solver(m, permitted);
  result["method"] = "QIWS-C";
  result["conv"] = solver.solve();
  result["chiSq"] = solver.chiSq();
  std::pair<double, bool> pVal = solver.pValue();
  result["pValue"] = pVal.first;
  if (!pVal.second)
  {
    result["warning"] = "p-value may be inaccurate";
  }
  result["error.margins"] = std::vector<uint32_t>(solver.residuals(), solver.residuals() + 2);
  const typename QIWS<2>::table_t& t = solver.result();

  const NDArray<2, double>& p = solver.stateProbabilities();
  Index<2, Index_Unfixed> idx(t.sizes());
  IntegerVector values(t.storageSize());
  NumericVector probs(t.storageSize());
  while (!idx.end())
  {
    values[idx.colMajorOffset()] = t[idx];
    probs[idx.colMajorOffset()] = p[idx];
    ++idx;
  }
  values.attr("dim") = dims;
  probs.attr("dim") = dims;
  result["p.hat"] = probs;
  result["x.hat"] = values;
  result["pop"] = flatten<2>(solver.population(), t);
}

void doSolveGeneral(List& result, IntegerVector dims, const std::vector<std::vector<uint32_t>>& m, const NDArray<2, double>& exoProbs)
{
  GQIWS solver(m, exoProbs);
  result["method"] = "QIWS-G";
  result["conv"] = solver.solve();
  //result["chiSq"] = solver.chiSq();
  //std::pair<double, bool> pVal = solver.pValue();
  //result["pValue"] = pVal.first;
  // if (!pVal.second)
  // {
  //   result["warning"] = "p-value may be inaccurate";
  // }
  //result["error.margins"] = std::vector<uint32_t>(solver.residuals(), solver.residuals() + 2);
  const typename QIWS<2>::table_t& t = solver.result();
  //
  // const NDArray<2, double>& p = solver.stateProbabilities();
  Index<2, Index_Unfixed> idx(t.sizes());
  IntegerVector values(t.storageSize());
  // NumericVector probs(t.storageSize());
  while (!idx.end())
  {
    values[idx.colMajorOffset()] = t[idx];
  //   probs[idx.colMajorOffset()] = p[idx];
    ++idx;
  }
  values.attr("dim") = dims;
  // probs.attr("dim") = dims;
  // result["p.hat"] = probs;
  result["x.hat"] = values;
  result["pop"] = flatten<2>(solver.population(), t);
}

void doSolveCorrelated(List& result, IntegerVector dims, const std::vector<std::vector<uint32_t>>& m, double rho)
{
  RQIWS solver(m, rho);
  result["method"] = "QIWS-R";
  result["conv"] = solver.solve();
  result["chiSq"] = solver.chiSq();
  std::pair<double, bool> pVal = solver.pValue();
  result["pValue"] = pVal.first;
  if (!pVal.second)
  {
    result["warning"] = "p-value may be inaccurate";
  }
  result["error.margins"] = std::vector<uint32_t>(solver.residuals(), solver.residuals() + 2);
  const typename QIWS<2>::table_t& t = solver.result();

  const NDArray<2, double>& p = solver.stateProbabilities();
  Index<2, Index_Unfixed> idx(t.sizes());
  IntegerVector values(t.storageSize());
  NumericVector probs(t.storageSize());
  while (!idx.end())
  {
    values[idx.colMajorOffset()] = t[idx];
    probs[idx.colMajorOffset()] = p[idx];
    ++idx;
  }
  values.attr("dim") = dims;
  probs.attr("dim") = dims;
  result["p.hat"] = probs;
  result["x.hat"] = values;
  result["pop"] = flatten<2>(solver.population(), t);
}


void doConstrain(List& result, NDArray<2, uint32_t>& population, const NDArray<2, bool>& permitted)
{
  Constrain::Status status = CQIWS::constrain(population, permitted, population.storageSize());

  switch(status)
  {
  case Constrain::SUCCESS:
    result["conv"] = true;
    break;
  case Constrain::ITERLIMIT:
    result["conv"] = false;
    result["error"] = "iteration limit reached";
    break;
  case Constrain::STUCK:
    result["conv"] = false;
    result["error"] = "cannot adjust population: check validity of constraint";
    break;
  default:
    result["conv"] = false;
    result["error"] = "constrain algorithm status invalid. please report this bug";
  }

  IntegerVector dims;
  dims.push_back(population.sizes()[0]);
  dims.push_back(population.sizes()[1]);
  Index<2, Index_Unfixed> idx(population.sizes());
  IntegerVector values(population.storageSize());
  size_t pop = 0;
  while (!idx.end())
  {
    values[idx.colMajorOffset()] = population[idx];
    pop += population[idx];
    ++idx;
  }
  values.attr("dim") = dims;
  result["x.hat"] = values;
  result["pop"] = flatten<2>(pop, population);
}



//' Generate a population in n dimensions given n marginals.
//'
//' Using Quasirandom Integer Without-replacement Sampling (QIWS), this function
//' generates an n-dimensional population table where elements sum to the input marginals, and supplemental data.
//' @param marginals a List of n integer vectors containing marginal data (2 <= n <= 12). The sum of elements in each vector must be identical
//' @return an object containing: the population matrix, the occupancy probability matrix, a convergence flag, the chi-squared statistic, p-value, and error value (nonzero if not converged)
//' @examples
//' synthPop(list(c(1,2,3,4), c(3,4,3)))
//' @export
// [[Rcpp::export]]
List synthPop(List marginals)
{
  const size_t dim = marginals.size();
  std::vector<std::vector<uint32_t>> m(dim);
  IntegerVector dims;
  // TODO verbose flag? Rcout << "Dimension: " << dim << "\nMarginals:" << std::endl;
  for (size_t i = 0; i < dim; ++i)
  {
    const IntegerVector& iv = marginals[i];
    m[i].reserve(iv.size());
    std::copy(iv.begin(), iv.end(), std::back_inserter(m[i]));
    //Rcout << "[" << std::accumulate(m[i].begin(), m[i].end(), 0) << "] ";
    //print(m[i].data(), m[i].size(), m[i].size(), Rcout);
    dims.push_back(iv.size());
  }
  List result;

  // Workaround for fact that dimensionality is a template param and thus fixed at compile time
  switch(dim)
  {
  case 2:
    doSolve<QIWS<2>>(result, dims, m);
    break;
  case 3:
    doSolve<QIWS<3>>(result, dims, m);
    break;
  case 4:
    doSolve<QIWS<4>>(result, dims, m);
    break;
  case 5:
    doSolve<QIWS<5>>(result, dims, m);
    break;
  case 6:
    doSolve<QIWS<6>>(result, dims, m);
    break;
  case 7:
    doSolve<QIWS<7>>(result, dims, m);
    break;
  case 8:
    doSolve<QIWS<8>>(result, dims, m);
    break;
  case 9:
    doSolve<QIWS<9>>(result, dims, m);
    break;
  case 10:
    doSolve<QIWS<10>>(result, dims, m);
    break;
  case 11:
    doSolve<QIWS<11>>(result, dims, m);
    break;
  case 12:
    doSolve<QIWS<12>>(result, dims, m);
    break;
  default:
    throw std::runtime_error("invalid dimensionality: " + std::to_string(dim));
  }

  return result;
}

//' Generate a constrained population in 2 dimensions given 2 marginals and a constraint matrix.
//'
//' Using Quasirandom Integer Without-replacement Sampling (QIWS), this function
//' generates a 2-dimensional population table where elements sum to the input marginals.
//' It then uses an iterative algorithm to reassign the population to only the permitted states.
//' @param marginals a List of 2 integer vectors containing marginal data. The sum of elements in each vector must be identical
//' @param permittedStates a matrix of booleans containing allowed states. The matrix dimensions must be the length of each marginal
//' @return an object containing: the population matrix, the occupancy probability matrix, a convergence flag, the chi-squared statistic, p-value, and error value (nonzero if not converged)
//' @examples
//' r = c(0, 3, 17, 124, 167, 79, 46, 22)
//' # rooms (1,2,3...9+)
//' b = c(0, 15, 165, 238, 33, 7) # bedrooms {0, 1,2...5+}
//' p = matrix(rep(TRUE,length(r)*length(b)), nrow=length(r)) # all states permitted
//' # now disallow bedrooms>rooms
//'   for (i in 1:length(r)) {
//'     for (j in 1:length(b)) {
//'       if (j > i + 1)
//'         p[i,j] = FALSE;
//'     }
//'   }
//' res = humanleague::synthPopC(list(r,b),p)
//' @export
// [[Rcpp::export]]
List synthPopC(List marginals, LogicalMatrix permittedStates)
{
  if (marginals.size() != 2)
    throw std::runtime_error("CQIWS invalid dimensionality: " + std::to_string(marginals.size()));

  std::vector<std::vector<uint32_t>> m(2);

  const IntegerVector& iv0 = marginals[0];
  const IntegerVector& iv1 = marginals[1];
  IntegerVector dims(2);
  dims[0] = iv0.size();
  dims[1] = iv1.size();
  m[0].reserve(dims[0]);
  m[1].reserve(dims[1]);
  std::copy(iv0.begin(), iv0.end(), std::back_inserter(m[0]));
  std::copy(iv1.begin(), iv1.end(), std::back_inserter(m[1]));

  if (permittedStates.rows() != dims[0] || permittedStates.cols() != dims[1])
    throw std::runtime_error("CQIWS invalid permittedStates matrix size");

  size_t d[2] = { (size_t)dims[0], (size_t)dims[1] };
  NDArray<2,bool> permitted(d);

  for (d[0] = 0; d[0] < dims[0]; ++d[0])
  {
    for (d[1] = 0; d[1] < dims[1]; ++d[1])
    {
      permitted[d] = permittedStates(d[0],d[1]);
    }
  }

  List result;
  doSolveConstrained(result, dims, m, permitted);

  return result;
}


// [[Rcpp::export]]
List synthPopG(List marginals, NumericMatrix exoProbsIn)
{
  if (marginals.size() != 2)
    throw std::runtime_error("CQIWS invalid dimensionality: " + std::to_string(marginals.size()));

  std::vector<std::vector<uint32_t>> m(2);

  const IntegerVector& iv0 = marginals[0];
  const IntegerVector& iv1 = marginals[1];
  IntegerVector dims(2);
  dims[0] = iv0.size();
  dims[1] = iv1.size();
  m[0].reserve(dims[0]);
  m[1].reserve(dims[1]);
  std::copy(iv0.begin(), iv0.end(), std::back_inserter(m[0]));
  std::copy(iv1.begin(), iv1.end(), std::back_inserter(m[1]));

  if (exoProbsIn.rows() != dims[0] || exoProbsIn.cols() != dims[1])
    throw std::runtime_error("CQIWS invalid permittedStates matrix size");

  size_t d[2] = { (size_t)dims[0], (size_t)dims[1] };
  NDArray<2,double> exoProbs(d);

  for (d[0] = 0; d[0] < dims[0]; ++d[0])
  {
    for (d[1] = 0; d[1] < dims[1]; ++d[1])
    {
      exoProbs[d] = exoProbsIn(d[0],d[1]);
    }
  }

  List result;
  doSolveGeneral(result, dims, m, exoProbs);

  return result;
}


//' Generate a correlated population in 2 dimensions given 2 marginals and a flat correlation.
//'
//' Using Quasirandom Integer Without-replacement Sampling (QIWS), this function
//' generates a 2-dimensional population table where elements sum to the input marginals.
//' @param marginals a List of 2 integer vectors containing marginal data. The sum of elements in each vector must be identical
//' @param rho correlation
//' @return an object containing: the population matrix, the occupancy probability matrix, a convergence flag, the chi-squared statistic, p-value, and error value (nonzero if not converged)
//' @export
// [[Rcpp::export]]
List synthPopR(List marginals, double rho)
{
  if (marginals.size() != 2)
    throw std::runtime_error("CQIWS invalid dimensionality: " + std::to_string(marginals.size()));

  std::vector<std::vector<uint32_t>> m(2);

  const IntegerVector& iv0 = marginals[0];
  const IntegerVector& iv1 = marginals[1];
  IntegerVector dims(2);
  dims[0] = iv0.size();
  dims[1] = iv1.size();
  m[0].reserve(dims[0]);
  m[1].reserve(dims[1]);
  std::copy(iv0.begin(), iv0.end(), std::back_inserter(m[0]));
  std::copy(iv1.begin(), iv1.end(), std::back_inserter(m[1]));

  size_t d[2] = { (size_t)dims[0], (size_t)dims[1] };

  List result;
  doSolveCorrelated(result, dims, m, rho);

  return result;
}

template<size_t D>
void doIPF(const std::vector<long>& s, NumericVector seed, NumericVector r, const std::vector<std::vector<double>>& m,
           List& result)
{
  // Read-only shallow copy of seed
  const NDArray<D, double> seedwrapper(const_cast<long*>(&s[0]), (double*)&seed[0]);
  // Do IPF
  IPF<D> ipf(seedwrapper, m);
  // Copy result data into R array
  const NDArray<D, double>& tmp = ipf.result();
  std::copy(tmp.rawData(), tmp.rawData() + tmp.storageSize(), r.begin());
  result["conv"] = ipf.conv();
  result["result"] = r;
  result["pop"] = ipf.population();
  result["iterations"] = ipf.iters();
  result["errors"] = ipf.errors();
  result["maxError"] = ipf.maxError();
}

template<size_t D>
void doQSIPF(const std::vector<long>& s, NumericVector seed, IntegerVector r, const std::vector<std::vector<long>>& m,
           List& result)
{
  // Read-only shallow copy of seed
  const NDArray<D, double> seedwrapper(const_cast<long*>(&s[0]), (double*)&seed[0]);
  // Do IPF
  QSIPF<D> qsipf(seedwrapper, m);
  // Copy result data into R array
  const NDArray<D, long>& tmp = qsipf.sample();
  std::copy(tmp.rawData(), tmp.rawData() + tmp.storageSize(), r.begin());
  result["conv"] = qsipf.conv();
  result["result"] = r;
  result["pop"] = qsipf.population();
  result["iterations"] = qsipf.iters();
  result["errors"] = qsipf.errors();
  result["maxError"] = qsipf.maxError();
}


//' IPF
//'
//' C++ IPF implementation
//' @param seed an n-dimensional array of seed values
//' @param marginals a List of n integer vectors containing marginal data. The sum of elements in each vector must be identical
//' @return an object containing: ...
//' @export
// [[Rcpp::export]]
List ipf(NumericVector seed, List marginals)
{
  const size_t dim = marginals.size();

  IntegerVector sizes = seed.attr("dim");
  std::vector<std::vector<double>> m(dim);
  std::vector<long> s(dim);

  if (sizes.size() != dim)
    throw std::runtime_error("no. of marginals not equal to seed dimension");

  // insert marginals in reverse order
  for (size_t i = 0; i < dim; ++i)
  {
    const NumericVector& iv = marginals[i];
    if (iv.size() != sizes[i])
      throw std::runtime_error("seed-marginal size mismatch");
    s[dim-i-1] = sizes[i];
    m[dim-i-1].reserve(iv.size());
    std::copy(iv.begin(), iv.end(), std::back_inserter(m[dim-i-1]));
  }

  //print(s);

  // Deep copy of seed for result (preserves diimesion, values will be overwritten)
  NumericVector r(seed);

  List result;
  // Workaround for fact that dimensionality is a template param and thus fixed at compile time
  switch(dim)
  {
  case 2:
    doIPF<2>(s, seed, r, m, result);
    break;
  case 3:
    doIPF<3>(s, seed, r, m, result);
    break;
  case 4:
    doIPF<4>(s, seed, r, m, result);
    break;
  case 5:
    doIPF<5>(s, seed, r, m, result);
    break;
  case 6:
    doIPF<6>(s, seed, r, m, result);
    break;
  case 7:
    doIPF<7>(s, seed, r, m, result);
    break;
  case 8:
    doIPF<8>(s, seed, r, m, result);
    break;
  case 9:
    doIPF<9>(s, seed, r, m, result);
    break;
  case 10:
    doIPF<10>(s, seed, r, m, result);
    break;
  case 11:
    doIPF<11>(s, seed, r, m, result);
    break;
  case 12:
    doIPF<12>(s, seed, r, m, result);
    break;
  default:
    throw std::runtime_error("IPF only works for 2D - 12D problems");
  }

  return result;
}

//' IPF
//'
//' C++ IPF implementation
//' @param seed an n-dimensional array of seed values
//' @param marginals a List of n integer vectors containing marginal data. The sum of elements in each vector must be identical
//' @return an object containing: ...
//' @export
// [[Rcpp::export]]
List qsipf(NumericVector seed, List marginals)
{
  const size_t dim = marginals.size();

  IntegerVector sizes = seed.attr("dim");
  std::vector<std::vector<long>> m(dim);
  std::vector<long> s(dim);

  if (sizes.size() != dim)
    throw std::runtime_error("no. of marginals not equal to seed dimension");

  // insert marginals in reverse order
  for (size_t i = 0; i < dim; ++i)
  {
    const IntegerVector& iv = marginals[i];
    if (iv.size() != sizes[i])
      throw std::runtime_error("seed-marginal size mismatch");
    s[dim-i-1] = sizes[i];
    m[dim-i-1].reserve(iv.size());
    std::copy(iv.begin(), iv.end(), std::back_inserter(m[dim-i-1]));
  }

  //print(s);

  // Deep copy of seed for result (preserves diimesion, values will be overwritten)
  IntegerVector r(seed);

  List result;
  // Workaround for fact that dimensionality is a template param and thus fixed at compile time
  switch(dim)
  {
  case 2:
    doQSIPF<2>(s, seed, r, m, result);
    break;
  case 3:
    doQSIPF<3>(s, seed, r, m, result);
    break;
  case 4:
    doQSIPF<4>(s, seed, r, m, result);
    break;
  case 5:
    doQSIPF<5>(s, seed, r, m, result);
    break;
  case 6:
    doQSIPF<6>(s, seed, r, m, result);
    break;
  case 7:
    doQSIPF<7>(s, seed, r, m, result);
    break;
  case 8:
    doQSIPF<8>(s, seed, r, m, result);
    break;
  case 9:
    doQSIPF<9>(s, seed, r, m, result);
    break;
  case 10:
    doQSIPF<10>(s, seed, r, m, result);
    break;
  case 11:
    doQSIPF<11>(s, seed, r, m, result);
    break;
  case 12:
    doQSIPF<12>(s, seed, r, m, result);
    break;
  default:
    throw std::runtime_error("QSIPF only works for 2D - 12D problems");
  }

  return result;
}

// // a = array(rep(1,16), c(2,2,2,2))
// // b = test(a)
// // [[Rcpp::export]]
// List test(NumericVector ndarray)
// {
//   IntegerVector sizes = ndarray.attr("dim");
//   NumericVector copy(ndarray.begin(), ndarray.end());
//   copy.attr("dim") = sizes;
//
//   std::vector<long> s(4);
//   for (size_t i = 0; i < 4; ++i)
//   {
//     s[i] = sizes[i];
//   }
//
//   //print(s);
//
//   // Shallow copy of "copy"
//   NDArray<4, double> wrapper(&s[0], (double*)&copy[0]);
//   double x = 0.0;
//   for (Index<4, Index_Unfixed> i(wrapper.sizes()); !i.end(); ++i)
//   {
//     wrapper[i] = x;
//     x += 0.1;
//   }
//
//   //std::vector<double> cpparray(ndarray.begin(), ndarray.end());
//   List result;
//   result["dim"] =   s.size();
//   result["sizes"] =   s;
//   // TODO set dim attr...
//   result["ndarray"] = copy;
//
//   return result;
// }

//' Constrained a pregenerated population in 2 dimensions given a constraint matrix.
//'
//' Using an iterative algorithm, this function
//' adjusts a 2-dimensional population table, reassigning populations in disallowed states to allowed ones, preserving the two marginal distributions
//' states where elements sum to the input marginals.
//' Users need to ensure that the supplied constraint matrix permits a valid population to be computed - this is not always obvious from the input data.
//' @param population an integer matrix containing the population.
//' @param permittedStates a matrix of booleans containing allowed states. The matrix dimensions must be the length of each marginal
//' @return an object containing: the adjusted population matrix and a convergence flag.
//' @examples
//' r = c(0, 3, 17, 124, 167, 79, 46, 22)
//' # rooms (1,2,3...9+)
//' b = c(0, 15, 165, 238, 33, 7) # bedrooms {0, 1,2...5+}
//' p = matrix(rep(TRUE,length(r)*length(b)), nrow=length(r)) # all states permitted
//' # now disallow bedrooms>rooms
//' for (i in 1:length(r)) {
//'   for (j in 1:length(b)) {
//'     if (j > i + 1)
//'       p[i,j] = FALSE;
//'   }
//' }
//' res = humanleague::synthPop(list(r,b)) # unconstrained synthesis
//' res = humanleague::constrain(res$x.hat, p)
//' @export
// [[Rcpp::export]]
List constrain(IntegerMatrix population, LogicalMatrix permittedStates)
{
  size_t d[2] = { (size_t)population.rows(), (size_t)population.cols() };

  if (permittedStates.rows() != d[0] || permittedStates.cols() != d[1])
    throw std::runtime_error("CQIWS population / permittedStates matrix size mismatch");

  NDArray<2,bool> permitted(d);
  NDArray<2,uint32_t> pop(d);

  size_t idx[2];

  for (idx[0] = 0; idx[0] < d[0]; ++idx[0])
  {
    for (idx[1] = 0; idx[1] < d[1]; ++idx[1])
    {
      pop[idx] = population(idx[0],idx[1]);
      permitted[idx] = permittedStates(idx[0],idx[1]);
    }
  }
  //
  List result;
  doConstrain(result, pop, permitted);

  return result;
}


//' Generate integer frequencies from discrete probabilities and an overall population.
//'
//' This function will generate the closest integer vector to the probabilities scaled to the population.
//' @param pIn a numeric vector of state occupation probabilities. Must sum to unity (to within double precision epsilon)
//' @param pop the total population
//' @return an integer vector of frequencies that sums to pop.
//' @examples
//' prob2IntFreq(c(0.1,0.2,0.3,0.4), 11)
//' @export
// [[Rcpp::export]]
List prob2IntFreq(NumericVector pIn, int pop)
{
  double var;
  const std::vector<double>& p = as<std::vector<double>>(pIn);

  if (pop < 1)
  {
    throw std::runtime_error("population must be strictly positive");
  }

  if (fabs(std::accumulate(p.begin(), p.end(), -1.0)) > 1000*std::numeric_limits<double>::epsilon())
  {
    throw std::runtime_error("probabilities do not sum to unity");
  }
  std::vector<int> f = integeriseMarginalDistribution(p, pop, var);

  List result;
  result["freq"] = f;
  result["var"] = var;

  return result;
}

//' Generate Sobol' quasirandom sequence
//'
//' @param dim dimensions
//' @param n number of variates to sample
//' @param skip number of variates to skip (actual number skipped will be largest power of 2 less than k)
//' @return a n-by-d matrix of uniform probabilities in (0,1).
//' @examples
//' sobolSequence(2, 1000, 1000) # will skip 512 numbers!
//' @export
// [[Rcpp::export]]
NumericMatrix sobolSequence(int dim, int n, int skip = 0)
{
  static const double scale = 0.5 / (1ull<<31);

  NumericMatrix m(n, dim);

  Sobol s(dim, skip);

  for (int j = 0; j <n ; ++j)
    for (int i = 0; i < dim; ++i)
      m(j,i) = s() * scale;

  return m;
}


//' Generate correlated 2D Sobol' quasirandom sequence
//'
//' @param rho correlation
//' @param n number of variates to sample
//' @param skip number of variates to skip (actual number skipped will be largest power of 2 less than k)
//' @return a n-by-2 matrix of uniform correlated probabilities in (0,1).
//' @examples
//' correlatedSobol2Sequence(0.2, 1000)
//' @export
// [[Rcpp::export]]
NumericMatrix correlatedSobol2Sequence(double rho, int n, int skip = 0)
{
  static const double scale = 0.5 / (1ull<<31);

  NumericMatrix m(n, 2);

  Sobol s(2, skip);

  Cholesky cholesky(rho);
  for (int j = 0; j <n ; ++j)
  {
    const std::pair<uint32_t, uint32_t>& buf = cholesky(s.buf());
    m(j,0) = buf.first * scale;
    m(j,1) = buf.second * scale;
  }

  return m;
}


//' Entry point to enable running unit tests within R (e.g. in testthat)
//'
//' @return a List containing, number of tests run, number of failures, and any error messages.
//' @examples
//' unitTest()
//' @export
// [[Rcpp::export]]
List unitTest()
{
  const unittest::Logger& log = unittest::run();

  List result;
  result["nTests"] = log.testsRun;
  result["nFails"] = log.testsFailed;
  result["errors"] = log.errors;

  return result;
}

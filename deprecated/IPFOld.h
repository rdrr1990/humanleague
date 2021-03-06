#pragma once

#include "NDArrayOld.h"
#include "NDArrayUtilsOld.h"

#include "NDArrayUtils.h"

#include <algorithm>
#include <vector>
//#include <array> sadly cant use this (easily) as length is defined in type

#include <cmath>

namespace old {

template<size_t D>
class IPF
{
public:

  static const size_t Dim = D;

  // construct from factional marginals
  IPF(const old::NDArray<D, double>& seed, const std::vector<std::vector<double>>& marginals)
  : m_result(seed.sizes()), m_marginals(marginals), m_errors(D), m_conv(false)
  {
    if (m_marginals.size() != Dim)
      throw std::runtime_error("no. of marginals doesnt match dimensionalty");
    solve(seed);
  }

  // construct from integer marginals
  IPF(const old::NDArray<D, double>& seed, const std::vector<std::vector<int64_t>>& marginals)
  : m_result(seed.sizes()), m_marginals(D), m_errors(D), m_conv(false)
  {
    if (marginals.size() != Dim)
      throw std::runtime_error("no. of marginals doesnt match dimensionalty");
    for (size_t d = 0; d < Dim; ++d)
    {
      m_marginals[d].reserve(marginals[d].size());
      std::copy(marginals[d].begin(), marginals[d].end(), std::back_inserter(m_marginals[d]));
    }
    solve(seed);
  }

  IPF(const IPF&) = delete;
  IPF(IPF&&) = delete;

  IPF& operator=(const IPF&) = delete;
  IPF& operator=(IPF&&) = delete;

  virtual ~IPF() { }

  void solve(const old::NDArray<D, double>& seed)
  {
    // reset convergence flag
    m_conv = false;
    m_population = std::accumulate(m_marginals[0].begin(), m_marginals[0].end(), 0.0);
    // checks on marginals, dimensions etc
    for (size_t i = 0; i < Dim; ++i)
    {
      if (m_marginals[i].size() != m_result.sizes()[i])
        throw std::runtime_error("marginal doesnt have correct length");

      double mpop = std::accumulate(m_marginals[i].begin(), m_marginals[i].end(), 0.0);
      if (mpop != m_population)
        throw std::runtime_error("marginal doesnt have correct population");
    }

    //print(seed.rawData(), seed.storageSize(), m_marginals[1].size());
    std::copy(seed.rawData(), seed.rawData() + seed.storageSize(), const_cast<double*>(m_result.rawData()));
    for (size_t d = 0; d < Dim; ++d)
    {
      m_errors[d].resize(m_marginals[d].size());
      //print(m_marginals[d]);
    }
    //print(m_result.rawData(), m_result.storageSize(), m_marginals[1].size());

    for (m_iters = 0; !m_conv && m_iters < s_MAXITER; ++m_iters)
    {
      rScale<Dim>(m_result, m_marginals);
      // inefficient copying?
      std::vector<std::vector<double>> diffs(Dim);

      rDiff<Dim>(diffs, m_result, m_marginals);

      m_conv = computeErrors(diffs);
    }
  }

  virtual size_t population() const
  {
    return m_population;
  }

  const old::NDArray<Dim, double>& result() const
  {
    return m_result;
  }

  const std::vector<std::vector<double>> errors() const
  {
    return m_errors;
  }

  double maxError() const
  {
    return m_maxError;
  }

  virtual bool conv() const
  {
    return m_conv;
  }

  virtual size_t iters() const
  {
    return m_iters;
  }

protected:

  template<size_t I>
  static void rScale(old::NDArray<D, double>& result, const std::vector<std::vector<double>>& marginals)
  {
    const size_t Direction = I - 1;
    const std::vector<double>& r = old::reduce<D, double, Direction>(result);
    for (size_t p = 0; p < marginals[Direction].size(); ++p)
    {
      for (old::Index<Dim, Direction> index(result.sizes(), p); !index.end(); ++index)
      {
        // avoid division by zero (assume 0/0 -> 0)
        if (r[p] == 0.0 && marginals[Direction][index[Direction]] != 0.0)
          throw std::runtime_error("div0 in rScale with m>0");
        if (r[p] != 0.0)
          result[index] *= marginals[Direction][index[Direction]] / r[p];
        else
          result[index] = 0.0;
      }
    }
    rScale<I-1>(result, marginals);
  }

  template<size_t I>
  static void rDiff(std::vector<std::vector<double>>& diffs, const old::NDArray<D, double>& result, const std::vector<std::vector<double>>& marginals)
  {
    const size_t Direction = I - 1;
    diffs[Direction] = diff(old::reduce<Dim, double, Direction>(result), marginals[Direction]);
    rDiff<I-1>(diffs, result, marginals);
  }

  // this is close to repeating the above
  bool computeErrors(std::vector<std::vector<double>>& diffs)
  {
    //calcResiduals<Dim>(diffs);
    m_maxError = -std::numeric_limits<double>::max();
    for (size_t d = 0; d < Dim; ++d)
    {
      for (size_t i = 0; i < diffs[d].size(); ++i)
      {
        double e = std::fabs(diffs[d][i]);
        m_errors[d][i] = e;
        m_maxError = std::max(m_maxError, e);
      }
    }
    return m_maxError < m_tol;
  }

  static const size_t s_MAXITER = 10;

  old::NDArray<Dim, double> m_result;
  std::vector<std::vector<double>> m_marginals;
  std::vector<std::vector<double>> m_errors;
  size_t m_population;
  size_t m_iters;
  bool m_conv;
  const double m_tol = 1e-8;
  double m_maxError;
};


// Specialisation to terminate the recursion for each instantiation
# define SPECIALISE_RSCALE(d) \
template<> \
template<> \
void IPF<d>::rScale<0>(old::NDArray<d, double>&, const std::vector<std::vector<double>>&) { }

SPECIALISE_RSCALE(2)
SPECIALISE_RSCALE(3)
SPECIALISE_RSCALE(4)
SPECIALISE_RSCALE(5)
SPECIALISE_RSCALE(6)
SPECIALISE_RSCALE(7)
SPECIALISE_RSCALE(8)
SPECIALISE_RSCALE(9)
SPECIALISE_RSCALE(10)
SPECIALISE_RSCALE(11)
SPECIALISE_RSCALE(12)

// Specialisation to terminate the diff for each instantiation
# define SPECIALISE_RDIFF(d) \
template<> \
template<> \
void IPF<d>::rDiff<0>(std::vector<std::vector<double>>&, const old::NDArray<d, double>&, const std::vector<std::vector<double>>&) { }

SPECIALISE_RDIFF(2)
SPECIALISE_RDIFF(3)
SPECIALISE_RDIFF(4)
SPECIALISE_RDIFF(5)
SPECIALISE_RDIFF(6)
SPECIALISE_RDIFF(7)
SPECIALISE_RDIFF(8)
SPECIALISE_RDIFF(9)
SPECIALISE_RDIFF(10)
SPECIALISE_RDIFF(11)
SPECIALISE_RDIFF(12)

// disable trivial and nonsensical dimensionalities
template<> class IPF<0>;
template<> class IPF<1>;

// remove macro pollution
#undef SPECIALISE_RSCALE
#undef SPECIALISE_RDIFF

}


#include "IPF.h"
#include "NDArrayUtils.h"

#include <algorithm>

#include <cmath>

namespace deprecated {

class IPF
{
public:

  // construct from fractional marginals
  IPF(const NDArray<double>& seed, const std::vector<std::vector<double>>& marginals);

  // construct from integer marginals
  IPF(const NDArray<double>& seed, const std::vector<std::vector<int64_t>>& marginals);

  IPF(const IPF&) = delete;
  IPF(IPF&&) = delete;

  IPF& operator=(const IPF&) = delete;
  IPF& operator=(IPF&&) = delete;

  virtual ~IPF() { }

  void solve(const NDArray<double>& seed);

  virtual size_t population() const;

  const NDArray<double>& result() const;

  const std::vector<std::vector<double>> errors() const;

  double maxError() const;

  virtual bool conv() const;

  virtual size_t iters() const;

protected:

  bool computeErrors(std::vector<std::vector<double>>& diffs);

  static const size_t s_MAXITER = 1000;

  NDArray<double> m_result;
  std::vector<std::vector<double>> m_marginals;
  std::vector<std::vector<double>> m_errors;
  size_t m_population;
  size_t m_iters;
  bool m_conv;
  const double m_tol = 1e-8;
  double m_maxError;
};


}

namespace {

void rScale(NDArray<double>& result, const std::vector<std::vector<double>>& marginals)
{
  for (size_t d = 0; d < result.dim(); ++d)
  {
    const std::vector<double>& r = reduce<double>(result, d);
    for (size_t p = 0; p < marginals[d].size(); ++p)
    {
      for (Index index(result.sizes(), std::make_pair(d, p)); !index.end(); ++index)
      {
        const std::vector<int64_t>& ref = index;
        // avoid division by zero (assume 0/0 -> 0)
#ifndef NDEBUG
        if (r[p] == 0.0 && marginals[d][ref[d]] != 0.0)
          throw std::runtime_error("div0 in rScale with m>0");
        if (r[p] != 0.0)
          result[index] *= marginals[d][ref[d]] / r[p];
        else
          result[index] = 0.0;
#else
        result[index] = r[p] > 0.0 ? marginals[d][ref[d]] / r[p] : 0.0;
#endif
      }
    }
  }
}

void rDiff(std::vector<std::vector<double>>& diffs, const NDArray<double>& result, const std::vector<std::vector<double>>& marginals)
{
  int64_t n = result.dim();
  for (int64_t d = 0; d < n; ++d)
    diffs[d] = diff(reduce<double>(result, d), marginals[d]);
}

}

namespace deprecated {

// construct from fractional marginals
IPF::IPF(const NDArray<double>& seed, const std::vector<std::vector<double>>& marginals)
  : m_result(seed.sizes()), m_marginals(marginals), m_errors(seed.dim()), m_conv(false)
{
  if (m_marginals.size() != seed.dim())
    throw std::runtime_error("no. of marginals doesnt match dimensionalty");
  solve(seed);
}

// construct from integer marginals
IPF::IPF(const NDArray<double>& seed, const std::vector<std::vector<int64_t>>& marginals)
: m_result(seed.sizes()), m_marginals(marginals.size()), m_errors(marginals.size()), m_conv(false)
{
  if (marginals.size() != seed.dim())
    throw std::runtime_error("no. of marginals doesnt match dimensionalty");
  for (size_t d = 0; d < seed.dim(); ++d)
  {
    m_marginals[d].reserve(marginals[d].size());
    std::copy(marginals[d].begin(), marginals[d].end(), std::back_inserter(m_marginals[d]));
  }
  solve(seed);
}

void IPF::solve(const NDArray<double>& seed)
{
  // reset convergence flag
  m_conv = false;
  m_population = std::accumulate(m_marginals[0].begin(), m_marginals[0].end(), 0.0);
  // checks on marginals, dimensions etc
  for (size_t i = 0; i < seed.dim(); ++i)
  {
    if ((int64_t)m_marginals[i].size() != m_result.sizes()[i])
      throw std::runtime_error("marginal doesnt have correct length");

    double mpop = std::accumulate(m_marginals[i].begin(), m_marginals[i].end(), 0.0);
    if (mpop != m_population)
      throw std::runtime_error("marginal doesnt have correct population");
  }

  //print(seed.rawData(), seed.storageSize(), m_marginals[1].size());
  std::copy(seed.rawData(), seed.rawData() + seed.storageSize(), const_cast<double*>(m_result.rawData()));
  for (size_t d = 0; d < m_result.dim(); ++d)
  {
    m_errors[d].resize(m_marginals[d].size());
    //print(m_marginals[d]);
  }
  //print(m_result.rawData(), m_result.storageSize(), m_marginals[1].size());

  std::vector<std::vector<double>> diffs(m_result.dim());
  for (m_iters = 0; !m_conv && m_iters < s_MAXITER; ++m_iters)
  {
    rScale(m_result, m_marginals);
    // inefficient copying?

    rDiff(diffs, m_result, m_marginals);

    m_conv = computeErrors(diffs);
  }
}

size_t IPF::population() const
{
  return m_population;
}

const NDArray<double>& IPF::result() const
{
  return m_result;
}

const std::vector<std::vector<double>> IPF::errors() const
{
  return m_errors;
}

double IPF::maxError() const
{
  return m_maxError;
}

bool IPF::conv() const
{
  return m_conv;
}

size_t IPF::iters() const
{
  return m_iters;
}

bool IPF::computeErrors(std::vector<std::vector<double>>& diffs)
{
  m_maxError = -std::numeric_limits<double>::max();
  for (size_t d = 0; d < m_result.dim(); ++d)
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

}
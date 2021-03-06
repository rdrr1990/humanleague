% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/RcppExports.R
\name{synthPopG}
\alias{synthPopG}
\title{[Legacy] Generate a population in 2 dimensions given 2 1-d marginals and a further constraint.}
\usage{
synthPopG(marginals, exoProbsIn)
}
\arguments{
\item{marginals}{a List of 2 integer vectors containing marginal data. The sum of elements in each vector must be identical}

\item{exoProbsIn}{a 2d array of exogenous state probabilities}
}
\value{
an object containing: the population matrix, the occupancy probability matrix, a convergence flag, the chi-squared statistic, p-value, and error value (nonzero if not converged)
}
\description{
Using Quasirandom Integer Without-replacement Sampling (QIWS), this function
generates an n-dimensional population table where elements sum to the input marginals, and supplemental data.
The scope of this function is rather limited (qisi is more general).
}
\examples{
synthPopG(list(c(1,2,3,4), c(3,4,3)), array(rep(1,12), dim=c(4,3)))
}

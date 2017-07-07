
#include "Object.h"

#include "humanleague/src/Sobol.h"
#include "humanleague/src/RQIWS.h"

#include <Python.h>

#include <vector>
#include <stdexcept>

#include <iostream>


template<size_t D>
pycpp::List flatten(const size_t pop, const NDArray<D,uint32_t>& t)
{
  std::vector<std::vector<int>> list = listify<D>(pop, t);

  pycpp::List outer(D);
  for (size_t i = 0; i < D; ++i)
  {
//    pycpp::List inner(list[i].size());
//    for (size_t j = 0; j < list[i].size(); ++j) 
//    {
//      inner.set(j, pycpp::Int(list[i][j])); // = pycpp::List(list[i]);    
//    }
//    outer.set(i, std::move(inner));
    outer.set(i, pycpp::List(list[i]));
  }

  return outer;
}

// TODO multidim
template<typename S>
void doSolve(pycpp::Dict& result, size_t dims, const std::vector<std::vector<uint32_t>>& m)
{
  S qiws(m); 
  result.insert("conv", pycpp::Bool(qiws.solve()));
  result.insert("result", flatten(qiws.population(), qiws.result()));
  result.insert("p-value", pycpp::Double(qiws.pValue().first));
  result.insert("chiSq", pycpp::Double(qiws.chiSq()));
  result.insert("pop", pycpp::Int(qiws.population()));
}

// prevents name mangling (but works without this)
extern "C" PyObject* humanleague_sobol(PyObject *self, PyObject *args)
{
  try 
  {
    int dim, length, skips = 0;

    // args e.g. "s" for string "i" for integer, "d" for double "ss" for 2 strings
    if (!PyArg_ParseTuple(args, "ii|i", &dim, &length, &skips))
      return nullptr;
      
    pycpp::List outer(length);
    if (!outer)
      return nullptr;

    Sobol sobol(dim, skips);
    double scale = 0.5 / (1u << 31);

    for (int n = 0; n < length; ++n)
    {
      std::vector<pycpp::List> inner(length, pycpp::List(dim));
      const std::vector<unsigned int>& v = sobol.buf();
      for (int i = 0; i < dim; ++i) 
      {
        inner[n].set(i, pycpp::Double(v[i] * scale));
      }
      outer.set(n, std::move(inner[n]));
    }
    return outer.release();
  }
  catch(const std::exception& e)
  {
    return &pycpp::String(e.what());
  }
  catch(...)
  {
    return &pycpp::String("unexpected exception");
  }
}


// prevents name mangling (but works without this)
extern "C" PyObject* humanleague_synthPop(PyObject *self, PyObject *args)
{
  try 
  {
    PyObject* arrayArg;

    // args e.g. "s" for string "i" for integer, "d" for float "ss" for 2 strings
    if (!PyArg_ParseTuple(args, "O!", &PyList_Type, &arrayArg))
      return nullptr;
    
    pycpp::List list(arrayArg);
    
    size_t dim = list.size();
    std::vector<size_t> sizes(dim);
    std::vector<std::vector<uint32_t>> marginals(dim);
      
    for (size_t i = 0; i < dim; ++i) 
    {
      pycpp::List l = pycpp::List(list[i]);
      sizes[i] = l.size();
      marginals[i] = l.toVector<uint32_t>();
    }

    pycpp::Dict retval;

    switch(dim)
    {
    case 2:
      doSolve<QIWS<2>>(retval, dim, marginals);
      break;
    case 3:
      doSolve<QIWS<3>>(retval, dim, marginals);
      break;
//    case 4:
//      doSolve<QIWS<4>>(retval, dim, marginals);
//      break;
//    case 5:
//      doSolve<QIWS<5>>(retval, dim, marginals);
//      break;
//    case 6:
//      doSolve<QIWS<6>>(retval, dim, marginals);
//      break;
//    case 7:
//      doSolve<QIWS<7>>(retval, dim, marginals);
//      break;
//    case 8:
//      doSolve<QIWS<8>>(retval, dim, marginals);
//      break;
//    case 9:
//      doSolve<QIWS<9>>(retval, dim, marginals);
//      break;
//    case 10:
//      doSolve<QIWS<10>>(retval, dim, marginals);
//      break;
//    case 11:
//      doSolve<QIWS<11>>(retval, dim, marginals);
//      break;
//    case 12:
//      doSolve<QIWS<12>>(retval, dim, marginals);
//      break;
    default:
      throw std::runtime_error("invalid dimensionality: " + std::to_string(dim));
    }
    return retval.release();
  }
  catch(const std::exception& e)
  {
    return &pycpp::String(e.what());
  }
  catch(...)
  {
    return &pycpp::String("unexpected exception");
  }
}

//// prevents name mangling (but works without this)
//extern "C" PyObject* humanleague_synthPopC(PyObject *self, PyObject *args)
//{
//  try 
//  {
//    PyObject* marginal0Arg;
//    PyObject* marginal1Arg;
//    double rho;

//    // args e.g. "s" for string "i" for integer, "d" for float "ss" for 2 strings
//    if (!PyArg_ParseTuple(args, "O!O!d", &PyList_Type, &marginal0Arg, 
//                                         &PyList_Type, &marginal1Arg, &rho))
//      return nullptr;
//      
//    pycpp::List marginal0(marginal0Arg);
//    pycpp::List marginal1(marginal1Arg);
//    
//    std::vector<std::vector<uint32_t>> marginals(2);
//      
//    marginals[0] = marginal0.toVector<uint32_t>();
//    marginals[1] = marginal1.toVector<uint32_t>();
//    CQIWS cqiws(marginals, rho);
//    pycpp::Dict retval;
//    retval.set("conv", pycpp::Bool(rqiws.solve()));
//    retval.set("result", flatten(rqiws.population(), rqiws.result()));
//    retval.set("pop", pycpp::Int(rqiws.population()));
//    return retval.release();
//  }
//  catch(const std::exception& e)
//  {
//    return &pycpp::String(e.what());
//  }
//  catch(...)
//  {
//    return &pycpp::String("unexpected exception");
//  }
//}

// prevents name mangling (but works without this)
extern "C" PyObject* humanleague_synthPopR(PyObject *self, PyObject *args)
{
  try 
  {
    PyObject* marginal0Arg;
    PyObject* marginal1Arg;
    double rho;

    // args e.g. "s" for string "i" for integer, "d" for float "ss" for 2 strings
    if (!PyArg_ParseTuple(args, "O!O!d", &PyList_Type, &marginal0Arg, 
                                         &PyList_Type, &marginal1Arg, &rho))
      return nullptr;
      
    pycpp::List marginal0(marginal0Arg);
    pycpp::List marginal1(marginal1Arg);
    
    std::vector<std::vector<uint32_t>> marginals(2);
      
    marginals[0] = marginal0.toVector<uint32_t>();
    marginals[1] = marginal1.toVector<uint32_t>();
    RQIWS rqiws(marginals, rho);
    pycpp::Dict retval;
    retval.insert("conv", pycpp::Bool(rqiws.solve()));
    retval.insert("result", flatten(rqiws.population(), rqiws.result()));
    retval.insert("pop", pycpp::Int(rqiws.population()));
    return retval.release();
  }
  catch(const std::exception& e)
  {
    return &pycpp::String(e.what());
  }
  catch(...)
  {
    return &pycpp::String("unexpected exception");
  }
}


namespace {

PyMethodDef entryPoints[] = {
  {"sobolSequence", humanleague_sobol, METH_VARARGS, "Returns a Sobol sequence."},
  {"synthPop", humanleague_synthPop, METH_VARARGS, "Synthpop."},
  {"synthPopR", humanleague_synthPopR, METH_VARARGS, "Synthpop correlated."},
  {nullptr, nullptr, 0, nullptr}        /* terminator */
};

}

PyMODINIT_FUNC inithumanleague()
{
  Py_InitModule("humanleague", entryPoints);
}



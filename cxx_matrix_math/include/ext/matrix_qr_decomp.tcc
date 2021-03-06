#ifndef MATRIX_QR_DECOMP_TCC
#define MATRIX_QR_DECOMP_TCC 1


#include <cstdlib>
#include <iostream>
#include <vector>
#include <cmath>


namespace matrix
{



/**
 * Constructs the QR decomposition of a[0..n_rows - 1][0..n_cols - 1].  The upper triangular matrix R
 * is returned in the upper triangle of a except the diagonal elements of R which are returned
 * in d[0..n_cols - 1].  The orthogonal matrix Q is represented as a product of n - 1 Householder
 * matrices Q_0...Q_n-2 where Q_j = 1 - u_j x u_j/c_j.  The ith component of u_j is zero for
 * i = 
 */
template<typename _Matrix, typename _Vector>
  void
  qr_decomp(std::size_t n_rows, std::size_t n_cols,
	    _Matrix& a,
	    _Vector& c, _Vector& d, bool& singular)
  {
    using _NumTp = std::remove_reference_t<decltype(a[0][0])>;

    _NumTp sigma, sum, tau;

    //  Allocate the two vectors.
    //c.resize(n);
    //d.resize(n);

    singular = false;
    for (std::size_t k = 0; k < n_cols - 1; ++k)
      {
	//  See if the matrix is singular in this column.
	_NumTp scale = _NumTp{0};
	for (std::size_t i = k; i < n_rows; ++i)
	  if (scale < std::abs(a[i][k]))
	    scale = std::abs(a[i][k]);

	if (scale == _NumTp{0})
	  {
	    singular = true;
	    c[k] = d[k] = _NumTp{0};
	  }
	else
	  {
	    sum = _NumTp{0};
	    for (std::size_t i = k; i < n_rows; ++i)
	      {
		a[i][k] /= scale;
		sum += a[i][k] * a[i][k];
	      }
	    sigma = std::sqrt(sum);
	    if (a[k][k] < _NumTp{0})
	      sigma = -sigma;
	    a[k][k] += sigma;
	    c[k] = sigma * a[k][k];
	    d[k] = -scale * sigma;
	    for (std::size_t j = k + 1; j < n_cols; ++j)
	      {
		sum = _NumTp{0};
		for (std::size_t i = k; i < n_rows; ++i)
		  sum += a[i][k] * a[i][j];
		tau = sum/c[k];
		for (std::size_t i = k; i < n_rows; ++i)
		  a[i][j] -= tau * a[i][k];
	      }
	  }
      }
    c[n_cols - 1] = _NumTp{0};
    d[n_cols - 1] = a[n_cols - 1][n_cols - 1];

    if (d[n_cols - 1] == _NumTp{0})
      singular = true;

    return;
  }


/**
 * This routine solves the set of equations Rx = b where R is the upper triangular
 * matrix stored in a[0..n_rows - 1][0..n_cols - 1] and d[0..n_cols - 1].
 * Here n_rows >= n_cols.
 */
template<typename _Matrix, typename _Vector, typename _VectorB>
  void
  r_backsub(std::size_t n_rows, std::size_t n_cols,
	    const _Matrix& a, const _Vector& d,
	    _VectorB& b)
  {
    using _NumTp = std::decay_t<decltype(a[0][0])>;

    b[n_cols - 1] /= d[n_cols - 1];
    for (int i = n_cols - 2; i >= 0; --i)
      {
	_NumTp sum = _NumTp{0};
	for (std::size_t j = i + 1; j < n_rows; ++j)
	  sum += a[i][j] * b[j];
	b[i] = (b[i] - sum) / d[i];
      }

    return;
  }


/**
 * This routine solves the set of equations Ax = b.
 * The inputs are the QR decomposition of the matrix in a[0..n_rows - 1][0..n_cols - 1],
 * c[0..n_cols - 1], and d[0..n_cols - 1].
 * The vector b[0..n_rows - 1] is input as the "RHS" and output and the solution.
 * Here n_rows >= n_cols.
 */
template<typename _Matrix, typename _Vector, typename _VectorB>
  void
  qr_backsub(const std::size_t n_rows, const std::size_t n_cols,
	     const _Matrix& a, const _Vector& c, const _Vector& d,
	     _VectorB& b)
  {
    using _NumTp = std::decay_t<decltype(a[0][0])>;

    //  Form Qt.b.
    for (std::size_t j = 0; j < n_cols - 1; ++j)
      {
	_NumTp sum = _NumTp{0};
	for (std::size_t i = j; i < n_rows; ++i)
	  sum += a[i][j] * b[i];
	_NumTp tau = sum/c[j];
	for (std::size_t i = j; i < n_rows; ++i)
	  b[i] -= tau * a[i][j];
      }

    //  Solve R.x = Qt.b
    r_backsub(n_rows, n_cols, a, d, b);

    return;
  }


/**
 * Inverts a matrix given the QR decomposed matrix.
 * The inverse matrix is allocated in this routine so make sure the pointer is freed first.
 *
 * The inverse matrix is NOT in QR form.
 */
template<typename _Matrix, typename _Vector>
  void
  qr_invert(std::size_t n_rows, std::size_t n_cols,
	    const _Matrix& a_qr, const _Vector& c, const _Vector& d,
	    _Matrix& a_inv)
  {
    using _NumTp = std::decay_t<decltype(a_qr[0][0])>;

    std::vector<_NumTp> col(n_rows);

    for (std::size_t j = 0; j < n_rows; ++j)
      {
	for (std::size_t i = 0; i < n_rows; ++i)
	  col[i] = _NumTp{0};
	col[j] = _NumTp{1};

	qr_backsub(n_rows, n_cols, a_qr, c, d, col);

	for (std::size_t i = 0; i < n_cols; ++i)
	  a_inv[i][j] = col[i];
      }

    return;
  }



/**
 *  Update the QR decomposition.
 */
template<typename _Matrix, typename _Vector>
  void
  qr_update(std::size_t n_rows, std::size_t n_cols,
	    _Matrix& r, _Matrix& qt,
	    _Vector& u, _Vector& v)
  {
    using _NumTp = std::remove_reference_t<decltype(qt[0][0])>;

    //  Find largest k such that uk != 0.
    std::ptrdiff_t k;
    for (std::ptrdiff_t k = n_cols - 1; k >= 0; --k)
      if (u[k] != _NumTp{0})
	break;

    // Transform R + u x v to upper triangular.
    for (std::ptrdiff_t i = k - 1; i >= 0; --i)
      {
	jacobi_rotate(i, n_rows, n_cols, r, qt, u[i], -u[i + 1]);
	if (u[i] == 0)
	  u[i] = std::abs(u[i + 1]);
	else if (std::abs(u[i]) > std::abs(u[i + 1]))
	  u[i] = std::abs(u[i]) * std::sqrt(_NumTp{1} + (u[i + 1] / u[i]) * (u[i + 1] / u[i]));
	else
	  u[i] = std::abs(u[i + 1]) * std::sqrt(_NumTp{1} + (u[i] / u[i+1]) * (u[i] / u[i + 1]));
      }
    for (std::size_t j = 0; j < n_cols; ++j)
      r[0][j] += u[0] * v[j];

    // Transform upper Hessenberg matrix to upper triangular.
    for (std::size_t i = 0; i < k; ++i)
      jacobi_rotate(i, n_rows, n_cols, r, qt, r[i][i], -r[i + 1][i]);

    return;
  }


/**
 *  Do a Jacobi rotation on rows i and i+1 of the matrices r[0..n_cols - 1][0..n_cols - 1]
 *  and qt[0..n_cols - 1][0..n_rows - 1].
 *  The angle is specified with a and b such that
 *    cos(theta) = a/sqrt(a^2 + b^2)
 *    sin(theta) = b/sqrt(a^2 + b^2).
 */
template<typename _NumTp, typename _Matrix, typename _Vector>
  void
  jacobi_rotate(const int i, const int n_rows, const int n_cols,
		_Matrix& r, _Matrix& qt,
		_NumTp a, _NumTp b)
  {
    _NumTp c, s;
    if (a == _NumTp{0})
      {
	// Avoid underflow or overflow.
	c = _NumTp{0};
	s = _NumTp{1};
	if (b < _NumTp{0})
	  s = -_NumTp{1};
      }
    else
      {
	auto fact = std::hypot(a, b);
	c = a / fact;
	s = b / fact;
      }

    // Multiply r by the Jacobi rotation.
    for (std::size_t j = i; j < n_cols; ++j)
      {
	auto y = r[  i  ][j];
	auto w = r[i + 1][j];
	r[  i  ][j] = c * y - s * w;
	r[i + 1][j] = s * y + c * w;
      }

    // Multiply qt by the Jacobi rotation.
    for (std::size_t j = 0; j < n_rows; ++j)
      {
	auto y = qt[  i  ][j];
	auto w = qt[i + 1][j];
	qt[  i  ][j] = c * y - s * w;
	qt[i + 1][j] = s * y + c * w;
      }

    return;
  }

} // namespace matrix

#endif // MATRIX_QR_DECOMP_TCC


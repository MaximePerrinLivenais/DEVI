#include "gaussian_directional_2d.hpp"

#include <algorithm>
#include <cmath>
#include <mln/core/image/ndimage.hpp>

namespace
{

  struct recursivefilter_coef_
  {
    enum FilterType
    {
      DericheGaussian,
      DericheGaussianFirstDerivative,
      DericheGaussianSecondDerivative
    };

    float n[5];
    float d[5];
    float nm[5];
    float dm[5];
    float sumA, sumC;

    recursivefilter_coef_(float a0, float a1, float b0, float b1, float c0, float c1, float w0, float w1, float s,
                          FilterType filter_type)
    {
      b0 /= s;
      b1 /= s;
      w0 /= s;
      w1 /= s;

      float sin0 = std::sin(w0);
      float sin1 = std::sin(w1);
      float cos0 = std::cos(w0);
      float cos1 = std::cos(w1);

      switch (filter_type)
      {

      case DericheGaussian:
        sumA = (2.0 * a1 * exp(b0) * cos0 * cos0 - a0 * sin0 * exp(2.0 * b0) + a0 * sin0 - 2.0 * a1 * exp(b0)) /
               ((2.0 * cos0 * exp(b0) - exp(2.0 * b0) - 1) * sin0);

        sumC = (2.0 * c1 * exp(b1) * cos1 * cos1 - c0 * sin1 * exp(2.0 * b1) + c0 * sin1 - 2.0 * c1 * exp(b1)) /
               ((2.0 * cos1 * exp(b1) - exp(2.0 * b1) - 1) * sin1);
        break;

      case DericheGaussianFirstDerivative:
        sumA = -2.f *
               (a0 * cos0 - a1 * sin0 + a1 * sin0 * exp(2.0 * b0) + a0 * cos0 * exp(2.0 * b0) - 2.0 * a0 * exp(b0)) *
               exp(b0) /
               (exp(4.0 * b0) - 4.0 * cos0 * exp(3.0 * b0) + 2.0 * exp(2.0 * b0) + 4.0 * cos0 * cos0 * exp(2.0 * b0) +
                1.0 - 4.0 * cos0 * exp(b0));
        sumC = -2.f *
               (c0 * cos1 - c1 * sin1 + c1 * sin1 * exp(2.0 * b1) + c0 * cos1 * exp(2.0 * b1) - 2.0 * c0 * exp(b1)) *
               exp(b1) /
               (exp(4.0 * b1) - 4.0 * cos1 * exp(3.0 * b1) + 2.0 * exp(2.0 * b1) + 4.0 * cos1 * cos1 * exp(2.0 * b1) +
                1.0 - 4.0 * cos1 * exp(b1));
        break;

      case DericheGaussianSecondDerivative:
        float aux;
        aux = 12.0 * cos0 * exp(3.0 * b0) - 3.0 * exp(2.0 * b0) + 8.0 * cos0 * cos0 * cos0 * exp(3.0 * b0) -
              12.0 * cos0 * cos0 * exp(4.0 * b0) - (3.0 * exp(4.0 * b0)) + 6.0 * cos0 * exp(5.0 * b0) - exp(6.0 * b0) +
              6.0 * cos0 * exp(b0) - (1.0 + 12.0 * cos0 * cos0 * exp(2.0 * b0));
        sumA = 4.0 * a0 * sin0 * exp(3.0 * b0) + a1 * cos0 * cos0 * exp(4.0 * b0) -
               (4.0 * a0 * sin0 * exp(b0) + 6.0 * a1 * cos0 * cos0 * exp(2.0 * b0)) +
               2.0 * a1 * cos0 * cos0 * cos0 * exp(b0) - 2.0 * a1 * cos0 * exp(b0) +
               2.0 * a1 * cos0 * cos0 * cos0 * exp(3.0 * b0) - 2.0 * a1 * cos0 * exp(3.0 * b0) + a1 * cos0 * cos0 -
               a1 * exp(4.0 * b0) + 2.0 * a0 * sin0 * cos0 * cos0 * exp(b0) -
               2.0 * a0 * sin0 * cos0 * cos0 * exp(3.0 * b0) - (a0 * sin0 * cos0 * exp(4.0 * b0) + a1) +
               6.0 * a1 * exp(2.0 * b0) + a0 * cos0 * sin0 * 2.0 * exp(b0) / (aux * sin0);
        aux = 12.0 * cos1 * exp(3.0 * b1) - 3.0 * exp(2.0 * b1) + 8.0 * cos1 * cos1 * cos1 * exp(3.0 * b1) -
              12.0 * cos1 * cos1 * exp(4.0 * b1) - 3.0 * exp(4.0 * b1) + 6.0 * cos1 * exp(5.0 * b1) - exp(6.0 * b1) +
              6.0 * cos1 * exp(b1) - (1.0 + 12.0 * cos1 * cos1 * exp(2.0 * b1));
        sumC = 4.0 * c0 * sin1 * exp(3.0 * b1) + c1 * cos1 * cos1 * exp(4.0 * b1) -
               (4.0 * c0 * sin1 * exp(b1) + 6.0 * c1 * cos1 * cos1 * exp(2.0 * b1)) +
               2.0 * c1 * cos1 * cos1 * cos1 * exp(b1) - 2.0 * c1 * cos1 * exp(b1) +
               2.0 * c1 * cos1 * cos1 * cos1 * exp(3.0 * b1) - 2.0 * c1 * cos1 * exp(3.0 * b1) + c1 * cos1 * cos1 -
               c1 * exp(4.0 * b1) + 2.0 * c0 * sin1 * cos1 * cos1 * exp(b1) -
               2.0 * c0 * sin1 * cos1 * cos1 * exp(3.0 * b1) - (c0 * sin1 * cos1 * exp(4.0 * b1) + c1) +
               6.0 * c1 * exp(2.0 * b1) + c0 * cos1 * sin1 * 2.0 * exp(b1) / (aux * sin1);
        sumA /= 2;
        sumC /= 2;
        break;
      }

      a0 /= (sumA + sumC);
      a1 /= (sumA + sumC);
      c0 /= (sumA + sumC);
      c1 /= (sumA + sumC);

      n[3] = exp(-b1 - 2 * b0) * (c1 * sin1 - cos1 * c0) + exp(-b0 - 2 * b1) * (a1 * sin0 - cos0 * a0);
      n[2] = 2 * exp(-b0 - b1) * ((a0 + c0) * cos1 * cos0 - cos1 * a1 * sin0 - cos0 * c1 * sin1) + c0 * exp(-2 * b0) +
             a0 * exp(-2 * b1);
      n[1] = exp(-b1) * (c1 * sin1 - (c0 + 2 * a0) * cos1) + exp(-b0) * (a1 * sin0 - (2 * c0 + a0) * cos0);
      n[0] = a0 + c0;

      d[4] = exp(-2 * b0 - 2 * b1);
      d[3] = -2 * cos0 * exp(-b0 - 2 * b1) - 2 * cos1 * exp(-b1 - 2 * b0);
      d[2] = 4 * cos1 * cos0 * exp(-b0 - b1) + exp(-2 * b1) + exp(-2 * b0);
      d[1] = -2 * exp(-b1) * cos1 - 2 * exp(-b0) * cos0;

      switch (filter_type)
      {
      case DericheGaussian:
      case DericheGaussianSecondDerivative:

        for (unsigned i = 1; i <= 3; ++i)
        {
          dm[i] = d[i];
          nm[i] = n[i] - d[i] * n[0];
        }
        dm[4] = d[4];
        nm[4] = -d[4] * n[0];
        break;

      case DericheGaussianFirstDerivative:

        for (unsigned i = 1; i <= 3; ++i)
        {
          dm[i] = d[i];
          nm[i] = -(n[i] - d[i] * n[0]);
        }
        dm[4] = d[4];
        nm[4] = d[4] * n[0];
        break;
      }
    }
  };


  void gaussian1d(recursivefilter_coef_ c, float* input, std::ptrdiff_t size, float* tmp1, float* tmp2)
  {
    tmp1[0] = c.n[0] * input[0];

    tmp1[1] = 0                   //
              + c.n[0] * input[1] //
              + c.n[1] * input[0] //
              - c.d[1] * tmp1[0];

    tmp1[2] = 0                   //
              + c.n[0] * input[2] //
              + c.n[1] * input[1] //
              + c.n[2] * input[0] //
              - c.d[1] * tmp1[1]  //
              - c.d[2] * tmp1[0];

    tmp1[3] = 0                   //
              + c.n[0] * input[3] //
              + c.n[1] * input[2] //
              + c.n[2] * input[1] //
              + c.n[3] * input[0] //
              - c.d[1] * tmp1[2]  //
              - c.d[2] * tmp1[1]  //
              - c.d[3] * tmp1[0];

    for (int i = 4; i < size; i++)
    {
      tmp1[i] = c.n[0] * input[i + 0] + c.n[1] * input[i - 1] + //
                c.n[2] * input[i - 2] + c.n[3] * input[i - 3] - //
                c.d[1] * tmp1[i - 1] - c.d[2] * tmp1[i - 2] -   //
                c.d[3] * tmp1[i - 3] - c.d[4] * tmp1[i - 4];    //
    }

    // Non causal part

    tmp2[size - 1] = 0;

    tmp2[size - 2] = c.nm[1] * input[size - 1]; //

    tmp2[size - 3] = c.nm[1] * input[size - 2] + //
                     c.nm[2] * input[size - 1] - //
                     c.dm[1] * tmp2[size - 2];   //

    tmp2[size - 4] = c.nm[1] * input[size - 3] + //
                     c.nm[2] * input[size - 2] + //
                     c.nm[3] * input[size - 1] - //
                     c.dm[1] * tmp2[size - 3] -  //
                     c.dm[2] * tmp2[size - 2];   //

    for (int i = size - 5; i >= 0; --i)
    {
      tmp2[i] = c.nm[1] * input[i + 1] +                        //
                c.nm[2] * input[i + 2] +                        //
                c.nm[3] * input[i + 3] +                        //
                c.nm[4] * input[i + 4] -                        //
                c.dm[1] * tmp2[i + 1] - c.dm[2] * tmp2[i + 2] - //
                c.dm[3] * tmp2[i + 3] - c.dm[4] * tmp2[i + 4];  //
    }

    for (int i = 0; i < size; ++i)
      input[i] = tmp1[i] + tmp2[i];
  }


  template <class T>
  void copy_from_column(const mln::image2d<T>& f, int c, float* buffer)
  {
    int h = f.height();
    int y0 = f.domain().y();
    for (int y = 0; y < h; ++y)
      buffer[y] = f({c, y + y0});
  }

  template <class T>
  void copy_to_column(const float* buffer, int c, mln::image2d<T>& f)
  {
    int h = f.height();
    int y0 = f.domain().y();
    for (int y = 0; y < h; ++y)
      f({c, y + y0}) = buffer[y];
  }


  template <class T>
  void gaussian2d_T(mln::image2d<T>& input, float h_sigma, float v_sigma, T border_value)
  {




    int b      = 5 * static_cast<int>(std::max(h_sigma, v_sigma) + 0.5f);
    int width  = input.width();
    int height = input.height();

    int tmp_size = std::max(width, height) + 2 * b;

    // Allocate temporary buffer
    std::vector<float> tmp(3 * tmp_size);
    float*             i_buffer = tmp.data();
    float*             tmp1     = tmp.data() + tmp_size;
    float*             tmp2     = tmp.data() + 2 * tmp_size;

    // Fill border
    std::fill_n(i_buffer, tmp_size, border_value);


    const int x0 = input.domain().x();
    if (v_sigma != 0.f)
    {
      recursivefilter_coef_ coef(1.68f, 3.735f, 1.783f, 1.723f, -0.6803f, -0.2598f, 0.6318f, 1.997f, v_sigma,
                                 recursivefilter_coef_::DericheGaussian);


      int size = height + 2 * b;
      for (int x = 0; x < width; ++x)
      {
        copy_from_column(input, x + x0, i_buffer + b);
        gaussian1d(coef, i_buffer, size, tmp1, tmp2);
        copy_to_column(i_buffer + b, x + x0, input);
      }
    }


    // Fill border
    std::fill_n(i_buffer, tmp_size, border_value);


    if (h_sigma != 0.f)
    {
      recursivefilter_coef_ coef(1.68f, 3.735f, 1.783f, 1.723f, -0.6803f, -0.2598f, 0.6318f, 1.997f, h_sigma,
                                 recursivefilter_coef_::DericheGaussian);

      int size = width + 2 * b;
      uint8_t* lineptr = input.buffer();
      for (int y = 0; y < height; ++y)
      {
        std::copy_n(lineptr, width, i_buffer + b);
        gaussian1d(coef, i_buffer, size, tmp1, tmp2);
        std::copy_n(i_buffer + b, width, lineptr);
        lineptr += input.stride();
      }
    }
  }
} // namespace



void gaussian2d(mln::image2d<uint8_t>& input, float h_sigma, float v_sigma, uint8_t border_value)
{
  gaussian2d_T(input, h_sigma, v_sigma, border_value);
}

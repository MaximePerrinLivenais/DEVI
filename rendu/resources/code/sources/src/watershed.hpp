#pragma once

#include <mln/morpho/watershed.hpp>



/******************************************/
/****          Implementation          ****/
/******************************************/

namespace impl
{

  template <class I, class N, class O>
  int watershed(I input, N nbh, O markers, int nlabel)
  {
    using Label_t = mln::image_value_t<O>;

    // 1. Labelize minima (note that output is initialized to -1)
    // const int nlabel = mln::labeling::experimental::impl::local_minima(input, nbh, output, std::less<Label_t>());
    O output = markers;


    constexpr int kUnlabeled = -2;
    constexpr int kInqueue   = -1;
    constexpr int kWaterline = 0;

    // 2. inset neighbors inqueue
    // Pixels in the border gets the status 0 (deja vu)
    // Pixels in the queue get -1
    // Pixels not in the queue get -2
    constexpr auto impl_type = mln::morpho::details::pqueue_impl::linked_list;
    mln::morpho::details::pqueue_fifo<I, impl_type, /* reversed = */ true> pqueue(input);
    {
      output.extension().fill(kWaterline);

      mln_foreach (auto px, output.pixels())
      {
        // Not a local minimum => early exit
        if (px.val() != 0)
          continue;

        bool is_local_min_neighbor = false;
        for (auto nx : nbh(px))
          if (nx.val() > 0) // p is neighbhor to a local minimum
          {
            is_local_min_neighbor = true;
            break;
          }
        if (is_local_min_neighbor)
        {
          px.val() = kInqueue;
          pqueue.push(input(px.point()), px.point());
        }
        else
        {
          px.val() = kUnlabeled;
        }
      }
    }

    // 3. flood from minima
    {
      while (!pqueue.empty())
      {
        auto [level, p] = pqueue.top();

        auto pxOut = output.pixel(p);
        mln_assertion(pxOut.val() == kInqueue);
        pqueue.pop();

        // Check if there is a single marked neighbor
        Label_t common_label               = kWaterline;
        bool    has_single_adjacent_marker = false;
        for (auto nxOut : nbh(pxOut))
        {
          int nlbl = nxOut.val();
          if (nlbl <= 0)
            continue;
          else if (common_label == kWaterline)
          {
            common_label               = nlbl;
            has_single_adjacent_marker = true;
          }
          else if (nlbl != common_label)
          {
            has_single_adjacent_marker = false;
            break;
          }
        }

        if (!has_single_adjacent_marker)
        {
          // If there are multiple labels => waterline
          pxOut.val() = kWaterline;
        }
        else
        {
          // If a single label, it gets labeled
          // Add neighbors in the queue
          pxOut.val() = common_label;
          for (auto q : nbh(p))
          {
            auto nlbl = output.at(q);
            if (nlbl == kUnlabeled)
            {
              pqueue.push(input(q), q);
              output(q) = kInqueue;
            }
          }
        }
      }
    }

    // 3. Label all unlabeled pixels
    {
      mln::for_each(output, [](auto& v) {
        if (v < 0)
          v = kWaterline;
      });
    }

    return nlabel;
  }
} // namespace impl

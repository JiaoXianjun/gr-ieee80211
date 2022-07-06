/*
 *
 *     GNU Radio IEEE 802.11a/g/n/ac 2x2
 *     Encoder of 802.11a/g/n/ac 1x1 and 2x2 payload part
 *     Copyright (C) June 1, 2022  Zelin Yun
 *
 *     This program is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU Affero General Public License as published
 *     by the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     This program is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU Affero General Public License for more details.
 *
 *     You should have received a copy of the GNU Affero General Public License
 *     along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef INCLUDED_IEEE80211_ENCODE_IMPL_H
#define INCLUDED_IEEE80211_ENCODE_IMPL_H

#include <ieee80211/encode.h>
#include <gnuradio/blocks/pdu.h>
#include "cloud80211phy.h"

#define ENCODE_S_IDLE 0
#define ENCODE_S_SCEDULE 1
#define ENCODE_S_ENCODE 2
#define ENCODE_S_COPY 3

#define DECODE_B_MAX 16000
#define DECODE_D_MAX 2000

namespace gr {
  namespace ieee80211 {

    class encode_impl : public encode
    {
    private:
      // block
      int d_sEncode;
      int d_nGen;
      int d_nBitsGen;
      int d_nBitsGenProcd;
      // msg
      void msgRead(pmt::pmt_t msg);
      uint8_t d_msg[DECODE_D_MAX];
      // modulation
      c8p_mod d_m;
      
     protected:
      int calculate_output_stream_length(const gr_vector_int &ninput_items);

     public:
      encode_impl(const std::string& lengthtagname = "packet_len");
      ~encode_impl();

      // Where all the action really happens
      int work(
              int noutput_items,
              gr_vector_int &ninput_items,
              gr_vector_const_void_star &input_items,
              gr_vector_void_star &output_items
      );
    };

  } // namespace ieee80211
} // namespace gr

#endif /* INCLUDED_IEEE80211_ENCODE_IMPL_H */

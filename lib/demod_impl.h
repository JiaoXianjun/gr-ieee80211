/*
 *
 *     GNU Radio IEEE 802.11a/g/n/ac 2x2
 *     Demodulation of 802.11a/g/n/ac 1x1 and 2x2 formats
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

#ifndef INCLUDED_IEEE80211_DEMOD_IMPL_H
#define INCLUDED_IEEE80211_DEMOD_IMPL_H

#include <ieee80211/demod.h>
#include <fftw3.h>
#include "cloud80211phy.h"

#define DEMOD_S_IDEL 0
#define DEMOD_S_FORMAT_CHECK 1
#define DEMOD_S_NONL_CHANNEL 2
#define DEMOD_S_VHT_SIGB 3
#define DEMOD_S_DEMOD 4

namespace gr {
  namespace ieee80211 {

    class demod_impl : public demod
    {
     private:
      // block
      int d_nProc;
      int d_sDemod;
      // received info from tag
      std::vector<gr::tag_t> tags;
      int d_nSigLMcs;
      int d_nSigLLen;
      gr_complex d_H[64];
      // check format
      gr_complex d_sig1[64];
      gr_complex d_sig2[64];
      float d_sigHtIntedLlr[96];
      float d_sigHtCodedLlr[96];
      float d_sigVhtAIntedLlr[96];
      float d_sigVhtACodedLlr[96];
      float d_sigVhtB20IntedLlr[52];
      float d_sigVhtB20CodedLlr[52];
      uint8_t d_sigHtBits[48];
      uint8_t d_sigVhtABits[48];
      uint8_t d_sigVhtB20Bits[26];
      // fft
      fftw_complex* d_fftLtfIn1;
      fftw_complex* d_fftLtfIn2;
      fftw_complex* d_fftLtfOut1;
      fftw_complex* d_fftLtfOut2;
      fftw_plan d_fftP;
      // packet info
      int d_format;
      c8p_mod d_m;
      c8p_sigHt d_sigHt;
      c8p_sigVhtA d_sigVhtA;
      int d_nSym;
      int d_nSymProcd;
      int d_nSymSamp;
      int d_nSamp;
      int d_nSampProcd;
      int d_ampdu;
      int d_nUnCoded;
      int d_nCodedProcd;
      gr_complex d_qam[52];
      float d_llr[832];     // this ver support upto 2x2
      // non-legacy channel
      /*      P_LTF 4x4
      | 1 -1  1  1 |
      | 1  1 -1  1 |
      | 1  1  1 -1 |
      |-1  1  1  1 |
      for 2x2 in the array
      | 0  1 |
      | 2  3 |      */
      gr_complex d_H_NL[64][C8P_MAX_LTF];
      // demod, convert qam to llr and deinterleave
      float d_symIntedLlr[1664];   // coded per symbol for 20MHz 4x4

     public:
      demod_impl();
      ~demod_impl();

      // Where all the action really happens
      void forecast (int noutput_items, gr_vector_int &ninput_items_required);

      int general_work(int noutput_items,
           gr_vector_int &ninput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);

    };

  } // namespace ieee80211
} // namespace gr

#endif /* INCLUDED_IEEE80211_DEMOD_IMPL_H */
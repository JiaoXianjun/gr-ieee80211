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

#include <gnuradio/io_signature.h>
#include "demod_impl.h"

namespace gr {
  namespace ieee80211 {

    demod::sptr
    demod::make()
    {
      return gnuradio::make_block_sptr<demod_impl>(
        );
    }

    demod_impl::demod_impl()
      : gr::block("demod",
              gr::io_signature::make(1, 1, sizeof(gr_complex)),
              gr::io_signature::make(1, 1, sizeof(uint8_t)))
    {
      d_nProc = 0;
      d_sDemod = S_WAIT;

      d_fftLtfIn1 = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * 64);
      d_fftLtfIn2 = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * 64);
      d_fftLtfOut1 = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * 64);
      d_fftLtfOut2 = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * 64);
    }

    demod_impl::~demod_impl()
    {
      fftw_free(d_fftLtfIn1);
      fftw_free(d_fftLtfIn2);
      fftw_free(d_fftLtfOut1);
      fftw_free(d_fftLtfOut2);
      fftw_destroy_plan(d_fftP);
    }

    void
    demod_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
      ninput_items_required[0] = noutput_items;
    }

    int
    demod_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {
      const gr_complex* inSig = static_cast<const gr_complex*>(input_items[0]);
      float* outLlrs = static_cast<float*>(output_items[0]);

      d_nProc = ninput_items[0];
      
      if(d_sDemod == S_WAIT)
      {
        // get tagged info, if legacy packet, go to demod directly
        get_tags_in_range(tags, 0, nitems_read(0) , nitems_read(0) + 1);
        if (tags.size()) {
          pmt::pmt_t d_meta = pmt::make_dict();
          for (auto tag : tags){
            d_meta = pmt::dict_add(d_meta, tag.key, tag.value);
          }
          d_nSigLMcs = pmt::to_long(pmt::dict_ref(d_meta, pmt::mp("mcs"), pmt::from_long(9999)));
          d_nSigLLen = pmt::to_long(pmt::dict_ref(d_meta, pmt::mp("len"), pmt::from_long(9999)));
          if((d_nSigLMcs >=0) && (d_nSigLMcs <8) && (d_nSigLLen >= 0) && (d_nSigLLen < 4096) && pmt::dict_has_key(d_meta, pmt::mp("csi")))
          {
            std::vector<gr_complex> tmp_csi = pmt::c32vector_elements(pmt::dict_ref(d_meta, pmt::mp("csi"), pmt::PMT_NIL));
            std::copy(tmp_csi.begin(), tmp_csi.end(), d_H);
            d_format = C8P_F_L;
            std::cout<<"ieee80211 demod, tagged mcs:"<<d_nSigLMcs<<", len:"<<d_nSigLLen<<std::endl;
            if(d_nSigLMcs == 0)
            {
              d_sDemod = S_FCHECK;
            }
            else
            {
              d_sDemod = S_DEMOD;
              signalParserL(d_nSigLMcs, d_nSigLLen, &d_m);
              d_nSym = (d_nSigLLen*8 + 22)/d_m.nDBPS + (((d_nSigLLen*8 + 22)%d_m.nDBPS) != 0);
              d_nSamp = d_nSym * 80;
              d_nSampProcd = 0;
            }
            consume_each(0);
            return 0;
          }
        }
      }
      else if(d_sDemod == S_FCHECK)
      {
        if(d_nProc >= 160)
        {
          std::cout<<"ieee80211 demod, legacy check with samples."<<std::endl;
          for(int i=0;i<64;i++)
          {
            d_fftLtfIn1[i][0] = (double)inSig[i+8].real();
            d_fftLtfIn1[i][1] = (double)inSig[i+8].imag();
            d_fftLtfIn2[i][0] = (double)inSig[i+8+80].real();
            d_fftLtfIn2[i][1] = (double)inSig[i+8+80].imag();
          }
          d_fftP = fftw_plan_dft_1d(64, d_fftLtfIn1, d_fftLtfOut1, FFTW_FORWARD, FFTW_ESTIMATE);
          fftw_execute(d_fftP);
          d_fftP = fftw_plan_dft_1d(64, d_fftLtfIn2, d_fftLtfOut2, FFTW_FORWARD, FFTW_ESTIMATE);
          fftw_execute(d_fftP);
          for(int i=0;i<64;i++)
          {
            if(i==0 || (i>=27 && i<=37))
            {
            }
            else
            {
              d_sig1[i] = gr_complex((float)d_fftLtfOut1[i][0], (float)d_fftLtfOut1[i][1]) / d_H[i];
              d_sig2[i] = gr_complex((float)d_fftLtfOut2[i][0], (float)d_fftLtfOut2[i][1]) / d_H[i];
            }
          }
          gr_complex tmpPilotSum1 = std::conj(d_sig1[7] - d_sig1[21] + d_sig1[43] + d_sig1[57]);
          gr_complex tmpPilotSum2 = std::conj(d_sig2[7] - d_sig2[21] + d_sig2[43] + d_sig2[57]);
          float tmpPilotSumAbs1 = std::abs(tmpPilotSum1);
          float tmpPilotSumAbs2 = std::abs(tmpPilotSum2);
          int j=24;
          gr_complex tmpM1, tmpM2;
          for(int i=0;i<64;i++)
          {
            if(i==0 || (i>=27 && i<=37) || i==7 || i==21 || i==43 || i==57){}
            else
            {
              tmpM1 = d_sig1[i] * tmpPilotSum1 / tmpPilotSumAbs1;
              tmpM2 = d_sig2[i] * tmpPilotSum2 / tmpPilotSumAbs2;
              d_sigLIntedLlr[j] = tmpM1.real();
              d_sigHtIntedLlr[j] = tmpM1.imag();
              d_sigVhtAIntedLlr[j] = tmpM1.real();
              d_sigLIntedLlr[j + 48] = tmpM2.real();
              d_sigHtIntedLlr[j + 48] = tmpM2.imag();
              d_sigVhtAIntedLlr[j + 48] = tmpM2.imag();
              j++;
              if(j == 48)
              {
                j = 0;
              }
            }
          }
          procDeintLegacyBpsk(d_sigHtIntedLlr, d_sigHtCodedLlr);
          procDeintLegacyBpsk(&d_sigHtIntedLlr[48], &d_sigHtCodedLlr[48]);
          SV_Decode_Sig(d_sigHtCodedLlr, d_sigHtBits, 48);
          if(signalCheckHt(d_sigHtBits))
          {
            d_format = C8P_F_HT;
            
          }
          else
          {
            procDeintLegacyBpsk(d_sigVhtAIntedLlr, d_sigVhtACodedLlr);
            procDeintLegacyBpsk(&d_sigVhtAIntedLlr[48], &d_sigVhtACodedLlr[48]);
            SV_Decode_Sig(d_sigVhtACodedLlr, d_sigVhtABits, 48);
            if(signalCheckVht(d_sigVhtABits))
            {
              d_format = C8P_F_VHT;
            }
          }

          if(d_format == C8P_F_L)
          {
            procDeintLegacyBpsk(d_sigLIntedLlr, d_sigLCodedLlr);
            procDeintLegacyBpsk(&d_sigLIntedLlr[48], &d_sigLCodedLlr[48]);
            // legacy parser
            signalParserL(d_nSigLMcs, d_nSigLLen, &d_m);
            d_nSym = (d_nSigLLen*8 + 22)/d_m.nDBPS + (((d_nSigLLen*8 + 22)%d_m.nDBPS) != 0);
            d_nSamp = d_nSym * 80;
            d_nSampProcd = 0;
          }
          d_sDemod = S_DEMOD;
          consume_each (160);
          return 0;
        }
        else
        {
          std::cout<<"ieee80211 demod, legacy check no samples."<<std::endl;
          consume_each (0);
          return 0;
        }
      }
      else if(d_sDemod = S_DEMOD)
      {
        consume_each (d_nProc);
        return 0;
      }

      // Tell runtime system how many input items we consumed on
      // each input stream.
      consume_each (0);

      // Tell runtime system how many output items we produced.
      return 0;
    }

  } /* namespace ieee80211 */
} /* namespace gr */

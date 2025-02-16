#include "ap_axi_sdata.h"
#include "ap_int.h"
#include "hls_stream.h"

#define DWIDTH 512
#define TDWIDTH 16
#define UWIDTH 96
#define NUM_RANK 2
#define MAX_BUFFER_SIZE 22528 // 1408*8*NUM_RANK(=2) -- it should be multiples of 1408 -- in this example, we have 8 tlast for input stream and 2*8 tlast for output stream (b/c its allgether)

// template<int D,int U,int TI,int TD>
// struct ap_axis{
//  ap_int<D> data;
//  ap_uint<D/8> keep;
//  ap_uint<D/8> strb;
//  ap_uint<U> user;
//  ap_uint<1> last;
//  ap_uint<TI> id;
//  ap_uint<TD> dest;
// };

// template<int D,int U,int TI,int TD>
// struct ap_axiu{
//  ap_uint<D> data;
//  ap_uint<D/8> keep;
//  ap_uint<D/8> strb;
//  ap_uint<U> user;
//  ap_uint<1> last;
//  ap_uint<TI> id;
//  ap_uint<TD> dest;
// };

// Note: tvalid and tready are always there! In other words, when you use "void example(int A[50], int B[50]) { #pragma HLS INTERFACE axis port=A"
// then you will have mandatory signals {tdata, tvalid, tready} but when you use ap_axiu struct you are adding optional signals like keep, strb, ... to
// {tdata, tvalid, tready}. They will be left unconnected if the downstream does not use them. 

typedef ap_axiu<DWIDTH, UWIDTH, 1, TDWIDTH> pkt;

extern "C" {
void krnl_loopback(hls::stream<pkt> &n2k,    // Internal Stream
	               hls::stream<pkt> &k2n,
                   unsigned int     size     // Size in bytes (size-local)
               ) {
#pragma HLS INTERFACE axis port = n2k
#pragma HLS INTERFACE axis port = k2n

  unsigned int bytes_per_beat = (DWIDTH / 8);
  unsigned int size_tot = NUM_RANK * size;
  unsigned int num_iter_local = (size / bytes_per_beat);
  unsigned int num_iter_global = (size_tot / bytes_per_beat);
  //unsigned int count = 0:

data_mover:
//   int i = 0;
  unsigned int buffer_idx_rank0 = 0;
  unsigned int buffer_idx_rank1 = 0;
  unsigned int buffer_idx_mux = 0;
  /*. For larger # of nodes, use:
  static unsigned int buffer_idx[NUM_RANK] = {0, 0};
  #pragma HLS ARRAY_PARTITION variable=buffer_idx complete dim=0
  #pragma HLS UNROLL
  */
  unsigned int global_idx = 0;
  pkt pkt_in;
  pkt pkt_out;
  // unsigned int recvd_rank = 0;
  ap_uint<8> recvd_this_IP;
  ap_uint<8> recvd_this_Port;
  // ap_uint<8> recvd_theirIP_0, recvd_theirIP_1;
  // ap_uint<8> recvd_theirPort_0, recvd_theirPort_1;
  ap_uint<8> this_rank;

  static ap_uint<32> acc_buf0[MAX_BUFFER_SIZE] = {0}; // when u use static you dont need to spend cycles for the initialization of memory and it is instead embedded into bitstream generation
  #pragma HLS BIND_STORAGE variable=acc_buf0 type=RAM_2P impl=BRAM

  static ap_uint<32> acc_buf1[MAX_BUFFER_SIZE] = {0};
  #pragma HLS BIND_STORAGE variable=acc_buf1 type=RAM_2P impl=BRAM

  static ap_uint<32> acc_buf2[MAX_BUFFER_SIZE] = {0};
  #pragma HLS BIND_STORAGE variable=acc_buf2 type=RAM_2P impl=BRAM

  static ap_uint<32> acc_buf3[MAX_BUFFER_SIZE] = {0};
  #pragma HLS BIND_STORAGE variable=acc_buf3 type=RAM_2P impl=BRAM

  static ap_uint<32> acc_buf4[MAX_BUFFER_SIZE] = {0};
  #pragma HLS BIND_STORAGE variable=acc_buf4 type=RAM_2P impl=BRAM

  static ap_uint<32> acc_buf5[MAX_BUFFER_SIZE] = {0};
  #pragma HLS BIND_STORAGE variable=acc_buf5 type=RAM_2P impl=BRAM

  static ap_uint<32> acc_buf6[MAX_BUFFER_SIZE] = {0};
  #pragma HLS BIND_STORAGE variable=acc_buf6 type=RAM_2P impl=BRAM

  static ap_uint<32> acc_buf7[MAX_BUFFER_SIZE] = {0};
  #pragma HLS BIND_STORAGE variable=acc_buf7 type=RAM_2P impl=BRAM

  static ap_uint<32> acc_buf8[MAX_BUFFER_SIZE] = {0};
  #pragma HLS BIND_STORAGE variable=acc_buf8 type=RAM_2P impl=BRAM

  static ap_uint<32> acc_buf9[MAX_BUFFER_SIZE] = {0};
  #pragma HLS BIND_STORAGE variable=acc_buf9 type=RAM_2P impl=BRAM

  static ap_uint<32> acc_buf10[MAX_BUFFER_SIZE] = {0};
  #pragma HLS BIND_STORAGE variable=acc_buf10 type=RAM_2P impl=BRAM

  static ap_uint<32> acc_buf11[MAX_BUFFER_SIZE] = {0};
  #pragma HLS BIND_STORAGE variable=acc_buf11 type=RAM_2P impl=BRAM

  static ap_uint<32> acc_buf12[MAX_BUFFER_SIZE] = {0};
  #pragma HLS BIND_STORAGE variable=acc_buf12 type=RAM_2P impl=BRAM

  static ap_uint<32> acc_buf13[MAX_BUFFER_SIZE] = {0};
  #pragma HLS BIND_STORAGE variable=acc_buf13 type=RAM_2P impl=BRAM

  static ap_uint<32> acc_buf14[MAX_BUFFER_SIZE] = {0};
  #pragma HLS BIND_STORAGE variable=acc_buf14 type=RAM_2P impl=BRAM

  static ap_uint<32> acc_buf15[MAX_BUFFER_SIZE] = {0};
  #pragma HLS BIND_STORAGE variable=acc_buf15 type=RAM_2P impl=BRAM

  ap_uint<32> temp_acc;
//   float temp = 2.0f; 
//   float data_d0, data_d1, data_d2, data_d3, data_d4, data_d5, data_d6, data_d7, data_d8, data_d9, data_d10, data_d11, data_d12, data_d13, data_d14, data_d15;

  // Auto-pipeline is going to apply pipeline to this loop
  while (global_idx < num_iter_global) {
    //Read incoming packet
    pkt_in = n2k.read();

    // ----------- compute the incoming rank
    // recvd_this_IP = pkt_in.user.range(39,32);
    // recvd_this_Port = pkt_in.user.range(7,0);
    this_rank = pkt_in.user.range(7,0); //myPort
    // this_rank = (ap_uint<16>(recvd_this_IP) << 8) | (ap_uint<16>(recvd_this_Port));
    // -----------

    // ----------- Pick the right index to update the buffer
    if (this_rank == 0){
      buffer_idx_mux = buffer_idx_rank0;
    }
    else{
      buffer_idx_mux = buffer_idx_rank1 + num_iter_local;
    }
    /*. For larger # of nodes, use:
    buffer_idx_mux = buffer_idx[this_rank];
    */
    // -----------

    // ---------- Perform the processing (accumulation)
    // unroll
    acc_buf0[buffer_idx_mux] = pkt_in.data.range(31,0);
    acc_buf1[buffer_idx_mux] = pkt_in.data.range(63,32);
    acc_buf2[buffer_idx_mux] = pkt_in.data.range(95,64);
    acc_buf3[buffer_idx_mux] = pkt_in.data.range(127,96);
    acc_buf4[buffer_idx_mux] = pkt_in.data.range(159,128);
    acc_buf5[buffer_idx_mux] = pkt_in.data.range(191,160);
    acc_buf6[buffer_idx_mux] = pkt_in.data.range(223,192);
    acc_buf7[buffer_idx_mux] = pkt_in.data.range(255,224);
    acc_buf8[buffer_idx_mux] = pkt_in.data.range(287,256);
    acc_buf9[buffer_idx_mux] = pkt_in.data.range(319,288);
    acc_buf10[buffer_idx_mux] = pkt_in.data.range(351,320);
    acc_buf11[buffer_idx_mux] = pkt_in.data.range(383,352);
    acc_buf12[buffer_idx_mux] = pkt_in.data.range(415,384);
    acc_buf13[buffer_idx_mux] = pkt_in.data.range(447,416);
    acc_buf14[buffer_idx_mux] = pkt_in.data.range(479,448);
    acc_buf15[buffer_idx_mux] = pkt_in.data.range(511,480);
    // -------------

    // ------------- update the indices
    // making indices to zero is not necessary
    global_idx++;
    if (this_rank == 0){
      if (buffer_idx_rank0 == num_iter_local - 1)
          buffer_idx_rank0 = 0;
      else
          buffer_idx_rank0++;
    }
    else{
      if (buffer_idx_rank1 == num_iter_local - 1)
          buffer_idx_rank1 = 0;
      else
          buffer_idx_rank1++;
    }
    /*. For larger # of nodes, use:
    if (buffer_idx[this_rank] == num_iter_local - 1)
          buffer_idx[this_rank] = 0;
      else
          buffer_idx[this_rank]++;
    */
    // --------

  }
    // literally doing a multicast
    // write to output stream (rank 0)
    for (int i = 0; i < num_iter_local*NUM_RANK; i++) {
        #pragma HLS LATENCY min=1 max=1000
        #pragma HLS PIPELINE
        temp_acc = acc_buf0[i] + acc_buf1[i] + acc_buf2[i] + acc_buf3[i] + acc_buf4[i] + acc_buf5[i] + acc_buf6[i] + acc_buf7[i] + acc_buf8[i] + acc_buf9[i] + acc_buf10[i] + acc_buf11[i] + acc_buf12[i] + acc_buf13[i] + acc_buf14[i] + acc_buf15[i];
        acc_buf0[i] = temp_acc;
        acc_buf1[i] = temp_acc;
        acc_buf2[i] = temp_acc;
        acc_buf3[i] = temp_acc;
        acc_buf4[i] = temp_acc;
        acc_buf5[i] = temp_acc;
        acc_buf6[i] = temp_acc;
        acc_buf7[i] = temp_acc;
        acc_buf8[i] = temp_acc;
        acc_buf9[i] = temp_acc;
        acc_buf10[i] = temp_acc;
        acc_buf11[i] = temp_acc;
        acc_buf12[i] = temp_acc;
        acc_buf13[i] = temp_acc;
        acc_buf14[i] = temp_acc;
        acc_buf15[i] = temp_acc;
    }

    for (int i = 0; i < num_iter_local*NUM_RANK; i++) {
        #pragma HLS LATENCY min=1 max=1000
        #pragma HLS PIPELINE
        pkt_out.data.range(31,0) = acc_buf0[i];
        pkt_out.data.range(63,32) = acc_buf1[i];
        pkt_out.data.range(95,64) = acc_buf2[i];
        pkt_out.data.range(127,96) = acc_buf3[i];
        pkt_out.data.range(159,128) = acc_buf4[i];
        pkt_out.data.range(191,160) = acc_buf5[i];
        pkt_out.data.range(223,192) = acc_buf6[i];
        pkt_out.data.range(255,224) = acc_buf7[i];
        pkt_out.data.range(287,256) = acc_buf8[i];
        pkt_out.data.range(319,288) = acc_buf9[i];
        pkt_out.data.range(351,320) = acc_buf10[i];
        pkt_out.data.range(383,352) = acc_buf11[i];
        pkt_out.data.range(415,384) = acc_buf12[i];
        pkt_out.data.range(447,416) = acc_buf13[i];
        pkt_out.data.range(479,448) = acc_buf14[i];
        pkt_out.data.range(511,480) = acc_buf15[i];
        pkt_out.keep = -1;
        if ((((size / bytes_per_beat) - 1)==i) || ((((i + 1) * DWIDTH/8) % 1408) == 0))
            pkt_out.last = 1;
        else 
            pkt_out.last = 0;
        pkt_out.dest = 1; // sending to the first NIC (the second entry of Arp table)
        k2n.write(pkt_out);
    }
      // write to output stream (rank 1)
      for (int i = 0; i < num_iter_local*NUM_RANK; i++) {
        #pragma HLS LATENCY min=1 max=1000
        #pragma HLS PIPELINE
        pkt_out.data.range(31,0) = acc_buf0[i];
        pkt_out.data.range(63,32) = acc_buf1[i];
        pkt_out.data.range(95,64) = acc_buf2[i];
        pkt_out.data.range(127,96) = acc_buf3[i];
        pkt_out.data.range(159,128) = acc_buf4[i];
        pkt_out.data.range(191,160) = acc_buf5[i];
        pkt_out.data.range(223,192) = acc_buf6[i];
        pkt_out.data.range(255,224) = acc_buf7[i];
        pkt_out.data.range(287,256) = acc_buf8[i];
        pkt_out.data.range(319,288) = acc_buf9[i];
        pkt_out.data.range(351,320) = acc_buf10[i];
        pkt_out.data.range(383,352) = acc_buf11[i];
        pkt_out.data.range(415,384) = acc_buf12[i];
        pkt_out.data.range(447,416) = acc_buf13[i];
        pkt_out.data.range(479,448) = acc_buf14[i];
        pkt_out.data.range(511,480) = acc_buf15[i];
        pkt_out.keep = -1;
        if ((((size / bytes_per_beat) - 1)==i) || ((((i + 1) * DWIDTH/8) % 1408) == 0))
            pkt_out.last = 1;
        else 
            pkt_out.last = 0;
        pkt_out.dest = 2; // sending to the second NIC (the third entry of Arp table)
        k2n.write(pkt_out);
    }
}
}
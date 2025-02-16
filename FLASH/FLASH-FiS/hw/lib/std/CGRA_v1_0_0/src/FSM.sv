`timescale 1ns / 1ps

`ifndef MY_INTERFACE
    `define MY_INTERFACE
    `include "my_interface.vh"
`endif

module FSM(input logic [entry_sz_state-1:0] entry_table,
        input logic clk,
        input logic rst,
        output logic [dwidth_int-1:0] itr_i, // outer-most loop
        output logic [dwidth_int-1:0] itr_j,
        output logic [dwidth_int-1:0] itr_k, // inner-most loop
        output logic [dwidth_RFadd-1:0] smart_ptr, // ptr to state_table and config_table
//        output logic done,
        input logic done_loader,
        input logic start_stream_in,
        output logic ready_stream_in // I have to wait (backpressure to stream_in) if done_loader has not been asserted yet
//        output logic [1:0] o_curr_state // DEBUG
//        output logic keep_start_stream_in // // if start_stream_in becomes one but keep_start_stream_in is one, do not deassert start_stream_in
        );

// k (inner-most loop) is handled differently than i and j b/c check_end is always with body
// but for i and j, we might have:
// for {
//    for{}
//    statements
//    }

// possible values for type_entry
localparam [1:0] init = 2'b00; //initialization; when you just see the paranthesis in the for Loops
localparam [1:0] bodyAndCheckEnd = 2'b01; // Body + end of for loop }
localparam [1:0] level_k = 2'b00; // level_k (innermost) is L=0
localparam [1:0] level_j = 2'b01;
localparam [1:0] level_i = 2'b10;

localparam [1:0] waiting = 2'b00;
localparam [1:0] inbound_started = 2'b01;
//localparam [1:0] stream_in_started = 2'b10;
localparam [1:0] ready_state_hold = 2'b10;
localparam [1:0] ready_state = 2'b11;

logic [1:0] next_state, curr_state;
logic valid;
logic t_done, t_done_d1; // t_done_d1 is one clk delayed of t_done
logic [1:0] level;
logic [4:0] sc;
logic [4:0] num_sc;
logic [1:0] type_entry;
logic [dwidth_int-1:0] triggered_on;
logic [dwidth_int-1:0] t_itr_i;
logic [dwidth_int-1:0] t_itr_j;
logic [dwidth_int-1:0] t_itr_k;
logic [dwidth_int-1:0] cmp_i;
logic [dwidth_int-1:0] cmp_j;
logic [dwidth_int-1:0] cmp_k;
logic [dwidth_RFadd-1:0] t_smart_ptr;
logic [dwidth_RFadd-1:0] label_j;
logic [dwidth_RFadd-1:0] label_k;


//assignments
assign valid = entry_table[47];
assign level = entry_table[46:45];
assign sc = entry_table[44:40];
assign num_sc = entry_table[39:35];
assign type_entry = entry_table[34:33];
assign triggered_on = entry_table[31:0];


always_ff @(posedge clk) begin
  if(rst) begin
    // They are all registers
    t_smart_ptr <= 0;
    t_itr_i <= 0;
    t_itr_j <= 0;
    t_itr_k <= 0;
    cmp_i <= 0; // cmp registers are used to store trigger_on from config tables which will be then compared to itr registers
    cmp_j <= 0;
    cmp_k <= 0;
    label_j <= 0;
    label_k <= 0;
  end
  else begin
    if (curr_state != ready_state) begin
        t_smart_ptr <= 0; // do not proceed
        t_itr_i <= 0;
        t_itr_j <= 0;
        t_itr_k <= 0;
        cmp_i <= 0; // cmp registers are used to store trigger_on from config tables which will be then compared to itr registers
        cmp_j <= 0;
        cmp_k <= 0;
        label_j <= 0;
        label_k <= 0;
    end
        
    else if (curr_state == ready_state && valid == 1'b0)
        t_smart_ptr <= 0; // valid = 0 => pull t_smart_ptr back to zero (useful at the end of comp)
    
    else if (curr_state == ready_state && valid && type_entry == init) begin// init type
      if (level == level_k) begin
        cmp_k <= triggered_on; // write to cmp registers
        t_smart_ptr <= t_smart_ptr + 1; // increment smart_ptr
      end
      else if (level == level_j) begin
        cmp_j <= triggered_on; // write to cmp registers
        label_j <= t_smart_ptr + 1; // write the address of current state_table+1 to label registers
        t_smart_ptr <= t_smart_ptr + 1; // increment smart_ptr
      end
      else if (level == level_i) begin
        cmp_i <= triggered_on; // write to cmp registers
        label_k <= t_smart_ptr + 1; // write the address of current state_table+1 to label registers
        t_smart_ptr <= t_smart_ptr + 1; // increment smart_ptr
      end
    end
    
    else if (curr_state == ready_state && valid && type_entry == bodyAndCheckEnd) begin // bodyAndCheckEnd state
      if (sc == num_sc - 1) begin
        if (level == level_k) begin
          if (cmp_k == t_itr_k + 1) begin //check_end is true
            t_itr_k <= 0; // reset the current itr
            t_smart_ptr <= t_smart_ptr + 1; // increment smart_ptr
          end
          else begin //check_end is not true
            t_itr_k <= t_itr_k + 1; // increment current itr
          end
        end
        else if (level == level_j) begin 
          if (cmp_j == t_itr_j + 1) begin // chceck_end is true 
            t_itr_j <= 0; // reset the current itr
            t_smart_ptr <= t_smart_ptr + 1; // increment smart_ptr
          end 
          else begin //check_end is not true
            t_itr_j <= t_itr_j + 1; // increment current itr
            t_smart_ptr <= label_j; // jmp to current label
          end
        end
        else if (level == level_i) begin 
            if (cmp_i == t_itr_i + 1) begin // check_end is true
                t_itr_i <= 0; // reset the current itr
                t_smart_ptr <= t_smart_ptr + 1; // increment smart_ptr
            end 
            else begin //check_end is not true
                t_itr_i <= t_itr_i + 1; // increment current itr
                t_smart_ptr <= label_k; // jmp to current label
            end
        end
      end
      else // sc!= num_sc - 1
        t_smart_ptr <= t_smart_ptr + 1; // increment smart_ptr
    end
  end
end

//always@(posedge clk) begin
//    if (rst)
//        t_done_d1 <= 1'b0;
//    else
//        t_done_d1 <= t_done;
//end

// we indicate the end of prcessing when we have reached an invalid state table entry
// done is asseted one cycle b/c once it is valid the next is invalid
always_comb begin
  if(t_smart_ptr==0) 
    t_done = 1'b0; // to avoid asserting done when we even havent started yet
  else begin
    if(valid) 
        t_done = 1'b0;
    else 
        t_done = 1'b1;
  end
end

assign smart_ptr = t_smart_ptr;
//assign done = t_done & (~t_done_d1);
//assign done = t_done;

assign itr_i = t_itr_i;
assign itr_j = t_itr_j;
assign itr_k = t_itr_k;

// state machine for generating ready signal:
// I have to wait (backpressure to stream_in) if done_loader has not been asserted yet
always_ff @(posedge clk) begin
    if (rst)
        curr_state <= waiting;
    else 
        curr_state <= next_state;
end

//    if (curr_state == waiting && done_loader == 1'b0)
//        next_state = waiting;
//    else if (curr_state == waiting && done_loader == 1'b1) 
//        next_state = inbound_started;
//    else if (curr_state == inbound_started && done_loader == 1'b0 && start_stream_in == 1'b1)
//        next_state = ready_state_hold;
//    else if (curr_state == ready_state_hold && start_stream_in == 1'b0)
//        next_state = ready_state;
//    else if (curr_state == ready_state && t_done == 1'b1)
//        next_state = waiting;

always_comb begin
    case(curr_state)
        waiting: next_state = (done_loader) ? inbound_started: waiting;
        inbound_started: next_state = (start_stream_in && !done_loader)? ready_state_hold: inbound_started;
        ready_state_hold: next_state = (!start_stream_in)? ready_state: ready_state_hold;
        ready_state: next_state = (t_done) ? waiting: ready_state;
        default: next_state = waiting;
    endcase
end

//assign ready = (curr_state == inbound_started || curr_state == ready_state)? 1'b1: 1'b0;
//assign keep_start_stream_in = (curr_state != inbound_started)? 1'b1: 1'b0; // if start_stream_in becomes 1 but keep_start_stream_in is one, do not deassert start_stream_in
assign ready_stream_in = (curr_state == ready_state_hold || (curr_state == ready_state && t_done == 1'b0)) ? 1'b1: 1'b0; // 4-phase handshaking for ready_stream_in and start_stream_in. when ready becomes high start should be low and the next cycle after deasserting start, stream-in should send valid data.
//assign o_curr_state = curr_state;

endmodule

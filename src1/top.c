void wino_systolic_top(
    INPUT_PORTS_DECLARE(input_DDR),
    WEIGHT_PORTS_DECLARE(weight_DDR),
    ap_uint<ODDR_WIDTH*BATCH_SIZE*OUT_PORT_BATCH_NUM> *output_DDR0,
    ap_uint<ODDR_WIDTH*BATCH_SIZE*OUT_PORT_BATCH_NUM> *output_DDR1,
    // ap_uint<ODDR_WIDTH*BATCH_SIZE*OUT_PORT_BATCH_NUM> *output_DDR2,
    // ap_uint<ODDR_WIDTH*BATCH_SIZE*OUT_PORT_BATCH_NUM> *output_DDR3,
    // ConvDesc_t conv_desc,
    ap_int<32> *mem_params
    // ap_int<32> *bias_mem
    // #ifdef __SDSVHLS__
    // ,ap_uint<1> ap_clk_div2
    // #endif
    )
{

    // #ifndef __SDSVHLS__
    ap_uint<1> ap_clk_div2=0;
    // #endif

    // #pragma HLS interface m_axi port= input_DDR3 offset=slave   bundle=input_DDR3 depth=65535
    // #pragma HLS interface m_axi port= input_DDR2 offset=slave   bundle=input_DDR2 depth=65535
    #pragma HLS interface m_axi port= input_DDR1 offset=slave   bundle=input_DDR1 depth=524288
    #pragma HLS interface m_axi port= input_DDR0 offset=slave   bundle=input_DDR0 depth=524288
    #pragma HLS interface m_axi port= weight_DDR0 offset=slave  bundle=weight_DDR0 depth=1179648
    #pragma HLS interface m_axi port= weight_DDR1 offset=slave  bundle=weight_DDR1 depth=1179648
    #pragma HLS interface m_axi port= weight_DDR2 offset=slave  bundle=weight_DDR2 depth=1179648
    #pragma HLS interface m_axi port= weight_DDR3 offset=slave  bundle=weight_DDR3 depth=1179648
    // #pragma HLS interface m_axi port= output_DDR3 offset=slave  bundle=output_DDR3 depth=65535
    // #pragma HLS interface m_axi port= output_DDR2 offset=slave  bundle=output_DDR2 depth=65535
    #pragma HLS interface m_axi port= output_DDR1 offset=slave  bundle=output_DDR1 depth=524288
    #pragma HLS interface m_axi port= output_DDR0 offset=slave  bundle=output_DDR0 depth=524288
    // #pragma HLS interface m_axi port= weight_DDR3 offset=slave depth=65535
    // #pragma HLS interface m_axi port= weight_DDR2 offset=slave depth=65535
    // #pragma HLS interface m_axi port= weight_DDR1 offset=slave depth=65535

    #pragma HLS interface m_axi port= mem_params offset=slave  bundle=mem_params depth=65535
    // #pragma HLS interface m_axi port= bias_mem offset=slave  bundle=bias_mem depth=65535
	#pragma HLS INTERFACE s_axilite register port=return

    //input buffer declaration
    ap_uint<16> input_buffer[INBUFFER_HEIGHT][INBUFFER_WIDTH][INPUT_BUFFER_DEPTH];
    #pragma HLS array_partition variable=input_buffer complete dim=1 
    #pragma HLS array_partition variable=input_buffer complete dim=2 
    #pragma HLS resource variable=input_buffer core=RAM_S2P_BRAM 
    ap_uint<OUT_WIDTH*2> output_buffer0[WINO_OUT_SIZE_CELL][OUTDEPTH_MINITILE_SIZE/WINO_H2][WINO_WIDTH/WINO_W2][WINO_H2][WINO_W2][WINO_OUT_SIZE_CELL][OUTPUT_BUFFER_DEPTH];
    #pragma HLS array_partition variable=output_buffer0 complete dim=1 
    #pragma HLS array_partition variable=output_buffer0 complete dim=2
    #pragma HLS resource variable=output_buffer0 core=RAM_T2P_BRAM  
    ap_uint<OUT_WIDTH*2> output_buffer1[WINO_OUT_SIZE_CELL][OUTDEPTH_MINITILE_SIZE/WINO_H2][WINO_WIDTH/WINO_W2][WINO_H2][WINO_W2][WINO_OUT_SIZE_CELL][OUTPUT_BUFFER_DEPTH];
    #pragma HLS array_partition variable=output_buffer1 complete dim=1 
    #pragma HLS array_partition variable=output_buffer1 complete dim=2 
    #pragma HLS resource variable=output_buffer1 core=RAM_T2P_BRAM 
    ConvDesc_t conv_desc;

	ap_int<16> bias_buffer0[8][BIAS_BUFFER_DEPTH];
    #pragma HLS array_partition variable=bias_buffer0 complete dim=1 

	ap_int<16> bias_buffer1[8][BIAS_BUFFER_DEPTH];
    #pragma HLS array_partition variable=bias_buffer1 complete dim=1 
    
    ap_uint<1> pingpong=0;

    #if DEBUG_FILE_PRINT
        // clear_buffer_content<INBUFFER_HEIGHT,INBUFFER_WIDTH, INPUT_BUFFER_DEPTH>(input_buffer);
        clear_output_buffer_content<OUT_WIDTH,BATCH_SIZE,WINO_HEIGHT,WINO_WIDTH,WINO_OUT_SIZE_CELL,OUTPUT_BUFFER_DEPTH>(output_buffer0);
        clear_output_buffer_content<OUT_WIDTH,BATCH_SIZE,WINO_HEIGHT,WINO_WIDTH,WINO_OUT_SIZE_CELL,OUTPUT_BUFFER_DEPTH>(output_buffer1);
    #endif



    load_params(mem_params,conv_desc);
    #if WEIGHT_PORT_NUM==1
    load_bias_value(weight_DDR0, bias_buffer0, bias_buffer1,conv_desc.outdepth_align8);
    #elif WEIGHT_PORT_NUM==2
    load_bias_value(weight_DDR1, bias_buffer0, bias_buffer1,conv_desc.outdepth_align8);
    #else
    load_bias_value(weight_DDR3, bias_buffer0, bias_buffer1,conv_desc.outdepth_align8);
    #endif
    
    wino_input_compute(
        INPUT_PORTS_CALL(input_DDR),
        WEIGHT_PORTS_CALL(weight_DDR),
        output_buffer1,
        0, //ap_uint<16> start_output_row,
        0, //ap_uint<16> next_start_row,
        0, //ap_int<16> start_row_idx_minus_pad_size,
        1, //ap_uint<1> first_input_flag,
        0, //ap_uint<1> first_flag,
        0, //ap_uint<1> last_flag,
        conv_desc,
        ap_clk_div2
    );

   


  
    // write_params(weight_DDR0, conv_desc);
    
    #if DEBUG_FILE_PRINT
        // attach_input_buffer_content_uniformed<INBUFFER_HEIGHT,INBUFFER_WIDTH, INPUT_BUFFER_DEPTH>(input_buffer,0,(char*) "input_buffer_content.txt");
    #endif



    
    ap_int<16> write_start_row= -conv_desc.out_rowstep;
    ap_int<16> next_start_row= conv_desc.out_rowstep;





    for( ap_int<16> compute_start_row =0; compute_start_row < conv_desc.outheight; compute_start_row+=conv_desc.out_rowstep)
    {

        ap_uint<16> start_row_idx_minus_pad_size=compute_start_row-conv_desc.pad_size_h;
       
        if(pingpong )
        {

            // #if WINO_HEIGHT==2

            write_output_to_DDR3(
            output_DDR0,
            output_DDR1,
            // output_DDR2,
            // output_DDR3,
            output_buffer1,
            write_start_row,
            write_start_row==0,
            conv_desc
            );
            // #else

            // write_output_to_DDR2(
            // output_DDR0,
            // output_DDR1,
            // output_DDR2,
            // output_DDR3,
            // output_buffer1,
            // write_start_row,
            // write_start_row==0,
            // bias_buffer0,
            // bias_buffer1,
            // conv_desc
            // );

            // #endif

            wino_input_compute(
                INPUT_PORTS_CALL(input_DDR),
                WEIGHT_PORTS_CALL(weight_DDR),
                output_buffer0,
                compute_start_row,
                next_start_row,
                start_row_idx_minus_pad_size,
                0,
                compute_start_row==0,
                next_start_row > conv_desc.outheight,
                conv_desc,
                ap_clk_div2
            );

            #if 0
            char outfilename[100];
            sprintf(outfilename,"outbuffer.txt");
            attach_output_buffer_content_uniformed_hw<OUT_WIDTH,BATCH_SIZE,WINO_HEIGHT,WINO_WIDTH,WINO_OUT_SIZE_CELL,OUTPUT_BUFFER_DEPTH>(
                output_buffer0,0,outfilename);
            getchar();
            #endif



        }
        else
        {
            // #if WINO_HEIGHT==2
            write_output_to_DDR3(
            output_DDR0,
            output_DDR1,
            // output_DDR2,
            // output_DDR3,
            output_buffer0,
            write_start_row,
            write_start_row==0,
            conv_desc
            );
            // #else

            // write_output_to_DDR2(
            // output_DDR0,
            // output_DDR1,
            // output_DDR2,
            // output_DDR3,
            // output_buffer0,
            // write_start_row,
            // write_start_row==0,
            // bias_buffer0,
            // bias_buffer1,
            // conv_desc
            // );

            // #endif


            wino_input_compute(
                INPUT_PORTS_CALL(input_DDR),
                WEIGHT_PORTS_CALL(weight_DDR),
                output_buffer1,
                compute_start_row,
                next_start_row,
                start_row_idx_minus_pad_size,
                0,
                compute_start_row==0,
                next_start_row > conv_desc.outheight,
                conv_desc,
                ap_clk_div2);

            #if 0
            char outfilename[100];
            sprintf(outfilename,"outbuffer.txt");
            attach_output_buffer_content_uniformed_hw<OUT_WIDTH,BATCH_SIZE,WINO_HEIGHT,WINO_WIDTH,WINO_OUT_SIZE_CELL,OUTPUT_BUFFER_DEPTH>(
                output_buffer1,0,outfilename);
            getchar();
            #endif




        }


        pingpong =~pingpong;
        write_start_row+=conv_desc.out_rowstep;
        next_start_row+=conv_desc.out_rowstep;
        // #if DEBUG_FILE_PRINT
        //     // attach_input_buffer_content_uniformed<INBUFFER_HEIGHT,INBUFFER_WIDTH, INPUT_BUFFER_DEPTH>(input_buffer,0,(char*) "input_buffer_content.txt");
        // #endif
 
        
    }

    if(pingpong )
    {
        // #if WINO_HEIGHT==2
        write_output_to_DDR3(
        output_DDR0,
        output_DDR1,
        // output_DDR2,
        // output_DDR3,
        output_buffer1,
        write_start_row,
        write_start_row==0,
        conv_desc
        );
        // #else
        // write_output_to_DDR2(
        // output_DDR0,
        // output_DDR1,
        // output_DDR2,
        // output_DDR3,
        // output_buffer1,
        // write_start_row,
        // write_start_row==0,
        // bias_buffer0,
        // bias_buffer1,
        // conv_desc
        // );
        // #endif

    }
    else
    {
        // #if WINO_HEIGHT==2
        write_output_to_DDR3(
        output_DDR0,
        output_DDR1,
        // output_DDR2,
        // output_DDR3,
        output_buffer0,
        write_start_row,
        write_start_row==0,
        conv_desc
        );
        // #else
        // write_output_to_DDR2(
        // output_DDR0,
        // output_DDR1,
        // output_DDR2,
        // output_DDR3,
        // output_buffer0,
        // write_start_row,
        // write_start_row==0,
        // bias_buffer0,
        // bias_buffer1,
        // conv_desc
        // );
        // #endif

    }
}
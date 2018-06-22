#pragma HLS interface ap_fifo port=_desc_mat.data
#pragma HLS interface ap_fifo port=_in_mat.data

	hls::stream< XF_TNAME(SRC_T,NPC) > in_strm;
	hls::stream< XF_CTUNAME(SRC_T,NPC) > in[IMG_COLOR];
	hls::stream< XF_SNAME(XF_576UW) > _block_strm;
	hls::stream< XF_TNAME(DST_T,NPC) > desc_strm;

#pragma HLS DATAFLOW

	int IN_TC=(ROWS*COLS>>XF_BITSHIFT(NPC));
	for (int i = 0; i < _in_mat.size; i++)
	{
#pragma HLS pipeline ii=1
#pragma HLS LOOP_TRIPCOUNT min=1 max=IN_TC
		in_strm.write(*(_in_mat.data + i));
	}

	// Reads the input data from Input stream and writes the data to the output stream
	xFHOGReadFromStream < ROWS,COLS,IMG_COLOR > (in_strm,in,_in_mat.rows,_in_mat.cols);

	// Process function: performs HoG over the input stream and writes the descriptor data to the output stream
	xFDHOG < WIN_HEIGHT,WIN_WIDTH,WIN_STRIDE,BLOCK_HEIGHT,BLOCK_WIDTH,CELL_HEIGHT,CELL_WIDTH,
	NOB,ROWS,COLS,XF_8UP,XF_16UP,XF_NPPC1,XF_8UW,XF_576UW,IMG_COLOR > (in,_block_strm,_in_mat.rows,_in_mat.cols);

	if (OUTPUT_VARIANT == XF_HOG_RB) {
		// writes the Descriptor data Window wise
		xFWriteHOGDescRB < WIN_HEIGHT,WIN_WIDTH,WIN_STRIDE,CELL_HEIGHT,CELL_WIDTH,NOB,
		ROWS,COLS,XF_16UP,XF_16UP,XF_NPPC1,XF_576UW,XF_32UW > (_block_strm,desc_strm,_in_mat.rows,_in_mat.cols);
	}
	else if (OUTPUT_VARIANT == XF_HOG_NRB) {
		// writes the block data and the descriptors are formed on the host
		xFWriteHOGDescNRB < BLOCK_HEIGHT,BLOCK_WIDTH,CELL_HEIGHT,CELL_WIDTH,NOB,XF_DHOG,
		ROWS,COLS,XF_16UP,XF_NPPC1,XF_576UW,XF_TNAME(DST_T,NPC) > (_block_strm,desc_strm,_in_mat.rows,_in_mat.cols);
	}

	int OUT_TC=(ROWS*COLS>>XF_BITSHIFT(NPC));
	for (int i = 0; i < _desc_mat.size; i++)
	{
#pragma HLS pipeline ii=1
#pragma HLS LOOP_TRIPCOUNT min=1 max=IN_TC
		_desc_mat.data[i] = desc_strm.read();
	}
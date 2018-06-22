#pragma HLS interface m_axi offset=direct port=y1 bundle=y1
#pragma HLS interface m_axi offset=direct port=dy bundle=dy
#pragma HLS interface m_axi offset=direct port=status bundle=status
#pragma HLS interface m_axi offset=direct port=x1 bundle=x1
#pragma HLS interface m_axi offset=direct port=_in_mat.data bundle=_in_mat.data
#pragma HLS interface m_axi offset=direct port=dx bundle=dx
#pragma HLS interface m_axi offset=direct port=obj_height bundle=obj_height
#pragma HLS interface m_axi offset=direct port=obj_width bundle=obj_width
	// local arrays for memcopy
	uint16_t img_height[1],img_width[1],objects[1],frame[1];
	uint16_t tlx[MAXOBJ],tly[MAXOBJ],_obj_height[MAXOBJ],
		_obj_width[MAXOBJ],dispx[MAXOBJ],dispy[MAXOBJ];
	uint16_t track_status[MAXOBJ];

	memcpy(tlx,x1,2*MAXOBJ);
	memcpy(tly,y1,2*MAXOBJ);
	memcpy(_obj_height,obj_height,2*MAXOBJ);
	memcpy(_obj_width,obj_width,2*MAXOBJ);
	memcpy(dispx,dx,2*MAXOBJ);
	memcpy(dispy,dy,2*MAXOBJ);
	memcpy(track_status,status,2*MAXOBJ);

	xFMeanShiftKernel<OBJ_ROWS,OBJ_COLS,MAXOBJ,MAXITERS,NPC,XF_32UW>((unsigned int*)_in_mat.data,_in_mat.rows,_in_mat.cols,tlx,tly,_obj_height,
			_obj_width,dispx,dispy,track_status,frame_status,no_objects,no_iters);

	memcpy(dx,dispx,2*MAXOBJ);
	memcpy(dy,dispy,2*MAXOBJ);
	memcpy(status,track_status,2*MAXOBJ);
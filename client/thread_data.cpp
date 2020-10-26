#include"thread_data.hpp"
#include"dr_api.h"

ThreadData::ThreadData(int thread_id){
  tid = thread_id;
  cur_point = Point();

  // Initialization for the buffer containting memory references
  seg_base = reinterpret_cast<byte*>(dr_get_dr_segment_base(tls_seg));
  buf_base = reinterpret_cast<mem_ref_t*>(dr_raw_mem_alloc(MEM_BUF_SIZE,
                              DR_MEMPROT_READ | DR_MEMPROT_WRITE, nullptr));
// execution, such as BBV and LRU Stack Distance
  DR_ASSERT(seg_base != nullptr && buf_base != nullptr);
  /* Keep seg_base in a per-thread data structure so we can get the TLS
   * slot and find where the pointer points to in the buffer.
   */

  BUF_PTR(seg_base) = buf_base;
}

void ThreadData::save_floating_points(int fp_count){
	cur_point.update_fp_count(fp_count);
	return;
}


void ThreadData::set_time_start(double time_start){
	cur_point.set_start(time_start);
#ifdef VALIDATE
	dr_printf(">> Gathering timing information START <<\n");
#endif
return;
}


void ThreadData::set_time_end(double time_end){
	cur_point.set_end(time_end);
	return;
}

void ThreadData::save_bytes(void){
	mem_ref_t *mem_ref, *buf_ptr;
	buf_ptr = BUF_PTR(seg_base);

	for(mem_ref = (mem_ref_t *)(buf_base); mem_ref < buf_ptr; mem_ref++){
#ifdef VALIDATE_VERBOSE
		    dr_printf(">>Adding accessed Bytes: %lu ", mem_ref->size);
		    dr_printf("accessed by instruction at @" PFX "\n", mem_ref->addr);
#endif
		    cur_point.update_bytes(mem_ref->size);
            if(mem_ref->type == 0)
                cur_point.update_read_bytes(mem_ref->size);
            else if(mem_ref->type == 1)
                cur_point.update_write_bytes(mem_ref->size);
	}

	BUF_PTR(seg_base) = buf_base;
	     return;
}


void ThreadData::clean_buffer(void){
	BUF_PTR(seg_base) = buf_base;
}


// Saves the point pushing it to the point list.
void ThreadData::save_point(std::string label, unsigned int line, std::string src_file){
	if(cur_point.get_label().compare(label) != 0){
		if(!label.empty()){
			dr_printf("> WARNING: Ending ROI label '%s' does not match the starting one '%s'\n",
					cur_point.get_label().c_str(),
					label.c_str());
		}
	}

#ifdef VALIDATE
	dr_printf("> Saving ROI \n");
	dr_printf("> ROI detected label %s\n", label.c_str());
	dr_printf("> ROI detected %d line number \n", line);
	dr_printf("> ROI detected source file name '%s'\n",src_file.c_str());
#endif


	cur_point.set_line_end(line);
	cur_point.set_src_file_end(src_file);

	// Add the point to the list
	point_list.push_back(cur_point);
	cur_point.reset();

}

// Saves the point pushing it to the point list.
void ThreadData::new_point(std::string label, unsigned int line, std::string src_file){

#ifdef VALIDATE
	dr_printf("> A new ROI has been created!\n");
	dr_printf("> ROI detected label %s\n", label.c_str());
	dr_printf("> ROI detected %d line number \n", line);
	dr_printf("> ROI detected source file name '%s'\n",src_file.c_str());
#endif

	cur_point.reset();
	cur_point.set_label(label);
	cur_point.set_line_start(line);
	cur_point.set_src_file_start(src_file);
	return;

}




void ThreadData::save_to_file(file_t out_file){
    // Upon saving the different data points, if some of them have the same label,
    // make sure to output their name as label+ExecutionCount
    std::unordered_map<std::string, int> execution_count;
	for(std::list<Point>::iterator it = point_list.begin(); it != point_list.end(); it++){
        std::string point_label = it->get_label();
        auto search = execution_count.find(point_label);
        // If it's not the first time we see a given label, let's save it with its execution number 
        if(search != execution_count.end()){
            it->dump_info(out_file, point_label + std::to_string(search->second));
            execution_count[point_label] = execution_count[point_label] + 1;
        }
        // Else, let's keep track of this encounter
        else{
            execution_count[point_label] = 1;
            it->dump_info(out_file, point_label);
        }
        /// ELSLELELEEL
	}
	return;
}

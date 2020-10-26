#include"point.hpp"

Point::Point(){
	start = 0.0;
	end = 0.0;
	line_number_start=0;
	line_number_end=0;
	flops=0;
	bytes=0;
    write_bytes = 0;
    read_bytes = 0;

}


void Point::set_label(std::string roi_label){
	label = roi_label;
}


void Point::set_line_start(unsigned int src_line){
	line_number_start = src_line;
}

void Point::set_line_end(unsigned int src_line){
	line_number_end = src_line;
}


void Point::set_src_file_start(std::string src_file){
	src_file_start=src_file;
}


void Point::set_src_file_end(std::string src_file){
	src_file_end=src_file;
}


void Point::update_read_bytes(ushort bytes_accessed){
	read_bytes = read_bytes + (unsigned long long)bytes_accessed;
	return;
}

void Point::update_write_bytes(ushort bytes_accessed){
	write_bytes = write_bytes + (unsigned long long)bytes_accessed;
	return;
}


void Point::update_bytes(ushort bytes_accessed){
	bytes = bytes + (unsigned long long)bytes_accessed;
	return;
}

void Point::set_start(double time_start){
	start = time_start;
	return;
}


void Point::set_end(double time_end){
	end = time_end;
	return;
}


void Point::update_fp_count(int fp_count){
	flops = flops + (unsigned long long)fp_count;
	return;
}


std::string Point::get_label(void){
	return label;

}
void Point::reset(){
	start = 0.0;
	end = 0.0;
	label.clear();
	src_file_start.clear();
	src_file_end.clear();
	line_number_start=0;
	line_number_end=0;
	flops=0;
	bytes=0;

	return;

}

void Point::dump_info(file_t out_file, std::string actual_label){

#ifdef VALIDATE
	dr_printf("Executed FP operations are: %llu \n", flops);
	dr_printf("Accessed Bytes: %llu\n", bytes);
#endif

	dr_fprintf(out_file, "<point label=\"%s\">\n", actual_label.c_str());
	if(!time_run.get_value()){
		//This should be split between the threadData data structure and the point itself.
		dr_fprintf(out_file, "<flops>%llu</flops>\n", flops);
		dr_fprintf(out_file, "<bytes>%llu</bytes>\n", bytes);
		dr_fprintf(out_file, "<read_bytes>%llu</read_bytes>\n", read_bytes);
		dr_fprintf(out_file, "<write_bytes>%llu</write_bytes>\n", write_bytes);
		dr_fprintf(out_file, "<src_file_start>%s</src_file_start>\n", src_file_start.c_str());
		dr_fprintf(out_file, "<src_file_end>%s</src_file_end>\n", src_file_end.c_str());
		dr_fprintf(out_file, "<line_n_start>%u</line_n_start>\n", line_number_start);
		dr_fprintf(out_file, "<line_n_end>%u</line_n_end>\n",line_number_end);
	}
	else{
		double elapsed = end - start;
		dr_fprintf(out_file, "<time>%f</time>\n", elapsed);
#ifdef VALIDATE
		dr_printf("Start time is: %f\n",start);
		dr_printf("End time is: %f\n",end);
		dr_printf("Elapsed time is: %f\n", elapsed);
#endif
	}

	dr_fprintf(out_file, "</point>\n");
	return;
}

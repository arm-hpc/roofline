#ifndef POINT_DATA_H
#define POINT_DATA_H


#include<string>
#include"dr_api.h"
#include"droption.h"

using ::dynamorio::droption::droption_t;

extern droption_t<bool> time_run;

/* A Point is a simple representation for gathered performance data
 * for a specified (or detected) region of interest in the code.
 * This is called a 'Point' because this piece of information will actually
 * be drawn as a point in the Roofline plot
 * */


class Point{
	//TODO: Change to private
	public:
		Point();
		std::string label;
		std::string src_file_start;
		std::string src_file_end;
		unsigned int line_number_start;
		unsigned int line_number_end;
		unsigned long long flops;
		unsigned long long bytes;
		unsigned long long read_bytes;
		unsigned long long write_bytes;

		// Timing information
		double start;
		double end;

		//Setters
		void update_bytes(ushort bytes_accessed);
        void update_read_bytes(ushort bytes_accessed);
        void update_write_bytes(ushort bytes_accessed);
		void update_fp_count(int fp_count);
		void set_start(double time_start);
		void set_end(double time_end);
		void set_label(std::string label);
		void set_line_start(unsigned int src_file);
		void set_line_end(unsigned int src_file);
		void set_src_file_start(std::string src_file);
		void set_src_file_end(std::string src_file);

		// Getters
		std::string get_label(void);

		void reset();
        void dump_info(file_t out_file, std::string actual_label);

};



#endif

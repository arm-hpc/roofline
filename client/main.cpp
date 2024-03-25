/* ******************************************************************************
 * Copyright (c) 2011-2018 Google, Inc.  All rights reserved.
 * Copyright (c) 2010 Massachusetts Institute of Technology  All rights reserved.
 * ******************************************************************************/

/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of VMware, Inc. nor the names of its contributors may be
 *   used to endorse or promote products derived from this software without
 *   specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL VMWARE, INC. OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

// C++ libraries
#include <list>
#include "thread_data.hpp"
#include "point.hpp"
#include "count_fp.hpp"

// C libraries
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h> /* for offsetof */
#include <sys/time.h> /* for timing information*/
#include <inttypes.h> /* for printing uint64_t properly*/


// DynamoRIO Dependencies
#include "dr_api.h"
#include "drmgr.h"
#include "drreg.h"
#include "drutil.h"
#include "drwrap.h"
#include "drsyms.h"
#include "droption.h"

using ::dynamorio::droption::droption_parser_t;
using ::dynamorio::droption::DROPTION_SCOPE_ALL;
using ::dynamorio::droption::DROPTION_SCOPE_FRONTEND;
using ::dynamorio::droption::DROPTION_SCOPE_CLIENT;
using ::dynamorio::droption::droption_t;


static client_id_t client_id;

reg_id_t tls_seg;
uint tls_offs;
int tls_idx;

// Region of interest data structures.


static bool in_roi = false;
static int roi_start_detected = 0;
static int roi_end_detected = 0;

typedef struct{
	std::string f_name;
	void (*f_pre)(void* wrapcxt, DR_PARAM_OUT void **user_data);
	void (*f_post)(void* wrapcxt, DR_PARAM_OUT void *user_data);
} wrap_callback_t;




// Get current time.
// The function implementation the same as the ERT one.
double get_time(){
	struct timeval tm;
	gettimeofday(&tm, NULL);
	return tm.tv_sec + (tm.tv_usec / 1000000.0);
}


droption_t<bool> read_bytes_only(
		DROPTION_SCOPE_CLIENT, "read_bytes_only", false,
		"Take into account only loaded bytes",
		"Take into account only loaded bytes"
		);

droption_t<bool> write_bytes_only(
		DROPTION_SCOPE_CLIENT, "write_bytes_only", false,
		"Take into account only stored bytes",
		"Take into account only stored bytes"
		);


droption_t<bool> time_run(
		DROPTION_SCOPE_CLIENT, "time_run", false,
		"Run the target application to gather timinng information only,",
		"Run the target application to gather timinng information only,"
		);


static droption_t<std::string> output_folder(
		DROPTION_SCOPE_CLIENT, "output_folder", ".",
		"Output folder in which it will be stored what is traced by the tool",
		"Output folder in which it will be stored what is traced by the tool");


static droption_t<std::string> roi_start(
		DROPTION_SCOPE_CLIENT, "roi_start", "",
		"Specify a function name inside the target application which will be treated as demarker for the Region of Interest",
		"Specify a function name inside the target application which will be treated as demarker for the Region of Interest");


static droption_t<std::string> roi_end(
		DROPTION_SCOPE_CLIENT, "roi_end", "",
		"Specify a function name inside the target application which will be treated as demarker for the Region of Interest",
		"Specify a function name inside the target application which will be treated as demarker for the Region of Interest");


static droption_t<std::string> trace_f(
		DROPTION_SCOPE_CLIENT, "trace_f", "",
		"Trace the execution of the given function name. (Inlined functions not supported)\n",
		"Trace the execution of the given function name. (Inlined functions not supported)\n");


static droption_t<int> up_to_call(
		DROPTION_SCOPE_CLIENT, "up_to_call", 0,
		"Trace function execution up to the specified call number.\n Default value is 0 - Trace all function calls",
		"Trace function execution up to the specified call number.\n Default value is 0 - Trace all function calls");


static droption_t<bool> calls_as_separate_roi(
		DROPTION_SCOPE_CLIENT, "calls_as_separate_roi", false,
		"Take into account each function call as a separate ROI\n Default value is false",
		"Take into account each function call as a separate ROI\n Default value is false");



// get_label_and_assign_id retrieves the label for the given function, and appends a unique identifier representing the n_th
// time the traced function is being executed.
// get_label_and_assign_id is supposed to work when --trace_f <Funcion Name> is specified
static std::string get_label_and_assign_id(void *wrapcxt, droption_t<std::string> user_defined_delimiter){
	DR_ASSERT_MSG(user_defined_delimiter.get_value() != "", "> ERROR: Function name is unspecified when using --trace_f\n");

	std::string label;
	static int id = 1;
	static std::string last_seen_delimiter;
	label = user_defined_delimiter.get_value() + std::to_string(id);
#ifdef VALIDATE
	dr_printf("Getting Label %s\n", label.c_str());
#endif
	// Increment the id for the next timE
	if(last_seen_delimiter == label){
		id++;
	}
	else
		last_seen_delimiter = label;
	return label;
}


static std::string get_label(void *wrapcxt, droption_t<std::string> user_defined_delimiter){
	std::string label;
	if (user_defined_delimiter.get_value() != "")
		label = user_defined_delimiter.get_value();
	else
		label = std::string((const char*) drwrap_get_arg(wrapcxt,0));
#ifdef VALIDATE
	dr_printf("Getting Label %s\n", label.c_str());
#endif
	return label;
}

static unsigned int get_line_n(void *wrapcxt, droption_t<std::string> user_defined_delimiter){
	unsigned int line_n;
	if (user_defined_delimiter.get_value() != "")
		line_n = 0; 
	else
		line_n = (unsigned int)(ptr_int_t) drwrap_get_arg(wrapcxt,1);
	return line_n;
}

static std::string get_src_file_name(void *wrapcxt, droption_t<std::string> user_defined_delimiter){
	std::string name;
	if (user_defined_delimiter.get_value() != "")
		name = "";
	else
		return std::string((const char*) drwrap_get_arg(wrapcxt,2));
	return name;
}


static void event_roi_init(void *wrapcxt, DR_PARAM_OUT void**user_data){
#ifdef VALIDATE
	dr_printf(">> ROI Start <<\n");
#endif
	roi_start_detected++;
	in_roi = true;

	ThreadData *data = reinterpret_cast<ThreadData*>(drmgr_get_tls_field(drwrap_get_drcontext(wrapcxt), tls_idx));
	// Initialize current Point
	if(calls_as_separate_roi.get_value() == false && trace_f.get_value() != ""){
		//Since all function complete executions are merged into a single ROI,
		//We initialize a point only the very first time.
		static bool new_point_only_once=false;
		if(new_point_only_once == false){
			data->new_point(get_label(wrapcxt, trace_f),
					get_line_n(wrapcxt, trace_f),
					get_src_file_name(wrapcxt, trace_f));
			new_point_only_once = true;
		}
	
	}
	else if(calls_as_separate_roi.get_value() == true){
		data->new_point(get_label_and_assign_id(wrapcxt, trace_f),
				get_line_n(wrapcxt, trace_f),
				get_src_file_name(wrapcxt, trace_f));

	}
	else{
		data->new_point(get_label(wrapcxt, roi_start),
				get_line_n(wrapcxt, roi_start),
				get_src_file_name(wrapcxt, roi_start));
	}
	if(time_run.get_value()){
		data->set_time_start(get_time());
	}
}


// This function is almost equivalent to event_roi_end but it's wrapped in a post function execution scenario
static void symbol_roi_end(void *wrapcxt, DR_PARAM_OUT void *user_data){

#ifdef VALIDATE
	dr_printf(">> Symbol ROI End <<\n");
#endif

	in_roi = false;
	roi_end_detected++;

	ThreadData *data = reinterpret_cast<ThreadData*>(drmgr_get_tls_field(drwrap_get_drcontext(wrapcxt), tls_idx));
	if(time_run.get_value()){
		data->set_time_end(get_time());
#ifdef VALIDATE
		dr_printf(">> Gathering timing information STOP <<\n");
#endif
	}

	if(trace_f.get_value() == ""){
		data->save_point(get_label(wrapcxt,roi_end),
				get_line_n(wrapcxt,roi_end),
				get_src_file_name(wrapcxt,roi_end));

	}
	else if(calls_as_separate_roi.get_value() == true){
		data->save_point(get_label_and_assign_id(wrapcxt, trace_f),
				get_line_n(wrapcxt, trace_f),
				get_src_file_name(wrapcxt, trace_f));
	}

}




static void event_roi_end(void *wrapcxt, DR_PARAM_OUT void **user_data){

#ifdef VALIDATE
	dr_printf(">> ROI End <<\n");
#endif

	in_roi = false;
	roi_end_detected++;

	ThreadData *data = reinterpret_cast<ThreadData*>(drmgr_get_tls_field(drwrap_get_drcontext(wrapcxt), tls_idx));
	if(time_run.get_value()){
		data->set_time_end(get_time());
#ifdef VALIDATE
		dr_printf(">> Gathering timing information STOP <<\n");
#endif
	}

	if(trace_f.get_value() == ""){
		// Just take the label as the starting function name
		data->save_point(get_label(wrapcxt,roi_start),
				get_line_n(wrapcxt,roi_end),
				get_src_file_name(wrapcxt,roi_end));

	}
	else if(calls_as_separate_roi.get_value() == true){
		data->save_point(get_label(wrapcxt, trace_f),
				get_line_n(wrapcxt, trace_f),
				get_src_file_name(wrapcxt, trace_f));
	}

}



#ifdef VALIDATE
static file_t modules_f;
file_t debug_file;
file_t disassemble_file;
#endif
file_t out_file;


#define TLS_SLOT(tls_base, enum_val) (void **)((byte *)(tls_base) + tls_offs + (enum_val))
#define BUF_PTR(tls_base) *(mem_ref_t **)TLS_SLOT(tls_base, MEMTRACE_TLS_OFFS_BUF_PTR)

#define MINSERT instrlist_meta_preinsert


#ifdef VALIDATE_VERBOSE
static void clean_call(int fp_instr_count, uint64_t address){
#else
static void clean_call(int fp_instr_count){
#endif
    // Make the memory reference buffer empty no matter what.
    // IF we are in ROI, save the partial result.

#ifdef VALIDATE_VERBOSE

    if(in_roi){
	dr_printf("Clean call on @ " PFX "\n", address);
    }


    static int times=0;
    if(fp_instr_count > 0){
	    dr_printf("Called with %d Floating point operations\n", fp_instr_count);
	    times++;
	    dr_printf("Clean Call has been performed for %d times\n", times);
    }

#endif

    void *drcontext = dr_get_current_drcontext();
    ThreadData *data = reinterpret_cast<ThreadData*>(drmgr_get_tls_field(drcontext, tls_idx));
    //TODO: wrap it in a validate.
    DR_ASSERT_MSG(data != NULL, ">>> DynamoRIO Client ERROR: Failed initialization for per thread class\n");

    // If in ROI, update the floating point value
    if(in_roi){
	    data->save_floating_points(fp_instr_count);
	    data->save_bytes();
    }
    else{
	    data->clean_buffer();
    }
    return;
}

static void
insert_load_buf_ptr(void *drcontext, instrlist_t *ilist, instr_t *where, reg_id_t reg_ptr)
{
    dr_insert_read_raw_tls(drcontext, ilist, where, tls_seg,
                           tls_offs + MEMTRACE_TLS_OFFS_BUF_PTR, reg_ptr);
}

static void
insert_update_buf_ptr(void *drcontext, instrlist_t *ilist, instr_t *where,
                      reg_id_t reg_ptr, int adjust)
{
    MINSERT(
        ilist, where,
        XINST_CREATE_add(drcontext, opnd_create_reg(reg_ptr), OPND_CREATE_INT16(adjust)));
    dr_insert_write_raw_tls(drcontext, ilist, where, tls_seg,
                            tls_offs + MEMTRACE_TLS_OFFS_BUF_PTR, reg_ptr);
}

static void
insert_save_type(void *drcontext, instrlist_t *ilist, instr_t *where, reg_id_t base,
                 reg_id_t scratch, ushort type)
{
    scratch = reg_resize_to_opsz(scratch, OPSZ_2);
    MINSERT(ilist, where,
            XINST_CREATE_load_int(drcontext, opnd_create_reg(scratch),
                                  OPND_CREATE_INT16(type)));
    MINSERT(ilist, where,
            XINST_CREATE_store_2bytes(drcontext,
                                      OPND_CREATE_MEM16(base, offsetof(mem_ref_t, type)),
                                      opnd_create_reg(scratch)));
}

static void
insert_save_size(void *drcontext, instrlist_t *ilist, instr_t *where, reg_id_t base,
                 reg_id_t scratch, ushort size)
{
    scratch = reg_resize_to_opsz(scratch, OPSZ_2);
    MINSERT(ilist, where,
            XINST_CREATE_load_int(drcontext, opnd_create_reg(scratch),
                                  OPND_CREATE_INT16(size)));
    MINSERT(ilist, where,
            XINST_CREATE_store_2bytes(drcontext,
                                      OPND_CREATE_MEM16(base, offsetof(mem_ref_t, size)),
                                      opnd_create_reg(scratch)));
}

#ifdef VALIDATE_VERBOSE
static void
insert_save_pc(void *drcontext, instrlist_t *ilist, instr_t *where, reg_id_t base,
               reg_id_t scratch, app_pc pc)
{
    instrlist_insert_mov_immed_ptrsz(drcontext, (ptr_int_t)pc, opnd_create_reg(scratch),
                                     ilist, where, NULL, NULL);
    MINSERT(ilist, where,
            XINST_CREATE_store(drcontext,
                               OPND_CREATE_MEMPTR(base, offsetof(mem_ref_t, addr)),
                               opnd_create_reg(scratch)));
}
#endif


/* insert inline code to add an instruction entry into the buffer */
static void
instrument_instr(void *drcontext, instrlist_t *ilist, instr_t *where)
{
    /* We need two scratch registers */
    reg_id_t reg_ptr, reg_tmp;
    /* we don't want to predicate this, because an instruction fetch always occurs */
    instrlist_set_auto_predicate(ilist, DR_PRED_NONE);
    if (drreg_reserve_register(drcontext, ilist, where, NULL, &reg_ptr) !=
            DRREG_SUCCESS ||
        drreg_reserve_register(drcontext, ilist, where, NULL, &reg_tmp) !=
            DRREG_SUCCESS) {
        DR_ASSERT(false); /* cannot recover */
        return;
    }
    insert_load_buf_ptr(drcontext, ilist, where, reg_ptr);
    insert_save_size(drcontext, ilist, where, reg_ptr, reg_tmp, (ushort)instr_memory_reference_size(where));
    //Save type for 
    // 0 --> Instruction will READ memory
    // 1 --> Instruction will WRITE memory
    ushort instr_kind;
    if(instr_reads_memory(where))
        instr_kind = 0;
    else if(instr_writes_memory(where))
        instr_kind = 1;
    insert_save_type(drcontext, ilist, where, reg_ptr, reg_tmp, instr_kind);
#ifdef VALIDATE_VERBOSE
    insert_save_pc(drcontext, ilist, where, reg_ptr, reg_tmp, instr_get_app_pc(where));
#endif
    insert_update_buf_ptr(drcontext, ilist, where, reg_ptr, sizeof(mem_ref_t));
    /* Restore scratch registers */
    if (drreg_unreserve_register(drcontext, ilist, where, reg_ptr) != DRREG_SUCCESS ||
        drreg_unreserve_register(drcontext, ilist, where, reg_tmp) != DRREG_SUCCESS)
        DR_ASSERT(false);
    instrlist_set_auto_predicate(ilist, instr_get_predicate(where));
}



/* For each memory reference app instr, we insert inline code to fill the buffer
 * with an instruction entry and memory reference entries.
 */
static dr_emit_flags_t
event_app_instruction(void *drcontext, void *tag, instrlist_t *bb, instr_t *instr,
                      bool for_trace, bool translating, void *user_data)
{

    drmgr_disable_auto_predication(drcontext, bb);

    // Instrument the target application
    if(instr_is_app(instr)){
	    if(read_bytes_only.get_value() == true){
		    if(instr_reads_memory(instr))
			    instrument_instr(drcontext, bb, instr);
	    }
	    else if(write_bytes_only.get_value() == true){
		    if(instr_writes_memory(instr))
			    instrument_instr(drcontext, bb, instr);
	    }
	    else if(instr_reads_memory(instr) || instr_writes_memory(instr)){
		    /* insert code to add an entry for app instruction */
		    instrument_instr(drcontext, bb, instr);
	    }


	    // We insturment a clean call only for the very first instruction within the basic block.
	    // In a super optimized version, you may want to try some approach to avoid the clean call.
	    if(drmgr_is_first_instr(drcontext, instr)){

		    // Compute the number of floating point instructions in this basic block
		    uint32_t fp_instr_count = 0;
		    instr_t *instr_it;
		    for(instr_it = instrlist_first_app(bb); instr_it != nullptr; instr_it = instr_get_next_app(instr_it)){
			    fp_instr_count = fp_instr_count + count_fp_instr(instr_it);
		    }

#ifdef VALIDATE_VERBOSE
		    if (fp_instr_count > 0){
			    dr_fprintf(debug_file, "Number of FP Instructions detected: %d\n", fp_instr_count);
		    }
		    uint64_t address = reinterpret_cast<uint64_t>(tag);
#endif
		    /* Insert code to call clean_call for processing the buffer
		     * In this way what you get is that the instrumented basic block will perform a clean call
		     * at runtime, whose argument is its number of floating point instructions
		     * that are going to be executed
		     */
		    // For the time being I want to be conservative and only take into account in_roi at runtime.
		    if (IF_AARCHXX_ELSE(!instr_is_exclusive_store(instr), true))
#ifdef VALIDATE_VERBOSE
			    dr_insert_clean_call(drcontext, bb, instr, (void *)clean_call, false, 2, OPND_CREATE_INT32(fp_instr_count), OPND_CREATE_INT64(address));
#else
			    dr_insert_clean_call(drcontext, bb, instr, (void *)clean_call, false, 1, OPND_CREATE_INT32(fp_instr_count));
#endif
	    }

#ifdef VALIDATE_VERBOSE
		    instrlist_disassemble(drcontext, (app_pc)tag, bb, disassemble_file);
		    dr_flush_file(disassemble_file);
#endif


    }
    return DR_EMIT_DEFAULT;
}


// Registers the given functions.
void trace_symbol(std::vector<wrap_callback_t> symbols, const module_data_t *mod){

	bool wrap_result;
	for(std::vector<wrap_callback_t>::iterator it = symbols.begin(); it != symbols.end(); ++it){
		size_t modoffs = 0;

		drsym_error_t symres = DRSYM_ERROR;
		symres = drsym_lookup_symbol(mod->full_path, it->f_name.c_str(), &modoffs, DRSYM_DEMANGLE);

		if(symres == DRSYM_SUCCESS){
			app_pc startup_wrap = modoffs + mod->start;
			dr_printf("<wrapping %s @" PFX "\n", it->f_name.c_str(), startup_wrap);
			wrap_result = drwrap_wrap(startup_wrap, it->f_pre, it->f_post);
			DR_ASSERT_MSG(wrap_result, ">DR Roofline Client ERROR: Couldn't use specified function as a ROI delimiter\n");
		}
	}
	return;
}

// Detect Region of Interest Functions
//TODO: Wrap this function code in other functions.
static void module_load_event(void *drcontext, const module_data_t *mod, bool loaded){
#ifdef VALIDATE
	static bool first_time = true;
	if(first_time){
		modules_f = dr_open_file("loaded.modules", DR_FILE_WRITE_OVERWRITE);
		first_time = false;
	}
	dr_fprintf(modules_f, "<loading %s @" PFX "\n", mod->full_path, mod->start);
#endif

	if(trace_f.get_value() != ""){
#ifdef VALIDATE
		dr_printf("> Roofline is going to trace function %s\n", trace_f.get_value().c_str());
#endif
		std::vector<wrap_callback_t> roi_f = {
			{
				.f_name=trace_f.get_value().c_str(),
				.f_pre=event_roi_init,
				.f_post=symbol_roi_end
			}
		}; 
		trace_symbol(roi_f, mod);
	}
	
	else{
		std::vector<wrap_callback_t> start_stop_roi_f = {
			{.f_name=roi_start.get_value() == "" ? "_RoiStart" : roi_start.get_value(), .f_pre=event_roi_init, .f_post=NULL},
			{.f_name=roi_end.get_value() == "" ? "_RoiEnd": roi_end.get_value(), .f_pre=event_roi_end, .f_post=NULL}
		};


		trace_symbol(start_stop_roi_f, mod);
	}
}






/* We transform string loops into regular loops so we can more easily
 * monitor every memory reference they make.
 */
static dr_emit_flags_t
event_bb_app2app(void *drcontext, void *tag, instrlist_t *bb, bool for_trace,
                 bool translating)
{
    if (!drutil_expand_rep_string(drcontext, bb)) {
        DR_ASSERT(false);
    }
    return DR_EMIT_DEFAULT;
}

static void
event_thread_init(void *drcontext)
{
    ThreadData *data = reinterpret_cast<ThreadData*>(dr_thread_alloc(drcontext, sizeof(data)));
    data = new ThreadData{dr_get_thread_id(drcontext)};
    //TODO: Remember to deallocate this.
    DR_ASSERT_MSG(data != NULL, ">>> DynamoRIO Client ERROR: Failed initialization for per thread class\n");
    //TODO: Andrea Is it ok to have vvv here?
    drmgr_set_tls_field(drcontext, tls_idx, data);

}

static void
event_thread_exit(void *drcontext){
    ThreadData *data = reinterpret_cast<ThreadData*>(drmgr_get_tls_field(drcontext, tls_idx));

    // If we have been tracing a multiple functions executions as a single ROI,
    // not it is the time to save this a single point.
    if(calls_as_separate_roi.get_value() == false && trace_f.get_value() != ""){
	    data->save_point(trace_f.get_value(), 0, "");
    }
    //TODO: There should be a post execution function that iterates over all the dead thread data structures.
    //TODO: The assumption, for now, is that you have one thread only.
    dr_fprintf(out_file, "<?xml version=\"1.0\"?>\n");
    dr_fprintf(out_file, "<roofline>\n");
    data->save_to_file(out_file);
    dr_fprintf(out_file, "</roofline>\n");

#ifdef VALIDATE
    dr_printf("> Deallocating Thread Data\n");
#endif
    // TODO: Properly deallocate everything.
    //Deallocate the pointer which we have deallocated upon thread initialization
    dr_raw_mem_free(data->buf_base, MEM_BUF_SIZE);
    delete data;
    dr_thread_free(drcontext, data, sizeof(data));
#ifdef VALIDATE
    dr_printf("> Deallocated Thread Data\n");
#endif
}

static void event_exit(void)
{
    if (!dr_raw_tls_cfree(tls_offs, MEMTRACE_TLS_COUNT))
        DR_ASSERT(false);

    if(time_run.get_value()){
	    if(!drmgr_unregister_module_load_event(module_load_event) ||
	       !drmgr_unregister_thread_init_event(event_thread_init) ||
	       !drmgr_unregister_thread_exit_event(event_thread_exit)){
		    DR_ASSERT_MSG(false, "ERROR: Couldn't unsubscribe module_load_event");
	    }
    }

    else{
	    if (!drmgr_unregister_thread_init_event(event_thread_init) ||
	        !drmgr_unregister_tls_field(tls_idx) ||
	        !drmgr_unregister_module_load_event(module_load_event) ||
	        !drmgr_unregister_thread_exit_event(event_thread_exit) ||
		!drmgr_unregister_bb_app2app_event(event_bb_app2app) ||
	        !drmgr_unregister_bb_insertion_event(event_app_instruction))
	    DR_ASSERT_MSG(false, "ERROR: Couldn't perform event unsubscription");
    }

    DR_ASSERT_MSG(roi_start_detected > 0,
		    "> ERROR: Roi Start function has not be detected. Please check that you've written the right name and that the compiler has not inlined it\n");
    DR_ASSERT_MSG(roi_end_detected > 0 , 
		    "> ERROR: Roi End function has not be detected. Please check that you've written the right name and that the compiler has not inlined it\n");
    DR_ASSERT_MSG(roi_start_detected == roi_end_detected , 
		    "> ERROR: Uneven detection for ROI Start and Stop functions\n");

    if(drreg_exit() != DRREG_SUCCESS)
        DR_ASSERT(false);

#ifdef VALIDATE
    dr_close_file(modules_f);
    dr_close_file(debug_file);
    dr_close_file(disassemble_file);
#endif
    dr_close_file(out_file);
    drwrap_exit();
    drutil_exit();
    drmgr_exit();
    drsym_exit();
}

DR_EXPORT void
dr_client_main(client_id_t id, int argc, const char *argv[])
{
    /* We need 2 reg slots beyond drreg's eflags slots => 3 slots */
    drreg_options_t ops = { sizeof(ops), 3, false };
    dr_set_client_name("DynamoRIO Sample Client 'Roofline'","http://dynamorio.org/issues");
    dr_log(NULL, DR_LOG_ALL, 1, "Roofline Initializing\n");


    if(!droption_parser_t::parse_argv(DROPTION_SCOPE_CLIENT, argc, argv, NULL, NULL))
	    DR_ASSERT_MSG(false, "Flag options couldn't be parsed correctly\n");
    
    if(roi_start.get_value() != "" || roi_end.get_value() != ""){
	    DR_ASSERT_MSG(roi_start.get_value() != "", "> ERROR: roi_start has not been specified.\n");
	    DR_ASSERT_MSG(roi_end.get_value() != "", "> ERROR: roi_end has not been specified.\n");
	    DR_ASSERT_MSG(trace_f.get_value() == "", "> ERROR: Please specify either roi_start and roi_end function or trace_f\n");
    }
    if(trace_f.get_value() != ""){
	    DR_ASSERT_MSG(roi_start.get_value() == "", "> ERROR: Please specify either roi_start and roi_end function or trace_f\n");
	    DR_ASSERT_MSG(roi_end.get_value() == "",  "> ERROR: Please specify either roi_start and roi_end function or trace_f\n");
    }

    if (!drmgr_init() || drreg_init(&ops) != DRREG_SUCCESS || !drutil_init() || !drwrap_init())
        DR_ASSERT(false);
    drsym_init(0);

    /* register events */
    dr_register_exit_event(event_exit);

    if(time_run.get_value()){
	    dr_printf("> Roofline is running for gathering timining information\n");
	    if(!drmgr_register_module_load_event(module_load_event) ||
	       !drmgr_register_thread_init_event(event_thread_init) ||
	       !drmgr_register_thread_exit_event(event_thread_exit)){
		    DR_ASSERT_MSG(false, "ERROR: Timing Run - Couldn't perform event subscription\n");
	    }
    }
    else{
	    dr_printf("> Roofline is running to get FP and Bytes accessed.\n");
	    if (!drmgr_register_thread_init_event(event_thread_init) ||
	        !drmgr_register_module_load_event(module_load_event) ||
		!drmgr_register_thread_exit_event(event_thread_exit) ||
		!drmgr_register_bb_app2app_event(event_bb_app2app, NULL) ||
		!drmgr_register_bb_instrumentation_event(NULL /*analysis_func*/,
				    event_app_instruction, NULL))
	    DR_ASSERT_MSG(false, "ERROR: Couldn't perform event subscription\n");
	    if(read_bytes_only.get_value() == true)
		    dr_printf("> Roofline: Detecting Read Bytes only as requested\n");
	    if(write_bytes_only.get_value() == true)
		    dr_printf("> Roofline: Detecting Written Bytes only as requested\n");
    }


    client_id = id;

    tls_idx = drmgr_register_tls_field();
    DR_ASSERT(tls_idx != -1);
    /* The TLS field provided by DR cannot be directly accessed from the code cache.
     * For better performance, we allocate raw TLS so that we can directly
     * access and update it with a single instruction.
     */
    if (!dr_raw_tls_calloc(&tls_seg, &tls_offs, MEMTRACE_TLS_COUNT, 0))
        DR_ASSERT(false);

    /* make it easy to tell, by looking at log file, which client executed */
    dr_log(NULL, DR_LOG_ALL, 1, "Client 'Roofline' initializing\n");

#ifdef VALIDATE
    debug_file = dr_open_file("roofline.log", DR_FILE_WRITE_OVERWRITE);
    disassemble_file = dr_open_file("roofline.disassemble", DR_FILE_WRITE_OVERWRITE);
#endif


    if(time_run.get_value()){
	    std::string file_name = "/roofline_time.xml";
	    std::string output_file = output_folder.get_value() + file_name; 
	    out_file = dr_open_file(output_file.c_str(), DR_FILE_WRITE_OVERWRITE);
    }
    else{
	    std::string file_name = "/roofline.xml";
	    std::string output_file = output_folder.get_value() + file_name; 
	    out_file = dr_open_file(output_file.c_str(), DR_FILE_WRITE_OVERWRITE);
    }
}

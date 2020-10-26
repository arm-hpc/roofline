#ifndef THREAD_DATA_H
#define THREAD_DATA_H


#include "dr_api.h"
#include "point.hpp"
#include <list>
#include <unordered_map>

/* Allocated TLS slot offsets */
enum {
    MEMTRACE_TLS_OFFS_BUF_PTR,
    MEMTRACE_TLS_COUNT, /* total number of TLS slots allocated */
};


/* Each mem_ref_t is a <type, size, addr> entry representing a memory reference
 * instruction or the reference information, e.g.:
 * - mem ref instr: { type = 42 (call), size = 5, addr = 0x7f59c2d002d3 }
 * - mem ref info:  { type = 1 (write), size = 8, addr = 0x7ffeacab0ec8 }
 *   Andrea: For the time being I'm keeping this, since it may be useful for debugging.
 */
typedef struct _mem_ref_t {
    ushort size; /* mem ref size or instr length */
    ushort type; /* r(0), w(1), or opcode (assuming 0/1 are invalid opcode) */
#ifdef VALIDATE_VERBOSE
    app_pc addr; /* mem ref addr or instr pc */
#endif
} mem_ref_t;

extern reg_id_t tls_seg;
extern uint tls_offs;

/* Max number of mem_ref a buffer can have. It should be big enough
 * to hold all entries between clean calls.
 */
#define MAX_NUM_MEM_REFS 4096
/* The maximum size of buffer for holding mem_refs. */
#define MEM_BUF_SIZE (sizeof(mem_ref_t) * MAX_NUM_MEM_REFS)


#define TLS_SLOT(tls_base, enum_val) (void **)((byte *)(tls_base) + tls_offs + (enum_val))

#define BUF_PTR(tls_base) *(mem_ref_t **)TLS_SLOT(tls_base, MEMTRACE_TLS_OFFS_BUF_PTR)



class ThreadData{
public:
   /* We store an array of points, effectively providing the capability of tracing
    * * differents parts in the code.
    * These different points will be then plotted together in the same plot
    * for a better compariso
    * * */
  std::list<Point> point_list;

  unsigned int tid; // Thread id

  ThreadData(int thread_id); // Constructor

  void save_bytes(void);
  void save_floating_points(int fp_count);
  void set_time_start(double start_time);
  void set_time_end(double end_time);
  void new_point(std::string label, unsigned int line, std::string src_file);
  void save_point(std::string label, unsigned int line, std::string src_file);
  void clean_buffer(void);
  void save_to_file(file_t out_file);


  //TODO: Put back to private
  mem_ref_t *buf_base;

private:
  // Status for the current point
  Point cur_point;
  // Memory buffer containig those instructions which have not yet been
  // fed to the treap.
  byte *seg_base;

};


#endif

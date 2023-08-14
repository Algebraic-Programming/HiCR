#include <cstring>
#include <cstdio>
#include <iostream>

#include <hicr.hpp>
#include "lpf/core.h"

#define MAX_MEM_SLOTS 20

using namespace HiCR;


class LPFMemorySlot : MemorySlot {

    private:
        lpf_memslot_t _lpf_slot;

    public:
        LPFMemorySlot(lpf_memslot_t lpf_slot) : _lpf_slot(lpf_slot) {
        }
};

void memcpy(
        LPFMemorySlot & destination, const size_t dst_locality, const size_t dst_offset,
        const LPFMemorySlot & source, const size_t src_locality, const size_t src_offset,
        const size_t size,
        const TagSlot& tag
        )  {
    std::cout << "Enter memcpy with 2 LPF memory locations\n";
}

class LPFMemoryResource : MemoryResource {
    private:
        lpf_t _ctx;
        std::vector<lpf_memslot_t> _lpf_slots;
    public:
        LPFMemoryResource(lpf_t ctx) :  _ctx(ctx) {
            auto rc = lpf_resize_memory_register( _ctx, MAX_MEM_SLOTS);
            assert (rc == LPF_SUCCESS);
        }

        LPFMemorySlot * allocateMemorySlot(const size_t size) {
            void * buff = malloc(size);
            lpf_memslot_t * lpf_slot = new lpf_memslot_t();
            // initialize ctx first!
            std::cout << "Before register local\n";
            auto rc = lpf_register_local(_ctx, buff, size, lpf_slot);
            std::cout << "After register local with rc = " << rc << std::endl;
            _lpf_slots.push_back(*lpf_slot);
            if (rc != LPF_SUCCESS) {
                std::cerr << "lpf_register_local failed\n";
                exit(-1);
            }
            LPFMemorySlot * slot = new LPFMemorySlot(*lpf_slot);
            std::cout << "After LPFMemorySlot constructor\n";
            return slot;
        }

};


void spmd( lpf_t ctx, lpf_pid_t pid, lpf_pid_t nprocs, lpf_args_t args) {

    LPFMemoryResource lpf(ctx);
    auto slot1 = lpf.allocateMemorySlot(1000);
    auto slot2 = lpf.allocateMemorySlot(1000);
    //LPFBackend lpf_back; 
    memcpy(*slot2, 0, 0, *slot1, 0, 0, 1000, 0);

}

int main( int argc, char** argv )
{


    lpf_err_t rc = lpf_exec( LPF_ROOT, LPF_MAX_P, spmd, LPF_NO_ARGS);
    assert(rc == LPF_SUCCESS);


 return 0;
}

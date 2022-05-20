#include "transaction_generator.h"

namespace dramsim3 {

void TransactionGenerator::ReadCallBack(uint64_t addr) {
    return;
}
void TransactionGenerator::WriteCallBack(uint64_t addr) {
    return;
}

// Map 64-bit hex_address into structured address
uint64_t TransactionGenerator::ReverseAddressMapping(Address& addr) {
    uint64_t hex_addr = 0;
    hex_addr += ((uint64_t)addr.channel) << config_->ch_pos;
    hex_addr += ((uint64_t)addr.rank) << config_->ra_pos;
    hex_addr += ((uint64_t)addr.bankgroup) << config_->bg_pos;
    hex_addr += ((uint64_t)addr.bank) << config_->ba_pos;
    hex_addr += ((uint64_t)addr.row) << config_->ro_pos;
    hex_addr += ((uint64_t)addr.column) << config_->co_pos;
    return hex_addr << config_->shift_bits;
}

// Returns the minimum multiple of stride that is higher than num
uint64_t TransactionGenerator::Ceiling(uint64_t num, uint64_t stride) {
    return ((num + stride - 1) / stride) * stride;
}

uint64_t TransactionGenerator::Flooring(uint64_t num, uint64_t stride) {
    return num - num % stride;
}

// Send transaction to memory_system (DRAMsim3 + PIM Functional Simulator)
//  hex_addr : address to RD/WR from physical memory or change bank mode
//  is_write : denotes to Read or Write
//  *DataPtr : buffer used for both RD/WR transaction (read common.h)
void TransactionGenerator::TryAddTransaction(uint64_t hex_addr, bool is_write) {
    // Wait until memory_system is ready to get Transaction
    while (!memory_system_.WillAcceptTransaction(hex_addr, is_write)) {
        memory_system_.ClockTick();
        clk_++;
    }
    // Send transaction to memory_system
    memory_system_.AddTransaction(hex_addr, is_write);
    num_trans_ += 1;
    memory_system_.ClockTick();
    clk_++;
}

// Prevent turning out of order between transaction parts
//  Change memory's threshold and wait until all pending transactions are
//  executed
void TransactionGenerator::Barrier() {
    //return;
    memory_system_.SetWriteBufferThreshold(0);
    while (memory_system_.IsPendingTransaction()) {
        memory_system_.ClockTick();
        clk_++;
    }
    memory_system_.SetWriteBufferThreshold(-1);
}

// Initialize variables and ukernel
void GemvTransactionGenerator::Initialize() {
}

// Write operand data and μkernel to physical memory and PIM registers
void GemvTransactionGenerator::SetData() {
}

// Execute PIM computation
void GemvTransactionGenerator::Execute() {
    uint64_t idx = 0;

    Address sbmr_addr(0, 0, 0, 0, MAP_SBMR, 0);
    uint64_t hex_sbmr_addr = ReverseAddressMapping(sbmr_addr);
    Address abmr_addr(0, 0, 0, 0, MAP_ABMR, 0);
    uint64_t hex_abmr_addr = ReverseAddressMapping(abmr_addr);
    Address pim_op_addr(0, 0, 0, 0, MAP_PIM_OP_MODE, 0);
    uint64_t hex_pim_op_addr = ReverseAddressMapping(pim_op_addr);

    if (is_dataflow_opt_ && ri_ * ko_ > ro_ * ki_) {
        is_output_stationary_ = false;
    } else {
        is_output_stationary_ = true;
    }

    uint64_t prev_x = -1;
    uint64_t prev_y = -1;
    bool is_dirty = false;

    TryAddTransaction(hex_abmr_addr, false);
    for (int xoo = 0; xoo < xoo_; xoo++) {
        for (int yoo = 0; yoo < yoo_; yoo++) {
            for (int xoi = 0; xoi < xoi_; xoi++) {
                for (int yoi = 0; yoi < yoi_; yoi++) {
                    uint64_t x = xoo * xoi_ * ki_ + xoi * ki_;
                    uint64_t y = yoo * yoi_ * ko_ + yoi * ko_;

                    if (!is_register_reuse_ || (is_register_reuse_ && x != prev_x)) {
                        // Read output register
                        if (is_dirty) {
                            TryAddTransaction(hex_sbmr_addr, false);
                            for (int y_idx = y % ro_; y_idx < y % ro_ + ko_; y_idx++) {
                                Address addr(0, 0, 0, 0, MAP_GRF, 8+y_idx);
                                uint64_t hex_addr = ReverseAddressMapping(addr);
                                TryAddTransaction(hex_addr, false);
                            }
                            Barrier();
                            TryAddTransaction(hex_abmr_addr, false);
                        }

                        // Initialize output register
                        prev_x = x;
                        for (int y_idx = y % ro_; y_idx < y % ro_ + ko_; y_idx++) {
                            Address addr(0, 0, 0, 0, MAP_GRF, 8+y_idx);
                            uint64_t hex_addr = ReverseAddressMapping(addr);
                            TryAddTransaction(hex_addr, true);
                        }
                    }

                    // Write input register
                    if (!is_register_reuse_ || (is_register_reuse_ && y != prev_y)) {
                        prev_y = y;
                        if (is_input_vector_) {
                            for (int x_idx = x % ri_; x_idx < x % ri_ + ki_; x_idx++) {
                                Address addr(0, 0, 0, 0, MAP_GRF, x_idx);
                                uint64_t hex_addr = ReverseAddressMapping(addr);
                                TryAddTransaction(hex_addr, true);
                            }
                        } else {
                            Address addr(0, 0, 0, 0, MAP_SRF, 0);
                            uint64_t hex_addr = ReverseAddressMapping(addr);
                            TryAddTransaction(hex_addr, true);
                        }
                    }

                    // Execute kernel
                    TryAddTransaction(hex_pim_op_addr, true);
                    Barrier();
                    if (is_output_stationary_) {
                        for (int ko = 0; ko < ko_; ko++) {
                            for (int ki = 0; ki < ki_; ki++) {
                                uint64_t offset = idx % (ri_*ro_);
                                uint64_t base = idx - offset;

                                uint64_t i = offset % ri_;
                                uint64_t o = offset / ri_;

                                uint64_t addr = (base + (o*ri_+i)) * SIZE_WORD;
                                TryAddTransaction(addr, false);
                                idx++;
                            }
                        }
                    } else {
                        for (int ki = 0; ki < ki_; ki++) {
                            for (int ko = 0; ko < ko_; ko++) {
                                uint64_t offset = idx % (ri_*ro_);
                                uint64_t base = idx - offset;

                                uint64_t i = offset % ro_;
                                uint64_t o = offset / ro_;

                                uint64_t addr = (base + (i*ro_+o)) * SIZE_WORD;
                                TryAddTransaction(addr, false);
                                idx++;
                            }
                        }
                    }
                    is_dirty = true;
                }
            }
        }    
    }
    // Read output register
    TryAddTransaction(hex_sbmr_addr, false);
    for (int y_idx = prev_y % ro_; y_idx < prev_y % ro_ + ko_; y_idx++) {
        Address addr(0, 0, 0, 0, MAP_GRF, 8+y_idx);
        uint64_t hex_addr = ReverseAddressMapping(addr);
        TryAddTransaction(hex_addr, false);
    }
    Barrier();
}


// Read PIM computation result from physical memory
void GemvTransactionGenerator::GetResult() {
    return;
}

// Calculate error between the result of PIM computation and actual answer
void GemvTransactionGenerator::CheckResult() {
    return;
}

}  // namespace dramsim3

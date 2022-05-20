#ifndef __TRANSACTION_GENERATOR_H
#define __TRANSACTION_GENERATOR_H

#include <sys/mman.h>
#include <time.h>
#include <stdlib.h>
#include <string>
#include <cstdint>
#include "./memory_system.h"
#include "./configuration.h"
#include "./common.h"
#include "./pim_config.h"

#define EVEN_BANK 0
#define ODD_BANK  1

#define NUM_WORD_PER_ROW     32
#define NUM_UNIT_PER_WORD    16
#define NUM_CHANNEL          16
#define NUM_BANK_PER_CHANNEL 16
#define NUM_BANK             (NUM_BANK_PER_CHANNEL * NUM_CHANNEL)
#define SIZE_WORD            32
#define SIZE_ROW             (SIZE_WORD * NUM_WORD_PER_ROW)

#define MAP_SBMR             0x3fff
#define MAP_ABMR             0x3ffe
#define MAP_PIM_OP_MODE      0x3ffd
#define MAP_CRF              0x3ffc
#define MAP_GRF              0x3ffb
#define MAP_SRF              0x3ffa

#define C_NORMAL "\033[0m"
#define C_RED    "\033[031m"
#define C_GREEN  "\033[032m"
#define C_YELLOW "\033[033m"
#define C_BLUE   "\033[034m"

namespace dramsim3 {

class TransactionGenerator {
 public:
    TransactionGenerator(const std::string& config_file,
                         const::string& output_dir)
        : memory_system_(
              config_file, output_dir,
              std::bind(&TransactionGenerator::ReadCallBack, this,
                        std::placeholders::_1),
              std::bind(&TransactionGenerator::WriteCallBack, this,
                        std::placeholders::_1)),
          config_(new Config(config_file, output_dir)),
          clk_(0) {

        start_clk_ = 0;
        cnt_ = 0;
        num_trans_ = 0;
    }
    ~TransactionGenerator() { delete(config_); }
    // virtual void ClockTick() = 0;
    virtual void Initialize() = 0;
    virtual void SetData() = 0;
    virtual void Execute() = 0;
    virtual void GetResult() = 0;
    virtual void CheckResult() = 0;

    void ReadCallBack(uint64_t addr);
    void WriteCallBack(uint64_t addr);
    void PrintStats() { memory_system_.PrintStats(); }
    uint64_t ReverseAddressMapping(Address& addr);
    uint64_t Ceiling(uint64_t num, uint64_t stride);
    uint64_t Flooring(uint64_t num, uint64_t stride);
    void TryAddTransaction(uint64_t hex_addr, bool is_write);
    void Barrier();
	uint64_t GetClk() { return clk_; }

    bool is_print_;
    uint64_t start_clk_;
    int cnt_;

 public:
    int num_trans_;
    MemorySystem memory_system_;
    const Config *config_;
    uint64_t clk_;
};

class GemvTransactionGenerator : public TransactionGenerator {
 public:
    GemvTransactionGenerator(const std::string& config_file,
                             const std::string& output_dir,
                             uint64_t x,
                             uint64_t y,
                             uint64_t xch,
                             uint64_t ych,
                             uint64_t xoo,
                             uint64_t yoo,
                             uint64_t xoi,
                             uint64_t yoi,
                             uint64_t ri,
                             uint64_t ro,
                             uint64_t ki,
                             uint64_t ko,
                             bool is_output_stationary,
                             bool is_input_vector,
                             bool is_register_reuse,
                             bool is_dataflow_opt)
        : TransactionGenerator(config_file, output_dir),
          x_(x), y_(y), xch_(xch), ych_(ych), xoo_(xoo), yoo_(yoo), xoi_(xoi), yoi_(yoi), ri_(ri), ro_(ro), ki_(ki), ko_(ko), is_output_stationary_(is_output_stationary), is_input_vector_(is_input_vector), is_register_reuse_(is_register_reuse), is_dataflow_opt_(is_dataflow_opt) {}
    void Initialize() override;
    void SetData() override;
    void Execute() override;
    void GetResult() override;
    void CheckResult() override;

 public:
    uint64_t x_, y_;
    uint64_t xch_, ych_;
    uint64_t xoo_, yoo_, xoi_, yoi_;
    uint64_t ri_, ro_;
    uint64_t ki_, ko_;
    bool is_output_stationary_;
    bool is_input_vector_;
    bool is_register_reuse_;
    bool is_dataflow_opt_;
};

}  // namespace dramsim3

#endif // __TRANSACTION_GENERATOR_H

#include <iostream>
#include <random>
#include "./../ext/headers/args.hxx"
#include "./transaction_generator.h"

using namespace dramsim3;

// main code to simulate PIM simulator
int main(int argc, const char **argv) {
    srand(time(NULL));
    // parse simulation settings
    args::ArgumentParser parser(
        "PIM-DRAM Simulator.",
        "Examples: \n."
        "./build/pimdramsim3main configs/DDR4_8Gb_x8_3200.ini -c 100 -t "
        "sample_trace.txt\n"
        "./build/pimdramsim3main configs/DDR4_8Gb_x8_3200.ini -s random -c 100");
    args::HelpFlag help(parser, "help", "Display the help menu", {'h', "help"});
    args::ValueFlag<std::string> output_dir_arg(
        parser, "output_dir", "Output directory for stats files",
        {'o', "output-dir"}, ".");
    args::ValueFlag<uint64_t> x_arg(
        parser, "x", "[GEMV] Number of rows of the matrix A",
        {"x"}, 4096);
    args::ValueFlag<uint64_t> y_arg(
        parser, "y", "[GEMV] Number of columns of the matrix A",
        {"y"}, 4096);
    args::ValueFlag<std::string> input_type_arg(
        parser, "input_type", "",
        {"input-type"}, "vector");
    args::ValueFlag<bool> register_reuse_arg(
        parser, "register_reuse", "",
        {"register-reuse"}, false);
    args::ValueFlag<uint64_t> ri_arg(
        parser, "ri", "",
        {"ri"}, 8);
    args::ValueFlag<uint64_t> ro_arg(
        parser, "ro", "",
        {"ro"}, 8);
    args::ValueFlag<uint64_t> ch_arg(
        parser, "ch", "",
        {"ch"}, 16);
    args::ValueFlag<uint64_t> yp_arg(
        parser, "yp", "",
        {"yp"}, 16);
    args::ValueFlag<bool> dataflow_opt_arg(
        parser, "dataflow_opt", "",
        {"dataflow-opt"}, false);
    args::Positional<std::string> config_arg(
        parser, "config", "The config file name (mandatory)");

    try {
        parser.ParseCLI(argc, argv);
    } catch (args::Help) {
        std::cout << parser;
        return 0;
    } catch (args::ParseError e) {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return 1;
    }

    std::string config_file = args::get(config_arg);
    if (config_file.empty()) {
        std::cerr << parser;
        return 1;
    }

    // Initialize modules of PIM-Simulator
    //  Transaction Generator + DRAMsim3 + PIM Functional Simulator
    //TransactionGenerator * tx_generator;

    std::string output_dir = args::get(output_dir_arg);
    uint64_t x = args::get(x_arg);
    uint64_t y = args::get(y_arg);
    std::string input_type = args::get(input_type_arg);
    bool is_register_reuse = args::get(register_reuse_arg);
    uint64_t ri = args::get(ri_arg);
    uint64_t ro = args::get(ro_arg);
    uint64_t ch = args::get(ch_arg);
    uint64_t yp = args::get(yp_arg);
    bool is_dataflow_opt = args::get(dataflow_opt_arg);

    uint64_t ci = 1;
    uint64_t co = 1;
    bool is_input_vector;
    if (input_type == "vector") {
        ci = 16;
        is_input_vector = true;
    } else {
        co = 16;
        is_input_vector = false;
    }

    std::cout << "xch ych xoo yoo xoi yoi ki ko num_trans cycles\n";

    for (uint64_t xch = 1; xch <= ch; xch *= 2) {
        uint64_t ych = ch / xch;
        for (uint64_t ki = 1; ki <= ri; ki *= 2) {
            uint64_t xi = ki * ci;
            for (uint64_t ko = 1; ko <= ro; ko *= 2) {
                if (is_dataflow_opt && ri * ko <= ro * ki)
                    continue;

                uint64_t yi = ko * co;

                uint64_t xo = x / xch / xi;
                uint64_t yo = y / ych / yi / yp;

                if (xo == 0 || yo == 0)
                    continue;

                for (uint64_t xoo = 1; xoo <= xo; xoo *= 2) {
                    uint64_t xoi = xo / xoo;
                    for (uint64_t yoo = 1; yoo <= yo; yoo *= 2) {
                        uint64_t yoi = yo / yoo;
                        /*
                        GemvTransactionGenerator *tx_generator;
                        tx_generator = new GemvTransactionGenerator(
                                config_file,
                                output_dir,
                                x, y, xch, ych, xoo, yoo, xoi, yoi,
                                ri, ro, ki, ko,
                                true,
                                is_input_vector,
                                is_register_reuse,
                                is_dataflow_opt
                                );
                        tx_generator->Execute();
                        std::cout << xch << " ";
                        std::cout << ych << " ";
                        std::cout << xoo << " ";
                        std::cout << yoo << " ";
                        std::cout << xoi << " ";
                        std::cout << yoi << " ";
                        std::cout << ki << " ";
                        std::cout << ko << " ";
                        std::cout << tx_generator->num_trans_ << " ";
                        std::cout << tx_generator->GetClk() << std::endl;
                        delete tx_generator;
                        */
                        std::cout << xch << " ";
                        std::cout << ych << " ";
                        std::cout << xoo << " ";
                        std::cout << yoo << " ";
                        std::cout << xoi << " ";
                        std::cout << yoi << " ";
                        std::cout << ki << " ";
                        std::cout << ko << " ";
                        GemvTransactionGenerator *tx_generator;
                        tx_generator = new GemvTransactionGenerator(
                                config_file,
                                output_dir,
                                x, y, xch, ych, xoo, yoo, xoi, yoi,
                                ri, ro, ki, ko,
                                true,
                                is_input_vector,
                                is_register_reuse,
                                false
                                );
                        tx_generator->Execute();
                        std::cout << tx_generator->GetClk() << " ";
                        tx_generator = new GemvTransactionGenerator(
                                config_file,
                                output_dir,
                                x, y, xch, ych, xoo, yoo, xoi, yoi,
                                ri, ro, ki, ko,
                                true,
                                is_input_vector,
                                is_register_reuse,
                                true
                                );
                        tx_generator->Execute();
                        std::cout << tx_generator->GetClk() << std::endl;
                        
                    }
                }
            }
        }
    }

    return 0;
}

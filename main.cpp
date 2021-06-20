#include "dpll.h"

#include <fstream>
#include <stdexcept>
#include <sstream>

int main(int argc, char **argv)
{
    std::ostringstream error_message;
    if (argc != 2)
    {
        error_message << "Usage: " << argv[0] << " <dimacs file>";
        throw std::runtime_error(error_message.str());
    }

    std::ifstream dimacsStream{argv[1]};
    if (!dimacsStream)
    {
        error_message << "Bad path to dimacs file: " << argv[1];
        throw std::runtime_error(error_message.str());
    }
    
    DPLL solver{dimacsStream};
    OptionalPartialValuation solution = solver.solve2wl();
    
    if (solution)
        std::cout << solution.value() << std::endl;
    else
        std::cout << "UNSAT" << std::endl;
    
    return 0;
}

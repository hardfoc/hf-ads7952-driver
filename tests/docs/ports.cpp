/**
 * @file ports.cpp
 * @brief Instantiates the driver over the documented port, so the whole API compiles.
 */
#include "ads7952.hpp"
#include "my_spi.hpp"

template class ads7952::ADS7952<MySpi>;

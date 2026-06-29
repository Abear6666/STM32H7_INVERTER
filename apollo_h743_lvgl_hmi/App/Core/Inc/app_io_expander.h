#ifndef APP_IO_EXPANDER_H
#define APP_IO_EXPANDER_H

#include <stdbool.h>

bool app_io_expander_init(void);
bool app_io_expander_set_rs485_tx(bool enable);

#endif

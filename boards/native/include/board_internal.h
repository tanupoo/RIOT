#ifdef MODULE_UART0
#include <sys/select.h>
void _native_handle_uart0_input(void);
/**
 * @brief: initialize uart0
 *
 * @param stdiotype: "stdio", "tcp", "udp" io redirection
 * #param stderrtype: "stdio" or "file"
 */
void _native_init_uart0(char *stdiotype, char *stderrtype, char *nullouttype, char *ioparam);
int _native_set_uart_fds(void);
#endif

void board_init(void);

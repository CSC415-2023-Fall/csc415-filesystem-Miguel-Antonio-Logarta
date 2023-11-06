#define DEBUG 1

// Debug statements that will only compile when the debug flag is on
#ifdef DEBUG
# define DEBUG_PRINT(x) printf x
#else
# define DEBUG_PRINT(x) do {} while (0)
#endif
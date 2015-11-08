#include "lp.h"
#include "lpregs.h"
#include "intrins.h"

#define SCL_BIT 1
#define SDA_BIT 2
#define SCL_PORT_O OEA
#define SDA_PORT_O OEA
#define SCL_PORT_I IOA_0
#define SDA_PORT_I IOA_1

#include "i2c_impl.h"
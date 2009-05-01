#ifndef NVM_H
#define NVM_H

#include <stdint.h>

typedef enum
{
	nvm_none,
	nvm_sst,
	nvm_st,
	nvm_atmel,
	nvm_type_max,
} nvm_type_t;


typedef enum
{
	nvm_err_none = 0,
	nvm_err_invalid_interface,
	nvm_err_invalid_type,
	nvm_err_invalid_pointer,
	nvm_err_write_protect,
	nvm_err_verify,
	nvm_err_overflow,
	nvm_err_blank_check,
	nvm_err_restricted,
	nvm_err_max,
} nvm_err_t;

typedef enum
{
	NVM_INTERFACE_INTERNAL,
	NVM_INTERFACE_EXTERNAL,
	nvm_interface_max,
} nvm_interface_t;

/* ROM code seems to be THUMB */
/* need to be in a THUMB block before calling them */
extern volatile nvm_err_t (*nvm_detect)(nvm_interface_t nvm_interface,nvm_type_t* pNvmType);
extern volatile nvm_err_t (*nvm_read)(nvm_interface_t nvm_interface , nvm_type_t nvmType , void *pDest, uint32_t address, uint32_t numBytes);
extern volatile void(*nvm_setsvar)(uint32_t locked);
#endif //NVM_H

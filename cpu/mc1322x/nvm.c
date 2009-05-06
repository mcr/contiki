#include "nvm.h"

volatile nvm_err_t (*nvm_detect)(nvm_interface_t nvm_interface, nvm_type_t* pNvmType) = (void *)0x00006cb9;
volatile nvm_err_t (*nvm_read)(nvm_interface_t nvm_interface, nvm_type_t nvmType , void *pDest, uint32_t address, uint32_t numBytes) = (void *)0x00006d69;
volatile nvm_err_t (*nvm_write)(nvm_interface_t nvmInterface, nvm_type_t nvmType ,void *pSrc, uint32_t address, uint32_t numBytes) = (void *)0x00006ec5;
volatile void(*nvm_setsvar)(uint32_t locked) = (void *)0x00007085;

/*
	GPIO driver for MC1322x
*/

#include "system.h"

// Control registers for MC1322x GPIOs
typedef struct {
	uint32_t GPIO_PAD_DIR0;
	uint32_t GPIO_PAD_DIR1;
	uint32_t GPIO_DATA0;
	uint32_t GPIO_DATA1;
	uint32_t GPIO_PAD_PU_EN0;
	uint32_t GPIO_PAD_PU_EN1;
	uint32_t GPIO_FUNC_SEL0;
	uint32_t GPIO_FUNC_SEL1;
	uint32_t GPIO_FUNC_SEL2;
	uint32_t GPIO_FUNC_SEL3;
	uint32_t GPIO_DATA_SEL0;
	uint32_t GPIO_DATA_SEL1;
	uint32_t GPIO_PAD_PU_SEL0;
	uint32_t GPIO_PAD_PU_SEL1;
	uint32_t GPIO_PAD_HYST_EN0;
	uint32_t GPIO_PAD_HYST_EN1;
	uint32_t GPIO_PAD_KEEP0;
	uint32_t GPIO_PAD_KEEP1;
	uint32_t GPIO_DATA_SET0;
	uint32_t GPIO_DATA_SET1;
	uint32_t GPIO_DATA_RESET0;
	uint32_t GPIO_DATA_RESET1;
	uint32_t GPIO_PAD_DIR_SET0;
	uint32_t GPIO_PAD_DIR_SET1;
	uint32_t GPIO_PAD_DIR_RESET0;
	uint32_t GPIO_PAD_DIR_RESET1;
} gpio_regs_t;

static volatile gpio_regs_t* const gpio_regs = GPIO_BASE;

// Set a range of pines as input according to mask
inline gpio_err_t gpio_set_port_dir_input(gpio_port_t port, uint32_t mask) {
	if(port == gpio_port_0)
		gpio_regs->GPIO_PAD_DIR_RESET0 = mask;
	else if(port == gpio_port_1)
		gpio_regs->GPIO_PAD_DIR_RESET1 = mask;
	else
		return gpio_invalid_parameter;
	return gpio_no_error;
}

// Set a range of pines as output according to mask
inline gpio_err_t gpio_set_port_dir_output(gpio_port_t port, uint32_t mask) {
	if(port == gpio_port_0)
		gpio_regs->GPIO_PAD_DIR_SET0 = mask;
	else if(port == gpio_port_1)
		gpio_regs->GPIO_PAD_DIR_SET1 = mask;
	else
		return gpio_invalid_parameter;
	return gpio_no_error;
}

// Set a target pin as input
inline gpio_err_t gpio_set_pin_dir_input(gpio_pin_t pin) {
	if(pin < gpio_pin_0 && pin > gpio_pin_63)
		return gpio_invalid_parameter;
	else if(pin < gpio_pin_32)
		gpio_regs->GPIO_PAD_DIR_RESET0 = (1 << pin);
	else
		gpio_regs->GPIO_PAD_DIR_RESET1 = (1 << (pin - gpio_pin_32));
	return gpio_no_error;
}

// Set a target pin as output
inline gpio_err_t gpio_set_pin_dir_output(gpio_pin_t pin) {
	if(pin < gpio_pin_0 && pin > gpio_pin_63)
		return gpio_invalid_parameter;
	else if(pin < gpio_pin_32)
		gpio_regs->GPIO_PAD_DIR_SET0 = (1 << pin);
	else
		gpio_regs->GPIO_PAD_DIR_SET1 = (1 << (pin - gpio_pin_32));
	return gpio_no_error;
}

// Set a range of pines to a mask
inline gpio_err_t gpio_set_port(gpio_port_t port, uint32_t mask) {
	if(port == gpio_port_0)
		gpio_regs->GPIO_DATA_SET0 = mask;
	else if(port == gpio_port_1)
		gpio_regs->GPIO_DATA_SET1 = mask;
	else
		return gpio_invalid_parameter;
	return gpio_no_error;
}


// Clear a range of pines
inline gpio_err_t gpio_clear_port (gpio_port_t port, uint32_t mask) {
	if(port == gpio_port_0)
		gpio_regs->GPIO_DATA_RESET0 = mask;
	else if(port == gpio_port_1)
		gpio_regs->GPIO_DATA_RESET1 = mask;
	else
		return gpio_invalid_parameter;

	return gpio_no_error;
}

// Activate an specific pin
inline gpio_err_t gpio_set_pin (gpio_pin_t pin) {
	if(pin < gpio_pin_0 && pin > gpio_pin_63)
		return gpio_invalid_parameter;
	else if(pin < gpio_pin_32)
		gpio_regs->GPIO_DATA_SET0 = (1 << pin);
	else
		gpio_regs->GPIO_DATA_SET1 = (1 << (pin - gpio_pin_32));
	return gpio_no_error;
}

// Disable an specific pin
inline gpio_err_t gpio_clear_pin (gpio_pin_t pin) {
	if(pin < gpio_pin_0 && pin > gpio_pin_63)
		return gpio_invalid_parameter;
	else if(pin < gpio_pin_32)
		gpio_regs->GPIO_DATA_RESET0 = (1 << pin);
	else
		gpio_regs->GPIO_DATA_RESET1 = (1 << (pin - gpio_pin_32));
	return gpio_no_error;
}

// Read a range of pines
inline gpio_err_t gpio_get_port (gpio_port_t port, uint32_t *port_data) {
	if(port == gpio_port_0)
		*port_data=gpio_regs->GPIO_DATA0;
	else if(port == gpio_port_1)
		*port_data=gpio_regs->GPIO_DATA1;
	else
		return gpio_invalid_parameter;

	return gpio_no_error;
}

// Get an specific pin value
inline gpio_err_t gpio_get_pin (gpio_pin_t pin, uint32_t *pin_data) {
	if(*pin_data < gpio_pin_0 && *pin_data > gpio_pin_63)
		return gpio_invalid_parameter;
	else if(*pin_data < gpio_pin_32){
		*pin_data=(gpio_regs->GPIO_DATA0) & (1 << pin);
	}
	else{
		*pin_data=(gpio_regs->GPIO_DATA1) & (1 << (pin - gpio_pin_32));
	}
	return gpio_no_error;
}

// Configure a function for a range of pines
inline gpio_err_t gpio_set_port_func (gpio_port_t port, gpio_func_t func, uint32_t mask) {
	uint32_t function;

	if(func == gpio_func_normal)
		function = (uint32_t) 0x00000000;
	else if(func == gpio_func_alternate_1)
	function = (uint32_t) 0x00000001;
	else if(func == gpio_func_alternate_2)
	function = (uint32_t) 0x00000002;
	else if(func == gpio_func_alternate_3)
	function = (uint32_t) 0x00000003;
	else
		return gpio_invalid_parameter;


		if(port == gpio_port_0){
			uint32_t i = 0;
			uint32_t j = 0;

			for(;i < gpio_pin_16;i++,j++)
				if(mask & (1 << i))
					gpio_regs->GPIO_FUNC_SEL0 = (gpio_regs->GPIO_FUNC_SEL0 & ~(3 << j*2)) | (function << j*2);

			for(j=0;i < gpio_pin_32;i++,j++)
				if(mask & (1 << i))
					gpio_regs->GPIO_FUNC_SEL1 = (gpio_regs->GPIO_FUNC_SEL1 & ~(3 << j*2)) | (function << j*2);

		}
		else if(port == gpio_port_1){
			uint32_t i = 0;
			uint32_t j = 0;

			for(;i < gpio_pin_16;i++,j++)
				if(mask & (1 << i))
					gpio_regs->GPIO_FUNC_SEL2 = (gpio_regs->GPIO_FUNC_SEL2 & ~(3 << j*2)) | (function << j*2);

			for(j=0;i < gpio_pin_32;i++,j++)
				if(mask & (1 << i))
					gpio_regs->GPIO_FUNC_SEL3 = (gpio_regs->GPIO_FUNC_SEL3 & ~(3 << j*2)) | (function << j*2);
		}
		else
			return gpio_invalid_parameter;

	return gpio_no_error;
}

// Configure a function for a specific pin
inline gpio_err_t gpio_set_pin_func (gpio_pin_t pin, gpio_func_t func) {
	uint32_t function;

	if(func == gpio_func_normal)
		function = (uint32_t) 0x00000000;
	else if(func == gpio_func_alternate_1)
	function = (uint32_t) 0x00000001;
	else if(func == gpio_func_alternate_2)
	function = (uint32_t) 0x00000002;
	else if(func == gpio_func_alternate_3)
	function = (uint32_t) 0x00000003;
	else
		return gpio_invalid_parameter;


		if(pin < gpio_pin_0 && pin > gpio_pin_63)
			return gpio_invalid_parameter;
		else if(pin < gpio_pin_16){
			gpio_regs->GPIO_FUNC_SEL0 = (gpio_regs->GPIO_FUNC_SEL0 & ~(3 << pin*2)) | (function << pin*2);
		}
		else if(pin < gpio_pin_32){
			gpio_regs->GPIO_FUNC_SEL1 = (gpio_regs->GPIO_FUNC_SEL1 & ~(3 << (pin-16)*2)) | (function << (pin-16)*2);
		}
		else if(pin < gpio_pin_48){
			gpio_regs->GPIO_FUNC_SEL2 = (gpio_regs->GPIO_FUNC_SEL2 & ~(3 << (pin-32)*2)) | (function << (pin-32)*2);
		}
		else if(pin < gpio_pin_max){
			gpio_regs->GPIO_FUNC_SEL3 = (gpio_regs->GPIO_FUNC_SEL3 & ~(3 << (pin-48)*2)) | (function << (pin-48)*2);
		}
		else
			return gpio_invalid_parameter;

		return gpio_no_error;
}

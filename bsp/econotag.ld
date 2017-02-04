/*
 * Sistemas Empotrados
 * Linker script para la Redwire EconoTAG
 * Runtime de C para ser cargado por la BIOS de la placa
 */

/*
 * Punto de entrada
 */
ENTRY(_vector_table)

/*
 * Mapa de memoria de la placa
 */
MEMORY
{
        ram   : org = 0x00400000,       len = 0x00018000        /*  96 KB */
}

SECTIONS
{
/*Reservamos espacio para la tabla de vectores
y la tabla de manejadores*/
/*.vectors : {
		. += 0x20 ;
		_excep_handlers = . ;
		. += 0x20 ;
	} > ram*/

	/* Imagen del firmware */
	/* Generar una sección al principio de la RAM que organice las secciones del firmware al comienzo de la RAM de la plataforma */
.imagen : ALIGN(4)
{
/*Los cuatro primeros bytes de la imagen deben usarse para indicar su tamaño, ya que el
 bootloader de la ROM leerá el contenido de estos cuatro bytes para saber cuántos bytes
 tiene que copiar desde el origen de la imagen hacia la RAM.(Con ALIGN(4))*/
  *(.startup);/*Codigo del cargador(se encuentra en crt0)*/
  *(.text);/*Codigo de la aplicacion*/
  *(.rodata*);/*Constantes globales*/
  . = ALIGN(4) ;
  *(.data);/*Valores iniciales de las variables globales*/
  . = ALIGN(4) ;



} > ram
	/* Sección .bss */
        /* Generamos una sección para las variables globales sin inicializar */
.bss :
{

  _bss_start = . ;
  *(.bss);/*variables globales inicializadas a 0*/
  . = ALIGN(4) ;
  *(COMMON);/*variables globales sin inicializar*/
  . = ALIGN(4) ;
  _bss_end = . ;

} > ram
        /* Gestión de las pilas */
	/* Generar una sección al final de la RAM para las pilas de cada modo y definir símbolos para el tope de cada pila */
  _ram_limit = ORIGIN(ram) + LENGTH(ram);/*Dejamos una zona al final de la RAM para alojar la pila*/
        _sys_stack_size = 1024 ;
        _irq_stack_size = 256 ;
        _fiq_stack_size = 256 ;
        _svc_stack_size = 256 ;
        _abt_stack_size = 16 ;
        _und_stack_size = 16 ;
        _stacks_size = _stacks_top - _stacks_bottom ;

        .stacks _ram_limit - _stacks_size :
        {
            _stacks_bottom = . ;/*Final de la pila*/
            . += _sys_stack_size ;
            _sys_stack_top = . ;/*Tope de pila para modo System/User */
            . += _svc_stack_size ;
            _svc_stack_top = . ;/*Tope de pila para modo Supervisor */
            . += _abt_stack_size ;
            _abt_stack_top = . ;/*Tope de pila para modo Abort */
            . += _und_stack_size ;
            _und_stack_top = . ;/*Tope de pila para modo Undefined */
            . += _irq_stack_size ;
            _irq_stack_top = . ;/*Tope de pila para modo IRQ */
            . += _fiq_stack_size ;
            _fiq_stack_top = . ;/*Tope de pila para modo FIQ */
            _stacks_top = . ;/*Tope de pila */
        }

 	/* Gestión del heap */
	/* Generar una sección que ocupe el espacio entre la sección .bss y las pilas para el heap, con los símbolos de inicio y fin del heap */
  _heap_size = _stacks_bottom - _bss_end ;
       .heap _bss_end :
       {
           _heap_start = . ;
           . += _heap_size;
           _heap_end = . ;
       }
}

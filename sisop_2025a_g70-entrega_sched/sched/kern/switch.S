/*
 * Your code here for context_switch
 *
 * It should restore the register values in the Trapframe with the 'iret' instruction.
 * This exits the kernel and starts executing some environment's code.
 *
 * This function does not return.
 */

.globl context_switch;
context_switch:
	movl 4(%esp), %esp		#Cambio de puntero del Stack, apunta a la primera posición del trapframe pasado por parámetro
	popal					#Descargar toda la información para los registros de propósito general
	popl %es				#Descargar información desde stack al registro tf_ds (inlcuyendo el tf_padding1)
	popl %ds				#Descargar información desde stack al registro tf_ds (inlcuyendo el tf_padding2)
	addl $0x8, %esp			#Descartar información campos tf_trapno y tf_err

	
	iret					#Retorno
spin:
	jmp spin

/*
*El manejo de los registros (push, pop, pusha, popa) se dará con "l" ya que indica que se estará dando un manejo de 32 bits
*/